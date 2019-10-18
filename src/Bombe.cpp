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
	const static int BUFFERSIZE = 16;

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

		configParam(KEY_PARAM, 0.0, 11.0, 0.0, "Key");
		paramQuantities[KEY_PARAM]->description = "Key from which chords are selected"; 

		configParam(MODE_PARAM, 0.0, 6.0, 0.0, "Mode"); 
		paramQuantities[MODE_PARAM]->description = "Mode from which chords are selected"; 

		configParam(LENGTH_PARAM, 2.0, 16.0, 4.0, "Length of loop"); 
		configParam(X_PARAM, 0.0, 1.0, 0.5, "Update probability", "%", 0.0f, -100.0f, 100.0f);
		paramQuantities[X_PARAM]->description = "Probability that the next chord will be changed";

		configParam(Y_PARAM, 0.0, 1.0, 0.5, "Deviation probability", "%", 0.0f, 100.0f);
		paramQuantities[Y_PARAM]->description = "The deviation of the next chord update from the mode rule";

		for(int i = 0; i < BUFFERSIZE; i++) {
			buffer[i].setVoltages(music::defaultChord.formula, offset);
		}

	}

	void process(const ProcessArgs &args) override;
	void modeRandom(const BombeChord &lastValue, float y);
	void modeSimple(const BombeChord &lastValue, float y);
	void modeKey(const BombeChord &lastValue, float y);
	void modeGalaxy(const BombeChord &lastValue, float y);

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
		if (offsetJ) offset = json_integer_value(offsetJ);

		// mode
		json_t *modeJ = json_object_get(rootJ, "mode");
		if (modeJ) mode = json_integer_value(modeJ);

		// mode
		json_t *inversionsJ = json_object_get(rootJ, "inversions");
		if (inversionsJ) allowedInversions = json_integer_value(inversionsJ);

	}

 	int MajorScale[7] = {0,2,4,5,7,9,11};
	int Quality2Chord[N_QUALITIES] = {0, 1, 54}; // M, m, dim
	int QualityMap[3][QMAP_SIZE] = { 
		{00,00,00,00,00,00,00,00,00,00,07,07,07,07,07,07,07,07,06,06},	// M Maj7 7
		{01,01,01,01,01,01,01,01,01,01,38,38,38,38,38,38,38,38,06,06},	// m m7 7
		{54,54,54,54,54,54,54,54,54,54,56,56,56,56,56,56,56,56,56,56}	// dim dimM7
	};

	int InversionMap[3][QMAP_SIZE] = { 
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1},
		{0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,2,2},
	};

	rack::dsp::SchmittTrigger clockTrigger;

	int currRoot = 1;
	int currMode = 1;
	int currInversion = 0;
	int length = 16;

	int offset = 12; 			// 0 = random, 12 = lower octave, 24 = repeat, 36 = upper octave
	int mode = 1; 				// 0 = random chord, 1 = chord in key, 2 = chord in mode
	int allowedInversions = 0;	// 0 = root only, 1 = root + first, 2 = root, first, second

	music::KnownChords knownChords;

	std::string rootName;
	std::string modeName;

	BombeChord buffer[BUFFERSIZE];
	BombeChord displayBuffer[BUFFERSIZE];

};

