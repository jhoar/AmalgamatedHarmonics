#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

struct Arpeggio {

	virtual const std::string & getName() = 0;

	virtual void initialise(int nPitches, int offset, bool repeatEnds) = 0;
	
	virtual void advance() = 0;
	
	virtual std::size_t getPitch() = 0;
	
	virtual bool isArpeggioFinished() = 0;

	// For RL and LR arps we have the following logic
	// Convert from npitch (1-6) to index (0 -> 9), but do no repeat first note
	// 1,2,3,4,5,6 (6) -> 
	// 0 (1)
	// 1 (2)
	// 2 (3)
	// 3 (4)
	// 4 (5)
	// 5 (6)
	// 6 (5)
	// 7 (3)
	// 8 (2)
	// 9 (END, do not repeat 1)

};

struct RightArp : Arpeggio {

	std::size_t index = 0;
	std::size_t nPitches = 0;
	const std::string name = "Right";

	const std::string & getName() override {
		return name;
	};

	void initialise(int np, int offset, bool repeatEnds) override {

		if (offset != 0) {
			index = offset % np;
		} else {
			index = 0;
		}

		nPitches = np;

//		std::cout << nPitches << " " << offset << " " << index << std::endl;

	}
	
	void advance() override {
		index++;
	}
	
	size_t getPitch() override {
		return index;
	}
	
	bool isArpeggioFinished() override {
		return (index >= nPitches - 1);
	}
	
};

struct LeftArp : Arpeggio {

	std::size_t index = 0;
	std::size_t nPitches = 0;
	const std::string name = "Left";

	const std::string & getName() override {
		return name;
	};
	
	void initialise(int np, int offset, bool repeatEnds) override {

		if (offset != 0) {
			offset = offset % np;
			index = np - offset - 1;
		} else {
			index = np - 1;
		}

		nPitches = np;

//		std::cout << nPitches << " " << offset << " " << index << std::endl;

	}
	
	void advance() override {
		index--;
	}
	
	std::size_t getPitch() override {
		return index;
	}

	bool isArpeggioFinished() override {
		return (index == 0);
	}
	
};

struct RightLeftArp : Arpeggio {

	int currSt = 0;
	int mag = 0; // index of last pitch
	int end = 0; // index of end of arp
	int nPitches = 0;
	const std::string name = "RightLeft";
	
	const std::string & getName() override {
		return name;
	};


	void initialise(int np, int offset, bool repeatEnds) override {

		nPitches = np;
		mag = np - 1;
		end = 2 * mag - 1;

		if (end < 1) {
			end = 1;
		}

		currSt = offset;

		if (end < currSt) {
			end = currSt;
		} else {
			if (offset > 0) {
				end++;
			}
		}

		if (repeatEnds) {
			end++;
		}

	}
	
	void advance() override {
		currSt++;
	}
	
	size_t getPitch() override {
		return abs((mag - abs(mag - currSt)) % nPitches);
	}

	bool isArpeggioFinished() override {
		return(currSt == end);
	}
	
};

struct LeftRightArp : Arpeggio {

	int currSt = 0;
	int mag = 0;
	int end = 0;
	int nPitches = 0;
	const std::string name = "LeftRight";
	
	const std::string & getName() override {
		return name;
	};


	void initialise(int np, int offset, bool repeatEnds) override {

		nPitches = np;
		mag = np - 1;
		end = 2 * mag - 1;

		if (end < 1) {
			end = 1;
		}

		currSt = offset;

		if (end < currSt) {
			end = currSt;
		} else {
			if (offset > 0) {
				end++;
			}
		}

		if (repeatEnds) {
			end++;
		}

	}
	
	void advance() override {
		currSt++;
	}
	
	std::size_t getPitch() override {
		return abs(abs(mag - currSt) % nPitches);
	}

	bool isArpeggioFinished() override {
		return(currSt == end);
	}
	
};

using namespace ah;

struct Arp31 : core::AHModule {
	
	const static int MAX_STEPS = 16;
	const static int MAX_DIST = 12; //Octave

	enum ParamIds {
		ARP_PARAM,
		OFFSET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		PITCH_INPUT,
		GATE_INPUT,
		ARP_INPUT,
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
	
	Arp31() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		configParam(OFFSET_PARAM, 0.0, 10.0, 0.0, "Start offset");
		paramQuantities[OFFSET_PARAM]->description = "Number of steps into the arpeggio to start";

