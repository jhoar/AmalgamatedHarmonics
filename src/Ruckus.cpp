#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"

#include <iostream>
#include <sstream>  


struct Ruckus : AHModule {

	enum ParamIds {
		DIV_PARAM,
		PROB_PARAM = DIV_PARAM + 16,
		SHIFT_PARAM = PROB_PARAM + 16,
		XMUTE_PARAM = SHIFT_PARAM + 16, 
		YMUTE_PARAM = XMUTE_PARAM + 4, 
		NUM_PARAMS = YMUTE_PARAM + 4,
	};
	enum InputIds {
		TRIG_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		XOUT_OUTPUT,
		YOUT_OUTPUT = XOUT_OUTPUT + 4,
		NUM_OUTPUTS = YOUT_OUTPUT + 4,
	};
	enum LightIds {
		XMUTE_LIGHT, 
		YMUTE_LIGHT = XMUTE_LIGHT + 4, 
		NUM_LIGHTS = YMUTE_LIGHT + 4,
	};

	Ruckus() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		reset();
	}
	
	void step() override;
	float calculateBPM(int index); // From ML
	
	void reset() override {	}
	
	bool init = false;
	
	int displayC = 0;
	std::string pState = "";
	
	void receiveEvent(ParamEvent e) override {
		if (init) {
			std::stringstream ss;
			switch(e.spec) {
				case ParamEvent::INT: 
					ss << e.name << ": " << (int)e.value;
					break;
				case ParamEvent::FP:
					ss << e.name << ": " << e.value;
					break;
				default:
					ss << e.name << ": " << (int)e.value;
			}
			pState = std::string(ss.str());
		}
		displayC = 0;
	}
	
	
	Core core;

	AHPulseGenerator xGate[4];
	AHPulseGenerator yGate[4];
	
	bool xMute[4] = {true, true, true, true};
	bool yMute[4] = {true, true, true, true};
	
	SchmittTrigger xLockTrigger[4];
	SchmittTrigger yLockTrigger[4];

	SchmittTrigger inTrigger;

	int division[16];
	int shift[16];
	float prob[16];
	
	unsigned int counter = 100;
	
};

void Ruckus::step() {
	
	init = true;
	
	stepX++;
	
	// Timeout for display
	displayC++;
	if (displayC > 50000) {
		pState = ""; // Truncate
	}
	
	bool haveTrigger = inTrigger.process(inputs[TRIG_INPUT].value);
	
	for (int i = 0; i < 4; i++) {
		if (xLockTrigger[i].process(params[XMUTE_PARAM + i].value)) {
			xMute[i] = !xMute[i];
		}
		if (yLockTrigger[i].process(params[YMUTE_PARAM + i].value)) {
			yMute[i] = !yMute[i];
		}
	}

	for (int i = 0; i < 16; i++) {
		division[i] = params[DIV_PARAM + i].value;
		prob[i] = params[PROB_PARAM + i].value;
		shift[i] = params[SHIFT_PARAM + i].value;
	}
	
	if (haveTrigger) {

		counter++;

		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				int i = y * 4 + x;
						
				if(division[i] == 0) {
					continue; // 0 == skip
				}
				
				if ((counter + shift[i]) % division[i] == 0) { 
					float f = randomUniform();
					if (f < prob[i]) {
						xGate[x].trigger(1e-4);
						yGate[y].trigger(1e-4);
					}
				} 
			}
		}
	}

	for (int i = 0; i < 4; i++) {

		if(xGate[i].process(delta) && xMute[i]) {
			outputs[XOUT_OUTPUT + i].value = 10.0f;		
		} else {
			outputs[XOUT_OUTPUT + i].value = 0.0f;		
		}
		
		lights[XMUTE_LIGHT + i].value = xMute[i] ? 1.0 : 0.0;

		if(yGate[i].process(delta) && yMute[i]) {
			outputs[YOUT_OUTPUT + i].value = 10.0f;		
		} else {
			outputs[YOUT_OUTPUT + i].value = 0.0f;		
		}
		
		lights[YMUTE_LIGHT + i].value = yMute[i] ? 1.0 : 0.0;

	}
	
}

struct StateDisplay : TransparentWidget {
	
	Ruckus *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	StateDisplay() {
		font = Font::load(assetPlugin(plugin, "res/DSEG14ClassicMini-BoldItalic.ttf"));
	}

