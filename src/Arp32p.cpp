#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct Pattern2 {
	
	std::vector<int> notes;
	unsigned int nNotes = 0;

	unsigned int patternLength = 0;
	int stepSize = 0;
	unsigned int stepScale = 0;
	unsigned int patternOffset = 0;
	bool repeatLast = false;

	unsigned int index = 0;

	unsigned int MAJOR[7] = {0,2,4,5,7,9,11};
	unsigned int MINOR[7] = {0,2,3,5,7,8,10};
		
	virtual const std::string & getName() = 0;

	virtual void initialise(unsigned int _length, unsigned int _scale, int _size, unsigned int _offset, bool _repeat) {
		patternLength = _length;
		stepSize = _size;
		stepScale = _scale;
		patternOffset = _offset;
		repeatLast = _repeat;
	};

	void advance() {
		// std::cout << "ADV " << index << std::endl;
		index++;
	};

	void reset() {
		// std::cout << "RES" << std::endl;
		index = patternOffset;
	}

	unsigned int getOffset() {
		// std::cout << "OUT " << index << " " << notes[index] << std::endl;
		return notes[index];
	}

	bool isPatternFinished() {
		// std::cout << "FIN " << index << " " << nNotes << std::endl;
		return (index >= nNotes - 1); 
	}

	int getMajor(int count) {
		int i = abs(count);
		int sign = (count < 0) ? -1 : (count > 0);
		return sign * ((i / 7) * 12 + MAJOR[i % 7]);
	}

	int getMinor(int count) {
		int i = abs(count);
		int sign = (count < 0) ? -1 : (count > 0);
		return sign * ((i / 7) * 12 + MINOR[i % 7]);
	}

	void randomize() {
		int length = nNotes - patternOffset;
		int p1 = (rand() % length) + patternOffset;
		int p2 = (rand() % length) + patternOffset;
		int tries = 0;

		while (p1 == p2 && tries < 5) { // Make some effort to change the sequence, break after 5 attempts
			p2 = (rand() % length) + patternOffset;
			tries++;
		}

		// std::cout << "RND " << length << " " << p1 << " " << p2 << " "; 

		unsigned int t = notes[p1];
		notes[p1] = notes[p2];
		notes[p2] = t;

		// for (int i = 0; i < nPitches; i++) {
		// 	std::cout << indexes[i];
		// }

		// std::cout << std::endl;

	}

};

struct DivergePattern2 : Pattern2 {

	const std::string name = "Diverge";

	const std::string & getName() override {
		return name;
	};

	void initialise(unsigned int _length, unsigned int _scale, int _size, unsigned int _offset, bool _repeat) override {

		Pattern2::initialise(_length, _scale, _size, _offset, _repeat);

		// std::cout << name;
		// std::cout << " DEF ";
		notes.clear();

		for (unsigned int i = 0; i < patternLength; i++) {
			
			int n;
			switch(stepScale) {
				case 0: n = i * stepSize; break;
				case 1: n = getMajor(i * stepSize); break;
				case 2: n = getMinor(i * stepSize); break;
				default:
					n = i * stepSize; break;
			}

			notes.push_back(n);
			// std::cout << n << " ";

		}

		nNotes = notes.size();
		patternOffset = patternOffset % nNotes;
		index = patternOffset;
		// std::cout << " NP=" << nNotes << " -> " << index << std::endl;

	}

};

struct ConvergePattern2 : Pattern2 {

	const std::string name = "Converge";

	const std::string & getName() override {
		return name;
	};

	void initialise(unsigned int _length, unsigned int _scale, int _size, unsigned int _offset, bool _repeat) override {

		Pattern2::initialise(_length, _scale, _size, _offset, _repeat);

		// std::cout << name;
		// std::cout << " DEF ";
		notes.clear();

		for (int i = patternLength - 1; i >= 0; i--) {
			
			int n;
			switch(stepScale) {
				case 0: n = i * stepSize; break;
				case 1: n = getMajor(i * stepSize); break;
				case 2: n = getMinor(i * stepSize); break;
				default:
					n = i * stepSize; break;
			}

			notes.push_back(n);
			// std::cout << n << " ";

		}

		nNotes = notes.size();
		patternOffset = patternOffset % nNotes;
		index = patternOffset;
		// std::cout << " NP=" << nNotes << " -> " << index << std::endl;

	}

};

struct ReturnPattern2 : Pattern2 {

	const std::string name = "Return";

	const std::string & getName() override {
		return name;
	};

