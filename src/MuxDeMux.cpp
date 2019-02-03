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

	bool mask = false;

	void step() override {
	
		AHModule::step();

		// Process poly input
		int i = 0;
		for( ; i < inputs[POLYCV_INPUT].getChannels(); i++) {
			if (mask && inputs[POLYGATE_INPUT].isConnected()) { // mask -- set to 0.0v every input where the gate is low
				if (inputs[POLYGATE_INPUT].getVoltage(i) > 0.0f) {
					outputs[MONO_OUTPUT + i].setVoltage(inputs[POLYCV_INPUT].getVoltage(i));		
				} else {
					outputs[MONO_OUTPUT + i].setVoltage(0.0f);		
				}
			} else { // Passthrough -- either no gate or ignore gate and set mono output from poly channel
				outputs[MONO_OUTPUT + i].setVoltage(inputs[POLYCV_INPUT].getVoltage(i));
			}
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
				outputs[POLYGATE_OUTPUT].setVoltage(10.0, j);
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

	void appendContextMenu(Menu *menu) override {

		MuxDeMux *mdm = dynamic_cast<MuxDeMux*>(module);
		assert(mdm);

		struct MaskItem : MenuItem {
			MuxDeMux *module;
			bool mask;
			void onAction(const event::Action &e) override {
				module->mask = mask;
			}
		};

		struct MaskMenu : MenuItem {
			MuxDeMux *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<bool> modes = {true, false};
				std::vector<std::string> names = {"Mask", "Passthrough"};
				for (size_t i = 0; i < modes.size(); i++) {
					MaskItem *item = createMenuItem<MaskItem>(names[i], CHECKMARK(module->mask == modes[i]));
					item->module = module;
					item->mask = modes[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());
		MaskMenu *item = createMenuItem<MaskMenu>("Mask polyphonic input CV");
		item->module = mdm;
		menu->addChild(item);

     }


};

Model *modelMuxDeMux = createModel<MuxDeMux, MuxDeMuxWidget>("MuxDeMux");
