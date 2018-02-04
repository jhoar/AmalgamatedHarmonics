#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"

#include <iostream>

struct ScaleQuantizer2 : Module {

	enum ParamIds {
		KEY_PARAM,
		SCALE_PARAM,
		SHIFT_PARAM,
		TRANS_PARAM = SHIFT_PARAM + 8,
		NUM_PARAMS
	};
	enum InputIds {
		IN_INPUT,
		KEY_INPUT = IN_INPUT + 8,
		SCALE_INPUT,
		TRANS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS = OUT_OUTPUT + 8
	};
	enum LightIds {
		KEY_LIGHT,
		SCALE_LIGHT = KEY_LIGHT + 12,
		NUM_LIGHTS = SCALE_LIGHT + 12
	};

	ScaleQuantizer2() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	bool firstStep = true;
	int lastScale = 0;
	int lastRoot = 0;
	float lastTrans = -10000.0;
	
	int currScale = 0;
	int currRoot = 0;

};

void ScaleQuantizer2::step() {
	
	lastScale = currScale;
	lastRoot = currRoot;

	int currNote = 0;
	int currDegree = 0;

	if (inputs[KEY_INPUT].active) {
		float fRoot = inputs[KEY_INPUT].value;
		currRoot = CoreUtil().getKeyFromVolts(fRoot);
	} else {
		currRoot = params[KEY_PARAM].value;
	}
	
	if (inputs[SCALE_INPUT].active) {
		float fScale = inputs[SCALE_INPUT].value;
		currScale = CoreUtil().getScaleFromVolts(fScale);
	} else {
		currScale = params[SCALE_PARAM].value;
	}

	float trans = (inputs[TRANS_INPUT].value + params[TRANS_PARAM].value) / 12.0;
	if (trans != 0.0) {
		if (trans != lastTrans) {
			int i;
			int d;
			trans = CoreUtil().getPitchFromVolts(trans, Core::NOTE_C, Core::SCALE_CHROMATIC, &i, &d);
			lastTrans = trans;
		} else {
			trans = lastTrans;
		}
	}
	
	for (int i = 0; i < 8; i++) {
		float volts = inputs[IN_INPUT + i].value;
		float shift = params[SHIFT_PARAM + i].value;

		// Calculate output pitch from raw voltage
		float currPitch = 0.0;
		if (inputs[IN_INPUT + i].active) { 
			currPitch = CoreUtil().getPitchFromVolts(volts, currRoot, currScale, &currNote, &currDegree);
		} 
		
		// Set the value
		outputs[OUT_OUTPUT + i].value = currPitch + shift + trans;
	}

	if (lastScale != currScale || firstStep) {
		for (int i = 0; i < Core::NUM_NOTES; i++) {
			lights[SCALE_LIGHT + i].value = 0.0;
		}
		lights[SCALE_LIGHT + currScale].value = 1.0;
	} 

	if (lastRoot != currRoot || firstStep) {
		for (int i = 0; i < Core::NUM_NOTES; i++) {
			lights[KEY_LIGHT + i].value = 0.0;
		}
		lights[KEY_LIGHT + currRoot].value = 1.0;
	} 

	firstStep = false;

}

ScaleQuantizer2Widget::ScaleQuantizer2Widget() {
	ScaleQuantizer2 *module = new ScaleQuantizer2();
	
	UI ui;
	
	setModule(module);
	box.size = Vec(240, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/ScaleQuantizerMkII.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 5, true, false), module, ScaleQuantizer2::KEY_INPUT));
    addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 1, 5, true, false), module, ScaleQuantizer2::KEY_PARAM, 0.0, 11.0, 0.0)); // 12 notes
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 5, true, false), module, ScaleQuantizer2::SCALE_INPUT));
    addParam(createParam<AHKnobSnap>(ui.getPosition(UI::PORT, 3, 5, true, false), module, ScaleQuantizer2::SCALE_PARAM, 0.0, 11.0, 0.0)); // 12 notes
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 5, true, false), module, ScaleQuantizer2::TRANS_INPUT));
    addParam(createParam<AHKnobSnap>(ui.getPosition(UI::PORT, 5, 5, true, false), module, ScaleQuantizer2::TRANS_PARAM, -11.0, 11.0, 0.0)); // 12 notes

	for (int i = 0; i < 8; i++) {
		addInput(createInput<PJ301MPort>(Vec(6 + i * 29, 61), module, ScaleQuantizer2::IN_INPUT + i));
		addParam(createParam<AHTrimpotSnap>(Vec(9 + i * 29.1, 91), module, ScaleQuantizer2::SHIFT_PARAM + i, -3.0, 3.0, 0.0));
		addOutput(createOutput<PJ301MPort>(Vec(6 + i * 29, 116), module, ScaleQuantizer2::OUT_OUTPUT + i));
	}

	float xOffset = 18.0;
	float xSpace = 21.0;
	float xPos = 0.0;
	float yPos = 0.0;
	int scale = 0;

	for (int i = 0; i < 12; i++) {
		addChild(createLight<SmallLight<GreenLight>>(Vec(xOffset + i * 18.0, 280.0), module, ScaleQuantizer2::SCALE_LIGHT + i));

		ui.calculateKeyboard(i, xSpace, xOffset, 230.0, &xPos, &yPos, &scale);
		addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, ScaleQuantizer2::KEY_LIGHT + scale));

	}

}
