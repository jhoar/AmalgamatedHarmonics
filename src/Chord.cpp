#include "AH.hpp"

#include "string.hpp"

#include "AHCommon.hpp"
#include "VCO.hpp"

#include <iostream>

using namespace ah;

const float PI_180 = core::PI / 180.0f;
const float POSMAX = 90.0f * 0.5f * PI_180;
const float ONE_POSMAX = 1.0f / POSMAX;
const float SQRT2_2 = sqrt(2.0) / 2.0;
const int 	NUM_PITCHES = 6;

struct Chord : core::AHModule {

	enum ParamIds {
		ENUMS(WAVE_PARAM,6),
		ENUMS(OCTAVE_PARAM,6),
		ENUMS(DETUNE_PARAM,6),
		ENUMS(PW_PARAM,6),
		ENUMS(PWM_PARAM,6),
		ENUMS(ATTN_PARAM,6),
		ENUMS(PAN_PARAM,6),
		SPREAD_PARAM,
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

	Chord() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

		struct WaveParamQuantity : engine::ParamQuantity {
			std::string getDisplayValueString() override {
				switch (static_cast<int>(getValue())) {
					case 0:
						return "Sine " + ParamQuantity::getDisplayValueString();
						break;
					case 1:
						return "Saw " + ParamQuantity::getDisplayValueString();
						break;
					case 2:
						return "Triangle " + ParamQuantity::getDisplayValueString();
						break;
					case 3:
						return "Square " + ParamQuantity::getDisplayValueString();
						break;
					case 4:
						return "Even " + ParamQuantity::getDisplayValueString();
						break;
					default:
						return ParamQuantity::getDisplayValueString();
						break;
				}
			}
		};

		struct PanParamQuantity : engine::ParamQuantity {

			std::string getDisplayValueString() override {
				float v = getSmoothValue() * ONE_POSMAX;

				std::string s = "Unknown";
				if (v == 0.0f) {
					s = "0";
				}
				if (v < 0.0f) {
					s = string::f("%.*g", 3, math::normalizeZero(fabs(v) * 100.0f)) + "% L";
				}
				if (v > 0.0f) {
					s = string::f("%.*g", 3, math::normalizeZero(v * 100.0f)) + "% R";
				}
				return s;
			}
		};

		float voicePosDeg[6] = {-90.0f, 90.0f, -54.0f, 54.0f, -18.0f, 18.0f};

		for (int n = 0; n < NUM_PITCHES; n++) {
			configParam(WAVE_PARAM + n, 0.0f, 4.0f, 0.0f, "Waveform");
			configParam(OCTAVE_PARAM + n, -3.0f, 3.0f, 0.0f, "Octave");
			configParam(DETUNE_PARAM + n, -1.0f, 1.0f, 0.0f, "Fine tune", "V");
			configParam(PW_PARAM + n, -1.0f, 1.0f, 0.0f, "Pulse width");
			configParam(PWM_PARAM + n, 0.0f, 1.0f, 0.0f, "Pulse width modulation CV");
			configParam(ATTN_PARAM + n, 0.0f, 1.0f, 1.0f, "Level", "%", 0.0f, 100.0f);
			configParam(PAN_PARAM + n, -POSMAX, POSMAX, voicePosDeg[n] * 0.5f * PI_180, "Stereo pan (L-R)", "", 0.0f, -1.0f / voicePosDeg[0] * 0.5f * PI_180);
		}

		configParam(SPREAD_PARAM, 0.0f, 1.0f, 1.0f, "Spread");
		paramQuantities[SPREAD_PARAM]->description = "Spread of voices across stereo field";

	}

	void process(const ProcessArgs &args) override;

	rack::dsp::SchmittTrigger moveTrigger;
	rack::dsp::PulseGenerator triggerPulse;

	float angle = 0.0;
	float left  = SQRT2_2 * (cos(0.0) - sin(0.0));
	float right = SQRT2_2 * (cos(0.0) + sin(0.0));

	EvenVCO oscillator[NUM_PITCHES];

};

