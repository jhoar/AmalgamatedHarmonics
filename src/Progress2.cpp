#include "AH.hpp"
#include "AHCommon.hpp"

#include "ProgressState.hpp"

#include <iostream>

using namespace ah;

struct Progress2 : core::AHModule {

	const static int NUM_PITCHES = 6;
	
	enum ParamIds {
		CLOCK_PARAM,
		RUN_PARAM,
		RESET_PARAM,
		STEPS_PARAM,
		KEY_PARAM,
		MODE_PARAM,
		ENUMS(GATE_PARAM,8),
		PART_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		KEY_INPUT,
		MODE_INPUT,
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		PART_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATES_OUTPUT,
		PITCH_OUTPUT,
		ENUMS(GATE_OUTPUT,8),
		NUM_OUTPUTS
	};
	enum LightIds {
		RUNNING_LIGHT,
		RESET_LIGHT,
		GATES_LIGHT,
		ENUMS(GATE_LIGHTS,16),
		NUM_LIGHTS
	};

	Progress2() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) { 

		params[CLOCK_PARAM].config(-2.0, 6.0, 2.0, "Frequency");
		params[RUN_PARAM].config(0.0, 1.0, 0.0, "Run");
		params[RESET_PARAM].config(0.0, 1.0, 0.0, "Reset");
		params[STEPS_PARAM].config(1.0, 8.0, 8.0, "Steps");

		params[KEY_PARAM].config(0.0, 11.0, 0.0, "Key");
		params[KEY_PARAM].description = "Key from which chords are selected"; 

		params[MODE_PARAM].config(0.0, 6.0, 0.0, "Mode"); 
		params[MODE_PARAM].description = "Mode from which chords are selected"; 

		params[PART_PARAM].config(0.0, 31.0, 0.0, "Part"); 

		for (int i = 0; i < 8; i++) {
			params[GATE_PARAM + i].config(0.0, 1.0, 0.0, "Gate active");
		}

		pState.knownChords = knownChords;

		onReset();

	}
	
	void process(const ProcessArgs &args) override;
			
	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// running
		json_object_set_new(rootJ, "running", json_boolean(running));

		// gateMode
		json_t *gateModeJ = json_integer((int) gateMode);
		json_object_set_new(rootJ, "gateMode", gateModeJ);

		// ProgressState
		json_object_set_new(rootJ, "state", pState.toJson());

		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override {

		// running
		json_t *runningJ = json_object_get(rootJ, "running");
		if (runningJ)
			running = json_is_true(runningJ);

		// gateMode
		json_t *gateModeJ = json_object_get(rootJ, "gateMode");
		if (gateModeJ)
			gateMode = (GateMode)json_integer_value(gateModeJ);

		// ProgressState
		json_t *pStateJ = json_object_get(rootJ, "state");
		if (pStateJ)
			pState.fromJson(pStateJ);

	}
	
	music::KnownChords knownChords;

	bool running = true;
	
	// for external clock
	rack::dsp::SchmittTrigger clockTrigger; 
	
	// For buttons
	rack::dsp::SchmittTrigger runningTrigger;
	rack::dsp::SchmittTrigger resetTrigger;
	rack::dsp::SchmittTrigger gateTriggers[8];
		
	rack::dsp::PulseGenerator gatePulse;

	/** Phase of internal LFO */
	float phase = 0.0f;

	// Step index
	int index = 0;

	ProgressState pState;

	float resetLight = 0.0f;
	float gateLight = 0.0f;
	float stepLights[8] = {};

	enum GateMode {
		TRIGGER,
		RETRIGGER,
		CONTINUOUS,
	};
	GateMode gateMode = CONTINUOUS;
	
	void onReset() override {
		pState.onReset();
	}
	
	void setIndex(int index, int nSteps) {
		phase = 0.0f;
		this->index = index;
		if (this->index >= nSteps) {
			this->index = 0;
		}
		this->gatePulse.trigger(digital::TRIGGER);
	}
	
};

