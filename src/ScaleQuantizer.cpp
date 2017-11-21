#include "AH.hpp"
#include "Core.hpp"

#include <iostream>

struct ScaleQuantizer : Module {

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
		GATE_OUTPUT,
		NUM_OUTPUTS = GATE_OUTPUT + 12
	};
	enum LightIds {
		NOTE_LIGHT,
		KEY_LIGHT = NOTE_LIGHT + 12,
		SCALE_LIGHT = KEY_LIGHT + 12,
		DEGREE_LIGHT = SCALE_LIGHT + 12,
		NUM_LIGHTS = DEGREE_LIGHT + 12
	};

	ScaleQuantizer() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	void setQuantizer(Quantizer &quant);
	
	Quantizer q;
	bool firstStep = true;
	int lastScale = 0;
	int lastRoot = 0;
	
	int currScale = 0;
	int currRoot = 0;
	int currNote = 0;
	int currDegree = 0;

};

void ScaleQuantizer::setQuantizer(Quantizer &quantizer) {
	q = quantizer;
}

void ScaleQuantizer::step() {
	
	lastScale = currScale;
	lastRoot = currRoot;

	// Get the input pitch
	float volts = inputs[IN_INPUT].value;
	float root =  inputs[KEY_INPUT].value;
	float scale = inputs[SCALE_INPUT].value;

	// Calculate output pitch from raw voltage
	float pitch = q.getPitchFromVolts(volts, root, scale, &currRoot, &currScale, &currNote, &currDegree);

	// Set the value
	outputs[OUT_OUTPUT].value = pitch;

	// update tone lights
	for (int i = 0; i < Quantizer::NUM_NOTES; i++) {
		lights[NOTE_LIGHT + i].value = 0.0;
	}
	lights[NOTE_LIGHT + currNote].value = 1.0;

	// update degree lights
	for (int i = 0; i < Quantizer::NUM_NOTES; i++) {
		lights[DEGREE_LIGHT + i].value = 0.0;
		outputs[GATE_OUTPUT + i].value = 0.0;
	}
	lights[DEGREE_LIGHT + currDegree].value = 1.0;
	outputs[GATE_OUTPUT + currDegree].value = 10.0;

	if (lastScale != currScale || firstStep) {
		for (int i = 0; i < Quantizer::NUM_NOTES; i++) {
			lights[SCALE_LIGHT + i].value = 0.0;
		}
		lights[SCALE_LIGHT + currScale].value = 1.0;
	} 

	if (lastRoot != currRoot || firstStep) {
		for (int i = 0; i < Quantizer::NUM_NOTES; i++) {
			lights[KEY_LIGHT + i].value = 0.0;
		}
		lights[KEY_LIGHT + currRoot].value = 1.0;
	} 

	firstStep = false;

}

ScaleQuantizerWidget::ScaleQuantizerWidget() {
	ScaleQuantizer *module = new ScaleQuantizer();
	
	Quantizer quant = Quantizer();
	module->setQuantizer(quant);
	
	setModule(module);
	box.size = Vec(240, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/ScaleQuantizer.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

	addInput(createInput<PJ301MPort>(Vec(18, 329), module, ScaleQuantizer::IN_INPUT));
	addInput(createInput<PJ301MPort>(Vec(78, 329), module, ScaleQuantizer::KEY_INPUT));
	addInput(createInput<PJ301MPort>(Vec(138, 329), module, ScaleQuantizer::SCALE_INPUT));

	addOutput(createOutput<PJ301MPort>(Vec(198, 329), module, ScaleQuantizer::OUT_OUTPUT));

	float xOffset = 18.0;
	float xSpace = 21.0;
	float xPos = 0.0;
	float yPos = 0.0;
	int scale = 0;

	for (int i = 0; i < 12; i++) {
		addChild(createLight<SmallLight<GreenLight>>(Vec(xOffset + i * 18.0, 280.0), module, ScaleQuantizer::SCALE_LIGHT + i));
	}
 
	for (int i = 0; i < 12; i++) {
		quant.calculateKey(i, xSpace, xOffset, 230.0, &xPos, &yPos, &scale);
		addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, ScaleQuantizer::KEY_LIGHT + scale));

		quant.calculateKey(i, xSpace, xOffset + 72.0, 165.0, &xPos, &yPos, &scale);
		addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, ScaleQuantizer::NOTE_LIGHT + scale));

		quant.calculateKey(i, 30.0, xOffset + 9.5, 110.0, &xPos, &yPos, &scale);
		addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, ScaleQuantizer::DEGREE_LIGHT + scale));

		quant.calculateKey(i, 30.0, xOffset, 85.0, &xPos, &yPos, &scale);
		addOutput(createOutput<PJ301MPort>(Vec(xPos, yPos), module, ScaleQuantizer::GATE_OUTPUT + scale));
	}

}
