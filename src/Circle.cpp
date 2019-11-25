#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct Circle : core::AHModule {

	enum ParamIds {
		KEY_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ROTL_INPUT,
		ROTR_INPUT,
		KEY_INPUT,
		MODE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		KEY_OUTPUT,
		MODE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(MODE_LIGHT,7),
		ENUMS(BKEY_LIGHT,12),
		ENUMS(CKEY_LIGHT,12),
		NUM_LIGHTS
	};

	music::RootScaling inVoltScale = music::RootScaling::CIRCLE;
	music::RootScaling outVoltScale = music::RootScaling::CIRCLE;
	
	Circle() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

		configParam(KEY_PARAM, 0.0, 11.0, 0.0, "Key"); 
		paramQuantities[KEY_PARAM]->description = "Starting key for progression";

		configParam(MODE_PARAM, 0.0, 6.0, 0.0, "Mode"); 
		paramQuantities[MODE_PARAM]->description = "Mode of progression";

	}

	void process(const ProcessArgs &args) override;
	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// inScale
		json_t *inScaleModeJ = json_integer((int) inVoltScale);
		json_object_set_new(rootJ, "scale", inScaleModeJ);

		// outVoltScale
		json_t *outScaleModeJ = json_integer((int) outVoltScale);
		json_object_set_new(rootJ, "outscale", outScaleModeJ);

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		// inScale
		json_t *inScaleModeJ = json_object_get(rootJ, "scale");
		if (inScaleModeJ) inVoltScale = (music::RootScaling)json_integer_value(inScaleModeJ);

		// outVoltScale
		json_t *outScaleModeJ = json_object_get(rootJ, "outscale");
		if (outScaleModeJ) outVoltScale = (music::RootScaling)json_integer_value(outScaleModeJ);
	}

	rack::dsp::SchmittTrigger rotLTrigger;
	rack::dsp::SchmittTrigger rotRTrigger;

	rack::dsp::PulseGenerator stepPulse;

	int baseKeyIndex = 0;
	int curKeyIndex = 0;
	int curMode = 0;

};

void Circle::process(const ProcessArgs &args) {

	AHModule::step();

	// Get inputs from Rack
	float rotLInput		= inputs[ROTL_INPUT].getVoltage();
	float rotRInput		= inputs[ROTR_INPUT].getVoltage();
	
	int newKeyIndex = 0;
	if (inputs[KEY_INPUT].isConnected()) {
		float fRoot = inputs[KEY_INPUT].getVoltage();
		if (inVoltScale == music::RootScaling::CIRCLE) {
			newKeyIndex = music::getKeyFromVolts(fRoot);
		} else {
			int deg;
			music::getPitchFromVolts(fRoot, music::Notes::NOTE_C, music::Scales::SCALE_CHROMATIC, &newKeyIndex, &deg);
		}
	} else {
		newKeyIndex = params[KEY_PARAM].getValue();
	}

	int newMode = 0;
	if (inputs[MODE_INPUT].isConnected()) {
		float fMode = inputs[MODE_INPUT].getVoltage();
		newMode = round(rescale(fabs(fMode), 0.0f, 10.0f, 0.0f, 6.0f)); 
	} else {
		newMode = params[MODE_PARAM].getValue();
	}

	curMode = newMode;

	// Process inputs
	bool rotLStatus		= rotLTrigger.process(rotLInput);
	bool rotRStatus		= rotRTrigger.process(rotRInput);

	if (rotLStatus) {
		if (debugEnabled()) { std::cout << stepX << " Rotate left: " << curKeyIndex; }
		if (inVoltScale == music::RootScaling::CIRCLE) {
			curKeyIndex = curKeyIndex ? 11 : curKeyIndex - 1; // Wrap
		} else {
			curKeyIndex = curKeyIndex + 5;
			if (curKeyIndex > 11) {
				curKeyIndex = curKeyIndex - 12;
			}
		}

		if (debugEnabled()) { std::cout << " -> " << curKeyIndex << std::endl;	}
	} 

	if (rotRStatus) {
		if (debugEnabled()) { std::cout << stepX << " Rotate right: " << curKeyIndex; }
		if (inVoltScale == music::RootScaling::CIRCLE) {
			curKeyIndex = curKeyIndex == 11 ? 0 : curKeyIndex + 1; // Wrap
		} else {
			curKeyIndex = curKeyIndex - 5;
			if (curKeyIndex < 0) {
				curKeyIndex = curKeyIndex + 12;
			}
		}
		if (debugEnabled()) { std::cout << " -> " << curKeyIndex << std::endl;	}
	} 

	if (rotLStatus && rotRStatus) {
		if (debugEnabled()) { std::cout << stepX << " Reset " << curKeyIndex << std::endl;	}
		curKeyIndex = baseKeyIndex;
	}

	if (newKeyIndex != baseKeyIndex) {
		if (debugEnabled()) { std::cout << stepX << " New base: " << newKeyIndex << std::endl;}
		baseKeyIndex = newKeyIndex;
		curKeyIndex = newKeyIndex;
	}

	int curKey;
	int baseKey;

	if (inVoltScale == music::RootScaling::CIRCLE) {
		curKey = music::CIRCLE_FIFTHS[curKeyIndex];
		baseKey = music::CIRCLE_FIFTHS[baseKeyIndex];
	} else {
		curKey = curKeyIndex;
		baseKey = baseKeyIndex;		
	}

	float keyVolts = music::getVoltsFromKey(curKey);
	if (outVoltScale == music::RootScaling::CIRCLE) {
		keyVolts = music::getVoltsFromKey(curKey);
	} else {
		keyVolts = curKey * music::SEMITONE;
	}

	float modeVolts = music::getVoltsFromMode(curMode);

	for (int i = 0; i < music::Notes::NUM_NOTES; i++) {
		lights[CKEY_LIGHT + i].setBrightness(0.0);
		lights[BKEY_LIGHT + i].setBrightness(0.0);
	}

	lights[CKEY_LIGHT + curKey].setBrightness(10.0);
	lights[BKEY_LIGHT + baseKey].setBrightness(10.0);

	for (int i = 0; i < music::Modes::NUM_MODES; i++) {
		lights[MODE_LIGHT + i].setBrightness(0.0);
	}
	lights[MODE_LIGHT + curMode].setBrightness(10.0);

	outputs[KEY_OUTPUT].setVoltage(keyVolts);
	outputs[MODE_OUTPUT].setVoltage(modeVolts);

}

