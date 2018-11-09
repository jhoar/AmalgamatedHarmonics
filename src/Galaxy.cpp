#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"
#include "dsp/digital.hpp"

#include <iostream>

struct Galaxy : AHModule {

	const static int NUM_PITCHES = 6;
	const static int N_QUALITIES = 5;
	const static int N_NOTES = 12;

	enum ParamIds {
		KEY_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		MOVE_INPUT,
		KEY_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(PITCH_OUTPUT,6),
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

	int degree = 0;
	int quality = 0;
	int noteIndex = 0; 
	int inversion = 0;

	int lastQuality = 0;
	int lastNoteIndex = 0; 
	int lastInversion = 0;

	int currRoot = -1;
	int light;

	int offset = 12; 	   // 0 = random, 12 = lower octave, 24 = repeat, 36 = upper octave
	int mode = 0; 	   // 0 = random chords in scale, 1 = random progressions

};

void Galaxy::step() {
	
	AHModule::step();

	// Get inputs from Rack
	bool move = moveTrigger.process(inputs[MOVE_INPUT].value);

	if (move) {

		bool changed = false;

		if (inputs[KEY_INPUT].active) {
			float fRoot = inputs[KEY_INPUT].value;
			currRoot = CoreUtil().getKeyFromVolts(fRoot);
		} else {
			currRoot = params[KEY_PARAM].value;
		}

		// TODO Implement selection function
		int rotSign = rand() % 2 ? 1 : -1;
		int rotateInput = rotSign * (rand() % 1 + 1); // -2 to 2

		int radSign = rand() % 2 ? 1 : -1;
		int radialInput = radSign * (rand() % 2 + 1); // -2 to 2

		// std::cout << "Rotate: " << rotateInput << "  Radial: " << radialInput << std::endl;

		// std::cout << "Str position: Root: " << currRoot << 
		// 	" degree: " << degree << 
		// 	" quality: " << quality <<
		// 	" noteIndex: " << noteIndex << std::endl;

		// Determine move around the grid
		quality += rotateInput;
		if (quality < 0) {
			quality += N_QUALITIES;
		} else if (quality >= N_QUALITIES) {
			quality -= N_QUALITIES;
		}

		if (quality != lastQuality) {
			changed = true;
			lastQuality = quality;
		}

		// Determine move in/out of the grid
		int *curScaleArr = CoreUtil().ASCALE_IONIAN;
		int notesInScale = LENGTHOF(CoreUtil().ASCALE_IONIAN);

		degree += radialInput; // Next degree of the scale
		if (degree < 0) {
			degree += notesInScale;
		} else if (degree >= notesInScale) {
			degree -= notesInScale;
		}

		noteIndex = (currRoot + curScaleArr[degree]) % 12;

		// std::cout << "End position: Root: " << currRoot << 
		// 	" degree: " << degree << 
		// 	" quality: " << quality <<
		// 	" noteIndex: " << noteIndex << std::endl << std::endl;

		if (noteIndex != lastNoteIndex) {
			changed = true;
			lastNoteIndex = noteIndex;
		}

		// Determine which chord corresponds to the grid position
		int chord = ChordTable[quality];
		int inversion = rand() % 3;
		int *chordArray;

		switch(inversion) {
			case Core::ROOT:  		chordArray = CoreUtil().ChordTable[chord].root; 	break;
			case Core::FIRST_INV:  	chordArray = CoreUtil().ChordTable[chord].first; 	break;
			case Core::SECOND_INV:  chordArray = CoreUtil().ChordTable[chord].second;	break;
			default: 				chordArray = CoreUtil().ChordTable[chord].root;
		}

		if (inversion != lastInversion) {
			changed = true;
			lastInversion = inversion;
		}

		// Determine which notes corresponds to the chord
		for (int j = 0; j < NUM_PITCHES; j++) {
 
		// Set the pitches for this step. If the chord has less than 6 notes, the empty slots are
		// filled with repeated notes. These notes are identified by a  24 semi-tome negative
		// offset. We correct for that offset now, pitching thaem back into the original octave.
		// They could be pitched into the octave above (or below)
			if (chordArray[j] < 0) {
				int off = offset;
				if (off == 0) {
					off = (rand() % 3 + 1) * 12;
				}
				outVolts[j] = CoreUtil().getVoltsFromPitch(chordArray[j] + off, noteIndex);			
			} else {
				outVolts[j] = CoreUtil().getVoltsFromPitch(chordArray[j], noteIndex);			
			}	
		}

		int newlight = noteIndex + (quality * N_NOTES);
		if (changed) {
			lights[NOTE_LIGHT + light].value = 0.0f;
			lights[NOTE_LIGHT + newlight].value = 1.0f;
			light = newlight;
		}

	}

	// Set the output pitches and lights
	for (int i = 0; i < NUM_PITCHES; i++) {
		outputs[PITCH_OUTPUT + i].value = outVolts[i];
	}

}

struct GalaxyWidget : ModuleWidget {

