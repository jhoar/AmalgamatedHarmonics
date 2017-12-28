#include "AH.hpp"
#include "Core.hpp"
#include "components.hpp"
#include "dsp/digital.hpp"

#include <iostream>

// TODO
// - Mode mode: chord namer
// UI

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

struct AHKnobNoSnap : RoundKnob {
	AHKnobNoSnap() {
		snap = false;
		setSVG(SVG::load(assetPlugin(plugin,"res/ComponentLibrary/AHKnob.svg")));
	}
};


struct Progress : Module {

	const static int NUM_PITCHES = 6;
	
	enum Inversion {
		ROOT,
		FIRST_INV,
		SECOND_INV,
		NUM_INV
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
		
	bool modeMode = false;
	bool prevModeMode = false;
	
	int offset = 24; // Repeated notes in chord and expressed in the chord definition as being transposed 2 octaves lower. When played this offset needs to be removed (or the notes removed, or the notes transposed to an octave higher)
	
	int stepX = 0;
	int poll = 50000;
	
	bool gate = true;		
	
	float prevRootInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};
	float prevChrInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};

	float prevTonicInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};
	float prevQualityInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};

	float prevInvInput[8] = {-100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0, -100.0};

	float currRootInput[8];
	float currChrInput[8];

	float currTonicInput[8];
	float currQualityInput[8];

	float currInvInput[8];
	
	int currMode;
	int currKey;
	int prevMode = -1;
	int prevKey = -1;
	
	int currRoot[8];
	int currChord[8];
	int currInv[8];	

	int currTonic[8];
	int currQuality[8];
	
	float pitches[8][6];
		
	inline bool debug() {
		// if (stepX % poll == 0) {
		// 	return true;
		// } else {
		// 	return false;
		// }
		return false;

	}
	
	void reset() override {
		
		for (int i = 0; i < 8; i++) {
			gateState[i] = true;
		}
				
	}
	
};



