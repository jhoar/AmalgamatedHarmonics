#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct ScaleQuantizer2 : core::AHModule {

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

	ScaleQuantizer2() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		params[KEY_PARAM].config(0.0f, 11.0f, 0.0f, "Key"); // 12 notes

		params[SCALE_PARAM].config(0.0f, 11.0f, 0.0f, "Scale"); // 12 scales

		params[TRANS_PARAM].config(-11.0f, 11.0f, 0.0f, "Global transposition", " semitones"); // 12 notes
		params[KEY_PARAM].description = "Transposition of all outputs post-quantisation";
		
		for (int i = 0; i < 8; i++) {
			params[SHIFT_PARAM + i].config(-3.0f, 3.0f, 0.0f, "Octave shift", " octaves");
		}

	}

	void process(const ProcessArgs &args) override;

	bool firstStep = true;
	int lastScale = 0;
	int lastRoot = 0;
	float lastTrans = -10000.0f;
	
	dsp::SchmittTrigger holdTrigger[8];
	dsp::PulseGenerator triggerPulse[8][16];

	float holdPitch[8][16] = {};
	float lastPitch[8][16] = {};
	
	int currScale = 0;
	int currRoot = 0;

};

void ScaleQuantizer2::process(const ProcessArgs &args) {
	
	AHModule::step();
	
	lastScale = currScale;
	lastRoot = currRoot;

	int currNote = 0;
	int currDegree = 0;

	if (inputs[KEY_INPUT].isConnected()) {
		float fRoot = inputs[KEY_INPUT].getVoltage();
		currRoot = music::getKeyFromVolts(fRoot);
	} else {
		currRoot = params[KEY_PARAM].getValue();
	}
	
	if (inputs[SCALE_INPUT].isConnected()) {
		float fScale = inputs[SCALE_INPUT].getVoltage();
		currScale = music::getScaleFromVolts(fScale);
	} else {
		currScale = params[SCALE_PARAM].getValue();
	}

	float trans = (inputs[TRANS_INPUT].getVoltage() + params[TRANS_PARAM].getValue()) / 12.0;
	if (trans != 0.0) {
		if (trans != lastTrans) {
			trans = music::getPitchFromVolts(trans, music::NOTE_C, music::SCALE_CHROMATIC);
			lastTrans = trans;
		} else {
			trans = lastTrans;
		}
	}
	
	for (int i = 0; i < 8; i++) {
		float holdInput		= inputs[HOLD_INPUT + i].getVoltage();
		bool  holdActive	= inputs[HOLD_INPUT + i].isConnected();
		bool  holdStatus	= holdTrigger[i].process(holdInput);
		float shift 		= params[SHIFT_PARAM + i].getValue();

		int nChannels = inputs[IN_INPUT + i].getChannels();
		outputs[OUT_OUTPUT + i].setChannels(nChannels);
		outputs[TRIG_OUTPUT + i].setChannels(nChannels);

		for (int j = 0; j < nChannels; j++) {

			float volts = inputs[IN_INPUT + i].getVoltage(j);

			if (holdActive) { 
				
				// Sample the pitch
				if (holdStatus && inputs[IN_INPUT + i].isConnected()) {
					holdPitch[i][j] = music::getPitchFromVolts(volts, currRoot, currScale, &currNote, &currDegree);
				}
				
			} else {

				if (inputs[IN_INPUT + i].isConnected()) { 
					holdPitch[i][j] = music::getPitchFromVolts(volts, currRoot, currScale, &currNote, &currDegree);
				} 
				
			}
			
			// If the quantises pitch has changed
			if (lastPitch[i][j] != holdPitch[i][j]) {
				// Record the pitch
				lastPitch[i][j] = holdPitch[i][j];

				// Pulse the gate
				triggerPulse[i][j].trigger(digital::TRIGGER);
			} 

			outputs[OUT_OUTPUT + i].setVoltage(holdPitch[i][j] + shift + trans, j);

			if (triggerPulse[i][j].process(args.sampleTime)) {
				outputs[TRIG_OUTPUT + i].setVoltage(10.0f, j);
			} else {
				outputs[TRIG_OUTPUT + i].setVoltage(0.0f, j);
			}

		}

	}

	if (lastScale != currScale || firstStep) {
		for (int i = 0; i < music::NUM_NOTES; i++) {
			lights[SCALE_LIGHT + i].setBrightness(0.0f);
		}
		lights[SCALE_LIGHT + currScale].setBrightness(10.0f);
	} 

	if (lastRoot != currRoot || firstStep) {
		for (int i = 0; i < music::NUM_NOTES; i++) {
			lights[KEY_LIGHT + i].setBrightness(0.0f);
		}
		lights[KEY_LIGHT + currRoot].setBrightness(10.0f);
	} 

	firstStep = false;

}

struct ScaleQuantizer2Widget : ModuleWidget {

	ScaleQuantizer2Widget(ScaleQuantizer2 *module) {
		
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ScaleQuantizerMkII.svg")));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 5, true, false), module, ScaleQuantizer2::KEY_INPUT));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 1, 5, true, false), module, ScaleQuantizer2::KEY_PARAM)); 
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 3, 5, true, false), module, ScaleQuantizer2::SCALE_INPUT));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::PORT, 4, 5, true, false), module, ScaleQuantizer2::SCALE_PARAM));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 6, 5, true, false), module, ScaleQuantizer2::TRANS_INPUT));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::PORT, 7, 5, true, false), module, ScaleQuantizer2::TRANS_PARAM));

		for (int i = 0; i < 8; i++) {
			addInput(createInput<PJ301MPort>(Vec(6 + i * 29, 41), module, ScaleQuantizer2::IN_INPUT + i));
			addParam(createParam<gui::AHTrimpotSnap>(Vec(9 + i * 29.1, 101), module, ScaleQuantizer2::SHIFT_PARAM + i));
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
			gui::calculateKeyboard(i, xSpace, xOffset, 230.0f, &xPos, &yPos, &scale);
			addChild(createLight<SmallLight<GreenLight>>(Vec(xOffset + i * 18.0f, 280.0f), module, ScaleQuantizer2::SCALE_LIGHT + i));
			addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, ScaleQuantizer2::KEY_LIGHT + scale));

		}

	}

};

Model *modelScaleQuantizer2 = createModel<ScaleQuantizer2, ScaleQuantizer2Widget>("ScaleQuantizer2");
