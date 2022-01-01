#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct ScaleQuantizer : core::AHModule {

	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		IN_INPUT,
		KEY_INPUT,
		SCALE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		TRIG_OUTPUT,
		ENUMS(GATE_OUTPUT,12),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(NOTE_LIGHT,12),
		ENUMS(KEY_LIGHT,12),
		ENUMS(SCALE_LIGHT,12),
		ENUMS(DEGREE_LIGHT,12),
		NUM_LIGHTS
	};

	ScaleQuantizer() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

	void process(const ProcessArgs &args) override;

	bool firstStep = true;
	int lastScale = 0;
	int lastRoot = 0;
	float lastPitch = 0.0;
	
	int currScale = 0;
	int currRoot = 0;
	int currNote = 0;
	int currInterval = 0;
	float currPitch = 0.0;

};

void ScaleQuantizer::process(const ProcessArgs &args) {
	
	lastScale = currScale;
	lastRoot = currRoot;
	lastPitch = currPitch;

	// Get the input pitch
	float volts = inputs[IN_INPUT].value;
	float root =  inputs[KEY_INPUT].value;
	float scale = inputs[SCALE_INPUT].value;

	// Calculate output pitch from raw voltage
	currPitch =  music::getPitchFromVolts(volts, root, scale, &currRoot, &currScale, &currNote, &currInterval);

	// Set the value
	outputs[OUT_OUTPUT].value = currPitch;

	// update tone lights
	for (int i = 0; i < music::Notes::NUM_NOTES; i++) {
		lights[NOTE_LIGHT + i].value = 0.0;
	}
	lights[NOTE_LIGHT + currNote].value = 1.0;

	// update degree lights
	for (int i = 0; i < music::Notes::NUM_NOTES; i++) {
		lights[DEGREE_LIGHT + i].value = 0.0;
		outputs[GATE_OUTPUT + i].value = 0.0;
	}
	lights[DEGREE_LIGHT + currInterval].value = 1.0;
	outputs[GATE_OUTPUT + currInterval].value = 10.0;

	if (lastScale != currScale || firstStep) {
		for (int i = 0; i <music::Notes::NUM_NOTES; i++) {
			lights[SCALE_LIGHT + i].value = 0.0;
		}
		lights[SCALE_LIGHT + currScale].value = 1.0;
	} 

	if (lastRoot != currRoot || firstStep) {
		for (int i = 0; i < music::Notes::NUM_NOTES; i++) {
			lights[KEY_LIGHT + i].value = 0.0;
		}
		lights[KEY_LIGHT + currRoot].value = 1.0;
	} 

	if (lastPitch != currPitch || firstStep) {
		outputs[TRIG_OUTPUT].value = 10.0;
	} else {
		outputs[TRIG_OUTPUT].value = 0.0;		
	}

	firstStep = false;

}

struct ScaleQuantizerWidget : ModuleWidget {