	void draw(NVGcontext *vg) override {
	
		Vec pos = Vec(0, 15);

		nvgFontSize(vg, 14);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -1);

		nvgFillColor(vg, nvgRGBA(255, 0, 0, 0xff));
	
		char text[128];
		snprintf(text, sizeof(text), "%s", module->pState.c_str());
		nvgText(vg, pos.x + 10, pos.y + 5, text, NULL);			

	}
	
};

struct RuckusWidget : ModuleWidget {
	RuckusWidget(Ruckus *module);
};

RuckusWidget::RuckusWidget(Ruckus *module) : ModuleWidget(module) {
	
	UI ui;
	
	box.size = Vec(390, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Ruckus.svg")));
		addChild(panel);
	}

	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 30, 365)));
	
	{
		StateDisplay *display = new StateDisplay();
		display->module = module;
		display->box.pos = Vec(30, 335);
		display->box.size = Vec(100, 140);
		addChild(display);
	}
	
	addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 9, 8, true, true), Port::INPUT, module, Ruckus::TRIG_INPUT));

	float xd = 18.0f;
	float yd = 20.0f;

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			int i = y * 4 + x;
//			std::cout << x << " " << y << " " << i << "Xc="<< x * 2 << "Yc=" << 1 + y * 2 << std::endl;
			Vec v = ui.getPosition(UI::KNOB, 1 + x * 2, y * 2, true, true);
			
			AHKnobSnap *divW = ParamWidget::create<AHKnobSnap>(v, module, Ruckus::DIV_PARAM + i, 0, 64, 0);
			AHParam::set<AHKnobSnap>(divW, "Div " + std::to_string(i), ParamEvent::INT);
			addParam(divW);

			AHTrimpotNoSnap *probW = ParamWidget::create<AHTrimpotNoSnap>(Vec(v.x + xd, v.y + yd), module, Ruckus::PROB_PARAM + i, 0.0f, 1.0f, 1.0f);
			AHParam::set<AHTrimpotNoSnap>(probW, "Prob " + std::to_string(i), ParamEvent::FP);
			addParam(probW);

			AHTrimpotSnap *shiftW = ParamWidget::create<AHTrimpotSnap>(Vec(v.x - xd + 4, v.y + yd), module, Ruckus::SHIFT_PARAM + i, -16.0f, 16.0f, 0.0f);
			AHParam::set<AHTrimpotSnap>(shiftW, "Shift " + std::to_string(i), ParamEvent::INT);
			addParam(shiftW);
		}
	}

	float d = 12.0f;

	// X out
	for (int x = 0; x < 4; x++) {
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 1 + x * 2, 8, true, true), Port::OUTPUT, module, Ruckus::XOUT_OUTPUT + x));
		
		Vec bVec = ui.getPosition(UI::BUTTON, 1 + x * 2, 7, true, true);
		bVec.y = bVec.y + d;
		AHButton *buttonW = ParamWidget::create<AHButton>(bVec, module, Ruckus::XMUTE_PARAM + x, 0.0, 1.0, 1.0);
		addParam(buttonW);
		
		Vec lVec = ui.getPosition(UI::LIGHT, 1 + x * 2, 7, true, true);
		lVec.y = lVec.y + d;
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(lVec, module, Ruckus::XMUTE_LIGHT + x));

	}

	for (int y = 0; y < 4; y++) {
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT,9, y * 2, true, true), Port::OUTPUT, module, Ruckus::YOUT_OUTPUT + y));

		Vec bVec = ui.getPosition(UI::BUTTON, 8, y * 2, true, true);
		bVec.x = bVec.x + d;		
		AHButton *buttonW = ParamWidget::create<AHButton>(bVec, module, Ruckus::YMUTE_PARAM + y, 0.0, 1.0, 1.0);
		addParam(buttonW);

		Vec lVec = ui.getPosition(UI::LIGHT, 8, y * 2, true, true);
		lVec.x = lVec.x + d;
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(lVec, module, Ruckus::YMUTE_LIGHT + y));

	}

}

Model *modelRuckus = Model::create<Ruckus, RuckusWidget>( "Amalgamated Harmonics", "Ruckus", "Ruckus", SEQUENCER_TAG);