	void initialise(unsigned int _length, unsigned int _scale, int _size, unsigned int _offset, bool _repeat) override {

		Pattern2::initialise(_length, _scale, _size, _offset, _repeat);

		// std::cout << name;
		// std::cout << " DEF ";
		notes.clear();

		for (unsigned int i = 0; i < patternLength; i++) {
			
			int n;
			switch(stepScale) {
				case 0: n = i * stepSize; break;
				case 1: n = getMajor(i * stepSize); break;
				case 2: n = getMinor(i * stepSize); break;
				default:
					n = i * stepSize; break;
			}

			notes.push_back(n);
			// std::cout << n << " ";

		}

		int end = repeatLast ? 0 : 1;

		for (int i = patternLength - 2; i >= end; i--) {
			
			int n;
			switch(stepScale) {
				case 0: n = i * stepSize; break;
				case 1: n = getMajor(i * stepSize); break;
				case 2: n = getMinor(i * stepSize); break;
				default:
					n = i * stepSize; break;
			}

			notes.push_back(n);
			// std::cout << n << " ";
		}

		nNotes = notes.size();
		patternOffset = patternOffset % nNotes;
		index = patternOffset;
		// std::cout << " NP=" << nNotes << " -> " << index << std::endl;

	}

};

struct BouncePattern2 : Pattern2 {

	const std::string name = "Bounce";

	const std::string & getName() override {
		return name;
	};

	void initialise(unsigned int _length, unsigned int _scale, int _size, unsigned int _offset, bool _repeat) override {

		Pattern2::initialise(_length, _scale, _size, _offset, _repeat);

		// std::cout << name;
		// std::cout << " DEF ";
		notes.clear();

		for (int i = patternLength - 1; i >= 0; i--) {
			
			int n;
			switch(stepScale) {
				case 0: n = i * stepSize; break;
				case 1: n = getMajor(i * stepSize); break;
				case 2: n = getMinor(i * stepSize); break;
				default:
					n = i * stepSize; break;
			}

			notes.push_back(n);
			// std::cout << n << " ";

		}

		int end = repeatLast ? 0 : 1;

		for (unsigned int i = 1; i < patternLength - end; i++) {
			
			int n;
			switch(stepScale) {
				case 0: n = i * stepSize; break;
				case 1: n = getMajor(i * stepSize); break;
				case 2: n = getMinor(i * stepSize); break;
				default:
					n = i * stepSize; break;
			}

			notes.push_back(n);
			// std::cout << n << " ";

		}

		nNotes = notes.size();
		patternOffset = patternOffset % nNotes;
		index = patternOffset;
		// std::cout << " NP=" << nNotes << " -> " << index << std::endl;

	}

};

struct NotePattern2 : Pattern2 {

	void initialise(unsigned int _length, unsigned int _scale, int _size, unsigned int _offset, bool _repeat) override {

		Pattern2::initialise(_length, _scale, _size, _offset, _repeat);

		nNotes = notes.size();
		patternOffset = patternOffset % nNotes;
		index = patternOffset;

	}

};

struct RezPattern2 : NotePattern2 {

	const std::string name = "Rez";

	const std::string & getName() override {
		return name;
	};

	RezPattern2() {
		notes.clear();
		notes.push_back(0);
		notes.push_back(12);
		notes.push_back(0);
		notes.push_back(0);
		notes.push_back(8);
		notes.push_back(0);
		notes.push_back(0);
		notes.push_back(3);		
		notes.push_back(0);
		notes.push_back(0);
		notes.push_back(3);
		notes.push_back(0);
		notes.push_back(3);
		notes.push_back(0);
		notes.push_back(8);
		notes.push_back(0);
	}

};

struct OnTheRunPattern2 : NotePattern2 {
	
	const std::string name = "On The Run";

	const std::string & getName() override {
		return name;
	};

	OnTheRunPattern2() {
		notes.clear();
		notes.push_back(0);
		notes.push_back(4);
		notes.push_back(6);
		notes.push_back(4);
		notes.push_back(9);
		notes.push_back(11);
		notes.push_back(13);
		notes.push_back(11);
	}
	
};

struct Arp32 : core::AHModule {

	const static int MAX_STEPS = 16;
	const static int MAX_DIST = 12; // Octave

	enum ParamIds {
		PATT_PARAM,
		LENGTH_PARAM,
		SIZE_PARAM,
		SCALE_PARAM,
		OFFSET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		PITCH_INPUT,
		PATT_INPUT,
		LENGTH_INPUT,
		SIZE_INPUT,
		HOLD_INPUT,
		RANDOM_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		GATE_OUTPUT,
		EOC_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Arp32() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		configParam(PATT_PARAM, 0.0, 5.0, 0.0, "Pattern"); 

