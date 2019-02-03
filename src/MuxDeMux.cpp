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
		POLYCV_INPUT,
		POLYGATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(MONO_OUTPUT,16),
		POLYCV_OUTPUT,
		POLYGATE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	MuxDeMux() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {	}

	void step() override {
	
		AHModule::step();

		// Process poly input
		for(int i = 0; i < inputs[POLYCV_INPUT].getChannels(); i++) {
			if (inputs[POLYGATE_INPUT].isConnected()) {
				if (inputs[POLYGATE_INPUT].getVoltage(i) > 0.0f) {
					outputs[MONO_OUTPUT + i].setVoltage(inputs[POLYCV_INPUT].getVoltage(i));		
				} else {
					outputs[MONO_OUTPUT + i].setVoltage(0.0f);		
				}
			} else {
				outputs[MONO_OUTPUT + i].setVoltage(inputs[POLYCV_INPUT].getVoltage(i));
			}
		}

		int maxChan = -1;

		for(int i = 0; i < engine::PORT_MAX_CHANNELS; i++) {
			if (inputs[MONO_INPUT + i].isConnected()) {
				maxChan = i;
			}
		}

		// Number of channels is index + 1
		maxChan++;

		outputs[POLYCV_OUTPUT].setChannels(maxChan);
		outputs[POLYGATE_OUTPUT].setChannels(maxChan);

		// Process mono input
		for(int i = 0; i < maxChan; i++) {
			if (inputs[MONO_INPUT + i].isConnected()) {
				outputs[POLYCV_OUTPUT].setVoltage(inputs[MONO_INPUT + i].getVoltage(), i);
				outputs[POLYGATE_OUTPUT].setVoltage(10.0, i);
			} else {
				outputs[POLYCV_OUTPUT].setVoltage(0.0, i);
				outputs[POLYGATE_OUTPUT].setVoltage(0.0, i);
			}
		}
	}
};

struct MuxDeMuxWidget : ModuleWidget {

	MuxDeMuxWidget(MuxDeMux *module) {
		
		setModule(module);
		setPanel(SVG::load(asset::plugin(pluginInstance, "res/MuxDeMux.svg")));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 1, true, true), module, MuxDeMux::POLYCV_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 3, true, true), module, MuxDeMux::POLYGATE_INPUT));
		for (int i = 0; i < 8; i++) {
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 1 + i, true, true),  module, MuxDeMux::MONO_OUTPUT + i));
		}

		for (int i = 8; i < 16; i++) {
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 1 + i - 8, true, true),  module, MuxDeMux::MONO_OUTPUT + i));
		}


		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 6, true, true),  module, MuxDeMux::POLYCV_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 8, true, true),  module, MuxDeMux::POLYGATE_OUTPUT));
		for (int i = 0; i < 8; i++) {
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 3, 1 + i, true, true), module, MuxDeMux::MONO_INPUT + i));
		}

		for (int i = 8; i < 16; i++) {
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 4, 1 + i - 8, true, true), module, MuxDeMux::MONO_INPUT + i));
		}

	}

};

Model *modelMuxDeMux = createModel<MuxDeMux, MuxDeMuxWidget>("MuxDeMux");
