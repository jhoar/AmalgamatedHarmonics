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

		// Mask
		Vec p1 = gui::getPosition(gui::PORT, 0, 1, true, true);
		p1.x = 3;
		addInput(createInput<PJ301MPort>(p1, module, PolyUtils::MASK_INPUT));
	
		Vec p2 = gui::getPosition(gui::PORT, 0, 2, true, true);
		p2.x = 20;
		addParam(createParam<gui::AHKnobSnap>(p2, module, PolyUtils::MASK_PARAM)); 

		Vec p3 = gui::getPosition(gui::PORT, 0, 3, true, true);
		p3.x = 3;
		addOutput(createOutput<PJ301MPort>(p3, module, PolyUtils::MASK_OUTPUT));

		// Split
		Vec p4 = gui::getPosition(gui::PORT, 0, 5, true, true);
		p4.x = 3;
		addInput(createInput<PJ301MPort>(p4, module, PolyUtils::SPLIT_INPUT));

		Vec p5 = gui::getPosition(gui::PORT, 0, 6, true, true);
		p5.x = 20;
		addParam(createParam<gui::AHKnobSnap>(p5, module, PolyUtils::SPLIT_PARAM)); 

		Vec p6 = gui::getPosition(gui::PORT, 0, 7, true, true);
		p6.x = 3;
		addOutput(createOutput<PJ301MPort>(p6,  module, PolyUtils::SPLIT_OUTPUT));

		Vec p7 = gui::getPosition(gui::PORT, 0, 8, true, true);
		p7.x = 20;
		addOutput(createOutput<PJ301MPort>(p7,  module, PolyUtils::SPLIT_OUTPUT + 1));

	}

};

Model *modelPolyUtils = createModel<PolyUtils, PolyUtilsWidget>("PolyUtils");
