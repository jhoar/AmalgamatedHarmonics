#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>
#include <array>

using namespace ah;

struct Imp : core::AHModule {

	enum ParamIds {
		DELAY_PARAM,
		DELAYSPREAD_PARAM,
		LENGTH_PARAM,
		LENGTHSPREAD_PARAM,
		DIVISION_PARAM,
		PROB_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		TRIG_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(OUT_LIGHT,2),
		NUM_LIGHTS
	};

	Imp() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

		configParam(PROB_PARAM, 0.0, 1.0, 1.0, "Clock-tick probability", "%", 0.0f, 100.0f);

		configParam(DELAY_PARAM, 1.0f, 2.0f, 1.0f, "Delay length", "ms", -2.0f, 1000.0f, 0.0f);

		configParam(DELAYSPREAD_PARAM, 1.0f, 2.0f, 1.0f, "Delay length spread", "ms", -2.0f, 2000.0f, 0.0f);
		paramQuantities[DELAYSPREAD_PARAM]->description = "Magnitude of random time applied to delay length";

		configParam(LENGTH_PARAM, 1.001f, 2.0f, 1.001f, "Gate length", "ms", -2.0f, 1000.0f, 0.0f); // Always produce gate

		configParam(LENGTHSPREAD_PARAM, 1.0, 2.0, 1.0f, "Gate length spread", "ms", -2.0f, 2000.0f, 0.0f);
		paramQuantities[LENGTHSPREAD_PARAM]->description = "Magnitude of random time applied to gate length";

		configParam(DIVISION_PARAM, 1, 64, 1, "Clock division");

		debugFlag = false;

		onReset();

	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// randomZero
		json_t *randomZeroJ = json_boolean(randomZero);
		json_object_set_new(rootJ, "randomzero", randomZeroJ);

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {

		// randomZero
		json_t *randomZeroJ = json_object_get(rootJ, "randomzero");
		if (randomZeroJ)
			randomZero = json_boolean_value(randomZeroJ);

	}

	void process(const ProcessArgs &args) override;

	void onReset() override {
		coreState.reset();
		for (int i = 0; i < 16; i++) {
			state[i].reset();
		}
	}

	ImperfectSetting setting;
	ImperfectState coreState;
	std::array<ImperfectState,16> state;

	rack::dsp::SchmittTrigger inTrigger;

	int counter = 0;
	bool randomZero = true;

	digital::BpmCalculator bpmCalc;

};

