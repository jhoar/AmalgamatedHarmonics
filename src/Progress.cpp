#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"

#include <iostream>

struct Progress : AHModule {

	const static int NUM_PITCHES = 6;
	
	enum ParamIds {
		CLOCK_PARAM,
		RUN_PARAM,
		RESET_PARAM,
		STEPS_PARAM,
		ENUMS(ROOT_PARAM,8),
		ENUMS(CHORD_PARAM,8),
		ENUMS(INV_PARAM,8),
		ENUMS(GATE_PARAM,8),
		NUM_PARAMS
	};
	enum InputIds {
		KEY_INPUT,
		MODE_INPUT,
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATES_OUTPUT,
		ENUMS(PITCH_OUTPUT,6),
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

	Progress() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) { 

		params[CLOCK_PARAM].config(-2.0, 6.0, 2.0, "Frequency");
		params[RUN_PARAM].config(0.0, 1.0, 0.0, "Run");
		params[RESET_PARAM].config(0.0, 1.0, 0.0, "Reset");
		params[STEPS_PARAM].config(1.0, 8.0, 8.0, "Steps");

		for (int i = 0; i < 8; i++) {
			params[ROOT_PARAM + i].config(0.0, 10.0, 0.0, "Root note");
			params[ROOT_PARAM + i].description = "Root note [degree of scale]";

			params[CHORD_PARAM + i].config(0.0, 10.0, 0.0, "Chord");

			params[INV_PARAM + i].config(0.0, 2.0, 0.0, "Inversion");
			params[INV_PARAM + i].description = "Root, first of second inversion";

			params[GATE_PARAM + i].config(0.0, 1.0, 0.0, "Gate active");
		}

		onReset();

	}
	
	void step() override;
	
	enum ParamType {
		ROOT_TYPE,
		CHORD_TYPE,
		INV_TYPE
	};

	void receiveEvent(ParamEvent e) override {
		if (receiveEvents && e.pType != -1) { // AHParamWidgets that are no config through set<>() have a pType of -1
			if (modeMode) {
				paramState = "> " + 
					CoreUtil().noteNames[currRoot[e.pId]] + 
					CoreUtil().ChordTable[currChord[e.pId]].quality + " " +  
					CoreUtil().inversionNames[currInv[e.pId]] + " " + "[" + 
					CoreUtil().degreeNames[currDegree[e.pId] * 3 + currQuality[e.pId]] + "]"; 
			} else {
				paramState = "> " + 
					CoreUtil().noteNames[currRoot[e.pId]] + 
					CoreUtil().ChordTable[currChord[e.pId]].quality + " " +  
					CoreUtil().inversionNames[currInv[e.pId]];
			}
		}
		keepStateDisplay = 0;
	}
	
		
	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// running
		json_object_set_new(rootJ, "running", json_boolean(running));

		// gates
		json_t *gatesJ = json_array();
		for (int i = 0; i < 8; i++) {
			json_t *gateJ = json_integer((int) gates[i]);
			json_array_append_new(gatesJ, gateJ);
		}
		json_object_set_new(rootJ, "gates", gatesJ);

		// gateMode
		json_t *gateModeJ = json_integer((int) gateMode);
		json_object_set_new(rootJ, "gateMode", gateModeJ);

		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override {
		// running
		json_t *runningJ = json_object_get(rootJ, "running");
		if (runningJ)
			running = json_is_true(runningJ);

		// gates
		json_t *gatesJ = json_object_get(rootJ, "gates");
		if (gatesJ) {
			for (int i = 0; i < 8; i++) {
				json_t *gateJ = json_array_get(gatesJ, i);
				if (gateJ)
					gates[i] = !!json_integer_value(gateJ);
			}
		}

		// gateMode
		json_t *gateModeJ = json_object_get(rootJ, "gateMode");
		if (gateModeJ)
			gateMode = (GateMode)json_integer_value(gateModeJ);
	}
	
	bool running = true;
	
	// for external clock
	dsp::SchmittTrigger clockTrigger; 
	