		configParam(SIZE_PARAM, -24, 24, 1, "Step Size"); 
		paramQuantities[SIZE_PARAM]->description = "Size of each step in the pattern";

		configParam(LENGTH_PARAM, 1.0, 16.0, 1.0, "Number of steps in the pattern");

		configParam(OFFSET_PARAM, 0.0, 10.0, 0.0, "Start offset"); 
		paramQuantities[OFFSET_PARAM]->description = "Number of steps into the arpeggio to start";

		struct ScaleParamQuantity : engine::ParamQuantity {
			std::string getDisplayValueString() override {
				int v = (int)getValue();
				if (v == 0) {
					return "Semitone " + ParamQuantity::getDisplayValueString();
				}
				if (v == 1) {
					return "Major interval " + ParamQuantity::getDisplayValueString();
				}
				if (v == 2) {
					return "Minor interval " + ParamQuantity::getDisplayValueString();
				}
				return "Semitone (probably) " + ParamQuantity::getDisplayValueString();
			}
		};
		configParam(SCALE_PARAM, 0, 2, 0, "Step type"); 
		paramQuantities[SCALE_PARAM]->description = "Type of step: semitones or major or minor intervals"; 

		patterns.push_back(&patt_diverge);
		patterns.push_back(&patt_converge);
		patterns.push_back(&patt_return);
		patterns.push_back(&patt_bounce);
		patterns.push_back(&patt_rez);
		patterns.push_back(&patt_ontherun);

		nextPattern = patterns[0]->getName();

		onReset();
		id = rand();
		debugFlag = false;
	}

	void process(const ProcessArgs &args) override;

	void onReset() override {
		isRunning = false;
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// gateMode
		json_t *gateModeJ = json_integer((int) gateMode);
		json_object_set_new(rootJ, "gateMode", gateModeJ);

		// repeatMode
		json_t *repeatModeJ = json_boolean((bool) repeatEnd);
		json_object_set_new(rootJ, "repeatMode", repeatModeJ);

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		// gateMode
		json_t *gateModeJ = json_object_get(rootJ, "gateMode");
		if (gateModeJ) gateMode = (GateMode)json_integer_value(gateModeJ);

		// repeatMode
		json_t *repeatModeJ = json_object_get(rootJ, "repeatMode");
		if (repeatModeJ) repeatEnd = json_boolean_value(repeatModeJ);
	}

	enum GateMode {
		TRIGGER,
		RETRIGGER,
		CONTINUOUS,
	};
	GateMode gateMode = TRIGGER;

	rack::dsp::SchmittTrigger clockTrigger; // for clock
	rack::dsp::SchmittTrigger randomTrigger; // for random

	rack::dsp::PulseGenerator gatePulse;
	rack::dsp::PulseGenerator eocPulse;

	int id = 0;
	float outVolts = 0;
	float rootPitch = 0.0;
	bool isRunning = false;
	bool eoc = false;
	bool repeatEnd = false;

	unsigned int inputPat = 0;
	unsigned int inputLen = 0;
	int inputSize = 0;
	unsigned int inputScale = 0;

	int pattern = 0;
	int length = 0;
	float size = 0;
	float scale = 0;

	std::vector<Pattern2 *>patterns;

	DivergePattern2			patt_diverge; 
	ConvergePattern2 		patt_converge; 
	ReturnPattern2 			patt_return;
	BouncePattern2 			patt_bounce;
	RezPattern2 			patt_rez;
	OnTheRunPattern2		patt_ontherun;

	Pattern2 *currPatt = &patt_diverge;
	std::string nextPattern;

};

