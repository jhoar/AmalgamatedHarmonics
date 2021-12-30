#include "common.hpp"
#include "dsp/noise.hpp"

#include "AH.hpp"
#include "AHCommon.hpp"

using namespace ah;

struct SLN : core::AHModule {

	enum ParamIds {
		SPEED_PARAM,
		SLOPE_PARAM,
		NOISE_PARAM,
		ATTN_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		TRIG_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NOISE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	SLN() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		configParam(SPEED_PARAM, 0.0, 1.0, 0.0, "Inertia", "%", 0.0f, 100.0f);
		paramQuantities[SPEED_PARAM]->description = "Resistance of the signal to change";

		configParam(SLOPE_PARAM, 0.0, 1.0, 0.0, "Slope");
		paramQuantities[SLOPE_PARAM]->description = "Linear to exponential slope";

		struct NoiseParamQuantity : engine::ParamQuantity {
			std::string getDisplayValueString() override {
				int v = (int)getValue();
				switch (v)
				{
					case 0:
						return "White noise " + ParamQuantity::getDisplayValueString();
						break;
					case 1:
						return "Pink noise " + ParamQuantity::getDisplayValueString();
						break;
					case 2:
						return "Brown noise " + ParamQuantity::getDisplayValueString();
						break;
					default:
						return ParamQuantity::getDisplayValueString();
						break;
				}
			}
		};
		configParam(NOISE_PARAM, 0.0, 2.0, 0.0, "Noise type");
		paramQuantities[NOISE_PARAM]->description = "White, pink (1/f) or brown (1/f^2) noise";

		configParam(ATTN_PARAM, 0.0, 1.0, 1.0, "Level", "%", 0.0f, 100.0f);
	}

	void process(const ProcessArgs &args) override;

	rack::dsp::SchmittTrigger inTrigger;
	bogaudio::dsp::WhiteNoiseGenerator white;
	bogaudio::dsp::PinkNoiseGenerator pink;
	bogaudio::dsp::RedNoiseGenerator brown;

	float target = 0.0f;
	float current = 0.0f;

	// minimum and maximum slopes in volts per second
	const float slewMin = 0.1;
	const float slewMax = 10000.0;	
	const float slewRatio = slewMin / slewMax;

	// Amount of extra slew per voltage difference
	const float shapeScale = 1.0/10.0;

};

void SLN::process(const ProcessArgs &args) {

	AHModule::step();

	float noise;
	int noiseType = params[NOISE_PARAM].getValue();
	float attn = params[ATTN_PARAM].getValue();

	switch(noiseType) {
		case 0:
			noise = clamp(white.next() * 10.0f, -10.0f, 10.f);
			break;
		case 1:
			noise = clamp(pink.next() * 15.0f, -10.0f, 10.f);
			break;
		case 2:
			noise = clamp(brown.next() * 35.0f, -10.0f, 10.f);
			break;
		default:
			noise = clamp(white.next() * 10.0f, -10.0f, 10.f);
	}

	// Capture noise
	if (inTrigger.process(inputs[TRIG_INPUT].getVoltage() / 0.7)) {
		target = noise;
	} 

	float shape = params[SLOPE_PARAM].getValue();
	float speed = params[SPEED_PARAM].getValue();

	float slew = slewMax * powf(slewRatio, speed);

	// Rise
	if (target > current) {
		current += slew * crossfade(1.0f, shapeScale * (target - current), shape) * args.sampleTime;
		if (current > target) // Trap overshoot
			current = target;
	}
	// Fall
	else if (target < current) {
		current -= slew * crossfade(1.0f, shapeScale * (current - target), shape) * args.sampleTime;
		if (current < target) // Trap overshoot
			current = target;
	}

	outputs[OUT_OUTPUT].setVoltage(current * attn);
	outputs[NOISE_OUTPUT].setVoltage(noise);

}

struct SLNWidget : ModuleWidget {

	SLNWidget(SLN *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SLN.svg")));

		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(26.104, 110.898), module, SLN::SPEED_PARAM));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(15.119, 146.814), module, SLN::SLOPE_PARAM));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(26.104, 180.814), module, SLN::NOISE_PARAM));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(15.119, 216.898), module, SLN::ATTN_PARAM));

		addInput(createInputCentered<gui::AHPort>(Vec(22.5, 59.301), module, SLN::TRIG_INPUT));

		addOutput(createOutputCentered<gui::AHPort>(Vec(22.5, 284.85), module, SLN::OUT_OUTPUT));
		addOutput(createOutputCentered<gui::AHPort>(Vec(22.5, 334.716), module, SLN::NOISE_OUTPUT));

	}

};

Model *modelSLN = createModel<SLN, SLNWidget>("SLN");

