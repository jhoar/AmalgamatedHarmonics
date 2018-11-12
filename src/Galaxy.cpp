#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"
#include "dsp/digital.hpp"

#include <iostream>

struct Galaxy : AHModule {

	const static int NUM_PITCHES = 6;
	const static int N_QUALITIES = 6;
	const static int N_NOTES = 12;

	enum ParamIds {
		KEY_PARAM,
		MODE_PARAM,
		BAD_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		MOVE_INPUT,
		KEY_INPUT,
		MODE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(PITCH_OUTPUT,6),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(NOTE_LIGHT,72),
		NUM_LIGHTS
	};
	
	Galaxy() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	void getFromRandom();
	void getFromKey();
	void getFromKeyMode();

	json_t *toJson() override {
		json_t *rootJ = json_object();

		// offset
		json_t *offsetJ = json_integer((int) offset);
		json_object_set_new(rootJ, "offset", offsetJ);

		// mode
		json_t *modeJ = json_integer((int) mode);
		json_object_set_new(rootJ, "mode", modeJ);

		return rootJ;
	}
	
	void fromJson(json_t *rootJ) override {

		// offset
		json_t *offsetJ = json_object_get(rootJ, "offset");
		if (offsetJ)
			offset = json_integer_value(offsetJ);

		// mode
		json_t *modeJ = json_object_get(rootJ, "mode");
		if (modeJ)
			mode = json_integer_value(modeJ);

	}

	Core core;

	int ChordTable[N_QUALITIES] = { 1, 31, 78, 25, 71, 91 };
	int Quality2Quality[3] = {0, 4, 5};
	float outVolts[NUM_PITCHES];
	
	int poll = 50000;

	SchmittTrigger moveTrigger;

	int degree = 0;
	int quality = 0;
	int qualityIndex = 0;
	int noteIndex = 0; 
	int inversion = 0;

	int lastQuality = 0;
	int lastNoteIndex = 0; 
	int lastInversion = 0;

	int currRoot = 1;
	int currMode = 1;
	int light;

	bool haveRoot = false;
	bool haveMode = false;

	int offset = 12; 	   // 0 = random, 12 = lower octave, 24 = repeat, 36 = upper octave
	int mode = 0; 	   // 0 = random chord, 1 = chord in key, 2 = chord in mode

	std::string rootName;
	std::string modeName;

	std::string chordName;
	std::string chordExtName;
};