void Imp::process(const ProcessArgs &args) {

	AHModule::step();

	bool generateSignal = false;

	bool inputActive = inputs[TRIG_INPUT].isConnected();
	bool haveTrigger = inTrigger.process(inputs[TRIG_INPUT].getVoltage());

	if (inputActive) {

		coreState.bpm = bpmCalc.calculateBPM(args.sampleTime, inputs[TRIG_INPUT].getVoltage());

		if (haveTrigger) {
			generateSignal = true;
		}
	} 

	setting.dlyLen = log2(params[DELAY_PARAM].getValue());
	setting.dlySpr = log2(params[DELAYSPREAD_PARAM].getValue());
	setting.gateLen = log2(params[LENGTH_PARAM].getValue());
	setting.gateSpr = log2(params[LENGTHSPREAD_PARAM].getValue());
	setting.division = params[DIVISION_PARAM].getValue();
	setting.prob = params[PROB_PARAM].getValue() * 100.0f;

	if (generateSignal) {

		counter++;

		// Check clock division and Bern. gate
		if ((counter % setting.division == 0) && (random::uniform() < params[PROB_PARAM].getValue())) { 

			// check that we are not in the gate phase
			if (!coreState.gatePhase.ishigh() && !coreState.delayPhase.ishigh()) {

				// Determine delay and gate times for all active outputs
				// The modified gate time cannot be earlier than the start of the delay
				coreState.fixed(clamp(setting.dlyLen, 0.0f, 100.0f),
					clamp(setting.gateLen, digital::TRIGGER, 100.0f));	

				// Calculate the overall delay time for display
				coreState.delayState = true;
				coreState.delayPhase.trigger(coreState.delayTime);

			}

			for (int i = 0; i < 16; i++) {

				// check that we are not in the gate phase
				if (!state[i].gatePhase.ishigh() && !state[i].delayPhase.ishigh()) {

					if (i == 0 && !randomZero) {
						// Non-randomised delay and gate length
						state[i].fixed(coreState.delayTime, coreState.gateTime);	
					} else {
						state[i].jitter(setting);
					}

					// Trigger the respective delay pulse generator
					state[i].delayPhase.trigger(state[i].delayTime);
					state[i].delayState = true;

				}
			}
		}
	}

	if (coreState.delayState && !coreState.delayPhase.process(args.sampleTime)) {
		coreState.gatePhase.trigger(coreState.gateTime);
		coreState.gateState = true;
		coreState.delayState = false;
	}

	if (coreState.gatePhase.process(args.sampleTime)) {
		lights[OUT_LIGHT].setSmoothBrightness(1.0f, args.sampleTime);
		lights[OUT_LIGHT + 1].setSmoothBrightness(0.0f, args.sampleTime);
	} else {
		coreState.gateState = false;

		if (coreState.delayState) {
			lights[OUT_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
			lights[OUT_LIGHT + 1].setSmoothBrightness(1.0f, args.sampleTime);
		} else {
			lights[OUT_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
			lights[OUT_LIGHT + 1].setSmoothBrightness(0.0f, args.sampleTime);
		}
	}

	outputs[OUT_OUTPUT].setChannels(16);
	for (int i = 0; i < 16; i++) {
		if (state[i].delayState && !state[i].delayPhase.process(args.sampleTime)) {
			state[i].gatePhase.trigger(state[i].gateTime);
			state[i].gateState = true;
			state[i].delayState = false;
		}

		if (state[i].gatePhase.process(args.sampleTime)) {
			outputs[OUT_OUTPUT].setVoltage(10.0f, i);
		} else {
			outputs[OUT_OUTPUT].setVoltage(0.0f, i);
			state[i].gateState = false;
		}
	}	
}

struct ImpBox : TransparentWidget {
	
	Imp *module;
	std::shared_ptr<Font> font;

	ImperfectSetting *setting;
	ImperfectState *coreState;

	ImpBox() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/DSEG14ClassicMini-BoldItalic.ttf"));
	}

	void draw(const DrawArgs &ctx) override {
	
		if (module == NULL) {
			return;
	    }

		Vec pos(15.0, 0.0);

		float n = 35.0;

		nvgFontSize(ctx.vg, 10);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgTextLetterSpacing(ctx.vg, -1);
		nvgTextAlign(ctx.vg, NVGalign::NVG_ALIGN_LEFT);
		nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
	
		char text[10];
		if (coreState->bpm == 0.0f) {
			snprintf(text, sizeof(text), "-");
		} else {
			snprintf(text, sizeof(text), "%.1f", coreState->bpm);
		}
		nvgText(ctx.vg, pos.x + 75, pos.y + -1 * n, text, NULL);

		snprintf(text, sizeof(text), "%.1f", setting->prob);
		nvgText(ctx.vg, pos.x + 75, pos.y, text, NULL);

		snprintf(text, sizeof(text), "%d", static_cast<int>(setting->dlyLen * 1000));
		nvgText(ctx.vg, pos.x + 75, pos.y + 1 * n, text, NULL);

		if (setting->dlySpr != 0) {
			snprintf(text, sizeof(text), "%d", static_cast<int>(setting->dlySpr * 2000)); // * 2000 as it is scaled in jitter()
			nvgText(ctx.vg, pos.x + 75, pos.y + 2 * n, text, NULL);
		}

		snprintf(text, sizeof(text), "%d", static_cast<int>(setting->gateLen * 1000));
		nvgText(ctx.vg, pos.x + 75, pos.y + 3 * n, text, NULL);

		if (setting->gateSpr != 0) {
			snprintf(text, sizeof(text), "%d", static_cast<int>(setting->gateSpr * 2000)); // * 2000 as it is scaled in jitter()
			nvgText(ctx.vg, pos.x + 75, pos.y + 4 * n, text, NULL);
		}

		snprintf(text, sizeof(text), "%d", setting->division);
		nvgText(ctx.vg, pos.x + 75, pos.y + 5 * n, text, NULL);
		
	}

};

struct ImpWidget : ModuleWidget {

	std::vector<MenuOption<bool>> randomOptions;

	ImpWidget(Imp *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Imp.svg")));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 0, true, true), module, Imp::TRIG_INPUT));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 1, 1, true, true), module, Imp::PROB_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 1, 2, true, true), module, Imp::DELAY_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 1, 3, true, true), module, Imp::DELAYSPREAD_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 1, 4, true, true), module, Imp::LENGTH_PARAM)); 
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 1, 5, true, true), module, Imp::LENGTHSPREAD_PARAM)); 
		addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 1, 6, true, true), module, Imp::DIVISION_PARAM));
		addChild(createLight<MediumLight<GreenRedLight>>(gui::getPosition(gui::LIGHT, 1, 7, true, true), module, Imp::OUT_LIGHT * 2));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 8, true, true), module, Imp::OUT_OUTPUT));

		if (module != NULL) {
			ImpBox *display = createWidget<ImpBox>(Vec(0, 82));

			display->module = module;
			display->box.size = Vec(20, 200);

			display->setting = &(module->setting);
			display->coreState = &(module->coreState);

			addChild(display);
		}	

		randomOptions.emplace_back("Randomized", true);
		randomOptions.emplace_back("Non-randomized", false);

	}

	void appendContextMenu(Menu *menu) override {

		Imp *imp = dynamic_cast<Imp*>(module);
		assert(imp);

		struct ImpMenu : MenuItem {
			Imp *module;
			ImpWidget *parent;
		};

		struct RandomZeroItem : ImpMenu {
			bool randomZero;
			void onAction(const rack::event::Action &e) override {
				module->randomZero = randomZero;
			}
		};

		struct RandomZeroMenu : ImpMenu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->randomOptions) {
					RandomZeroItem *item = createMenuItem<RandomZeroItem>(opt.name, CHECKMARK(module->randomZero == opt.value));
					item->module = module;
					item->randomZero = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());
		RandomZeroMenu *randomZeroItem = createMenuItem<RandomZeroMenu>("Randomize first output");
		randomZeroItem->module = imp;
		randomZeroItem->parent = this;
		menu->addChild(randomZeroItem);

	}

};

Model *modelImp = createModel<Imp, ImpWidget>("Imp");

