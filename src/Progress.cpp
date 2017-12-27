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

struct AHKnob : RoundKnob {
	AHKnob() {
		snap = true;
		setSVG(SVG::load(assetPlugin(plugin,"res/ComponentLibrary/AHKnob.svg")));
	}
};

struct Progress : Module {

	const static int NUM_PITCHES = 6;
	
	enum SlotStatus {
		ROOT,
		FIRST_INV,
		SECOND_INV,
		NUM_STATUS
	};
	
	std::string status[5] {
		"Root",
		"1st Inv",
		"2nd Inv"
	};
	
	
	enum ParamIds {
		CLOCK_PARAM,
		RUN_PARAM,
		RESET_PARAM,
		STEPS_PARAM,
		KEY_PARAM, // JH
		MODE_PARAM, // JH
		ROOT_PARAM,
		CHORD_PARAM = ROOT_PARAM + 8,
		INV_PARAM   = CHORD_PARAM + 8,  
		GATE_PARAM  = INV_PARAM + 8,
		NUM_PARAMS  = GATE_PARAM + 8
	};
	enum InputIds {
		KEY_INPUT,
		MODE_INPUT,
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATES_OUTPUT,
		PITCH_OUTPUT,
		GATE_OUTPUT = PITCH_OUTPUT + 6,
		NUM_OUTPUTS = GATE_OUTPUT + 8
	};
	enum LightIds {
		RUNNING_LIGHT,
		RESET_LIGHT,
		GATES_LIGHT,
		GATE_LIGHTS,
		NUM_LIGHTS = GATE_LIGHTS + 8
	};
		
	Progress() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	
	Quantizer q;
	Chord chords;
	
	bool running = true;
	SchmittTrigger clockTrigger; // for external clock
	// For buttons
	SchmittTrigger runningTrigger;
	SchmittTrigger resetTrigger;
	SchmittTrigger gateTriggers[8];
	float phase = 0.0;
	int index = 0;
	bool gateState[8] = {};
	float resetLight = 0.0;
	float stepLights[8] = {};

	enum GateMode {
		TRIGGER,
		RETRIGGER,
		CONTINUOUS,
	};
	GateMode gateMode = TRIGGER;
	PulseGenerator gatePulse;
		
	int inMode = 0;
	bool haveMode = false;
	
	int inRoot = 0;
	bool haveRoot = false;
	
	int offset = 24; // Repeated notes in chord and expressed in the chord definition as being transposed 2 octaves lower. When played this offset needs to be removed (or the notes removed, or the notes transposed to an octave higher)
	
	int stepX = 0;
	int poll = 5000;
	
	bool gate = true;		
	
	float prevChrInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};
	float prevInvInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};
	float prevRootInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};

	float currChrInput[8];
	float currInvInput[8];
	float currRootInput[8];
	
	int currRoot[8];
	int currChord[8];
	int currInv[8];	
	
	float pitches[8][6];
		
	inline bool debug() {
		return true;
	}
	
};



