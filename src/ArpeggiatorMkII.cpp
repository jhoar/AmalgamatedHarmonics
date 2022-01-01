#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct Pattern {

	unsigned int length = 0;
	int trans = 0;
	unsigned int scale = 0;
	unsigned int count = 0;

	unsigned int MAJOR[7] = {0,2,4,5,7,9,11};
	unsigned int MINOR[7] = {0,2,3,5,7,8,10};

	virtual std::string getName() = 0;

	virtual void initialise(unsigned int l, unsigned int sc, int tr, bool freeRun) {
		length = l;
		trans = tr;
		scale = sc;
		count = 0;
	};

	virtual void advance() {
		count++;
	};

	virtual int getOffset() = 0;

	virtual bool isPatternFinished() = 0;

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

};

struct UpPattern : Pattern {

	std::string getName() override {
		return "Up";
	};

	void initialise(unsigned int l, unsigned int sc, int tr, bool fr) override {
		Pattern::initialise(l,sc,tr,fr);
	}

	int getOffset() override {

		switch(scale) {
			case 0: return count * trans; break;
			case 1: return getMajor(count * trans); break;
			case 2: return getMinor(count * trans); break;
			default:
				return count * trans; break;
		}

	}

	bool isPatternFinished() override {
		return(count == length);
	}

};

struct DownPattern : Pattern {

	int currSt = 0;

	std::string getName() override {
		return "Down";
	};

	void initialise(unsigned int l, unsigned int sc, int tr, bool fr) override {
		Pattern::initialise(l,sc,tr,fr);
		currSt = length - 1;
	}

	void advance() override {
		Pattern::advance();
		currSt--;
	}

	int getOffset() override {
		switch(scale) {
			case 0: return currSt * trans; break;
			case 1: return getMajor(currSt * trans); break;
			case 2: return getMinor(currSt * trans); break;
			default:
				return currSt * trans; break;
		}
	}

	bool isPatternFinished() override {
		return (currSt < 0);
	}

};

struct UpDownPattern : Pattern {

	unsigned int mag = 0;
	unsigned int end = 0;

	std::string getName() override {
		return "UpDown";
	};	

	void initialise(unsigned int l, unsigned int sc, int tr, bool fr) override {
		Pattern::initialise(l,sc,tr,fr);
		mag = l - 1;
		if (fr) {
			end = 2 * l - 2;
		} else {
			end = 2 * l - 1;
		}
		if (end < 1) {
			end = 1;
		}
	}

	int getOffset() override {
		
		int note = (mag - abs(static_cast<int>(mag - count)));
		
		switch(scale) {
			case 0: return note * trans; break;
			case 1: return getMajor(note * trans); break;
			case 2: return getMinor(note * trans); break;
			default:
				return note * trans; break;
		}

	}

	bool isPatternFinished() override {
		return(count == end);
	}

};

struct DownUpPattern : Pattern {

	unsigned int mag = 0;
	unsigned int end = 0;
	
	std::string getName() override {
		return "DownUp";
	};	

	void initialise(unsigned int l, unsigned int sc, int tr, bool fr) override {
		Pattern::initialise(l,sc,tr,fr);
		mag = l - 1;
		if (fr) {
			end = 2 * l - 2;
		} else {
			end = 2 * l - 1;
		}
		if (end < 1) {
			end = 1;
		}
	}

	int getOffset() override {

		int note = -(mag - abs(static_cast<int>(mag - count)));

		switch(scale) {
			case 0: return note * trans; break;
			case 1: return getMajor(note * trans); break;
			case 2: return getMinor(note * trans); break;
			default:
				return note * trans; break;
		}

	}

	bool isPatternFinished() override {
		return(count == end);
	}

};

struct NotePattern : Pattern {

	std::vector<int> notes;

	void initialise(unsigned int l, unsigned int sc, int tr, bool fr) override {
		Pattern::initialise(l,sc,tr,fr);
	}

	int getOffset() override {
		return getNote(count);
	}

	bool isPatternFinished() override {
		return (count == static_cast<unsigned int>(notes.size()));
	}

	int getNote(int i) {
		return notes[i];
	}

};

struct RezPattern : NotePattern {
	
	std::string getName() override {
		return "Rez";
	};	

