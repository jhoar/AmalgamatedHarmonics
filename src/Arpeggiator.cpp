#include "AH.hpp"
#include "Core.hpp"
#include "components.hpp"
#include "dsp/digital.hpp"

#include <iostream>

struct AHButton : SVGSwitch, MomentarySwitch {
	AHButton() {
		addFrame(SVG::load(assetPlugin(plugin,"res/ComponentLibrary/AHButton.svg")));
	}
};

struct Arpeggiator : Module {

	const static int MAX_STEPS = 16;
	const static int MAX_DIST = 12; //Octave
	const static int NUM_PITCHES = 6;

	enum ParamIds {
		PDIR_PARAM,
		SDIR_PARAM,
		LOCK_PARAM,
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
	
	int PATT_UP[17] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}; 
	int PATT_DN[17] = {0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15,-16}; 

	Arpeggiator() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	
		
	SchmittTrigger clockTrigger; // for clock
	SchmittTrigger trigTrigger;  // for step trigger
    SchmittTrigger lockTrigger;
	
	PulseGenerator gatePulse;
	PulseGenerator eosPulse;
	PulseGenerator eocPulse;

	float pitches[NUM_PITCHES];
	float inputPitches[NUM_PITCHES];

	int inputPDir;
	int pDir = 0;
	
	int inputSDir;
	int sDir = 0;
	int *sDirection; // FIXME eventually remove this

	int inputStep = 0;
	int nStep = 0;
	
	int inputDist = 0;
	int nDist = 0;
	
	bool locked = false;

	float outVolts;
	bool isRunning = false;
	bool wasTriggered = false;
	bool newCycle = false;

	int cycleLength = 0;
	int stepI = 0;
	int cycleI = 0;
	int stepsRemaining;
	int cycleRemaining;

	bool debug = false;
	int stepX = 0;
	int poll = 5000;

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
		if (debug) { std::cout << "Toggling lock: " << locked << std::endl; }
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
	
	// Read sequence config every step for UI
	inputPDir = params[PDIR_PARAM].value;
	inputSDir = params[SDIR_PARAM].value;
	
	inputStep = round(rescalef(inputs[STEP_INPUT].value, -10, 10, 0, MAX_STEPS));		
	inputDist = round(rescalef(inputs[DIST_INPUT].value, -10, 10, 0, MAX_DIST));		
	
	// Check if we have been triggered 
	if (isTriggered) {
		
		// Set the cycle
		isRunning = true;
		newCycle = true;
		stepI = 0;
		cycleI = 0;
	
		// Set flag to advance sequence
		advance = true;
	
		if (debug) {
			std::cout << "Triggered Steps: " << nStep << " Dist: " << nDist << " pitchScan: " << pDir << " Pattern: " << sDir << std::endl;
			std::cout << "Advance from Trigger" << std::endl;
		}
	
		// Need this to stop clock gates from immediately advancing sequence. Probably should be a pulse
		wasTriggered = true;
		
	} 
	
	// Set the pitches
	// if there is a new cycle and the pitches are unlocked or we have been triggered
	bool readCycleSettings = false;
	
	if (isTriggered && !locked) {
		if (debug) {
			std::cout << "Read pitches from trigger: " << isTriggered << std::endl;
		}
		readCycleSettings = true;
	}
	
	if (isClocked && isRunning && newCycle && !locked) {
		if (debug) {
			std::cout << "Read pitches from clock: " << isClocked << isRunning << newCycle << !locked << std::endl;
		}
		readCycleSettings = true;		
	}