void Progress::step() {
	
	stepX++;
	
	const float lightLambda = 0.075;
	
	float sampleRate = engineGetSampleRate();
	// Run
	if (runningTrigger.process(params[RUN_PARAM].value)) {
		running = !running;
	}
	lights[RUNNING_LIGHT].value = running ? 1.0 : 0.0;

	bool nextStep = false;

	if (running) {
		if (inputs[EXT_CLOCK_INPUT].active) {
			// External clock
			if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].value)) {
				phase = 0.0;
				nextStep = true;
			}
		}
		else {
			// Internal clock
			float clockTime = powf(2.0, params[CLOCK_PARAM].value + inputs[CLOCK_INPUT].value);
			phase += clockTime / sampleRate;
			if (phase >= 1.0) {
				phase -= 1.0;
				nextStep = true;
			}
		}
	}

	// Reset
	if (resetTrigger.process(params[RESET_PARAM].value + inputs[RESET_INPUT].value)) {
		phase = 0.0;
		index = 8;
		nextStep = true;
		resetLight = 1.0;
	}

	if (nextStep) {
		// Advance step
		int numSteps = clampi(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1, 8);
		index += 1;
		if (index >= numSteps) {
			index = 0;
		}
		stepLights[index] = 1.0;
		gatePulse.trigger(1e-3);
	}

	resetLight -= resetLight / lightLambda / sampleRate;

	bool pulse = gatePulse.process(1.0 / sampleRate);

	// Gate buttons
	for (int i = 0; i < 8; i++) {
		if (gateTriggers[i].process(params[GATE_PARAM + i].value)) {
			gateState[i] = !gateState[i];
		}
		bool gateOn = (running && i == index && gateState[i]);
		if (gateMode == TRIGGER)
			gateOn = gateOn && pulse;
		else if (gateMode == RETRIGGER)
			gateOn = gateOn && !pulse;

		outputs[GATE_OUTPUT + i].value = gateOn ? 10.0 : 0.0;
		stepLights[i] -= stepLights[i] / lightLambda / sampleRate;
		lights[GATE_LIGHTS + i].value = gateState[i] ? 1.0 - stepLights[i] : stepLights[i];
	}

	// index is our current step
	if (inputs[KEY_INPUT].active) {
		haveRoot = true;
		float fRoot = inputs[KEY_INPUT].value;
		inRoot = q.getKeyFromVolts(fRoot);
	} else {
		haveRoot = false;
		inRoot = params[KEY_PARAM].value;
	}

	if (inputs[MODE_INPUT].active) {
		haveMode = true;
		float fMode = inputs[MODE_INPUT].value;
		inMode = q.getModeFromVolts(fMode);
		
	} else {
		haveMode = false;
		inMode = params[MODE_PARAM].value;
	}

	// Read inputs
	for (int step = 0; step < 8; step++) {
		currChrInput[step]  = params[CHORD_PARAM + step].value;;
		currRootInput[step] = params[ROOT_PARAM + step].value;
		currInvInput[step]  = params[INV_PARAM + step].value;
	}
	
	// Check for changes, try to minimize how mauch p
	for (int step = 0; step < 8; step++) {
		
		bool update = false;
		
		// If anything has changed, recalculate output for that step
		if (prevRootInput[step] != currRootInput[step]) {
			prevRootInput[step] = currRootInput[step];
			currRoot[step] = round(rescalef(fabs(currRootInput[step]), 0.0, 10.0, 0, Quantizer::NUM_NOTES - 1)); // Param range is 0 to 10, mapped to 0 to 11
			update = true;
		}
		
		if (prevChrInput[step] != currChrInput[step]) {
			prevChrInput[step]  = currChrInput[step]; 
			currChord[step] = round(rescalef(fabs(currChrInput[step]), 0.0, 10.0, 1, 98)); // Param range is 0 to 10		
			update = true;
		}
		
		if (prevInvInput[step] != currInvInput[step]) {
			prevInvInput[step]  = currInvInput[step];		
			currInv[step] = currInvInput[step];
			update = true;
		}

		if (update) {
			
			if (debug()) { std::cout << stepX << ":" << index << " Input: Root: " << currRoot[index] << " Chord: " << currChord[index] << " Inversion: "
		 		<< currInv[step] << std::endl; }
		
			int *chordArray;
	
			switch(currInv[step]) {
				case ROOT:  		chordArray = chords.Chords[currChord[step]].root; 		break;
				case FIRST_INV:  	chordArray = chords.Chords[currChord[step]].first; 	break;
				case SECOND_INV:  	chordArray = chords.Chords[currChord[step]].second; 	break;
				default: chordArray = chords.Chords[currChord[step]].root;
			}
			
			if (debug()) { std::cout << stepX  << " Step: " << index << " Output: Chord: "
					<< q.noteNames[currRoot[step]] << chords.Chords[currChord[step]].quality << " (" << status[currInv[step]] << ") " << std::endl; }
	 	
			for (int j = 0; j < NUM_PITCHES; j++) {
	
				if (chordArray[j] < 0) {
					pitches[step][j] = q.getVoltsFromPitch(chordArray[j] + offset,currRoot[step]);			
				} else {
					pitches[step][j] = q.getVoltsFromPitch(chordArray[j],currRoot[step]);			
				}
			
			}
		}
				
	}
	
	for (int i = 0; i < NUM_PITCHES; i++) {
		outputs[PITCH_OUTPUT + i].value = pitches[index][i];
	}
	
	bool gatesOn = (running && gateState[index]);
	if (gateMode == TRIGGER)
		gatesOn = gatesOn && pulse;
	else if (gateMode == RETRIGGER)
		gatesOn = gatesOn && !pulse;

	// Outputs
	outputs[GATES_OUTPUT].value = gatesOn ? 10.0 : 0.0;
	lights[RESET_LIGHT].value = resetLight;
	lights[GATES_LIGHT].value = gatesOn ? 1.0 : 0.0;	

}

