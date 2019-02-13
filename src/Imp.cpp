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

		params[DELAY_PARAM].config(1.0f, 2.0f, 1.0f, "Delay length", "ms", 2.0f, 500.0f, -1000.0f);

		params[DELAYSPREAD_PARAM].config(1.0f, 2.0f, 1.0f, "Delay length spread", "ms", 2.0f, 500.0f, -1000.0f);
		params[DELAYSPREAD_PARAM].description = "Magnitude of random time applied to delay length";

		params[LENGTH_PARAM].config(1.001f, 2.0f, 1.001f, "Gate length", "ms", 2.0f, 500.0f, -1000.0f); // Always produce gate

		params[LENGTHSPREAD_PARAM].config(1.0, 2.0, 1.0f, "Gate length spread", "ms", 2.0f, 500.0f, -1000.0f);
		params[LENGTHSPREAD_PARAM].description = "Magnitude of random time applied to gate length";

		params[DIVISION_PARAM].config(1, 64, 1, "Clock division");

		debugFlag = false;

		onReset();

	}
	
	void step() override;
	
	void onReset() override {
		delayState = false;
		gateState = false;
		delayTime = 0.0;
		gateTime = 0.0;
		bpm = 0.0;
	}

	bool delayState;
	bool gateState;
	float delayTime;
	int delayTimeMs;
	int delaySprMs;
	float gateTime;
	int gateTimeMs;
	int gateSprMs;
	float bpm;
	int division;
	int actDelayMs = 0;
	int actGateMs = 0;
	
	digital::AHPulseGenerator delayPhase;
	digital::AHPulseGenerator gatePhase;
	rack::dsp::SchmittTrigger inTrigger;

	int counter = 0;
	
	digital::BpmCalculator bpmCalc;
		
};

void Imp::step() {
	
	core::AHModule::step();

	float dlyLen;
	float dlySpr;
	float gateLen;
	float gateSpr;
	
	bool generateSignal = false;
	
	bool inputActive = inputs[TRIG_INPUT].isConnected();
	bool haveTrigger = inTrigger.process(inputs[TRIG_INPUT].getVoltage());
	
	// This is where we manage row-chaining/normalisation, i.e a row can be active without an
	// input by receiving the input clock from a previous (higher) row
	// If we have an active input, we should forget about previous valid inputs
	if (inputActive) {
		
		bpm = bpmCalc.calculateBPM(delta, inputs[TRIG_INPUT].getVoltage());
	
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
			
	if (generateSignal) {

		counter++;
	
		if (debugEnabled()) { 
			std::cout << stepX << " Div: " << ": Target: " << division << " Cnt: " << counter << " Exp: " << counter % division << std::endl; 
		}

		if (counter % division == 0) { 

			// check that we are not in the gate phase
			if (!gatePhase.ishigh() && !delayPhase.ishigh()) {

			// Determine delay and gate times for all active outputs
				double rndD = clamp(digital::gaussrand(), -2.0f, 2.0f);
				delayTime = clamp(dlyLen + dlySpr * rndD, 0.0f, 100.0f);
			
				// The modified gate time cannot be earlier than the start of the delay
				double rndG = clamp(digital::gaussrand(), -2.0f, 2.0f);
				gateTime = clamp(gateLen + gateSpr * rndG, digital::TRIGGER, 100.0f);

				if (debugEnabled()) { 
					std::cout << stepX << " Delay: " << ": Len: " << dlyLen << " Spr: " << dlySpr << " r: " << rndD << " = " << delayTime << std::endl; 
					std::cout << stepX << " Gate: " << ": Len: " << gateLen << ", Spr: " << gateSpr << " r: " << rndG << " = " << gateTime << std::endl; 
				}

				// Trigger the respective delay pulse generator
				delayState = true;
				if (delayPhase.trigger(delayTime)) {
					actDelayMs = delayTime * 1000;
				}
			}
		}
	}
	
	if (delayState && !delayPhase.process(delta)) {
		if (gatePhase.trigger(gateTime)) {
			actGateMs = gateTime * 1000;
		}
		gateState = true;
		delayState = false;
	}

	if (gatePhase.process(delta)) {
		outputs[OUT_OUTPUT].setVoltage(10.0f);

		lights[OUT_LIGHT].setBrightnessSmooth(1.0f);
		lights[OUT_LIGHT + 1].setBrightnessSmooth(0.0f);

	} else {
		outputs[OUT_OUTPUT].setVoltage(0.0f);
		gateState = false;

		if (delayState) {
			lights[OUT_LIGHT].setBrightnessSmooth(0.0f);
			lights[OUT_LIGHT + 1].setBrightnessSmooth(1.0f);
		} else {
			lights[OUT_LIGHT].setBrightnessSmooth(0.0f);
			lights[OUT_LIGHT + 1].setBrightnessSmooth(0.0f);
		}
		
	}
	
}

struct ImpBox : TransparentWidget {
	
	Imp *module;
	std::shared_ptr<Font> font;
	float *bpm;
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

	void draw(const DrawContext &ctx) override {
	
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
		
		snprintf(text, sizeof(text), "%d", *actDly);
		nvgText(ctx.vg, pos.x + 75, pos.y + 6 * n, text, NULL);

		snprintf(text, sizeof(text), "%d", *actGate);
		nvgText(ctx.vg, pos.x, pos.y + 6 * n, text, NULL);

	}
	
};

struct ImpWidget : ModuleWidget {

	ImpWidget(Imp *module) {
	
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Imp.svg")));

		Vec v1 = gui::getPosition(gui::PORT, 1, 0, true, true);
		Vec v2 = gui::getPosition(gui::PORT, 1, 1, true, true);

		std::cout << v1.y - v2.y << std::endl;

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 1, true, true), module, Imp::TRIG_INPUT));
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
};

Model *modelImp = createModel<Imp, ImpWidget>("Imp");

