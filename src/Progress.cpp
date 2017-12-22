#include "AH.hpp"
#include "Core.hpp"
#include "components.hpp"
#include "dsp/digital.hpp"

#include <iostream>

struct AHButton : SVGSwitch, MomentarySwitch {
	AHButton() {
		addFrame(SVG::load(assetPlugin(plugin,"res/ComponentLibrary/AHButton.svg")));
	}
};

struct AHKnob : RoundKnob {
	AHKnob() {
		snap = true;
		setSVG(SVG::load(assetPlugin(plugin,"res/ComponentLibrary/AHKnob.svg")));
	}
};

struct Progress : Module {

	const static int NUM_PITCHES = 6;
	
	enum ParamIds {
		KEY_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		STEP_INPUT,
		KEY_INPUT,
		MODE_INPUT,
		PATT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		PITCH_OUTPUT,
		NUM_OUTPUTS = PITCH_OUTPUT + NUM_PITCHES
	};
	enum LightIds {
		NUM_LIGHTS
	};
	
	Progress() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	
	void setQuantizer(Quantizer &quant);	
	Quantizer q;
	Chord chords;
	
	SchmittTrigger stepTrigger; 
	SchmittTrigger pattTrigger; 
	
	PulseGenerator stepPulse;
	PulseGenerator pattPulse;
	
	int currMode = 0;
	bool haveMode = false;
	
	int currRoot = 0;
	bool haveRoot = false;
	
	int stepIndex = 0;
	int offset = 24; // Repeated notes in chord and expressed in the chord definition as being transposed 2 octaves lower. When played this offset needs to be removed (or the notes removed, or the notes transposed to an octave higher)
	
	int stepX = 0;
	int poll = 5000;
	
	float pitches[6];
	
	int currPattern = 0;
	bool switchPatt = false;
	bool firstStep = true;
	
	int pattRoot[2][8] = {
		{Quantizer::NOTE_B,Quantizer::NOTE_G,Quantizer::NOTE_D,Quantizer::NOTE_A,Quantizer::NOTE_B,Quantizer::NOTE_G,Quantizer::NOTE_D,Quantizer::NOTE_A},
		{Quantizer::NOTE_E,Quantizer::NOTE_A,Quantizer::NOTE_D,Quantizer::NOTE_C,Quantizer::NOTE_E,Quantizer::NOTE_A,Quantizer::NOTE_D,Quantizer::NOTE_F}
	};
	
	int pattChr[2][8] = {
		{71,1,1,1,71,1,1,1},
		{71,1,1,1,71,1,1,1}
	};
	
	int pattInv[2][8] = {
		{0,0,0,0,0,1,2,0},
		{0,0,0,0,0,0,0,0}
	};
	
	inline bool debug() {
		return true;
	}
	
};

void Progress::setQuantizer(Quantizer &quantizer) {
	q = quantizer;
}

