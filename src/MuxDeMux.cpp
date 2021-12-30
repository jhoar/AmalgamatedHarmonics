#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct MuxDeMux : core::AHModule {

	enum ParamIds {
		BIAS_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(MONO_INPUT,16),
		POLYCV_INPUT,
		BIAS_INPUT,
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

	MuxDeMux() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		configParam(BIAS_PARAM, -10.0, 10.0, 10.0, "Bias", "V");
		paramQuantities[BIAS_PARAM]->description = "Voltage for polyphonic output gates"; 
	}

	bool mask = false;

	void process(const ProcessArgs &args) override {

		AHModule::step();

		float bias = 10.0f;

		if (inputs[BIAS_INPUT].isConnected()) {
			bias = inputs[BIAS_INPUT].getVoltage();
		} else {
			bias = params[BIAS_PARAM].getValue();
		}

		// Process poly input
		int i = 0;
		for( ; i < inputs[POLYCV_INPUT].getChannels(); i++) {
			outputs[MONO_OUTPUT + i].setVoltage(inputs[POLYCV_INPUT].getVoltage(i));		
		}

		for( ; i < engine::PORT_MAX_CHANNELS; i++) {
			outputs[MONO_OUTPUT + i].setVoltage(0.0f);		
		}

		// Process mono input
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

		int j = 0;
		for( ; j < maxChan; j++) {
			if (inputs[MONO_INPUT + j].isConnected()) {
				outputs[POLYCV_OUTPUT].setVoltage(inputs[MONO_INPUT + j].getVoltage(), j);
				outputs[POLYGATE_OUTPUT].setVoltage(bias, j);
			} else {
				outputs[POLYCV_OUTPUT].setVoltage(0.0, j);
				outputs[POLYGATE_OUTPUT].setVoltage(0.0, j);
			}
		}

		for( ; j < engine::PORT_MAX_CHANNELS; j++) {
			outputs[POLYCV_OUTPUT].setVoltage(0.0, j);
			outputs[POLYGATE_OUTPUT].setVoltage(0.0, j);
		}
	}
};

struct MuxDeMuxWidget : ModuleWidget {

	MuxDeMuxWidget(MuxDeMux *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MuxDeMux.svg")));

		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(213.464, 132.652), module, MuxDeMux::BIAS_PARAM));

		addInput(createInputCentered<gui::AHPort>(Vec(27.526, 70.611), module, MuxDeMux::POLYCV_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(213.464, 172.845), module, MuxDeMux::BIAS_INPUT));

		addOutput(createOutputCentered<gui::AHPort>(Vec(212.983, 242.867), module, MuxDeMux::POLYCV_OUTPUT));
		addOutput(createOutputCentered<gui::AHPort>(Vec(211.78, 315.45), module, MuxDeMux::POLYGATE_OUTPUT));

		for (int i = 0; i < 8; i++) {
			addOutput(createOutput<gui::AHPort>(gui::getPosition(gui::PORT, 1, 1 + i, true, true),  module, MuxDeMux::MONO_OUTPUT + i));
		}

		for (int i = 8; i < 16; i++) {
			addOutput(createOutput<gui::AHPort>(gui::getPosition(gui::PORT, 2, 1 + i - 8, true, true),  module, MuxDeMux::MONO_OUTPUT + i));
		}


		for (int i = 0; i < 8; i++) {
			addInput(createInput<gui::AHPort>(gui::getPosition(gui::PORT, 3, 1 + i, true, true), module, MuxDeMux::MONO_INPUT + i));
		}

		for (int i = 8; i < 16; i++) {
			addInput(createInput<gui::AHPort>(gui::getPosition(gui::PORT, 4, 1 + i - 8, true, true), module, MuxDeMux::MONO_INPUT + i));
		}

	}

};

Model *modelMuxDeMux = createModel<MuxDeMux, MuxDeMuxWidget>("MuxDeMux");
