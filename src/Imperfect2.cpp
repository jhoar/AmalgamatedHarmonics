#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>
#include <array>

using namespace ah;

struct Imperfect2 : core::AHModule {

	enum ParamIds {
		ENUMS(DELAY_PARAM,4),
		ENUMS(DELAYSPREAD_PARAM,4),
		ENUMS(LENGTH_PARAM,4),
		ENUMS(LENGTHSPREAD_PARAM,4),
		ENUMS(DIVISION_PARAM,4),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(TRIG_INPUT,4),
		ENUMS(DELAY_INPUT,4),
		ENUMS(DELAYSPREAD_INPUT,4),
		ENUMS(LENGTH_INPUT,4),
		ENUMS(LENGTHSPREAD_INPUT,4),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(OUT_OUTPUT,4),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(OUT_LIGHT,8),
		NUM_LIGHTS
	};

	Imperfect2() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

		for (int i = 0; i < 4; i++) {
			configParam(DELAY_PARAM + i, 1.0f, 2.0f, 1.0f, "Delay length", "ms", -2.0f, 1000.0f, 0.0f);

			configParam(DELAYSPREAD_PARAM + i, 1.0f, 2.0f, 1.0f, "Delay length spread", "ms", -2.0f, 2000.0f, 0.0f);
			paramQuantities[DELAYSPREAD_PARAM + i]->description = "Magnitude of random time applied to delay length";

			configParam(LENGTH_PARAM + i, 1.001f, 2.0f, 1.001f, "Gate length", "ms", -2.0f, 1000.0f, 0.0f); // Always produce gate

			configParam(LENGTHSPREAD_PARAM + i, 1.0, 2.0, 1.0f, "Gate length spread", "ms", -2.0f, 2000.0f, 0.0f);
			paramQuantities[LENGTHSPREAD_PARAM + i]->description = "Magnitude of random time applied to gate length";

			configParam(DIVISION_PARAM + i, 1, 64, 1, "Clock division");

		}

		onReset();

	}

	void process(const ProcessArgs &args) override;

	void onReset() override {
		for (int i = 0; i < 4; i++) {
			state[i].reset();
		}
	}

	std::array<ImperfectState,4> state;
	std::array<ImperfectSetting,4> setting;
	std::array<rack::dsp::SchmittTrigger,4> inTrigger;
	std::array<int,4> counter;
	std::array<digital::BpmCalculator,4> bpmCalc;

};

