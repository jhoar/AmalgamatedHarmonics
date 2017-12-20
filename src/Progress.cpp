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
	
	int currScale = 0;
	int currRoot = 0;
	
	int pIndex = 1;
	int inversion = 0;
	
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
	
	if (inputs[KEY_INPUT].active) {
		float fRoot = inputs[KEY_INPUT].value;
		currRoot = q.getKeyFromVolts(fRoot);
	} else {
		currRoot = params[KEY_PARAM].value;
	}
	
	// Process inputs
	bool stepStatus		= stepTrigger.process(stepInput);
	
	if (stepStatus) {
		stepPulse.trigger(5e-5);
		
		pIndex++;
		
		if (pIndex == Chord::NUM_CHORDS) {
			pIndex = 0;
		}

		inversion = rand() % 3;

		std::cout << "Chord: " << q.noteNames[currRoot] << chords.Chords[pIndex].quality << inversion << std::endl;
		
	}
	
	int *chordArray;
	
	switch(inversion) {
		case 0:  chordArray = chords.Chords[pIndex].root; break;
		case 1:  chordArray = chords.Chords[pIndex].first; break;
		case 2:  chordArray = chords.Chords[pIndex].second; break;
		default: chordArray = chords.Chords[pIndex].root;
	}	
		 
	int offset = 36; // Repeated notes in chord in octave above
	
	for (int i = 0; i < NUM_PITCHES; i++) {
		
		int note = chordArray[i];

		if (chordArray[i] < 0) {
			note += offset;
		}
	
		outputs[PITCH_OUTPUT + i].value = q.getVoltsFromPitch(note,currRoot);
		
	}
	
	bool stepped = stepPulse.process(delta);	
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
		addOutput(createOutput<PJ301MPort>(Vec(xPos + connDelta, 49.0),  module, Progress::PITCH_OUTPUT + i));
	}
	
	addInput(createInput<PJ301MPort>(Vec(20.0, 329), module, Progress::STEP_INPUT));
	addInput(createInput<PJ301MPort>(Vec(55.0, 329), module, Progress::KEY_INPUT));
	addParam(createParam<AHKnob>(Vec(90.0, 329), module, Progress::KEY_PARAM, 0.0, 11.0, 0.0)); // 12 notes
	

}