void Chord::process(const ProcessArgs &args) {

	AHModule::step();

	float out[2] = {0.0f, 0.0f};
	float nP[2]  = {0.0f, 0.0f};

	float spread = params[SPREAD_PARAM].getValue();

	for (int i = 0; i < NUM_PITCHES; i++) {

		float inputPitchCV = 0.0f;

		if (inputs[PITCH_INPUT + i].isConnected()) {
			inputPitchCV = inputs[PITCH_INPUT + i].getVoltage();
		} else {
			if (inputs[PITCH_INPUT].getChannels() > i) {
				inputPitchCV = inputs[PITCH_INPUT].getVoltage(i);
			} else {
				inputPitchCV = inputs[PITCH_INPUT].getVoltage(0);
			}
		}

		int side = i % 2;

		float pitchCv = inputPitchCV + params[OCTAVE_PARAM + i].getValue();
		float pitchFine = params[DETUNE_PARAM + i].getValue() / 12.0; // +- 1V
		float attn = params[ATTN_PARAM + i].getValue();
		oscillator[i].pw = params[PW_PARAM + i].getValue() + params[PWM_PARAM + i].getValue() * inputs[PW_INPUT + i].getVoltage() / 10.0f;
		oscillator[i].step(args.sampleTime, pitchFine + pitchCv); // 1V/OCT

		float amp = 0.0f;
		nP[side] += 1.0f;

		int wave = params[WAVE_PARAM + i].getValue();
		switch(wave) {
			case 0:		amp = oscillator[i].sine * attn;		break;
			case 1:		amp = oscillator[i].saw * attn;			break;
			case 2:		amp = oscillator[i].doubleSaw * attn;	break;
			case 3:		amp = oscillator[i].square * attn;		break;
			case 4:		amp = oscillator[i].even * attn;		break;
			default:	amp = oscillator[i].sine * attn;		break;
		};

		float newAngle = spread * params[PAN_PARAM + i].getValue();
		if (newAngle != angle) {
			angle = newAngle;
			left  = SQRT2_2 * (cos(angle) - sin(angle));
			right = SQRT2_2 * (cos(angle) + sin(angle));
		}

		out[0] += left * amp;
		out[1] += right * amp;

	}

	if (nP[0] > 0.0f) {
		out[0] = (out[0] * 5.0f) / nP[0];
	} 

	if (nP[1] > 0.0f) {
		out[1] = (out[1] * 5.0f) / nP[1];
	} 

	if (outputs[OUT_OUTPUT].isConnected() && outputs[OUT_OUTPUT + 1].isConnected()) {
		outputs[OUT_OUTPUT].setVoltage(out[0]);
		outputs[OUT_OUTPUT + 1].setVoltage(out[1]);
	} else if (!outputs[OUT_OUTPUT].isConnected() && outputs[OUT_OUTPUT + 1].isConnected()) {
		outputs[OUT_OUTPUT].setVoltage(0.0f);
		outputs[OUT_OUTPUT + 1].setVoltage((out[0] + out[1]) / 2.0f);
	} else if (outputs[OUT_OUTPUT].isConnected() && !outputs[OUT_OUTPUT + 1].isConnected()) {
		outputs[OUT_OUTPUT].setVoltage((out[0] + out[1]) / 2.0f);
		outputs[OUT_OUTPUT + 1].setVoltage(0.0f);
	}

}

struct ChordWidget : ModuleWidget {

