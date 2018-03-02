#include "dsp/digital.hpp"

#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"

#include <iostream>

struct Imperfect2 : Module {

	enum ParamIds {
		DELAY_PARAM,
		DELAYSPREAD_PARAM = DELAY_PARAM + 4,
		LENGTH_PARAM = DELAYSPREAD_PARAM + 4,
		LENGTHSPREAD_PARAM = LENGTH_PARAM + 4,
		DIVISION_PARAM = LENGTHSPREAD_PARAM + 4,
		NUM_PARAMS = DIVISION_PARAM + 4
	};
	enum InputIds {
		TRIG_INPUT,
		DELAY_INPUT = TRIG_INPUT + 4,
		DELAYSPREAD_INPUT = DELAY_INPUT + 4,
		LENGTH_INPUT = DELAYSPREAD_INPUT + 4,
		LENGTHSPREAD_INPUT = LENGTH_INPUT + 4,
		NUM_INPUTS = LENGTHSPREAD_INPUT + 4
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS = OUT_OUTPUT + 4
	};
	enum LightIds {
		OUT_LIGHT,
		NUM_LIGHTS = OUT_LIGHT + 8
	};

	float delta;
		
	Imperfect2() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		reset();
		delta = 1.0 / engineGetSampleRate();
	}
	
	void onSampleRateChange() override { 
		delta = 1.0 / engineGetSampleRate();
	}

	void step() override;
	float calculateBPM(int index); // From ML
	
	void reset() override {
		for (int i = 0; i < 4; i++) {
			delayState[i] = false;
			gateState[i] = false;
			delayTime[i] = 0.0;
			gateTime[i] = 0.0;
			bpm[i] = 0.0;
		}
	}
	
	
	Core core;

	bool delayState[4];
	bool gateState[4];
	float delayTime[4];
	int delayTimeMs[4];
	int delaySprMs[4];
	float gateTime[4];
	int gateTimeMs[4];
	int gateSprMs[4];
	float bpm[4];
	int division[4];
	
	PulseGenerator delayPhase[4];
	PulseGenerator gatePhase[4];
	SchmittTrigger inTrigger[4];

	int counter[4];
	int counterIndex[4];

	int stepX;
	
	inline bool debug() {
		return false;
	}
	
	float timer[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	int misses[4] = {0, 0, 0, 0};
	float seconds[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	SchmittTrigger gateTrigger[4];
	
	inline bool checkBeat(float timer, int mult, float seconds, int misses) {
		return ( ((timer - mult * seconds) * (timer - mult * seconds) / (seconds * seconds) < 0.2f ) && misses < 4);
	}
	
};

float Imperfect2::calculateBPM(int i) {

	if (gateTrigger[i].process(inputs[TRIG_INPUT + i].value) ) {

		if (timer[i] > 0) {

			float new_seconds;
			bool found = false;

			for(int mult = 1; !found && mult < 20; mult++ )  {

				if (checkBeat(timer[i], mult, seconds[i], misses[i])) {
					new_seconds = timer[i] / mult;
					if (mult == 1) {
						misses[i] = 0;
					} else {
						misses[i]++;
					}
					
					found = true;
				};
				
			};

			if (!found) {
				// std::cerr << "default. misses = " << misses << "\n";
				new_seconds = timer[i];
				misses[i] = 0;
			}

			float a = 0.5f; // params[SMOOTH_PARAM].value;
			seconds[i] = ((1.0f - a) * seconds[i] + a * new_seconds);
			timer[i] -= seconds[i];

		}

	};

	timer[i] += delta;
	if (seconds[i] < 2.0e-05) {
		return 0.0f;
	} else {
		return 60.0f / seconds[i];
	}
};


void Imperfect2::step() {
	
	stepX++;
	
	float dlyLen;
	float dlySpr;
	float gateLen;
	float gateSpr;
	
	int lastValidInput = -1;
	
	for (int i = 0; i < 4; i++) {
		
		bool generateSignal = false;
		
		bool inputActive = inputs[TRIG_INPUT + i].active;
		bool haveTrigger = inTrigger[i].process(inputs[TRIG_INPUT + i].value);
		bool outputActive = outputs[OUT_OUTPUT + i].active;
		
		// If we have an active input, we should forget about previous valid inputs
		if (inputActive) {
			
			bpm[i] = calculateBPM(i);
		
			lastValidInput = -1;
	
			if (haveTrigger) {
				if (debug()) { std::cout << stepX << " " << i << " has active input and has received trigger" << std::endl; }
				generateSignal = true;
				lastValidInput = i;
			}
			
		} else {
			// We have an output and previously seen a trigger
			if (outputActive && lastValidInput > -1) {
				if (debug()) { std::cout << stepX << " " << i << " has active out and has seen trigger on " << lastValidInput << std::endl; }
				generateSignal = true;
			}
			
			bpm[i] = 0.0;
			
		}
		
		if (inputs[DELAY_INPUT + i].active) {
			dlyLen = log2(fabs(inputs[DELAY_INPUT + i].value) + 1.0f); 
		} else {
			dlyLen = log2(params[DELAY_PARAM + i].value);
		}	

		if (inputs[DELAYSPREAD_INPUT + i].active) {
			dlySpr = log2(fabs(inputs[DELAYSPREAD_INPUT + i].value) + 1.0f); 
		} else {
			dlySpr = log2(params[DELAYSPREAD_PARAM + i].value);
		}	
		
		if (inputs[LENGTH_INPUT + i].active) {
			gateLen = log2(fabs(inputs[LENGTH_INPUT + i].value) + 1.001f); 
		} else {
			gateLen = log2(params[LENGTH_PARAM + i].value);
		}	
		
		if (inputs[LENGTHSPREAD_INPUT + i].active) {
			gateSpr = log2(fabs(inputs[LENGTHSPREAD_INPUT + i].value) + 1.0f); 
		} else {
			gateSpr = log2(params[LENGTHSPREAD_PARAM + i].value);
		}	
		
		division[i] = params[DIVISION_PARAM + i].value;
		
		delayTimeMs[i] = dlyLen * 1000;
		delaySprMs[i] = dlySpr * 1000;
		gateTimeMs[i] = gateLen * 1000;
		gateSprMs[i] = gateSpr * 1000;
				
		if (generateSignal) {

			counter[i]++;
			int target = division[i];
		
			if (debug()) { 
				std::cout << stepX << " Div: " << i << ": Target: " << target << " Cnt: " << counter[lastValidInput] << " Exp: " << counter[lastValidInput] % division[i] << std::endl; 
			}

			if (counter[lastValidInput] % target == 0) { 

				// Determine delay and gate times for all active outputs
				double rndD = clamp(core.gaussrand(), -2.0f, 2.0f);
				delayTime[i] = clamp(dlyLen + dlySpr * rndD, 0.0f, 100.0f);
				
				// The modified gate time cannot be earlier than the start of the delay
				double rndG = clamp(core.gaussrand(), -2.0f, 2.0f);
				gateTime[i] = clamp(gateLen + gateSpr * rndG, 0.02f, 100.0f);

				if (debug()) { 
					std::cout << stepX << " Delay: " << i << ": Len: " << dlyLen << " Spr: " << dlySpr << " r: " << rndD << " = " << delayTime[i] << std::endl; 
					std::cout << stepX << " Gate: " << i << ": Len: " << gateLen << ", Spr: " << gateSpr << " r: " << rndG << " = " << gateTime[i] << std::endl; 
				}

				// Trigger the respective delay pulse generators
				delayState[i] = true;
				delayPhase[i].trigger(delayTime[i]);
			
			}
		}
	}
	
	for (int i = 0; i < 4; i++) {
	
		if (delayState[i] && !delayPhase[i].process(delta)) {
			gatePhase[i].trigger(gateTime[i]);
			gateState[i] = true;
			delayState[i] = false;
		}

		lights[OUT_LIGHT + i * 2].value = 0.0f;
		lights[OUT_LIGHT + i * 2 + 1].value = 0.0f;
		
		if (gatePhase[i].process(delta)) {
			outputs[OUT_OUTPUT + i].value = 10.0f;

			lights[OUT_LIGHT + i * 2].value = 1.0f;
			lights[OUT_LIGHT + i * 2 + 1].value = 0.0f;

		} else {
			outputs[OUT_OUTPUT + i].value = 0.0f;
			gateState[i] = false;

			if (delayState[i]) {
				lights[OUT_LIGHT + i * 2].value = 0.0f;
				lights[OUT_LIGHT + i * 2 + 1].value = 1.0f;
			} 
			
		}
			
	}
	
}

struct Imperfect2FloatBox : TransparentWidget {
	
	Imperfect2 *module;
	int frame = 0;
	std::shared_ptr<Font> font;
	float *value;
	
	Imperfect2FloatBox() {
		font = Font::load(assetPlugin(plugin, "res/DSEG7ClassicMini-BoldItalic.ttf"));
	}

	void draw(NVGcontext *vg) override {
	
		Vec pos = Vec(0, 15);

		nvgFontSize(vg, 14);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -1);

		nvgFillColor(vg, nvgRGBA(255, 0, 0, 0xff));
	
		char text[6];
		snprintf(text, sizeof(text), "%.1f", *value);
		nvgText(vg, pos.x, pos.y, text, NULL);
		
	}
	
};

