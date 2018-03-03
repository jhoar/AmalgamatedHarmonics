#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"

#include <iostream>


struct Ruckus : Module {

	enum ParamIds {
		DIV_PARAM,
		PROB_PARAM = DIV_PARAM + 16,
		SHIFT_PARAM = PROB_PARAM + 16,
		NUM_PARAMS = SHIFT_PARAM + 16
	};
	enum InputIds {
		TRIG_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS = OUT_OUTPUT + 9
	};
	enum LightIds {
		OUT_LIGHT,
		NUM_LIGHTS = OUT_LIGHT + 9
	};

	float delta;
		
	Ruckus() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		reset();
		delta = 1.0 / engineGetSampleRate();
	}
	
	void onSampleRateChange() override { 
		delta = 1.0 / engineGetSampleRate();
	}

	void step() override;
	float calculateBPM(int index); // From ML
	
	void reset() override {	}
	
	Core core;

	float bpm;
	
	AHPulseGenerator xGate[4];
	AHPulseGenerator yGate[4];
	AHPulseGenerator allGate;
	
	SchmittTrigger inTrigger;

	int division[16];
	int shift[16];
	float prob[16];
	
	unsigned int counter = 0;

	int stepX;
	
	inline bool debug() {
		return false;
	}
	
	BpmCalculator bpmCalc;
		
};

void Ruckus::step() {
	
	stepX++;

	bool inputActive = inputs[TRIG_INPUT].active;
	bool haveTrigger = inTrigger.process(inputs[TRIG_INPUT].value);

	// If we have an active input, we should forget about previous valid inputs
	if (inputActive) {
		bpm = bpmCalc.calculateBPM(delta, inputs[TRIG_INPUT].value);
	} else {
		bpm = 0.0;
	}

	for (int i = 0; i < 16; i++) {
		division[i] = params[DIV_PARAM + i].value;
		prob[i] = params[PROB_PARAM + i].value;
		shift[i] = params[SHIFT_PARAM + i].value;
//		std::cout << i << " D=" << division[i] << std::endl;
	}
	
	if (haveTrigger) {

		counter++;

		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				int i = y * 4 + x;
						
				if(division[i] == 0) {
					continue; // 0 == skip
				}
	
				if (counter % (division[i] + shift[i]) == 0) { 
					float f = randomUniform();
					if (f < prob[i]) {
//					std::cout << counter << " X=" << x << " Y=" << y << " D=" << division[i] << std::endl;
						xGate[x].trigger(1e-2);
						yGate[y].trigger(1e-2);
						allGate.trigger(1e-2);
					}
				} 
			}
		}
	}

	
	for (int i = 0; i < 4; i++) {

		if(xGate[i].process(delta)) {
			outputs[OUT_OUTPUT + i].value = 10.0f;		
			lights[OUT_LIGHT + i].value = 1.0f;
		} else {
//			std::cout << counter << " SKIP X" << i << std::endl;
			outputs[OUT_OUTPUT + i].value = 0.0f;		
			lights[OUT_LIGHT + i].value = 0.0f;
		}

	}

	for (int i = 0; i < 4; i++) {

		if(yGate[i].process(delta)) {
			outputs[OUT_OUTPUT + 4 + i].value = 10.0f;		
			lights[OUT_LIGHT + 4 + i].value = 1.0f;
		} else {
//			std::cout << counter << " SKIP Y" << i << std::endl;
			outputs[OUT_OUTPUT + 4 + i].value = 0.0f;		
			lights[OUT_LIGHT + 4 + i].value = 0.0f;
		}

	}

	if(allGate.process(delta)) {
		outputs[OUT_OUTPUT + 8].value = 10.0f;		
		lights[OUT_LIGHT + 8].value = 1.0f;
	} else {
		outputs[OUT_OUTPUT + 8].value = 0.0f;		
		lights[OUT_LIGHT + 8].value = 0.0f;		
	}
	
}

struct RuckusWidget : ModuleWidget {
	RuckusWidget(Ruckus *module);
};

RuckusWidget::RuckusWidget(Ruckus *module) : ModuleWidget(module) {
	
	UI ui;
	
	box.size = Vec(450, 380);

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
	
	addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 11, 2, true, true), Port::INPUT, module, Ruckus::TRIG_INPUT));

	float xd = 18.0f;
	float yd = 20.0f;

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			int i = y * 4 + x;
//			std::cout << x << " " << y << " " << i << "Xc="<< x * 2 << "Yc=" << 1 + y * 2 << std::endl;
			Vec v = ui.getPosition(UI::KNOB, 1 + x * 2, y * 2, true, true);
			addParam(ParamWidget::create<AHKnobSnap>(v, module, Ruckus::DIV_PARAM + i, 0, 64, 0));
			addParam(ParamWidget::create<AHTrimpotNoSnap>(Vec(v.x + xd, v.y + yd), module, Ruckus::PROB_PARAM + i, 0.0f, 1.0f, 1.0f));
			addParam(ParamWidget::create<AHTrimpotSnap>(Vec(v.x - xd, v.y + yd), module, Ruckus::SHIFT_PARAM + i, -16.0f, 16.0f, 0.0f));
			
		}
	}


	// X out
	for (int x = 0; x < 4; x++) {
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 1 + x * 2, 8, true, true), Port::OUTPUT, module, Ruckus::OUT_OUTPUT + x));
	}

	for (int y = 0; y < 4; y++) {
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT,9, y * 2, true, true), Port::OUTPUT, module, Ruckus::OUT_OUTPUT + 4 + y));
	}

	addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT,9, 8, true, true), Port::OUTPUT, module, Ruckus::OUT_OUTPUT + 8));

	
}

Model *modelRuckus = Model::create<Ruckus, RuckusWidget>( "Amalgamated Harmonics", "Ruckus", "Ruckus", SEQUENCER_TAG);

