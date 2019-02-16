#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct BombeChord : music::Chord {
	int mode = -1;
	int key = -1;
};

struct Bombe : core::AHModule {

	const static int NUM_PITCHES = 6;
	const static int N_NOTES = 12;
	const static int N_DEGREES = 7;
	const static int N_QUALITIES = 6;
	const static int QMAP_SIZE = 20;

	enum ParamIds {
		KEY_PARAM,
		MODE_PARAM,
		LENGTH_PARAM,
		X_PARAM,
		Y_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		KEY_INPUT,
		MODE_INPUT,
		FREEZE_INPUT,
		Y_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(PITCH_OUTPUT,6),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(LOCK_LIGHT,2),
		NUM_LIGHTS
	};
	
	Bombe() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

		params[KEY_PARAM].config(0.0, 11.0, 0.0, "Key");
		params[KEY_PARAM].description = "Key from which chords are selected"; 

		params[MODE_PARAM].config(0.0, 6.0, 0.0, "Mode"); 
		params[MODE_PARAM].description = "Mode from which chords are selected"; 

		params[LENGTH_PARAM].config(2.0, 16.0, 4.0, "Length of loop"); 
		params[X_PARAM].config(0.0, 1.0, 0.5, "Update probability", "%", 0.0f, -100.0f, 100.0f);
		params[X_PARAM].description = "Probability that the next chord will be changed";

		params[Y_PARAM].config(0.0, 1.0, 0.5, "Deviation probability", "%", 0.0f, 100.0f);
		params[Y_PARAM].description = "The deviation of the next chord update from the mode rule";

		for(int i = 0; i < BUFFERSIZE; i++) {
			buffer[i].setVoltages(music::defaultChord.formula, offset);
		}

		// knownChords.dump();
	}

	void step() override;
	void modeRandom(BombeChord lastValue, float y);
	void modeSimple(BombeChord lastValue, float y);
	void modeKey(BombeChord lastValue, float y);
	void modeGalaxy(BombeChord lastValue, float y);

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// polymode
		json_t *polymodeJ = json_boolean(polymode);
		json_object_set_new(rootJ, "polymode", polymodeJ);

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

		// polymode
		json_t *polymodeJ = json_object_get(rootJ, "polymode");
		if (polymodeJ)
			polymode = json_boolean_value(polymodeJ);

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

//	const static int N_CHORDS = 98;

//	int ChordMap[N_CHORDS] = {1,2,26,29,71,28,72,91,31,97,25,44,54,61,78,95,10,14,15,17,48,79,81,85,11,30,89,94,24,3,90,98,96,60,55,86,5,93,7,56,92,16,32,46,62,77,18,49,65,68,70,82,20,22,23,45,83,87,6,21,27,42,80,9,52,69,76,13,37,88,53,58,8,41,57,47,64,73,19,50,59,66,74,12,35,38,63,33,34,51,4,36,40,43,84,67,39,75};
 	int MajorScale[7] = {0,2,4,5,7,9,11};
	int Quality2Chord[N_QUALITIES] = { 0, 1, 54 }; // M, m, dim
	int QualityMap[3][QMAP_SIZE] = { 
		{00,00,00,00,00,00,00,00,00,00,07,07,07,07,07,07,07,07,06,06}, // M Maj7 7
		{01,01,01,01,01,01,01,01,01,01,38,38,38,38,38,38,38,38,06,06}, // m m7 7
		{54,54,54,54,54,54,54,54,54,54,56,56,56,56,56,56,56,56,56,56}  // dim dimM7
	};

	int InversionMap[3][QMAP_SIZE] = { 
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1},
		{0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,2,2},
	};
	
	int poll = 50000;

	rack::dsp::SchmittTrigger clockTrigger;

	bool polymode = false;

	int currRoot = 1;
	int currMode = 1;
	int currInversion = 0;
	int length = 16;

	int offset = 12; 	   // 0 = random, 12 = lower octave, 24 = repeat, 36 = upper octave
	int mode = 1; 	   // 0 = random chord, 1 = chord in key, 2 = chord in mode
	int allowedInversions = 0; // 0 = root only, 1 = root + first, 2 = root, first, second

	music::KnownChords knownChords;

	std::string rootName = "";
	std::string modeName = "";

	const static int BUFFERSIZE = 16;
	BombeChord buffer[BUFFERSIZE];
	BombeChord displayBuffer[BUFFERSIZE];

};

