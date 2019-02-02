#include <iostream>

#include "AH.hpp"
#include "AHCommon.hpp"

using namespace ah;

struct Galaxy : core::AHModule {

	const static int NUM_PITCHES = 6;
	const static int N_QUALITIES = 6;
	const static int N_NOTES = 12;
	const static int QMAP_SIZE = 20;

	std::string degNames[42] { // Degree * 3 + Quality
		"I",
		"I7",
		"im7",
		"IM7",
		"i",
		"i°",
		"II",
		"II7",
		"iim7",
		"IIM7",
		"ii",
		"ii°",
		"III",
		"III7",
		"iiim7",
		"IIIM7",
		"iii",
		"iii°",
		"IV",
		"IV7",
		"ivm7",
		"IVM7",
		"iv",
		"iv°",
		"V",
		"V7",
		"vm7",
		"VM7",
		"v",
		"v°",
		"VI",
		"VI7",
		"vim7",
		"VIM7",
		"vi",
		"vi°",
		"VII",
		"VII7",
		"viim7",
		"VIIM7",
		"vii",
		"vii°"
	};

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
		ENUMS(BAD_LIGHT,2),
		NUM_LIGHTS
	};
	
	Galaxy() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		params[KEY_PARAM].config(0.0, 11.0, 0.0, "Key");
		params[KEY_PARAM].description = "Key from which chords are selected"; 

		params[MODE_PARAM].config(0.0, 6.0, 0.0, "Mode");
		params[MODE_PARAM].description = "Mode from which chord are selected"; 

		params[BAD_PARAM].config(0.0, 1.0, 0.0, "Bad"); 
		params[BAD_PARAM].description = "Deviation from chord selection rule for the mode";
	}

	void step() override;

	void getFromRandom();
	void getFromKey();
	void getFromKeyMode();

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// offset
		json_t *offsetJ = json_integer((int) offset);
		json_object_set_new(rootJ, "offset", offsetJ);

		// mode
		json_t *modeJ = json_integer((int) mode);
		json_object_set_new(rootJ, "mode", modeJ);

		// inversions
		json_t *inversionsJ = json_integer((int) allowedInversions);
		json_object_set_new(rootJ, "inversions", inversionsJ);

		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override {

		// offset
		json_t *offsetJ = json_object_get(rootJ, "offset");
		if (offsetJ)
			offset = json_integer_value(offsetJ);

		// mode
		json_t *modeJ = json_object_get(rootJ, "mode");
		if (modeJ)
			mode = json_integer_value(modeJ);

		// mode
		json_t *inversionsJ = json_object_get(rootJ, "inversions");
		if (inversionsJ)
			allowedInversions = json_integer_value(inversionsJ);

	}

	int ChordTable[N_QUALITIES] = { 1, 31, 78, 25, 71, 91 }; // M, 7, m7, M7, m, dim
	int QualityMap[3][QMAP_SIZE] = { 
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,3,1},
		{4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,2,2,1},
		{5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}
	};
	int InversionMap[3][QMAP_SIZE] = { 
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1},
		{0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,2,2},
	};

	float outVolts[NUM_PITCHES];
	
	int poll = 50000;

	rack::dsp::SchmittTrigger moveTrigger;

	int degree = 0;
	int quality = 0;
	int noteIndex = 0; 
	int inversion = 0;

	int lastQuality = 0;
	int lastNoteIndex = 0; 
	int lastInversion = 0;

	int currRoot = 1;
	int currMode = 1;
	int light = 0;

	bool haveRoot = false;
	bool haveMode = false;

	int offset = 12; 	   // 0 = random, 12 = lower octave, 24 = repeat, 36 = upper octave
	int mode = 1; 	   // 0 = random chord, 1 = chord in key, 2 = chord in mode
	int allowedInversions = 0; // 0 = root only, 1 = root + first, 2 = root, first, second

	std::string rootName = "";
	std::string modeName = "";

	std::string chordName = "";
	std::string chordExtName = "";
};

