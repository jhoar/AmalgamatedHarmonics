#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

struct Arpeggio2 {

	std::vector<int> indexes;
	int index = 0;
	int offset = 0;	
	int nPitches = 0;
	bool repeatEnds = false;

	Arpeggio2() {
		indexes.push_back(0);
		nPitches = 1;
	}

	virtual const std::string & getName() = 0;

	virtual void initialise(int _nPitches, int _offset, bool _repeatEnds) {
		nPitches = _nPitches;
		offset = _offset;
		repeatEnds = _repeatEnds;
	};
	
	void advance() {
		// std::cout << "ADV" << std::endl;
		index++;
	}

	void reset() {
		// std::cout << "RES" << std::endl;
		index = offset;
	}

	void randomize() {
		int length = nPitches - offset;
		int p1 = (rand() % length) + offset;
		int p2 = (rand() % length) + offset;
		int tries = 0;

		while (p1 == p2 && tries < 5) { // Make some effort to change the sequence, break after 5 attempts
			p2 = (rand() % length) + offset;
			tries++;
		}

		// std::cout << "RND " << length << " " << p1 << " " << p2 << " "; 

		int t = indexes[p1];
		indexes[p1] = indexes[p2];
		indexes[p2] = t;

		// for (int i = 0; i < nPitches; i++) {
		// 	std::cout << indexes[i];
		// }

		// std::cout << std::endl;

	}

	size_t getPitch() {
		// std::cout << "OUT " << index << " " << indexes[index] << std::endl;
		return indexes[index];
	}
	
	bool isArpeggioFinished() {
		// std::cout << "FIN " << index << " " << nPitches << std::endl;
		return (index >= nPitches - 1);
	}

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

struct RightArp2 : Arpeggio2 {

	const std::string name = "Straight-R";

	const std::string & getName() override {
		return name;
	};

	void initialise(int _np, int _offset, bool _repeatEnds) override {

		Arpeggio2::initialise(_np, _offset, _repeatEnds);

		// std::cout << name;
		// std::cout << " DEF ";
		indexes.clear();

		for (int i = 0; i < nPitches; i++) {
			// std::cout << i;
			indexes.push_back(i);
		}

		nPitches = indexes.size();
		offset = offset % nPitches;
		index = offset;
		// std::cout << " NP=" << nPitches << " -> " << index << std::endl;

	}
		
};

struct LeftArp2 : Arpeggio2 {

	const std::string name = "Straight-L";

	const std::string & getName() override {
		return name;
	};
	
	void initialise(int _np, int _offset, bool _repeatEnds) override {

		Arpeggio2::initialise(_np, _offset, _repeatEnds);

		// std::cout << name;
		// std::cout << " DEF ";
		indexes.clear();

		for (int i = nPitches - 1; i >= 0; i--) {
			// std::cout << i;
			indexes.push_back(i);
		}

		nPitches = indexes.size();
		offset = offset % nPitches;
		index = offset;
		// std::cout << " NP=" << nPitches << " -> " << index << std::endl;

	}
		
};

struct RightLeftArp2 : Arpeggio2 {

	const std::string name = "Straight-RL";
	
	const std::string & getName() override {
		return name;
	};

	void initialise(int _np, int _offset, bool _repeatEnds) override {

		Arpeggio2::initialise(_np, _offset, _repeatEnds);
		// std::cout << name;
		// std::cout << " DEF ";
		indexes.clear();

		for (int i = 0; i < nPitches; i++) {
			// std::cout << i;
			indexes.push_back(i);
		}

		int end = repeatEnds ? 0 : 1;

		for (int i = nPitches - 2; i >= end; i--) {
			// std::cout << i;
			indexes.push_back(i);
		}

		nPitches = indexes.size();
		offset = offset % nPitches;
		index = offset;
		// std::cout << " NP=" << nPitches << " -> " << index << std::endl;

	}
		
};

struct LeftRightArp2 : Arpeggio2 {

	const std::string name = "Straight-LR";
	
	const std::string & getName() override {
		return name;
	};

