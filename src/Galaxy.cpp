#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"
#include "dsp/digital.hpp"

#include <iostream>

// TODO switchable chromatic vs fifths,  add root input (from 5/4), transition probabilities, 
// note offset switchable (upper vs lower octave), UI

struct Galaxy : AHModule {

	const static int NUM_PITCHES = 6;
	const static int N_QUALITIES = 5;
	const static int N_NOTES = 12;

	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		MOVE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(PITCH_OUTPUT,6),
		GATE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(NOTE_LIGHT,60),
		NUM_LIGHTS
	};
	
	Galaxy() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
		
	Core core;

	int ChordTable[N_QUALITIES] = { 1, 31, 78, 25, 71 };
	float outVolts[NUM_PITCHES];
	
	int poll = 50000;

	SchmittTrigger moveTrigger;
	PulseGenerator triggerPulse;

	int quality = 0;
	int noteIndex = 0; 
	int note;
	int light = 0;

	// These should be changable through context menu
	bool chromatic = true; // false = fifths
	int offset = 12; 	   // 12 = lower octave, 24 = repeat, 36 = upper octave

};

void Galaxy::step() {
	
	AHModule::step();

	// Get inputs from Rack
	bool move = moveTrigger.process(inputs[MOVE_INPUT].value);

	if (move) {

		// TODO Implement selection function
		int rotateInput = (rand() % 5) - 2; // -2 to 2
		int radialInput = (rand() % 5) - 2; // -2 to 2

		// Determine move around the grid
		quality += rotateInput;
		if (quality < 0) {
			quality += N_QUALITIES;
		} else if (quality >= N_QUALITIES) {
			quality -= N_QUALITIES;
		}

		// Determine move in/out of the grid
		noteIndex += radialInput;
		if (noteIndex < 0) {
			noteIndex += N_NOTES;
		} else if (noteIndex >= N_NOTES) {
			noteIndex -= N_NOTES;
		}

		// Get root note from FIFTHs
		if (chromatic) { 
			note = noteIndex;
		} else {
			note = CoreUtil().CIRCLE_FIFTHS[noteIndex];
		}

		// Determine which chord corresponds to the grid position
		int chord = ChordTable[quality];
		int *chordArray = CoreUtil().ChordTable[chord].root;

		// Determine which notes corresponds to the chord
		for (int j = 0; j < NUM_PITCHES; j++) {
 
		// Set the pitches for this step. If the chord has less than 6 notes, the empty slots are
		// filled with repeated notes. These notes are identified by a  24 semi-tome negative
		// offset. We correct for that offset now, pitching thaem back into the original octave.
		// They could be pitched into the octave above (or below)
			if (chordArray[j] < 0) {
				outVolts[j] = CoreUtil().getVoltsFromPitch(chordArray[j] + offset, note);			
			} else {
				outVolts[j] = CoreUtil().getVoltsFromPitch(chordArray[j], note);			
			}	
		}

		int newlight = noteIndex + (quality * N_NOTES);
		if (newlight != light) {
			triggerPulse.trigger(Core::TRIGGER);
			lights[NOTE_LIGHT + light].value = 0.0f;
			lights[NOTE_LIGHT + newlight].value = 1.0f;
			light = newlight;
		}

	}

	// Set the output pitches and lights
	for (int i = 0; i < NUM_PITCHES; i++) {
		outputs[PITCH_OUTPUT + i].value = outVolts[i];
	}

	bool gPulse = triggerPulse.process(delta);
	outputs[GATE_OUTPUT].value = gPulse ? 10.0 : 0.0;

}

struct GalaxyWidget : ModuleWidget {
	GalaxyWidget(Galaxy *module);
};

GalaxyWidget::GalaxyWidget(Galaxy *module) : ModuleWidget(module) {
	
	UI ui;
	
	box.size = Vec(285, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Galaxy.svg")));
		addChild(panel);
	}

	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 30, 365)));

	addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 0, 0, true, false), Port::INPUT, module, Galaxy::MOVE_INPUT));

	float div = (PI * 2) / (float)Galaxy::N_QUALITIES;
	float div2 = (PI * 2) / (float)(Galaxy::N_QUALITIES * Galaxy::N_QUALITIES);

	for (int q = 0; q < Galaxy::N_QUALITIES; q++) {

		for (int n = 0; n < Galaxy::N_NOTES; n++) {

			float cosDiv = cos(div * q + div2 * n);
			float sinDiv = sin(div * q + div2 * n);

			float xPos  = sinDiv * (32.5 + (7.5 * n));
			float yPos  = cosDiv * (32.5 + (7.5 * n));

			int l = n + (q * Galaxy::N_NOTES);

			addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(Vec(xPos + 136.5, 149.5 - yPos), module, Galaxy::NOTE_LIGHT + l));

		}

	}
	
	for (int i = 0; i < 6; i++) {
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, i, 5, false, false), Port::OUTPUT, module, Galaxy::PITCH_OUTPUT + i));
	}	

	addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 5, 0, false, false), Port::OUTPUT, module, Galaxy::GATE_OUTPUT));

}

Model *modelGalaxy = Model::create<Galaxy, GalaxyWidget>( "Amalgamated Harmonics", "Galaxy", "Chord Galaxy", SEQUENCER_TAG);

// ♯♭