void Progress::step() {
	
	stepX++;
	
	// Wait a few steps for the inputs to flow through Rack
	if (stepX < 10) { 
		return;
	}
	
	// Get the clock rate and semi-tone
	float delta = 1.0 / engineGetSampleRate();	
	
	// Get inputs from Rack
	float stepInput		= inputs[STEP_INPUT].value;
	float pattInput		= inputs[PATT_INPUT].value;
	
	if (inputs[KEY_INPUT].active) {
		haveRoot = true;
		float fRoot = inputs[KEY_INPUT].value;
		currRoot = q.getKeyFromVolts(fRoot);
	} else {
		haveRoot = false;
		currRoot = params[KEY_PARAM].value;
	}

	if (inputs[MODE_INPUT].active) {
		haveMode = true;
		float fMode = inputs[MODE_INPUT].value;
		currMode = q.getScaleFromVolts(fMode);
		
	} else {
		haveMode = false;
		currMode = params[MODE_PARAM].value;
	}

	bool pattStatus = pattTrigger.process(pattInput);
	if (pattStatus) {
		if (debug()) { std::cout << stepX << " Triggered pattern switch" << std::endl; }
		pattPulse.trigger(5e-5);
		switchPatt = true; // Only switch patterns on a STEP
	}

	// Process inputs
	bool stepStatus = stepTrigger.process(stepInput);
	if (stepStatus) {
		if (debug()) { std::cout << stepX << " Advance progression" << std::endl; }		
		stepPulse.trigger(5e-5);
		stepIndex++;
		
		if (stepIndex == 8) {
			if (debug()) { std::cout << stepX << " Loop progression" << std::endl; }
			stepIndex = 0; // loop
		}
		
		// If we have flag a pattern switch, do it now.
		if (switchPatt) {
			if (debug()) { std::cout << stepX << " Switching patterns, reset progression" << std::endl; }
			currPattern++;
			stepIndex = 0; // Start at beginning of progression
			switchPatt = false;
			
			if (currPattern == 2) {
				if (debug()) { std::cout << stepX << " Loop pattern" << std::endl; }
				currPattern = 0; // loop
			}
		}
	}
	
	if (firstStep || stepStatus) { // Set chords. Some this should go into an initialise function
		
		int currInv 	= pattInv[currPattern][stepIndex];
		int currCrd 	= pattChr[currPattern][stepIndex];
		int currRoot 	= pattRoot[currPattern][stepIndex];
		
		int *chordArray;
	
		switch(currInv) {
			case 0:  chordArray = chords.Chords[currCrd].root; break;
			case 1:  chordArray = chords.Chords[currCrd].first; break;
			case 2:  chordArray = chords.Chords[currCrd].second; break;
			default: chordArray = chords.Chords[currCrd].root;
		}	

		if (debug()) { std::cout << stepX  << " Chord: " << q.noteNames[currRoot] << chords.Chords[currCrd].quality << " (" << currInv << ")" << std::endl; }
		 	
		for (int i = 0; i < NUM_PITCHES; i++) {
		
			int note = chordArray[i];

			if (chordArray[i] < 0) {
				note += offset;
			}
	
			pitches[i] = q.getVoltsFromPitch(note,currRoot);
		
		}
		
	}
	
	for (int i = 0; i < NUM_PITCHES; i++) {
		outputs[PITCH_OUTPUT + i].value = pitches[i];
	}
		
	if (firstStep) {
		firstStep = false;
	}
	
}

struct ProgressDisplay : TransparentWidget {
	
	Progress *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	ProgressDisplay() {
		font = Font::load(assetPlugin(plugin, "res/Roboto-Light.ttf"));
	}

	void draw(NVGcontext *vg) override {
	
		Vec pos = Vec(0, 20);
		
		nvgFontSize(vg, 20);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -1);
		
		nvgFillColor(vg, nvgRGBA(212, 175, 55, 0xff));
		
	}
	
};

ProgressWidget::ProgressWidget() {
	
	Progress *module = new Progress();
	setModule(module);
	
	Quantizer quant = Quantizer();
	module->setQuantizer(quant);
	
	box.size = Vec(240, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Progress.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

	{
		ProgressDisplay *display = new ProgressDisplay();
		display->module = module;
		display->box.pos = Vec(10, 95);
		display->box.size = Vec(100, 140);
		addChild(display);
	}

	float xStart = 5.0;
	float boxWidth = 35.0;
	float gap = 10.0;
	float connDelta = 5.0;
		
	for (int i = 0; i < Progress::NUM_PITCHES; i++) {
		float xPos = xStart + ((float)i * boxWidth + gap);
		addOutput(createOutput<PJ301MPort>(Vec(xPos + connDelta, 49.0),  module, Progress::PITCH_OUTPUT + i));
	}
	
	addInput(createInput<PJ301MPort>(Vec(20.0, 329), module, Progress::STEP_INPUT));
	addInput(createInput<PJ301MPort>(Vec(55.0, 329), module, Progress::KEY_INPUT));
	addParam(createParam<AHKnob>(Vec(90.0, 329), module, Progress::KEY_PARAM, 0.0, 11.0, 0.0)); // 12 notes
	addInput(createInput<PJ301MPort>(Vec(125.0, 329), module, Progress::MODE_INPUT));
	addParam(createParam<AHKnob>(Vec(160.0, 329), module, Progress::MODE_PARAM, 0.0, 6.0, 0.0)); // 12 notes
	addInput(createInput<PJ301MPort>(Vec(195.0, 329), module, Progress::PATT_INPUT));
	

}



