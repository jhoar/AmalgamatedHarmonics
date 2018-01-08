#include "dsp/digital.hpp"

#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"

#include <iostream>

struct Imperfect : Module {

	enum ParamIds {
		DELAY_PARAM,
		DELAYSPREAD_PARAM,
		DELAYSPREADSCALE_PARAM,
		LENGTH_PARAM = DELAYSPREADSCALE_PARAM + 8,
		LENGTHSPREAD_PARAM,
		LENGTHSPREADSCALE_PARAM,
		NUM_PARAMS = LENGTHSPREADSCALE_PARAM + 8
	};
	enum InputIds {
		TRIG_INPUT,
		DELAY_INPUT,
		DELAYSPREAD_INPUT,
		LENGTH_INPUT,
		LENGTHSPREAD_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS = OUT_OUTPUT + 8
	};
	enum LightIds {
		OUT_LIGHT,
		NUM_LIGHTS = OUT_LIGHT + 16
	};

	Imperfect() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	Core core;

	bool delayState[8];
	bool gateState[8];
	float delayTime[8];
	float gateTime[8];
	PulseGenerator delayPhase[8];
	PulseGenerator gatePhase[8];
	SchmittTrigger inTrigger;

	int stepX;
	
	inline bool debug() {
		return false;
	}
	
};

void Imperfect::step() {
	
	stepX++;
	
	float dlyLen;
	float dlySpr;
	float gateLen;
	float gateSpr;

	float delta = 1.0 / engineGetSampleRate();	

	if (inputs[DELAY_INPUT].active) {
		dlyLen = inputs[DELAY_INPUT].value;
	} else {
		dlyLen = params[DELAY_PARAM].value;
	}

	if (inputs[DELAYSPREAD_INPUT].active) {
		dlySpr = inputs[DELAYSPREAD_INPUT].value;
	} else {
		dlySpr = params[DELAYSPREAD_PARAM].value;
	}

	if (inputs[LENGTH_INPUT].active) {
		gateLen = inputs[LENGTH_INPUT].value;
	} else {
		gateLen = params[LENGTH_PARAM].value;
	}

	if (inputs[LENGTHSPREAD_INPUT].active) {
		gateSpr = inputs[LENGTHSPREAD_INPUT].value;
	} else {
		gateSpr = params[LENGTHSPREAD_PARAM].value;
	}
	
	if (inputs[TRIG_INPUT].active) {
	
		if (inTrigger.process(inputs[TRIG_INPUT].value)) {

		
			for (int i = 0; i < 8; i++) {			
			
				if (outputs[OUT_OUTPUT + i].active) {

					float dlyScale = params[DELAYSPREADSCALE_PARAM + i].value;
					float gateScale = params[LENGTHSPREADSCALE_PARAM + i].value;

					// Determine delay and gate times for all active outputs
					double rndD = clampf(core.gaussrand(), -2.0, 2.0);
					delayTime[i] = dlyLen + fabs(dlySpr * rndD * dlyScale);

					if (debug()) { 
						std::cout << stepX << " Delay: " << i << ": Len: " << dlyLen << " Spr: " << dlySpr << " Scr: " << dlyScale << " r: " << rndD << " = " << delayTime[i] << std::endl; 
					}
					
					// The modified gate time cannot be earlier than the start of the delay
					double rndG = clampf(core.gaussrand(), -2.0, 2.0);

					gateTime[i] = gateLen + fabs(gateSpr * rndG * gateScale);
					if (debug()) { 
						std::cout << stepX << " Gate: " << i << ": Len = " << gateLen << ", Spr: " << gateSpr << " Scl: " << gateScale << " r: " << rndG << " = " << gateTime[i] << std::endl; 
					}

					// Trigger the respective delay pulse generators
					delayState[i] = true;
					delayPhase[i].trigger(delayTime[i]);
				
				}	
			}
		}
	}

	
	for (int i = 0; i < 8; i++) {			
	
		if (delayState[i] && !delayPhase[i].process(delta)) {
			gatePhase[i].trigger(gateTime[i]);
			gateState[i] = true;
			delayState[i] = false;
		}
		
		if (gatePhase[i].process(delta)) {
			outputs[OUT_OUTPUT + i].value = 10.0;
			lights[OUT_LIGHT + i * 2].value = 1.0;
			lights[OUT_LIGHT + i * 2 + 1].value = 0.0;
		} else {
			outputs[OUT_OUTPUT + i].value = 0.0;
			if (delayState[i]) {
				lights[OUT_LIGHT + i * 2].value = 0.0;
				lights[OUT_LIGHT + i * 2 + 1].value = 1.0;
			} else {
				lights[OUT_LIGHT + i * 2].value = 0.0;
				lights[OUT_LIGHT + i * 2 + 1].value = 0.0;
			}
			
		}
			
	}
	

}

ImperfectWidget::ImperfectWidget() {
	Imperfect *module = new Imperfect();
	
	UI ui;
	
	setModule(module);
	box.size = Vec(240, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Imperfect.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 4, true, false), module, Imperfect::TRIG_INPUT));
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 4, true, false), module, Imperfect::DELAY_INPUT));
	addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 3, 4, true, false), module, Imperfect::DELAY_PARAM, 0.0, 2.0, 0.0));
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 4, true, false), module, Imperfect::DELAYSPREAD_INPUT));
	addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 5, 4, true, false), module, Imperfect::DELAYSPREAD_PARAM, 0.0, 2.0, 1.0));
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 5, true, false), module, Imperfect::LENGTH_INPUT));
	addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 3, 5, true, false), module, Imperfect::LENGTH_PARAM, 0.02, 2.0, 0.0));
	addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 5, true, false), module, Imperfect::LENGTHSPREAD_INPUT));
	addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 5, 5, true, false), module, Imperfect::LENGTHSPREAD_PARAM, 0.0, 2.0, 1.0)); 

	for (int i = 0; i < 8; i++) {
		addOutput(createOutput<PJ301MPort>(Vec(6 + i * 29, 61), module, Imperfect::OUT_OUTPUT + i));
		addParam(createParam<AHTrimpotNoSnap>(Vec(9 + i * 29.1, 91), module, Imperfect::DELAYSPREADSCALE_PARAM + i, 0.0, 2.0, 1.0));
		addParam(createParam<AHTrimpotNoSnap>(Vec(9 + i * 29.1, 116), module, Imperfect::LENGTHSPREADSCALE_PARAM + i, 0.0, 2.0, 1.0));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(13.5 + i * 29.1, 141), module, Imperfect::OUT_LIGHT + i * 2));
	}
	
	
	

}

