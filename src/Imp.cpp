#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

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
		coreDelayState = false;
		coreGateState = false;
		coreDelayTime = 0.0;
		coreGateTime = 0.0;
		for (int i = 0; i < 16; i++) {
			delayState[i] = false;
			gateState[i] = false;
			delayTime[i] = 0.0;
			gateTime[i] = 0.0;
		}
		bpm = 0.0;
	}

	int delayTimeMs;
	int delaySprMs;
	int gateTimeMs;
	int gateSprMs;
	float bpm;
	int division;
	int actDelayMs = 0;
	int actGateMs = 0;
	float prob;

	bool coreDelayState;
	bool coreGateState;
	float coreDelayTime;
	float coreGateTime;
	digital::AHPulseGenerator coreDelayPhase;
	digital::AHPulseGenerator coreGatePhase;

	bool delayState[16];
	bool gateState[16];
	float delayTime[16];
	float gateTime[16];
	digital::AHPulseGenerator delayPhase[16];
	digital::AHPulseGenerator gatePhase[16];

	rack::dsp::SchmittTrigger inTrigger;

	int counter = 0;
	bool randomZero = true;

	digital::BpmCalculator bpmCalc;
		
};

void Imp::process(const ProcessArgs &args) {
	
	AHModule::step();

	float dlyLen;
	float dlySpr;
	float gateLen;
	float gateSpr;
	
	bool generateSignal = false;
	
	bool inputActive = inputs[TRIG_INPUT].isConnected();
	bool haveTrigger = inTrigger.process(inputs[TRIG_INPUT].getVoltage());
	
	if (inputActive) {
		
		bpm = bpmCalc.calculateBPM(args.sampleTime, inputs[TRIG_INPUT].getVoltage());
	
		if (haveTrigger) {
			if (debugEnabled()) { std::cout << stepX << " have active input and has received trigger" << std::endl; }
			generateSignal = true;
		}
	} 
	
	dlyLen = log2(params[DELAY_PARAM].getValue());
	dlySpr = log2(params[DELAYSPREAD_PARAM].getValue());
	gateLen = log2(params[LENGTH_PARAM].getValue());
	gateSpr = log2(params[LENGTHSPREAD_PARAM].getValue());
	division = params[DIVISION_PARAM].getValue();
	
	delayTimeMs = dlyLen * 1000;
	delaySprMs = dlySpr * 2000; // scaled by ±2 below
	gateTimeMs = gateLen * 1000;
	gateSprMs = gateSpr * 2000; // scaled by ±2 below
	prob = params[PROB_PARAM].getValue() * 100.0f;
			
	if (generateSignal) {

		counter++;
	
		if (debugEnabled()) { 
			std::cout << stepX << " Div: " << ": Target: " << division << " Cnt: " << counter << " Exp: " << counter % division << std::endl; 
		}

		// Check clock division and Bern. gate
		if ((counter % division == 0) && (random::uniform() < params[PROB_PARAM].getValue())) { 

			// check that we are not in the gate phase
			if (!coreGatePhase.ishigh() && !coreDelayPhase.ishigh()) {

				// Determine delay and gate times for all active outputs
				coreDelayTime = clamp(dlyLen, 0.0f, 100.0f);
			
				// The modified gate time cannot be earlier than the start of the delay
				coreGateTime = clamp(gateLen, digital::TRIGGER, 100.0f);

				if (debugEnabled()) { 
					std::cout << stepX << " Delay: " << ": Len = " << delayTime << std::endl; 
					std::cout << stepX << " Gate: " << ": Len = " << gateTime << std::endl; 
				}

				// Trigger the respective delay pulse generator
				coreDelayState = true;
				if (coreDelayPhase.trigger(coreDelayTime)) {
					actDelayMs = coreDelayTime * 1000;
				}
			}

			for (int i = 0; i < 16; i++) {

				// check that we are not in the gate phase
				if (!gatePhase[i].ishigh() && !delayPhase[i].ishigh()) {

					if (i == 0 && !randomZero) {

						// Non-randomised delay and gate length
						delayTime[i] = coreDelayTime;
						gateTime[i] = coreGateTime;

						if (debugEnabled()) { 
							std::cout << stepX << " Delay: " << ": Len: " << dlyLen << " Spr: " << dlySpr << " = " << delayTime << std::endl; 
							std::cout << stepX << " Gate: " << ": Len: " << gateLen << ", Spr: " << gateSpr << " = " << gateTime << std::endl; 
						}

					} else {

						// Determine delay and gate times for all active outputs
						double rndD = clamp(random::normal(), -2.0f, 2.0f);
						delayTime[i] = clamp(dlyLen + dlySpr * rndD, 0.0f, 100.0f);
					
						// The modified gate time cannot be earlier than the start of the delay
						double rndG = clamp(random::normal(), -2.0f, 2.0f);
						gateTime[i] = clamp(gateLen + gateSpr * rndG, digital::TRIGGER, 100.0f);

						if (debugEnabled()) { 
							std::cout << stepX << " Delay: " << ": Len: " << dlyLen << " Spr: " << dlySpr << " r: " << rndD << " = " << delayTime << std::endl; 
							std::cout << stepX << " Gate: " << ": Len: " << gateLen << ", Spr: " << gateSpr << " r: " << rndG << " = " << gateTime << std::endl; 
						}

					}

					// Trigger the respective delay pulse generator
					delayPhase[i].trigger(delayTime[i]);
					delayState[i] = true;

				}
			}
		}
	}

	if (coreDelayState && !coreDelayPhase.process(args.sampleTime)) {
		if (coreGatePhase.trigger(coreGateTime)) {
			actGateMs = coreGateTime * 1000;
		}
		coreGateState = true;
		coreDelayState = false;
	}

	if (coreGatePhase.process(args.sampleTime)) {
		lights[OUT_LIGHT].setSmoothBrightness(1.0f, args.sampleTime);
		lights[OUT_LIGHT + 1].setSmoothBrightness(0.0f, args.sampleTime);
	} else {
		coreGateState = false;

		if (coreDelayState) {
			lights[OUT_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
			lights[OUT_LIGHT + 1].setSmoothBrightness(1.0f, args.sampleTime);
		} else {
			lights[OUT_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
			lights[OUT_LIGHT + 1].setSmoothBrightness(0.0f, args.sampleTime);
		}
	}

	outputs[OUT_OUTPUT].setChannels(16);
	for (int i = 0; i < 16; i++) {
		if (delayState[i] && !delayPhase[i].process(args.sampleTime)) {
			gatePhase[i].trigger(gateTime[i]);
			gateState[i] = true;
			delayState[i] = false;
		}

		if (gatePhase[i].process(args.sampleTime)) {
			outputs[OUT_OUTPUT].setVoltage(10.0f, i);
		} else {
			outputs[OUT_OUTPUT].setVoltage(0.0f, i);
			gateState[i] = false;
		}
	}	
}

struct ImpBox : TransparentWidget {
	
	Imp *module;
	std::shared_ptr<Font> font;
	float *bpm;
	float *prob;
	int *dly;
	int *dlySpr;
	int *gate;
	int *gateSpr;
	int *division;
	int *actDly;
	int *actGate;
	
	ImpBox() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/DSEG14ClassicMini-BoldItalic.ttf"));
	}

	void draw(const DrawArgs &ctx) override {
	
		Vec pos(15.0, 0.0);

		float n = 35.0;

		nvgFontSize(ctx.vg, 10);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgTextLetterSpacing(ctx.vg, -1);
		nvgTextAlign(ctx.vg, NVGalign::NVG_ALIGN_LEFT);
		nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
	
		char text[10];
		if (*bpm == 0.0f) {
			snprintf(text, sizeof(text), "-");
		} else {
			snprintf(text, sizeof(text), "%.1f", *bpm);
		}
		nvgText(ctx.vg, pos.x + 75, pos.y + -1 * n, text, NULL);

		snprintf(text, sizeof(text), "%.1f", *prob);
		nvgText(ctx.vg, pos.x + 75, pos.y, text, NULL);

		snprintf(text, sizeof(text), "%d", *dly);
		nvgText(ctx.vg, pos.x + 75, pos.y + 1 * n, text, NULL);

		if (*dlySpr != 0) {
			snprintf(text, sizeof(text), "%d", *dlySpr);
			nvgText(ctx.vg, pos.x + 75, pos.y + 2 * n, text, NULL);
		}

		snprintf(text, sizeof(text), "%d", *gate);
		nvgText(ctx.vg, pos.x + 75, pos.y + 3 * n, text, NULL);

		if (*gateSpr != 0) {
			snprintf(text, sizeof(text), "%d", *gateSpr);
			nvgText(ctx.vg, pos.x + 75, pos.y + 4 * n, text, NULL);
		}

		snprintf(text, sizeof(text), "%d", *division);
		nvgText(ctx.vg, pos.x + 75, pos.y + 5 * n, text, NULL);
		
		snprintf(text, sizeof(text), "%d", *actGate);
		nvgText(ctx.vg, pos.x + 75, pos.y + 6 * n, text, NULL);

		nvgTextAlign(ctx.vg, NVGalign::NVG_ALIGN_RIGHT);
		snprintf(text, sizeof(text), "%d", *actDly);
		nvgText(ctx.vg, pos.x + 27.5, pos.y + 6 * n, text, NULL);

	}
	
};

struct ImpWidget : ModuleWidget {

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

			display->bpm = &(module->bpm);
			display->prob = &(module->prob);
			display->dly = &(module->delayTimeMs);
			display->dlySpr = &(module->delaySprMs);
			display->gate = &(module->gateTimeMs);
			display->gateSpr = &(module->gateSprMs);
			display->division = &(module->division);
			display->actDly = &(module->actDelayMs);
			display->actGate = &(module->actGateMs);
			
			addChild(display);
		}	
	}

	void appendContextMenu(Menu *menu) override {

		Imp *imp = dynamic_cast<Imp*>(module);
		assert(imp);

		struct RandomZeroItem : MenuItem {
			Imp *module;
			bool randomZero;
			void onAction(const rack::event::Action &e) override {
				module->randomZero = randomZero;
			}
		};

		struct RandomZeroMenu : MenuItem {
			Imp *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<bool> modes = {true, false};
				std::vector<std::string> names = {"Randomized", "Non-randomized"};
				for (size_t i = 0; i < modes.size(); i++) {
					RandomZeroItem *item = createMenuItem<RandomZeroItem>(names[i], CHECKMARK(module->randomZero == modes[i]));
					item->module = module;
					item->randomZero = modes[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());
		RandomZeroMenu *randomZeroItem = createMenuItem<RandomZeroMenu>("Randomize first output");
		randomZeroItem->module = imp;
		menu->addChild(randomZeroItem);

     }

};

Model *modelImp = createModel<Imp, ImpWidget>("Imp");

