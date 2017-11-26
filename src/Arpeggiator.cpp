#include "AH.hpp"
#include "Core.hpp"
#include "components.hpp"
#include "dsp/digital.hpp"

#include <iostream>

struct AHKnob : SVGKnob {
	AHKnob() {
		setSVG(SVG::load(assetPlugin(plugin,"res/ComponentLibrary/AHKnob.svg")));
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
	}
};

struct Arpeggiator : Module {

	enum ParamIds {
		DIR_PARAM,
		LOCK_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		STEP_INPUT,
		DIST_INPUT,
		TRIG_INPUT,
		PITCH_INPUT,
		NUM_INPUTS = PITCH_INPUT + 5
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
	
	int PATT_UP[17] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}; 
	int PATT_DN[17] = {0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15,-16}; 

	Arpeggiator() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	
	int MAX_STEPS = 16;
	int NUM_PITCHES = 5;
		
	SchmittTrigger clockTrigger; // for clock
	SchmittTrigger trigTrigger;  // for step trigger
    SchmittTrigger lockTrigger;
	
	PulseGenerator gatePulse;
	PulseGenerator eosPulse;
	PulseGenerator eocPulse;
		
	bool locked = false;
	float pitches[5];
	float outVolts;	
	bool isRunning = false;
	float inputPitches[5];
	bool debug = false;;
	int cycleLength = 0;
	bool newCycle = false;
	int stepI = 0;
	int cycleI = 0;
	int stepsRemaining;
	int cycleRemaining;
	int *direction;
	int nStep;
	int nDist;
	int stepX = 0;
	int poll = 5000;
	bool wasTriggered = false;

};

ArpeggiatorWidget::ArpeggiatorWidget() {
	Arpeggiator *module = new Arpeggiator();
	
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

	addOutput(createOutput<PJ301MPort>(Vec(11.5, 49), module, Arpeggiator::OUT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(59.5, 49), module, Arpeggiator::GATE_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(155.5, 49), module, Arpeggiator::EOC_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(203.5, 49), module, Arpeggiator::EOS_OUTPUT));

	addInput(createInput<PJ301MPort>(Vec(11.5, 329),  module, Arpeggiator::PITCH_INPUT));
	addInput(createInput<PJ301MPort>(Vec(59.5, 329),  module, Arpeggiator::PITCH_INPUT + 1));
	addInput(createInput<PJ301MPort>(Vec(107.5, 329), module, Arpeggiator::PITCH_INPUT + 2));
	addInput(createInput<PJ301MPort>(Vec(155.5, 329), module, Arpeggiator::PITCH_INPUT + 3));
	addInput(createInput<PJ301MPort>(Vec(203.5, 329), module, Arpeggiator::PITCH_INPUT + 4));
	
	addInput(createInput<PJ301MPort>(Vec(11.5, 269), module, Arpeggiator::STEP_INPUT));
	addInput(createInput<PJ301MPort>(Vec(59.5, 269), module, Arpeggiator::DIST_INPUT));
	addParam(createParam<BefacoSwitch>(Vec(106, 266), module, Arpeggiator::DIR_PARAM, 0, 1, 0));
	addInput(createInput<PJ301MPort>(Vec(155.5, 269), module, Arpeggiator::TRIG_INPUT));
	addInput(createInput<PJ301MPort>(Vec(203.5, 269), module, Arpeggiator::CLOCK_INPUT));
	
    addParam(createParam<LEDButton>(Vec(60, 101-1), module, Arpeggiator::LOCK_PARAM, 0.0, 1.0, 0.0));
    addChild(createLight<MediumLight<GreenLight>>(Vec(64.4, 104.4), module, Arpeggiator::LOCK_LIGHT));
	

}

struct Sequence {
	virtual void build(float basePitch, int nSteps, int dist);
	virtual float step();
};

struct UpSequence : Sequence {
	void build(float basePitch, int nSteps, int dist) {
		return;
	}
	float step() {
		return 0.0;
	}
};

