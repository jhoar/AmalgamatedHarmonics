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
	
	int MAJOR[6] = {0,2,4,-1,-1,-1};
	
	

	enum ParamIds {
		KEY_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		STEP_INPUT,
		KEY_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		PITCH_OUTPUT,
		NUM_OUTPUTS = PITCH_OUTPUT + NUM_PITCHES
	};
	enum LightIds {
		STEP_LIGHT,
		NUM_LIGHTS
	};
	
	Progress() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	
	void setQuantizer(Quantizer &quant);	
	Quantizer q;
	Chord chords;
	
	SchmittTrigger stepTrigger; 
	
	PulseGenerator stepPulse;
	
	float outVolts[NUM_PITCHES];
	
	
	int pIndex = 0;
	int cIndex = 0;
	int kIndex = 0;

	int prg_stp    = 3;
	int prg_key[6] = {Quantizer::NOTE_C, Quantizer::NOTE_C + 4, Quantizer::NOTE_C + 5, -1, -1, -1};
	int prg_crd[6] = {Chord::MAJOR, Chord::MAJOR, Chord::MAJOR, -1, -1, -1};
	
	int stepX = 0;
	int poll = 5000;
	
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
	
	
	// Process inputs
	bool stepStatus		= stepTrigger.process(stepInput);

	
	if (stepStatus) {
		stepPulse.trigger(5e-5);
		pIndex = (pIndex + 1) % 6;
		if(pIndex == prg_stp) {
			pIndex = 0;
		}
		cIndex = prg_crd[pIndex];
		kIndex = prg_key[pIndex];
		
		if (debug()) { std::cout << stepX << " New chord: pIndex: " << pIndex << " chord: " << q.noteNames[kIndex] << " " << chords.ChordNames[cIndex] << std::endl; }
	}
	
	
	int *chordArray;
	switch (cIndex){
		case Chord::MAJOR:		chordArray = chords.CHORD_MAJOR; break;
		case Chord::MINOR:		chordArray = chords.CHORD_MINOR; break;
		default: 				chordArray = chords.CHORD_MAJOR;
	}
		
	for (int i = 0; i < NUM_PITCHES; i++) {
		outVolts[i] = q.getVoltsFromPitch(chordArray[i],kIndex);
	}
	
	bool stepped = stepPulse.process(delta);	
	
	for (int i = 0; i < NUM_PITCHES; i++) {
		outputs[PITCH_OUTPUT + i].value = outVolts[i];
	}
	lights[STEP_LIGHT].value = stepped ? 1.0 : 0.0;
	
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
		addOutput(createOutput<PJ301MPort>(Vec(xPos + connDelta, 33.0 + 16.0),  module, Progress::PITCH_OUTPUT + i));
	}
	
	addInput(createInput<PJ301MPort>(Vec(15.0  + connDelta, 329), module, Progress::STEP_INPUT));

	float xOffset = 18.0;
	float xSpace = 21.0;
	float xPos = 0.0;
	float yPos = 0.0;
	int scale = 0;
 

}



