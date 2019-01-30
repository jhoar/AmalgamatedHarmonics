#include "common.hpp"

#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"

struct Ruckus : AHModule {

	enum ParamIds {
		ENUMS(DIV_PARAM,16),
		ENUMS(PROB_PARAM,16),
		ENUMS(SHIFT_PARAM,16),
		ENUMS(XMUTE_PARAM,16), 
		ENUMS(YMUTE_PARAM,4),
		NUM_PARAMS
	};
	enum InputIds {
		TRIG_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(XOUT_OUTPUT,4),
		ENUMS(YOUT_OUTPUT,4),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(XMUTE_LIGHT,4), 
		ENUMS(YMUTE_LIGHT,4), 
		ENUMS(ACTIVE_LIGHT,16),
		ENUMS(TRIG_LIGHT,16),
		NUM_LIGHTS
	};

	Ruckus() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

		for (int y = 0; y < 4; y++) {

			params[XMUTE_PARAM + y].config(0.0, 1.0, 0.0);
			params[YMUTE_PARAM + y].config(0.0, 1.0, 0.0);

			for (int x = 0; x < 4; x++) {
				int i = y * 4 + x;
				params[DIV_PARAM + i].config(0, 64, 0);
				params[PROB_PARAM + i].config(0.0f, 1.0f, 1.0f);
				params[SHIFT_PARAM + i].config(-64.0f, 64.0f, 0.0f);
			}
		}

		onReset();
	}
	
	void step() override;

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// gates
		json_t *xMutesJ = json_array();
		json_t *yMutesJ = json_array();
		
		for (int i = 0; i < 4; i++) {
			json_t *xMuteJ = json_integer((int) xMute[i]);
			json_array_append_new(xMutesJ, xMuteJ);

			json_t *yMuteJ = json_integer((int) yMute[i]);
			json_array_append_new(yMutesJ, yMuteJ);
		}
		
		json_object_set_new(rootJ, "xMutes", xMutesJ);
		json_object_set_new(rootJ, "yMutes", yMutesJ);

		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override {
		// gates
		json_t *xMutesJ = json_object_get(rootJ, "xMutes");
		if (xMutesJ) {
			for (int i = 0; i < 4; i++) {
				json_t *xMuteJ = json_array_get(xMutesJ, i);
				if (xMuteJ)
					xMute[i] = !!json_integer_value(xMuteJ);
			}
		}

		json_t *yMutesJ = json_object_get(rootJ, "yMutes");
		if (yMutesJ) {
			for (int i = 0; i < 4; i++) {
				json_t *yMuteJ = json_array_get(yMutesJ, i);
				if (yMuteJ)
					yMute[i] = !!json_integer_value(yMuteJ);
			}
		}
	}

	enum ParamType {
		DIV_TYPE,
		SHIFT_TYPE,
		PROB_TYPE
	};
	
	void receiveEvent(ParamEvent e) override {
		if (receiveEvents) {
			switch(e.pType) {
				case ParamType::DIV_TYPE: 
					paramState = "> Division: " + std::to_string((int)e.value);
					break;
				case ParamType::SHIFT_TYPE: 
					paramState = "> Beat shift: " + std::to_string((int)e.value);
					break;
				case ParamType::PROB_TYPE:
					paramState = "> Probability: " + std::to_string(e.value * 100.0f).substr(0,6) + "%";
					break;
				default:
					paramState = "> UNK:" + std::to_string(e.value).substr(0,6);
			}
		}
		keepStateDisplay = 0;
	}
	
	
	void onReset() override {	
		for (int i = 0; i < 4; i++) {
			xMute[i] = true;
			yMute[i] = true;
		}
	}	
	
	Core core;

	AHPulseGenerator xGate[4];
	AHPulseGenerator yGate[4];
	
	bool xMute[4] = {true, true, true, true};
	bool yMute[4] = {true, true, true, true};
	
	dsp::SchmittTrigger xLockTrigger[4];
	dsp::SchmittTrigger yLockTrigger[4];

	dsp::SchmittTrigger inTrigger;
	dsp::SchmittTrigger resetTrigger;

	int division[16];
	int shift[16];
	float prob[16];
	int state[16];

	unsigned int beatCounter = 0;
	
};