	RezPattern() {
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

struct OnTheRunPattern : NotePattern {

	std::string getName() override {
		return "On The Run";
	};	

	OnTheRunPattern() {
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

struct Arpeggio {

	virtual std::string getName() = 0;

	virtual void initialise(unsigned int nPitches, bool fr) = 0;

	virtual void advance() = 0;

	virtual unsigned int getPitch() = 0;

	virtual bool isArpeggioFinished() = 0;

};

struct RightArp : Arpeggio {

	unsigned int index = 0;
	unsigned int nPitches = 0;

	std::string getName() override {
		return "Right";
	};

	void initialise(unsigned int np, bool fr) override {
		index = 0;
		nPitches = np;
	}

	void advance() override {
		index++;
	}

	unsigned int getPitch() override {
		return index;
	}

	bool isArpeggioFinished() override {
		return (index == nPitches);
	}

};

struct LeftArp : Arpeggio {

	int index = 0;
	unsigned int nPitches = 0;

	std::string getName() override {
		return "Left";
	};

	void initialise(unsigned int np, bool fr) override {
		nPitches = np;
		index = nPitches - 1;
	}

	void advance() override {
		index--;
	}

	unsigned int getPitch() override {
		return index;
	}

	bool isArpeggioFinished() override {
		return (index < 0);
	}

};

struct RightLeftArp : Arpeggio {

	unsigned int currSt = 0;
	unsigned int mag = 0;
	unsigned int end = 0;

	std::string getName() override {
		return "RightLeft";
	};	

	void initialise(unsigned int l, bool fr) override {
		mag = l - 1;
		if (fr) {
			end = 2 * l - 2;
		} else {
			end = 2 * l - 1;
		}
		if (end < 1) {
			end = 1;
		}
		currSt = 0;
	}

	void advance() override {
		currSt++;
	}

	unsigned int getPitch() override {
		return mag - abs(static_cast<int>(mag - currSt));
	}

	bool isArpeggioFinished() override {
		return(currSt == end);
	}

};

struct LeftRightArp : Arpeggio {

	unsigned int currSt = 0;
	unsigned int mag = 0;
	unsigned int end = 0;

	std::string getName() override {
		return "LeftRight";
	};

	void initialise(unsigned int l, bool fr) override {
		mag = l - 1;
		if (fr) {
			end = 2 * l - 2;
		} else {
			end = 2 * l - 1;
		}
		if (end < 1) {
			end = 1;
		}
		currSt = 0;
	}

	void advance() override {
		currSt++;
	}

	unsigned int getPitch() override {
		return abs(static_cast<int>(mag - currSt));
	}

	bool isArpeggioFinished() override {
		return(currSt == end);
	}

};

struct Arpeggiator2 : core::AHModule {

	const static unsigned int MAX_STEPS = 16;
	const static unsigned int MAX_DIST = 12; //Octave
	const static unsigned int NUM_PITCHES = 6;

	enum ParamIds {
		LOCK_PARAM,
		TRIGGER_PARAM,
		PATT_PARAM,
		ARP_PARAM,
		LENGTH_PARAM,
		TRANS_PARAM,
		SCALE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		TRIG_INPUT,
		ENUMS(PITCH_INPUT,6),
		PATT_INPUT,
		ARP_INPUT,
		LENGTH_INPUT,
		TRANS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		GATE_OUTPUT,
		EOC_OUTPUT,
		EOS_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		LOCK_LIGHT,
		NUM_LIGHTS
	};

	Arpeggiator2() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

		configParam(LOCK_PARAM, 0.0, 1.0, 0.0, "Input lock");

		configParam(TRIGGER_PARAM, 0.0, 1.0, 0.0, "Trigger one sequence");

		configParam(ARP_PARAM, 0.0, 3.0, 0.0, "Arpeggio"); 
		paramQuantities[ARP_PARAM]->description = "Arpeggio over input notes"; 

		configParam(SCALE_PARAM, 0, 2, 0, "Step size"); 
		paramQuantities[SCALE_PARAM]->description = "Size of each step, semitones or major or minor intervals"; 

		configParam(PATT_PARAM, 0.0, 5.0, 0.0, "Pattern"); 
		paramQuantities[ARP_PARAM]->description = "Pattern applied to note arpeggio as a whole"; 

		configParam(TRANS_PARAM, -24, 24, 0, "Pattern steps"); 

		configParam(LENGTH_PARAM, 1.0, 16.0, 1.0); 

		onReset();
		id = rand();
		debugFlag = false;

	}

	void process(const ProcessArgs &args) override;

	void onReset() override {
		newSequence = 0;
		newCycle = 0;
		isRunning = false;
		freeRunning = false;
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// gateMode
		json_t *gateModeJ = json_integer((int) gateMode);
		json_object_set_new(rootJ, "gateMode", gateModeJ);

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		// gateMode
		json_t *gateModeJ = json_object_get(rootJ, "gateMode");
		
		if (gateModeJ) {
			gateMode = (GateMode)json_integer_value(gateModeJ);
		}
	}

	enum GateMode {
		TRIGGER,
		RETRIGGER,
		CONTINUOUS,
	};
	GateMode gateMode = TRIGGER;
	
	rack::dsp::SchmittTrigger clockTrigger; // for clock
	rack::dsp::SchmittTrigger trigTrigger;  // for step trigger
	rack::dsp::SchmittTrigger lockTrigger;
	rack::dsp::SchmittTrigger buttonTrigger;

	rack::dsp::PulseGenerator triggerPulse;
	rack::dsp::PulseGenerator gatePulse;
	rack::dsp::PulseGenerator eosPulse;
	rack::dsp::PulseGenerator eocPulse;

	bool locked = false;

	float outVolts = 0;
	bool isRunning = false;
	bool freeRunning = false;
	int error = 0;

	int newSequence = 0;
	int newCycle = 0;
	const static int LAUNCH = 1;
	const static int COUNTDOWN = 3;

	unsigned int inputPat = 0;
	unsigned int inputArp = 0;
	unsigned int inputLen = 0;
	int inputTrans = 0;
	unsigned int inputScale = 0;

	int poll = 5000;

	unsigned int pattern = 0;
	unsigned int arp = 0;
	unsigned int length = 0;
	float trans = 0;
	unsigned int scale = 0;

	UpPattern		patt_up; 
	DownPattern		patt_down; 
	UpDownPattern	patt_updown;
	DownUpPattern	patt_downup;
	RezPattern		patt_rez;
	OnTheRunPattern	patt_ontherun;

	UpPattern		ui_patt_up; 
	DownPattern		ui_patt_down; 
	UpDownPattern	ui_patt_updown;
	DownUpPattern	ui_patt_downup;
	RezPattern		ui_patt_rez;
	OnTheRunPattern	ui_patt_ontherun;

	RightArp		arp_right;
	LeftArp			arp_left;
	RightLeftArp	arp_rightleft;
	LeftRightArp	arp_leftright;

	RightArp		ui_arp_right;
	LeftArp			ui_arp_left;
	RightLeftArp	ui_arp_rightleft;
	LeftRightArp	ui_arp_leftright;

	Pattern *currPatt = &patt_up;
	Arpeggio *currArp = &arp_right;

	Pattern *uiPatt = &ui_patt_up;
	Arpeggio *uiArp = &arp_right;

	float pitches[6];
	unsigned int nPitches = 0;
	int id = 0;

};

void Arpeggiator2::process(const ProcessArgs &args) {

	AHModule::step();

	// Wait a few steps for the inputs to flow through Rack
	if (stepX < 10) { 
		return;
	}
	
	// Get inputs from Rack
	float clockInput	= inputs[CLOCK_INPUT].getVoltage();
	bool  clockActive	= inputs[CLOCK_INPUT].isConnected();
	float trigInput		= inputs[TRIG_INPUT].getVoltage();
	bool  trigActive	= inputs[TRIG_INPUT].isConnected();
	float lockInput		= params[LOCK_PARAM].getValue();
	float buttonInput	= params[TRIGGER_PARAM].getValue();
	
	// Read param section	
	if (inputs[PATT_INPUT].isConnected()) {
		inputPat = clamp(static_cast<unsigned int>(inputs[PATT_INPUT].getVoltage()), 0, 5);
	} else {
		inputPat = params[PATT_PARAM].getValue();
	}

	if (inputs[ARP_INPUT].isConnected()) {
		inputArp = clamp(static_cast<unsigned int>(inputs[ARP_INPUT].getVoltage()), 0, 3);
	} else {
		inputArp = params[ARP_PARAM].getValue();
	}	

	if (inputs[LENGTH_INPUT].isConnected()) {
		inputLen = clamp(static_cast<unsigned int>(inputs[LENGTH_INPUT].getVoltage()), 1, 16);
	} else {
		inputLen = params[LENGTH_PARAM].getValue();
	}	

	if (inputs[TRANS_INPUT].isConnected()) {
		inputTrans = clamp(static_cast<int>(inputs[TRANS_INPUT].getVoltage()), -24, 24);
	} else {
		inputTrans = params[TRANS_PARAM].getValue();
	}

	inputScale = params[SCALE_PARAM].getValue();

	// Process inputs
	bool clockStatus	= clockTrigger.process(clockInput);
	bool triggerStatus	= trigTrigger.process(trigInput);
	bool lockStatus		= lockTrigger.process(lockInput);
	bool buttonStatus	= buttonTrigger.process(buttonInput);

	// Read input pitches and assign to pitch array
	unsigned int nValidPitches = 0;
	float inputPitches[NUM_PITCHES];
	for (unsigned int p = 0; p < NUM_PITCHES; p++) {
		unsigned int index = PITCH_INPUT + p;
		if (inputs[index].isConnected()) {
			inputPitches[nValidPitches] = inputs[index].getVoltage();
			nValidPitches++;
		} else {
			inputPitches[nValidPitches] = 0.0;
		}
	}

	// if (debugEnabled()) {
	// 	for (int p = 0; p < nValidPitches; p++) {
	// 		std::cout << inputPitches[p] << std::endl;
	// 	}
	// }

	// Always play something
	if (nValidPitches == 0) {
		if (debugEnabled()) { std::cout << stepX << " " << id  << " No inputs, assume single 0V pitch" << std::endl; }
		nValidPitches = 1;
	}

	// Need to understand why this happens
	if (inputLen == 0) {
		if (debugEnabled()) { std::cout << stepX << " " << id  << " InputLen == 0, aborting" << std::endl; }
		return; // No inputs, no music
	}

	// If there is no clock input, then force that we are not running
	if (!clockActive) {
		isRunning = false;
	}

	// Has the trigger input been fired
	if (triggerStatus) {
		triggerPulse.trigger(5e-5);
		if (debugEnabled()) { std::cout << stepX << " " << id  << " Triggered" << std::endl; }
	}

	// Update the trigger pulse and determine if it is still high
	bool triggerHigh = triggerPulse.process(args.sampleTime);
	if (debugEnabled()) {
		if (triggerHigh) {
			std::cout << stepX << " " << id  << " Trigger is high" << std::endl;
		}
	}

	// Update lock
	if (lockStatus) {
		if (debugEnabled()) { std::cout << "Toggling lock: " << locked << std::endl; }
		locked = !locked;
	}

	if (newSequence) {
		newSequence--;
		if (debugEnabled()) { std::cout << stepX << " " << id  << " Countdown newSequence: " << newSequence << std::endl; }
	}

	if (newCycle) {
		newCycle--;
		if (debugEnabled()) { std::cout << stepX << " " << id  << " Countdown newCycle: " << newCycle << std::endl; }
	}

	// OK so the problem here might be that the clock gate is still high right after the trigger gate fired on the previous step
	// So we need to wait a while for the clock gate to go low
	// Has the clock input been fired
	bool isClocked = false;
	if (clockStatus && !triggerHigh) {
		if (debugEnabled()) { std::cout << stepX << " " << id  << " Clocked" << std::endl; }
		isClocked = true;
	}

	// Has the trigger input been fired, either on the input or button
	if (triggerStatus || buttonStatus) {
		if (debugEnabled()) { std::cout << stepX << " " << id  << " Start countdown " << clockActive <<std::endl; }
		if (clockActive) {
			newSequence = COUNTDOWN;
			newCycle = COUNTDOWN;
		}
	}

	// Received trigger before EOS, fire EOS gate anyway
	if (triggerStatus && isRunning && !currPatt->isPatternFinished()) {
			// Pulse the EOS gate
		eosPulse.trigger(digital::TRIGGER);
		if (debugEnabled()) { std::cout << stepX << " " << id  << " Short sequence" << std::endl; }
	}

	// So this is where the free-running could be triggered
	if (isClocked && !isRunning) { // Must have a clock and not be already running
		if (!trigActive) { // If nothing plugged into the TRIG input
			if (debugEnabled()) { std::cout << stepX << " " << id  << " Free running sequence; starting" << std::endl; }
			freeRunning = true; // We're free-running
			newSequence = COUNTDOWN;
			newCycle = LAUNCH;
		} else {
			if (debugEnabled()) { std::cout << stepX << " " << id  << " Triggered sequence; wait for trigger" << std::endl; }
			freeRunning = false;
		}
	}

	// Detect cable being plugged in when free-running, stop free-running
	if (freeRunning && trigActive && isRunning) {
		if (debugEnabled()) { std::cout << stepX << " " << id  << " TRIG input re-connected" << std::endl; }
		freeRunning = false;
	}	

	// Reached the end of the cycle
	if (isRunning && isClocked && currArp->isArpeggioFinished()) {

		// Completed 1 step
		currPatt->advance();

		// Pulse the EOC gate
		eocPulse.trigger(digital::TRIGGER);
		if (debugEnabled()) { std::cout << stepX << " " << id  << " Finished Cycle" << std::endl; }

		// Reached the end of the sequence
		if (isRunning && currPatt->isPatternFinished()) {

			// Free running, so start new seqeuence & cycle
			if (freeRunning) {
				newCycle = COUNTDOWN;
				newSequence = COUNTDOWN;
			}

			isRunning = false;
	
			// Pulse the EOS gate
			eosPulse.trigger(digital::TRIGGER);
			if (debugEnabled()) { std::cout << stepX << " " << id  << " Finished Sequence, flag: " << isRunning << std::endl; }

		} else {
			newCycle = LAUNCH;
			if (debugEnabled()) { std::cout << stepX << " " << id  << " Flagging new cycle" << std::endl; }
		}

	}

	// If we have been triggered, start a new sequence
	if (newSequence == LAUNCH) {

		// At the first step of the sequence
		// So this is where we tweak the sequence parameters

		if (!locked) {
			pattern = inputPat;
			length = inputLen;
			trans = inputTrans;
			scale = inputScale;

			switch(pattern) {
				case 0:		currPatt = &patt_up; 		break;
				case 1:		currPatt = &patt_down;		break;
				case 2:		currPatt = &patt_updown;	break;
				case 3:		currPatt = &patt_downup;	break;
				case 4:		currPatt = &patt_rez;		break;
				case 5:		currPatt = &patt_ontherun;	break;
				default:	currPatt = &patt_up;		break;
			};

		}

		if (debugEnabled()) { std::cout << stepX << " " << id  << " Initiatise new Sequence: Pattern: " << currPatt->getName() << 
			" Length: " << inputLen <<
			" Locked: " << locked << std::endl; }

		currPatt->initialise(length, scale, trans, freeRunning);

		// We're running now
		isRunning = true;

	}

	// Starting a new cycle
	if (newCycle == LAUNCH) {

		/// Reset the cycle counters
		if (!locked) {

			arp = inputArp;

			switch(arp) {
				case 0: 	currArp = &arp_right;		break;
				case 1: 	currArp = &arp_left;		break;
				case 2: 	currArp = &arp_rightleft;	break;
				case 3: 	currArp = &arp_leftright;	break;
				default:	currArp = &arp_right;		break; 	
			};

			// Copy pitches
			for (unsigned int p = 0; p < nValidPitches; p++) {
				pitches[p] = inputPitches[p];
			}
			nPitches = nValidPitches;

		}

		if (debugEnabled()) { std::cout << stepX << " " << id  << " Initiatise new Cycle: " << nPitches << " " << currArp->getName() << std::endl; }

		currArp->initialise(nPitches, freeRunning);

	}

	// Advance the sequence
	// Are we starting a sequence or are running and have been clocked; if so advance the sequence
	// Only advance from the clock
	if (isRunning && (isClocked || newCycle == LAUNCH)) {

		if (debugEnabled()) { std::cout << stepX << " " << id  << " Advance Cycle: " << currArp->getPitch() << std::endl; }

		if (debugEnabled()) { std::cout << stepX << " " << id  << " Advance Cycle: " << pitches[currArp->getPitch()] << " " << (float)currPatt->getOffset() << std::endl; }

		// Finally set the out voltage
		outVolts = clamp(pitches[currArp->getPitch()] + music::SEMITONE * (float)currPatt->getOffset(), -10.0f, 10.0f);

		if (debugEnabled()) { std::cout << stepX << " " << id  << " Output V = " << outVolts << std::endl; }

		// Update counters
		currArp->advance();

		// Pulse the output gate
		gatePulse.trigger(digital::TRIGGER);
		
	}

	// Update UI
	switch(inputPat) {
		case 0:		uiPatt = &ui_patt_up; 		break;
		case 1:		uiPatt = &ui_patt_down;		break;
		case 2:		uiPatt = &ui_patt_updown;	break;
		case 3:		uiPatt = &ui_patt_downup;	break;
		case 4:		uiPatt = &ui_patt_rez;		break;
		case 5:		uiPatt = &ui_patt_ontherun;	break;
		default:	uiPatt = &ui_patt_up;		break;
	};

	uiPatt->initialise(inputLen, inputScale, inputTrans, freeRunning);

	switch(inputArp) {
		case 0: 	uiArp = &ui_arp_right;		break;
		case 1: 	uiArp = &ui_arp_left;		break;
		case 2: 	uiArp = &ui_arp_rightleft;	break;
		case 3: 	uiArp = &ui_arp_leftright;	break;
		default:	uiArp = &ui_arp_right;		break; 	
	};

	uiArp->initialise(nPitches, freeRunning);

	// Set the value
	lights[LOCK_LIGHT].setBrightness(locked ? 1.0 : 0.0);
	outputs[OUT_OUTPUT].setVoltage(outVolts);

	bool gPulse = gatePulse.process(args.sampleTime);
	bool sPulse = eosPulse.process(args.sampleTime);
	bool cPulse = eocPulse.process(args.sampleTime);

	bool gatesOn = isRunning;
	if (gateMode == TRIGGER) {
		gatesOn = gatesOn && gPulse;
	} else if (gateMode == RETRIGGER) {
		gatesOn = gatesOn && !gPulse;
	}

	outputs[GATE_OUTPUT].setVoltage(gatesOn ? 10.0 : 0.0);
	outputs[EOS_OUTPUT].setVoltage(sPulse ? 10.0 : 0.0);
	outputs[EOC_OUTPUT].setVoltage(cPulse ? 10.0 : 0.0);

}

struct Arpeggiator2Display : TransparentWidget {
	
	Arpeggiator2 *module;
	int frame = 0;

	void draw(const DrawArgs &ctx) override {

		if (module == NULL) {
			return;
		}

		Vec pos = Vec(0, 15);

		std::shared_ptr<Font> font = APP->window->loadFont("res/RobotoCondensed-Bold.ttf");

		if (font) {		

			nvgFontSize(ctx.vg, 18);
			nvgFontFaceId(ctx.vg, font->handle);
			nvgTextLetterSpacing(ctx.vg, -1);
			nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));

			char text[128];
			if (module->inputLen == 0) {
				snprintf(text, sizeof(text), "Error: inputLen == 0");
				nvgText(ctx.vg, pos.x + 10, pos.y + 5, text, NULL);			
			} else {
				snprintf(text, sizeof(text), "Pattern: %s", module->uiPatt->getName().c_str());
				nvgText(ctx.vg, pos.x + 10, pos.y + 5, text, NULL);

				snprintf(text, sizeof(text), "Length: %d", module->uiPatt->length);
				nvgText(ctx.vg, pos.x + 10, pos.y + 25, text, NULL);

				switch(module->uiPatt->scale) {
					case 0: snprintf(text, sizeof(text), "Transpose: %d s.t.", module->uiPatt->trans); break;
					case 1: snprintf(text, sizeof(text), "Transpose: %d Maj. int.", module->uiPatt->trans); break;
					case 2: snprintf(text, sizeof(text), "Transpose: %d Min. int.", module->uiPatt->trans); break;
					default: snprintf(text, sizeof(text), "Error..."); break;
				}
				nvgText(ctx.vg, pos.x + 10, pos.y + 45, text, NULL);

				snprintf(text, sizeof(text), "Arpeggio: %s", module->uiArp->getName().c_str());
				nvgText(ctx.vg, pos.x + 10, pos.y + 65, text, NULL);
			}
		}
	}

};

struct Arpeggiator2Widget : ModuleWidget {

