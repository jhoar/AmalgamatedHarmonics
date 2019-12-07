#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct PolyUtils : core::AHModule {

	enum ParamIds {
		ENUMS(MASK_PARAM,4),
		ENUMS(SPLIT_PARAM,4),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(SPLIT_INPUT,4),
		ENUMS(MASK_INPUT,4),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(SPLIT_OUTPUT,8),
		ENUMS(MASK_OUTPUT,4),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	PolyUtils() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		for (int i = 0; i < 4; i++) {
			configParam(SPLIT_PARAM + i, 0.0, 4.0, 0.0, "Split groups");
			configParam(MASK_PARAM + i, 1.0, 16.0, 0.0, "Inputs to preserve");
		}
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
		
		for (int i = 0; i < 4; i++) {
			int nChans = params[MASK_PARAM + i].getValue();

			int j = 0;
			for (; j < nChans; j++) {
				float inV = inputs[MASK_INPUT + i].getVoltage(j);
				outputs[MASK_OUTPUT + i].setVoltage(inV, j);
			}
			for (; j < 16; j++) {
				outputs[MASK_OUTPUT + i].setVoltage(0.0f, j);
			}

			outputs[MASK_OUTPUT + i].setChannels(nChans);
		}

		for (int inPortIdx = 0; inPortIdx < 4; inPortIdx++) {

			rack::engine::Input input = inputs[SPLIT_INPUT + inPortIdx];

			int outPortIdx = inPortIdx * 2;
			int count[2] = {0, 0};
			int groups = params[SPLIT_PARAM + inPortIdx].getValue();

			for (int inChan = 0; inChan < input.getChannels(); inChan++) {

				int side = map[groups][inChan];
				int outChan = count[side];

				outputs[SPLIT_OUTPUT + outPortIdx + side].setVoltage(input.getVoltage(inChan), outChan);
				count[side]++;
			}

			outputs[SPLIT_OUTPUT + outPortIdx + 0].setChannels(count[0]);
			outputs[SPLIT_OUTPUT + outPortIdx + 1].setChannels(count[1]);

		}
	}
};

struct PolyUtilsWidget : ModuleWidget {

	PolyUtilsWidget(PolyUtils *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyUtils.svg")));

		// Mask
		for (int i = 0; i < 4; i++) {
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, i, true, true), module, PolyUtils::MASK_INPUT + i));
			addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 1, i, true, true), module, PolyUtils::MASK_PARAM + i)); 
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 2, i, true, true),  module, PolyUtils::MASK_OUTPUT + i));
		}

		for (int i = 0; i < 4; i++) {
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 5 + i, true, true), module, PolyUtils::SPLIT_INPUT + i));
			addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 1, 5 + i, true, true), module, PolyUtils::SPLIT_PARAM + i)); 
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 5 + i, true, true),  module, PolyUtils::SPLIT_OUTPUT + (i * 2) + 0));
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 3, 5 + i, true, true),  module, PolyUtils::SPLIT_OUTPUT + (i * 2) + 1));
		}

	}

};

Model *modelPolyUtils = createModel<PolyUtils, PolyUtilsWidget>("PolyUtils");
