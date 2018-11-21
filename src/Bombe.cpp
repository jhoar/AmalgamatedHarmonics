#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"
#include "dsp/digital.hpp"

#include <iostream>

struct BombeChord {
	int rootNote;
	int quality;
	int chord;
	int modeDegree; // -1 no mode
	int inversion;
	float outVolts[6];
};

struct Bombe : AHModule {

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
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(PITCH_OUTPUT,6),
		NUM_OUTPUTS
	};
	enum LightIds {
		LOCK_LIGHT,
		NUM_LIGHTS
	};
	
	Bombe() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		for(int i = 0; i < BUFFERSIZE; i++) {
			buffer[i].modeDegree = 0;
			buffer[i].rootNote = 0;
			buffer[i].chord = 0;
			buffer[i].quality = 0;
			buffer[i].inversion = 0;
			for (int j = 0; j < NUM_PITCHES; j++) {
				buffer[i].outVolts[j] = 0.0f;
			}
		}
	}
	void step() override;

	json_t *toJson() override {
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
	
	void fromJson(json_t *rootJ) override {

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

	Core core;

	int Quality2Chord[N_QUALITIES] = { 1, 71, 91 }; // M, m, dim

	int InversionMap[3][QMAP_SIZE] = { 
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1},
		{0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,2,2},
	};
	
	int poll = 50000;

	SchmittTrigger clockTrigger;

	int currRoot = 1;
	int currMode = 1;
	int currInversion = 0;

	int offset = 12; 	   // 0 = random, 12 = lower octave, 24 = repeat, 36 = upper octave
	int mode = 1; 	   // 0 = random chord, 1 = chord in key, 2 = chord in mode
	int allowedInversions = 0; // 0 = root only, 1 = root + first, 2 = root, first, second

	std::string rootName = "";
	std::string modeName = "";

	const static int BUFFERSIZE = 16;
	BombeChord buffer[BUFFERSIZE];

};

void Bombe::step() {
	
	AHModule::step();

	// Get inputs from Rack
	bool clocked = clockTrigger.process(inputs[CLOCK_INPUT].value);

	if (inputs[MODE_INPUT].active) {
		float fMode = inputs[MODE_INPUT].value;
		currMode = CoreUtil().getModeFromVolts(fMode);
	} else {
		currMode = params[MODE_PARAM].value;
	}

	if (inputs[KEY_INPUT].active) {
		float fRoot = inputs[KEY_INPUT].value;
		currRoot = CoreUtil().getKeyFromVolts(fRoot);
	} else {
		currRoot = params[KEY_PARAM].value;
	}

	float x = params[X_PARAM].value;
	float y = params[Y_PARAM].value;
	int length = params[LENGTH_PARAM].value;

	bool locked = (x >= 1.0f);

	if (mode == 1) {
		rootName = CoreUtil().noteNames[currRoot];
		modeName = "";
	} else if (mode == 2) {
		rootName = CoreUtil().noteNames[currRoot];
		modeName = CoreUtil().modeNames[currMode];
	} else {
		rootName = "";
		modeName = "";
	}

	if (clocked) {

		// Grab value from last element of sub-array
		BombeChord lastValue = buffer[length - 1];

		// Shift buffer
		for(int i = length; i > 0; i--) {
			buffer[i] = buffer[i-1];
		}

		// Set first element
		if (locked) {
			// Buffer is locked, so just copy last value
			buffer[0] = lastValue;
		} else {
			// Recalculate new value of buffer[0].outVolts
			int shift = (rand() % (N_DEGREES - 1)) + 1; // 1 - 6 - always new chord
			buffer[0].modeDegree = (buffer[0].modeDegree + shift) % N_DEGREES;

			int q;
			int n;
			CoreUtil().getRootFromMode(currMode,currRoot,buffer[0].modeDegree,&n,&q);
			buffer[0].rootNote = n;
			buffer[0].quality = q; // 0 = Maj, 1 = Min, 2 = Dim
			buffer[0].chord = Quality2Chord[q]; // Get the index into the main chord table
			buffer[0].inversion = InversionMap[allowedInversions][rand() % QMAP_SIZE];

			// Determine which chord corresponds to the grid position
			int *chordArray;
			switch(buffer[0].inversion) {
				case 0: 	chordArray = CoreUtil().ChordTable[buffer[0].chord].root; 	break;
				case 1: 	chordArray = CoreUtil().ChordTable[buffer[0].chord].first; 	break;
				case 2: 	chordArray = CoreUtil().ChordTable[buffer[0].chord].second;	break;
				default: 	chordArray = CoreUtil().ChordTable[buffer[0].chord].root;
			}

			// Determine which notes corresponds to the chord
			for (int j = 0; j < NUM_PITCHES; j++) {
				if (chordArray[j] < 0) {
					int off = offset;
					if (offset == 0) { // if offset = 0, randomise offset per note
						off = (rand() % 3 + 1) * 12;
					}
					buffer[0].outVolts[j] = CoreUtil().getVoltsFromPitch(chordArray[j] + off, buffer[0].rootNote);			
				} else {
					buffer[0].outVolts[j] = CoreUtil().getVoltsFromPitch(chordArray[j], buffer[0].rootNote);			
				}	
			}
		}
	}

	if (locked) {
		lights[LOCK_LIGHT].setBrightnessSmooth(1.0f);
	} else {
		lights[LOCK_LIGHT].setBrightnessSmooth(0.0f);
	}

	// Set the output pitches and lights
	for (int i = 0; i < NUM_PITCHES; i++) {
		outputs[PITCH_OUTPUT + i].value = buffer[0].outVolts[i];
	}

}