void Progress2::process(const ProcessArgs &args) {
	
	AHModule::step();
	
	// Run
	if (runningTrigger.process(params[RUN_PARAM].getValue())) {
		running = !running;
	}

	pState.nSteps = (int) clamp(roundf(params[STEPS_PARAM].getValue() + inputs[STEPS_INPUT].getVoltage()), 1.0f, 8.0f);

	if (running) {
		if (inputs[EXT_CLOCK_INPUT].isConnected()) {
			// External clock
			if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].getVoltage())) {
				setIndex(index + 1, pState.nSteps);
			}
		}
		else {
			// Internal clock
			float clockTime = powf(2.0f, params[CLOCK_PARAM].getValue() + inputs[CLOCK_INPUT].getVoltage());
			phase += clockTime * args.sampleTime;
			if (phase >= 1.0f) {
				setIndex(index + 1, pState.nSteps);
			}
		}
	}

	// Reset
	if (resetTrigger.process(params[RESET_PARAM].getValue() + inputs[RESET_INPUT].getVoltage())) {
		setIndex(0, pState.nSteps);
	}

	if (inputs[MODE_INPUT].isConnected()) {
		pState.setMode(music::getModeFromVolts(inputs[MODE_INPUT].getVoltage()));
	} else {
		pState.setMode(params[MODE_PARAM].getValue());
	}

	if (inputs[KEY_INPUT].isConnected()) {
		pState.setKey(music::getKeyFromVolts(inputs[KEY_INPUT].getVoltage()));
	} else {
		pState.setKey(params[KEY_PARAM].getValue());
	}

	if (inputs[PART_INPUT].isConnected()) {
		float pVal = math::clamp(inputs[PART_INPUT].getVoltage(), 0.0f, 10.0f);
		pState.setPart((int)math::rescale(pVal, 0.0f, 10.0f, 0, 31));
	} else {
		pState.setPart(params[PART_PARAM].getValue());
	}

	// Update
	pState.update();

	// So, after all that, we calculate the pitch output
	bool pulse = gatePulse.process(args.sampleTime);
	
	// Gate buttons
	for (int i = 0; i < 8; i++) {
		if (gateTriggers[i].process(params[GATE_PARAM + i].getValue())) {
			pState.toggleGate(pState.currentPart, i);
		}
		
		bool gateOn = (running && i == index && pState.gateState(pState.currentPart, i));
		if (gateMode == TRIGGER) {
			gateOn = gateOn && pulse;
		} else if (gateMode == RETRIGGER) {
			gateOn = gateOn && !pulse;
		}
		
		outputs[GATE_OUTPUT + i].setVoltage(gateOn ? 10.0f : 0.0f);	
		
		if (i == index) {
			if (pState.gateState(pState.currentPart, i)) {
				// Gate is on and active = flash green
				lights[GATE_LIGHTS + i * 2].setSmoothBrightness(1.0f, args.sampleTime);
				lights[GATE_LIGHTS + i * 2 + 1].setSmoothBrightness(0.0f, args.sampleTime);
			} else {
				// Gate is off and active = flash dull yellow
				lights[GATE_LIGHTS + i * 2].setSmoothBrightness(0.20f, args.sampleTime);
				lights[GATE_LIGHTS + i * 2 + 1].setSmoothBrightness(0.20f, args.sampleTime);
			}
		} else {
			if (pState.gateState(pState.currentPart, i)) {
				// Gate is on and not active = red
				lights[GATE_LIGHTS + i * 2].setSmoothBrightness(0.0f, args.sampleTime);
				lights[GATE_LIGHTS + i * 2 + 1].setSmoothBrightness(1.0f, args.sampleTime);
			} else {
				// Gate is off and not active = black
				lights[GATE_LIGHTS + i * 2].setSmoothBrightness(0.0f, args.sampleTime);
				lights[GATE_LIGHTS + i * 2 + 1].setSmoothBrightness(0.0f, args.sampleTime);
			}			
		}
	}

	bool gatesOn = (running && pState.gateState(pState.currentPart, index));
	if (gateMode == TRIGGER) {
		gatesOn = gatesOn && pulse;
	} else if (gateMode == RETRIGGER) {
		gatesOn = gatesOn && !pulse;
	}

	// Outputs
	outputs[GATES_OUTPUT].setVoltage(gatesOn ? 10.0f : 0.0f);
	lights[RUNNING_LIGHT].setBrightness(running);
	lights[RESET_LIGHT].setSmoothBrightness(resetTrigger.isHigh(), args.sampleTime);
	lights[GATES_LIGHT].setSmoothBrightness(pulse, args.sampleTime);

	// Set the output pitches 
	outputs[PITCH_OUTPUT].setChannels(6);
	float *volts = pState.getChordVoltages(pState.currentPart, index);
	for (int i = 0; i < NUM_PITCHES; i++) {
		outputs[PITCH_OUTPUT].setVoltage(volts[i], i);
	}
	
}

struct Progress2Widget : ModuleWidget {

