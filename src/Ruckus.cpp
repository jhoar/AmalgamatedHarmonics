#include "common.hpp"

#include "AH.hpp"
#include "AHCommon.hpp"

#include <array>

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
				configParam(DIV_PARAM + i, 0, 64, 0, "[" + std::to_string(x) + "," + std::to_string(y) + "] Clock division");

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
				if (xMuteJ)	xMute[i] = !!json_integer_value(xMuteJ);
			}
		}

		json_t *yMutesJ = json_object_get(rootJ, "yMutes");
		if (yMutesJ) {
			for (int i = 0; i < 4; i++) {
				json_t *yMuteJ = json_array_get(yMutesJ, i);
				if (yMuteJ)	yMute[i] = !!json_integer_value(yMuteJ);
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

	std::array<digital::AHPulseGenerator,4> xGate;
	std::array<digital::AHPulseGenerator,4> yGate;

	std::array<bool,4> xMute {{true, true, true, true}};
	std::array<bool,4> yMute {{true, true, true, true}};

	std::array<rack::dsp::SchmittTrigger,4> xLockTrigger;
	std::array<rack::dsp::SchmittTrigger,4> yLockTrigger;

	rack::dsp::SchmittTrigger inTrigger;
	rack::dsp::SchmittTrigger resetTrigger;

	std::array<int,16> division;
	std::array<int,16> shift;
	std::array<float,16> prob;
	std::array<int,16> state;

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
				state[i] = 0; // Not active
			} else {
				state[i] = 1; // Active
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
						state[i] = 2; // Triggered
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

		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(32.738, 44.488), module, Ruckus::DIV_PARAM + 0));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(102.738, 44.488), module, Ruckus::DIV_PARAM + 1));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(172.738, 44.488), module, Ruckus::DIV_PARAM + 2));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(242.738, 44.488), module, Ruckus::DIV_PARAM + 3));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(32.738, 113.176), module, Ruckus::DIV_PARAM + 4));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(102.738, 113.176), module, Ruckus::DIV_PARAM + 5));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(172.738, 113.176), module, Ruckus::DIV_PARAM + 6));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(242.738, 113.176), module, Ruckus::DIV_PARAM + 7));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(32.738, 181.683), module, Ruckus::DIV_PARAM + 8));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(102.738, 181.683), module, Ruckus::DIV_PARAM + 9));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(172.738, 181.683), module, Ruckus::DIV_PARAM + 10));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(242.738, 181.683), module, Ruckus::DIV_PARAM + 11));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(32.738, 252.313), module, Ruckus::DIV_PARAM + 12));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(102.738, 252.313), module, Ruckus::DIV_PARAM + 13));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(172.738, 252.313), module, Ruckus::DIV_PARAM + 14));
		addParam(createParamCentered<gui::AHBigKnobSnap>(Vec(242.738, 252.313), module, Ruckus::DIV_PARAM + 15));

		addParam(createParamCentered<gui::AHKnobSnap>(Vec(16.607, 72.488), module, Ruckus::SHIFT_PARAM + 0));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(86.607, 72.488), module, Ruckus::SHIFT_PARAM + 1));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(156.607, 72.488), module, Ruckus::SHIFT_PARAM + 2));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(226.607, 72.488), module, Ruckus::SHIFT_PARAM + 3));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(16.607, 141.176), module, Ruckus::SHIFT_PARAM + 4));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(86.607, 141.176), module, Ruckus::SHIFT_PARAM + 5));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(156.607, 141.176), module, Ruckus::SHIFT_PARAM + 6));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(226.607, 141.176), module, Ruckus::SHIFT_PARAM + 7));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(16.607, 209.683), module, Ruckus::SHIFT_PARAM + 8));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(86.607, 209.683), module, Ruckus::SHIFT_PARAM + 9));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(156.607, 209.683), module, Ruckus::SHIFT_PARAM + 10));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(226.607, 209.683), module, Ruckus::SHIFT_PARAM + 11));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(16.607, 280.313), module, Ruckus::SHIFT_PARAM + 12));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(86.607, 280.313), module, Ruckus::SHIFT_PARAM + 13));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(156.607, 280.313), module, Ruckus::SHIFT_PARAM + 14));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(226.607, 280.313), module, Ruckus::SHIFT_PARAM + 15));

		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(48.868, 72.488), module, Ruckus::PROB_PARAM + 0));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(118.869, 72.488), module, Ruckus::PROB_PARAM + 1));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(188.869, 72.488), module, Ruckus::PROB_PARAM + 2));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(258.868, 72.488), module, Ruckus::PROB_PARAM + 3));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(48.868, 141.176), module, Ruckus::PROB_PARAM + 4));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(118.869, 141.176), module, Ruckus::PROB_PARAM + 5));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(188.869, 141.176), module, Ruckus::PROB_PARAM + 6));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(258.868, 141.176), module, Ruckus::PROB_PARAM + 7));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(48.868, 209.683), module, Ruckus::PROB_PARAM + 8));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(118.869, 209.683), module, Ruckus::PROB_PARAM + 9));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(188.869, 209.683), module, Ruckus::PROB_PARAM + 10));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(258.868, 209.683), module, Ruckus::PROB_PARAM + 11));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(48.868, 280.313), module, Ruckus::PROB_PARAM + 12));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(118.869, 280.313), module, Ruckus::PROB_PARAM + 13));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(188.869, 280.313), module, Ruckus::PROB_PARAM + 14));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(258.868, 280.313), module, Ruckus::PROB_PARAM + 15));

		addParam(createParamCentered<gui::AHButton>(Vec(32.738, 305.683), module, Ruckus::XMUTE_PARAM + 0));
		addParam(createParamCentered<gui::AHButton>(Vec(102.738, 305.683), module, Ruckus::XMUTE_PARAM + 1));
		addParam(createParamCentered<gui::AHButton>(Vec(172.738, 305.683), module, Ruckus::XMUTE_PARAM + 2));
		addParam(createParamCentered<gui::AHButton>(Vec(242.738, 305.683), module, Ruckus::XMUTE_PARAM + 3));

		addParam(createParamCentered<gui::AHButton>(Vec(290.804, 44.488), module, Ruckus::YMUTE_PARAM + 0));
		addParam(createParamCentered<gui::AHButton>(Vec(290.804, 113.176), module, Ruckus::YMUTE_PARAM + 1));
		addParam(createParamCentered<gui::AHButton>(Vec(290.804, 181.683), module, Ruckus::YMUTE_PARAM + 2));
		addParam(createParamCentered<gui::AHButton>(Vec(290.804, 251.683), module, Ruckus::YMUTE_PARAM + 3));

		addInput(createInputCentered<gui::AHPort>(Vec(382.293, 62.155), module, Ruckus::POLY_DIV_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(382.293, 119.244), module, Ruckus::POLY_PROB_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(382.293, 176.332), module, Ruckus::POLY_SHIFT_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(382.293, 237.318), module, Ruckus::TRIG_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(382.293, 294.407), module, Ruckus::RESET_INPUT));

		addOutput(createOutputCentered<gui::AHPort>(Vec(320.981, 44.579), module, Ruckus::YOUT_OUTPUT + 0));
		addOutput(createOutputCentered<gui::AHPort>(Vec(320.981, 113.267), module, Ruckus::YOUT_OUTPUT + 1));
		addOutput(createOutputCentered<gui::AHPort>(Vec(320.981, 181.774), module, Ruckus::YOUT_OUTPUT + 2));
		addOutput(createOutputCentered<gui::AHPort>(Vec(320.981, 251.774), module, Ruckus::YOUT_OUTPUT + 3));

		addOutput(createOutputCentered<gui::AHPort>(Vec(32.738, 332.826), module, Ruckus::XOUT_OUTPUT + 0));
		addOutput(createOutputCentered<gui::AHPort>(Vec(102.738, 332.826), module, Ruckus::XOUT_OUTPUT + 1));
		addOutput(createOutputCentered<gui::AHPort>(Vec(172.738, 332.826), module, Ruckus::XOUT_OUTPUT + 2));
		addOutput(createOutputCentered<gui::AHPort>(Vec(242.738, 332.826), module, Ruckus::XOUT_OUTPUT + 3));

		addChild(createLightCentered<SmallLight<RedLight>>(Vec(52.579, 30.329), module, Ruckus::TRIG_LIGHT + 0));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(122.579, 30.329), module, Ruckus::TRIG_LIGHT + 1));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(192.579, 30.329), module, Ruckus::TRIG_LIGHT + 2));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(262.579, 30.329), module, Ruckus::TRIG_LIGHT + 3));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(52.579, 99.017), module, Ruckus::TRIG_LIGHT + 4));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(122.579, 99.017), module, Ruckus::TRIG_LIGHT + 5));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(192.579, 99.017), module, Ruckus::TRIG_LIGHT + 6));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(262.579, 99.017), module, Ruckus::TRIG_LIGHT + 7));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(52.579, 167.524), module, Ruckus::TRIG_LIGHT + 8));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(122.579, 167.524), module, Ruckus::TRIG_LIGHT + 9));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(192.579, 167.524), module, Ruckus::TRIG_LIGHT + 10));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(262.579, 167.524), module, Ruckus::TRIG_LIGHT + 11));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(52.579, 238.155), module, Ruckus::TRIG_LIGHT + 12));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(122.579, 238.155), module, Ruckus::TRIG_LIGHT + 13));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(192.579, 238.155), module, Ruckus::TRIG_LIGHT + 14));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(262.579, 238.155), module, Ruckus::TRIG_LIGHT + 15));

		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(12.896, 30.329), module, Ruckus::ACTIVE_LIGHT + 0));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(82.897, 30.329), module, Ruckus::ACTIVE_LIGHT + 1));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(152.897, 30.329), module, Ruckus::ACTIVE_LIGHT + 2));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(222.896, 30.329), module, Ruckus::ACTIVE_LIGHT + 3));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(12.896, 99.017), module, Ruckus::ACTIVE_LIGHT + 4));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(82.897, 99.017), module, Ruckus::ACTIVE_LIGHT + 5));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(152.897, 99.017), module, Ruckus::ACTIVE_LIGHT + 6));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(222.896, 99.017), module, Ruckus::ACTIVE_LIGHT + 7));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(12.896, 167.524), module, Ruckus::ACTIVE_LIGHT + 8));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(82.897, 167.524), module, Ruckus::ACTIVE_LIGHT + 9));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(152.897, 167.524), module, Ruckus::ACTIVE_LIGHT + 10));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(222.896, 167.524), module, Ruckus::ACTIVE_LIGHT + 11));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(12.896, 238.155), module, Ruckus::ACTIVE_LIGHT + 12));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(82.897, 238.155), module, Ruckus::ACTIVE_LIGHT + 13));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(152.897, 238.155), module, Ruckus::ACTIVE_LIGHT + 14));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(222.896, 238.155), module, Ruckus::ACTIVE_LIGHT + 15));

		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(32.738, 305.683), module, Ruckus::XMUTE_LIGHT + 0));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(102.738, 305.683), module, Ruckus::XMUTE_LIGHT + 1));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(172.738, 305.683), module, Ruckus::XMUTE_LIGHT + 2));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(242.738, 305.683), module, Ruckus::XMUTE_LIGHT + 3));

		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(290.804, 44.488), module, Ruckus::YMUTE_LIGHT + 0));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(290.804, 113.176), module, Ruckus::YMUTE_LIGHT + 1));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(290.804, 181.683), module, Ruckus::YMUTE_LIGHT + 2));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(290.804, 251.683), module, Ruckus::YMUTE_LIGHT + 3));

		if (module != NULL) {
			gui::StateDisplay *display = createWidget<gui::StateDisplay>(Vec(30, 335));
			display->module = module;
			display->box.size = Vec(100, 140);
			addChild(display);
		}

	}
};

Model *modelRuckus = createModel<Ruckus, RuckusWidget>("Ruckus");