void Arp32::process(const ProcessArgs &args) {
	
	AHModule::step();

	// Wait a few steps for the inputs to flow through Rack
	if (stepX < 10) { 
		return;
	}

	// Get inputs from Rack
	float clockInput	= inputs[CLOCK_INPUT].getVoltage();
	float clockActive	= inputs[CLOCK_INPUT].isConnected();
	float randomInput	= inputs[RANDOM_INPUT].getVoltage();

	// Read param section	
	if (inputs[PATT_INPUT].isConnected()) {
		inputPat = clamp(static_cast<unsigned int>(inputs[PATT_INPUT].getVoltage()), 0, 5);
	} else {
		inputPat = params[PATT_PARAM].getValue();
	}	

	if (inputs[LENGTH_INPUT].isConnected()) {
		inputLen = clamp(static_cast<unsigned int>(inputs[LENGTH_INPUT].getVoltage()), 1, 16);
	} else {
		inputLen = params[LENGTH_PARAM].getValue();
	}	

	if (inputs[SIZE_INPUT].isConnected()) {
		inputSize = clamp(static_cast<int>(inputs[SIZE_INPUT].getVoltage()), -24, 24);
	} else {
		inputSize = params[SIZE_PARAM].getValue();
	}	

	inputScale = static_cast<int>(params[SCALE_PARAM].getValue());
	unsigned int offset = static_cast<unsigned int>(params[OFFSET_PARAM].getValue());
	int hold = digital::sgn(inputs[HOLD_INPUT].getVoltage(), 0.001);

	// Process inputs
	bool clockStatus = clockTrigger.process(clockInput);
	bool randomStatus = randomTrigger.process(randomInput);

	// Need to understand why this happens
	if (inputLen == 0) {
		if (debugEnabled(5000)) { std::cout << stepX << " " << id  << " InputLen == 0, aborting" << std::endl; }
		return; // No inputs, no music
	}

	// If there is no clock input, then force that we are not running
	if (!clockActive) {
		isRunning = false;
	}

	bool restart = false;

	// Have we been clocked?
	if (clockStatus) {

		// EOC was fired at last sequence step
		if (eoc) {
			eocPulse.trigger(digital::TRIGGER);
			eoc = false;
		}	
		
		// If we are already running, process cycle
		if (isRunning) {

			if (debugEnabled()) { std::cout << stepX << " " << id  << " Advance Cycle: " << currPatt->getOffset() << std::endl; }

			// Reached the end of the pattern?
			if (currPatt->isPatternFinished()) {

				// Trigger EOC mechanism
				eoc = true;

				if (debugEnabled()) { std::cout << stepX << " " << id  << " Finished Cycle" << std::endl; }
				restart = true;

			} 

			// Finally set the out voltage
			outVolts = clamp(rootPitch + music::SEMITONE * (float)currPatt->getOffset(), -10.0f, 10.0f);

			if (debugEnabled()) { std::cout << stepX << " " << id  << " Output V = " << outVolts << std::endl; }

			// Pulse the output gate
			gatePulse.trigger(digital::TRIGGER);

			// Completed 1 step
			currPatt->advance();

		} else {

			// Start a cycle
			restart = true;

		}

	}

	// Randomise if triggered
	if (randomStatus && isRunning && hold != -1) {
		currPatt->randomize();
	}

	// If we have been triggered, start a new sequence
	if (restart) {

		if (!hold) {

			// Read input pitch
			float inputPitch;
			if (inputs[PITCH_INPUT].isConnected()) {
				inputPitch = inputs[PITCH_INPUT].getVoltage();
			} else {
				inputPitch = 0.0;
			}

			// At the first step of the cycle
			// So this is where we tweak the cycle parameters
			pattern = inputPat;
			length = inputLen;
			size = inputSize;
			scale = inputScale;

			currPatt = patterns[pattern];

			// Save pitch
			rootPitch = inputPitch;

			if (debugEnabled()) { std::cout << stepX << " " << id  << 
				" Initiatise new Cycle: Pattern: " << currPatt->getName() << 
				" Length: " << inputLen << std::endl; 
			}

			currPatt->initialise(length, scale, size, offset, repeatEnd);

		} else {

			if (debugEnabled()) { std::cout << stepX << " " << id  << 
				" Hold new Cycle: Pattern: " << currPatt->getName() << 
				" Length: " << inputLen << std::endl; 
			}

			currPatt->reset();

		}

		// Start
		isRunning = true;

	} 

	nextPattern = patterns[inputPat]->getName();

	// Set the value
	outputs[OUT_OUTPUT].setVoltage(outVolts);

	bool gPulse = gatePulse.process(args.sampleTime);
	bool cPulse = eocPulse.process(args.sampleTime);

	bool gatesOn = isRunning;
	if (gateMode == TRIGGER) {
		gatesOn = gatesOn && gPulse;
	} else if (gateMode == RETRIGGER) {
		gatesOn = gatesOn && !gPulse;
	}

	outputs[GATE_OUTPUT].setVoltage(gatesOn ? 10.0 : 0.0);
	outputs[EOC_OUTPUT].setVoltage(cPulse ? 10.0 : 0.0);

}

struct Arp32Display : TransparentWidget {

	Arp32 *module;
	std::shared_ptr<Font> font;

