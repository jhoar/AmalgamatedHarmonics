#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"
#include "dsp/digital.hpp"

#include <iostream>

struct Circle : AHModule {

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
	enum Scaling {
		CHROMATIC,
		FIFTHS
	};
	Scaling voltScale = FIFTHS;
	
	Circle() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

		params[KEY_PARAM].config(0.0, 11.0, 0.0); 
		params[MODE_PARAM].config(0.0, 6.0, 0.0); 

	}

	void step() override;
	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// gateMode
		json_t *scaleModeJ = json_integer((int) voltScale);
		json_object_set_new(rootJ, "scale", scaleModeJ);

		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override {
		// gateMode
		json_t *scaleModeJ = json_object_get(rootJ, "scale");
		
		if (scaleModeJ) {
			voltScale = (Scaling)json_integer_value(scaleModeJ);
		}
	}
	
	dsp::SchmittTrigger rotLTrigger;
	dsp::SchmittTrigger rotRTrigger;
	
	dsp::PulseGenerator stepPulse;
	
	int baseKeyIndex = 0;
	int curKeyIndex = 0;
	
	int curMode = 0;
		
	int poll = 50000;
		
};

void Circle::step() {
	
	AHModule::step();

	// Get inputs from Rack
	float rotLInput		= inputs[ROTL_INPUT].value;
	float rotRInput		= inputs[ROTR_INPUT].value;
	
	int newKeyIndex = 0;
	int deg;
	if (inputs[KEY_INPUT].active) {
		float fRoot = inputs[KEY_INPUT].value;
		if (voltScale == FIFTHS) {
			newKeyIndex = CoreUtil().getKeyFromVolts(fRoot);
		} else {
			CoreUtil().getPitchFromVolts(fRoot, Core::NOTE_C, Core::SCALE_CHROMATIC, &newKeyIndex, &deg);
		}
	} else {
		newKeyIndex = params[KEY_PARAM].value;
	}

	int newMode = 0;
	if (inputs[MODE_INPUT].active) {
		float fMode = inputs[MODE_INPUT].value;
		newMode = round(rescale(fabs(fMode), 0.0f, 10.0f, 0.0f, 6.0f)); 
	} else {
		newMode = params[MODE_PARAM].value;
	}

	curMode = newMode;
		
	// Process inputs
	bool rotLStatus		= rotLTrigger.process(rotLInput);
	bool rotRStatus		= rotRTrigger.process(rotRInput);
		
	if (rotLStatus) {
		if (debugEnabled()) { std::cout << stepX << " Rotate left: " << curKeyIndex; }
		if (voltScale == FIFTHS) {
			if (curKeyIndex == 0) {
				curKeyIndex = 11;
			} else {
				curKeyIndex--;
			}
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
		if (voltScale == FIFTHS) {
			if (curKeyIndex == 11) {
				curKeyIndex = 0;
			} else {
				curKeyIndex++;
			}
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

	if (voltScale == FIFTHS) {
		curKey = CoreUtil().CIRCLE_FIFTHS[curKeyIndex];
		baseKey = CoreUtil().CIRCLE_FIFTHS[baseKeyIndex];
	} else {
		curKey = curKeyIndex;
		baseKey = baseKeyIndex;		
	}


	float keyVolts = CoreUtil().getVoltsFromKey(curKey);
	float modeVolts = CoreUtil().getVoltsFromMode(curMode);
	
	for (int i = 0; i < Core::NUM_NOTES; i++) {
		lights[CKEY_LIGHT + i].value = 0.0;
		lights[BKEY_LIGHT + i].value = 0.0;
	}

	lights[CKEY_LIGHT + curKey].value = 1.0;
	lights[BKEY_LIGHT + baseKey].value = 1.0;
	
	for (int i = 0; i < Core::NUM_MODES; i++) {
		lights[MODE_LIGHT + i].value = 0.0;
	}
	lights[MODE_LIGHT + curMode].value = 1.0;
		
	outputs[KEY_OUTPUT].value = keyVolts;
	outputs[MODE_OUTPUT].value = modeVolts;
	
}

struct CircleWidget : ModuleWidget {

	CircleWidget(Circle *module) {
		
		setModule(module);
		setPanel(SVG::load(asset::plugin(pluginInstance, "res/Circle.svg")));
		UI ui;

		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 0, true, false), module, Circle::ROTL_INPUT));
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 5, 0, true, false), module, Circle::ROTR_INPUT));
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 5, true, false), module, Circle::KEY_INPUT));
		addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 1, 5, true, false), module, Circle::KEY_PARAM)); 
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 5, true, false), module, Circle::MODE_INPUT));
		addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 3, 5, true, false), module, Circle::MODE_PARAM)); 

		float div = (PI * 2) / 12.0;

		for (int i = 0; i < 12; i++) {

			float cosDiv = cos(div * i);
			float sinDiv = sin(div * i);
		
			float xPos  = sinDiv * 52.5;
			float yPos  = cosDiv * 52.5;
			float xxPos = sinDiv * 60.0;
			float yyPos = cosDiv * 60.0;

	//		ui.calculateKeyboard(i, xSpace, xOffset, 230.0, &xPos, &yPos, &scale);
			addChild(createLight<SmallLight<GreenLight>>(Vec(xxPos + 116.5, 149.5 - yyPos), module, Circle::CKEY_LIGHT + CoreUtil().CIRCLE_FIFTHS[i]));

	//		ui.calculateKeyboard(i, xSpace, xOffset + 72.0, 165.0, &xPos, &yPos, &scale);
			addChild(createLight<SmallLight<RedLight>>(Vec(xPos + 116.5, 149.5 - yPos), module, Circle::BKEY_LIGHT + CoreUtil().CIRCLE_FIFTHS[i]));
		}
		
		float xOffset = 18.0;
		
		for (int i = 0; i < 7; i++) {
			float xPos = 2 * xOffset + i * 18.2;
			addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, 280.0), module, Circle::MODE_LIGHT + i));
		}

		addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 5, true, false), module, Circle::KEY_OUTPUT));
		addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 5, 5, true, false), module, Circle::MODE_OUTPUT));

	}

	void appendContextMenu(Menu *menu) override {

		Circle *circle = dynamic_cast<Circle*>(module);
		assert(circle);

		struct ScalingItem : MenuItem {
			Circle *module;
			Circle::Scaling voltScale;
			void onAction(const event::Action &e) override {
				module->voltScale = voltScale;
			}
		};

		struct ScalingMenu : MenuItem {
			Circle *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<Circle::Scaling> modes = {Circle::FIFTHS, Circle::CHROMATIC};
				std::vector<std::string> names = {"Fifths", "Chromatic (V/OCT)"};
				for (size_t i = 0; i < modes.size(); i++) {
					ScalingItem *item = createMenuItem<ScalingItem>(names[i], CHECKMARK(module->voltScale == modes[i]));
					item->module = module;
					item->voltScale = modes[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());
		ScalingMenu *item = createMenuItem<ScalingMenu>("Root Volt Scaling");
		item->module = circle;
		menu->addChild(item);

     }
	 
};

Model *modelCircle = createModel<Circle, CircleWidget>("Circle");

// ♯♭