		configParam(ARP_PARAM, 0.0, 3.0, 0.0, "Arpeggio type"); 

		arps.push_back(&arp_right);
		arps.push_back(&arp_left);
		arps.push_back(&arp_rightleft);
		arps.push_back(&arp_leftright);
		nextArp = arps[0]->getName();

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
	
	rack::dsp::PulseGenerator gatePulse;
	rack::dsp::PulseGenerator eocPulse;

	int id = 0;
	int currLight = 0;
	float outVolts = 0;
	bool isRunning = false;
	size_t inputArp = 0;
	bool eoc = false;
	bool repeatEnd = false;

	std::vector<Arpeggio *>arps;

	RightArp 		arp_right;
	LeftArp 		arp_left;
	RightLeftArp 	arp_rightleft;
	LeftRightArp 	arp_leftright;

	Arpeggio *currArp = &arp_right;
	
	std::vector<float> pitches;
	std::string nextArp;

};

void Arp31::process(const ProcessArgs &args) {
	
	AHModule::step();

	// Wait a few steps for the inputs to flow through Rack
	if (stepX < 10) { 
		return;
	}
	
	// Get inputs from Rack
	float clockInput	= inputs[CLOCK_INPUT].getVoltage();
	bool  clockActive	= inputs[CLOCK_INPUT].isConnected();
	
	if (inputs[ARP_INPUT].isConnected()) {
		inputArp = inputs[ARP_INPUT].getVoltage();
	} else {
		inputArp = params[ARP_PARAM].getValue();
	}	

	int offset = params[OFFSET_PARAM].getValue();

	// Process inputs
	bool clockStatus = clockTrigger.process(clockInput);
	
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

			if (debugEnabled()) { std::cout << stepX << " " << id  << " Advance Cycle: " << currArp->getPitch() << " " << pitches[currArp->getPitch()] << std::endl; }

			// Reached the end of the pattern?
			if (currArp->isArpeggioFinished()) {

				// Trigger EOC mechanism
				eoc = true;

				if (debugEnabled()) { std::cout << stepX << " " << id  << " Finished Cycle" << std::endl; }
				restart = true;

			} 

			// Finally set the out voltage
			size_t idx = currArp->getPitch();
			outVolts = clamp(pitches[idx], -10.0f, 10.0f);

			if (debugEnabled()) { std::cout << stepX << " " << id  << " Index: " << idx << " V: " << outVolts << " Light: " << currLight << std::endl; }

			// Pulse the output gate
			gatePulse.trigger(digital::TRIGGER);

			// Completed 1 step
			currArp->advance();

		} else {

			// Start a cycle
			restart = true;

		}

	}

	// If we have been triggered, start a new sequence
	if (restart) {

		// Read input pitches and assign to pitch array
		pitches.clear();
		if (inputs[PITCH_INPUT].isConnected()) {
			int channels = inputs[PITCH_INPUT].getChannels();
			if (debugEnabled()) { std::cout << stepX << " " << id  << " Channels: " << channels << std::endl; }

			if (inputs[GATE_INPUT].isConnected()) {
				for (int p = 0; p < channels; p++) {
					if (inputs[GATE_INPUT].getVoltage(p) > 0.0f) {
						pitches.push_back(inputs[PITCH_INPUT].getVoltage(p));
					}
				}
			} else { // No gate info, read sequentially;
				for (int p = 0; p < channels; p++) {
					pitches.push_back(inputs[PITCH_INPUT].getVoltage(p));
				}
			}

		} 

		if (pitches.size() == 0) {
			if (debugEnabled()) { std::cout << stepX << " " << id  << " No inputs, assume single 0V pitch" << std::endl; }
			pitches.push_back(0.0f);
		}

		if (debugEnabled()) { std::cout << stepX << " " << id  << " Pitches: " << pitches.size() << std::endl; }

		// At the first step of the cycle
		// So this is where we tweak the cycle parameters
		currArp = arps[inputArp];

		if (debugEnabled()) { std::cout << stepX << " " << id  << " Initiatise new Cycle: Pattern: " << currArp->getName() << " nPitches: " << pitches.size() << std::endl; }
		
		currArp->initialise(pitches.size(), offset, repeatEnd);

		// Start
		isRunning = true;
		
	} 

	nextArp = arps[inputArp]->getName();

	// Set the value
	outputs[OUT_OUTPUT].setVoltage(outVolts);

	bool gPulse = gatePulse.process(args.sampleTime);
	
	bool gatesOn = isRunning;
	if (gateMode == TRIGGER) {
		gatesOn = gatesOn && gPulse;
	} else if (gateMode == RETRIGGER) {
		gatesOn = gatesOn && !gPulse;
	}
	
	bool cPulse = eocPulse.process(args.sampleTime);

	outputs[GATE_OUTPUT].setVoltage(gatesOn ? 10.0 : 0.0);
	outputs[EOC_OUTPUT].setVoltage(cPulse ? 10.0 : 0.0);
	
}