	Arp32Display() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/EurostileBold.ttf"));
	}

	void draw(const DrawArgs &ctx) override {

		if (module == NULL) {
			return;
		}

		Vec pos = Vec(0, 15);

		nvgFontSize(ctx.vg, 14);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgTextLetterSpacing(ctx.vg, -1);

		nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
	
		char text[128];
		if (module->inputLen == 0) {
			snprintf(text, sizeof(text), "Error: inputLen == 0");
			nvgText(ctx.vg, pos.x + 10, pos.y, text, NULL);
		} else {
			snprintf(text, sizeof(text), "%s", module->nextPattern.c_str());
			nvgText(ctx.vg, pos.x + 10, pos.y, text, NULL);
			snprintf(text, sizeof(text), "L : %d", module->inputLen);
			nvgText(ctx.vg, pos.x + 10, pos.y + 15, text, NULL);
			switch(module->inputScale) {
				case 0: 
					snprintf(text, sizeof(text), "S : %dst", module->inputSize);
					break;
				case 1: 
					snprintf(text, sizeof(text), "S : %dM", module->inputSize);
					break;
				case 2: 
					snprintf(text, sizeof(text), "S : %dm", module->inputSize);
					break;
				default: snprintf(text, sizeof(text), "Error..."); break;
			}
			nvgText(ctx.vg, pos.x + 60, pos.y + 15, text, NULL);

		}
	}

};

struct Arp32Widget : ModuleWidget {

	std::vector<MenuOption<Arp32::GateMode>> gateOptions;
	std::vector<MenuOption<bool>> noteOptions;

	Arp32Widget(Arp32 *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Arp32p.svg")));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 0, true, false), module, Arp32::PITCH_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 0, true, false), module, Arp32::HOLD_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 0, true, false), module, Arp32::RANDOM_INPUT));

		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 0, 2, true, false), module, Arp32::PATT_PARAM));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 3, true, false), module, Arp32::PATT_INPUT));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 1, 2, true, false), module, Arp32::SIZE_PARAM));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 3, true, false), module, Arp32::SIZE_INPUT)); 
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 2, 2, true, false), module, Arp32::LENGTH_PARAM));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 3, true, false), module, Arp32::LENGTH_INPUT));


		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 4, true, false), module, Arp32::CLOCK_INPUT));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 1, 4, true, false), module, Arp32::OFFSET_PARAM));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 2, 4, true, false), module, Arp32::SCALE_PARAM));

		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 5, true, false), module, Arp32::OUT_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 5, true, false), module, Arp32::GATE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 5, true, false), module, Arp32::EOC_OUTPUT));

		if (module != NULL) {
			Arp32Display *displayW = createWidget<Arp32Display>(Vec(10, 90));
			displayW->box.size = Vec(100, 140);
			displayW->module = module;
			addChild(displayW);
		}

		gateOptions.emplace_back(std::string("Trigger"), Arp32::TRIGGER);
		gateOptions.emplace_back(std::string("Retrigger"), Arp32::RETRIGGER);
		gateOptions.emplace_back(std::string("Continuous"), Arp32::CONTINUOUS);

		noteOptions.emplace_back(std::string("Omit last note"), false);
		noteOptions.emplace_back(std::string("Play last note"), true);

	}

	void appendContextMenu(Menu *menu) override {

		Arp32 *arp = dynamic_cast<Arp32*>(module);
		assert(arp);

		struct Arp32Menu : MenuItem {
			Arp32 *module;
			Arp32Widget *parent;
		};

		struct GateModeItem : Arp32Menu {
			Arp32::GateMode gateMode;
			void onAction(const rack::event::Action &e) override {
				module->gateMode = gateMode;
			}
		};

		struct GateModeMenu : Arp32Menu {
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

		struct RepeatModeItem : Arp32Menu {
			bool repeatEnd;
			void onAction(const rack::event::Action &e) override {
				module->repeatEnd = repeatEnd;
			}
		};

		struct RepeatModeMenu : Arp32Menu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->noteOptions) {
					RepeatModeItem *item = createMenuItem<RepeatModeItem>(opt.name, CHECKMARK(module->repeatEnd == opt.value));
					item->module = module;
					item->repeatEnd = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());

		GateModeMenu *item = createMenuItem<GateModeMenu>("Gate Mode");
		item->module = arp;
		item->parent = this;
		menu->addChild(item);

		RepeatModeMenu *ritem = createMenuItem<RepeatModeMenu>("Play last note in cyclical patterns");
		ritem->module = arp;
		ritem->parent = this;
		menu->addChild(ritem);

	}

};

Model *modelArp32 = createModel<Arp32, Arp32Widget>("Arp32");

