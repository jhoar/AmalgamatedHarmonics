#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"
#include "components.hpp"
#include "dsp/digital.hpp"

#include <iostream>

struct Pattern {
	
	virtual std::string getName() = 0;

	virtual void initialise(int l) = 0;

	virtual void advance() = 0;

	virtual int getOffset() = 0;
	
	virtual bool isPatternFinished() = 0;

};

struct UpPattern : Pattern {

	int currSt;
	int length;
	bool started = false;

	std::string name{"Up"};

	std::string getName() override {
		return name;
	};

	void initialise(int l) override {
		length = l;
		currSt = 0;
	}
	
	void advance() override {
		currSt++;
	}
	
	int getOffset() override {
		return currSt;
	}

	bool isPatternFinished() override {
		return(currSt == length);
	}
	
};

struct DownPattern : Pattern {

	int currSt;
	int length;
	
	std::string name{"Down"};
	
	std::string getName() override {
		return name;
	};	

	void initialise(int l) override {
		length = l;
		currSt = length - 1;
	}
	
	void advance() override {
		currSt--;
	}
	
	int getOffset() override {
		return currSt;
	}

	bool isPatternFinished() override {
		return (currSt < 0);
	}
	
};

struct UpDownPattern : Pattern {

	int currSt;
	int mag;
	int end;
	
	std::string name{"UpDown"};
	
	std::string getName() override {
		return name;
	};	

	void initialise(int l) override {
		mag = l - 1;
		end = 2 * l - 2;
		if (end < 1) {
			end = 1;
		}
		currSt = 0;
	}
	
	void advance() override {
		currSt++;
	}
	
	int getOffset() override {
		return mag - abs(mag - currSt);
	}

	bool isPatternFinished() override {
		return(currSt == end);
	}
	
};

struct DownUpPattern : Pattern {

	int currSt;
	int mag;
	int end;
	
	std::string name{"DownUp"};
	
	std::string getName() override {
		return name;
	};	

	void initialise(int l) override {
		mag = l - 1;
		end = 2 * l - 2;
		if (end < 1) {
			end = 1;
		}
		currSt = 0;
	}
	
	void advance() override {
		currSt++;
	}
	
	int getOffset() override {
		return -(mag - abs(mag - currSt));
	}

	bool isPatternFinished() override {
		return(currSt == end);
	}
	
};



struct Arpeggio {

	std::string name{"null"};

	virtual void initialise(int nPitches) = 0;
	
	virtual void advance() = 0;
	
	virtual int getPitch() = 0;
	
	virtual bool isArpeggioFinished() = 0;
	
	

};

struct UpArp : Arpeggio {

	std::string name{"Up"};
	
	int index;
	int nPitches;

	void initialise(int np) override {
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
		PARAM_PARAM,
		VALUE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		TRIG_INPUT,
		PITCH_INPUT,
		PATT_INPUT = PITCH_INPUT + NUM_PITCHES,
		ARP_INPUT,
		LENGTH_INPUT,
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
	
	enum Parameters {
		STEPSIZE,
		REPEATS	
	};
	
	float delta;
		
	Arpeggiator2() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		delta = 1.0 / engineGetSampleRate();
	}
	
	void onSampleRateChange() override { 
		delta = 1.0 / engineGetSampleRate();
	}
	
	void step() override;
	
	
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
	
	int stepX = 0;
	int poll = 5000;
		
	inline bool debug() {
		return true;
	}
	
	
	int pattern;
	int arp;
	int length;
	float param1;
	float param2;
	
	float semiTone = 1.0 / 12.0;

	UpPattern patt_up; 
	DownPattern patt_down; 
	UpDownPattern patt_updown;
	DownUpPattern patt_downup;
	
	UpArp arp_up;

	Pattern *currPatt = &patt_up;
	Arpeggio *currArp = &arp_up;
	
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
	float trigInput		= inputs[TRIG_INPUT].value;
	float trigActive	= inputs[TRIG_INPUT].active;
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
	int pParam   = params[PARAM_PARAM].value;
	float pValue = params[VALUE_PARAM].value;

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
		newSequence = COUNTDOWN;
		newCycle = COUNTDOWN;		
		if (debug()) { std::cout << stepX << " Triggered" << std::endl; }
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

