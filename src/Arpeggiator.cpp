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
		NUM_LIGHTS
	};
	
	int PATT_UP[17] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}; 
	int PATT_DN[17] = {0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15,-16}; 

	Arpeggiator() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	
	bool isRunning = false;
	int MAX_STEPS = 16;
	int NUM_PITCHES = 5;
		
	SchmittTrigger clockTrigger; // for clock
	SchmittTrigger trigTrigger;  // for step trigger
	PulseGenerator gatePulse;
	PulseGenerator eosPulse;
	PulseGenerator eocPulse;
		
	float inputPitches[5];
	float pitches[16 * 5];
	int currStep;
	int stepsRemaining;
	int cycleRemaining;
	float outVolts;

};

void Arpeggiator::step() {

	bool isClocked = false;
	bool isTriggered = false;
	bool runSeq = false;
	int activePitches = 0;
	
	
	// Clock running sequence
	if (inputs[CLOCK_INPUT].active) {
		// External clock
		if (clockTrigger.process(inputs[CLOCK_INPUT].value)) {
			isClocked = true;
		} 
	}
	
	// Trigger a new sequence
	if (inputs[TRIG_INPUT].active) {
		// External clock
		if (trigTrigger.process(inputs[TRIG_INPUT].value)) {
			isTriggered = true;
		} 
	}
	
	// Get the input pitch
	for (int p = 0; p < NUM_PITCHES; p++) {
		if (inputs[PITCH_INPUT + p].active) {
			inputPitches[activePitches] = inputs[PITCH_INPUT + p].value;
			activePitches++;
		} 
	}
	
	int dir = params[DIR_PARAM].value;
	
	// If triggered 
	if (isTriggered && activePitches) {
	
		int nStep = inputs[STEP_INPUT].value;
		nStep = clampi(nStep, 0, MAX_STEPS);

		int nDist = inputs[DIST_INPUT].value;
		nDist = clampi(nDist, 0, 10);

		float semiTone = 1.0 / 12.0;

		// std::cout << "Steps: " << nStep << " Dist:" << nDist << " ActivePitches " << activePitches << std::endl;

		// Calculate the subsequent pitches, need direction, number of steps and step size
		int *direction;
		switch (dir){
			case 1:			direction = PATT_UP; break;
			case 0:			direction = PATT_DN; break;
			default: 		direction = PATT_UP;
		}
		
		// Include current pitch
		for (int s = 0; s < nStep; s++) {
			for (int p = 0; p < activePitches; p++) {
				int seqNote = s * activePitches + p;
				float v = semiTone * nDist * direction[s];
				pitches[seqNote] = clampf(inputPitches[p] + v, -10.0, 10.0);
				// std::cout << seqNote << "=" <<  pitches[seqNote] << " ";
			}
		}
		// std::cout << std::endl;
		
		isRunning = true;
		currStep = 0;
		stepsRemaining = nStep * activePitches;
		cycleRemaining = activePitches;
		runSeq = true;

	} else {
		// If running a sequence and is clocked, step sequence and emit gate
		if (isRunning && isClocked) {
			runSeq = true;
		} else {
			// Do nothing
		}
	}
	
	if (runSeq) {	
		outVolts = pitches[currStep];
		currStep++;
		stepsRemaining--;
		cycleRemaining--;
		gatePulse.trigger(1e-3);
		if (cycleRemaining == 0) {
			cycleRemaining = activePitches;
			eocPulse.trigger(1e-3);
		}
		if (stepsRemaining == 0) {
			isRunning = false;
			eosPulse.trigger(1e-3);
		}
	} 
	
	float delta = 1.0 / engineGetSampleRate();

	bool gPulse = gatePulse.process(delta);
	bool sPulse = eosPulse.process(delta);
	bool cPulse = eocPulse.process(delta);

	// Set the value
	outputs[OUT_OUTPUT].value = outVolts;
	outputs[GATE_OUTPUT].value = gPulse ? 10.0 : 0.0;
	outputs[EOS_OUTPUT].value = sPulse ? 10.0 : 0.0;
	outputs[EOC_OUTPUT].value = cPulse ? 10.0 : 0.0;

}

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
