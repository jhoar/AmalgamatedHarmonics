#include "common.hpp"

#include "AH.hpp"
#include "AHCommon.hpp"

using namespace ah;

struct Ruckus : core::AHModule {

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
		POLY_DIV_INPUT,
		POLY_PROB_INPUT,
		POLY_SHIFT_INPUT,
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

	Ruckus() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

		for (int y = 0; y < 4; y++) {

			configParam(XMUTE_PARAM + y, 0.0, 1.0, 0.0, "Output active");
			paramQuantities[XMUTE_PARAM + y]->description = "Output clock-chain";

			configParam(YMUTE_PARAM + y, 0.0, 1.0, 0.0, "Output active");
			paramQuantities[XMUTE_PARAM + y]->description = "Output clock-chain";

			for (int x = 0; x < 4; x++) {
				int i = y * 4 + x;
				configParam(DIV_PARAM + i, 0, 64, 0, "Clock division");

				configParam(PROB_PARAM + i, 0.0f, 1.0f, 1.0f, "Clock-tick probability", "%", 0.0f, 100.0f);

				configParam(SHIFT_PARAM + i, -64.0f, 64.0f, 0.0f, "Clock shift");
				paramQuantities[SHIFT_PARAM + i]->description = "Relative clock shift w.r.t. master clock";
			}
		}

		onReset();

	}
	
	void process(const ProcessArgs &args) override;

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
	
	void receiveEvent(core::ParamEvent e) override {
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
	
	digital::AHPulseGenerator xGate[4];
	digital::AHPulseGenerator yGate[4];
	
	bool xMute[4] = {true, true, true, true};
	bool yMute[4] = {true, true, true, true};
	
	rack::dsp::SchmittTrigger xLockTrigger[4];
	rack::dsp::SchmittTrigger yLockTrigger[4];

	rack::dsp::SchmittTrigger inTrigger;
	rack::dsp::SchmittTrigger resetTrigger;

	int division[16];
	int shift[16];
	float prob[16];
	int state[16];

	unsigned int beatCounter = 0;
	
};

void Ruckus::process(const ProcessArgs &args) {
	
	AHModule::step();
		
	for (int i = 0; i < 4; i++) {

		if (xLockTrigger[i].process(params[XMUTE_PARAM + i].getValue())) {
			xMute[i] = !xMute[i];
		}
		if (yLockTrigger[i].process(params[YMUTE_PARAM + i].getValue())) {
			yMute[i] = !yMute[i];
		}
	}

	for (int i = 0; i < 16; i++) {
		division[i] = clamp((int)(params[DIV_PARAM + i].getValue() + (inputs[POLY_DIV_INPUT].getVoltage(i) * 6.4f)), 0, 64);
		prob[i] = clamp(params[PROB_PARAM + i].getValue() + (inputs[POLY_PROB_INPUT].getVoltage(i) * 0.1f), 0.0f, 1.0f);
		shift[i] = clamp((int)(params[SHIFT_PARAM + i].getValue() + (inputs[POLY_SHIFT_INPUT].getVoltage(i) * 12.8f)), -64, 64);
	}

	if (resetTrigger.process(inputs[RESET_INPUT].getVoltage())) {
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

	if (inTrigger.process(inputs[TRIG_INPUT].getVoltage())) {

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
						xGate[x].trigger(digital::TRIGGER);
						yGate[y].trigger(digital::TRIGGER);
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
				lights[ACTIVE_LIGHT + i].setSmoothBrightness(0.0f, args.sampleTime);
				lights[TRIG_LIGHT + i].setSmoothBrightness(0.0f, args.sampleTime);
				break;
			case 1:
				lights[ACTIVE_LIGHT + i].setSmoothBrightness(1.0f, args.sampleTime);
				lights[TRIG_LIGHT + i].setSmoothBrightness(0.0f, args.sampleTime);
				break;
			case 2:
				lights[ACTIVE_LIGHT + i].setSmoothBrightness(1.0f, args.sampleTime);
				lights[TRIG_LIGHT + i].setSmoothBrightness(1.0f, args.sampleTime);
				break;
			default:
				lights[ACTIVE_LIGHT + i].setSmoothBrightness(0.0f, args.sampleTime);
				lights[TRIG_LIGHT + i].setSmoothBrightness(0.0f, args.sampleTime);
			}

		}
	}


	for (int i = 0; i < 4; i++) {

		if (xGate[i].process(args.sampleTime) && xMute[i]) {
			outputs[XOUT_OUTPUT + i].setVoltage(10.0f);		
		} else {
			outputs[XOUT_OUTPUT + i].setVoltage(0.0f);		
		}
		
		lights[XMUTE_LIGHT + i].setBrightness(xMute[i] ? 1.0 : 0.0);

		if (yGate[i].process(args.sampleTime) && yMute[i]) {
			outputs[YOUT_OUTPUT + i].setVoltage(10.0f);		
		} else {
			outputs[YOUT_OUTPUT + i].setVoltage(0.0f);		
		}
		
		lights[YMUTE_LIGHT + i].setBrightness(yMute[i] ? 1.0 : 0.0);

	}
	
}