void Arpeggiator::step() {
	
	bool isClocked = false;
	bool isTriggered = false;
	bool advance = false;
	float semiTone = 1.0 / 12.0;
	stepX++;
	
	// std::cout << "Step: " << stepX << std::endl;

	if (lockTrigger.process(params[LOCK_PARAM].value)) {
		locked = !locked;
		std::cout << "Toggling lock: " << locked << std::endl;
	}
    lights[LOCK_LIGHT].value = locked ? 1.0 : 0.0;

	// Clock running sequence
	if (inputs[CLOCK_INPUT].active) {
		// External clock
		if (clockTrigger.process(inputs[CLOCK_INPUT].value)) {
			isClocked = true;
		} 
	}
	
	// OK so the problem here might be that the clock gate is still high right after the trigger gate fired on the previous step
	// So we need to wait a while for the clock gate to go low
	// Eat the clock gate for precisely one step after a gate
	if (wasTriggered) {
		isClocked = false;
		wasTriggered = false;
	}
	
	// Trigger a new sequence
	if (inputs[TRIG_INPUT].active) {
		// External clock
		if (trigTrigger.process(inputs[TRIG_INPUT].value)) {
			isTriggered = true;
		} 
	}
	
	int dir = params[DIR_PARAM].value;
		
	// Check if we have been triggered 
	if (isTriggered) {
		
		// start a new sequence
		// Read distance and steps, which will be constant for this sequence
		nStep = inputs[STEP_INPUT].value;
		nStep = clampi(nStep, 0, MAX_STEPS);
		
		nDist = inputs[DIST_INPUT].value;
		nDist = clampi(nDist, 0, 10);
		
		// Read the pattern
		// Calculate the subsequent pitches, need direction, number of steps and step size
		switch (dir) {
			case 1:			direction = PATT_UP; break;
			case 0:			direction = PATT_DN; break;
			default: 		direction = PATT_UP;
		}
		
		// Set the cycle
		isRunning = true;
		newCycle = true;
		stepI = 0;
		cycleI = 0;
		stepsRemaining = nStep;
		// Oops, how to do reset cycleRemaining
		
		// Set flag to advance sequence
		advance = true;

		if (false) {
			std::cout << "Triggered Steps: " << nStep << " Dist: " << nDist << " Pattern: " << dir << std::endl;
			std::cout << "Advance from Trigger" << std::endl;
		}
		
		// Need this to stop clock gates from immediately advancing sequence. Probably should be a pulse
		wasTriggered = true;
		
	} 
	
	// Set the pitches
	// if there is a new cycle and the pitches are unlocked or we have been triggered
	
	bool getPitches = false;
	
	if (isTriggered) {
		if (debug) {
			std::cout << "Read pitches from trigger: " << isTriggered << std::endl;
		}
		getPitches = true;
	}
	
	if (isClocked && isRunning && newCycle && !locked) {
		if (debug) {
			std::cout << "Read pitches from clock: " << isClocked << isRunning << newCycle << !locked << std::endl;
		}
		getPitches = true;		
	}

	if (getPitches) {
		
		cycleLength = 0;
		
		// Read out voltages
		for (int p = 0; p < NUM_PITCHES; p++) {
			int index = PITCH_INPUT + p;
			if (inputs[index].active) {
				inputPitches[cycleLength] = inputs[index].value;
				cycleLength++;
			} 
		}
				
		cycleRemaining = cycleLength;
		if (debug) {
			std::cout << "Defining cycle: seqLen: " << nStep * cycleLength << " cycLength: " << cycleLength;
			for (int i = 0; i < cycleLength; i++) {
				 std::cout << " P" << i << " V: " << inputPitches[i];
			}
			std::cout << std::endl;
		}
		
	} 
			
	// Are we running a sequence and are clocked
	if (isRunning && isClocked) {
		// Set flag to advance sequence
		if (debug) { std::cout << "Advance from Clock" << std::endl; }
		advance = true;
	}

	if (advance) {

		if (debug) { std::cout << "Advance S: " << stepI <<
			" C: " << cycleI <<
			" sRemain: " << stepsRemaining <<
			" cRemain: " << cycleRemaining << std::endl;
		}
	
		if (newCycle) {

			if (debug) { std::cout << "New Cycle: " << newCycle << std::endl; }
			
			for (int i = 0; i < cycleLength; i++) {
				
				float V;
				// Read the pitches according to direction
				if (dir) { // Up
					V = inputPitches[i];
				} else { // Down 
					V = inputPitches[cycleLength - i - 1];
				}	
				
				float dV = semiTone * nDist * direction[stepI];
				pitches[i] = clampf(V + dV, -10.0, 10.0);
				
				if (debug) {
					std::cout << i << " stepI: " << 
						" dV:" << dV <<
						" in: " << inputPitches[i] <<
						" out: " << pitches[i] << std::endl;
				}				
			}
			
			if (debug) {
				std::cout << "Output pitches: ";
				for (int i = 0; i < cycleLength; i++) {
					 std::cout << " P" << i << " V: " << pitches[i];
				}
				std::cout << std::endl;
			}
			
			newCycle = false;
			
		}
		
		outVolts = pitches[cycleI];
		if (debug) { std::cout << "V = " << outVolts << std::endl; }
		
		cycleI++;
		cycleRemaining--;
		
		gatePulse.trigger(5e-3);
		
		if (cycleRemaining == 0) {
			stepI++;
			stepsRemaining--;
			cycleRemaining = cycleLength;
			cycleI = 0;
			newCycle = true;
			eocPulse.trigger(5e-3);
			if (debug) { 
				std::cout << "Cycle Finished S: " << stepI <<
				" C: " << cycleI <<
				" sRemain: " << stepsRemaining <<
				" cRemain: " << cycleRemaining << 
				" flag:" << newCycle <<	std::endl;
			}
		}
		
		if (stepsRemaining == 0) {
			if (debug) { 
				std::cout << "Sequence Finished S: " << stepI <<
				" C: " << cycleI <<
				" sRemain: " << stepsRemaining <<
				" cRemain: " << cycleRemaining << 
				" flag:" << isRunning << std::endl;
			}
			isRunning = false;
			eosPulse.trigger(5e-3);
		}
		
	}	
	
	float delta = 1.0 / engineGetSampleRate();

	bool sPulse = eosPulse.process(delta);
	bool cPulse = eocPulse.process(delta);
	bool gPulse = gatePulse.process(delta);

	// Set the value
	outputs[OUT_OUTPUT].value = outVolts;
	// if (true) {
	// 	std::cout << "OUT: " << outVolts << " " << gPulse<< std::endl;
	// }
	outputs[GATE_OUTPUT].value = gPulse ? 10.0 : 0.0;
	outputs[EOS_OUTPUT].value = sPulse ? 10.0 : 0.0;
	outputs[EOC_OUTPUT].value = cPulse ? 10.0 : 0.0;
	
	
}
	

