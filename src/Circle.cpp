#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"
#include "dsp/digital.hpp"

#include <iostream>

struct Circle : Module {

	const static int NUM_PITCHES = 6;
	
	enum ParamIds {
		KEY_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ROTL_INPUT,
		ROTR_INPUT,
		KEY_INPUT,
		MODE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		KEY_OUTPUT,
		MODE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		MODE_LIGHT,
		BKEY_LIGHT = MODE_LIGHT + 7,
		CKEY_LIGHT = BKEY_LIGHT + 12, 
		NUM_LIGHTS = CKEY_LIGHT + 12
	};
	
	Circle() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	
	SchmittTrigger rotLTrigger;
	SchmittTrigger rotRTrigger;
	
	PulseGenerator stepPulse;
	
	float outVolts[NUM_PITCHES];
	
	int baseKeyIndex = 0;
	int curKeyIndex = 0;
	
	int curMode = 0;
		
	int stepX = 0;
	int poll = 5000;
	
	inline bool debug() {
		if (stepX % poll == 0) {
			return false;			
		}
		return false;
	}
	
};

void Circle::step() {
	
	stepX++;

	// Get inputs from Rack
	float rotLInput		= inputs[ROTL_INPUT].value;
	float rotRInput		= inputs[ROTR_INPUT].value;
	
	int newKeyIndex = 0;
	if (inputs[KEY_INPUT].active) {
		float fRoot = inputs[KEY_INPUT].value;
		newKeyIndex = CoreUtil().getKeyFromVolts(fRoot);
	} else {
		newKeyIndex = params[KEY_PARAM].value;
	}

	int newMode = 0;
	if (inputs[MODE_INPUT].active) {
		float fMode = inputs[MODE_INPUT].value;
		newMode = round(rescalef(fabs(fMode), 0.0, 10.0, 0.0, 6.0)); 
	} else {
		newMode = params[MODE_PARAM].value;
	}

	curMode = newMode;
		
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
	
	
	int curKey = CoreUtil().CIRCLE_FIFTHS[curKeyIndex];
	int baseKey = CoreUtil().CIRCLE_FIFTHS[baseKeyIndex];
	
	float keyVolts = CoreUtil().getVoltsFromKey(curKey);
	float modeVolts = CoreUtil().getVoltsFromMode(curMode);
	
	if (debug()) { std::cout << stepX << " Out: " << curKey << " " << curMode << std::endl;	}
			
	for (int i = 0; i < Core::NUM_NOTES; i++) {
		lights[CKEY_LIGHT + i].value = 0.0;
		lights[BKEY_LIGHT + i].value = 0.0;
	}

	lights[CKEY_LIGHT + curKey].value = 1.0;
	lights[BKEY_LIGHT + baseKey].value = 1.0;
	
	for (int i = 0; i < Core::NUM_MODES; i++) {
		lights[MODE_LIGHT + i].value = 0.0;
	}
	lights[MODE_LIGHT + curMode].value = 1.0;
		
	outputs[KEY_OUTPUT].value = keyVolts;
	outputs[MODE_OUTPUT].value = modeVolts;
	
}


CircleWidget::CircleWidget() {
	
	Circle *module = new Circle();
	setModule(module);
	
	UI ui;
	
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

	addInput(createInput<PJ301MPort>(ui.getPositionDense(UI::PORT, 0, 5), module, Circle::ROTL_INPUT));
	addInput(createInput<PJ301MPort>(ui.getPositionDense(UI::PORT, 1, 5), module, Circle::ROTR_INPUT));
	addInput(createInput<PJ301MPort>(ui.getPositionDense(UI::PORT, 2, 5), module, Circle::KEY_INPUT));
	addParam(createParam<AHKnob>(ui.getPositionDense(UI::KNOB, 3, 5), module, Circle::KEY_PARAM, 0.0, 11.0, 0.0)); 
	addInput(createInput<PJ301MPort>(ui.getPositionDense(UI::PORT, 4, 5), module, Circle::MODE_INPUT));
	addParam(createParam<AHKnob>(ui.getPositionDense(UI::KNOB, 5, 5), module, Circle::MODE_PARAM, 0.0, 6.0, 0.0)); 

	float xOffset = 18.0;
	float xSpace = 21.0;
	float xPos = 0.0;
	float yPos = 0.0;
	int scale = 0;

	for (int i = 0; i < 12; i++) {
		ui.calculateKeyboard(i, xSpace, xOffset, 230.0, &xPos, &yPos, &scale);
		addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, Circle::CKEY_LIGHT + scale));

		ui.calculateKeyboard(i, xSpace, xOffset + 72.0, 165.0, &xPos, &yPos, &scale);
		addChild(createLight<SmallLight<RedLight>>(Vec(xPos, yPos), module, Circle::BKEY_LIGHT + scale));
	}
	
	for (int i = 0; i < 7; i++) {
		addChild(createLight<SmallLight<GreenLight>>(Vec(2 * xOffset + i * 18.0, 280.0), module, Circle::MODE_LIGHT + i));
	}

	addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 0),  module, Circle::KEY_OUTPUT));
	addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 1, 0), module, Circle::MODE_OUTPUT));

}