void Bombe::step() {
	
	core::AHModule::step();

	// Get inputs from Rack
	bool clocked = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage());
	bool locked = false;
	bool updated = false;

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

	float x = params[X_PARAM].getValue();
	float y = clamp(params[Y_PARAM].getValue() + inputs[Y_INPUT].getVoltage() * 0.1f, 0.0, 1.0);
	length = params[LENGTH_PARAM].getValue();

	if ((x >= 1.0f) || (inputs[FREEZE_INPUT].getVoltage() > 0.000001f)) {
		locked = true;
	}

	switch(mode) {
		case 0: // Random
			rootName = "";
			modeName = "";
			break;
		case 1: // Simple
			rootName = music::NoteDegreeModeNames[currRoot][0][currMode];
			modeName = music::modeNames[currMode];
			break;
		case 2: // Galaxy
			rootName = music::NoteDegreeModeNames[currRoot][0][currMode];
			modeName = music::modeNames[currMode];
			break;
		default:
			rootName = "";
			modeName = "";
	}

	if (clocked) {

		// Grab value from last element of sub-array, which will be the new head value
		BombeChord lastValue = buffer[length - 1];

		// Shift buffer
		for(int i = length - 1; i > 0; i--) {
			buffer[i] = buffer[i-1];
		}

		// Set first element
		if (locked) {
			// Buffer is locked
			buffer[0] = lastValue;
		} else {

			if (random::uniform() < x) {
				// Buffer update skipped
				buffer[0] = lastValue;
			} else {
				
				// We are going to update this entry 
				updated = true;

				switch(mode) {
					case 0:	modeRandom(lastValue, y); break;
					case 1:	modeSimple(lastValue, y); break;
					case 2:	modeGalaxy(lastValue, y); break;
					default: modeSimple(lastValue, y);
				}

				music::InversionDefinition &invDef = knownChords.chords[buffer[0].chord].inversions[buffer[0].inversion];
				buffer[0].setVoltages(invDef.formula, offset);

			}
		}

		for(int i = BUFFERSIZE - 1; i > 0; i--) {
			displayBuffer[i] = displayBuffer[i-1];
		}
		displayBuffer[0] = buffer[0];

		// for(int i = 0; i < BUFFERSIZE; i++) {
		// 	std::cout << displayBuffer[i].chord << ",";
		// }
		// std::cout << std::endl;

		// for(int i = 0; i < BUFFERSIZE; i++) {
		// 	std::cout << displayBuffer[i].rootNote << ",";
		// }
		// std::cout << std::endl << std::endl;
		
	}

	if (updated) { // Green Update
		lights[LOCK_LIGHT].setBrightnessSmooth(1.0f);
		lights[LOCK_LIGHT + 1].setBrightnessSmooth(0.0f);
	} else if (locked) { // Yellow locked
		lights[LOCK_LIGHT].setBrightnessSmooth(0.0f);
		lights[LOCK_LIGHT + 1].setBrightnessSmooth(1.0f);
	} else { // No change
		lights[LOCK_LIGHT].setBrightnessSmooth(0.0f);
		lights[LOCK_LIGHT + 1].setBrightnessSmooth(0.0f);
	}

	// Set the output pitches 
	if (polymode) {
		outputs[PITCH_OUTPUT].setChannels(6);
		outputs[PITCH_OUTPUT + 1].setChannels(6);
		for (int i = 0; i < NUM_PITCHES; i++) {
			outputs[PITCH_OUTPUT].setVoltage(buffer[0].outVolts[i], i);
			outputs[PITCH_OUTPUT + 1].setVoltage(10.0, i);
		}
	} else {
		for (int i = 0; i < NUM_PITCHES; i++) {
			outputs[PITCH_OUTPUT + i].setVoltage(buffer[0].outVolts[i]);
		}
	}
}

