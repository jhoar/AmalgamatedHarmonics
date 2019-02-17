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
		params[SPEED_PARAM].config(0.0, 1.0, 0.0, "Inertia", "%", 0.0f, 100.0f);
		params[SPEED_PARAM].description = "Resistance of the signal to change";

		params[SLOPE_PARAM].config(0.0, 1.0, 0.0, "Slope");
		params[SLOPE_PARAM].description = "Linear to exponential slope";

		struct NoiseParamQuantity : app::ParamQuantity {
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
		params[NOISE_PARAM].config<NoiseParamQuantity>(0.0, 2.0, 0.0, "Noise type");
		params[NOISE_PARAM].description = "White, pink (1/f) or brown (1/f^2) noise";

		params[ATTN_PARAM].config(0.0, 1.0, 1.0, "Level", "%", 0.0f, 100.0f);
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
	
	core::AHModule::step();
	
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
		current += slew * crossfade(1.0f, shapeScale * (target - current), shape) * delta;
		if (current > target) // Trap overshoot
			current = target;
	}
	// Fall
	else if (target < current) {
		current -= slew * crossfade(1.0f, shapeScale * (current - target), shape) * delta;
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

		float panelwidth = 45.0;
		float portwidth = 25.0;
		float portX = (panelwidth - portwidth) / 2.0;

		Vec p1 = gui::getPosition(gui::PORT, 0, 0, false, false);
		p1.x = portX;
		addInput(createInput<PJ301MPort>(p1, module, SLN::TRIG_INPUT));
			
		Vec k1 = gui::getPosition(gui::PORT, 0, 2, false, true);
		k1.x = 20;
		addParam(createParam<gui::AHKnobNoSnap>(k1, module, SLN::SPEED_PARAM));

		Vec k2 = gui::getPosition(gui::PORT, 0, 3, false, true);
		k2.x = 3;
		addParam(createParam<gui::AHKnobNoSnap>(k2, module, SLN::SLOPE_PARAM));

		Vec k3 = gui::getPosition(gui::PORT, 0, 4, false, true);
		k3.x = 20;
		addParam(createParam<gui::AHKnobSnap>(k3, module, SLN::NOISE_PARAM));

		Vec k4 = gui::getPosition(gui::PORT, 0, 5, false, true);
		k4.x = 3;
		addParam(createParam<gui::AHKnobNoSnap>(k4, module, SLN::ATTN_PARAM)); 	

		Vec p2 = gui::getPosition(gui::PORT, 0, 4, false, false);
		p2.x = portX;	
		addOutput(createOutput<PJ301MPort>(p2, module, SLN::OUT_OUTPUT));

		Vec p3 = gui::getPosition(gui::PORT, 0, 5, false, false);
		p3.x = portX;		
		addOutput(createOutput<PJ301MPort>(p3, module, SLN::NOISE_OUTPUT));

	}

};

Model *modelSLN = createModel<SLN, SLNWidget>("SLN");