void Galaxy::step() {
	
	core::AHModule::step();

	int badLight = 0;

	// Get inputs from Rack
	bool move = moveTrigger.process(inputs[MOVE_INPUT].getVoltage());

	if (inputs[MODE_INPUT].isConnected()) {
		float fMode = inputs[MODE_INPUT].getVoltage();
		currMode = music::getModeFromVolts(fMode);
	} else {
		currMode = params[MODE_PARAM].getValue();
	}

	if (inputs[KEY_INPUT].isConnected()) {
		float fRoot = inputs[KEY_INPUT].getVoltage();
		currRoot = music::getKeyFromVolts(fRoot);
	} else {
		currRoot = params[KEY_PARAM].getValue();
	}

	if (mode == 1) {
		rootName = music::noteNames[currRoot];
		modeName = "";
	} else if (mode == 2) {
		rootName = music::noteNames[currRoot];
		modeName = music::modeNames[currMode];
	} else {
		rootName = "";
		modeName = "";
		chordExtName = "";
	}

	if (move) {

		bool changed = false;
		bool haveMode = false;

		// std::cout << "Str position: Root: " << currRoot << 
		// 	" Mode: " << currMode << 
		// 	" degree: " << degree << 
		// 	" quality: " << quality <<
		// 	" noteIndex: " << noteIndex << std::endl;

		if (mode == 0) {
			getFromRandom();
		} else if (mode == 1) {

			if (random::uniform() < params[BAD_PARAM].getValue()) {
				badLight = 2;
				getFromRandom();
			} else {
				getFromKey();
			}

		} else if (mode == 2) {

			float excess = params[BAD_PARAM].getValue() - random::uniform();

			if (excess < 0.0) {
				getFromKeyMode();
				haveMode = true;
			} else {
				if (excess < 0.2) {
					badLight = 1;
					getFromKey();
				} else {
					badLight = 2;
					getFromRandom();
				}
			}

		}

		inversion = InversionMap[allowedInversions][rand() % QMAP_SIZE];
		int chord = ChordTable[quality];

		// Determine which chord corresponds to the grid position
		int *chordArray;
		switch(inversion) {
			case 0: 	chordArray = music::ChordTable[chord].root; 	break;
			case 1: 	chordArray = music::ChordTable[chord].first; 	break;
			case 2: 	chordArray = music::ChordTable[chord].second;	break;
			default: 	chordArray = music::ChordTable[chord].root;
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
				outVolts[j] = music::getVoltsFromPitch(chordArray[j] + off, noteIndex);			
			} else {
				outVolts[j] = music::getVoltsFromPitch(chordArray[j], noteIndex);			
			}	
		}

		int newlight = noteIndex + (quality * N_NOTES);

		if (changed) {

			int chordIndex = ChordTable[quality];

			chordName = 
				music::noteNames[noteIndex] + 
				music::ChordTable[chordIndex].quality + " " + 
				music::inversionNames[inversion];

			if (mode == 2) {
				if (haveMode) {
					chordExtName = degNames[degree * 6 + quality];
				} else {
					chordExtName = "";
				} 
			}

			lights[NOTE_LIGHT + light].setBrightness(0.0f);
			lights[NOTE_LIGHT + newlight].setBrightness(10.0f);
			light = newlight;

		}


	}

	if (badLight == 1) { // Green (scale->key)
		lights[BAD_LIGHT].setBrightnessSmooth(1.0f);
		lights[BAD_LIGHT + 1].setBrightnessSmooth(0.0f);
	} else if (badLight == 2) { // Red (->random)
		lights[BAD_LIGHT].setBrightnessSmooth(0.0f);
		lights[BAD_LIGHT + 1].setBrightnessSmooth(1.0f);
	} else { // No change
		lights[BAD_LIGHT].setBrightnessSmooth(0.0f);
		lights[BAD_LIGHT + 1].setBrightnessSmooth(0.0f);
	}

	// Set the output pitches 

	// Count the number of active ports in P2-P6
	int portCount = 0;
	for (int i = 1; i < NUM_PITCHES - 1; i++) {
		if (outputs[PITCH_OUTPUT + i].isConnected()) {
			portCount++;
		}
	}

	// No active ports in P2-P6, so must be poly
	if (portCount == 0) {
		outputs[PITCH_OUTPUT].setChannels(6);
		for (int i = 0; i < NUM_PITCHES; i++) {
			outputs[PITCH_OUTPUT].setVoltage(outVolts[i], i);
		}
	} else {
		for (int i = 0; i < NUM_PITCHES; i++) {
			outputs[PITCH_OUTPUT + i].setVoltage(outVolts[i]);
		}
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
	int *curScaleArr = music::ASCALE_IONIAN;
	int notesInScale = LENGTHOF(music::ASCALE_IONIAN);

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
		degree += music::NUM_DEGREES;
	} else if (degree >= music::NUM_DEGREES) {
		degree -= music::NUM_DEGREES;
	}

	// From the input root, mode and degree, we can get the root chord note and quality (Major,Minor,Diminshed)
	int q;
	music::getRootFromMode(currMode,currRoot,degree,&noteIndex,&q);
	quality = QualityMap[q][rand() % QMAP_SIZE];

}

struct GalaxyDisplay : TransparentWidget {
	
	Galaxy *module;
	std::shared_ptr<Font> font;

	GalaxyDisplay() {
		font = Font::load(asset::plugin(pluginInstance, "res/EurostileBold.ttf"));
	}

	void draw(const DrawContext &ctx) override {
	
		nvgFontSize(ctx.vg, 12);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgFillColor(ctx.vg, nvgRGBA(255, 0, 0, 0xff));
		nvgTextLetterSpacing(ctx.vg, -1);

		char text[128];

		snprintf(text, sizeof(text), "%s", module->chordName.c_str());
		nvgText(ctx.vg, box.pos.x + 5, box.pos.y, text, NULL);

		snprintf(text, sizeof(text), "%s", module->chordExtName.c_str());
		nvgText(ctx.vg, box.pos.x + 5, box.pos.y + 11, text, NULL);

		nvgTextAlign(ctx.vg, NVG_ALIGN_RIGHT);
		snprintf(text, sizeof(text), "%s", module->rootName.c_str());
		nvgText(ctx.vg, box.size.x - 5, box.pos.y, text, NULL);

		snprintf(text, sizeof(text), "%s", module->modeName.c_str());
		nvgText(ctx.vg, box.size.x - 5, box.pos.y + 11, text, NULL);

	}
	
};