void Ruckus::step() {
	
	AHModule::step();
		
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

	if (resetTrigger.process(inputs[RESET_INPUT].value)) {
		beatCounter = 0;
	}

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			int i = y * 4 + x;

			if (division[i] == 0) {
				state[i] = 0;
			} else {
				state[i] = 1;
			}

		}
	}

	if (inTrigger.process(inputs[TRIG_INPUT].value)) {

		beatCounter++;

		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				int i = y * 4 + x;

				if(division[i] == 0) { // 0 == skip
					continue;
				}
				
				int target = beatCounter + shift[i];
				
				if (target < 0) { // shifted into negative count 
					continue; 
				}
				
				if (target % division[i] == 0) { 
					if (random::uniform() < prob[i]) {
						xGate[x].trigger(Core::TRIGGER);
						yGate[y].trigger(Core::TRIGGER);
						state[i] = 2;
					}
				}
			}
		}
	}

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			int i = y * 4 + x;

			switch (state[i]) {
			case 0: 
				lights[ACTIVE_LIGHT + i].setBrightnessSmooth(0.0f);
				lights[TRIG_LIGHT + i].setBrightnessSmooth(0.0f);
				break;
			case 1:
				lights[ACTIVE_LIGHT + i].setBrightnessSmooth(1.0f);
				lights[TRIG_LIGHT + i].setBrightnessSmooth(0.0f);
				break;
			case 2:
				lights[ACTIVE_LIGHT + i].setBrightnessSmooth(1.0f);
				lights[TRIG_LIGHT + i].setBrightnessSmooth(1.0f);
				break;
			default:
				lights[ACTIVE_LIGHT + i].setBrightnessSmooth(0.0f);
				lights[TRIG_LIGHT + i].setBrightnessSmooth(0.0f);
			}

		}
	}


	for (int i = 0; i < 4; i++) {

		if (xGate[i].process(delta) && xMute[i]) {
			outputs[XOUT_OUTPUT + i].value = 10.0f;		
		} else {
			outputs[XOUT_OUTPUT + i].value = 0.0f;		
		}
		
		lights[XMUTE_LIGHT + i].value = xMute[i] ? 1.0 : 0.0;

		if (yGate[i].process(delta) && yMute[i]) {
			outputs[YOUT_OUTPUT + i].value = 10.0f;		
		} else {
			outputs[YOUT_OUTPUT + i].value = 0.0f;		
		}
		
		lights[YMUTE_LIGHT + i].value = yMute[i] ? 1.0 : 0.0;

	}
	
}

struct RuckusWidget : ModuleWidget {

	RuckusWidget(Ruckus *module) {
	
		setModule(module);
		setPanel(SVG::load(asset::plugin(plugin, "res/Ruckus.svg")));
		UI ui;

		//299.5 329.7
		Vec a = ui.getPosition(UI::PORT, 6, 5, false, false);
		a.x = 312.0;

		//325.5 329.7
		Vec b = ui.getPosition(UI::PORT, 7, 5, false, false);
		b.x = 352.0;
		
		addInput(createInput<PJ301MPort>(a, module, Ruckus::TRIG_INPUT));
		addInput(createInput<PJ301MPort>(b, module, Ruckus::RESET_INPUT));

		float xd = 18.0f;
		float yd = 20.0f;

		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				int i = y * 4 + x;
				Vec v = ui.getPosition(UI::KNOB, 1 + x * 2, y * 2, true, true);
				
				AHKnobSnap *divW = createParam<AHKnobSnap>(v, module, Ruckus::DIV_PARAM + i);
				AHParamWidget::set<AHKnobSnap>(divW, Ruckus::DIV_TYPE, i);
				addParam(divW);

				AHTrimpotNoSnap *probW = createParam<AHTrimpotNoSnap>(Vec(v.x + xd, v.y + yd), module, Ruckus::PROB_PARAM + i);
				AHParamWidget::set<AHTrimpotNoSnap>(probW, Ruckus::PROB_TYPE, i);
				addParam(probW);

				AHTrimpotSnap *shiftW = createParam<AHTrimpotSnap>(Vec(v.x - xd + 4, v.y + yd), module, Ruckus::SHIFT_PARAM + i);
				AHParamWidget::set<AHTrimpotSnap>(shiftW, Ruckus::SHIFT_TYPE, i);
				addParam(shiftW);

				addChild(createLight<MediumLight<GreenLight>>(Vec(v.x - xd + 5, v.y - yd + 12), module, Ruckus::ACTIVE_LIGHT + i));
				addChild(createLight<MediumLight<RedLight>>(Vec(v.x + xd + 7, v.y - yd + 12), module, Ruckus::TRIG_LIGHT + i));

			}
		}

		float d = 12.0f;

		for (int x = 0; x < 4; x++) {
			addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 1 + x * 2, 8, true, true), module, Ruckus::XOUT_OUTPUT + x));
			
			Vec bVec = ui.getPosition(UI::BUTTON, 1 + x * 2, 7, true, true);
			bVec.y = bVec.y + d;
			addParam(createParam<AHButton>(bVec, module, Ruckus::XMUTE_PARAM + x));
			
			Vec lVec = ui.getPosition(UI::LIGHT, 1 + x * 2, 7, true, true);
			lVec.y = lVec.y + d;
			addChild(createLight<MediumLight<GreenLight>>(lVec, module, Ruckus::XMUTE_LIGHT + x));

		}

		for (int y = 0; y < 4; y++) {
			addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT,9, y * 2, true, true), module, Ruckus::YOUT_OUTPUT + y));

			Vec bVec = ui.getPosition(UI::BUTTON, 8, y * 2, true, true);
			bVec.x = bVec.x + d;		
			addParam(createParam<AHButton>(bVec, module, Ruckus::YMUTE_PARAM + y));

			Vec lVec = ui.getPosition(UI::LIGHT, 8, y * 2, true, true);
			lVec.x = lVec.x + d;
			addChild(createLight<MediumLight<GreenLight>>(lVec, module, Ruckus::YMUTE_LIGHT + y));

		}

		if (module != NULL) {
			StateDisplay *display = createWidget<StateDisplay>(mm2px(Vec(30, 335)));
			display->module = module;
			display->box.size = Vec(100, 140);
			addChild(display);
		}

	}
};

Model *modelRuckus = createModel<Ruckus, RuckusWidget>("Ruckus");