struct Arp31Display : TransparentWidget {
	
	Arp31 *module;
	std::shared_ptr<Font> font;

	Arp31Display() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/EurostileBold.ttf"));
	}

	void draw(const DrawArgs &ctx) override {

		if (module == NULL) {
			return;
		}

		Vec pos = Vec(0, 15);

		nvgFontSize(ctx.vg, 16);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgTextLetterSpacing(ctx.vg, -1);

		nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
	
		char text[128];
		snprintf(text, sizeof(text), "%s", module->nextArp.c_str());
		nvgText(ctx.vg, pos.x + 10, pos.y + 65, text, NULL);
		
	}
	
};

struct Arp31Widget : ModuleWidget {

	std::vector<MenuOption<Arp31::GateMode>> gateOptions;
	std::vector<MenuOption<bool>> noteOptions;

	Arp31Widget(Arp31 *module) {
	
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Arp31c.svg")));
		
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 5, true, false), module, Arp31::OUT_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 5, true, false), module, Arp31::GATE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 5, true, false), module, Arp31::EOC_OUTPUT));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 0, true, false), module, Arp31::PITCH_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 0, true, false), module, Arp31::GATE_INPUT));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 4, true, false), module, Arp31::CLOCK_INPUT));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 1, 4, true, false), module, Arp31::OFFSET_PARAM)); 

		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 0, 2, true, false), module, Arp31::ARP_PARAM)); 
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 3, true, false), module, Arp31::ARP_INPUT));

		if (module != NULL) {
			Arp31Display *displayW = createWidget<Arp31Display>(Vec(40, 100));
			displayW->box.size = Vec(100, 70);
			displayW->module = module;
			addChild(displayW);
		}

		gateOptions.emplace_back(std::string("Trigger"), Arp31::TRIGGER);
		gateOptions.emplace_back(std::string("Retrigger"), Arp31::RETRIGGER);
		gateOptions.emplace_back(std::string("Continuous"), Arp31::CONTINUOUS);

		noteOptions.emplace_back(std::string("Omit last note"), false);
		noteOptions.emplace_back(std::string("Play last note"), true);

	}

	void appendContextMenu(Menu *menu) override {

		Arp31 *arp = dynamic_cast<Arp31*>(module);
		assert(arp);

		struct Arp31Menu : MenuItem {
			Arp31 *module;
			Arp31Widget *parent;
		};

		struct GateModeItem : Arp31Menu {
			Arp31::GateMode gateMode;
			void onAction(const rack::event::Action &e) override {
				module->gateMode = gateMode;
			}
		};

		struct GateModeMenu : Arp31Menu {
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

		struct RepeatModeItem : Arp31Menu {
			bool repeatEnd;
			void onAction(const rack::event::Action &e) override {
				module->repeatEnd = repeatEnd;
			}
		};

		struct RepeatModeMenu : Arp31Menu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->gateOptions) {
					RepeatModeItem *item = createMenuItem<RepeatModeItem>(opt.name, CHECKMARK(module->repeatEnd == opt.value));
					item->module = module;
					item->repeatEnd = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());
		GateModeMenu *gitem = createMenuItem<GateModeMenu>("Gate Mode");
		gitem->module = arp;
		gitem->parent = this;
		menu->addChild(gitem);

		RepeatModeMenu *ritem = createMenuItem<RepeatModeMenu>("Play last note");
		ritem->module = arp;
		ritem->parent = this;
		menu->addChild(ritem);

     }
	 
};

Model *modelArp31 = createModel<Arp31, Arp31Widget>("Arp31");

