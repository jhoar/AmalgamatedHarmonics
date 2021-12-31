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
		configParam(KEY_PARAM, 0.0, 11.0, 0.0, "Key");
		paramQuantities[KEY_PARAM]->description = "Key from which chords are selected"; 

		configParam(MODE_PARAM, 0.0, 6.0, 0.0, "Mode");
		paramQuantities[MODE_PARAM]->description = "Mode from which chord are selected"; 

		configParam(BAD_PARAM, 0.0, 1.0, 0.0, "Bad", "%", 0.0f, 100.0f);
		paramQuantities[BAD_PARAM]->description = "Deviation from chord selection rule for the mode";

	}

	void process(const ProcessArgs &args) override;

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

		// voltscale
		json_t *scaleModeJ = json_integer((int) voltScale);
		json_object_set_new(rootJ, "voltscale", scaleModeJ);

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

		// voltscale
		json_t *scaleModeJ = json_object_get(rootJ, "voltscale");
		if (scaleModeJ) voltScale = (music::RootScaling)json_integer_value(scaleModeJ);

	}

	int GalaxyChords[N_QUALITIES] = { 0, 2, 83, 12, 1, 29 }; // M, 7, m7, M7, m, dim
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

	int poll = 50000;

	rack::dsp::SchmittTrigger moveTrigger;

	music::Chord currChord;

	music::KnownChords knownChords;

	music::RootScaling voltScale = music::RootScaling::CIRCLE;

	int lastQuality = 0;
	int lastNoteIndex = 0; 
	int lastInversion = 0;

	int currRoot = 1;
	int currMode = 1;
	int light = 0;

	bool haveRoot = false;
	bool haveMode = false;

	int offset = 12;			// 0 = random, 12 = lower octave, 24 = repeat, 36 = upper octave
	int mode = 1;				// 0 = random chord, 1 = chord in key, 2 = chord in mode
	int allowedInversions = 0;	// 0 = root only, 1 = root + first, 2 = root, first, second

	std::string rootName = "";
	std::string modeName = "";

	std::string chordName = "";
	std::string chordExtName = "";
};