void Galaxy::step() {
	
	AHModule::step();

	// Get inputs from Rack
	bool move = moveTrigger.process(inputs[MOVE_INPUT].value);

	if (move) {

		bool changed = false;

		if (inputs[MODE_INPUT].active) {
			float fMode = inputs[MODE_INPUT].value;
			currMode = round(rescale(fabs(fMode), 0.0f, 10.0f, 0.0f, 6.0f)); 
		} else {
			currMode = params[MODE_PARAM].value;
		}

		if (inputs[KEY_INPUT].active) {
			float fRoot = inputs[KEY_INPUT].value;
			currRoot = CoreUtil().getKeyFromVolts(fRoot);
		} else {
			currRoot = params[KEY_PARAM].value;
		}

		// std::cout << "Str position: Root: " << currRoot << 
		// 	" Mode: " << currMode << 
		// 	" degree: " << degree << 
		// 	" quality: " << quality <<
		// 	" noteIndex: " << noteIndex << std::endl;

		if (mode == 0) {
			getFromRandom();
		} else if (mode == 1) {

			if (randomUniform() < params[BAD_PARAM].value) {
				getFromRandom();
			} else {
				getFromKey();
			}

		} else if (mode == 2) {

			float excess = params[BAD_PARAM].value - randomUniform();

			if (excess > 0.5) {
				getFromRandom();
			} else if (excess > 0.2) {
				getFromKey();
			} else {
				getFromKeyMode();
			}

		}

		// Determine which chord corresponds to the grid position
		int chord = ChordTable[quality];
		int *chordArray;

		switch(rand() % 10) {
			case 0: 
			case 1: 
			case 2: 
			case 3: 
			case 4: 	
			case 5: 	inversion = 0; chordArray = CoreUtil().ChordTable[chord].root; 	break;
			case 6: 
			case 7: 	
			case 8: 	inversion = 1; chordArray = CoreUtil().ChordTable[chord].first; 	break;
			case 9: 	inversion = 2; chordArray = CoreUtil().ChordTable[chord].second;	break;
			default: 	chordArray = CoreUtil().ChordTable[chord].root;
		}

		// std::cout << "End position: Root: " << currRoot << 
		// 	" Mode: " << currMode << 
		// 	" Degree: " << degree << 
		// 	" Quality: " << quality <<
		// 	" Inversion: " << inversion << " " << chordArray <<
		// 	" NoteIndex: " << noteIndex << std::endl << std::endl;

		if (quality != lastQuality) {
			changed = true;
			lastQuality = quality;
		}

		if (noteIndex != lastNoteIndex) {
			changed = true;
			lastNoteIndex = noteIndex;
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

			int chordIndex = ChordTable[quality];

			chordName = 
				CoreUtil().noteNames[noteIndex] + 
				CoreUtil().ChordTable[chordIndex].quality + " " + 
				CoreUtil().inversionNames[inversion];

			if (mode == 2) {
				rootName = CoreUtil().noteNames[currRoot];
				modeName = CoreUtil().modeNames[currMode];
				chordExtName = CoreUtil().degreeNames[degree * 3 + qualityIndex]; 
			} else {
				rootName = "";
				modeName = "";
				chordExtName = "";
			}

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

void Galaxy::getFromRandom() {

	int rotSign = rand() % 2 ? 1 : -1;
	int rotateInput = rotSign * (rand() % 1 + 1); // -2 to 2

	int radSign = rand() % 2 ? 1 : -1;
	int radialInput = radSign * (rand() % 2 + 1); // -2 to 2

	// std::cout << "Rotate: " << rotateInput << "  Radial: " << radialInput << std::endl;

	// Determine move around the grid
	quality += rotateInput;
	if (quality < 0) {
		quality += N_QUALITIES;
	} else if (quality >= N_QUALITIES) {
		quality -= N_QUALITIES;
	}

	noteIndex += radialInput;
	if (noteIndex < 0) {
		noteIndex += N_NOTES;
	} else if (noteIndex >= N_NOTES) {
		noteIndex -= N_NOTES;
	}

}

void Galaxy::getFromKey() {

	int rotSign = rand() % 2 ? 1 : -1;
	int rotateInput = rotSign * (rand() % 1 + 1); // -2 to 2

	int radSign = rand() % 2 ? 1 : -1;
	int radialInput = radSign * (rand() % 2 + 1); // -2 to 2

	// std::cout << "Rotate: " << rotateInput << "  Radial: " << radialInput << std::endl;

	// Determine move around the grid
	quality += rotateInput;
	if (quality < 0) {
		quality += N_QUALITIES;
	} else if (quality >= N_QUALITIES) {
		quality -= N_QUALITIES;
	}

	// Just major scale
	int *curScaleArr = CoreUtil().ASCALE_IONIAN;
	int notesInScale = LENGTHOF(CoreUtil().ASCALE_IONIAN);

	// Determine move through the scale
	degree += radialInput; 
	if (degree < 0) {
		degree += notesInScale;
	} else if (degree >= notesInScale) {
		degree -= notesInScale;
	}

	noteIndex = (currRoot + curScaleArr[degree]) % 12;

}

void Galaxy::getFromKeyMode() {

	int rotSign = rand() % 2 ? 1 : -1;
	int rotateInput = rotSign * (rand() % 1 + 1); // -2 to 2

	// Determine move through the scale
	degree += rotateInput;
	if (degree < 0) {
		degree += Core::NUM_DEGREES;
	} else if (degree >= Core::NUM_DEGREES) {
		degree -= Core::NUM_DEGREES;
	}

	// From the input root, mode and degree, we can get the root chord note and quality (Major,Minor,Diminshed)
	CoreUtil().getRootFromMode(currMode,currRoot,degree,&noteIndex,&qualityIndex);
	quality = Quality2Quality[qualityIndex];

}

struct GalaxyDisplay : TransparentWidget {
	
	Galaxy *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	GalaxyDisplay() {
		font = Font::load(assetPlugin(plugin, "res/EurostileBold.ttf"));
	}

	void draw(NVGcontext *vg) override {
	
		nvgFontSize(vg, 12);
		nvgFontFaceId(vg, font->handle);
		nvgFillColor(vg, nvgRGBA(255, 0, 0, 0xff));
		nvgTextLetterSpacing(vg, -1);

		char text[128];

		snprintf(text, sizeof(text), "%s", module->chordName.c_str());
		nvgText(vg, box.pos.x + 5, box.pos.y, text, NULL);

		snprintf(text, sizeof(text), "%s", module->chordExtName.c_str());
		nvgText(vg, box.pos.x + 5, box.pos.y + 11, text, NULL);

		nvgTextAlign(vg, NVG_ALIGN_RIGHT);
		snprintf(text, sizeof(text), "%s", module->rootName.c_str());
		nvgText(vg, box.size.x - 5, box.pos.y, text, NULL);

		snprintf(text, sizeof(text), "%s", module->modeName.c_str());
		nvgText(vg, box.size.x - 5, box.pos.y + 11, text, NULL);

	}
	
};

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

		{
			GalaxyDisplay *display = new GalaxyDisplay();
			display->module = module;
			display->box.pos = Vec(0, 20);
			display->box.size = Vec(240, 230);
			addChild(display);
		}

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

		addParam(ParamWidget::create<AHKnobSnap>(ui.getPosition(UI::KNOB, 4, 4, true, false), module, Galaxy::MODE_PARAM, 0.0, 6.0, 0.0)); 
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 5, 4, true, false), Port::INPUT, module, Galaxy::MODE_INPUT));

		Vec trim = ui.getPosition(UI::TRIMPOT, 5, 3, true, false);
		trim.x += 15;
		trim.y += 25;

		addParam(ParamWidget::create<AHTrimpotNoSnap>(trim, module, Galaxy::BAD_PARAM, 0.0, 1.0, 0.0)); 

	}

	void appendContextMenu(Menu *menu) override {
			Galaxy *gal = dynamic_cast<Galaxy*>(module);
			assert(gal);

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

			struct GalModeItem : MenuItem {
				Galaxy *gal;
				void onAction(EventAction &e) override {
					gal->mode++;
					if(gal->mode == 3) {
						gal->mode = 0;
					}
				}
				void step() override {
					if (gal->mode == 0) {
						rightText = "Random";
					} else if (gal->mode == 1) {
						rightText = "in Key";	
					} else if (gal->mode == 2) {
						rightText = "in Mode";
					} else {
						rightText = "Error!";	
					}
					MenuItem::step();
				}
			};

			menu->addChild(construct<MenuLabel>());
			menu->addChild(construct<GalOffsetItem>(&MenuItem::text, "Repeat Notes", &GalOffsetItem::gal, gal));
			menu->addChild(construct<GalModeItem>(&MenuItem::text, "Chord Selection", &GalModeItem::gal, gal));
	}

};

Model *modelGalaxy = Model::create<Galaxy, GalaxyWidget>( "Amalgamated Harmonics", "Galaxy", "Galaxy", SEQUENCER_TAG);

// ♯♭