	// For buttons
	dsp::SchmittTrigger runningTrigger;
	dsp::SchmittTrigger resetTrigger;
	dsp::SchmittTrigger gateTriggers[8];
		
	dsp::PulseGenerator gatePulse;

	/** Phase of internal LFO */
	float phase = 0.0f;

	// Step index
	int index = 0;
	bool gates[8] = {true,true,true,true,true,true,true,true};

	float resetLight = 0.0f;
	float gateLight = 0.0f;
	float stepLights[8] = {};

	enum GateMode {
		TRIGGER,
		RETRIGGER,
		CONTINUOUS,
	};
	GateMode gateMode = CONTINUOUS;
		
	bool modeMode = false;
	bool prevModeMode = false;
	
	int offset = 24; 	// Repeated notes in chord and expressed in the chord definition as being transposed 2 octaves lower. 
						// When played this offset needs to be removed (or the notes removed, or the notes transposed to an octave higher)
	
	float prevRootInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};
	float prevChrInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};

	float prevDegreeInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};
	float prevQualityInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};

	float prevInvInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};

	float currRootInput[8];
	float currChrInput[8];

	float currDegreeInput[8];
	float currQualityInput[8];

	float currInvInput[8];
	
	int currMode;
	int currKey;
	int prevMode = -1;
	int prevKey = -1;
	
	int currRoot[8];
	int currChord[8];
	int currInv[8];	

	int currDegree[8];
	int currQuality[8];
	
	float pitches[8][6];
	float oldPitches[6];
			
	void onReset() override {
		for (int i = 0; i < 8; i++) {
			gates[i] = true;
		}
	}
	
	void setIndex(int index, int nSteps) {
		phase = 0.0f;
		this->index = index;
		if (this->index >= nSteps) {
			this->index = 0;
		}
		this->gatePulse.trigger(Core::TRIGGER);
	}
	
};