void Galaxy::process(const ProcessArgs &args) {

	AHModule::step();

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
		float v = inputs[KEY_INPUT].getVoltage();
		if (voltScale == music::RootScaling::CIRCLE) {
			currRoot = music::getKeyFromVolts(v);
		} else {
			int intv;
			music::getPitchFromVolts(v, music::Notes::NOTE_C, music::Scales::SCALE_CHROMATIC, &currRoot, &intv);
		}
	} else {
		currRoot = params[KEY_PARAM].getValue();
	}

	if (mode == 1) {
		rootName = music::noteNames[currRoot];
		modeName = "";
	} else if (mode == 2) {
		rootName = music::NoteDegreeModeNames[currRoot][0][currMode];
		modeName = music::modeNames[currMode];
	} else {
		rootName = "";
		modeName = "";
		chordExtName = "";
	}

	if (move) {

		bool changed = false;
		bool haveMode = false;

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

		currChord.inversion = InversionMap[allowedInversions][rand() % QMAP_SIZE];
		currChord.chord = GalaxyChords[currChord.quality];
		const ah::music::InversionDefinition & invDef = knownChords.getChord(currChord);
	
		currChord.setVoltages(invDef.formula, offset);

		if (currChord.quality != lastQuality) {
			changed = true;
			lastQuality = currChord.quality;
		}

		if (currChord.rootNote != lastNoteIndex) {
			changed = true;
			lastNoteIndex = currChord.rootNote;
		}

		if (currChord.inversion != lastInversion) {
			changed = true;
			lastInversion = currChord.inversion;
		}

		int newlight = currChord.rootNote + (currChord.quality * N_NOTES);

		if (changed) {

			if (mode == 2) {
				if (haveMode) {
					chordName = invDef.getName(currMode, currRoot, currChord.modeDegree, currChord.rootNote);
					chordExtName = degNames[currChord.modeDegree * 6 + currChord.quality];
				} else {
					chordName = invDef.getName(currChord.rootNote);
					chordExtName = "";
				} 
			} else {
				chordName = invDef.getName(currChord.rootNote);
				chordExtName = "";
			}

			lights[NOTE_LIGHT + light].setBrightness(0.0f);
			lights[NOTE_LIGHT + newlight].setBrightness(10.0f);
			light = newlight;

		}

	}

	if (badLight == 1) { // Green (scale->key)
		lights[BAD_LIGHT].setSmoothBrightness(1.0f, args.sampleTime);
		lights[BAD_LIGHT + 1].setSmoothBrightness(0.0f, args.sampleTime);
	} else if (badLight == 2) { // Red (->random)
		lights[BAD_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		lights[BAD_LIGHT + 1].setSmoothBrightness(1.0f, args.sampleTime);
	} else { // No change
		lights[BAD_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		lights[BAD_LIGHT + 1].setSmoothBrightness(0.0f, args.sampleTime);
	}

	// Set the output pitches 
	outputs[PITCH_OUTPUT].setChannels(6);
	for (int i = 0; i < NUM_PITCHES; i++) {
		outputs[PITCH_OUTPUT].setVoltage(currChord.outVolts[i], i);
		outputs[PITCH_OUTPUT + i].setVoltage(currChord.outVolts[i]);
	}

}

signed short rndSign() {
	return rand() % 2 ? 1 : -1;
}

signed int signedRndNotZero(signed int magntiude) {
	return rndSign() * (rand() % abs(magntiude) + 1); 
}

void Galaxy::getFromRandom() {

	int rotateInput = signedRndNotZero(2);
	int radialInput = signedRndNotZero(2);

	if(debugEnabled(5000)) {
		std::cout << "Rotate: " << rotateInput << "  Radial: " << radialInput << std::endl;
	}

	// Determine move around the grid
	currChord.quality += rotateInput;
	currChord.quality = eucMod(currChord.quality, N_QUALITIES);

	currChord.rootNote += radialInput;
	currChord.rootNote = eucMod(currChord.rootNote, N_NOTES);

}

void Galaxy::getFromKey() {

	int rotateInput = signedRndNotZero(2);
	int radialInput = signedRndNotZero(2);

	if(debugEnabled(5000)) {
		std::cout << "Rotate: " << rotateInput << "  Radial: " << radialInput << std::endl;
	}

	// Determine move around the grid
	currChord.quality += rotateInput;
	currChord.quality = eucMod(currChord.quality, N_QUALITIES);

	// Just major scale
	int *curScaleArr = music::ASCALE_IONIAN;
	int notesInScale = LENGTHOF(music::ASCALE_IONIAN);

	// Determine move through the scale
	currChord.modeDegree += radialInput; 
	currChord.modeDegree = eucMod(currChord.modeDegree, notesInScale);

	currChord.rootNote = (currRoot + curScaleArr[currChord.modeDegree]) % 12;

}

void Galaxy::getFromKeyMode() {

	int rotateInput = signedRndNotZero(2);

	// Determine move through the scale
	currChord.modeDegree += rotateInput;
	currChord.modeDegree = eucMod(currChord.modeDegree, music::Degrees::NUM_DEGREES);

	// From the input root, mode and degree, we can get the root chord note and quality (Major,Minor,Diminshed)
	int q;
	music::getRootFromMode(currMode,currRoot,currChord.modeDegree,&(currChord.rootNote),&q);
	currChord.quality = QualityMap[q][rand() % QMAP_SIZE];

}

struct GalaxyDisplay : TransparentWidget {
	
	Galaxy *module;
	std::shared_ptr<Font> font;

	GalaxyDisplay() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/EurostileBold.ttf"));
	}

	void draw(const DrawArgs &ctx) override {

		if (module == NULL) {
			return;
		}
	
		nvgFontSize(ctx.vg, 12);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
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

	std::vector<MenuOption<int>> offsetOptions;
	std::vector<MenuOption<int>> modeOptions;
	std::vector<MenuOption<int>> invOptions;
	std::vector<MenuOption<music::RootScaling>> scalingOptions;

	GalaxyWidget(Galaxy *module)  {
	
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Galaxy.svg")));

        addInput(createInput<gui::AHPort>(Vec(102, 140), module, Galaxy::MOVE_INPUT));

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

		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(219.355, 253.075), module, Galaxy::BAD_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(81.659, 292.558), module, Galaxy::KEY_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(206.276, 292.558), module, Galaxy::MODE_PARAM));

		addInput(createInputCentered<gui::AHPort>(Vec(40.253, 292.558), module, Galaxy::KEY_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(164.87, 292.558), module, Galaxy::MODE_INPUT));

		addOutput(createOutputCentered<gui::AHPort>(Vec(30.588, 341.133), module, Galaxy::PITCH_OUTPUT + 0));
		addOutput(createOutputCentered<gui::AHPort>(Vec(67.123, 340.899), module, Galaxy::PITCH_OUTPUT + 1));
		addOutput(createOutputCentered<gui::AHPort>(Vec(103.659, 340.899), module, Galaxy::PITCH_OUTPUT + 2));
		addOutput(createOutputCentered<gui::AHPort>(Vec(140.194, 341.141), module, Galaxy::PITCH_OUTPUT + 3));
		addOutput(createOutputCentered<gui::AHPort>(Vec(176.729, 341.141), module, Galaxy::PITCH_OUTPUT + 4));
		addOutput(createOutputCentered<gui::AHPort>(Vec(213.265, 341.141), module, Galaxy::PITCH_OUTPUT + 5));

        addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(233.746, 264.651), module, Galaxy::BAD_LIGHT));

		if (module != NULL) {
			GalaxyDisplay *displayW = createWidget<GalaxyDisplay>(Vec(0, 20));
			displayW->box.size = Vec(240, 230);
			displayW->module = module;
			addChild(displayW);
		}

		offsetOptions.emplace_back(std::string("Lower"), 12);
		offsetOptions.emplace_back(std::string("Repeat"), 24);
		offsetOptions.emplace_back(std::string("Upper"), 36);
		offsetOptions.emplace_back(std::string("Random"), 0);

		modeOptions.emplace_back(std::string("Random"), 0);
		modeOptions.emplace_back(std::string("in Key"), 1);
		modeOptions.emplace_back(std::string("in Mode"), 2);

		invOptions.emplace_back(std::string("Root only"), 0);
		invOptions.emplace_back(std::string("Root and First"), 1);
		invOptions.emplace_back(std::string("Root, First and Second"), 2);

		scalingOptions.emplace_back(std::string("V/Oct"), music::RootScaling::VOCT);
		scalingOptions.emplace_back(std::string("Fourths and Fifths"), music::RootScaling::CIRCLE);

	}

	void appendContextMenu(Menu *menu) override {

		Galaxy *galaxy = dynamic_cast<Galaxy*>(module);
		assert(galaxy);

		struct GalaxyMenu : MenuItem {
			Galaxy *module;
			GalaxyWidget *parent;
		};

		struct OffsetItem : GalaxyMenu {
			int offset;
			void onAction(const rack::event::Action &e) override {
				module->offset = offset;
			}
		};

		struct OffsetMenu : GalaxyMenu {
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

		struct ModeItem : GalaxyMenu {
			int mode;
			void onAction(const rack::event::Action &e) override {
				module->mode = mode;
			}
		};

		struct ModeMenu : GalaxyMenu {
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

		struct InversionItem : GalaxyMenu {
			int allowedInversions;
			void onAction(const rack::event::Action &e) override {
				module->allowedInversions = allowedInversions;
			}
		};

		struct InversionMenu : GalaxyMenu {
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

		struct ScalingItem : GalaxyMenu {
			music::RootScaling voltScale;
			void onAction(const rack::event::Action &e) override {
				module->voltScale = voltScale;
			}
		};

		struct ScalingMenu : GalaxyMenu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->scalingOptions) {
					ScalingItem *item = createMenuItem<ScalingItem>(opt.name, CHECKMARK(module->voltScale == opt.value));
					item->module = module;
					item->voltScale = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};


		menu->addChild(construct<MenuLabel>());
		OffsetMenu *offsetItem = createMenuItem<OffsetMenu>("Repeat Notes");
		offsetItem->module = galaxy;
		offsetItem->parent = this;
		menu->addChild(offsetItem);

		ModeMenu *modeItem = createMenuItem<ModeMenu>("Chord Selection");
		modeItem->module = galaxy;
		modeItem->parent = this;
		menu->addChild(modeItem);

		InversionMenu *invItem = createMenuItem<InversionMenu>("Allowed Chord Inversions");
		invItem->module = galaxy;
		invItem->parent = this;
		menu->addChild(invItem);

		ScalingMenu *scaleItem = createMenuItem<ScalingMenu>("Root Volt Scaling");
		scaleItem->module = galaxy;
		scaleItem->parent = this;
		menu->addChild(scaleItem);

	}

};

Model *modelGalaxy = createModel<Galaxy, GalaxyWidget>("Galaxy");

// ♯♭