struct RuckusWidget : ModuleWidget {

	RuckusWidget(Ruckus *module) {
	
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Ruckus.svg")));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 10, 2, true, true), module, Ruckus::POLY_DIV_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 10, 4, true, true), module, Ruckus::POLY_PROB_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 10, 6, true, true), module, Ruckus::POLY_SHIFT_INPUT));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 8, 8, true, true), module, Ruckus::TRIG_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 10, 8, true, true), module, Ruckus::RESET_INPUT));


		float xd = 18.0f;
		float yd = 20.0f;

		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				int i = y * 4 + x;
				Vec v = gui::getPosition(gui::KNOB, x * 2, y * 2, true, true);
				
				gui::AHKnobSnap *divW = createParam<gui::AHKnobSnap>(v, module, Ruckus::DIV_PARAM + i);
				gui::AHParamWidget::set<gui::AHKnobSnap>(divW, Ruckus::DIV_TYPE, i);
				addParam(divW);

				gui::AHTrimpotNoSnap *probW = createParam<gui::AHTrimpotNoSnap>(Vec(v.x + xd, v.y + yd), module, Ruckus::PROB_PARAM + i);
				gui::AHParamWidget::set<gui::AHTrimpotNoSnap>(probW, Ruckus::PROB_TYPE, i);
				addParam(probW);

				gui::AHTrimpotSnap *shiftW = createParam<gui::AHTrimpotSnap>(Vec(v.x - xd + 4, v.y + yd), module, Ruckus::SHIFT_PARAM + i);
				gui::AHParamWidget::set<gui::AHTrimpotSnap>(shiftW, Ruckus::SHIFT_TYPE, i);
				addParam(shiftW);

				addChild(createLight<MediumLight<GreenLight>>(Vec(v.x - xd + 5, v.y - yd + 12), module, Ruckus::ACTIVE_LIGHT + i));
				addChild(createLight<MediumLight<RedLight>>(Vec(v.x + xd + 7, v.y - yd + 12), module, Ruckus::TRIG_LIGHT + i));

			}
		}

		float d = 12.0f;

		for (int x = 0; x < 4; x++) {
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, x * 2, 8, true, true), module, Ruckus::XOUT_OUTPUT + x));
			
			Vec bVec = gui::getPosition(gui::BUTTON, x * 2, 7, true, true, 0.0, d);
			addParam(createParam<gui::AHButton>(bVec, module, Ruckus::XMUTE_PARAM + x));
			
			Vec lVec = gui::getPosition(gui::LIGHT, x * 2, 7, true, true, 0.0, d);
			addChild(createLight<MediumLight<GreenLight>>(lVec, module, Ruckus::XMUTE_LIGHT + x));

		}

		for (int y = 0; y < 4; y++) {
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT,8, y * 2, true, true), module, Ruckus::YOUT_OUTPUT + y));

			Vec bVec = gui::getPosition(gui::BUTTON, 7, y * 2, true, true, d, 0.0f);
			addParam(createParam<gui::AHButton>(bVec, module, Ruckus::YMUTE_PARAM + y));

			Vec lVec = gui::getPosition(gui::LIGHT, 7, y * 2, true, true, d, 0.0f);
			addChild(createLight<MediumLight<GreenLight>>(lVec, module, Ruckus::YMUTE_LIGHT + y));

		}

		if (module != NULL) {
			gui::StateDisplay *display = createWidget<gui::StateDisplay>(Vec(30, 335));
			display->module = module;
			display->box.size = Vec(100, 140);
			addChild(display);
		}

	}
};

Model *modelRuckus = createModel<Ruckus, RuckusWidget>("Ruckus");