void Imperfect2::process(const ProcessArgs &args) {

	AHModule::step();

	int lastValidInput = -1;

	for (int i = 0; i < 4; i++) {

		bool generateSignal = false;

		bool inputActive = inputs[TRIG_INPUT + i].isConnected();
		bool haveTrigger = inTrigger[i].process(inputs[TRIG_INPUT + i].getVoltage());
		bool outputActive = outputs[OUT_OUTPUT + i].isConnected();

		// This is where we manage row-chaining/normalisation, i.e a row can be active without an
		// input by receiving the input clock from a previous (higher) row
		// If we have an active input, we should forget about previous valid inputs
		if (inputActive) {

			state[i].bpm = bpmCalc[i].calculateBPM(args.sampleTime, inputs[TRIG_INPUT + i].getVoltage());

			lastValidInput = -1;

			if (haveTrigger) {
				generateSignal = true;
				lastValidInput = i; // Row i has a valid input
			}

		} else {
			// We have an output plugged in this row and previously seen a trigger on previous row
			if (outputActive && lastValidInput > -1) {
				if (debugEnabled()) { std::cout << stepX << " " << i << " has active out and has seen trigger on " << lastValidInput << std::endl; }
				generateSignal = true;
			}

			state[i].bpm = 0.0;

		}

		if (inputs[DELAY_INPUT + i].isConnected()) {
			setting[i].dlyLen = log2(fabs(inputs[DELAY_INPUT + i].getVoltage()) + 1.0f); 
		} else {
			setting[i].dlyLen = log2(params[DELAY_PARAM + i].getValue());
		}

		if (inputs[DELAYSPREAD_INPUT + i].isConnected()) {
			setting[i].dlySpr = log2(fabs(inputs[DELAYSPREAD_INPUT + i].getVoltage()) + 1.0f); 
		} else {
			setting[i].dlySpr = log2(params[DELAYSPREAD_PARAM + i].getValue());
		}	

		if (inputs[LENGTH_INPUT + i].isConnected()) {
			setting[i].gateLen = log2(fabs(inputs[LENGTH_INPUT + i].getVoltage()) + 1.001f); 
		} else {
			setting[i].gateLen = log2(params[LENGTH_PARAM + i].getValue());
		}	

		if (inputs[LENGTHSPREAD_INPUT + i].isConnected()) {
			setting[i].gateSpr = log2(fabs(inputs[LENGTHSPREAD_INPUT + i].getVoltage()) + 1.0f); 
		} else {
			setting[i].gateSpr = log2(params[LENGTHSPREAD_PARAM + i].getValue());
		}	

		setting[i].division = params[DIVISION_PARAM + i].getValue();

		if (generateSignal) {

			counter[i]++;
			int target = setting[i].division;

			if (counter[lastValidInput] % target == 0) { 

				// check that we are not in the gate phase
				if (!state[i].gatePhase.ishigh() && !state[i].delayPhase.ishigh()) {

					// Generate randomised times
					state[i].jitter(setting[i]);

					// Trigger the respective delay pulse generator
					state[i].delayState = true;
					state[i].delayPhase.trigger(state[i].delayTime);
				}
			}
		}
	}

	for (int i = 0; i < 4; i++) {

		if (state[i].delayState && !state[i].delayPhase.process(args.sampleTime)) {
			state[i].gatePhase.trigger(state[i].gateTime);
			state[i].gateState = true;
			state[i].delayState = false;
		}

		if (state[i].gatePhase.process(args.sampleTime)) {
			outputs[OUT_OUTPUT + i].setVoltage(10.0f);

			lights[OUT_LIGHT + i * 2].setSmoothBrightness(1.0f, args.sampleTime);
			lights[OUT_LIGHT + i * 2 + 1].setSmoothBrightness(0.0f, args.sampleTime);

		} else {
			outputs[OUT_OUTPUT + i].setVoltage(0.0f);
			state[i].gateState = false;

			if (state[i].delayState) {
				lights[OUT_LIGHT + i * 2].setSmoothBrightness(0.0f, args.sampleTime);
				lights[OUT_LIGHT + i * 2 + 1].setSmoothBrightness(1.0f, args.sampleTime);
			} else {
				lights[OUT_LIGHT + i * 2].setSmoothBrightness(0.0f, args.sampleTime);
				lights[OUT_LIGHT + i * 2 + 1].setSmoothBrightness(0.0f, args.sampleTime);
			}
		}
	}
}

struct Imperfect2Box : TransparentWidget {

	Imperfect2 *module;
	std::shared_ptr<Font> font;

	ImperfectSetting *setting;
	ImperfectState *state;