	ScaleQuantizerWidget(ScaleQuantizer *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ScaleQuantizer.svg")));

		addInput(createInputCentered<gui::AHPort>(Vec(31.293, 342.279), module, ScaleQuantizer::IN_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(76.127, 342.279), module, ScaleQuantizer::KEY_INPUT));
		addInput(createInputCentered<gui::AHPort>(Vec(122.31, 342.279), module, ScaleQuantizer::SCALE_INPUT));

		addOutput(createOutputCentered<gui::AHPort>(Vec(30.955, 101.557), module, ScaleQuantizer::GATE_OUTPUT + 0));
		addOutput(createOutputCentered<gui::AHPort>(Vec(46.295, 74.987), module, ScaleQuantizer::GATE_OUTPUT + 1));
		addOutput(createOutputCentered<gui::AHPort>(Vec(61.635, 101.557), module, ScaleQuantizer::GATE_OUTPUT + 2));
		addOutput(createOutputCentered<gui::AHPort>(Vec(76.975, 74.987), module, ScaleQuantizer::GATE_OUTPUT + 3));
		addOutput(createOutputCentered<gui::AHPort>(Vec(92.315, 101.557), module, ScaleQuantizer::GATE_OUTPUT + 4));
		addOutput(createOutputCentered<gui::AHPort>(Vec(122.994, 101.557), module, ScaleQuantizer::GATE_OUTPUT + 5));
		addOutput(createOutputCentered<gui::AHPort>(Vec(138.334, 74.987), module, ScaleQuantizer::GATE_OUTPUT + 6));
		addOutput(createOutputCentered<gui::AHPort>(Vec(153.674, 101.557), module, ScaleQuantizer::GATE_OUTPUT + 7));
		addOutput(createOutputCentered<gui::AHPort>(Vec(169.014, 74.987), module, ScaleQuantizer::GATE_OUTPUT + 8));
		addOutput(createOutputCentered<gui::AHPort>(Vec(184.354, 101.557), module, ScaleQuantizer::GATE_OUTPUT + 9));
		addOutput(createOutputCentered<gui::AHPort>(Vec(199.694, 74.987), module, ScaleQuantizer::GATE_OUTPUT + 10));
		addOutput(createOutputCentered<gui::AHPort>(Vec(215.034, 101.557), module, ScaleQuantizer::GATE_OUTPUT + 11));

		addOutput(createOutputCentered<gui::AHPort>(Vec(168.493, 342.279), module, ScaleQuantizer::TRIG_OUTPUT));
		addOutput(createOutputCentered<gui::AHPort>(Vec(214.675, 342.279), module, ScaleQuantizer::OUT_OUTPUT));

		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(30.955, 115.398), module, ScaleQuantizer::DEGREE_LIGHT + 0));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(46.295, 88.828), module, ScaleQuantizer::DEGREE_LIGHT + 1));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(61.635, 115.398), module, ScaleQuantizer::DEGREE_LIGHT + 2));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(76.975, 88.828), module, ScaleQuantizer::DEGREE_LIGHT + 3));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(92.315, 115.398), module, ScaleQuantizer::DEGREE_LIGHT + 4));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(122.994, 115.398), module, ScaleQuantizer::DEGREE_LIGHT + 5));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(138.334, 88.828), module, ScaleQuantizer::DEGREE_LIGHT + 6));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(153.674, 115.398), module, ScaleQuantizer::DEGREE_LIGHT + 7));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(169.014, 88.828), module, ScaleQuantizer::DEGREE_LIGHT + 8));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(184.354, 115.398), module, ScaleQuantizer::DEGREE_LIGHT + 9));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(199.694, 88.828), module, ScaleQuantizer::DEGREE_LIGHT + 10));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(215.034, 115.398), module, ScaleQuantizer::DEGREE_LIGHT + 11));

		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(103.072, 148.645), module, ScaleQuantizer::NOTE_LIGHT + 1));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(123.889, 148.645), module, ScaleQuantizer::NOTE_LIGHT + 3));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(165.523, 148.645), module, ScaleQuantizer::NOTE_LIGHT + 6));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(186.34, 148.645), module, ScaleQuantizer::NOTE_LIGHT + 8));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(207.156, 148.645), module, ScaleQuantizer::NOTE_LIGHT + 10));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(92.664, 166.673), module, ScaleQuantizer::NOTE_LIGHT + 0));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(113.481, 166.673), module, ScaleQuantizer::NOTE_LIGHT + 2));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(134.297, 166.673), module, ScaleQuantizer::NOTE_LIGHT + 4));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(155.114, 166.673), module, ScaleQuantizer::NOTE_LIGHT + 5));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(175.931, 166.673), module, ScaleQuantizer::NOTE_LIGHT + 7));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(196.748, 166.673), module, ScaleQuantizer::NOTE_LIGHT + 9));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(217.565, 166.673), module, ScaleQuantizer::NOTE_LIGHT + 11));

		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(21.616, 232.673), module, ScaleQuantizer::KEY_LIGHT + 0));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(32.024, 214.645), module, ScaleQuantizer::KEY_LIGHT + 1));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(42.432, 232.673), module, ScaleQuantizer::KEY_LIGHT + 2));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(52.841, 214.645), module, ScaleQuantizer::KEY_LIGHT + 3));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(63.249, 232.673), module, ScaleQuantizer::KEY_LIGHT + 4));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(84.066, 232.673), module, ScaleQuantizer::KEY_LIGHT + 5));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(94.475, 214.645), module, ScaleQuantizer::KEY_LIGHT + 6));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(104.883, 232.673), module, ScaleQuantizer::KEY_LIGHT + 7));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(115.291, 214.645), module, ScaleQuantizer::KEY_LIGHT + 8));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(125.7, 232.673), module, ScaleQuantizer::KEY_LIGHT + 9));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(136.108, 214.645), module, ScaleQuantizer::KEY_LIGHT + 10));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(146.517, 232.673), module, ScaleQuantizer::KEY_LIGHT + 11));

		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(29.355, 282.658), module, ScaleQuantizer::SCALE_LIGHT + 0));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(45.834, 282.658), module, ScaleQuantizer::SCALE_LIGHT + 1));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(62.313, 282.658), module, ScaleQuantizer::SCALE_LIGHT + 2));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(78.791, 282.658), module, ScaleQuantizer::SCALE_LIGHT + 3));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(95.27, 282.658), module, ScaleQuantizer::SCALE_LIGHT + 4));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(111.749, 282.658), module, ScaleQuantizer::SCALE_LIGHT + 5));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(128.228, 282.658), module, ScaleQuantizer::SCALE_LIGHT + 6));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(144.706, 282.658), module, ScaleQuantizer::SCALE_LIGHT + 7));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(161.185, 282.658), module, ScaleQuantizer::SCALE_LIGHT + 8));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(177.664, 282.658), module, ScaleQuantizer::SCALE_LIGHT + 9));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(194.143, 282.658), module, ScaleQuantizer::SCALE_LIGHT + 10));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(210.621, 282.658), module, ScaleQuantizer::SCALE_LIGHT + 11));

    }
};

Model *modelScaleQuantizer = createModel<ScaleQuantizer, ScaleQuantizerWidget>("ScaleQuantizer");
