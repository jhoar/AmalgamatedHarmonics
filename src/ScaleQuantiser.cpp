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
	int currDegree = 0;
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
	currPitch =  music::getPitchFromVolts(volts, root, scale, &currRoot, &currScale, &currNote, &currDegree);

	// Set the value
	outputs[OUT_OUTPUT].value = currPitch;

	// update tone lights
	for (int i = 0; i < music::NUM_NOTES; i++) {
		lights[NOTE_LIGHT + i].value = 0.0;
	}
	lights[NOTE_LIGHT + currNote].value = 1.0;

	// update degree lights
	for (int i = 0; i < music::NUM_NOTES; i++) {
		lights[DEGREE_LIGHT + i].value = 0.0;
		outputs[GATE_OUTPUT + i].value = 0.0;
	}
	lights[DEGREE_LIGHT + currDegree].value = 1.0;
	outputs[GATE_OUTPUT + currDegree].value = 10.0;

	if (lastScale != currScale || firstStep) {
		for (int i = 0; i <music::NUM_NOTES; i++) {
			lights[SCALE_LIGHT + i].value = 0.0;
		}
		lights[SCALE_LIGHT + currScale].value = 1.0;
	} 

	if (lastRoot != currRoot || firstStep) {
		for (int i = 0; i < music::NUM_NOTES; i++) {
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

        addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 5, false, false), module, ScaleQuantizer::IN_INPUT));
        addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 5, false, false), module, ScaleQuantizer::KEY_INPUT));
        addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 5, false, false), module, ScaleQuantizer::SCALE_INPUT));
        addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 3, 5, false, false), module, ScaleQuantizer::TRIG_OUTPUT));
        addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 4, 5, false, false), module, ScaleQuantizer::OUT_OUTPUT));

        float xOffset = 18.0;
        float xSpace = 21.0;
        float xPos = 0.0;
        float yPos = 0.0;
        int scale = 0;

        for (int i = 0; i < 12; i++) {
            addChild(createLight<SmallLight<GreenLight>>(Vec(xOffset + i * 18.0, 280.0), module, ScaleQuantizer::SCALE_LIGHT + i));

            gui::calculateKeyboard(i, xSpace, xOffset, 230.0, &xPos, &yPos, &scale);
            addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, ScaleQuantizer::KEY_LIGHT + scale));

            gui::calculateKeyboard(i, xSpace, xOffset + 72.0, 165.0, &xPos, &yPos, &scale);
            addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, ScaleQuantizer::NOTE_LIGHT + scale));

            gui::calculateKeyboard(i, 30.0, xOffset + 9.5, 110.0, &xPos, &yPos, &scale);
            addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, ScaleQuantizer::DEGREE_LIGHT + scale));

            gui::calculateKeyboard(i, 30.0, xOffset, 85.0, &xPos, &yPos, &scale);

            addOutput(createOutput<PJ301MPort>(Vec(xPos, yPos), module, ScaleQuantizer::GATE_OUTPUT + scale));
        }
    }
};

Model *modelScaleQuantizer = createModel<ScaleQuantizer, ScaleQuantizerWidget>("ScaleQuantizer");
