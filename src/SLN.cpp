#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "dsp/noise.hpp"

#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"

struct SLN : AHModule {

	enum ParamIds {
		SPEED_PARAM,
		SLOPE_PARAM,
		NOISE_PARAM,
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

	SLN() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

	}
	
	void step() override;

	Core core;

	SchmittTrigger inTrigger;
	bogaudio::dsp::WhiteNoiseGenerator white;
	bogaudio::dsp::PinkNoiseGenerator pink;
	bogaudio::dsp::RedNoiseGenerator brown;

	float target = 0.0f;
	float current = 0.0f;
	
};

void SLN::step() {
	
	AHModule::step();
	
	float noise;
	int noiseType = params[NOISE_PARAM].value;
	
	switch(noiseType) {
		case 0:
			noise = white.next() * 10.0;;
			break;
		case 1:
			noise = pink.next() * 10.8; // scale to -10 to 10;
			break;
		case 2:
			noise = brown.next() * 23.4; // scale to -10 to 10;
			break;
		default:
			noise = white.next() * 10.0;
	}
	
	// Capture noise
	if (inTrigger.process(inputs[TRIG_INPUT].value / 0.7)) {
		target = noise;
	} 
				
	float shape = params[SLOPE_PARAM].value;

	// minimum and maximum slopes in volts per second
	const float slewMin = 0.1;
	const float slewMax = 10000.0;
	
	// Amount of extra slew per voltage difference
	const float shapeScale = 1.0/10.0;

	float speed = params[SPEED_PARAM].value;
	float slew = slewMax * powf(slewMin / slewMax, speed);

	// Rise
	if (target > current) {
		current += slew * crossfade(1.0f, shapeScale * (target - current), shape) * engineGetSampleTime();
		if (current > target) // Trap overshoot
			current = target;
	}
	// Fall
	else if (target < current) {
		current -= slew * crossfade(1.0f, shapeScale * (current - target), shape) * engineGetSampleTime();
		if (current < target) // Trap overshoot
			current = target;
	}

	outputs[OUT_OUTPUT].value = current;
	outputs[NOISE_OUTPUT].value = noise;	
	
}

struct SLNWidget : ModuleWidget {
	SLNWidget(SLN *module);
};

SLNWidget::SLNWidget(SLN *module) : ModuleWidget(module) {
	
	UI ui;
	
	box.size = Vec(45, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/SLN.svg")));
		addChild(panel);
	}

	float panelwidth = 45.0;
	float portwidth = 25.0;
	float knobwidth = 23.0;
	
	float portX = (panelwidth - portwidth) / 2.0;
	float knobX = (panelwidth - knobwidth) / 2.0;

	Vec p1 = ui.getPosition(UI::PORT, 0, 0, false, false);
	p1.x = portX;
	addInput(Port::create<PJ301MPort>(p1, Port::INPUT, module, SLN::TRIG_INPUT));
	
	Vec p2 = ui.getPosition(UI::PORT, 0, 4, false, false);
	p2.x = portX;	
	addOutput(Port::create<PJ301MPort>(p2, Port::OUTPUT, module, SLN::OUT_OUTPUT));
	
	Vec p3 = ui.getPosition(UI::PORT, 0, 5, false, false);
	p3.x = portX;		
	addOutput(Port::create<PJ301MPort>(p3, Port::OUTPUT, module, SLN::NOISE_OUTPUT));
	
	
	Vec k1 = ui.getPosition(UI::PORT, 0, 1, false, false);
	k1.x = knobX;
	AHKnobNoSnap *speedW = ParamWidget::create<AHKnobNoSnap>(k1, module, SLN::SPEED_PARAM, 0.0, 1.0, 0.0);
	addParam(speedW);

	Vec k2 = ui.getPosition(UI::PORT, 0, 2, false, false);
	k2.x = knobX;
	AHKnobNoSnap *slopeW = ParamWidget::create<AHKnobNoSnap>(k2, module, SLN::SLOPE_PARAM, 0.0, 1.0, 0.0);
	addParam(slopeW);

	Vec k3 = ui.getPosition(UI::PORT, 0, 3, false, false);
	k3.x = knobX;
	AHKnobSnap *noiseW = ParamWidget::create<AHKnobSnap>(k3, module, SLN::NOISE_PARAM, 0.0, 2.0, 0.0);
	addParam(noiseW);
		

}

Model *modelSLN = Model::create<SLN, SLNWidget>( "Amalgamated Harmonics", "SLN", "SLN", NOISE_TAG);

