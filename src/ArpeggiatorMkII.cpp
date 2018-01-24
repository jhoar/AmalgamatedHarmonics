#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"
#include "components.hpp"
#include "dsp/digital.hpp"

#include <iostream>

struct Pattern {
	
	int length;
	int trans;
	int scale;
	int count;
		
	int MAJOR[7] = {0,2,4,5,7,9,11};
	int MINOR[7] = {0,2,3,5,7,8,10};
		
	virtual std::string getName() = 0;

	virtual void initialise(int l, int sc, int tr, bool freeRun) {
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

	void initialise(int l, int sc, int tr, bool fr) override {
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

	int currSt;
	
	std::string getName() override {
		return "Down";
	};	

	void initialise(int l, int sc, int tr, bool fr) override {
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

	int mag;
	int end;
	
	std::string getName() override {
		return "UpDown";
	};	

	void initialise(int l, int sc, int tr, bool fr) override {
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
		
		int note = (mag - abs(mag - count));
		
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

	int mag;
	int end;
	
	std::string getName() override {
		return "DownUp";
	};	

	void initialise(int l, int sc, int tr, bool fr) override {
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

		int note = -(mag - abs(mag - count));
		
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
	
	void initialise(int l, int sc, int tr, bool fr) override {
		Pattern::initialise(l,sc,tr,fr);
	}
	
	int getOffset() override {
		return getNote(count);
	}

	bool isPatternFinished() override {
		return (count == (int)notes.size());
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

	virtual void initialise(int nPitches, bool fr) = 0;
	
	virtual void advance() = 0;
	
	virtual int getPitch() = 0;
	
	virtual bool isArpeggioFinished() = 0;

};

struct RightArp : Arpeggio {

	int index;
	int nPitches;

	std::string getName() override {
		return "Right";
	};

	void initialise(int np, bool fr) override {
		index = 0;
		nPitches = np;
	}
	
	void advance() override {
		index++;
	}
	
	int getPitch() override {
		return index;
	}
	
	bool isArpeggioFinished() override {
		return (index == nPitches);
	}
	
};

struct LeftArp : Arpeggio {

	int index;
	int nPitches;

	std::string getName() override {
		return "Left";
	};
	
	void initialise(int np, bool fr) override {
		nPitches = np;
		index = nPitches - 1;
	}
	
	void advance() override {
		index--;
	}
	
	int getPitch() override {
		return index;
	}

	bool isArpeggioFinished() override {
		return (index < 0);
	}
	
};

struct RightLeftArp : Arpeggio {

	int currSt;
	int mag;
	int end;
	
	std::string getName() override {
		return "RightLeft";
	};	

	void initialise(int l, bool fr) override {
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
	
	int getPitch() override {
		return mag - abs(mag - currSt);
	}

	bool isArpeggioFinished() override {
		return(currSt == end);
	}
	
};

struct LeftRightArp : Arpeggio {

	int currSt;
	int mag;
	int end;
	
	std::string getName() override {
		return "LeftRight";
	};	

	void initialise(int l, bool fr) override {
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
	
	int getPitch() override {
		return abs(mag - currSt);
	}

	bool isArpeggioFinished() override {
		return(currSt == end);
	}
	
};



struct Arpeggiator2 : Module {
	
	const static int MAX_STEPS = 16;
	const static int MAX_DIST = 12; //Octave
	const static int NUM_PITCHES = 6;

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
		PITCH_INPUT,
		PATT_INPUT = PITCH_INPUT + NUM_PITCHES,
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
	
	float delta;
		
	Arpeggiator2() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		delta = 1.0 / engineGetSampleRate();
	}
	
	void onSampleRateChange() override { 
		delta = 1.0 / engineGetSampleRate();
	}
	
	void step() override;
	
	json_t *toJson() override {
		json_t *rootJ = json_object();

		// gateMode
		json_t *gateModeJ = json_integer((int) gateMode);
		json_object_set_new(rootJ, "gateMode", gateModeJ);

		return rootJ;
	}
	
	void fromJson(json_t *rootJ) override {
		// gateMode
		json_t *gateModeJ = json_object_get(rootJ, "gateMode");
		if (gateModeJ)
			gateMode = (GateMode)json_integer_value(gateModeJ);
	}
	
	enum GateMode {
		TRIGGER,
		RETRIGGER,
		CONTINUOUS,
	};
	GateMode gateMode = TRIGGER;
	
	SchmittTrigger clockTrigger; // for clock
	SchmittTrigger trigTrigger;  // for step trigger
	SchmittTrigger lockTrigger;
	SchmittTrigger buttonTrigger;
	
	PulseGenerator triggerPulse;
	PulseGenerator gatePulse;
	PulseGenerator eosPulse;
	PulseGenerator eocPulse;

	bool locked = false;

	float outVolts;
	bool isRunning = false;
	bool freeRunning = false;
	
	int newSequence = 0;
	int newCycle = 0;
	const static int LAUNCH = 1;
	const static int COUNTDOWN = 3;
	
	int inputPat;
	int inputArp;
	int inputLen;
	int inputTrans;
	int inputScale;
	
	int stepX = 0;
	int poll = 5000;
		
	inline bool debug() {
		return false;
	}
	
	
	int pattern;
	int arp;
	int length;
	float trans;
	float scale;
	
	float semiTone = 1.0 / 12.0;

	UpPattern		patt_up; 
	DownPattern 	patt_down; 
	UpDownPattern 	patt_updown;
	DownUpPattern 	patt_downup;
	RezPattern 		patt_rez;
	OnTheRunPattern	patt_ontherun;
	
	RightArp 		arp_right;
	LeftArp 		arp_left;
	RightLeftArp 	arp_rightleft;
	LeftRightArp 	arp_leftright;

	Pattern *currPatt = &patt_up;
	Arpeggio *currArp = &arp_right;
	
	float pitches[6];
	int nPitches;
	
};


void Arpeggiator2::step() {
	
	stepX++;
	
	// Wait a few steps for the inputs to flow through Rack
	if (stepX < 10) { 
		return;
	}
		
	// Get inputs from Rack
	float clockInput	= inputs[CLOCK_INPUT].value;
	bool  clockActive	= inputs[CLOCK_INPUT].active;
	float trigInput		= inputs[TRIG_INPUT].value;
	bool  trigActive	= inputs[TRIG_INPUT].active;
	float lockInput		= params[LOCK_PARAM].value;
	float buttonInput	= params[TRIGGER_PARAM].value;
	
	
	if (inputs[PATT_INPUT].active) {
		inputPat = inputs[PATT_INPUT].value;
	} else {
		inputPat = params[PATT_PARAM].value;
	}	

	if (inputs[ARP_INPUT].active) {
		inputArp = inputs[ARP_INPUT].value;
	} else {
		inputArp = params[ARP_PARAM].value;
	}	

	if (inputs[LENGTH_INPUT].active) {
		inputLen = inputs[LENGTH_INPUT].value;
	} else {
		inputLen = params[LENGTH_PARAM].value;
	}	
	
	// Read param section
	if (inputs[TRANS_INPUT].active) {
		inputTrans = inputs[TRANS_INPUT].value;
	} else {
		inputTrans = params[TRANS_PARAM].value;
	}	

	inputScale = params[SCALE_PARAM].value;

	// Process inputs
	bool clockStatus	= clockTrigger.process(clockInput);
	bool triggerStatus	= trigTrigger.process(trigInput);
	bool lockStatus		= lockTrigger.process(lockInput);
	bool buttonStatus 	= buttonTrigger.process(buttonInput);
	
	
	int nValidPitches = 0;
	float inputPitches[NUM_PITCHES];
	for (int p = 0; p < NUM_PITCHES; p++) {
		int index = PITCH_INPUT + p;
		if (inputs[index].active) {
			inputPitches[nValidPitches] = inputs[index].value;
			nValidPitches++;
		}
	}
	
		
	// Check that we even have anything plugged in
	if (nValidPitches == 0) {
		return; // No inputs, no music
	}

	if (inputLen == 0) {
		return; // No inputs, no music
	}
	
	if (!clockActive) {
		isRunning = false;
	}
		
	// Has the trigger input been fired
	if (triggerStatus) {
		triggerPulse.trigger(5e-5);
		if (debug()) { std::cout << stepX << " Triggered" << std::endl; }
	}
	
	
	// Update the trigger pulse and determine if it is still high
	bool triggerHigh = triggerPulse.process(delta);
	if (debug()) { 
		if (triggerHigh) {
			std::cout << stepX << " Trigger is high" << std::endl;
		}
	}
	
	
	// Update lock
	if (lockStatus) {
		if (debug()) { std::cout << "Toggling lock: " << locked << std::endl; }
		locked = !locked;
	}
	
	
	if (newSequence) {
		newSequence--;
		if (debug()) { std::cout << stepX << " Countdown newSequence: " << newSequence << std::endl; }
	}


	if (newCycle) {
		newCycle--;
		if (debug()) { std::cout << stepX << " Countdown newCycle: " << newCycle << std::endl; }
	}
	
	// OK so the problem here might be that the clock gate is still high right after the trigger gate fired on the previous step
	// So we need to wait a while for the clock gate to go low
	// Has the clock input been fired
	bool isClocked = false;
	if (clockStatus && !triggerHigh) {
		if (debug()) { std::cout << stepX << " Clocked" << std::endl; }
		isClocked = true;
	}
	
	
	// Has the trigger input been fired, either on the input or button
	if (triggerStatus || buttonStatus) {
		if (debug()) { std::cout << stepX << " Start countdown " << clockActive <<std::endl; }
		if (clockActive) {
			newSequence = COUNTDOWN;
			newCycle = COUNTDOWN;
		}
	}
	
	
	// So this is where the free-running could be triggered
	if (isClocked && !isRunning) { // Must have a clock and not be already running
		if (!trigActive) { // If nothing plugged into the TRIG input
			if (debug()) { std::cout << stepX << " Free running sequence; starting" << std::endl; }
			freeRunning = true; // We're free-running
			newSequence = COUNTDOWN;
			newCycle = LAUNCH;
		} else {
			if (debug()) { std::cout << stepX << " Triggered sequence; wait for trigger" << std::endl; }
			freeRunning = false;
		}
	}
	
	
	// Detect cable being plugged in when free-running, stop free-running
	if (freeRunning && trigActive && isRunning) {
		if (debug()) { std::cout << stepX << " TRIG input re-connected" << std::endl; }
		freeRunning = false;
	}	
	
	
	// Reached the end of the cycle
	if (isRunning && isClocked && currArp->isArpeggioFinished()) {
		
		// Completed 1 step
		currPatt->advance();
		
		// Pulse the EOC gate
		eocPulse.trigger(5e-3);
		if (debug()) { std::cout << stepX << " Finished Cycle" << std::endl; }
		
		// Reached the end of the sequence
		if (isRunning && currPatt->isPatternFinished()) {
		
			// Free running, so start new seqeuence & cycle
			if (freeRunning) {
				newCycle = COUNTDOWN;
				newSequence = COUNTDOWN;
			} 

			isRunning = false;
			
			// Pulse the EOS gate
			eosPulse.trigger(5e-3);
			if (debug()) { std::cout << stepX << " Finished Sequence, flag: " << isRunning << std::endl; }

		} else {
			newCycle = LAUNCH;
			if (debug()) { std::cout << stepX << " Flagging new cycle" << std::endl; }
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

		if (debug()) { std::cout << stepX << " Initiatise new Sequence: Pattern: " << currPatt->getName() << 
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
			for (int p = 0; p < nValidPitches; p++) {
				pitches[p] = inputPitches[p];
			}
			nPitches = nValidPitches;
				
		}

		if (debug()) { std::cout << stepX << " Initiatise new Cycle: " << nPitches << " " << currArp->getName() << std::endl; }

		currArp->initialise(nPitches, freeRunning);
		
	}
	
	
	// Advance the sequence
	// Are we starting a sequence or are running and have been clocked; if so advance the sequence
	// Only advance from the clock
	if (isRunning && (isClocked || newCycle == LAUNCH)) {

		if (debug()) { std::cout << stepX << " Advance Cycle: " << currArp->getPitch() << std::endl; }
				
		// Finally set the out voltage
		outVolts = clampf(pitches[currArp->getPitch()] + semiTone * (float)currPatt->getOffset(), -10.0, 10.0);
		
		if (debug()) { std::cout << stepX << " Output V = " << outVolts << std::endl; }
		
		// Update counters
		currArp->advance();
		
		// Pulse the output gate
		gatePulse.trigger(5e-3);
		
	}	
	
	// Set the value
	lights[LOCK_LIGHT].value = locked ? 1.0 : 0.0;
	outputs[OUT_OUTPUT].value = outVolts;
	
	bool gPulse = gatePulse.process(delta);
	bool sPulse = eosPulse.process(delta);
	bool cPulse = eocPulse.process(delta);
	
	bool gatesOn = isRunning;
//	std::cout << gatesOn << " " << gPulse << std::endl;
	if (gateMode == TRIGGER)
		gatesOn = gatesOn && gPulse;
	else if (gateMode == RETRIGGER)
		gatesOn = gatesOn && !gPulse;
	
	outputs[GATE_OUTPUT].value = gatesOn ? 10.0 : 0.0;
	outputs[EOS_OUTPUT].value = sPulse ? 10.0 : 0.0;
	outputs[EOC_OUTPUT].value = cPulse ? 10.0 : 0.0;
			
}

struct Arpeggiator2Display : TransparentWidget {
	
	Arpeggiator2 *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	Arpeggiator2Display() {
		font = Font::load(assetPlugin(plugin, "res/Roboto-Light.ttf"));
	}

	void draw(NVGcontext *vg) override {
	
		Vec pos = Vec(0, 15);

		nvgFontSize(vg, 20);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -1);

		nvgFillColor(vg, nvgRGBA(212, 175, 55, 0xff));
	
		char text[128];

		snprintf(text, sizeof(text), "Pattern: %s", module->currPatt->getName().c_str());
		nvgText(vg, pos.x + 10, pos.y + 5, text, NULL);

		snprintf(text, sizeof(text), "Length: %d", module->currPatt->length);
		nvgText(vg, pos.x + 10, pos.y + 25, text, NULL);

		switch(module->currPatt->scale) {
			case 0: snprintf(text, sizeof(text), "Transpose: %d semitones", module->currPatt->trans); break;
			case 1: snprintf(text, sizeof(text), "Transpose: %d Major int.", module->currPatt->trans); break;
			case 2: snprintf(text, sizeof(text), "Transpose: %d Minor int.", module->currPatt->trans); break;
			default: snprintf(text, sizeof(text), "Error..."); break;
		}
		nvgText(vg, pos.x + 10, pos.y + 45, text, NULL);

		snprintf(text, sizeof(text), "Arpeggio: %s", module->currArp->getName().c_str());
		nvgText(vg, pos.x + 10, pos.y + 65, text, NULL);
		
	}
	
};

Arpeggiator2Widget::Arpeggiator2Widget() {
	Arpeggiator2 *module = new Arpeggiator2();
	
	UI ui;
	
	setModule(module);
	box.size = Vec(240, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Arpeggiator2.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

	{
		Arpeggiator2Display *display = new Arpeggiator2Display();
		display->module = module;
		display->box.pos = Vec(10, 95);
		display->box.size = Vec(100, 140);
		addChild(display);
	}

	addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 0, false, false),  module, Arpeggiator2::OUT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 1, 0, false, false),  module, Arpeggiator2::GATE_OUTPUT));
	addParam(createParam<AHButton>(ui.getPosition(UI::BUTTON, 2, 0, false, false), module, Arpeggiator2::LOCK_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(ui.getPosition(UI::LIGHT, 2, 0, false, false), module, Arpeggiator2::LOCK_LIGHT));
	addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 3, 0, false, false), module, Arpeggiator2::EOC_OUTPUT));
	addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 0, false, false), module, Arpeggiator2::EOS_OUTPUT));
		
	addParam(createParam<BefacoPush>(Vec(195, 148), module, Arpeggiator2::TRIGGER_PARAM, 0.0, 1.0, 0.0));
	
	for (int i = 0; i < Arpeggiator2::NUM_PITCHES; i++) {
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, i, 5, true, false),  module, Arpeggiator2::PITCH_INPUT + i));
	}
	
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 4, true, false), module, Arpeggiator2::ARP_INPUT));
	addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 5, 4, true, false), module, Arpeggiator2::ARP_PARAM, 0.0, 3.0, 0.0)); 
	
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 4, true, false), module, Arpeggiator2::TRIG_INPUT));
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 1, 4, true, false), module, Arpeggiator2::CLOCK_INPUT));
	addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 3, 4, true, false), module, Arpeggiator2::SCALE_PARAM, 0, 2, 0)); 

	
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 3, true, false), module, Arpeggiator2::PATT_INPUT));
	addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 1, 3, true, false), module, Arpeggiator2::PATT_PARAM, 0.0, 5.0, 0.0)); 
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 3, true, false), module, Arpeggiator2::TRANS_INPUT)); 
	addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 3, 3, true, false), module, Arpeggiator2::TRANS_PARAM, -24, 24, 0)); 
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 3, true, false), module, Arpeggiator2::LENGTH_INPUT));
	addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 5, 3, true, false), module, Arpeggiator2::LENGTH_PARAM, 1.0, 16.0, 1.0)); 

}