void Progress::step() {
	
	AHModule::step();
	
	// Run
	if (runningTrigger.process(params[RUN_PARAM].getValue())) {
		running = !running;
	}

	int numSteps = (int) clamp(roundf(params[STEPS_PARAM].getValue() + inputs[STEPS_INPUT].getVoltage()), 1.0f, 8.0f);

	if (running) {
		if (inputs[EXT_CLOCK_INPUT].isConnected()) {
			// External clock
			if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].getVoltage())) {
				setIndex(index + 1, numSteps);
			}
		}
		else {
			// Internal clock
			float clockTime = powf(2.0f, params[CLOCK_PARAM].getValue() + inputs[CLOCK_INPUT].getVoltage());
			phase += clockTime * delta;
			if (phase >= 1.0f) {
				setIndex(index + 1, numSteps);
			}
		}
	}

	// Reset
	if (resetTrigger.process(params[RESET_PARAM].getValue() + inputs[RESET_INPUT].getVoltage())) {
		setIndex(0, numSteps);
	}

	bool haveRoot = false;
	bool haveMode = false;

	// index is our current step
	if (inputs[KEY_INPUT].isConnected()) {
		float fRoot = inputs[KEY_INPUT].getVoltage();
		currKey = CoreUtil().getKeyFromVolts(fRoot);
		haveRoot = true;
	}

	if (inputs[MODE_INPUT].isConnected()) {
		float fMode = inputs[MODE_INPUT].getVoltage();
		currMode = CoreUtil().getModeFromVolts(fMode);	
		haveMode = true;
	}
	
	modeMode = haveRoot && haveMode;
	
	 if (modeMode && ((prevMode != currMode) || (prevKey != currKey))) { // Input changes so force re-read
	 	for (int step = 0; step < 8; step++) {
			prevDegreeInput[step]    = -100.0;
			prevQualityInput[step]  = -100.0;
		}
		
		prevMode = currMode;
		prevKey = currKey;
		
	}
	
	// Read inputs
	for (int step = 0; step < 8; step++) {
		if (modeMode) {
			currDegreeInput[step]  = params[CHORD_PARAM + step].getValue();
			currQualityInput[step] = params[ROOT_PARAM + step].getValue();
			if (prevModeMode != modeMode) { // Switching mode, so reset history to ensure re-read on return
				prevChrInput[step]  = -100.0;
				prevRootInput[step]  = -100.0;
			}
		} else {
			currChrInput[step]  = params[CHORD_PARAM + step].getValue();
			currRootInput[step] = params[ROOT_PARAM + step].getValue();
			if (prevModeMode != modeMode) { // Switching mode, so reset history to ensure re-read on return
				prevDegreeInput[step]  = -100.0;
				prevQualityInput[step]  = -100.0;
			}
		}
		currInvInput[step]  = params[INV_PARAM + step].getValue();
	}
	
	// Remember mode
	prevModeMode = modeMode;
	
	// Check for changes on all steps
	for (int step = 0; step < 8; step++) {
		
		bool update = false;
		
		if (modeMode) {			
		
			currDegreeInput[step]   = params[ROOT_PARAM + step].getValue();
			currQualityInput[step] = params[CHORD_PARAM + step].getValue();
							
			if (prevDegreeInput[step] != currDegreeInput[step]) {
				prevDegreeInput[step] = currDegreeInput[step];
				update = true;
			}
		
			if (prevQualityInput[step] != currQualityInput[step]) {
				prevQualityInput[step]  = currQualityInput[step]; 
				update = true;
			}
			
			if (update) {
				
				// Get Degree (I- VII)
				currDegree[step] = round(rescale(fabs(currDegreeInput[step]), 0.0f, 10.0f, 0.0f, Core::NUM_DEGREES - 1)); 

				// From the input root, mode and degree, we can get the root chord note and quality (Major,Minor,Diminshed)
				CoreUtil().getRootFromMode(currMode,currKey,currDegree[step],&currRoot[step],&currQuality[step]);

				// Now get the actual chord from the main list
				switch(currQuality[step]) {
					case Core::MAJ: 
						currChord[step] = round(rescale(fabs(currQualityInput[step]), 0.0f, 10.0f, 1.0f, 70.0f)); 
						break;
					case Core::MIN: 
						currChord[step] = round(rescale(fabs(currQualityInput[step]), 0.0f, 10.0f, 71.0f, 90.0f));
						break;
					case Core::DIM: 
						currChord[step] = round(rescale(fabs(currQualityInput[step]), 0.0f, 10.0f, 91.0f, 98.0f));
						break;		
				}
			
			}

		} else {
			
			// Chord Mode
			
			// If anything has changed, recalculate output for that step
			if (prevRootInput[step] != currRootInput[step]) {
				prevRootInput[step] = currRootInput[step];
				currRoot[step] = round(rescale(fabs(currRootInput[step]), 0.0f, 10.0f, 0.0f, Core::NUM_NOTES - 1)); // Param range is 0 to 10, mapped to 0 to 11
				update = true;
			}
		
			if (prevChrInput[step] != currChrInput[step]) {
				prevChrInput[step]  = currChrInput[step]; 
				currChord[step] = round(rescale(fabs(currChrInput[step]), 0.0f, 10.0f, 1.0f, 98.0f)); // Param range is 0 to 10		
				update = true;
			}
			
		}
		
		// Inversions remain the same between Chord and Mode mode
		if (prevInvInput[step] != currInvInput[step]) {
			prevInvInput[step]  = currInvInput[step];		
			currInv[step] = currInvInput[step];
			update = true;
		}
		
		// So, after all that, we calculate the pitch output
		if (update) {
					
			int *chordArray;
	
			// Get the array of pitches based on the inversion
			switch(currInv[step]) {
				case Core::ROOT:  		chordArray = CoreUtil().ChordTable[currChord[step]].root; 	break;
				case Core::FIRST_INV:  	chordArray = CoreUtil().ChordTable[currChord[step]].first; 	break;
				case Core::SECOND_INV:  chordArray = CoreUtil().ChordTable[currChord[step]].second;	break;
				default: chordArray = CoreUtil().ChordTable[currChord[step]].root;
			}
			
			for (int j = 0; j < NUM_PITCHES; j++) {
	
				// Set the pitches for this step. If the chord has less than 6 notes, the empty slots are
				// filled with repeated notes. These notes are identified by a  24 semi-tome negative
				// offset. We correct for that offset now, pitching thaem back into the original octave.
				// They could be pitched into the octave above (or below)
				if (chordArray[j] < 0) {
					pitches[step][j] = CoreUtil().getVoltsFromPitch(chordArray[j] + offset,currRoot[step]);			
				} else {
					pitches[step][j] = CoreUtil().getVoltsFromPitch(chordArray[j],currRoot[step]);			
				}	
			}
		}	
	}
	
	bool pulse = gatePulse.process(delta);
	
	// Gate buttons
	for (int i = 0; i < 8; i++) {
		if (gateTriggers[i].process(params[GATE_PARAM + i].getValue())) {
			gates[i] = !gates[i];
		}
		
		bool gateOn = (running && i == index && gates[i]);
		if (gateMode == TRIGGER) {
			gateOn = gateOn && pulse;
		} else if (gateMode == RETRIGGER) {
			gateOn = gateOn && !pulse;
		}
		
		outputs[GATE_OUTPUT + i].setVoltage(gateOn ? 10.0f : 0.0f);	
		
		if (i == index) {
			if (gates[i]) {
				// Gate is on and active = flash green
				lights[GATE_LIGHTS + i * 2].setBrightnessSmooth(1.0f);
				lights[GATE_LIGHTS + i * 2 + 1].setBrightnessSmooth(0.0f);
			} else {
				// Gate is off and active = flash dull yellow
				lights[GATE_LIGHTS + i * 2].setBrightnessSmooth(0.20f);
				lights[GATE_LIGHTS + i * 2 + 1].setBrightnessSmooth(0.20f);
			}
		} else {
			if (gates[i]) {
				// Gate is on and not active = red
				lights[GATE_LIGHTS + i * 2].setBrightnessSmooth(0.0f);
				lights[GATE_LIGHTS + i * 2 + 1].setBrightnessSmooth(1.0f);
			} else {
				// Gate is off and not active = black
				lights[GATE_LIGHTS + i * 2].setBrightnessSmooth(0.0f);
				lights[GATE_LIGHTS + i * 2 + 1].setBrightnessSmooth(0.0f);
			}			
		}
		
	}

	bool gatesOn = (running && gates[index]);
	if (gateMode == TRIGGER) {
		gatesOn = gatesOn && pulse;
	} else if (gateMode == RETRIGGER) {
		gatesOn = gatesOn && !pulse;
	}

	// Outputs
	outputs[GATES_OUTPUT].setVoltage(gatesOn ? 10.0f : 0.0f);
	lights[RUNNING_LIGHT].setBrightness(running);
	lights[RESET_LIGHT].setBrightnessSmooth(resetTrigger.isHigh());
	lights[GATES_LIGHT].setBrightnessSmooth(pulse);


	// Set the output pitches 

	// Count the number of active ports in P2-P6
	int portCount = 0;
	for (int i = 1; i < NUM_PITCHES - 1; i++) {
		if (outputs[PITCH_OUTPUT + i].isConnected()) {
			portCount++;
		}
	}

	// No active ports in P2-P6, so must be poly
	if (portCount == 0) {
		outputs[PITCH_OUTPUT].setChannels(6);
		for (int i = 0; i < NUM_PITCHES; i++) {
			outputs[PITCH_OUTPUT].setVoltage(pitches[index][i], i);
		}
	} else {
		for (int i = 0; i < NUM_PITCHES; i++) {
			outputs[PITCH_OUTPUT + i].setVoltage(pitches[index][i]);
		}
	}
	
}

