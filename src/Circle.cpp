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

struct Circle : Module {

	const static int NUM_PITCHES = 6;
	
	int MAJOR[6] = {0,2,4,-1,-1,-1};
	
	

	enum ParamIds {
		KEY_PARAM,
		SCALE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ROTL_INPUT,
		ROTR_INPUT,
		KEY_INPUT,
		SCALE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		KEY_OUTPUT,
		SCALE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		SCALE_LIGHT,
		BKEY_LIGHT,
		CKEY_LIGHT = BKEY_LIGHT + 12,
		NUM_LIGHTS = CKEY_LIGHT + 12
	};
	
	Circle() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	
	void setQuantizer(Quantizer &quant);	
	Quantizer q;
	Chord chords;
	
	SchmittTrigger rotLTrigger;
	SchmittTrigger rotRTrigger;
	
	PulseGenerator stepPulse;
	
	float outVolts[NUM_PITCHES];
	
	int baseKeyIndex = 0;
	int curKeyIndex = 0;
	
	int curScale = 0;
		
	int stepX = 0;
	int poll = 5000;
	
	inline bool debug() {
		if (stepX % poll == 0) {
			return false;			
		}
		return false;
	}
	
};

void Circle::setQuantizer(Quantizer &quantizer) {
	q = quantizer;
}

void Circle::step() {
	
	stepX++;

	// Get inputs from Rack
	float rotLInput		= inputs[ROTL_INPUT].value;
	float rotRInput		= inputs[ROTR_INPUT].value;
	
	int newKeyIndex = 0;
	if (inputs[KEY_INPUT].active) {
		float fRoot = inputs[KEY_INPUT].value;
		newKeyIndex = q.getKeyFromVolts(fRoot);
	} else {
		newKeyIndex = params[KEY_PARAM].value;
	}

	int newScale = 0;
	if (inputs[SCALE_INPUT].active) {
		float fScale = inputs[SCALE_INPUT].value;
		newScale = round(rescalef(fabs(fScale), 0.0, 10.0, 0.0, 1.0)); // Will switch to a switch
	} else {
		newScale = params[SCALE_PARAM].value;
	}

	if (newScale == 0) { // Map to scales in Core.cpp
		curScale = Quantizer::IONIAN;
	} else {
		curScale = Quantizer::AEOLIAN;
	}
		
	// Process inputs
	bool rotLStatus		= rotLTrigger.process(rotLInput);
	bool rotRStatus		= rotRTrigger.process(rotRInput);
		
		
	if (rotLStatus) {
		if (debug()) { std::cout << stepX << " Rotate left: " << curKeyIndex; }
		if (curKeyIndex == 0) {
			curKeyIndex = 11;
		} else {
			curKeyIndex--;
		}
		if (debug()) { std::cout << " -> " << curKeyIndex << std::endl;	}
	} 
	
	if (rotRStatus) {
		if (debug()) { std::cout << stepX << " Rotate right: " << curKeyIndex; }
		if (curKeyIndex == 11) {
			curKeyIndex = 0;
		} else {
			curKeyIndex++;
		}
		if (debug()) { std::cout << " -> " << curKeyIndex << std::endl;	}
	} 
	
	if (rotLStatus && rotRStatus) {
		if (debug()) { std::cout << stepX << " Reset " << curKeyIndex << std::endl;	}
		curKeyIndex = baseKeyIndex;
	}
	
	
	if (newKeyIndex != baseKeyIndex) {
		if (debug()) { std::cout << stepX << " New base: " << newKeyIndex << std::endl;}
		baseKeyIndex = newKeyIndex;
		curKeyIndex = newKeyIndex;
	}
	
	
	int curKey = q.CIRCLE_FIFTHS[curKeyIndex];
	int baseKey = q.CIRCLE_FIFTHS[baseKeyIndex];
	
	float keyVolts = q.getVoltsFromKey(curKey);
	float scaleVolts = q.getVoltsFromScale(curScale);
	
	if (debug()) { std::cout << stepX << " Out: " << curKey << " " << curScale << std::endl;	}
			
	for (int i = 0; i < Quantizer::NUM_NOTES; i++) {
		lights[CKEY_LIGHT + i].value = 0.0;
		lights[BKEY_LIGHT + i].value = 0.0;
	}

	lights[CKEY_LIGHT + curKey].value = 1.0;
	lights[BKEY_LIGHT + baseKey].value = 1.0;
	if (curScale == 1) {
		lights[SCALE_LIGHT].value = 1.0;		
	} else {
		lights[SCALE_LIGHT].value = 0.0;		
	}
	
	outputs[KEY_OUTPUT].value = keyVolts;
	outputs[SCALE_OUTPUT].value = scaleVolts;
	
}


CircleWidget::CircleWidget() {
	
	Circle *module = new Circle();
	setModule(module);
	
	Quantizer quant = Quantizer();
	module->setQuantizer(quant);
	
	box.size = Vec(240, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Circle.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

	addInput(createInput<PJ301MPort>(Vec(20.0, 329), module, Circle::ROTL_INPUT));
	addInput(createInput<PJ301MPort>(Vec(55.0, 329), module, Circle::ROTR_INPUT));
	addInput(createInput<PJ301MPort>(Vec(90.0, 329), module, Circle::KEY_INPUT));
	addParam(createParam<AHKnob>(Vec(125.0, 329), module, Circle::KEY_PARAM, 0.0, 11.0, 0.0)); 
	addInput(createInput<PJ301MPort>(Vec(160.0, 329), module, Circle::SCALE_INPUT));
	addParam(createParam<AHKnob>(Vec(195.0, 329), module, Circle::SCALE_PARAM, 0.0, 1.0, 0.0)); 

	float xOffset = 18.0;
	float xSpace = 21.0;
	float xPos = 0.0;
	float yPos = 0.0;
	int scale = 0;

	for (int i = 0; i < 12; i++) {
		quant.calculateKey(i, xSpace, xOffset, 230.0, &xPos, &yPos, &scale);
		addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, Circle::CKEY_LIGHT + scale));

		quant.calculateKey(i, xSpace, xOffset + 72.0, 165.0, &xPos, &yPos, &scale);
		addChild(createLight<SmallLight<RedLight>>(Vec(xPos, yPos), module, Circle::BKEY_LIGHT + scale));
	}

	addOutput(createOutput<PJ301MPort>(Vec(11.5, 49.0),  module, Circle::KEY_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(59.5, 49.0), module, Circle::SCALE_OUTPUT));
	addChild(createLight<MediumLight<GreenLight>>(Vec(114.9, 56.4), module, Circle::SCALE_LIGHT));

}