	Progress2Widget(Progress2 *module) {
		
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Progress2.svg")));
		
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 0, 0, true, false), module, Progress2::CLOCK_PARAM));
		addParam(createParam<gui::AHButton>(gui::getPosition(gui::BUTTON, 1, 0, true, false), module, Progress2::RUN_PARAM));
		addChild(createLight<MediumLight<GreenLight>>(gui::getPosition(gui::LIGHT, 1, 0, true, false), module, Progress2::RUNNING_LIGHT));
		addParam(createParam<gui::AHButton>(gui::getPosition(gui::BUTTON, 2, 0, true, false), module, Progress2::RESET_PARAM));
		addChild(createLight<MediumLight<GreenLight>>(gui::getPosition(gui::LIGHT, 2, 0, true, false), module, Progress2::RESET_LIGHT));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 3, 0, true, false), module, Progress2::STEPS_PARAM));

	//	static const float portX[13] = {20, 58, 96, 135, 173, 212, 250, 288, 326, 364, 402, 440, 478};
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 1, true, false), module, Progress2::CLOCK_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 1, true, false), module, Progress2::EXT_CLOCK_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 1, true, false), module, Progress2::RESET_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 3, 1, true, false), module, Progress2::STEPS_INPUT));

		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 4, 0, true, false), module, Progress2::KEY_PARAM));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 5, 0, true, false), module, Progress2::MODE_PARAM));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 6, 0, true, false), module, Progress2::PART_PARAM));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 4, 1, true, false), module, Progress2::KEY_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 1, true, false), module, Progress2::MODE_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 6, 1, true, false), module, Progress2::PART_INPUT));

		addChild(createLight<MediumLight<GreenLight>>(gui::getPosition(gui::LIGHT, 0, 5, true, false), module, Progress2::GATES_LIGHT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 7, 0, true, false), module, Progress2::GATES_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 8, 0, true, false), module, Progress2::PITCH_OUTPUT));

		for (int i = 0; i < 8; i++) {
			addParam(createParam<gui::AHButton>(gui::getPosition(gui::BUTTON, i + 1, 8, true, true, 0.0f, -4.0f), module, Progress2::GATE_PARAM + i));
			addChild(createLight<MediumLight<GreenRedLight>>(gui::getPosition(gui::LIGHT, i + 1, 8, true, true, 0.0f, -4.0f), module, Progress2::GATE_LIGHTS + i * 2));
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, i + 1, 5, true, false), module, Progress2::GATE_OUTPUT + i));
		}
		
		ProgressStateWidget *stateWidget = createWidget<ProgressStateWidget>(Vec(5.0, 130.0));
		stateWidget->box.size = Vec(275, 165);
		stateWidget->setPState(module ? &module->pState : NULL);
		addChild(stateWidget);

	}

	void appendContextMenu(Menu *menu) override {

		Progress2 *progress = dynamic_cast<Progress2*>(module);
		assert(progress);

		struct GateModeItem : MenuItem {
			Progress2 *module;
			Progress2::GateMode gateMode;
			void onAction(const event::Action &e) override {
				module->gateMode = gateMode;
			}
		};

		struct OffsetItem : MenuItem {
			Progress2 *module;
			int offset;
			void onAction(const event::Action &e) override {
				module->pState.offset = offset;
				module->pState.settingChanged = true;
			}
		};

		struct ChordModeItem : MenuItem {
			Progress2 *module;
			int chordMode;
			void onAction(const event::Action &e) override {
				module->pState.chordMode = chordMode;
				module->pState.settingChanged = true;
			}
		};

		struct GateModeMenu : MenuItem {
			Progress2 *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<Progress2::GateMode> modes = {Progress2::TRIGGER, Progress2::RETRIGGER, Progress2::CONTINUOUS};
				std::vector<std::string> names = {"Trigger", "Retrigger", "Continuous"};
				for (size_t i = 0; i < modes.size(); i++) {
					GateModeItem *item = createMenuItem<GateModeItem>(names[i], CHECKMARK(module->gateMode == modes[i]));
					item->module = module;
					item->gateMode = modes[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		struct OffsetMenu : MenuItem {
			Progress2 *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<int> offsets = {12, 24, 36, 0};
				std::vector<std::string> names = {"Lower", "Repeat", "Upper", "Random"};
				for (size_t i = 0; i < offsets.size(); i++) {
					OffsetItem *item = createMenuItem<OffsetItem>(names[i], CHECKMARK(module->pState.offset == offsets[i]));
					item->module = module;
					item->offset = offsets[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		struct ChordModeMenu : MenuItem {
			Progress2 *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<int> chordModes = {0, 1};
				std::vector<std::string> names = {"Normal Chords", "Chords from Mode"};
				for (size_t i = 0; i < chordModes.size(); i++) {
					ChordModeItem *item = createMenuItem<ChordModeItem>(names[i], CHECKMARK(module->pState.chordMode == chordModes[i]));
					item->module = module;
					item->chordMode = chordModes[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());
		ChordModeMenu *chordItem = createMenuItem<ChordModeMenu>("Chord Mode");
		chordItem->module = progress;
		menu->addChild(chordItem);

		GateModeMenu *gateItem = createMenuItem<GateModeMenu>("Gate Mode");
		gateItem->module = progress;
		menu->addChild(gateItem);

		OffsetMenu *offsetItem = createMenuItem<OffsetMenu>("Repeat Notes");
		offsetItem->module = progress;
		menu->addChild(offsetItem);

     }
	 
};

Model *modelProgress2 = createModel<Progress2, Progress2Widget>("Progress2");
