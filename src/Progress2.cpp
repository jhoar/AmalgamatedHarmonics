#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;


struct PChord {

	void setRoot(float v) {
		if (inRoot != v) {
			dirty = true;
		}
		inRoot = v;
	}

	void setChord(float v) {
		if (inChord != v) {
			dirty = true;
		}
		inChord = v;
	}

	void setDegree(float v) {
		if (inDegree != v) {
			dirty = true;
		}
		inDegree = v;
	}

	void setQuality(float v) {
		if (inQuality != v) {
			dirty = true;
		}
		inQuality = v;
	}

	void setInversion(float v) {
		if (inInversion != v) {
			dirty = true;
		}
		inInversion = v;
	}

	// Input analog value
	float inRoot;
	float inChord;
	float inDegree;
	float inQuality;
	float inInversion;

	// Intepreted value
	int root;
	int chord;
	int inversion;	
	int degree;
	int quality;

	float pitches[6];

	bool gate;

	bool dirty;

};

struct Progress2 : core::AHModule {

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
		PITCH_OUTPUT,
		POLYGATE_OUTPUT,
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
			
	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// running
		json_object_set_new(rootJ, "running", json_boolean(running));

		// gates
		json_t *gatesJ = json_array();
		for (int i = 0; i < 8; i++) {
			json_t *gateJ = json_integer((int) chords[i].gate);
			json_array_append_new(gatesJ, gateJ);
		}
		json_object_set_new(rootJ, "gates", gatesJ);

		// gateMode
		json_t *gateModeJ = json_integer((int) gateMode);
		json_object_set_new(rootJ, "gateMode", gateModeJ);