struct CircleWidget : ModuleWidget {

	std::vector<MenuOption<music::RootScaling>> inScalingOptions;
	std::vector<MenuOption<music::RootScaling>> outScalingOptions;

	CircleWidget(Circle *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Circle.svg")));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 0, true, false), module, Circle::ROTL_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 0, true, false), module, Circle::ROTR_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 5, true, false), module, Circle::KEY_INPUT));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 1, 5, true, false), module, Circle::KEY_PARAM)); 
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 5, true, false), module, Circle::MODE_INPUT));
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 3, 5, true, false), module, Circle::MODE_PARAM)); 

		float div = (core::PI * 2) / 12.0;

		for (int i = 0; i < 12; i++) {

			float cosDiv = cos(div * i);
			float sinDiv = sin(div * i);

			float xPos  = sinDiv * 52.5;
			float yPos  = cosDiv * 52.5;
			float xxPos = sinDiv * 60.0;
			float yyPos = cosDiv * 60.0;

			addChild(createLight<SmallLight<GreenLight>>(Vec(xxPos + 116.5, 149.5 - yyPos), module, Circle::CKEY_LIGHT + music::CIRCLE_FIFTHS[i]));
			addChild(createLight<SmallLight<RedLight>>(Vec(xPos + 116.5, 149.5 - yPos), module, Circle::BKEY_LIGHT + music::CIRCLE_FIFTHS[i]));
		}

		for (int i = 0; i < 7; i++) {
			float xPos = 36.0 + i * 18.2;
			addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, 280.0), module, Circle::MODE_LIGHT + i));
		}

		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 4, 5, true, false), module, Circle::KEY_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 5, true, false), module, Circle::MODE_OUTPUT));

		inScalingOptions.emplace_back(std::string("V/Oct"), music::RootScaling::VOCT);
		inScalingOptions.emplace_back(std::string("Classic"), music::RootScaling::CIRCLE);

		outScalingOptions.emplace_back(std::string("V/Oct"), music::RootScaling::VOCT);
		outScalingOptions.emplace_back(std::string("Classic"), music::RootScaling::CIRCLE);

	}

	void appendContextMenu(Menu *menu) override {

		Circle *circle = dynamic_cast<Circle*>(module);
		assert(circle);

		struct CircleMenu : MenuItem {
			Circle *module;
			CircleWidget *parent;
		};

		struct InScalingItem : CircleMenu {
			music::RootScaling inVoltScale;
			void onAction(const rack::event::Action &e) override {
				module->inVoltScale = inVoltScale;
			}
		};

		struct InScalingMenu : CircleMenu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->inScalingOptions) {
					InScalingItem *item = createMenuItem<InScalingItem>(opt.name, CHECKMARK(module->inVoltScale == opt.value));
					item->module = module;
					item->inVoltScale = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		struct OutScalingItem : CircleMenu {
			music::RootScaling outVoltScale;
			void onAction(const rack::event::Action &e) override {
				module->outVoltScale = outVoltScale;
			}
		};

		struct OutScalingMenu : CircleMenu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->outScalingOptions) {
					OutScalingItem *item = createMenuItem<OutScalingItem>(opt.name, CHECKMARK(module->outVoltScale == opt.value));
					item->module = module;
					item->outVoltScale = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());

		InScalingMenu *inItem = createMenuItem<InScalingMenu>("Input Volt Scaling");
		inItem->module = circle;
		inItem->parent = this;
		menu->addChild(inItem);

		OutScalingMenu *outItem = createMenuItem<OutScalingMenu>("Output Volt Scaling");
		outItem->module = circle;
		outItem->parent = this;
		menu->addChild(outItem);

	}

};

Model *modelCircle = createModel<Circle, CircleWidget>("Circle");

// ♯♭