	void initialise(int _np, int _offset, bool _repeatEnds) override {

		Arpeggio2::initialise(_np, _offset, _repeatEnds);

		// std::cout << name;
		// std::cout << " DEF ";
		indexes.clear();

		for (int i = nPitches - 1; i >= 0; i--) {
			// std::cout << i;
			indexes.push_back(i);
		}

		int end = repeatEnds ? 0 : 1;

		for (int i = 1; i < nPitches - end; i++) {
			// std::cout << i;
			indexes.push_back(i);
		}

		nPitches = indexes.size();
		offset = offset % nPitches;
		index = offset;
		// std::cout << " NP=" << nPitches << " -> " << index << std::endl;

	}
		
};

struct CrabRightArp2 : Arpeggio2 {

	const std::string name = "Crab-R";
	
	const std::string & getName() override {
		return name;
	};

	void initialise(int _np, int _offset, bool _repeatEnds) override {

		Arpeggio2::initialise(_np, _offset, _repeatEnds);

		// std::cout << name;
		// std::cout << " DEF ";
		indexes.clear();

		int steps[2] = {2, -1};

		// 1 = 1: 0 
		// 2 = 2: 0, 0
		// 3 = 2: 0, 2
		// 4 = 4: 0, 2, 1, 3
		// 5 = 6: 0, 2, 1, 3, 2, 4 
		// 6 = 8: 0, 2, 1, 3, 2, 4, 3, 5

		if (nPitches == 1) {
			indexes.push_back(0);
		} else if (nPitches == 2) {
			indexes.push_back(0);
			indexes.push_back(0);
		} else {
			int p = 0;
			int i = 0;

			while (true) {
				// std::cout << p;
				indexes.push_back(p);
				p = p + steps[i % 2];
				i++;
				if (p == nPitches - 1) {
					// std::cout << p;
					indexes.push_back(p);
					break;
				}
			} 
		}

		nPitches = indexes.size();
		offset = offset % nPitches;
		index = offset;
		// std::cout << " NP=" << nPitches << " -> " << index << std::endl;

	}
		
};

struct CrabLeftArp2 : Arpeggio2 {

	const std::string name = "Crab-L";
	
	const std::string & getName() override {
		return name;
	};

	void initialise(int _np, int _offset, bool _repeatEnds) override {

		Arpeggio2::initialise(_np, _offset, _repeatEnds);

		// std::cout << name;
		// std::cout << " DEF ";
		indexes.clear();

		int steps[2] = {-2, 1};

		// 1 = 1: 0 
		// 2 = 2: 1, 1
		// 3 = 2: 2, 0
		// 4 = 4: 3, 1, 2, 0
		// 5 = 6: 4, 2, 3, 1, 2, 0 
		// 6 = 8: 5, 3, 4, 2, 3, 1, 2, 0

		if (nPitches == 1) {
			indexes.push_back(nPitches - 1);
		} else if (nPitches == 2) {
			indexes.push_back(nPitches - 1);
			indexes.push_back(nPitches - 1);
		} else {
			int p = nPitches - 1;
			int i = 0;

			while (true) {
				// std::cout << p;
				indexes.push_back(p);
				p = p + steps[i % 2];
				i++;
				if (p == 0) {
					// std::cout << p;
					indexes.push_back(0);
					break;
				}
			} 
		}

		nPitches = indexes.size();
		offset = offset % nPitches;
		index = offset;
		// std::cout << " NP=" << nPitches << " -> " << index << std::endl;

	}
		
};

struct CrabRightLeftArp2 : Arpeggio2 {

	const std::string name = "Crab-RL";
	
	const std::string & getName() override {
		return name;
	};