		// offset
		json_t *offsetJ = json_integer((int) offset);
		json_object_set_new(rootJ, "offset", offsetJ);

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
					chords[i].gate = !!json_integer_value(gateJ);
			}
		}

		// gateMode
		json_t *gateModeJ = json_object_get(rootJ, "gateMode");
		if (gateModeJ)
			gateMode = (GateMode)json_integer_value(gateModeJ);

		// offset
		json_t *offsetJ = json_object_get(rootJ, "offset");
		if (offsetJ)
			offset = json_integer_value(offsetJ);

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

	float resetLight = 0.0f;
	float gateLight = 0.0f;
	float stepLights[8] = {};

	enum GateMode {
		TRIGGER,
		RETRIGGER,
		CONTINUOUS,
	};
	GateMode gateMode = CONTINUOUS;
	
	int offset = 24; 	// Repeated notes in chord and expressed in the chord definition as being transposed 2 octaves lower. 
						// When played this offset needs to be removed (or the notes removed, or the notes transposed to an octave higher)
	
	PChord chords[8];

	int currMode;
	int currKey;
	int prevMode = -1;
	int prevKey = -1;
	bool modeMode = false;
	bool prevModeMode = false;
	
	bool changeRepeat = false;

	void onReset() override {
		for (int i = 0; i < 8; i++) {
			chords[i].gate = true;
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

void Progress2::step() {
	
	core::AHModule::step();
	
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
	bool update = false;

	// Updated repeat mode
	if (changeRepeat) {
		update = true;
		changeRepeat = false;
	}

	// index is our current step
	if (inputs[KEY_INPUT].isConnected()) {
		currKey = music::getKeyFromVolts(inputs[KEY_INPUT].getVoltage());
		haveRoot = true;
	}

	if (inputs[MODE_INPUT].isConnected()) {
		currMode = music::getModeFromVolts(inputs[MODE_INPUT].getVoltage());	
		haveMode = true;
	}
	
	modeMode = haveRoot && haveMode;
	
	if (modeMode && ((prevMode != currMode) || (prevKey != currKey))) { // Input changes so force re-read
	 	for (int step = 0; step < 8; step++) {
			update = true;
		}
		
		prevMode = currMode;
		prevKey = currKey;	
	}

	if (prevModeMode != modeMode) { // Switching mode, so reset history to ensure re-read on return
		update = true;
	}

	// Update
	for (int step = 0; step < 8; step++) {

		chords[step].setDegree(params[CHORD_PARAM + step].getValue());
		chords[step].setQuality(params[ROOT_PARAM + step].getValue());
		chords[step].setChord(params[CHORD_PARAM + step].getValue());
		chords[step].setRoot(params[ROOT_PARAM + step].getValue());
		chords[step].setInversion(params[INV_PARAM + step].getValue());

		if (chords[step].dirty || update) {

			chords[step].dirty = false;

			chords[step].root  = round(rescale(fabs(chords[step].inRoot), 0.0f, 10.0f, 0.0f, music::NUM_NOTES - 1)); // Param range is 0 to 10, mapped to 0 to 11
			chords[step].chord = round(rescale(fabs(chords[step].inChord), 0.0f, 10.0f, 1.0f, 98.0f)); // Param range is 0 to 10		
			chords[step].degree = round(rescale(fabs(chords[step].inDegree), 0.0f, 10.0f, 0.0f, music::NUM_DEGREES - 1));
			chords[step].inversion = (int)chords[step].inInversion;
		
			// Update if we are in Mode mode
			if (modeMode) {			
			
				// Root and chord can get updated
				// From the input root, mode and degree, we can get the root chord note and quality (Major,Minor,Diminshed)
				music::getRootFromMode(currMode, currKey, chords[step].degree, &chords[step].root, &chords[step].quality);

				// Now get the actual chord from the main list
				switch(chords[step].quality) {
					case music::MAJ: 
						chords[step].chord = round(rescale(fabs(chords[step].inQuality), 0.0f, 10.0f, 1.0f, 70.0f)); 
						break;
					case music::MIN: 
						chords[step].chord = round(rescale(fabs(chords[step].inQuality), 0.0f, 10.0f, 71.0f, 90.0f));
						break;
					case music::DIM: 
						chords[step].chord = round(rescale(fabs(chords[step].inQuality), 0.0f, 10.0f, 91.0f, 98.0f));
						break;		
				}

			} 

			int *chordArray;

			// Get the array of pitches based on the inversion
			switch(chords[step].inversion) {
				case music::ROOT:  		chordArray = music::ChordTable[chords[step].chord].root; 	break;
				case music::FIRST_INV:  chordArray = music::ChordTable[chords[step].chord].first; 	break;
				case music::SECOND_INV: chordArray = music::ChordTable[chords[step].chord].second;	break;
				default: chordArray = music::ChordTable[chords[step].chord].root;
			}
			
			for (int j = 0; j < NUM_PITCHES; j++) {

				// Set the pitches for this step. If the chord has less than 6 notes, the empty slots are
				// filled with repeated notes. These notes are identified by a  24 semi-tome negative
				// offset. 
				if (chordArray[j] < 0) {
					int off = offset;
					if (offset == 0) { // if offset = 0, randomise offset per note
						off = (rand() % 3 + 1) * 12;
					}
					chords[step].pitches[j] = music::getVoltsFromPitch(chordArray[j] + off,chords[step].root);			
				} else {
					chords[step].pitches[j] = music::getVoltsFromPitch(chordArray[j],chords[step].root);
				}	
			}
		}
	}
	
	// Remember mode
	prevModeMode = modeMode;

	// So, after all that, we calculate the pitch output
	bool pulse = gatePulse.process(delta);
	
	// Gate buttons
	for (int i = 0; i < 8; i++) {
		if (gateTriggers[i].process(params[GATE_PARAM + i].getValue())) {
			chords[i].gate = !chords[i].gate;
		}
		
		bool gateOn = (running && i == index && chords[i].gate);
		if (gateMode == TRIGGER) {
			gateOn = gateOn && pulse;
		} else if (gateMode == RETRIGGER) {
			gateOn = gateOn && !pulse;
		}
		
		outputs[GATE_OUTPUT + i].setVoltage(gateOn ? 10.0f : 0.0f);	
		
		if (i == index) {
			if (chords[i].gate) {
				// Gate is on and active = flash green
				lights[GATE_LIGHTS + i * 2].setBrightnessSmooth(1.0f);
				lights[GATE_LIGHTS + i * 2 + 1].setBrightnessSmooth(0.0f);
			} else {
				// Gate is off and active = flash dull yellow
				lights[GATE_LIGHTS + i * 2].setBrightnessSmooth(0.20f);
				lights[GATE_LIGHTS + i * 2 + 1].setBrightnessSmooth(0.20f);
			}
		} else {
			if (chords[i].gate) {
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

	bool gatesOn = (running && chords[index].gate);
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
	outputs[PITCH_OUTPUT].setChannels(6);
	outputs[PITCH_OUTPUT + 1].setChannels(6);
	for (int i = 0; i < NUM_PITCHES; i++) {
		outputs[PITCH_OUTPUT].setVoltage(chords[index].pitches[i], i);
		outputs[PITCH_OUTPUT + 1].setVoltage(10.0, i);
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

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 4, 0, true, false), module, Progress2::KEY_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 0, true, false), module, Progress2::MODE_INPUT));

		addChild(createLight<MediumLight<GreenLight>>(gui::getPosition(gui::LIGHT, 0, 5, true, false), module, Progress2::GATES_LIGHT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 6, 0, true, false), module, Progress2::GATES_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 7, 0, true, false), module, Progress2::PITCH_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 8, 0, true, false), module, Progress2::POLYGATE_OUTPUT));

		for (int i = 0; i < 8; i++) {
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, i + 1, 4, true, true), module, Progress2::ROOT_PARAM + i));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, i + 1, 5, true, true), module, Progress2::CHORD_PARAM + i));
			addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, i + 1, 6, true, true), module, Progress2::INV_PARAM + i));

			addParam(createParam<gui::AHButton>(gui::getPosition(gui::BUTTON, i + 1, 8, true, true, 0.0f, -4.0f), module, Progress2::GATE_PARAM + i));
			addChild(createLight<MediumLight<GreenRedLight>>(gui::getPosition(gui::LIGHT, i + 1, 8, true, true, 0.0f, -4.0f), module, Progress2::GATE_LIGHTS + i * 2));
					
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, i + 1, 5, true, false), module, Progress2::GATE_OUTPUT + i));
		}
		
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
				module->offset = offset;
				module->changeRepeat = true;
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
					OffsetItem *item = createMenuItem<OffsetItem>(names[i], CHECKMARK(module->offset == offsets[i]));
					item->module = module;
					item->offset = offsets[i];
					menu->addChild(item);
				}
				return menu;
			}
		};


		menu->addChild(construct<MenuLabel>());
		GateModeMenu *item = createMenuItem<GateModeMenu>("Gate Mode");
		item->module = progress;
		menu->addChild(item);

		OffsetMenu *offsetItem = createMenuItem<OffsetMenu>("Repeat Notes");
		offsetItem->module = progress;
		menu->addChild(offsetItem);

     }
	 
};

Model *modelProgress2 = createModel<Progress2, Progress2Widget>("Progress2");