struct Imperfect2Box : TransparentWidget {
	
	Imperfect2 *module;
	int frame = 0;
	std::shared_ptr<Font> font;
	float *bpm;
	int *dly;
	int *dlySpr;
	int *gate;
	int *gateSpr;
	int *division;
	
	Imperfect2Box() {
		font = Font::load(assetPlugin(plugin, "res/DSEG7ClassicMini-BoldItalic.ttf"));
	}

	void draw(NVGcontext *vg) override {
	
		Vec pos = Vec(0, 15);

		nvgFontSize(vg, 10);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -1);
		nvgTextAlign(vg, NVGalign::NVG_ALIGN_CENTER);

		nvgFillColor(vg, nvgRGBA(255, 0, 0, 0xff));
	
		char text[10];
		if (*bpm == 0.0f) {
			snprintf(text, sizeof(text), "-");
		} else {
			snprintf(text, sizeof(text), "%.1f", *bpm);
		}
		nvgText(vg, pos.x + 20, pos.y, text, NULL);
		
		snprintf(text, sizeof(text), "%d", *dly);
		nvgText(vg, pos.x + 74, pos.y, text, NULL);

		if (*dlySpr != 0) {
			snprintf(text, sizeof(text), "%d", *dlySpr);
			nvgText(vg, pos.x + 144, pos.y, text, NULL);
		}