	void initialise(int _np, int _offset, bool _repeatEnds) override {

		Arpeggio2::initialise(_np, _offset, _repeatEnds);

		// std::cout << name;
		// std::cout << " DEF ";
		indexes.clear();

		int stepsR[2] = {2, -1};
		int stepsL[2] = {-2, 1};

		// 1 = 1:  0 
		// 2 = 2:  0, 0
		// 3 = 5:  0, 2, 1, 2, 0
		// 4 = 7:  0, 2, 1, 3, 1, 2, 0
		// 5 = 11: 0, 2, 1, 3, 2, 4, 2, 3, 1, 2, 0 
		// 6 = 15: 0, 2, 1, 3, 2, 4, 3, 5, 3, 4, 2, 3, 1, 2, 0

		// 021324353423120
		// 0213243534231

		if (nPitches == 1) {
			indexes.push_back(0);
		} else if (nPitches == 2) {
			indexes.push_back(0);
			indexes.push_back(0);
		} else {

			int p = 0;
			int i = 0;

			while (true) {
				// std::cout << p;
				indexes.push_back(p);
				p = p + stepsR[i % 2];
				i++;
				if (p == nPitches - 1) {
					// std::cout << p;
					indexes.push_back(p);
					break;
				}
			} 

			p = nPitches - 3;
			i = 1;
			int end = repeatEnds ? 0 : 1;

			while (true) {
				// std::cout << p;
				indexes.push_back(p);
				p = p + stepsL[i % 2];
				i++;
				if (p == end) {
					// std::cout << end;
					indexes.push_back(end);
					break;
				}
			} 
		}

		nPitches = indexes.size();
		offset = offset % nPitches;
		index = offset;
		// std::cout << " NP=" << nPitches << " -> " << index << std::endl;

	}
		
};

struct CrabLeftRightArp2 : Arpeggio2 {

	const std::string name = "Crab-LR";
	
	const std::string & getName() override {
		return name;
	};