struct BombeDisplay : TransparentWidget {
	
	Bombe *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	BombeDisplay() {
		font = Font::load(assetPlugin(plugin, "res/EurostileBold.ttf"));
	}

	void draw(NVGcontext *vg) override {
	
		nvgFontSize(vg, 12);
		nvgFontFaceId(vg, font->handle);
		nvgFillColor(vg, nvgRGBA(255, 0, 0, 0xff));
		nvgTextLetterSpacing(vg, -1);

		char text[128];

		std::string chordName = 
			CoreUtil().noteNames[module->buffer[0].rootNote] + 
			CoreUtil().ChordTable[module->buffer[0].chord].quality + " " + 
			CoreUtil().inversionNames[module->buffer[0].inversion];

		std::string chordExtName = "";

		if (module->mode == 2) {
			int index = module->buffer[0].modeDegree * 3 + module->buffer[0].quality;
			chordExtName = CoreUtil().degreeNames[index];
		}

		snprintf(text, sizeof(text), "%s", chordName.c_str());
		nvgText(vg, box.pos.x + 5, box.pos.y, text, NULL);

		snprintf(text, sizeof(text), "%s", chordExtName.c_str());
		nvgText(vg, box.pos.x + 5, box.pos.y + 11, text, NULL);

		nvgTextAlign(vg, NVG_ALIGN_RIGHT);
		snprintf(text, sizeof(text), "%s", module->rootName.c_str());
		nvgText(vg, box.size.x - 5, box.pos.y, text, NULL);

		snprintf(text, sizeof(text), "%s", module->modeName.c_str());
		nvgText(vg, box.size.x - 5, box.pos.y + 11, text, NULL);

	}
	
};

struct BombeWidget : ModuleWidget {

	Menu *createContextMenu() override;

	BombeWidget(Bombe *module) : ModuleWidget(module) {
	
		UI ui;
		
		box.size = Vec(240, 380);

		{
			SVGPanel *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/Bombe.svg")));
			addChild(panel);
		}

		addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 30, 365)));

		{
			BombeDisplay *display = new BombeDisplay();
			display->module = module;
			display->box.pos = Vec(0, 20);
			display->box.size = Vec(240, 230);
			addChild(display);
		}

		for (int i = 0; i < 6; i++) {
			addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, i, 5, true, false), Port::OUTPUT, module, Bombe::PITCH_OUTPUT + i));
		}	

		addParam(ParamWidget::create<AHKnobSnap>(ui.getPosition(UI::KNOB, 0, 4, true, false), module, Bombe::KEY_PARAM, 0.0, 11.0, 0.0)); 
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 1, 4, true, false), Port::INPUT, module, Bombe::KEY_INPUT));

		addParam(ParamWidget::create<AHKnobSnap>(ui.getPosition(UI::KNOB, 2, 4, true, false), module, Bombe::MODE_PARAM, 0.0, 6.0, 0.0)); 
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 3, 4, true, false), Port::INPUT, module, Bombe::MODE_INPUT));

		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 4, 4, true, false), Port::INPUT, module, Bombe::CLOCK_INPUT));
		addParam(ParamWidget::create<AHKnobSnap>(ui.getPosition(UI::KNOB, 5, 4, true, false), module, Bombe::LENGTH_PARAM, 2.0, 16.0, 0.0)); 

		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 0, 3, true, false), module, Bombe::X_PARAM, 0.0, 1.001, 0.0)); 
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 5, 3, true, false), module, Bombe::Y_PARAM, 0.0, 1.001, 0.0)); 

		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(ui.getPosition(UI::LIGHT, 1, 3, true, false), module, Bombe::LOCK_LIGHT));

	}

};

