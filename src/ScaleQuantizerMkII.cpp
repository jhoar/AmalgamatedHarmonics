#include "dsp/digital.hpp"

#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"

#include <iostream>

struct ScaleQuantizer2 : AHModule {

	enum ParamIds {
		KEY_PARAM,
		SCALE_PARAM,
		ENUMS(SHIFT_PARAM,8),
		TRANS_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(IN_INPUT,8),
		KEY_INPUT,
		SCALE_INPUT,
		TRANS_INPUT,
		ENUMS(HOLD_INPUT,8),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(OUT_OUTPUT,8),
		ENUMS(TRIG_OUTPUT,8),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(KEY_LIGHT,12),
		ENUMS(SCALE_LIGHT,12),
		NUM_LIGHTS
	};

	ScaleQuantizer2() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

		params[KEY_PARAM].config(0.0f, 11.0f, 0.0f); // 12 notes
		params[SCALE_PARAM].config(0.0f, 11.0f, 0.0f); // 12 notes
		params[TRANS_PARAM].config(-11.0f, 11.0f, 0.0f); // 12 notes

	}

	void step() override;

	bool firstStep = true;
	int lastScale = 0;
	int lastRoot = 0;
	float lastTrans = -10000.0f;
	
	dsp::SchmittTrigger holdTrigger[8];
	dsp::PulseGenerator triggerPulse[8];

	float holdPitch[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0};
	float lastPitch[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0};
	
	int currScale = 0;
	int currRoot = 0;

};

void ScaleQuantizer2::step() {
	
	AHModule::step();
	
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
		float holdInput		= inputs[HOLD_INPUT + i].value;
		bool  holdActive	= inputs[HOLD_INPUT + i].active;
		bool  holdStatus	= holdTrigger[i].process(holdInput);

		float volts = inputs[IN_INPUT + i].value;
		float shift = params[SHIFT_PARAM + i].value;

		if (holdActive) { 
			
			// Sample the pitch
			if (holdStatus && inputs[IN_INPUT + i].active) {
				holdPitch[i] = CoreUtil().getPitchFromVolts(volts, currRoot, currScale, &currNote, &currDegree);
			}
			
		} else {

			if (inputs[IN_INPUT + i].active) { 
				holdPitch[i] = CoreUtil().getPitchFromVolts(volts, currRoot, currScale, &currNote, &currDegree);
			} 
			
		}
		
		// If the quantised pitch has changed
		if (lastPitch[i] != holdPitch[i]) {
			// Pulse the gate
			triggerPulse[i].trigger(Core::TRIGGER);
			
			// Record the pitch
			lastPitch[i] = holdPitch[i];
		} 
			
		if (triggerPulse[i].process(delta)) {
			outputs[TRIG_OUTPUT + i].value = 10.0f;
		} else {
			outputs[TRIG_OUTPUT + i].value = 0.0f;
		}

		outputs[OUT_OUTPUT + i].value = holdPitch[i] + shift + trans;

	}

	if (lastScale != currScale || firstStep) {
		for (int i = 0; i < Core::NUM_NOTES; i++) {
			lights[SCALE_LIGHT + i].value = 0.0f;
		}
		lights[SCALE_LIGHT + currScale].value = 1.0f;
	} 

	if (lastRoot != currRoot || firstStep) {
		for (int i = 0; i < Core::NUM_NOTES; i++) {
			lights[KEY_LIGHT + i].value = 0.0f;
		}
		lights[KEY_LIGHT + currRoot].value = 1.0f;
	} 

	firstStep = false;

}

struct ScaleQuantizer2Widget : ModuleWidget {

	ScaleQuantizer2Widget(ScaleQuantizer2 *module) {
		
		setModule(module);
		setPanel(SVG::load(asset::plugin(pluginInstance, "res/ScaleQuantizerMkII.svg")));
		UI ui;

		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 5, true, false), module, ScaleQuantizer2::KEY_INPUT));
		addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 1, 5, true, false), module, ScaleQuantizer2::KEY_PARAM)); // 12 notes
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 3, 5, true, false), module, ScaleQuantizer2::SCALE_INPUT));
		addParam(createParam<AHKnobSnap>(ui.getPosition(UI::PORT, 4, 5, true, false), module, ScaleQuantizer2::SCALE_PARAM)); // 12 notes
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 6, 5, true, false), module, ScaleQuantizer2::TRANS_INPUT));
		addParam(createParam<AHKnobSnap>(ui.getPosition(UI::PORT, 7, 5, true, false), module, ScaleQuantizer2::TRANS_PARAM)); // 12 notes

		for (int i = 0; i < 8; i++) {
			addInput(createInput<PJ301MPort>(Vec(6 + i * 29, 41), module, ScaleQuantizer2::IN_INPUT + i));
			addParam(createParam<AHTrimpotSnap>(Vec(9 + i * 29.1, 101), module, ScaleQuantizer2::SHIFT_PARAM + i));
			addOutput(createOutput<PJ301MPort>(Vec(6 + i * 29, 125), module, ScaleQuantizer2::OUT_OUTPUT + i));
			addInput(createInput<PJ301MPort>(Vec(6 + i * 29, 71), module, ScaleQuantizer2::HOLD_INPUT + i));		
			addOutput(createOutput<PJ301MPort>(Vec(6 + i * 29, 155), module, ScaleQuantizer2::TRIG_OUTPUT + i));
		}

		float xOffset = 18.0;
		float xSpace = 21.0;
		float xPos = 0.0;
		float yPos = 0.0;
		int scale = 0;

		for (int i = 0; i < 12; i++) {
			addChild(createLight<SmallLight<GreenLight>>(Vec(xOffset + i * 18.0f, 280.0f), module, ScaleQuantizer2::SCALE_LIGHT + i));

			ui.calculateKeyboard(i, xSpace, xOffset, 230.0f, &xPos, &yPos, &scale);
			addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, ScaleQuantizer2::KEY_LIGHT + scale));

		}

	}

};

Model *modelScaleQuantizer2 = createModel<ScaleQuantizer2, ScaleQuantizer2Widget>("ScaleQuantizer2");