	void initialise(int _np, int _offset, bool _repeatEnds) override {

		Arpeggio2::initialise(_np, _offset, _repeatEnds);

		// std::cout << name;
		// std::cout << " DEF ";
		indexes.clear();

		int stepsR[2] = {2, -1};
		int stepsL[2] = {-2, 1};

		// 1 = 1:  0 
		// 2 = 2:  1, 1
		// 3 = 2:  2, 0, 1, 0, 2
		// 4 = 7:  3, 1, 2, 0, 2, 1, 3
		// 5 = 11: 4, 2, 3, 1, 2, 0, 2, 1, 3, 2, 4 
		// 6 = 15: 5, 3, 4, 2, 3, 1, 2, 0, 2, 1, 3, 2, 4, 3, 5

		// 534231202132435
		// 534231202132435

		if (nPitches == 1) {
			indexes.push_back(nPitches - 1);
		} else if (nPitches == 2) {
			indexes.push_back(nPitches - 1);
			indexes.push_back(nPitches - 1);
		} else {

			int p = nPitches - 1;
			int i = 0;

			while (true) {
				// std::cout << p;
				indexes.push_back(p);
				p = p + stepsL[i % 2];
				i++;
				if (p == 0) {
					// std::cout << p;
					indexes.push_back(0);
					break;
				}
			} 

			p = 2;
			i = 1;
			int end = repeatEnds ? 0 : 1;

			while (true) {
				// std::cout << p;
				indexes.push_back(p);
				p = p + stepsR[i % 2];
				i++;
				if (p == nPitches - 1 - end) {
					// std::cout << p;
					indexes.push_back(p);
					break;
				}
			} 
		}

		nPitches = indexes.size();
		offset = offset % nPitches;
		index = offset;
		// std::cout << " NP=" << nPitches << " -> " << index << std::endl;

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
	
	Arp31() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		configParam(OFFSET_PARAM, 0.0, 10.0, 0.0, "Start offset");
		paramQuantities[OFFSET_PARAM]->description = "Number of steps into the arpeggio to start";

		configParam(ARP_PARAM, 0.0, 7.0, 0.0, "Arpeggio type"); 

		arps.push_back(&arp_right);
		arps.push_back(&arp_left);
		arps.push_back(&arp_rightleft);
		arps.push_back(&arp_leftright);
		arps.push_back(&arp_crabright);
		arps.push_back(&arp_crableft);
		arps.push_back(&arp_crabrightleft);
		arps.push_back(&arp_crableftright);
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
	rack::dsp::SchmittTrigger randomTrigger; // for random
	
	rack::dsp::PulseGenerator gatePulse;
	rack::dsp::PulseGenerator eocPulse;

	int id = 0;
	int currLight = 0;
	float outVolts = 0;
	bool isRunning = false;
	size_t inputArp = 0;
	bool eoc = false;
	bool repeatEnd = false;

	std::vector<Arpeggio2 *> arps;

	RightArp2 			arp_right;
	LeftArp2 			arp_left;
	RightLeftArp2 		arp_rightleft;
	LeftRightArp2 		arp_leftright;
	CrabRightArp2 		arp_crabright;
	CrabLeftArp2 		arp_crableft;
	CrabRightLeftArp2	arp_crabrightleft;
	CrabLeftRightArp2	arp_crableftright;

	Arpeggio2 *currArp = &arp_right;
	
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
	float randomInput	= inputs[RANDOM_INPUT].getVoltage();

	if (inputs[ARP_INPUT].isConnected()) {
		inputArp = inputs[ARP_INPUT].getVoltage();
	} else {
		inputArp = params[ARP_PARAM].getValue();
	}	

	int offset = params[OFFSET_PARAM].getValue();
	int hold = digital::sgn(inputs[HOLD_INPUT].getVoltage(), 0.001);

	// Process inputs
	bool clockStatus = clockTrigger.process(clockInput);
	bool randomStatus = randomTrigger.process(randomInput);
	
	// If there is no clock input, then force that we are not running
	if (!clockActive) {
		isRunning = false;
	}

	bool restart = false;

	if (debugEnabled()) { std::cout << stepX << " " << id  << " Check clock" << std::endl; }

	// Have we been clocked?
	if (clockStatus) {

		if (debugEnabled()) { std::cout << stepX << " " << id  << " Check EOC" << std::endl; }

		// EOC was fired at last sequence step
		if (eoc) {
			if (debugEnabled()) { std::cout << stepX << " " << id  << " EOC fired" << std::endl; }
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

	// Randomise if triggered
	if (randomStatus && isRunning && hold != -1) {
		currArp->randomize();
	}

	if (debugEnabled()) { std::cout << stepX << " " << id  << " Check restart" << std::endl; }

	// If we have been triggered, start a new sequence
	if (restart) {

		if (!hold) {

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

		} else {

			if (pitches.size() == 0) {
				if (debugEnabled()) { std::cout << stepX << " " << id  << " No inputs, assume single 0V pitch" << std::endl; }
				pitches.push_back(0.0f);
			}

			if (debugEnabled()) { std::cout << stepX << " " << id  << " Hold Cycle: Pattern: " << currArp->getName() << " nPitches: " << pitches.size() << std::endl; }

			currArp->reset();

		}

		// Start
		isRunning = true;
		
	} 

	nextArp = arps[inputArp]->getName();

	// Set the value
	outputs[OUT_OUTPUT].setVoltage(outVolts);

	bool gPulse = gatePulse.process(args.sampleTime);

	if (debugEnabled()) { std::cout << stepX << " " << id  << " Do gate status" << std::endl; }

	bool gatesOn = isRunning;
	if (gateMode == TRIGGER) {
		gatesOn = gatesOn && gPulse;
	} else if (gateMode == RETRIGGER) {
		gatesOn = gatesOn && !gPulse;
	}
	if (debugEnabled()) { std::cout << stepX << " " << id  << " Checked gate status" << std::endl; }
	
	bool cPulse = eocPulse.process(args.sampleTime);

	outputs[GATE_OUTPUT].setVoltage(gatesOn ? 10.0 : 0.0);
	outputs[EOC_OUTPUT].setVoltage(cPulse ? 10.0 : 0.0);

	if (debugEnabled()) { std::cout << stepX << " " << id  << " Finish output phase" << std::endl; }

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
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 0, true, false), module, Arp31::HOLD_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 0, true, false), module, Arp31::RANDOM_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 1, true, false), module, Arp31::GATE_INPUT));

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
		GateModeMenu *gitem = createMenuItem<GateModeMenu>("Gate Mode");
		gitem->module = arp;
		gitem->parent = this;
		menu->addChild(gitem);

		RepeatModeMenu *ritem = createMenuItem<RepeatModeMenu>("Play last note in cyclical arpeggios");
		ritem->module = arp;
		ritem->parent = this;
		menu->addChild(ritem);

     }
	 
};

Model *modelArp31 = createModel<Arp31, Arp31Widget>("Arp31");