struct BombeOffsetItem : MenuItem {
	Bombe *bombe;
	int offset;
	void onAction(EventAction &e) override {
		bombe->offset = offset;
	}
	void step() override {
		rightText = (bombe->offset == offset) ? "✔" : "";
	}
};

struct BombeModeItem : MenuItem {
	Bombe *bombe;
	int mode;
	void onAction(EventAction &e) override {
		bombe->mode = mode;
	}
	void step() override {
		rightText = (bombe->mode == mode) ? "✔" : "";
	}
};

struct BombeInversionsItem : MenuItem {
	Bombe *bombe;
	int allowedInversions;
	void onAction(EventAction &e) override {
		bombe->allowedInversions = allowedInversions;
	}
	void step() override {
		rightText = (bombe->allowedInversions == allowedInversions) ? "✔" : "";
	}
};

Menu *BombeWidget::createContextMenu() {
	Menu *menu = ModuleWidget::createContextMenu();

	MenuLabel *spacerLabel = new MenuLabel();
	menu->addChild(spacerLabel);

	Bombe *bombe = dynamic_cast<Bombe*>(module);
	assert(bombe);

	MenuLabel *offsetLabel = new MenuLabel();
	offsetLabel->text = "Repeat Notes";
	menu->addChild(offsetLabel);

	BombeOffsetItem *offsetLowerItem = new BombeOffsetItem();
	offsetLowerItem->text = "Lower";
	offsetLowerItem->bombe = bombe;
	offsetLowerItem->offset = 12;
	menu->addChild(offsetLowerItem);

	BombeOffsetItem *offsetRepeatItem = new BombeOffsetItem();
	offsetRepeatItem->text = "Repeat";
	offsetRepeatItem->bombe = bombe;
	offsetRepeatItem->offset = 24;
	menu->addChild(offsetRepeatItem);

	BombeOffsetItem *offsetUpperItem = new BombeOffsetItem();
	offsetUpperItem->text = "Upper";
	offsetUpperItem->bombe = bombe;
	offsetUpperItem->offset = 36;
	menu->addChild(offsetUpperItem);

	BombeOffsetItem *offsetRandomItem = new BombeOffsetItem();
	offsetRandomItem->text = "Random";
	offsetRandomItem->bombe = bombe;
	offsetRandomItem->offset = 0;
	menu->addChild(offsetRandomItem);

	MenuLabel *modeLabel = new MenuLabel();
	modeLabel->text = "Chord Selection";
	menu->addChild(modeLabel);

	BombeModeItem *modeRandomItem = new BombeModeItem();
	modeRandomItem->text = "Random";
	modeRandomItem->bombe = bombe;
	modeRandomItem->mode = 0;
	menu->addChild(modeRandomItem);

	BombeModeItem *modeKeyItem = new BombeModeItem();
	modeKeyItem->text = "in Key";
	modeKeyItem->bombe = bombe;
	modeKeyItem->mode = 1;
	menu->addChild(modeKeyItem);

	BombeModeItem *modeModeItem = new BombeModeItem();
	modeModeItem->text = "in Mode";
	modeModeItem->bombe = bombe;
	modeModeItem->mode = 2;
	menu->addChild(modeModeItem);

	MenuLabel *invLabel = new MenuLabel();
	invLabel->text = "Allowed Chord Inversions";
	menu->addChild(invLabel);

	BombeInversionsItem *invRootItem = new BombeInversionsItem();
	invRootItem->text = "Root only";
	invRootItem->bombe = bombe;
	invRootItem->allowedInversions = 0;
	menu->addChild(invRootItem);

	BombeInversionsItem *invFirstItem = new BombeInversionsItem();
	invFirstItem->text = "Root and First";
	invFirstItem->bombe = bombe;
	invFirstItem->allowedInversions = 1;
	menu->addChild(invFirstItem);

	BombeInversionsItem *invSecondItem = new BombeInversionsItem();
	invSecondItem->text = "Root, First and Second";
	invSecondItem->bombe = bombe;
	invSecondItem->allowedInversions = 2;
	menu->addChild(invSecondItem);

	return menu;
}

Model *modelBombe = Model::create<Bombe, BombeWidget>( "Amalgamated Harmonics", "Bombe", "Bombe", SEQUENCER_TAG);

// ♯♭