struct GalaxyWidget : ModuleWidget {

	GalaxyWidget(Galaxy *module)  {
	
		setModule(module);
		setPanel(SVG::load(asset::plugin(pluginInstance, "res/Galaxy.svg")));

		float div = (core::PI * 2) / (float)Galaxy::N_QUALITIES;
		float div2 = (core::PI * 2) / (float)(Galaxy::N_QUALITIES * Galaxy::N_QUALITIES);

		for (int q = 0; q < Galaxy::N_QUALITIES; q++) {

			for (int n = 0; n < Galaxy::N_NOTES; n++) {

				float cosDiv = cos(div * q + div2 * n);
				float sinDiv = sin(div * q + div2 * n);

				float xPos  = sinDiv * (32.5 + (7.5 * n));
				float yPos  = cosDiv * (32.5 + (7.5 * n));

				int l = n + (q * Galaxy::N_NOTES);

				addChild(createLight<SmallLight<GreenLight>>(Vec(xPos + 110.5, 149.5 - yPos), module, Galaxy::NOTE_LIGHT + l));

			}

		}
		
		for (int i = 0; i < 6; i++) {
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, i, 5, true, false), module, Galaxy::PITCH_OUTPUT + i));
		}	

		addInput(createInput<PJ301MPort>(Vec(102, 140), module, Galaxy::MOVE_INPUT));

		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 0, 4, true, false), module, Galaxy::KEY_PARAM)); 
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 4, true, false), module, Galaxy::KEY_INPUT));

		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 4, 4, true, false), module, Galaxy::MODE_PARAM)); 
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 4, true, false), module, Galaxy::MODE_INPUT));

		Vec trim = gui::getPosition(gui::TRIMPOT, 5, 3, true, false);
		trim.x += 15;
		trim.y += 25;

		addParam(createParam<gui::AHTrimpotNoSnap>(trim, module, Galaxy::BAD_PARAM)); 

		trim.x += 15;
		trim.y += 20;
		addChild(createLight<SmallLight<GreenRedLight>>(trim, module, Galaxy::BAD_LIGHT));

		if (module != NULL) {
			GalaxyDisplay *displayW = createWidget<GalaxyDisplay>(Vec(0, 20));
			displayW->box.size = Vec(240, 230);
			displayW->module = module;
			addChild(displayW);
		}

	}

	void appendContextMenu(Menu *menu) override {

		Galaxy *galaxy = dynamic_cast<Galaxy*>(module);
		assert(galaxy);

		struct OffsetItem : MenuItem {
			Galaxy *module;
			int offset;
			void onAction(const event::Action &e) override {
				module->offset = offset;
			}
		};

		struct ModeItem : MenuItem {
			Galaxy *module;
			int mode;
			void onAction(const event::Action &e) override {
				module->mode = mode;
			}
		};

		struct InversionItem : MenuItem {
			Galaxy *module;
			int allowedInversions;
			void onAction(const event::Action &e) override {
				module->allowedInversions = allowedInversions;
			}
		};

		struct OffsetMenu : MenuItem {
			Galaxy *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<int> offsets = {12, 24, 36, 0};
				std::vector<std::string> names = {"Lower", "Repeat", "Upper", "Random"};
				for (size_t i = 0; i < offsets.size(); i++) {
					OffsetItem *item = createMenuItem<OffsetItem>(names[i], CHECKMARK(module->offset == offsets[i]));
					item->module = module;
					item->offset = offsets[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		struct ModeMenu : MenuItem {
			Galaxy *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<int> modes = {0, 1, 2};
				std::vector<std::string> names = {"Random", "in Key", "in Mode"};
				for (size_t i = 0; i < modes.size(); i++) {
					ModeItem *item = createMenuItem<ModeItem>(names[i], CHECKMARK(module->mode == modes[i]));
					item->module = module;
					item->mode = modes[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		struct InversionMenu : MenuItem {
			Galaxy *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<int> inversions = {0, 1, 2};
				std::vector<std::string> names = {"Root only", "Root and First", "Root, First and Second"};
				for (size_t i = 0; i < inversions.size(); i++) {
					InversionItem *item = createMenuItem<InversionItem>(names[i], CHECKMARK(module->allowedInversions == inversions[i]));
					item->module = module;
					item->allowedInversions = inversions[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());
		OffsetMenu *offsetItem = createMenuItem<OffsetMenu>("Repeat Notes");
		offsetItem->module = galaxy;
		menu->addChild(offsetItem);

		ModeMenu *modeItem = createMenuItem<ModeMenu>("Chord Selection");
		modeItem->module = galaxy;
		menu->addChild(modeItem);

		InversionMenu *invItem = createMenuItem<InversionMenu>("Allowed Chord Inversions");
		invItem->module = galaxy;
		menu->addChild(invItem);

     }

};

Model *modelGalaxy = createModel<Galaxy, GalaxyWidget>("Galaxy");

// ♯♭