void Bombe::modeSimple(BombeChord lastValue, float y) {

	// Recalculate new value of buffer[0].outVolts from lastValue
	int shift = (rand() % (N_DEGREES - 1)) + 1; // 1 - 6 - always new chord
	buffer[0].modeDegree = (lastValue.modeDegree + shift) % N_DEGREES; // FIXME, come from mode2 modeDeg == -1!

	// quality 0 = Maj, 1 = Min, 2 = Dim
	music::getRootFromMode(currMode,currRoot,buffer[0].modeDegree,&(buffer[0].rootNote),&(buffer[0].quality));

	if (random::uniform() < y) {
		buffer[0].chord = QualityMap[buffer[0].quality][rand() % QMAP_SIZE]; // Get the index into the main chord table
	} else {
		buffer[0].chord = Quality2Chord[buffer[0].quality]; // Get the index into the main chord table
	}

	buffer[0].inversion = InversionMap[allowedInversions][rand() % QMAP_SIZE];
	buffer[0].key = currRoot;
	buffer[0].mode = currMode;

}

void Bombe::modeRandom(BombeChord lastValue, float y) {

	// Recalculate new value of buffer[0].outVolts from lastValue
	float p = random::uniform();
	if (p < y) {
		buffer[0].rootNote = rand() % 12; 
	} else {
		buffer[0].rootNote = MajorScale[rand() % 7]; 
	}

	buffer[0].modeDegree = -1; 
	buffer[0].quality = -1; 
	buffer[0].key = -1; 
	buffer[0].mode = -1; 

	float index = (float)(knownChords.chords.size()) * y;

	buffer[0].chord = rand() % std::max(2, (int)index); // Major and minor chords always allowed
	buffer[0].inversion = InversionMap[allowedInversions][rand() % QMAP_SIZE];

}

void Bombe::modeKey(BombeChord lastValue, float y) {

	int shift = (rand() % (N_DEGREES - 1)) + 1; // 1 - 6 - always new chord
	buffer[0].modeDegree = (lastValue.modeDegree + shift) % N_DEGREES; // FIXME, come from mode2 modeDeg == -1!

	music::getRootFromMode(currMode,currRoot,buffer[0].modeDegree,&(buffer[0].rootNote),&(buffer[0].quality));

	buffer[0].chord = (rand() % (knownChords.chords.size() - 1)); // Get the index into the main chord table
	buffer[0].inversion = InversionMap[allowedInversions][rand() % QMAP_SIZE];
	buffer[0].key = currRoot;
	buffer[0].mode = currMode;

}

void Bombe::modeGalaxy(BombeChord lastValue, float y) {

	float excess = y - random::uniform();

	if (excess < 0.0) {
		modeSimple(lastValue, y);
	} else {
		if (excess < 0.2) {
			modeKey(lastValue, y);
		} else {
			modeRandom(lastValue, y);
		}
	}

}

struct BombeDisplay : TransparentWidget {
	
	Bombe *module;
	std::shared_ptr<Font> font;

	BombeDisplay() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/EurostileBold.ttf"));
	}

	void draw(const DrawContext &ctx) override {
	
		nvgFontSize(ctx.vg, 14);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
		nvgTextLetterSpacing(ctx.vg, -1);

		char text[128];

		for (int i = 0; i < 7; i++)  {

			std::string chordName = "";
			std::string chordExtName = "";

			BombeChord &bC = module->displayBuffer[i];

			music::InversionDefinition &invDef = module->knownChords.chords[bC.chord].inversions[bC.inversion];

			if (bC.key != -1 && bC.mode != -1) {
				chordName = invDef.getName(bC.mode, bC.key, bC.modeDegree, bC.rootNote);
			} else {
				chordName = invDef.getName(bC.rootNote);
			}

			if (bC.modeDegree != -1 && bC.quality != -1) { 
				chordExtName = music::degreeNames[bC.modeDegree * 3 + bC.quality];
			}

			snprintf(text, sizeof(text), "%s %s", chordName.c_str(), chordExtName.c_str());
			nvgText(ctx.vg, box.pos.x + 5, box.pos.y + i * 14, text, NULL);
			nvgFillColor(ctx.vg, nvgRGBA(0, 255, 255, 223 - i * 32));

		}

		nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));

		nvgTextAlign(ctx.vg, NVG_ALIGN_RIGHT);
		snprintf(text, sizeof(text), "%s", module->rootName.c_str());
		nvgText(ctx.vg, box.size.x - 5, box.pos.y, text, NULL);

		snprintf(text, sizeof(text), "%s", module->modeName.c_str());
		nvgText(ctx.vg, box.size.x - 5, box.pos.y + 11, text, NULL);

	}
	
};

struct BombeWidget : ModuleWidget {

