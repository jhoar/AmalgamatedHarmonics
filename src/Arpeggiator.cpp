#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"
#include "components.hpp"
#include "dsp/digital.hpp"

#include <iostream>

struct Arpeggiator : Module {

	const static int MAX_STEPS = 16;
	const static int MAX_DIST = 12; //Octave
	const static int NUM_PITCHES = 6;

	enum ParamIds {
		STEP_PARAM,
		DIST_PARAM,
		PDIR_PARAM,
		SDIR_PARAM,
		LOCK_PARAM,
		TRIGGER_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		STEP_INPUT,
		DIST_INPUT,
		TRIG_INPUT,
		PITCH_INPUT,
		NUM_INPUTS = PITCH_INPUT + NUM_PITCHES
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
	
	Arpeggiator() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	
	SchmittTrigger clockTrigger; // for clock
	SchmittTrigger trigTrigger;  // for step trigger
	SchmittTrigger lockTrigger;
	SchmittTrigger buttonTrigger;
	
	PulseGenerator triggerPulse;
	PulseGenerator gatePulse;
	PulseGenerator eosPulse;
	PulseGenerator eocPulse;

	float pitches[NUM_PITCHES];
	float inputPitches[NUM_PITCHES];
	bool pitchStatus[NUM_PITCHES];
	float pitchValue[NUM_PITCHES];

	int inputPDir;
	int pDir = 0;
	
	int inputSDir;
	int sDir = 0;

	int inputStep = 0;
	int nStep = 0;
	
	int inputDist = 0;
	int nDist = 0;
	
	bool locked = false;

	float outVolts;
	bool isRunning = false;
	bool freeRunning = false;

	int cycleLength = 0;
	int stepI = 0;
	int cycleI = 0;
	int stepsRemaining;
	int cycleRemaining;
	int currDist = 0;

	int stepX = 0;
	int poll = 5000;
	