ProgressWidget::ProgressWidget() {
	Progress *module = new Progress();
		
	setModule(module);
	box.size = Vec(15*22, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Progress2.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

	addParam(createParam<AHKnob>(Vec(18, 56), module, Progress::CLOCK_PARAM, -2.0, 6.0, 2.0));
	addParam(createParam<AHButton>(Vec(60, 61-1), module, Progress::RUN_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(Vec(64.4, 64.4), module, Progress::RUNNING_LIGHT));
	addParam(createParam<AHButton>(Vec(99, 61-1), module, Progress::RESET_PARAM, 0.0, 1.0, 0.0));
	addChild(createLight<MediumLight<GreenLight>>(Vec(103.4, 64.4), module, Progress::RESET_LIGHT));
	addParam(createParam<AHKnob>(Vec(132, 56), module, Progress::STEPS_PARAM, 1.0, 8.0, 8.0));
	addChild(createLight<MediumLight<GreenLight>>(Vec(179.4, 64.4), module, Progress::GATES_LIGHT));

	static const float portX[8] = {20, 58, 96, 135, 173, 212, 250, 289};
	addInput(createInput<PJ301MPort>(Vec(portX[0]-1, 98), module, Progress::CLOCK_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[1]-1, 98), module, Progress::EXT_CLOCK_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[2]-1, 98), module, Progress::RESET_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[3]-1, 98), module, Progress::STEPS_INPUT));
	addOutput(createOutput<PJ301MPort>(Vec(portX[4]-1, 98), module, Progress::GATES_OUTPUT));
		
	float xStart = 5.0;
	float boxWidth = 35.0;
	float gap = 10.0;
	float connDelta = 5.0;
	
	for (int i = 0; i < Progress::NUM_PITCHES; i++) {
		float xPos = xStart + ((float)i * boxWidth + gap);
		addOutput(createOutput<PJ301MPort>(Vec(xPos + connDelta, 19.0),  module, Progress::PITCH_OUTPUT + i));
	}	

	for (int i = 0; i < 8; i++) {
		addParam(createParam<AHKnob>(Vec(portX[i]-2, 157), module, Progress::ROOT_PARAM + i, 0.0, 10.0, 0.0));
		addParam(createParam<AHKnob>(Vec(portX[i]-2, 198), module, Progress::CHORD_PARAM + i, 0.0, 10.0, 0.0));
		addParam(createParam<AHKnob>(Vec(portX[i]-2, 240), module, Progress::INV_PARAM + i, 0.0, 2.0, 0.0));
		addParam(createParam<AHButton>(Vec(portX[i]+2, 278-1), module, Progress::GATE_PARAM + i, 0.0, 1.0, 0.0));
		addChild(createLight<MediumLight<GreenLight>>(Vec(portX[i]+6.4, 281.4), module, Progress::GATE_LIGHTS + i));
		addOutput(createOutput<PJ301MPort>(Vec(portX[i]-1, 307), module, Progress::GATE_OUTPUT + i));
	}
}

struct ProgressGateModeItem : MenuItem {
	Progress *progress;
	Progress::GateMode gateMode;
	void onAction(EventAction &e) override {
		progress->gateMode = gateMode;
	}
	void step() override {
		rightText = (progress->gateMode == gateMode) ? "✔" : "";
	}
};

Menu *ProgressWidget::createContextMenu() {
	Menu *menu = ModuleWidget::createContextMenu();

	MenuLabel *spacerLabel = new MenuLabel();
	menu->pushChild(spacerLabel);

	Progress *progress = dynamic_cast<Progress*>(module);
	assert(progress);

	MenuLabel *modeLabel = new MenuLabel();
	modeLabel->text = "Gate Mode";
	menu->pushChild(modeLabel);

	ProgressGateModeItem *triggerItem = new ProgressGateModeItem();
	triggerItem->text = "Trigger";
	triggerItem->progress = progress;
	triggerItem->gateMode = Progress::TRIGGER;
	menu->pushChild(triggerItem);

	ProgressGateModeItem *retriggerItem = new ProgressGateModeItem();
	retriggerItem->text = "Retrigger";
	retriggerItem->progress = progress;
	retriggerItem->gateMode = Progress::RETRIGGER;
	menu->pushChild(retriggerItem);

	ProgressGateModeItem *continuousItem = new ProgressGateModeItem();
	continuousItem->text = "Continuous";
	continuousItem->progress = progress;
	continuousItem->gateMode = Progress::CONTINUOUS;
	menu->pushChild(continuousItem);

	return menu;
}



