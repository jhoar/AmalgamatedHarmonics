#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"
#include "VCO.hpp"
#include "dsp/digital.hpp"
#include "dsp/resampler.hpp"
#include "dsp/filter.hpp"

#include <iostream>

struct Chord : AHModule {

	const static int NUM_PITCHES = 6;

	enum ParamIds {
		ENUMS(WAVE_PARAM,6),
		ENUMS(DETUNE_PARAM,6),
		ENUMS(PW_PARAM,6),
		ENUMS(PWM_PARAM,6),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(PITCH_INPUT,6),
		ENUMS(PW_INPUT,6),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(OUT_OUTPUT,2),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
	
	Chord() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

	void step() override;
		
	Core core;

	int poll = 50000;

	SchmittTrigger moveTrigger;
	PulseGenerator triggerPulse;

	EvenVCO oscillator[6];

};

void Chord::step() {
	
	AHModule::step();

	float out[2];
	int nPitches[2];

	out[0] = 0.0f;
	out[1] = 0.0f;
	nPitches[0] = 0;
	nPitches[1] = 0;

	for (int i = 0; i < NUM_PITCHES; i++) {

		int index = PITCH_INPUT + i;
		int side = i % 2;

		if (inputs[index].active) {

			nPitches[side]++;

			float pitchCv = inputs[index].value;
			float pitchFine = params[DETUNE_PARAM + i].value / 12.0; // +- 1V

			oscillator[i].pw = params[PW_PARAM + i].value + params[PWM_PARAM + i].value * inputs[PW_INPUT + i].value / 10.0f;
			oscillator[i].step(delta, pitchFine + pitchCv); // 1V/OCT

			int wave = params[WAVE_PARAM + i].value;
			switch(wave) {
				case 0:		out[side] += oscillator[i].sine; 		break;
				case 1:		out[side] += oscillator[i].saw;			break;
				case 2:		out[side] += oscillator[i].doubleSaw;	break;
				case 3:		out[side] += oscillator[i].square;		break;
				case 4:		out[side] += oscillator[i].even;		break;
				default:	out[side] += oscillator[i].sine;		break;
			};
		}
	}

	for (int i = 0; i < 2; i++) {
		if (nPitches[i] > 0) {
			out[i] = (out[i] / nPitches[i]) * 5.0;
		}
	}

	if (outputs[OUT_OUTPUT].active && outputs[OUT_OUTPUT + 1].active) {
		outputs[OUT_OUTPUT].value 		= out[0];
		outputs[OUT_OUTPUT + 1].value 	= out[1];
	} else if (!outputs[OUT_OUTPUT].active && outputs[OUT_OUTPUT + 1].active) {
		outputs[OUT_OUTPUT].value 		= 0.0f;
		outputs[OUT_OUTPUT + 1].value 	= (out[0] + out[1]) / 2.0;
	} else if (outputs[OUT_OUTPUT].active && !outputs[OUT_OUTPUT + 1].active) {
		outputs[OUT_OUTPUT].value 		= (out[0] + out[1]) / 2.0;
		outputs[OUT_OUTPUT + 1].value 	= 0.0f;
	}

}

struct ChordWidget : ModuleWidget {

	ChordWidget(Chord *module) : ModuleWidget(module) {
		
		UI ui;
		
		box.size = Vec(270, 380);

		{
			SVGPanel *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/Chord.svg")));
			addChild(panel);
		}

		addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 30, 365)));

		for (int n = 0; n < 6; n++) {
			addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, n, 0, true, false), Port::INPUT, module, Chord::PITCH_INPUT + n));
			addParam(ParamWidget::create<AHKnobSnap>(ui.getPosition(UI::KNOB, n, 2, true, true), module, Chord::WAVE_PARAM + n, 0.0f, 4.0f, 0.0f));
			addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, n, 3, true, true), module, Chord::DETUNE_PARAM + n, -1.0f, 1.0f, 0.0f));
			addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, n, 4, true, true), module, Chord::PW_PARAM + n, -1.0f, 1.0f, 0.0f));
			addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, n, 5, true, true), Port::INPUT, module, Chord::PW_INPUT + n));
			addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, n, 6, true, true), module, Chord::PWM_PARAM + n, 0.0f, 1.0f, 0.0f));
		}


		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 4, 5, true, false), Port::OUTPUT, module, Chord::OUT_OUTPUT));
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 5, 5, true, false), Port::OUTPUT, module, Chord::OUT_OUTPUT + 1));

	}
};

Model *modelChord = Model::create<Chord, ChordWidget>( "Amalgamated Harmonics", "Chord", "D'acchord", OSCILLATOR_TAG);

// ♯♭
