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
		IN_INPUT,
		CLOCK_INPUT,
		STEP_INPUT,
		DIST_INPUT,
		TRIG_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		GATE_OUTPUT,
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
	
	SchmittTrigger clockTrigger; // for clock
	SchmittTrigger trigTrigger;  // for step trigger
	PulseGenerator gatePulse;
	PulseGenerator eosPulse;
		
	float pitches[16];
	int currStep;
	int stepsRemaining;
	float outVolts;

};

void Arpeggiator::step() {

	bool isClocked = false;
	bool isTriggered = false;
	bool runSeq = false;
	bool isFinished = false;
	
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
	float inVolts = inputs[IN_INPUT].value;
	int dir = params[DIR_PARAM].value;
	
	// If triggered 
	if (isTriggered) {
	
		int nStep = inputs[STEP_INPUT].value;
		nStep = clampi(nStep, 0, MAX_STEPS);

		int nDist = inputs[DIST_INPUT].value;
		nDist = clampi(nDist, 0, 10);

		float semiTone = 1.0 / 12.0;

		// std::cout << "Steps: " << nStep << " Dist:" << nDist << std::endl;

		// Calculate the subsequent pitches, need direction, number of steps and step size
		int *direction;
		switch (dir){
			case 1:			direction = PATT_UP; break;
			case 0:			direction = PATT_DN; break;
			default: 		direction = PATT_UP;
		}
		
		// Include current pitch
		// std::cout << "V=" << inVolts << " ";
		for (int s = 0; s < nStep; s++) {
			float v = semiTone * nDist * direction[s];
			pitches[s] = clampf(inVolts + v, -10.0, 10.0);
			// std::cout << s << "=" <<  pitches[s] << " ";
		}
		//  std::cout << std::endl;
		
		isRunning = true;
		currStep = 0;
		stepsRemaining = nStep;
		runSeq = true;

	} else {
		// If running a sequence and is clockec, step sequence and emit gate
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
		gatePulse.trigger(1e-3);
		if (stepsRemaining == 0) {
			isRunning = false;
			isFinished = true;
			eosPulse.trigger(1e-3);
		}
	}

	bool gPulse = gatePulse.process(1.0 / engineGetSampleRate());
	bool ePulse = eosPulse.process(1.0 / engineGetSampleRate());

	// Set the value
	outputs[OUT_OUTPUT].value = outVolts;
	outputs[GATE_OUTPUT].value = gPulse ? 10.0 : 0.0;
	outputs[EOS_OUTPUT].value = ePulse ? 10.0 : 0.0;

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

	addInput(createInput<PJ301MPort>(Vec(18, 329), module, Arpeggiator::IN_INPUT));
	addInput(createInput<PJ301MPort>(Vec(78, 329), module, Arpeggiator::CLOCK_INPUT));
	addOutput(createOutput<PJ301MPort>(Vec(138, 329), module, Arpeggiator::GATE_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(198, 329), module, Arpeggiator::OUT_OUTPUT));

	addInput(createInput<PJ301MPort>(Vec(18, 269), module, Arpeggiator::STEP_INPUT));
	addInput(createInput<PJ301MPort>(Vec(78, 269), module, Arpeggiator::DIST_INPUT));
	addInput(createInput<PJ301MPort>(Vec(138, 269), module, Arpeggiator::TRIG_INPUT));
	addOutput(createOutput<PJ301MPort>(Vec(198, 269), module, Arpeggiator::EOS_OUTPUT));
	
	addParam(createParam<BefacoSwitch>(Vec(120, 50), module, Arpeggiator::DIR_PARAM, 0, 1, 0));

}