	inline bool debug() {
		return false;
	}
	
};

void Arpeggiator::step() {
	
	stepX++;
	
	// Wait a few steps for the inputs to flow through Rack
	if (stepX < 10) { 
		return;
	}
	
	// Get the clock rate and semi-tone
	float delta = 1.0 / engineGetSampleRate();	
	float semiTone = 1.0 / 12.0;
	
	// Get inputs from Rack
	float clockInput	= inputs[CLOCK_INPUT].value;
	float trigInput		= inputs[TRIG_INPUT].value;
	float trigActive	= inputs[TRIG_INPUT].active;
	float lockInput		= params[LOCK_PARAM].value;
	float buttonInput	= params[TRIGGER_PARAM].value;
	
	float iPDir			= params[PDIR_PARAM].value;
	float iSDir			= params[SDIR_PARAM].value;
	
	float iStep;
	if (inputs[STEP_INPUT].active) {
		iStep = inputs[STEP_INPUT].value;
		inputStep 	= round(rescalef(iStep, -10, 10, 0, MAX_STEPS));
	} else {
		iStep = params[STEP_PARAM].value;
		inputStep = iStep;
	}

	float iDist;
	if (inputs[DIST_INPUT].active) {
		iDist = inputs[DIST_INPUT].value;
		inputDist 	= round(rescalef(iDist, -10, 10, 0, MAX_DIST));
	} else {
		iDist = params[DIST_PARAM].value;
		inputDist = iDist;
	}
		
	for (int p = 0; p < NUM_PITCHES; p++) {
		int index = PITCH_INPUT + p;
		pitchStatus[p] 	= inputs[index].active;
		pitchValue[p] 	= inputs[index].value;
	}
	
	
	// Process inputs
	bool clockStatus	= clockTrigger.process(clockInput);
	bool triggerStatus	= trigTrigger.process(trigInput);
	bool lockStatus		= lockTrigger.process(lockInput);
	bool buttonStatus 	= buttonTrigger.process(buttonInput);
		
	inputPDir 	= iPDir;
	inputSDir	= iSDir;
	
	int cycleLength = 0;
	for (int p = 0; p < NUM_PITCHES; p++) {
		if (pitchStatus[p]) { //Plugged in 
			inputPitches[cycleLength] = pitchValue[p];
			cycleLength++;
		}
	}
	
	
	// Check that we even have anything plugged in
	if (cycleLength == 0) {
		return; // No inputs, no music
	}
	
	
	// If there are still steps left after the end of a cycle, start a new cycle
	bool newCycle = false;
	bool newSequence = false;
	
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
		newSequence = true;
		newCycle = true;		
		if (debug()) { std::cout << stepX << " Triggered" << std::endl; }
	}
	
	
	// So this is where the free-running could be triggered
	if (isClocked && !isRunning) { // Must have a clock and not be already running
		if (!trigActive) { // If nothing plugged into the TRIG input
			if (debug()) { std::cout << stepX << " Free running sequence; starting" << std::endl; }
			freeRunning = true; // We're free-running
			newSequence = true;
			newCycle = true;
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
	if (isRunning && isClocked && cycleRemaining == 0) {
		
		// Completed 1 step
		stepI++;
		stepsRemaining--;
		
		// Pulse the EOC gate
		eocPulse.trigger(5e-4);
		if (debug()) { std::cout << stepX << " Finished Cycle S: " << stepI <<
			" C: " << cycleI <<
			" sRemain: " << stepsRemaining <<
			" cRemain: " << cycleRemaining << std::endl;
		}
		
		// Reached the end of the sequence
		if (isRunning && stepsRemaining == 0) {
		
			// Free running, so start new seqeuence & cycle
			if (freeRunning) {
				newCycle = true;
				newSequence = true;
			} 

			isRunning = false;
			
			// Pulse the EOS gate
			eosPulse.trigger(5e-4);
			if (debug()) { std::cout << stepX << " Finished sequence S: " << stepI <<
				" C: " << cycleI <<
				" sRemain: " << stepsRemaining <<
				" cRemain: " << cycleRemaining << 
				" flag:" << isRunning << std::endl;
			}

		} else {
			newCycle = true;
			if (debug()) { std::cout << stepX << " Flagging new cycle" << std::endl; }
		}
		
	}
	
	
	
	// Capture the settings for this cycle
	if (newCycle) {
		
		// Update params
		if (!locked) {
			if (debug()) { std::cout << stepX << " Read sequence inputs" << std::endl; }
			nStep = inputStep;
			nDist = inputDist;
			pDir = inputPDir;
			sDir = inputSDir;
		}

		// If we have no steps to play, play nothing
		if (nStep == 0) {
			return; // No steps, abort
		} 
	
	}
	
	
	// If we have been triggered, start a new sequence
	if (newSequence) {
		
		if (debug()) { std::cout << stepX << " New Sequence" << std::endl;	}
		
		// We're running now
		isRunning = true;
		
		// At the first step of the sequence
		stepsRemaining = nStep;	
		stepI = 0;
	
		// At the beginning of the sequence (i.e. dist = 0)
		currDist = 0;
		
	} 
	
	
	// Starting a new cycle
	if (newCycle) {
		
		if (debug()) {
			std::cout << stepX << " Defining cycle: nStep: " << nStep << 
				" nDist: " << nDist << 
				" pDir: " << pDir << 
				" sDir: " << sDir << 
				" cycLength: " << cycleLength << 
				" seqLen: " << nStep * cycleLength;
			for (int i = 0; i < cycleLength; i++) {
				 std::cout << " P" << i << " V: " << inputPitches[i];
			}
			std::cout << std::endl;
		}
		
		/// Reset the cycle counters
		cycleRemaining = cycleLength;
		cycleI = 0;
		
		if (debug()) { std::cout << stepX << " New cycle" << std::endl; }
	
		// Deal with RND setting, when sDir == 1, force it up or down
		if (sDir == 1) {
			if (rand() % 2 == 0) {
				sDir = 0;
			} else {
				sDir = 2;
			}
		}
		
		// Only starting moving after the first cycle
		if (stepI) {	
			switch (sDir) {
				case 0:			currDist--; break;
				case 2:			currDist++; break;
				default: 		; 
			}
		}
		
		if (!locked) {// Pitches are locked, and so is the order. This keeps randomly generated arps fixed when locked
			for (int i = 0; i < cycleLength; i++) {
		
				int target;
						
				// Read the pitches according to direction, but we should do this for the sequence?
				switch (pDir) {
					case 0: target = cycleLength - i - 1; break; 		// DOWN
					case 1: target = rand() % cycleLength; break;		// RANDOM
					case 2: target = i; break;							// UP
					default: target = i; break; // For random case, read randomly from array, so order does not matter
				}

				// How many semi-tones do we need to shift
				float dV = semiTone * nDist * currDist;
				pitches[i] = clampf(inputPitches[target] + dV, -10.0, 10.0);
		
				if (debug()) {
					std::cout << stepX << " Pitch: " << i << " stepI: " << stepI <<
						" dV:" << dV <<
						" target: " << target <<
						" in: " << inputPitches[target] <<
						" out: " << pitches[target] << std::endl;
				}				
			}
		}
		
		if (debug()) {
			std::cout << stepX << " Output pitches: ";
			for (int i = 0; i < cycleLength; i++) {
				 std::cout << " P" << i << " V: " << pitches[i];
			}
			std::cout << std::endl;
		}
		
	}
	
	// Advance the sequence
	// Are we starting a sequence or are running and have been clocked; if so advance the sequence
	// Only advance from the clock
	if (isRunning && isClocked) {

		if (debug()) { std::cout << stepX << " Advance Cycle S: " << stepI <<
			" C: " << cycleI <<
			" sRemain: " << stepsRemaining <<
			" cRemain: " << cycleRemaining << std::endl;
		}
		
		// Finally set the out voltage
		outVolts = pitches[cycleI];
		
		if (debug()) { std::cout << stepX << " Output V = " << outVolts << std::endl; }
		
		// Update counters
		cycleI++;
		cycleRemaining--;
		
		// Pulse the output gate
		gatePulse.trigger(5e-4);
		
	}	
	
	bool sPulse = eosPulse.process(delta);
	bool cPulse = eocPulse.process(delta);
	bool gPulse = gatePulse.process(delta);
	
	// Set the value
	lights[LOCK_LIGHT].value = locked ? 1.0 : 0.0;
	outputs[OUT_OUTPUT].value = outVolts;
	outputs[GATE_OUTPUT].value = gPulse ? 10.0 : 0.0;
	outputs[EOS_OUTPUT].value = sPulse ? 10.0 : 0.0;
	outputs[EOC_OUTPUT].value = cPulse ? 10.0 : 0.0;
			
}

struct ArpeggiatorDisplay : TransparentWidget {
	
	Arpeggiator *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	ArpeggiatorDisplay() {
		font = Font::load(assetPlugin(plugin, "res/Roboto-Light.ttf"));
	}

	void draw(NVGcontext *vg) override {
	
		Vec pos = Vec(0, 20);

		nvgFontSize(vg, 20);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -1);

		nvgFillColor(vg, nvgRGBA(212, 175, 55, 0xff));
		char text[128];

		snprintf(text, sizeof(text), "STEP: %d [%d]", module->nStep, module->inputStep);
		nvgText(vg, pos.x + 10, pos.y + 5, text, NULL);
		snprintf(text, sizeof(text), "DIST: %d [%d]", module->nDist, module->inputDist);
		nvgText(vg, pos.x + 10, pos.y + 25, text, NULL);
		
		if (module->sDir == 0) {
			snprintf(text, sizeof(text), "SEQ: DSC");			
		} else {
			snprintf(text, sizeof(text), "SEQ: ASC");			
		}
		nvgText(vg, pos.x + 10, pos.y + 45, text, NULL);
		
		switch(module->pDir) {
			case 0: snprintf(text, sizeof(text), "ARP: R-L"); break;
			case 1: snprintf(text, sizeof(text), "ARP: RND"); break;
			case 2: snprintf(text, sizeof(text), "ARP: L-R"); break;
			default: snprintf(text, sizeof(text), "ARP: ERR"); break;
		}
		
		nvgText(vg, pos.x + 10, pos.y + 65, text, NULL);
		
		std::string inputs ("IN: ");
		
		for (int p = 0; p < Arpeggiator::NUM_PITCHES; p++) {
			if (module->pitchStatus[p] && module->pitchValue[p] > -9.999) { //Plugged in or approx -10.0
				inputs = inputs + std::to_string(p + 1);
			}
		}
		nvgText(vg, pos.x + 10, pos.y + 85, inputs.c_str(), NULL);
				
	}
	
};

ArpeggiatorWidget::ArpeggiatorWidget() {
	Arpeggiator *module = new Arpeggiator();
	
	UI ui;
	
	setModule(module);
	box.size = Vec(240, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Arpeggiator.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

	{
		ArpeggiatorDisplay *display = new ArpeggiatorDisplay();
		display->module = module;
		display->box.pos = Vec(10, 95);
		display->box.size = Vec(100, 140);
		addChild(display);
	}

	addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 0, false, false),  module, Arpeggiator::OUT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 1, 0, false, false),  module, Arpeggiator::GATE_OUTPUT));
	addParam(createParam<AHButton>(ui.getPosition(UI::BUTTON, 2, 0, false, false), module, Arpeggiator::LOCK_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(ui.getPosition(UI::LIGHT, 2, 0, false, false), module, Arpeggiator::LOCK_LIGHT));
	addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 3, 0, false, false), module, Arpeggiator::EOC_OUTPUT));
	addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 0, false, false), module, Arpeggiator::EOS_OUTPUT));
		
	addParam(createParam<BefacoPush>(Vec(127, 155), module, Arpeggiator::TRIGGER_PARAM, 0.0, 1.0, 0.0));
	
		
	for (int i = 0; i < Arpeggiator::NUM_PITCHES; i++) {
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, i, 5, true, false),  module, Arpeggiator::PITCH_INPUT + i));
	}
	
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 4, true, false), module, Arpeggiator::STEP_INPUT));
    addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 1, 4, true, false), module, Arpeggiator::STEP_PARAM, 0.0, 16.0, 0.0)); 
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 4, true, false), module, Arpeggiator::DIST_INPUT));
    addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 3, 4, true, false), module, Arpeggiator::DIST_PARAM, 0.0, 12.0, 0.0));
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 4, true, false), module, Arpeggiator::TRIG_INPUT));
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 5, 4, true, false), module, Arpeggiator::CLOCK_INPUT));
	
	addParam(createParam<BefacoSwitch>(Vec(178.5, 112.0), module, Arpeggiator::SDIR_PARAM, 0, 2, 0));
	addParam(createParam<BefacoSwitch>(Vec(178.5, 187.0), module, Arpeggiator::PDIR_PARAM, 0, 2, 0));

}