void Progress::step() {
	
	// int outRoot;
	// int outQ;
	// std::cout << " TEST " << std::endl;
	// for (int mode = 0; mode < Quantizer::NUM_MODES; mode++) {
	// 	for (int root = 0; root < Quantizer::NUM_NOTES; root++) {
	// 		for (int tonic = 0; tonic < Quantizer::NUM_TONICS; tonic++) {
	// 			chords.getRootFromMode(mode,root,tonic,&outRoot,&outQ);
	// 		}
	// 	}
	// }
	
	
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

	bool haveRoot = false;
	bool haveMode = false;

	// index is our current step
	if (inputs[KEY_INPUT].active) {
		float fRoot = inputs[KEY_INPUT].value;
		currKey = q.getKeyFromVolts(fRoot);
		haveRoot = true;
	}

	if (inputs[MODE_INPUT].active) {
		float fMode = inputs[MODE_INPUT].value;
		currMode = q.getModeFromVolts(fMode);	
		haveMode = true;
	}
	
	 modeMode = haveRoot && haveMode;
	
	 if (modeMode && ((prevMode != currMode) || (prevKey != currKey))) { // Input changes so force re-read
	 	for (int step = 0; step < 8; step++) {
			prevTonicInput[step]    = -100.0;
			prevQualityInput[step]  = -100.0;
		}
		
		prevMode = currMode;
		prevKey = currKey;
		
	 }
	 
	
	// Read inputs
	for (int step = 0; step < 8; step++) {
		if (modeMode) {
			currTonicInput[step]  = params[CHORD_PARAM + step].value;
			currQualityInput[step] = params[ROOT_PARAM + step].value;
			if (prevModeMode != modeMode) { // Switching mode, so reset history to ensure re-read on return
				prevChrInput[step]  = -100.0;
				prevRootInput[step]  = -100.0;
			}
		} else {
			currChrInput[step]  = params[CHORD_PARAM + step].value;
			currRootInput[step] = params[ROOT_PARAM + step].value;
			if (prevModeMode != modeMode) { // Switching mode, so reset history to ensure re-read on return
				prevTonicInput[step]  = -100.0;
				prevQualityInput[step]  = -100.0;
			}
		}
		currInvInput[step]  = params[INV_PARAM + step].value;
	}
	
	// Remember mode
	prevModeMode = modeMode;
	
	// Check for changes on all steps
	for (int step = 0; step < 8; step++) {
		
		bool update = false;
		
		if (modeMode) {			
		
			currTonicInput[step]   = params[ROOT_PARAM + step].value;
			currQualityInput[step] = params[CHORD_PARAM + step].value;
							
			if (prevTonicInput[step] != currTonicInput[step]) {
				prevTonicInput[step] = currTonicInput[step];
				update = true;
			}
		
			if (prevQualityInput[step] != currQualityInput[step]) {
				prevQualityInput[step]  = currQualityInput[step]; 
				update = true;
			}
			
			if (update) {
				
				// Get Tonic (I- VII)
				currTonic[step] = round(rescalef(fabs(currTonicInput[step]), 0.0, 10.0, 0, Quantizer::NUM_TONICS - 1)); 

				if (debug()) { std::cout << stepX << " MODE: Mode: " << currMode << " (" <<  q.modeNames[currMode] 
					<< ") currKey: " << currKey << " (" <<  q.noteNames[currKey] << ") Tonic: " << currTonic[step] << " (" << q.tonicNames[currTonic[step]] << ")" << std::endl; }

				// From the input root, mode and tonic, we can get the root chord note and quality (Major,Minor,Diminshed)
				chords.getRootFromMode(currMode,currKey,currTonic[step],&currRoot[step],&currQuality[step]);

				if (debug()) { std::cout << stepX << " MODE: Root: " << q.noteNames[currRoot[step]] << " Quality: " << chords.qualityNames[currQuality[step]] << std::endl; }

				// Now get the actual chord from the main list
				switch(currQuality[step]) {
					case Chord::MAJ: 
						currChord[step] = round(rescalef(fabs(currQualityInput[step]), 0.0, 10.0, 1, 70)); 
						break;
					case Chord::MIN: 
						currChord[step] = round(rescalef(fabs(currQualityInput[step]), 0.0, 10.0, 71, 90));
						break;
					case Chord::DIM: 
						currChord[step] = round(rescalef(fabs(currQualityInput[step]), 0.0, 10.0, 91, 98));
						break;		
				}
			
			}

		} else {

			// Chord Mode
			
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

		}
		
		// Inversions remain the same between Chord and Mode mode
		if (prevInvInput[step] != currInvInput[step]) {
			prevInvInput[step]  = currInvInput[step];		
			currInv[step] = currInvInput[step];
			update = true;
		}

		// So, after all that, we calculate the pitch output
		if (update) {
			
			if (debug()) { std::cout << stepX << " UPDATE Step: " << step << " Input: Root: " << currRoot[step] << " Chord: " << currChord[step] << " Inversion: "
		 		<< currInv[step] << std::endl; }
		
			int *chordArray;
	
			// Get the array of pitches based on the inversion
			switch(currInv[step]) {
				case ROOT:  		chordArray = chords.Chords[currChord[step]].root; 	break;
				case FIRST_INV:  	chordArray = chords.Chords[currChord[step]].first; 	break;
				case SECOND_INV:  	chordArray = chords.Chords[currChord[step]].second;	break;
				default: chordArray = chords.Chords[currChord[step]].root;
			}
			
			if (debug()) { std::cout << stepX  << " UPDATE Step: " << step << " Output: Chord: "
					<< q.noteNames[currRoot[step]] << chords.Chords[currChord[step]].quality << " (" << status[currInv[step]] << ") " << std::endl; }
	 	
			for (int j = 0; j < NUM_PITCHES; j++) {
	
				// Set the pitches for this step. If the chord has less than 6 notes, the empty slots are
				// filled with repeated notes. These notes are identified by a  24 semi-tome negative
				// offset. We correct for that offset now, pitching thaem back into the original octave.
				// They could be pitched into the octave above (or below)
				if (chordArray[j] < 0) {
					pitches[step][j] = q.getVoltsFromPitch(chordArray[j] + offset,currRoot[step]);			
				} else {
					pitches[step][j] = q.getVoltsFromPitch(chordArray[j],currRoot[step]);			
				}	
			}
		}	
	}
	
	// Set the pitch output
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
	box.size = Vec(15*34, 380);

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

	static const float portX[13] = {20, 58, 96, 135, 173, 212, 250, 288, 326, 364, 402, 440, 478};
	addInput(createInput<PJ301MPort>(Vec(portX[0]-1, 98), module, Progress::CLOCK_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[1]-1, 98), module, Progress::EXT_CLOCK_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[2]-1, 98), module, Progress::RESET_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[3]-1, 98), module, Progress::STEPS_INPUT));

	addInput(createInput<PJ301MPort>(Vec(portX[4]-1, 98), module, Progress::KEY_INPUT));
	addInput(createInput<PJ301MPort>(Vec(portX[5]-1, 98), module, Progress::MODE_INPUT));


	addOutput(createOutput<PJ301MPort>(Vec(portX[6]-1, 98), module, Progress::GATES_OUTPUT));
			
	for (int i = 0; i < Progress::NUM_PITCHES; i++) {
		addOutput(createOutput<PJ301MPort>(Vec(portX[i + 7], 98),  module, Progress::PITCH_OUTPUT + i));
	}	

	for (int i = 0; i < 8; i++) {
		addParam(createParam<AHKnobNoSnap>(Vec(portX[i]-2, 157), module, Progress::ROOT_PARAM + i, 0.0, 10.0, 0.0));
		addParam(createParam<AHKnobNoSnap>(Vec(portX[i]-2, 198), module, Progress::CHORD_PARAM + i, 0.0, 10.0, 0.0));
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



