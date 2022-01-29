#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct Progress : core::AHModule {

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

	Progress() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) { 

		configParam(CLOCK_PARAM, -2.0, 6.0, 2.0, "Clock tempo", " bpm", 2.f, 60.f);
		configParam(RUN_PARAM, 0.0, 1.0, 0.0, "Run");
		configParam(RESET_PARAM, 0.0, 1.0, 0.0, "Reset");
		configParam(STEPS_PARAM, 1.0, 8.0, 8.0, "Steps");

		for (int i = 0; i < 8; i++) {
			configParam(ROOT_PARAM + i, 0.0, 10.0, 0.0, "Root note");
			paramQuantities[ROOT_PARAM + i]->description = "Root note [degree of scale]";

			configParam(CHORD_PARAM + i, 0.0, 10.0, 0.0, "Chord");

			configParam(INV_PARAM + i, 0.0, 2.0, 0.0, "Inversion");
			paramQuantities[INV_PARAM + i]->description = "Root, first of second inversion";

			configParam(GATE_PARAM + i, 0.0, 1.0, 0.0, "Gate active");
		}

		onReset();

	}

	void process(const ProcessArgs &args) override;

	enum ParamType {
		ROOT_TYPE,
		CHORD_TYPE,
		INV_TYPE
	};

	void receiveEvent(core::ParamEvent e) override {
		if (receiveEvents && e.pType != -1) { // AHParamWidgets that are no config through set<>() have a pType of -1
			if (modeMode) {
				paramState = "> " + 
					music::noteNames[currRoot[e.pId]] + 
					music::ChordTable[currChord[e.pId]].name + " " +  
					music::inversionNames[currInv[e.pId]] + " " + "[" + 
					music::DegreeString[currMode][currDegree[e.pId]] + "]";
			} else {
				paramState = "> " + 
					music::noteNames[currRoot[e.pId]] + 
					music::ChordTable[currChord[e.pId]].name + " " +  
					music::inversionNames[currInv[e.pId]];
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
		this->gatePulse.trigger(digital::TRIGGER);
	}

};

void Progress::process(const ProcessArgs &args) {

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
			phase += clockTime * args.sampleTime;
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
		currKey = music::getKeyFromVolts(fRoot);
		haveRoot = true;
	}

	if (inputs[MODE_INPUT].isConnected()) {
		float fMode = inputs[MODE_INPUT].getVoltage();
		currMode = music::getModeFromVolts(fMode);	
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
				currDegree[step] = round(rescale(fabs(currDegreeInput[step]), 0.0f, 10.0f, 0.0f, music::NUM_DEGREES - 1)); 

				// From the input root, mode and degree, we can get the root chord note and quality (Major,Minor,Diminshed)
				music::getRootFromMode(currMode,currKey,currDegree[step],&currRoot[step],&currQuality[step]);

				// Now get the actual chord from the main list
				switch(currQuality[step]) {
					case music::Quality::MAJ: 
						currChord[step] = round(rescale(fabs(currQualityInput[step]), 0.0f, 10.0f, 1.0f, 70.0f)); 
						break;
					case music::Quality::MIN: 
						currChord[step] = round(rescale(fabs(currQualityInput[step]), 0.0f, 10.0f, 71.0f, 90.0f));
						break;
					case music::Quality::DIM: 
						currChord[step] = round(rescale(fabs(currQualityInput[step]), 0.0f, 10.0f, 91.0f, 98.0f));
						break;		
				}

			}

		} else {
			
			// Chord Mode
			
			// If anything has changed, recalculate output for that step
			if (prevRootInput[step] != currRootInput[step]) {
				prevRootInput[step] = currRootInput[step];
				currRoot[step] = round(rescale(fabs(currRootInput[step]), 0.0f, 10.0f, 0.0f, music::Notes::NUM_NOTES - 1)); // Param range is 0 to 10, mapped to 0 to 11
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
				case music::Inversion::ROOT:		chordArray = music::ChordTable[currChord[step]].root; 	break;
				case music::Inversion::FIRST_INV:	chordArray = music::ChordTable[currChord[step]].first; 	break;
				case music::Inversion::SECOND_INV:	chordArray = music::ChordTable[currChord[step]].second;	break;
				default: chordArray = music::ChordTable[currChord[step]].root;
			}

			for (int j = 0; j < NUM_PITCHES; j++) {

				// Set the pitches for this step. If the chord has less than 6 notes, the empty slots are
				// filled with repeated notes. These notes are identified by a  24 semi-tome negative
				// offset. We correct for that offset now, pitching thaem back into the original octave.
				// They could be pitched into the octave above (or below)
				if (chordArray[j] < 0) {
					pitches[step][j] = music::getVoltsFromPitch(chordArray[j] + offset,currRoot[step]);
				} else {
					pitches[step][j] = music::getVoltsFromPitch(chordArray[j],currRoot[step]);
				}
			}
		}
	}

	bool pulse = gatePulse.process(args.sampleTime);

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
				lights[GATE_LIGHTS + i * 2].setSmoothBrightness(1.0f, args.sampleTime);
				lights[GATE_LIGHTS + i * 2 + 1].setSmoothBrightness(0.0f, args.sampleTime);
			} else {
				// Gate is off and active = flash dull yellow
				lights[GATE_LIGHTS + i * 2].setSmoothBrightness(0.20f, args.sampleTime);
				lights[GATE_LIGHTS + i * 2 + 1].setSmoothBrightness(0.20f, args.sampleTime);
			}
		} else {
			if (gates[i]) {
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

	bool gatesOn = (running && gates[index]);
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
	for (int i = 0; i < NUM_PITCHES; i++) {
		outputs[PITCH_OUTPUT + i].setVoltage(pitches[index][i]);
	}

}

struct ProgressWidget : ModuleWidget {

	template <typename T = gui::AHParamWidget>
	static ParamWidget *setRoot(float x, float y, int pos, Progress *module) {	
		T *w = createParamCentered<T>(Vec(x, y), module, Progress::ROOT_PARAM + pos);
		gui::AHParamWidget::set<T>(w, Progress::ROOT_TYPE, pos);
		return w;
	}

	template <typename T = gui::AHParamWidget>
	static ParamWidget *setChord(float x, float y, int pos, Progress *module) {	
		T *w = createParamCentered<T>(Vec(x, y), module, Progress::CHORD_PARAM + pos);
		gui::AHParamWidget::set<T>(w, Progress::CHORD_TYPE, pos);
		return w;
	}

	template <typename T = gui::AHParamWidget>
	static ParamWidget *setInv(float x, float y, int pos, Progress *module) {	
		T *w = createParamCentered<T>(Vec(x, y), module, Progress::INV_PARAM + pos);
		gui::AHParamWidget::set<T>(w, Progress::INV_TYPE, pos);
		return w;
	}

	ProgressWidget(Progress *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Progress.svg")));

		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(68.661, 57.727), module, Progress::CLOCK_PARAM));
		addParam(createParamCentered<gui::AHButton>(Vec(104.774, 57.727), module, Progress::RUN_PARAM));
		addParam(createParamCentered<gui::AHButton>(Vec(139.569, 57.727), module, Progress::RESET_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(174.866, 57.727), module, Progress::STEPS_PARAM));

		addParam(ProgressWidget::setRoot<gui::AHKnobNoSnap>(68.661, 211.337, 0, module));
		addParam(ProgressWidget::setRoot<gui::AHKnobNoSnap>(104.774, 211.337, 1, module));
		addParam(ProgressWidget::setRoot<gui::AHKnobNoSnap>(139.569, 211.337, 2, module));
		addParam(ProgressWidget::setRoot<gui::AHKnobNoSnap>(174.866, 211.337, 3, module));
		addParam(ProgressWidget::setRoot<gui::AHKnobNoSnap>(209.682, 211.337, 4, module));
		addParam(ProgressWidget::setRoot<gui::AHKnobNoSnap>(244.663, 211.337, 5, module));
		addParam(ProgressWidget::setRoot<gui::AHKnobNoSnap>(279.882, 211.337, 6, module));
		addParam(ProgressWidget::setRoot<gui::AHKnobNoSnap>(314.661, 211.337, 7, module));

		addParam(ProgressWidget::setChord<gui::AHKnobNoSnap>(68.661, 246.654, 0, module));
		addParam(ProgressWidget::setChord<gui::AHKnobNoSnap>(104.774, 246.654, 1, module));
		addParam(ProgressWidget::setChord<gui::AHKnobNoSnap>(139.569, 246.654, 2, module));
		addParam(ProgressWidget::setChord<gui::AHKnobNoSnap>(174.866, 246.654, 3, module));
		addParam(ProgressWidget::setChord<gui::AHKnobNoSnap>(209.682, 246.654, 4, module));
		addParam(ProgressWidget::setChord<gui::AHKnobNoSnap>(244.663, 246.654, 5, module));
		addParam(ProgressWidget::setChord<gui::AHKnobNoSnap>(279.882, 246.654, 6, module));
		addParam(ProgressWidget::setChord<gui::AHKnobNoSnap>(314.661, 246.654, 7, module));

		addParam(ProgressWidget::setInv<gui::AHKnobSnap>(68.661, 281.97, 0, module));
		addParam(ProgressWidget::setInv<gui::AHKnobSnap>(104.774, 281.97, 1, module));
		addParam(ProgressWidget::setInv<gui::AHKnobSnap>(139.569, 281.97, 2, module));
		addParam(ProgressWidget::setInv<gui::AHKnobSnap>(174.866, 281.97, 3, module));
		addParam(ProgressWidget::setInv<gui::AHKnobSnap>(209.682, 281.97, 4, module));
		addParam(ProgressWidget::setInv<gui::AHKnobSnap>(244.663, 281.97, 5, module));
		addParam(ProgressWidget::setInv<gui::AHKnobSnap>(279.882, 281.97, 6, module));
		addParam(ProgressWidget::setInv<gui::AHKnobSnap>(314.661, 281.97, 7, module));

		addParam(createParamCentered<gui::AHButton>(Vec(68.661, 317.287), module, Progress::GATE_PARAM + 0));
		addParam(createParamCentered<gui::AHButton>(Vec(104.774, 317.287), module, Progress::GATE_PARAM + 1));
		addParam(createParamCentered<gui::AHButton>(Vec(139.569, 317.287), module, Progress::GATE_PARAM + 2));
		addParam(createParamCentered<gui::AHButton>(Vec(174.866, 317.287), module, Progress::GATE_PARAM + 3));
		addParam(createParamCentered<gui::AHButton>(Vec(209.682, 317.287), module, Progress::GATE_PARAM + 4));
		addParam(createParamCentered<gui::AHButton>(Vec(244.663, 317.287), module, Progress::GATE_PARAM + 5));
		addParam(createParamCentered<gui::AHButton>(Vec(279.882, 317.287), module, Progress::GATE_PARAM + 6));
		addParam(createParamCentered<gui::AHButton>(Vec(314.661, 317.287), module, Progress::GATE_PARAM + 7));

		addInput(createInputCentered<gui::AHPort>(Vec(209.682, 57.727), module, Progress::KEY_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(68.661, 98.014), module, Progress::CLOCK_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(104.774, 98.014), module, Progress::EXT_CLOCK_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(139.569, 98.014), module, Progress::RESET_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(174.866, 98.014), module, Progress::STEPS_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(209.682, 98.014), module, Progress::MODE_INPUT));

		addOutput(createOutputCentered<gui::AHPort>(Vec(244.663, 57.632), module, Progress::PITCH_OUTPUT + 0));
		addOutput(createOutputCentered<gui::AHPort>(Vec(279.882, 57.632), module, Progress::PITCH_OUTPUT + 1));
		addOutput(createOutputCentered<gui::AHPort>(Vec(314.086, 57.632), module, Progress::PITCH_OUTPUT + 2));
		addOutput(createOutputCentered<gui::AHPort>(Vec(244.663, 97.92), module, Progress::PITCH_OUTPUT + 3));
		addOutput(createOutputCentered<gui::AHPort>(Vec(279.882, 97.92), module, Progress::PITCH_OUTPUT + 4));
		addOutput(createOutputCentered<gui::AHPort>(Vec(314.602, 97.92), module, Progress::PITCH_OUTPUT + 5));

		addOutput(createOutputCentered<gui::AHPort>(Vec(68.661, 343.501), module, Progress::GATE_OUTPUT + 0));
		addOutput(createOutputCentered<gui::AHPort>(Vec(104.774, 343.501), module, Progress::GATE_OUTPUT + 1));
		addOutput(createOutputCentered<gui::AHPort>(Vec(139.569, 343.501), module, Progress::GATE_OUTPUT + 2));
		addOutput(createOutputCentered<gui::AHPort>(Vec(174.866, 343.501), module, Progress::GATE_OUTPUT + 3));
		addOutput(createOutputCentered<gui::AHPort>(Vec(209.682, 343.501), module, Progress::GATE_OUTPUT + 4));
		addOutput(createOutputCentered<gui::AHPort>(Vec(244.663, 343.501), module, Progress::GATE_OUTPUT + 5));
		addOutput(createOutputCentered<gui::AHPort>(Vec(279.882, 343.501), module, Progress::GATE_OUTPUT + 6));
		addOutput(createOutputCentered<gui::AHPort>(Vec(314.602, 343.501), module, Progress::GATE_OUTPUT + 7));

		addOutput(createOutputCentered<gui::AHPort>(Vec(358.661, 343.501), module, Progress::GATES_OUTPUT));

		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(104.774, 57.727), module, Progress::RUNNING_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(139.569, 57.727), module, Progress::RESET_LIGHT));

		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(68.661, 317.287), module, Progress::GATE_LIGHTS + 0));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(104.774, 317.287), module, Progress::GATE_LIGHTS + 2));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(139.569, 317.287), module, Progress::GATE_LIGHTS + 4));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(174.866, 317.287), module, Progress::GATE_LIGHTS + 6));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(209.682, 317.287), module, Progress::GATE_LIGHTS + 8));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(244.663, 317.287), module, Progress::GATE_LIGHTS + 10));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(279.882, 317.287), module, Progress::GATE_LIGHTS + 12));
		addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(314.661, 317.287), module, Progress::GATE_LIGHTS + 14));

		addChild(createLightCentered<MediumLight<RedLight>>(Vec(30.071, 343.501), module, Progress::GATES_LIGHT));

		if (module != NULL) {
			gui::StateDisplay *display = createWidget<gui::StateDisplay>(Vec(0, 135));
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
			void onAction(const rack::event::Action &e) override {
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