	Imperfect2Box() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/DSEG14ClassicMini-BoldItalic.ttf"));
	}

	void draw(const DrawArgs &ctx) override {

		if (module == NULL) {
			return;
		}

		Vec pos = Vec(0, 15);

		nvgFontSize(ctx.vg, 10);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgTextLetterSpacing(ctx.vg, -1);
		nvgTextAlign(ctx.vg, NVGalign::NVG_ALIGN_CENTER);
		nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));

		char text[10];
		if (state->bpm == 0.0f) {
			snprintf(text, sizeof(text), "-");
		} else {
			snprintf(text, sizeof(text), "%.1f", state->bpm);
		}
		nvgText(ctx.vg, pos.x + 20, pos.y, text, NULL);

		snprintf(text, sizeof(text), "%d", static_cast<int>(setting->dlyLen * 1000));
		nvgText(ctx.vg, pos.x + 74, pos.y, text, NULL);

		if (setting->dlySpr != 0) {
			snprintf(text, sizeof(text), "%d", static_cast<int>(setting->dlySpr * 2000));
			nvgText(ctx.vg, pos.x + 144, pos.y, text, NULL);
		}

		snprintf(text, sizeof(text), "%d", static_cast<int>(setting->gateLen * 1000));
		nvgText(ctx.vg, pos.x + 214, pos.y, text, NULL);

		if (setting->gateSpr != 0) {
			snprintf(text, sizeof(text), "%d", static_cast<int>(setting->gateSpr * 2000));
			nvgText(ctx.vg, pos.x + 284, pos.y, text, NULL);
		}

		snprintf(text, sizeof(text), "%d", setting->division);
		nvgText(ctx.vg, pos.x + 334, pos.y, text, NULL);

		nvgFillColor(ctx.vg, nvgRGBA(0, 0, 0, 0xff));
		snprintf(text, sizeof(text), "%d", static_cast<int>(state->delayTime * 1000));
		nvgText(ctx.vg, pos.x + 372, pos.y, text, NULL);

		snprintf(text, sizeof(text), "%d", static_cast<int>(state->gateTime * 1000));
		nvgText(ctx.vg, pos.x + 408, pos.y, text, NULL);

	}

};

struct Imperfect2Widget : ModuleWidget {

	Imperfect2Widget(Imperfect2 *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Imperfect2.svg")));

		for (int i = 0; i < 4; i++) {
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, i * 2 + 1, true, true), module, Imperfect2::TRIG_INPUT + i));
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, i * 2 + 1, true, true), module, Imperfect2::DELAY_INPUT + i));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 2, i * 2 + 1, true, true), module, Imperfect2::DELAY_PARAM + i));
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 3, i * 2 + 1, true, true), module, Imperfect2::DELAYSPREAD_INPUT + i));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 4, i * 2 + 1, true, true), module, Imperfect2::DELAYSPREAD_PARAM + i));
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 5, i * 2 + 1, true, true), module, Imperfect2::LENGTH_INPUT + i));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 6, i * 2 + 1, true, true), module, Imperfect2::LENGTH_PARAM + i)); 
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 7, i * 2 + 1, true, true), module, Imperfect2::LENGTHSPREAD_INPUT + i));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 8, i * 2 + 1, true, true), module, Imperfect2::LENGTHSPREAD_PARAM + i)); 
			addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 9, i * 2 + 1, true, true), module, Imperfect2::DIVISION_PARAM + i));
			addChild(createLight<MediumLight<GreenRedLight>>(gui::getPosition(gui::LIGHT, 10, i * 2 + 1, true, true), module, Imperfect2::OUT_LIGHT + i * 2));
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 11, i * 2+ 1, true, true), module, Imperfect2::OUT_OUTPUT + i));
		}

		if (module != NULL) {

			{
				Imperfect2Box *display = createWidget<Imperfect2Box>(Vec(10, 95));

				display->module = module;
				display->box.size = Vec(200, 20);
				display->state = &(module->state[0]);
				display->setting = &(module->setting[0]);

				addChild(display);
			}

			{
				Imperfect2Box *display = createWidget<Imperfect2Box>(Vec(10, 165));

				display->module = module;
				display->box.size = Vec(200, 20);
				display->state = &(module->state[1]);
				display->setting = &(module->setting[1]);

				addChild(display);
			}

			{
				Imperfect2Box *display = createWidget<Imperfect2Box>(Vec(10, 235));

				display->module = module;
				display->box.size = Vec(200, 20);
				display->state = &(module->state[2]);
				display->setting = &(module->setting[2]);

				addChild(display);
			}

			{
				Imperfect2Box *display = createWidget<Imperfect2Box>(Vec(10, 305));

				display->module = module;
				display->box.size = Vec(200, 20);
				display->state = &(module->state[3]);
				display->setting = &(module->setting[3]);

				addChild(display);
			}
		}
	}
};

Model *modelImperfect2 = createModel<Imperfect2, Imperfect2Widget>("Imperfect2");