		snprintf(text, sizeof(text), "%d", *gate);
		nvgText(vg, pos.x + 214, pos.y, text, NULL);

		if (*gateSpr != 0) {
			snprintf(text, sizeof(text), "%d", *gateSpr);
			nvgText(vg, pos.x + 284, pos.y, text, NULL);
		}

		snprintf(text, sizeof(text), "%d", *division);
		nvgText(vg, pos.x + 334, pos.y, text, NULL);

		
	}
	
};


struct Imperfect2Widget : ModuleWidget {
	Imperfect2Widget(Imperfect2 *module);
};

Imperfect2Widget::Imperfect2Widget(Imperfect2 *module) : ModuleWidget(module) {
	
	UI ui;
	
	box.size = Vec(450, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Imperfect2.svg")));
		addChild(panel);
	}

	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 30, 365)));
	
	{
		Imperfect2Box *display = new Imperfect2Box();
		display->module = module;
		display->box.pos = Vec(10, 95);
		display->box.size = Vec(200, 20);
		display->bpm = &(module->bpm[0]);
		display->dly = &(module->delayTimeMs[0]);
		display->dlySpr = &(module->delaySprMs[0]);
		display->gate = &(module->gateTimeMs[0]);
		display->gateSpr = &(module->gateSprMs[0]);
		display->division = &(module->division[0]);
			
		addChild(display);
	}	

	{
		Imperfect2Box *display = new Imperfect2Box();
		display->module = module;
		display->box.pos = Vec(10, 165);
		display->box.size = Vec(200, 20);
		display->bpm = &(module->bpm[1]);
		display->dly = &(module->delayTimeMs[1]);
		display->dlySpr = &(module->delaySprMs[1]);
		display->gate = &(module->gateTimeMs[1]);
		display->gateSpr = &(module->gateSprMs[1]);
		display->division = &(module->division[1]);
			
		addChild(display);
	}	

	{
		Imperfect2Box *display = new Imperfect2Box();
		display->module = module;
		display->box.pos = Vec(10, 235);
		display->box.size = Vec(200, 20);
		display->bpm = &(module->bpm[2]);
		display->dly = &(module->delayTimeMs[2]);
		display->dlySpr = &(module->delaySprMs[2]);
		display->gate = &(module->gateTimeMs[2]);
		display->gateSpr = &(module->gateSprMs[2]);
		display->division = &(module->division[2]);
			
		addChild(display);
	}	

	{
		Imperfect2Box *display = new Imperfect2Box();
		display->module = module;
		display->box.pos = Vec(10, 305);
		display->box.size = Vec(200, 20);
		display->bpm = &(module->bpm[3]);
		display->dly = &(module->delayTimeMs[3]);
		display->dlySpr = &(module->delaySprMs[3]);
		display->gate = &(module->gateTimeMs[3]);
		display->gateSpr = &(module->gateSprMs[3]);
		display->division = &(module->division[3]);
			
		addChild(display);
	}	



	for (int i = 0; i < 4; i++) {
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 0, i * 2 + 1, true, true), Port::INPUT, module, Imperfect2::TRIG_INPUT + i));
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 1, i * 2 + 1, true, true), Port::INPUT, module, Imperfect2::DELAY_INPUT + i));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 2, i * 2 + 1, true, true), module, Imperfect2::DELAY_PARAM + i, 1.0f, 2.0f, 1.0f));
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 3, i * 2 + 1, true, true), Port::INPUT, module, Imperfect2::DELAYSPREAD_INPUT + i));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 4, i * 2 + 1, true, true), module, Imperfect2::DELAYSPREAD_PARAM + i, 1.0f, 2.0f, 1.0f));
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 5, i * 2 + 1, true, true), Port::INPUT, module, Imperfect2::LENGTH_INPUT + i));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 6, i * 2 + 1, true, true), module, Imperfect2::LENGTH_PARAM + i, 1.001f, 2.0f, 1.001f)); // Always produce gate
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 7, i * 2 + 1, true, true), Port::INPUT, module, Imperfect2::LENGTHSPREAD_INPUT + i));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 8, i * 2 + 1, true, true), module, Imperfect2::LENGTHSPREAD_PARAM + i, 1.0, 2.0, 1.0f)); 
		addParam(ParamWidget::create<AHKnobSnap>(ui.getPosition(UI::KNOB, 9, i * 2 + 1, true, true), module, Imperfect2::DIVISION_PARAM + i, 1, 64, 1));
		addChild(ModuleLightWidget::create<MediumLight<GreenRedLight>>(ui.getPosition(UI::LIGHT, 10, i * 2 + 1, true, true), module, Imperfect2::OUT_LIGHT + i * 2));
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 11, i * 2+ 1, true, true), Port::OUTPUT, module, Imperfect2::OUT_OUTPUT + i));
	}
	
}

Model *modelImperfect2 = Model::create<Imperfect2, Imperfect2Widget>( "Amalgamated Harmonics", "Imperfect2", "Imperfect MkII", UTILITY_TAG);