	ChordWidget(Chord *module) {
		
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Chord.svg")));

		addParam(createParamCentered<gui::AHKnobSnap>(Vec(30.044, 88.542), module, Chord::WAVE_PARAM + 0));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(68.395, 88.542), module, Chord::WAVE_PARAM + 1));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(106.746, 88.542), module, Chord::WAVE_PARAM + 2));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(145.097, 88.542), module, Chord::WAVE_PARAM + 3));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(183.447, 88.542), module, Chord::WAVE_PARAM + 4));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(221.798, 88.542), module, Chord::WAVE_PARAM + 5));

		addParam(createParamCentered<gui::AHKnobSnap>(Vec(30.044, 120.883), module, Chord::OCTAVE_PARAM + 0));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(68.395, 120.883), module, Chord::OCTAVE_PARAM + 1));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(106.746, 120.883), module, Chord::OCTAVE_PARAM + 2));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(145.097, 120.883), module, Chord::OCTAVE_PARAM + 3));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(183.447, 120.883), module, Chord::OCTAVE_PARAM + 4));
		addParam(createParamCentered<gui::AHKnobSnap>(Vec(221.798, 120.883), module, Chord::OCTAVE_PARAM + 5));

		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(30.044, 153.223), module, Chord::DETUNE_PARAM + 0));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(68.395, 153.223), module, Chord::DETUNE_PARAM + 1));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(106.746, 153.223), module, Chord::DETUNE_PARAM + 2));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(145.097, 153.223), module, Chord::DETUNE_PARAM + 3));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(183.447, 153.223), module, Chord::DETUNE_PARAM + 4));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(221.798, 153.223), module, Chord::DETUNE_PARAM + 5));

		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(30.044, 224.163), module, Chord::PW_PARAM + 0));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(68.395, 224.163), module, Chord::PW_PARAM + 1));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(106.746, 224.163), module, Chord::PW_PARAM + 2));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(145.097, 224.163), module, Chord::PW_PARAM + 3));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(183.447, 224.163), module, Chord::PW_PARAM + 4));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(221.798, 224.163), module, Chord::PW_PARAM + 5));

		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(30.044, 253.852), module, Chord::PWM_PARAM + 0));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(68.395, 253.852), module, Chord::PWM_PARAM + 1));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(106.746, 253.852), module, Chord::PWM_PARAM + 2));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(145.097, 253.852), module, Chord::PWM_PARAM + 3));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(183.447, 253.852), module, Chord::PWM_PARAM + 4));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(221.798, 253.852), module, Chord::PWM_PARAM + 5));

		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(30.044, 291.541), module, Chord::ATTN_PARAM + 0));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(68.395, 291.541), module, Chord::ATTN_PARAM + 1));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(106.746, 291.541), module, Chord::ATTN_PARAM + 2));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(145.097, 291.541), module, Chord::ATTN_PARAM + 3));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(183.447, 291.541), module, Chord::ATTN_PARAM + 4));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(221.798, 291.541), module, Chord::ATTN_PARAM + 5));

		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(30.044, 321.23), module, Chord::PAN_PARAM + 0));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(68.395, 321.23), module, Chord::PAN_PARAM + 1));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(106.746, 321.23), module, Chord::PAN_PARAM + 2));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(145.097, 321.23), module, Chord::PAN_PARAM + 3));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(183.447, 321.23), module, Chord::PAN_PARAM + 4));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(221.798, 321.23), module, Chord::PAN_PARAM + 5));

		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(29.746, 356.339), module, Chord::SPREAD_PARAM));

		addInput(createInputCentered<gui::AHPort>(Vec(29.746, 46.859), module, Chord::PITCH_INPUT + 0));
		addInput(createInputCentered<gui::AHPort>(Vec(68.097, 46.859), module, Chord::PITCH_INPUT + 1));
		addInput(createInputCentered<gui::AHPort>(Vec(106.447, 46.859), module, Chord::PITCH_INPUT + 2));
		addInput(createInputCentered<gui::AHPort>(Vec(144.798, 46.859), module, Chord::PITCH_INPUT + 3));
		addInput(createInputCentered<gui::AHPort>(Vec(183.149, 46.859), module, Chord::PITCH_INPUT + 4));
		addInput(createInputCentered<gui::AHPort>(Vec(221.5, 46.859), module, Chord::PITCH_INPUT + 5));

		addInput(createInputCentered<gui::AHPort>(Vec(29.746, 191.916), module, Chord::PW_INPUT + 0));
		addInput(createInputCentered<gui::AHPort>(Vec(68.097, 191.916), module, Chord::PW_INPUT + 1));
		addInput(createInputCentered<gui::AHPort>(Vec(106.447, 191.916), module, Chord::PW_INPUT + 2));
		addInput(createInputCentered<gui::AHPort>(Vec(144.798, 191.916), module, Chord::PW_INPUT + 3));
		addInput(createInputCentered<gui::AHPort>(Vec(183.149, 191.916), module, Chord::PW_INPUT + 4));
		addInput(createInputCentered<gui::AHPort>(Vec(221.5, 191.916), module, Chord::PW_INPUT + 5));

		addOutput(createOutputCentered<gui::AHPort>(Vec(183.149, 363.566), module, Chord::OUT_OUTPUT + 0));
		addOutput(createOutputCentered<gui::AHPort>(Vec(221.5, 363.566), module, Chord::OUT_OUTPUT + 1));

	}

};

Model *modelChord = createModel<Chord, ChordWidget>("Chord");

// ♯♭
