#include "AH.hpp"
#include "AHCommon.hpp"

#include "ProgressState.hpp"

#include <iostream>
#include <array>

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
		COPYBTN_PARAM,
		COPYSRC_PARAM,
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
		STEP_INPUT,
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
		COPYBTN_LIGHT,
		NUM_LIGHTS
	};

	Progress2() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) { 

		configParam(CLOCK_PARAM, -2.0, 6.0, 2.0, "Clock tempo", " bpm", 2.f, 60.f);
		configParam(RUN_PARAM, 0.0, 1.0, 0.0, "Run");
		configParam(RESET_PARAM, 0.0, 1.0, 0.0, "Reset");
		configParam(STEPS_PARAM, 1.0, 8.0, 8.0, "Steps");

		configParam(KEY_PARAM, 0.0, 11.0, 0.0, "Key");
		paramQuantities[KEY_PARAM]->description = "Key from which chords are selected"; 

		configParam(MODE_PARAM, 0.0, 6.0, 0.0, "Mode"); 
		paramQuantities[MODE_PARAM]->description = "Mode from which chords are selected"; 

		configParam(PART_PARAM, 0.0, 31.0, 0.0, "Part"); 
		configParam(COPYBTN_PARAM, 0.0, 1.0, 0.0, "Copy a part to here");
		configParam(COPYSRC_PARAM, 0.0, 31.0, 0.0, "Source part to copy from");

		for (int i = 0; i < 8; i++) {
			configParam(GATE_PARAM + i, 0.0, 1.0, 0.0, "Gate active");
		}

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

		// voltscale
		json_t *scaleModeJ = json_integer((int) voltScale);
		json_object_set_new(rootJ, "voltscale", scaleModeJ);

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {

		// running
		json_t *runningJ = json_object_get(rootJ, "running");
		if (runningJ) running = json_is_true(runningJ);

		// gateMode
		json_t *gateModeJ = json_object_get(rootJ, "gateMode");
		if (gateModeJ) gateMode = (GateMode)json_integer_value(gateModeJ);

		// ProgressState
		json_t *pStateJ = json_object_get(rootJ, "state");
		if (pStateJ) pState.fromJson(pStateJ);

		// voltscale
		json_t *scaleModeJ = json_object_get(rootJ, "voltscale");
		if (scaleModeJ) voltScale = (music::RootScaling)json_integer_value(scaleModeJ);

	}

	music::RootScaling voltScale = music::RootScaling::CIRCLE;

	bool running = true;

	// for external clock
	rack::dsp::SchmittTrigger clockTrigger; 

	// For buttons
	rack::dsp::SchmittTrigger runningTrigger;
	rack::dsp::SchmittTrigger resetTrigger;
	std::array<rack::dsp::SchmittTrigger,8> gateTriggers;
	rack::dsp::SchmittTrigger copyTrigger;

	rack::dsp::PulseGenerator gatePulse;

	/** Phase of internal LFO */
	float phase = 0.0f;

	// Step index
	int index = 0;

	ProgressState pState;

	float resetLight = 0.0f;
	float gateLight = 0.0f;
	std::array<float,8> stepLights;

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

	if (copyTrigger.process(params[COPYBTN_PARAM].getValue())) {
		pState.copyPartFrom(params[COPYSRC_PARAM].getValue());
	}

	pState.nSteps = static_cast<int>(clamp(roundf(params[STEPS_PARAM].getValue() + inputs[STEPS_INPUT].getVoltage()), 1.0f, 8.0f));

	if (running) {
		if (inputs[STEP_INPUT].isConnected()) {
			int stepI = fabs(roundf(inputs[STEP_INPUT].getVoltage()));
			setIndex(stepI % pState.nSteps, pState.nSteps);
		} else {
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
		float v = inputs[KEY_INPUT].getVoltage();
		int currRoot = 0;
		if (voltScale == music::RootScaling::CIRCLE) {
			currRoot = music::getKeyFromVolts(v);
		} else {
			int intv;
			music::getPitchFromVolts(v, music::Notes::NOTE_C, music::Scales::SCALE_CHROMATIC, &currRoot, &intv);
		}
		pState.setKey(currRoot);
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
	lights[COPYBTN_LIGHT].setSmoothBrightness(copyTrigger.isHigh(), args.sampleTime);
	lights[GATES_LIGHT].setSmoothBrightness(pulse, args.sampleTime);

	// Set the output pitches 
	outputs[PITCH_OUTPUT].setChannels(6);
	float *volts = pState.getChordVoltages(pState.currentPart, index);
	for (int i = 0; i < NUM_PITCHES; i++) {
		outputs[PITCH_OUTPUT].setVoltage(volts[i], i);
	}

}

struct Progress2Widget : ModuleWidget {

	std::vector<MenuOption<int>> offsetOptions;
	std::vector<MenuOption<Progress2::GateMode>> gateOptions;
	std::vector<MenuOption<ChordMode>> chordOptions;
	std::vector<MenuOption<music::RootScaling>> scalingOptions;

	Progress2Widget(Progress2 *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Progress2.svg")));

		
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(31.755, 57.727), module, Progress2::CLOCK_PARAM));
		addParam(createParamCentered<gui::AHButton>(Vec(67.49, 57.727), module, Progress2::RUN_PARAM));
		addParam(createParamCentered<gui::AHButton>(Vec(101.583, 57.727), module, Progress2::RESET_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(136.583, 57.727), module, Progress2::STEPS_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(171.696, 57.727), module, Progress2::KEY_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(206.696, 57.727), module, Progress2::MODE_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(241.696, 57.644), module, Progress2::PART_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(277.492, 97.931), module, Progress2::COPYSRC_PARAM));
		addParam(createParamCentered<gui::AHButton>(Vec(312.495, 97.931), module, Progress2::COPYBTN_PARAM));

		addParam(createParamCentered<gui::AHButton>(Vec(68.661, 319.431), module, Progress2::GATE_PARAM + 0));
		addParam(createParamCentered<gui::AHButton>(Vec(104.774, 319.431), module, Progress2::GATE_PARAM + 1));
		addParam(createParamCentered<gui::AHButton>(Vec(139.569, 319.431), module, Progress2::GATE_PARAM + 2));
		addParam(createParamCentered<gui::AHButton>(Vec(174.866, 319.431), module, Progress2::GATE_PARAM + 3));
		addParam(createParamCentered<gui::AHButton>(Vec(209.682, 319.431), module, Progress2::GATE_PARAM + 4));
		addParam(createParamCentered<gui::AHButton>(Vec(244.664, 319.431), module, Progress2::GATE_PARAM + 5));
		addParam(createParamCentered<gui::AHButton>(Vec(279.883, 319.431), module, Progress2::GATE_PARAM + 6));
		addParam(createParamCentered<gui::AHButton>(Vec(314.603, 319.431), module, Progress2::GATE_PARAM + 7));

		addInput(createInputCentered<gui::AHPort>(Vec(241.696, 97.931), module, Progress2::PART_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(31.755, 98.015), module, Progress2::CLOCK_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(67.49, 98.015), module, Progress2::EXT_CLOCK_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(101.583, 98.015), module, Progress2::RESET_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(136.583, 98.015), module, Progress2::STEPS_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(171.696, 98.015), module, Progress2::KEY_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(206.696, 98.015), module, Progress2::MODE_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(29.184, 345.74), module, Progress2::STEP_INPUT));

		addOutput(createOutputCentered<gui::AHPort>(Vec(277.492, 64.126), module, Progress2::GATES_OUTPUT));
		addOutput(createOutputCentered<gui::AHPort>(Vec(312.495, 65.18), module, Progress2::PITCH_OUTPUT));

		addOutput(createOutputCentered<gui::AHPort>(Vec(68.411, 345.645), module, Progress2::GATE_OUTPUT + 0));
		addOutput(createOutputCentered<gui::AHPort>(Vec(104.524, 345.645), module, Progress2::GATE_OUTPUT + 1));
		addOutput(createOutputCentered<gui::AHPort>(Vec(139.319, 345.645), module, Progress2::GATE_OUTPUT + 2));
		addOutput(createOutputCentered<gui::AHPort>(Vec(174.616, 345.645), module, Progress2::GATE_OUTPUT + 3));
		addOutput(createOutputCentered<gui::AHPort>(Vec(209.432, 345.645), module, Progress2::GATE_OUTPUT + 4));
		addOutput(createOutputCentered<gui::AHPort>(Vec(244.414, 345.645), module, Progress2::GATE_OUTPUT + 5));
		addOutput(createOutputCentered<gui::AHPort>(Vec(279.633, 345.645), module, Progress2::GATE_OUTPUT + 6));
		addOutput(createOutputCentered<gui::AHPort>(Vec(314.353, 345.645), module, Progress2::GATE_OUTPUT + 7));

		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(265.124, 51.94), module, Progress2::GATES_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(67.49, 57.727), module, Progress2::RUNNING_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(101.583, 57.727), module, Progress2::RESET_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(312.495, 97.931), module, Progress2::COPYBTN_LIGHT));

		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(68.661, 319.431), module, Progress2::GATE_LIGHTS + 0));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(104.774, 319.431), module, Progress2::GATE_LIGHTS + 2));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(139.569, 319.431), module, Progress2::GATE_LIGHTS + 4));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(174.866, 319.431), module, Progress2::GATE_LIGHTS + 6));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(209.682, 319.431), module, Progress2::GATE_LIGHTS + 8));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(244.664, 319.431), module, Progress2::GATE_LIGHTS + 10));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(279.883, 319.431), module, Progress2::GATE_LIGHTS + 12));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(314.603, 319.431), module, Progress2::GATE_LIGHTS + 14));

		// addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 0, 0, true, false), module, Progress2::CLOCK_PARAM));
		// addParam(createParam<gui::AHButton>(gui::getPosition(gui::BUTTON, 1, 0, true, false), module, Progress2::RUN_PARAM));
		// addChild(createLight<MediumLight<GreenLight>>(gui::getPosition(gui::LIGHT, 1, 0, true, false), module, Progress2::RUNNING_LIGHT));
		// addParam(createParam<gui::AHButton>(gui::getPosition(gui::BUTTON, 2, 0, true, false), module, Progress2::RESET_PARAM));
		// addChild(createLight<MediumLight<GreenLight>>(gui::getPosition(gui::LIGHT, 2, 0, true, false), module, Progress2::RESET_LIGHT));
		// addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 3, 0, true, false), module, Progress2::STEPS_PARAM));

		// addInput(createInput<gui::AHPort>(gui::getPosition(gui::PORT, 0, 1, true, false), module, Progress2::CLOCK_INPUT));
		// addInput(createInput<gui::AHPort>(gui::getPosition(gui::PORT, 1, 1, true, false), module, Progress2::EXT_CLOCK_INPUT));
		// addInput(createInput<gui::AHPort>(gui::getPosition(gui::PORT, 2, 1, true, false), module, Progress2::RESET_INPUT));
		// addInput(createInput<gui::AHPort>(gui::getPosition(gui::PORT, 3, 1, true, false), module, Progress2::STEPS_INPUT));

		// addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 4, 0, true, false), module, Progress2::KEY_PARAM));
		// addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 5, 0, true, false), module, Progress2::MODE_PARAM));
		// addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 6, 0, true, false), module, Progress2::PART_PARAM));
		// addInput(createInput<gui::AHPort>(gui::getPosition(gui::PORT, 4, 1, true, false), module, Progress2::KEY_INPUT));
		// addInput(createInput<gui::AHPort>(gui::getPosition(gui::PORT, 5, 1, true, false), module, Progress2::MODE_INPUT));
		// addInput(createInput<gui::AHPort>(gui::getPosition(gui::PORT, 6, 1, true, false), module, Progress2::PART_INPUT));

		// addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 7, 1, true, false), module, Progress2::COPYSRC_PARAM));
		// addParam(createParam<gui::AHButton>(gui::getPosition(gui::BUTTON, 8, 1, true, false), module, Progress2::COPYBTN_PARAM));
		// addChild(createLight<MediumLight<GreenLight>>(gui::getPosition(gui::LIGHT, 8, 1, true, false), module, Progress2::COPYBTN_LIGHT));

		// Vec XLightPos;
		// XLightPos.x = 261;
		// XLightPos.y = 70;

		// addChild(createLight<SmallLight<GreenLight>>(XLightPos, module, Progress2::GATES_LIGHT));
		// addOutput(createOutput<gui::AHPort>(gui::getPosition(gui::PORT, 7, 0, true, false), module, Progress2::GATES_OUTPUT));
		// addOutput(createOutput<gui::AHPort>(gui::getPosition(gui::PORT, 8, 0, true, false), module, Progress2::PITCH_OUTPUT));

		// addInput(createInput<gui::AHPort>(gui::getPosition(gui::PORT, 0, 5, true, false), module, Progress2::STEP_INPUT));

		// for (int i = 0; i < 8; i++) {
		// 	addParam(createParam<gui::AHButton>(gui::getPosition(gui::BUTTON, i + 1, 8, true, true, 0.0f, -4.0f), module, Progress2::GATE_PARAM + i));
		// 	addChild(createLight<MediumLight<GreenRedLight>>(gui::getPosition(gui::LIGHT, i + 1, 8, true, true, 0.0f, -4.0f), module, Progress2::GATE_LIGHTS + i * 2));
		// 	addOutput(createOutput<gui::AHPort>(gui::getPosition(gui::PORT, i + 1, 5, true, false), module, Progress2::GATE_OUTPUT + i));
		// }

		ProgressStateWidget *stateWidget = createWidget<ProgressStateWidget>(Vec(5.0, 130.0));
		stateWidget->box.size = Vec(300, 165);
		stateWidget->setPState(module ? &module->pState : NULL);
		addChild(stateWidget);

		offsetOptions.emplace_back(std::string("Lower"), 12);
		offsetOptions.emplace_back(std::string("Repeat"), 24);
		offsetOptions.emplace_back(std::string("Upper"), 36);
		offsetOptions.emplace_back(std::string("Random"), 0);

		gateOptions.emplace_back(std::string("Trigger"), Progress2::GateMode::TRIGGER);
		gateOptions.emplace_back(std::string("Retrigger"), Progress2::GateMode::RETRIGGER);
		gateOptions.emplace_back(std::string("Continuous"), Progress2::GateMode::CONTINUOUS);

		chordOptions.emplace_back(std::string("All Chords"), ChordMode::NORMAL);
		chordOptions.emplace_back(std::string("Chords from Mode"), ChordMode::MODE);
		chordOptions.emplace_back(std::string("Chords from Mode (coerce)"), ChordMode::COERCE);

		scalingOptions.emplace_back(std::string("V/Oct"), music::RootScaling::VOCT);
		scalingOptions.emplace_back(std::string("Fourths and Fifths"), music::RootScaling::CIRCLE);

	}

	void appendContextMenu(Menu *menu) override {

		Progress2 *progress = dynamic_cast<Progress2*>(module);
		assert(progress);

		struct Progress2Menu : MenuItem {
			Progress2 *module;
			Progress2Widget *parent;
		};

		struct GateModeItem : Progress2Menu {
			Progress2::GateMode gateMode;
			void onAction(const rack::event::Action &e) override {
				module->gateMode = gateMode;
			}
		};

		struct OffsetItem : Progress2Menu {
			int offset;
			void onAction(const rack::event::Action &e) override {
				module->pState.offset = offset;
				module->pState.stateChanged = true;
			}
		};

		struct ChordModeItem : Progress2Menu {
			ChordMode chordMode;
			void onAction(const rack::event::Action &e) override {
				module->pState.chordMode = chordMode;
				module->pState.modeChanged = true;
			}
		};

		struct GateModeMenu : Progress2Menu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->gateOptions) {
					GateModeItem *item = createMenuItem<GateModeItem>(opt.name, CHECKMARK(module->gateMode == opt.value));
					item->module = module;
					item->gateMode = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		struct OffsetMenu : Progress2Menu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->offsetOptions) {
					OffsetItem *item = createMenuItem<OffsetItem>(opt.name, CHECKMARK(module->pState.offset == opt.value));
					item->module = module;
					item->offset = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		struct ChordModeMenu : Progress2Menu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->chordOptions) {
					ChordModeItem *item = createMenuItem<ChordModeItem>(opt.name, CHECKMARK(module->pState.chordMode == opt.value));
					item->module = module;
					item->chordMode = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		struct ScalingItem : Progress2Menu {
			music::RootScaling voltScale;
			void onAction(const rack::event::Action &e) override {
				module->voltScale = voltScale;
			}
		};

		struct ScalingMenu : Progress2Menu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->scalingOptions) {
					ScalingItem *item = createMenuItem<ScalingItem>(opt.name, CHECKMARK(module->voltScale == opt.value));
					item->module = module;
					item->voltScale = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};


		menu->addChild(construct<MenuLabel>());
		ChordModeMenu *chordItem = createMenuItem<ChordModeMenu>("Chord Selection");
		chordItem->module = progress;
		chordItem->parent = this;
		menu->addChild(chordItem);

		GateModeMenu *gateItem = createMenuItem<GateModeMenu>("Gate Mode");
		gateItem->module = progress;
		gateItem->parent = this;
		menu->addChild(gateItem);

		OffsetMenu *offsetItem = createMenuItem<OffsetMenu>("Repeat Notes");
		offsetItem->module = progress;
		offsetItem->parent = this;
		menu->addChild(offsetItem);

		ScalingMenu *scaleItem = createMenuItem<ScalingMenu>("Root Volt Scaling");
		scaleItem->module = progress;
		scaleItem->parent = this;
		menu->addChild(scaleItem);

	}

};

Model *modelProgress2 = createModel<Progress2, Progress2Widget>("Progress2");