	GalaxyWidget(Galaxy *module) : ModuleWidget(module) {
	
		UI ui;
		
		box.size = Vec(240, 380);

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

		float div = (PI * 2) / (float)Galaxy::N_QUALITIES;
		float div2 = (PI * 2) / (float)(Galaxy::N_QUALITIES * Galaxy::N_QUALITIES);

		for (int q = 0; q < Galaxy::N_QUALITIES; q++) {

			for (int n = 0; n < Galaxy::N_NOTES; n++) {

				float cosDiv = cos(div * q + div2 * n);
				float sinDiv = sin(div * q + div2 * n);

				float xPos  = sinDiv * (32.5 + (7.5 * n));
				float yPos  = cosDiv * (32.5 + (7.5 * n));

				int l = n + (q * Galaxy::N_NOTES);

				addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(Vec(xPos + 110.5, 149.5 - yPos), module, Galaxy::NOTE_LIGHT + l));

			}

		}
		
		for (int i = 0; i < 6; i++) {
			addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, i, 5, true, false), Port::OUTPUT, module, Galaxy::PITCH_OUTPUT + i));
		}	

		addInput(Port::create<PJ301MPort>(Vec(102, 140), Port::INPUT, module, Galaxy::MOVE_INPUT));

		addParam(ParamWidget::create<AHKnobSnap>(ui.getPosition(UI::KNOB, 0, 4, true, false), module, Galaxy::KEY_PARAM, 0.0, 11.0, 0.0)); 
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 1, 4, true, false), Port::INPUT, module, Galaxy::KEY_INPUT));

	}

	void appendContextMenu(Menu *menu) override {
			Galaxy *gal = dynamic_cast<Galaxy*>(module);
			assert(gal);

			struct GalModeItem : MenuItem {
				Galaxy *gal;
				void onAction(EventAction &e) override {
					gal->mode^= 1;
				}
				void step() override {
					rightText = gal->mode ? "Scale" : "Progression";
					MenuItem::step();
				}
			};

			struct GalOffsetItem : MenuItem {
				Galaxy *gal;
				void onAction(EventAction &e) override {
					gal->offset += 12;
					if(gal->offset == 48) {
						gal->offset = 0;
					}
				}
				void step() override {
					if (gal->offset == 12) {
						rightText = "Lower";
					} else if (gal->offset == 24) {
						rightText = "Repeat";	
					} else if (gal->offset == 36) {
						rightText = "Upper";	
					} else if (gal->offset == 0) {
						rightText = "Random";	
					}else {
						rightText = "Error!";	
					}
					MenuItem::step();
				}
			};

			menu->addChild(construct<MenuLabel>());
			menu->addChild(construct<GalOffsetItem>(&MenuItem::text, "Repeat Notes", &GalOffsetItem::gal, gal));
	}


};

Model *modelGalaxy = Model::create<Galaxy, GalaxyWidget>( "Amalgamated Harmonics", "Galaxy", "Galaxy", SEQUENCER_TAG);

// ♯♭