struct ArpGateModeItem : MenuItem {
	Arpeggiator2 *arp;
	Arpeggiator2::GateMode gateMode;
	void onAction(EventAction &e) override {
		arp->gateMode = gateMode;
	}
	void step() override {
		rightText = (arp->gateMode == gateMode) ? "✔" : "";
	}
};

Menu *Arpeggiator2Widget::createContextMenu() {
	Menu *menu = ModuleWidget::createContextMenu();

	MenuLabel *spacerLabel = new MenuLabel();
	menu->pushChild(spacerLabel);

	Arpeggiator2 *arp = dynamic_cast<Arpeggiator2*>(module);
	assert(arp);

	MenuLabel *modeLabel = new MenuLabel();
	modeLabel->text = "Gate Mode";
	menu->pushChild(modeLabel);

	ArpGateModeItem *triggerItem = new ArpGateModeItem();
	triggerItem->text = "Trigger";
	triggerItem->arp = arp;
	triggerItem->gateMode = Arpeggiator2::TRIGGER;
	menu->pushChild(triggerItem);

	ArpGateModeItem *retriggerItem = new ArpGateModeItem();
	retriggerItem->text = "Retrigger";
	retriggerItem->arp = arp;
	retriggerItem->gateMode = Arpeggiator2::RETRIGGER;
	menu->pushChild(retriggerItem);

	ArpGateModeItem *continuousItem = new ArpGateModeItem();
	continuousItem->text = "Continuous";
	continuousItem->arp = arp;
	continuousItem->gateMode = Arpeggiator2::CONTINUOUS;
	menu->pushChild(continuousItem);

	return menu;
}