void Bombe::process(const ProcessArgs &args) {

	AHModule::step();

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
		const BombeChord & lastValue = buffer[length - 1];

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
		lights[LOCK_LIGHT].setSmoothBrightness(1.0f, args.sampleTime);
		lights[LOCK_LIGHT + 1].setSmoothBrightness(0.0f, args.sampleTime);
	} else if (locked) { // Yellow locked
		lights[LOCK_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		lights[LOCK_LIGHT + 1].setSmoothBrightness(1.0f, args.sampleTime);
	} else { // No change
		lights[LOCK_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		lights[LOCK_LIGHT + 1].setSmoothBrightness(0.0f, args.sampleTime);
	}

	// Set the output pitches 
	outputs[PITCH_OUTPUT].setChannels(6);
	for (int i = 0; i < NUM_PITCHES; i++) {
		outputs[PITCH_OUTPUT].setVoltage(buffer[0].outVolts[i], i);
		outputs[PITCH_OUTPUT + i].setVoltage(buffer[0].outVolts[i]);
	}
}

void Bombe::modeSimple(const BombeChord & lastValue, float y) {

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

void Bombe::modeRandom(const BombeChord & lastValue, float y) {

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

void Bombe::modeKey(const BombeChord & lastValue, float y) {

	int shift = (rand() % (N_DEGREES - 1)) + 1; // 1 - 6 - always new chord
	buffer[0].modeDegree = (lastValue.modeDegree + shift) % N_DEGREES; // FIXME, come from mode2 modeDeg == -1!

	music::getRootFromMode(currMode,currRoot,buffer[0].modeDegree,&(buffer[0].rootNote),&(buffer[0].quality));

	buffer[0].chord = (rand() % (knownChords.chords.size() - 1)); // Get the index into the main chord table
	buffer[0].inversion = InversionMap[allowedInversions][rand() % QMAP_SIZE];
	buffer[0].key = currRoot;
	buffer[0].mode = currMode;

}

void Bombe::modeGalaxy(const BombeChord & lastValue, float y) {

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

	void draw(const DrawArgs &ctx) override {

		if (module == NULL) {
			return;
		}

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

			if (bC.modeDegree != -1 && bC.mode != -1) { 
				chordExtName = music::DegreeString[bC.mode][bC.modeDegree];
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

	std::vector<MenuOption<int>> offsetOptions;
	std::vector<MenuOption<int>> modeOptions;
	std::vector<MenuOption<int>> invOptions;

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

		offsetOptions.emplace_back(std::string("Lower"), 12);
		offsetOptions.emplace_back(std::string("Repeat"), 24);
		offsetOptions.emplace_back(std::string("Upper"), 36);
		offsetOptions.emplace_back(std::string("Random"), 0);

		modeOptions.emplace_back(std::string("Random"), 0);
		modeOptions.emplace_back(std::string("Simple"), 1);
		modeOptions.emplace_back(std::string("Galaxy"), 2);

		invOptions.emplace_back(std::string("Root only"), 0);
		invOptions.emplace_back(std::string("Root and First"), 1);
		invOptions.emplace_back(std::string("Root, First and Second"), 2);

	}

	void appendContextMenu(Menu *menu) override {

		Bombe *bombe = dynamic_cast<Bombe*>(module);
		assert(bombe);

		struct BombeMenu : MenuItem {
			Bombe *module;
			BombeWidget *parent;
		};

		struct OffsetItem : BombeMenu {
			int offset;
			void onAction(const rack::event::Action &e) override {
				module->offset = offset;
			}
		};

		struct OffsetMenu : BombeMenu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->offsetOptions) {
					OffsetItem *item = createMenuItem<OffsetItem>(opt.name, CHECKMARK(module->offset == opt.value));
					item->module = module;
					item->offset = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		struct ModeItem : BombeMenu {
			int mode;
			void onAction(const rack::event::Action &e) override {
				module->mode = mode;
			}
		};

		struct ModeMenu : BombeMenu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->modeOptions) {
					ModeItem *item = createMenuItem<ModeItem>(opt.name, CHECKMARK(module->mode == opt.value));
					item->module = module;
					item->mode = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		struct InversionItem : BombeMenu {
			int allowedInversions;
			void onAction(const rack::event::Action &e) override {
				module->allowedInversions = allowedInversions;
			}
		};

		struct InversionMenu : BombeMenu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->invOptions) {
					InversionItem *item = createMenuItem<InversionItem>(opt.name, CHECKMARK(module->allowedInversions == opt.value));
					item->module = module;
					item->allowedInversions = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());
		OffsetMenu *offsetItem = createMenuItem<OffsetMenu>("Repeat Notes");
		offsetItem->module = bombe;
		offsetItem->parent = this;
		menu->addChild(offsetItem);

		ModeMenu *modeItem = createMenuItem<ModeMenu>("Chord Selection");
		modeItem->module = bombe;
		modeItem->parent = this;
		menu->addChild(modeItem);

		InversionMenu *invItem = createMenuItem<InversionMenu>("Allowed Chord Inversions");
		invItem->module = bombe;
		invItem->parent = this;
		menu->addChild(invItem);

     }

};

Model *modelBombe = createModel<Bombe, BombeWidget>("Bombe");

// ♯♭