	BombeWidget(Bombe *module)  {
	
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Bombe.svg")));

		for (int i = 0; i < 6; i++) {
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, i, 5, true, false), module, Bombe::PITCH_OUTPUT + i));
		}	

		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 0, 4, true, false), module, Bombe::KEY_PARAM)); 
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 4, true, false), module, Bombe::KEY_INPUT));

		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 2, 4, true, false), module, Bombe::MODE_PARAM)); 
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 3, 4, true, false), module, Bombe::MODE_INPUT));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 4, 4, true, false), module, Bombe::CLOCK_INPUT));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 5, 4, true, false), module, Bombe::LENGTH_PARAM)); 

		Vec XParamPos;
		XParamPos.x = 33;
		XParamPos.y = 160;
		addParam(createParam<gui::AHBigKnobNoSnap>(XParamPos, module, Bombe::X_PARAM)); 

		Vec XFreezePos;
		XFreezePos.x = XParamPos.x - 12;
		XFreezePos.y = XParamPos.y + 60;
		addInput(createInput<PJ301MPort>(XFreezePos, module, Bombe::FREEZE_INPUT));

		Vec XLightPos;
		XLightPos.x = XParamPos.x + 63;
		XLightPos.y = XParamPos.y + 68;
		addChild(createLight<MediumLight<GreenRedLight>>(XLightPos, module, Bombe::LOCK_LIGHT));

		Vec YParamPos;
		YParamPos.x = 137;
		YParamPos.y = 160;
		addParam(createParam<gui::AHBigKnobNoSnap>(YParamPos, module, Bombe::Y_PARAM)); 

		Vec YInputPos;
		YInputPos.x = YParamPos.x - 12;
		YInputPos.y = YParamPos.y + 60;
		addInput(createInput<PJ301MPort>(YInputPos, module, Bombe::Y_INPUT));

		if (module != NULL) {
			BombeDisplay *displayW = createWidget<BombeDisplay>(Vec(0, 20));
			displayW->box.size = Vec(240, 230);
			displayW->module = module;
			addChild(displayW);
		}

	}

	void appendContextMenu(Menu *menu) override {

		Bombe *bombe = dynamic_cast<Bombe*>(module);
		assert(bombe);

		struct PolyModeItem : MenuItem {
			Bombe *module;
			bool polymode;
			void onAction(const event::Action &e) override {
				module->polymode = polymode;
			}
		};

		struct OffsetItem : MenuItem {
			Bombe *module;
			int offset;
			void onAction(const event::Action &e) override {
				module->offset = offset;
			}
		};

		struct ModeItem : MenuItem {
			Bombe *module;
			int mode;
			void onAction(const event::Action &e) override {
				module->mode = mode;
			}
		};

		struct InversionItem : MenuItem {
			Bombe *module;
			int allowedInversions;
			void onAction(const event::Action &e) override {
				module->allowedInversions = allowedInversions;
			}
		};

		struct PolyModeMenu : MenuItem {
			Bombe *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<bool> modes = {true, false};
				std::vector<std::string> names = {"Poly", "Mono"};
				for (size_t i = 0; i < modes.size(); i++) {
					PolyModeItem *item = createMenuItem<PolyModeItem>(names[i], CHECKMARK(module->polymode == modes[i]));
					item->module = module;
					item->polymode = modes[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		struct OffsetMenu : MenuItem {
			Bombe *module;
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
			Bombe *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<int> modes = {0, 1, 2};
				std::vector<std::string> names = {"Random", "Simple", "Galaxy"};
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
			Bombe *module;
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
		PolyModeMenu *polymodeItem = createMenuItem<PolyModeMenu>("Polyphony");
		polymodeItem->module = bombe;
		menu->addChild(polymodeItem);

		OffsetMenu *offsetItem = createMenuItem<OffsetMenu>("Repeat Notes");
		offsetItem->module = bombe;
		menu->addChild(offsetItem);

		ModeMenu *modeItem = createMenuItem<ModeMenu>("Chord Selection");
		modeItem->module = bombe;
		menu->addChild(modeItem);

		InversionMenu *invItem = createMenuItem<InversionMenu>("Allowed Chord Inversions");
		invItem->module = bombe;
		menu->addChild(invItem);

     }

};

Model *modelBombe = createModel<Bombe, BombeWidget>("Bombe");

// ♯♭
