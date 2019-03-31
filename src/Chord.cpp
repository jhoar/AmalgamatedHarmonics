#include "AH.hpp"

#include "string.hpp"

#include "AHCommon.hpp"
#include "VCO.hpp"

#include <iostream>

using namespace ah;

struct Chord : core::AHModule {

	const static int NUM_PITCHES = 6;

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

		float PI_180 = core::PI / 180.0;
		float posMax = 90.0 * 0.5 * PI_180;
		float voicePosDeg[6] = {-90.0f, 90.0f, -54.0f, 54.0f, -18.0f, 18.0f};
		float voicePosRad[6];	
		for(int i = 0; i < 6; i++) {
			voicePosRad[i] = voicePosDeg[i] * 0.5 * PI_180;
		}

		struct WaveParamQuantity : engine::ParamQuantity {
			std::string getDisplayValueString() override {
				int v = (int)getValue();
				switch (v)
				{
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

			float e = 1.0f / (90.0f * 0.5 * (core::PI / 180.0));

			std::string getDisplayValueString() override {
				float v = getSmoothValue() * e;

				std::string s = "Unknown";
				if (v == 0.f) {
					s = "0";
				}
				if (v < 0.f) {
					s = string::f("%.*g", 3, math::normalizeZero(fabs(v) * 100.0f)) + "% L";
				}
				if (v > 0.f) {
					s = string::f("%.*g", 3, math::normalizeZero(v * 100.0f)) + "% R";
				}
				return s;
			}
		};

		for (int n = 0; n < 6; n++) {
			configParam(WAVE_PARAM + n, 0.0f, 4.0f, 0.0f, "Waveform");
			configParam(OCTAVE_PARAM + n, -3.0f, 3.0f, 0.0f, "Octave");
			configParam(DETUNE_PARAM + n, -1.0f, 1.0f, 0.0f, "Fine tune", "V");
			configParam(PW_PARAM + n, -1.0f, 1.0f, 0.0f, "Pulse width");
			configParam(PWM_PARAM + n, 0.0f, 1.0f, 0.0f, "Pulse width modulation CV");
			configParam(ATTN_PARAM + n, 0.0f, 1.0f, 1.0f, "Level", "%", 0.0f, 100.0f);
			configParam(PAN_PARAM + n, -posMax, posMax, voicePosRad[n], "Stereo pan (L-R)", "", 0.0f, -1.0f / voicePosRad[0]);
		}

		configParam(SPREAD_PARAM, 0.0f, 1.0f, 1.0f, "Spread");
		paramQuantities[SPREAD_PARAM]->description = "Spread of voices across stereo field";

	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// polymode
		json_t *polymodeJ = json_boolean(polymode);
		json_object_set_new(rootJ, "polymode", polymodeJ);

		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override {

		// polymode
		json_t *polymodeJ = json_object_get(rootJ, "polymode");
		if (polymodeJ)
			polymode = json_boolean_value(polymodeJ);

	}


	void process(const ProcessArgs &args) override;
		
	int poll = 50000;

	rack::dsp::SchmittTrigger moveTrigger;
	rack::dsp::PulseGenerator triggerPulse;

	EvenVCO oscillator[6];

	bool polymode = false;
	bool switchMode = false;

};

void Chord::process(const ProcessArgs &args) {
	
	AHModule::step();

	float out[2] = {0.0f, 0.0f};
	int nP[2] = {0, 0};

	float spread = params[SPREAD_PARAM].getValue();
	float SQRT2_2 = sqrt(2.0) / 2.0;

	if(switchMode) {
		for (int i = 0; i < NUM_PITCHES; i++) {
			oscillator[i].reset();
		}
		switchMode = false;
	}

	bool haveGateIn = inputs[PITCH_INPUT + 1].isConnected();

	for (int i = 0; i < NUM_PITCHES; i++) {

		float inputPitchCV = 0.0f;
		bool haveInput = false;
		
		if (polymode) {
			if (haveGateIn) {
				if (inputs[PITCH_INPUT + 1].getVoltage(i) > 0.0f) {
					haveInput = true;
					inputPitchCV = inputs[PITCH_INPUT].getVoltage(i);
				} else {
					inputPitchCV = 0.0;
				}
			} else {
				haveInput = true;
				inputPitchCV = inputs[PITCH_INPUT].getVoltage(i);
			}
		} else {
			if (inputs[PITCH_INPUT + i].isConnected()) {
				haveInput = true;
				inputPitchCV = inputs[PITCH_INPUT + i].getVoltage();
			} else {
				inputPitchCV = 0.0;
			}
		}

		int side = i % 2;

		float pitchCv = inputPitchCV + params[OCTAVE_PARAM + i].getValue();
		float pitchFine = params[DETUNE_PARAM + i].getValue() / 12.0; // +- 1V
		float attn = params[ATTN_PARAM + i].getValue();
		oscillator[i].pw = params[PW_PARAM + i].getValue() + params[PWM_PARAM + i].getValue() * inputs[PW_INPUT + i].getVoltage() / 10.0f;
		oscillator[i].step(args.sampleTime, pitchFine + pitchCv); // 1V/OCT

		if (haveInput) {

			float amp = 0.0;
			nP[side]++;

			int wave = params[WAVE_PARAM + i].getValue();
			switch(wave) {
				case 0:		amp = oscillator[i].sine * attn;		break;
				case 1:		amp = oscillator[i].saw * attn;			break;
				case 2:		amp = oscillator[i].doubleSaw * attn;	break;
				case 3:		amp = oscillator[i].square * attn;		break;
				case 4:		amp = oscillator[i].even * attn;		break;
				default:	amp = oscillator[i].sine * attn;		break;
			};

			float angle = spread * params[PAN_PARAM + i].getValue();
			float left = SQRT2_2 * (cos(angle) - sin(angle));
    		float right = SQRT2_2 * (cos(angle) + sin(angle));

			out[0] += left * amp;
			out[1] += right * amp;

		}
	}

	if (nP[0] > 0) {
		out[0] = (out[0] * 5.0f) / (float)nP[0];
	} 

	if (nP[1] > 0) {
		out[1] = (out[1] * 5.0f) / (float)nP[1];
	} 

	if (debugEnabled(5000)) {
		std::cout << nP[0] << " " << nP[1] << " " << out[0] << " " << out[1] << std::endl;
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

		for (int n = 0; n < 6; n++) {
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, n, 0, true, true), module, Chord::PITCH_INPUT + n));
			addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, n, 1, true, true), module, Chord::WAVE_PARAM + n));
			addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, n, 2, true, true), module, Chord::OCTAVE_PARAM + n));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, n, 3, true, true), module, Chord::DETUNE_PARAM + n));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, n, 4, true, true), module, Chord::PW_PARAM + n));
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, n, 5, true, true), module, Chord::PW_INPUT + n));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, n, 6, true, true), module, Chord::PWM_PARAM + n));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, n, 7, true, true), module, Chord::ATTN_PARAM + n));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, n, 8, true, true), module, Chord::PAN_PARAM + n));
		}

		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 0, 9, true, true), module, Chord::SPREAD_PARAM));

		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 4, 9, true, true), module, Chord::OUT_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 9, true, true), module, Chord::OUT_OUTPUT + 1));

	}

	void appendContextMenu(Menu *menu) override {

		Chord *chord = dynamic_cast<Chord*>(module);
		assert(chord);

		struct PolyModeItem : MenuItem {
			Chord *module;
			bool polymode;
			void onAction(const ActionEvent &e) override {
				module->polymode = polymode;
				module->switchMode = true;
			}
		};

		struct PolyModeMenu : MenuItem {
			Chord *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<bool> modes = {true, false};
				std::vector<std::string> names = {"Poly", "Mono"};
				for (size_t i = 0; i < modes.size(); i++) {
					PolyModeItem *item = createMenuItem<PolyModeItem>(names[i], CHECKMARK(module->polymode == modes[i]));
					item->module = module;
					item->polymode = modes[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());
		PolyModeMenu *polymodeItem = createMenuItem<PolyModeMenu>("Input cable mode");
		polymodeItem->module = chord;
		menu->addChild(polymodeItem);

	}

};

Model *modelChord = createModel<Chord, ChordWidget>("Chord");

// ♯♭