			switch(pattern) {
				case 0:		currPatt = &patt_up; 		break;
				case 1:		currPatt = &patt_down;		break;
				case 2:		currPatt = &patt_updown;	break;
				case 3:		currPatt = &patt_downup;	break;
				default:	currPatt = &patt_up;		break;
			};
			
		}

		if (debug()) { std::cout << stepX << " Initiatise new Sequence: Pattern: " << currPatt->getName() << 
			" Length: " << inputLen <<
			" Locked: " << locked << std::endl; }
		
		currPatt->initialise(length);
		
		// We're running now
		isRunning = true;
		
	} 
	
	
	// Starting a new cycle
	if (newCycle == LAUNCH) {

		if (debug()) { std::cout << stepX << " Initiatise new Cycle" << std::endl; }
				
		/// Reset the cycle counters
		if (!locked) {
			
			arp = inputArp;
			
			switch(arp) {
				default:	currArp = &arp_up;	break;
			};
			
			// Copy pitches
			for (int p = 0; p < nValidPitches; p++) {
				pitches[p] = inputPitches[p];
			}
			nPitches = nValidPitches;
				
		}

		currArp->initialise(nPitches);
		
	}
	
	
	// Advance the sequence
	// Are we starting a sequence or are running and have been clocked; if so advance the sequence
	// Only advance from the clock
	if (isRunning && (isClocked || newCycle == LAUNCH)) {

		if (debug()) { std::cout << stepX << " Advance Cycle" << std::endl; }
		
		// Finally set the out voltage
		outVolts = clampf(pitches[currArp->getPitch()] + semiTone * (float)currPatt->getOffset(), -10.0, 10.0);
		
		if (debug()) { std::cout << stepX << " Output V = " << outVolts << std::endl; }
		
		// Update counters
		currArp->advance();
		
		// Pulse the output gate
		gatePulse.trigger(5e-4);
		
	}	
	
	// Set the value
	lights[LOCK_LIGHT].value = locked ? 1.0 : 0.0;
	outputs[OUT_OUTPUT].value = outVolts;
	
	bool gPulse = gatePulse.process(delta);
	bool sPulse = eosPulse.process(delta);
	bool cPulse = eocPulse.process(delta);
	outputs[GATE_OUTPUT].value = gPulse ? 10.0 : 0.0;
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
	
		Vec pos = Vec(0, 20);

		nvgFontSize(vg, 20);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -1);

		nvgFillColor(vg, nvgRGBA(212, 175, 55, 0xff));
	
		std::string inputs ("Hello!");
		
		nvgText(vg, pos.x + 10, pos.y + 85, inputs.c_str(), NULL);
				
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
		
	addParam(createParam<BefacoPush>(Vec(127, 155), module, Arpeggiator2::TRIGGER_PARAM, 0.0, 1.0, 0.0));
	
	for (int i = 0; i < Arpeggiator2::NUM_PITCHES; i++) {
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, i, 5, true, false),  module, Arpeggiator2::PITCH_INPUT + i));
	}
	
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 4, true, false), module, Arpeggiator2::PATT_INPUT));
	addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 3, 4, true, false), module, Arpeggiator2::PATT_PARAM, 0.0, 16.0, 0.0)); 
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 4, true, false), module, Arpeggiator2::ARP_INPUT));
	addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 5, 4, true, false), module, Arpeggiator2::ARP_PARAM, 0.0, 16.0, 0.0)); 
	
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 4, true, false), module, Arpeggiator2::TRIG_INPUT));
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 1, 4, true, false), module, Arpeggiator2::CLOCK_INPUT));
	
	addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 4, 2, true, false), module, Arpeggiator2::PARAM_PARAM, 0, 5, 0)); 
	addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 5, 2, true, false), module, Arpeggiator2::VALUE_PARAM, -5.0, 5.0, 0.0)); 
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 3, true, false), module, Arpeggiator2::LENGTH_INPUT));
	addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 5, 3, true, false), module, Arpeggiator2::LENGTH_PARAM, 1.0, 16.0, 1.0)); 

}



