#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"

struct SLN : AHModule {

	enum ParamIds {
		SPEED_PARAM,
		SLOPE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		TRIG_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		DIAG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	SLN() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

	}
	
	void step() override;

	json_t *toJson() override {
		json_t *rootJ = json_object();

		// // gates
		// json_t *xMutesJ = json_array();
		// json_t *yMutesJ = json_array();
		//
		// for (int i = 0; i < 4; i++) {
		// 	json_t *xMuteJ = json_integer((int) xMute[i]);
		// 	json_array_append_new(xMutesJ, xMuteJ);
		//
		// 	json_t *yMuteJ = json_integer((int) yMute[i]);
		// 	json_array_append_new(yMutesJ, yMuteJ);
		// }
		//
		// json_object_set_new(rootJ, "xMutes", xMutesJ);
		// json_object_set_new(rootJ, "yMutes", yMutesJ);

		return rootJ;
	}
	
	void fromJson(json_t *rootJ) override {
		// gates
		// json_t *xMutesJ = json_object_get(rootJ, "xMutes");
		// if (xMutesJ) {
		// 	for (int i = 0; i < 4; i++) {
		// 		json_t *xMuteJ = json_array_get(xMutesJ, i);
		// 		if (xMuteJ)
		// 			xMute[i] = !!json_integer_value(xMuteJ);
		// 	}
		// }
		//
		// json_t *yMutesJ = json_object_get(rootJ, "yMutes");
		// if (yMutesJ) {
		// 	for (int i = 0; i < 4; i++) {
		// 		json_t *yMuteJ = json_array_get(yMutesJ, i);
		// 		if (yMuteJ)
		// 			yMute[i] = !!json_integer_value(yMuteJ);
		// 	}
		// }
	}
	
	Core core;

	SchmittTrigger inTrigger;

	float in = 0.0f;
	float out = 0.0f;
	
};

void SLN::step() {
	
	AHModule::step();
	
	float noise = 2.0 * randomNormal();
				
	if (inTrigger.process(inputs[TRIG_INPUT].value / 0.7)) {
		in = noise;
	} 
				
	float shape = params[SLOPE_PARAM].value;

	// minimum and maximum slopes in volts per second
	const float slewMin = 0.1;
	const float slewMax = 10000.0;
	// Amount of extra slew per voltage difference
	const float shapeScale = 1.0/10.0;

	float speed = params[SPEED_PARAM].value;

	// Rise
	if (in > out) {
//		float rise = speed;
		float slew = slewMax * powf(slewMin / slewMax, speed);
		out += slew * crossfade(1.0f, shapeScale * (in - out), shape) * engineGetSampleTime();
		if (out > in)
			out = in;
	}
	// Fall
	else if (in < out) {
//		float fall = speed;
		float slew = slewMax * powf(slewMin / slewMax, speed);
		out -= slew * crossfade(1.0f, shapeScale * (out - in), shape) * engineGetSampleTime();
		if (out < in)
			out = in;
	}

	outputs[OUT_OUTPUT].value = out;
	outputs[DIAG_OUTPUT].value = noise;
	
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

	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 30, 365)));
			
	addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 0, 0, false, false), Port::INPUT, module, SLN::TRIG_INPUT));
	addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 0, 3, false, false), Port::OUTPUT, module, SLN::OUT_OUTPUT));
	addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 0, 4, false, false), Port::OUTPUT, module, SLN::DIAG_OUTPUT));
		
	AHKnobNoSnap *speedW = ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::PORT, 0, 1, false, false), module, SLN::SPEED_PARAM, 0.0, 1.0, 0.0);
	addParam(speedW);

	AHKnobNoSnap *slopeW = ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::PORT, 0, 2, false, false), module, SLN::SLOPE_PARAM, 0.0, 1.0, 0.0);
	addParam(slopeW);
		

}

Model *modelSLN = Model::create<SLN, SLNWidget>( "Amalgamated Harmonics", "SLN", "SLN", NOISE_TAG);