struct ProgressWidget : ModuleWidget {

	ProgressWidget(Progress *module) {
		
		setModule(module);
		setPanel(SVG::load(asset::plugin(pluginInstance, "res/Progress.svg")));
		UI ui;
		
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 0, 0, true, false), module, Progress::CLOCK_PARAM));
		addParam(createParam<AHButton>(ui.getPosition(UI::BUTTON, 1, 0, true, false), module, Progress::RUN_PARAM));
		addChild(createLight<MediumLight<GreenLight>>(ui.getPosition(UI::LIGHT, 1, 0, true, false), module, Progress::RUNNING_LIGHT));
		addParam(createParam<AHButton>(ui.getPosition(UI::BUTTON, 2, 0, true, false), module, Progress::RESET_PARAM));
		addChild(createLight<MediumLight<GreenLight>>(ui.getPosition(UI::LIGHT, 2, 0, true, false), module, Progress::RESET_LIGHT));
		addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 3, 0, true, false), module, Progress::STEPS_PARAM));
		addChild(createLight<MediumLight<GreenLight>>(ui.getPosition(UI::LIGHT, 4, 0, true, false), module, Progress::GATES_LIGHT));

	//	static const float portX[13] = {20, 58, 96, 135, 173, 212, 250, 288, 326, 364, 402, 440, 478};
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 1, true, false), module, Progress::CLOCK_INPUT));
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 1, 1, true, false), module, Progress::EXT_CLOCK_INPUT));
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 1, true, false), module, Progress::RESET_INPUT));
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 3, 1, true, false), module, Progress::STEPS_INPUT));

		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 1, true, false), module, Progress::KEY_INPUT));
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 5, 1, true, false), module, Progress::MODE_INPUT));

		for (int i = 0; i < 3; i++) {
			addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 7 + i, 0, true, false), module, Progress::PITCH_OUTPUT + i));
		}	

		for (int i = 0; i < 3; i++) {
			addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 7 + i, 1, true, false), module, Progress::PITCH_OUTPUT + 3 + i));
		}

		for (int i = 0; i < 8; i++) {
			AHKnobNoSnap *rootW = createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, i + 1, 4, true, true), module, Progress::ROOT_PARAM + i);
			AHParamWidget::set<AHKnobNoSnap>(rootW, Progress::ROOT_TYPE, i);
			addParam(rootW);
			
			AHKnobNoSnap *chordW = createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, i + 1, 5, true, true), module, Progress::CHORD_PARAM + i);
			AHParamWidget::set<AHKnobNoSnap>(chordW, Progress::CHORD_TYPE, i);
			addParam(chordW);

			AHKnobSnap *invW = createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, i + 1, 6, true, true), module, Progress::INV_PARAM + i);
			AHParamWidget::set<AHKnobSnap>(invW, Progress::INV_TYPE, i);
			addParam(invW);

			addParam(createParam<AHButton>(ui.getPosition(UI::BUTTON, i + 1, 7, true, true), module, Progress::GATE_PARAM + i));
			addChild(createLight<MediumLight<GreenRedLight>>(ui.getPosition(UI::LIGHT, i + 1, 7, true, true), module, Progress::GATE_LIGHTS + i * 2));
					
			addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, i + 1, 5, true, false), module, Progress::GATE_OUTPUT + i));
		}

		addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 9, 5, true, false), module, Progress::GATES_OUTPUT));
		
		if (module != NULL) {
			StateDisplay *display = createWidget<StateDisplay>(Vec(0, 135));
			display->module = module;
			display->box.size = Vec(100, 140);
			addChild(display);
		}

	}

	void appendContextMenu(Menu *menu) override {

		Progress *progress = dynamic_cast<Progress*>(module);
		assert(progress);

		struct GateModeItem : MenuItem {
			Progress *module;
			Progress::GateMode gateMode;
			void onAction(const event::Action &e) override {
				module->gateMode = gateMode;
			}
		};

		struct GateModeMenu : MenuItem {
			Progress *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<Progress::GateMode> modes = {Progress::TRIGGER, Progress::RETRIGGER, Progress::CONTINUOUS};
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

		menu->addChild(construct<MenuLabel>());
		GateModeMenu *item = createMenuItem<GateModeMenu>("Gate Mode");
		item->module = progress;
		menu->addChild(item);

     }
	 
};

Model *modelProgress = createModel<Progress, ProgressWidget>("Progress");


