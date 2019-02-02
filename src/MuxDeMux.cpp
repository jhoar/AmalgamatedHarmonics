#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct MuxDeMux : core::AHModule {

	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(MONO_INPUT,16),
		POLY_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(MONO_OUTPUT,16),
		POLY_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	MuxDeMux() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {	}

	void step() override {
	
		AHModule::step();

		outputs[POLY_OUTPUT].setChannels(engine::PORT_MAX_CHANNELS);
		for(int i = 0; i < engine::PORT_MAX_CHANNELS; i++) {
			outputs[MONO_OUTPUT + i].setVoltage(inputs[POLY_INPUT].getPolyVoltage(i));
			outputs[POLY_OUTPUT].setVoltage(inputs[MONO_INPUT + i].getVoltage(), i);
		}
	}
};

struct MuxDeMuxWidget : ModuleWidget {

	MuxDeMuxWidget(MuxDeMux *module) {
		
		setModule(module);
		setPanel(SVG::load(asset::plugin(pluginInstance, "res/MuxDeMux.svg")));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 1, true, true), module, MuxDeMux::POLY_INPUT));
		for (int i = 0; i < 8; i++) {
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 1 + i, true, true),  module, MuxDeMux::MONO_OUTPUT + i));
		}

		for (int i = 8; i < 16; i++) {
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 1 + i - 8, true, true),  module, MuxDeMux::MONO_OUTPUT + i));
		}


		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 8, true, true),  module, MuxDeMux::POLY_OUTPUT));
		for (int i = 0; i < 8; i++) {
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 3, 1 + i, true, true), module, MuxDeMux::MONO_INPUT + i));
		}

		for (int i = 8; i < 16; i++) {
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 4, 1 + i - 8, true, true), module, MuxDeMux::MONO_INPUT + i));
		}

	}

};

Model *modelMuxDeMux = createModel<MuxDeMux, MuxDeMuxWidget>("MuxDeMux");