	if (readCycleSettings) {
		
		// Freeze sequence params
		nStep = inputStep;
		nDist = inputDist;
		pDir = inputPDir;
		sDir = inputSDir;
	
		// Deal with RND setting // FIXME this will change as random will work differently
		if (sDir == 1) {
			if (rand() % 2 == 0) {
				sDir = 0;
			} else {
				sDir = 2;
			}
		}
			
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
	
	if (nStep == 0) {
		return; // No steps, abort
	} else {
		// If we had reachd the end of the sequence or just transitioned from 0 step length, reset the counter
		if (stepsRemaining == 0) { 
			stepsRemaining = nStep;
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
		
			// Read the pattern
			// Calculate the subsequent pitches, need direction, number of steps and step size
			switch (sDir) {
				case 0:			sDirection = PATT_DN; break;
				case 1:			// This might happen on reset, if there is not valid input
				case 2:			sDirection = PATT_UP; break;
				default: 		sDirection = PATT_UP;
			}

			for (int i = 0; i < cycleLength; i++) {
				
				int target;
				// Read the pitches according to direction, but we should do this for the sequence?
				switch (pDir) {
					case 0: target = cycleLength - i - 1; break; 	// DOWN
					case 2: target = i; break;						// UP
					default: target = i; break; // For random case, read randomly from array, so order does not matter
				}

				float dV = semiTone * nDist * sDirection[stepI];
				pitches[i] = clampf(inputPitches[target] + dV, -10.0, 10.0);
				
				if (debug) {
					std::cout << i << " stepI: " << 
						" dV:" << dV <<
						" in: " << inputPitches[target] <<
						" out: " << pitches[target] << std::endl;
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
		
		if (pDir == 1) {
			outVolts = pitches[rand() % cycleLength];
		} else {
			outVolts = pitches[cycleI];
		}
		
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
		nvgText(vg, pos.x + 10, pos.y + 20, text, NULL);
		snprintf(text, sizeof(text), "DIST: %d [%d]", module->nDist, module->inputDist);
		nvgText(vg, pos.x + 10, pos.y + 40, text, NULL);
		
		if (module->sDir == 0) {
			snprintf(text, sizeof(text), "SEQ: DSC");			
		} else {
			snprintf(text, sizeof(text), "SEQ: ASC");			
		}
		nvgText(vg, pos.x + 10, pos.y + 60, text, NULL);
		
		switch(module->pDir) {
			case 0: snprintf(text, sizeof(text), "ARP: R-L"); break;
			case 1: snprintf(text, sizeof(text), "ARP: RND"); break;
			case 2: snprintf(text, sizeof(text), "ARP: L-R"); break;
			default: snprintf(text, sizeof(text), "ARP: ERR"); break;
		}
		
		nvgText(vg, pos.x + 10, pos.y + 80, text, NULL);
		
	}
	
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

	{
		ArpeggiatorDisplay *display = new ArpeggiatorDisplay();
		display->module = module;
		display->box.pos = Vec(20, 95);
		display->box.size = Vec(100, 140);
		addChild(display);
	}

	addOutput(createOutput<PJ301MPort>(Vec(6.5 + 5.0, 33.0 + 16.0),  module, Arpeggiator::OUT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(54.5 + 5.0, 33.0 + 16.0),  module, Arpeggiator::GATE_OUTPUT));
    addParam(createParam<AHButton>(Vec(102.5 + 8.0, 33.0 + 19.0), module, Arpeggiator::LOCK_PARAM, 0.0, 1.0, 0.0));
    addChild(createLight<MediumLight<GreenLight>>(Vec(102.5 + 12.4, 33.0 + 23.4), module, Arpeggiator::LOCK_LIGHT));
	addOutput(createOutput<PJ301MPort>(Vec(150.5 + 5.0, 33.0 + 16.0), module, Arpeggiator::EOC_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(198.5 + 5.0, 33.0 + 16.0), module, Arpeggiator::EOS_OUTPUT));

	float xStart = 5.0;
	float boxWidth = 35.0;
	float gap = 10.0;
	float connDelta = 5.0;
	
	for (int i = 0; i < Arpeggiator::NUM_PITCHES; i++) {
		float xPos = xStart + ((float)i * boxWidth + gap);
		addInput(createInput<PJ301MPort>(Vec(xPos + connDelta, 329),  module, Arpeggiator::PITCH_INPUT + i));
	}
	
	addInput(createInput<PJ301MPort>(Vec(15.0  + connDelta, 269), module, Arpeggiator::STEP_INPUT));
	addInput(createInput<PJ301MPort>(Vec(50.0 + connDelta, 269), module, Arpeggiator::DIST_INPUT));
	addInput(createInput<PJ301MPort>(Vec(155 + connDelta, 269), module, Arpeggiator::TRIG_INPUT));
	addInput(createInput<PJ301MPort>(Vec(190 + connDelta, 269), module, Arpeggiator::CLOCK_INPUT));
	
	addParam(createParam<BefacoSwitch>(Vec(175.0 + 3.5, 95 + 17), module, Arpeggiator::SDIR_PARAM, 0, 2, 0));
	addParam(createParam<BefacoSwitch>(Vec(175.0 + 3.5, 170 + 17), module, Arpeggiator::PDIR_PARAM, 0, 2, 0));

}



