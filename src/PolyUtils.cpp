#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct PolyUtils : core::AHModule {

	enum ParamIds {
		MASK_PARAM,
		SPLIT_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		MASK_INPUT,
		SPLIT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		MASK_OUTPUT,
		ENUMS(SPLIT_OUTPUT,2),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	PolyUtils() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		configParam(SPLIT_PARAM, 0.0, 4.0, 0.0, "Split groups");
		configParam(MASK_PARAM, 1.0, 16.0, 0.0, "Inputs to preserve");
	}

	int map[5][16] = {
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1},
		{0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1},
		{0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1},
		{0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1}
	};

	void process(const ProcessArgs &args) override {

		AHModule::step();
		
		// Mask
		if (inputs[MASK_INPUT].isConnected()) {
			int maskChans = params[MASK_PARAM].getValue();
			int j = 0;
			for (; j < maskChans; j++) {
				float inV = inputs[MASK_INPUT].getVoltage(j);
				outputs[MASK_OUTPUT].setVoltage(inV, j);
			}
			for (; j < 16; j++) {
				outputs[MASK_OUTPUT].setVoltage(0.0f, j);
			}
			outputs[MASK_OUTPUT].setChannels(maskChans);
		} else {
			outputs[MASK_OUTPUT].setVoltage(0.0f);
			outputs[MASK_OUTPUT].setChannels(1);
		}

		// Split
		if (inputs[SPLIT_INPUT].isConnected()) {
			int count[2] = {0, 0};
			int groups = params[SPLIT_PARAM].getValue();
			for (int inChan = 0; inChan < inputs[SPLIT_INPUT].getChannels(); inChan++) {
				int side = map[groups][inChan];
				int outChan = count[side];

				outputs[SPLIT_OUTPUT + side].setVoltage(inputs[SPLIT_PARAM].getVoltage(inChan), outChan);
				count[side]++;
			}
			outputs[SPLIT_OUTPUT].setChannels(count[0]);
			outputs[SPLIT_OUTPUT + 1].setChannels(count[1]);
		} else {
			outputs[SPLIT_OUTPUT].setVoltage(0.0f);
			outputs[SPLIT_OUTPUT].setChannels(1);
			outputs[SPLIT_OUTPUT + 1].setVoltage(0.0f);
			outputs[SPLIT_OUTPUT + 1].setChannels(1);
		}

	}
};

struct PolyUtilsWidget : ModuleWidget {

	PolyUtilsWidget(PolyUtils *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyUtils.svg")));

		addParam(createParamCentered<gui::AHKnobSnap>(Vec(43.608, 110.986), module, PolyUtils::MASK_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(43.608, 254.986), module, PolyUtils::SPLIT_PARAM));

		addInput(createInputCentered<gui::AHPort>(Vec(30.0, 70.823), module, PolyUtils::MASK_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(30.0, 224.177), module, PolyUtils::SPLIT_INPUT));

		addOutput(createOutputCentered<gui::AHPort>(Vec(30.0, 147.592), module, PolyUtils::MASK_OUTPUT));
		addOutput(createOutputCentered<gui::AHPort>(Vec(19.565, 289.488), module, PolyUtils::SPLIT_OUTPUT + 0));
		addOutput(createOutputCentered<gui::AHPort>(Vec(40.331, 331.183), module, PolyUtils::SPLIT_OUTPUT + 1));

	}

};

Model *modelPolyUtils = createModel<PolyUtils, PolyUtilsWidget>("PolyUtils");