	Arpeggiator2Widget(Arpeggiator2 *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Arpeggiator2.svg")));

		addParam(createParamCentered<gui::AHButton>(Vec(120.0, 65.085), module, Arpeggiator2::LOCK_PARAM));
		addParam(createParamCentered<BefacoPush>(Vec(213.104, 158.235), module, Arpeggiator2::TRIGGER_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(66.955, 229.808), module, Arpeggiator2::PATT_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(221.792, 229.808), module, Arpeggiator2::LENGTH_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(143.654, 229.891), module, Arpeggiator2::TRANS_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(221.792, 281.223), module, Arpeggiator2::ARP_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(102.248, 279.733), module, Arpeggiator2::SCALE_PARAM));

		addInput(createInputCentered<gui::AHPort>(Vec(25.55, 228.913), module, Arpeggiator2::PATT_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(180.386, 228.913), module, Arpeggiator2::LENGTH_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(102.248, 228.997), module, Arpeggiator2::TRANS_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(25.55, 279.733), module, Arpeggiator2::TRIG_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(66.955, 279.733), module, Arpeggiator2::CLOCK_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(180.386, 280.328), module, Arpeggiator2::ARP_INPUT));

		addInput(createInputCentered<gui::AHPort>(Vec(35.0, 335.938), module, Arpeggiator2::PITCH_INPUT + 0));
		addInput(createInputCentered<gui::AHPort>(Vec(69.0, 335.938), module, Arpeggiator2::PITCH_INPUT + 1));
		addInput(createInputCentered<gui::AHPort>(Vec(103.0, 335.938), module, Arpeggiator2::PITCH_INPUT + 2));
		addInput(createInputCentered<gui::AHPort>(Vec(137.0, 335.938), module, Arpeggiator2::PITCH_INPUT + 3));
		addInput(createInputCentered<gui::AHPort>(Vec(171.0, 335.938), module, Arpeggiator2::PITCH_INPUT + 4));
		addInput(createInputCentered<gui::AHPort>(Vec(205.0, 335.938), module, Arpeggiator2::PITCH_INPUT + 5));

		addOutput(createOutputCentered<gui::AHPort>(Vec(24.0, 65.085), module, Arpeggiator2::OUT_OUTPUT));
		addOutput(createOutputCentered<gui::AHPort>(Vec(71.997, 65.085), module, Arpeggiator2::GATE_OUTPUT));
		addOutput(createOutputCentered<gui::AHPort>(Vec(168.143, 65.085), module, Arpeggiator2::EOC_OUTPUT));
		addOutput(createOutputCentered<gui::AHPort>(Vec(215.995, 65.085), module, Arpeggiator2::EOS_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(120.0, 65.085), module, Arpeggiator2::LOCK_LIGHT));

		if (module != NULL) {
			Arpeggiator2Display *display = createWidget<Arpeggiator2Display>(Vec(10, 95));
			display->module = module;
			display->box.size = Vec(100, 140);
			addChild(display);
		}

	}

	void appendContextMenu(Menu *menu) override {

		Arpeggiator2 *arp = dynamic_cast<Arpeggiator2*>(module);
		assert(arp);

		struct GateModeItem : MenuItem {
			Arpeggiator2 *module;
			Arpeggiator2::GateMode gateMode;
			void onAction(const rack::event::Action &e) override {
				module->gateMode = gateMode;
			}
		};

		struct GateModeMenu : MenuItem {
			Arpeggiator2 *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<Arpeggiator2::GateMode> modes = {Arpeggiator2::TRIGGER, Arpeggiator2::RETRIGGER, Arpeggiator2::CONTINUOUS};
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
		item->module = arp;
		menu->addChild(item);

	}

};

Model *modelArpeggiator2 = createModel<Arpeggiator2, Arpeggiator2Widget>("Arpeggiator2");

