#include "common.hpp"

#include "dsp/noise.hpp"

#include "AH.hpp"
#include "AHCommon.hpp"
#include "VCO.hpp"

using namespace ah;

struct Generative : core::AHModule {

	enum ParamIds {
		FREQ_PARAM,
		WAVE_PARAM,
		FM_PARAM,
		AM_PARAM,
		NOISE_PARAM,
		CLOCK_PARAM,
		PROB_PARAM,
		DELAYL_PARAM,
		DELAYS_PARAM,
		GATEL_PARAM,
		GATES_PARAM,
		SLOPE_PARAM,
		SPEED_PARAM,
		ATTN_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		WAVE_INPUT,
		FM_INPUT,
		AM_INPUT,
		NOISE_INPUT,
		SAMPLE_INPUT,
		CLOCK_INPUT,
		PROB_INPUT,
		HOLD_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		LFO_OUTPUT,
		MIXED_OUTPUT,
		NOISE_OUTPUT,
		OUT_OUTPUT,
		GATE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(GATE_LIGHT,2),
		NUM_LIGHTS
	};

	Generative() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) { 

		// LFO section
		params[FREQ_PARAM].config(-8.0f, 10.0f, 1.0f, "Frequency");
		params[FREQ_PARAM].description = "Core LFO frequency";


		struct WaveParamQuantity : app::ParamQuantity {
			std::string getDisplayValueString() override {
				float v = getValue();
				if (v == 0.f) {
					return "Sine " + ParamQuantity::getDisplayValueString();
				}
				if (v == 1.f) {
					return "Triangle " + ParamQuantity::getDisplayValueString();
				}
				if (v == 2.f) {
					return "Saw " + ParamQuantity::getDisplayValueString();
				}
				if (v == 3.f) {
					return "Square " + ParamQuantity::getDisplayValueString();
				}
				if (v == 4.f) {
					return "Sine " + ParamQuantity::getDisplayValueString();
				}
				if (v > 0.f && v < 1.f/3.f) {
					return "Sine|..Triangle " + ParamQuantity::getDisplayValueString();
				}
				if (v >= 1.f/3.f && v < 2.f/3.f) {
					return "Sine.|.Triangle " + ParamQuantity::getDisplayValueString();		
				}
				if (v >= 2.f/3.f && v < 1.f) {
					return "Sine..|Triangle " + ParamQuantity::getDisplayValueString();
				}
				if (v > 1.f && v < 4.f/3.f) {
					return "Triangle|..Saw " + ParamQuantity::getDisplayValueString();
				}
				if (v >= 4.f/3.f && v < 5.f/3.f) {
					return "Triangle.|.Saw " + ParamQuantity::getDisplayValueString();		
				}
				if (v >= 5.f/3.f && v < 2.f) {
					return "Triangle..|Saw " + ParamQuantity::getDisplayValueString();
				}
				if (v > 2.f && v < 7.f/3.f) {
					return "Saw|..Square " + ParamQuantity::getDisplayValueString();
				}
				if (v >= 7.f/3.f && v < 8.f/3.f) {
					return "Saw.|.Square " + ParamQuantity::getDisplayValueString();		
				}
				if (v >= 8.f/3.f && v < 3.f) {
					return "Saw..|Square " + ParamQuantity::getDisplayValueString();
				}
				if (v > 3.f && v < 10.f/3.f) {
					return "Square|..Sine " + ParamQuantity::getDisplayValueString();
				}
				if (v >= 10.f/3.f && v < 11.f/3.f) {
					return "Square.|.Sine " + ParamQuantity::getDisplayValueString();		
				}
				if (v >= 11.f/3.f && v < 4.f) {
					return "Square..|Sine " + ParamQuantity::getDisplayValueString();
				}

				return "Sine (probably) " + ParamQuantity::getDisplayValueString();

			}
		};

		params[WAVE_PARAM].config<WaveParamQuantity>(0.0f, 4.0f, 1.5f, "Waveform");
		params[WAVE_PARAM].description = "Continuous: Sine - Triangle - Saw - Square - Sine";

		params[FM_PARAM].config(0.0f, 1.0f, 0.5f, "Frequency Modulation CV");

		params[AM_PARAM].config(0.0f, 1.0f, 0.5f, "Amplitude Modulation Mix");
		params[AM_PARAM].description = "Mix between the FM modulated LFO and the voltage supplied in AM input";

		params[NOISE_PARAM].config(0.0f, 1.0f, 0.5f, "Noise Mix");
		params[NOISE_PARAM].description = "Mix between the FM-AM modulated LFO and the internal noise source";

		params[CLOCK_PARAM].config(-2.0, 6.0, 1.0, "Clock frequency");
		
		params[PROB_PARAM].config(0.0, 1.0, 1.0, "Clock-tick probability", "%", 0.0f, 100.0f);

		params[DELAYL_PARAM].config(1.0f, 2.0f, 1.0f, "Delay length", "ms", 2.0f, 500.0f, -1000.0f);

		params[GATEL_PARAM].config(1.0f, 2.0f, 1.0f, "Gate length", "ms", 2.0f, 500.0f, -1000.0f);

		params[DELAYS_PARAM].config(1.0f, 2.0f, 1.0f, "Delay length spread", "ms", 2.0f, 500.0f, -1000.0f);
		params[DELAYS_PARAM].description = "Magnitude of random time applied to delay length";

		params[GATES_PARAM].config(1.0f, 2.0f, 1.0f, "Gate length spread", "ms", 2.0f, 500.0f, -1000.0f);
		params[GATES_PARAM].description = "Magnitude of random time applied to gate length";

		params[SLOPE_PARAM].config(0.0, 1.0, 0.0, "Slope");
		params[SLOPE_PARAM].description = "Linear to exponential slope";

		params[SPEED_PARAM].config(0.0, 1.0, 0.0, "Inertia", "%", 0.0f, 100.0f);
		params[SPEED_PARAM].description = "Resistance of the signal to change";

		params[ATTN_PARAM].config(0.0, 1.0, 1.0, "Attenuation", "%", 0.0f, -100.0f, 100.0f);

	}
	
	void step() override;

		json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// quantise
		json_t *quantiseJ = json_boolean(quantise);
		json_object_set_new(rootJ, "quantise", quantiseJ);

		// offset
		json_t *offsetJ = json_boolean(offset);
		json_object_set_new(rootJ, "offset", offsetJ);


		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override {
		// quantise
		json_t *quantiseJ = json_object_get(rootJ, "quantise");
		
		if (quantiseJ) {
			quantise = json_boolean_value(quantiseJ);
		}

		// offset
		json_t *offsetJ = json_object_get(rootJ, "offset");
		
		if (offsetJ) {
			offset = json_boolean_value(offsetJ);
		}

	}

	rack::dsp::SchmittTrigger sampleTrigger;
	rack::dsp::SchmittTrigger holdTrigger;
	rack::dsp::SchmittTrigger clockTrigger;
	bogaudio::dsp::PinkNoiseGenerator pink;
	LowFrequencyOscillator oscillator;
	LowFrequencyOscillator clock;
	digital::AHPulseGenerator delayPhase;
	digital::AHPulseGenerator gatePhase;

	float target = 0.0f;
	float current = 0.0f;
	bool quantise = false;
	bool offset = false;
	bool delayState = false;
	bool gateState = false;
	
	// minimum and maximum slopes in volts per second
	const float slewMin = 0.1;
	const float slewMax = 10000.0;
	const float slewRatio = slewMin / slewMax;
	
	// Amount of extra slew per voltage difference
	const float shapeScale = 1.0 / 10.0;

	float delayTime;
	float gateTime;

};

void Generative::step() {
	
	AHModule::step();

	oscillator.setPitch(params[FREQ_PARAM].getValue() + params[FM_PARAM].getValue() * inputs[FM_INPUT].getVoltage());
	oscillator.offset = offset;
	oscillator.step(delta);

	clock.setPitch(clamp(params[CLOCK_PARAM].getValue() + inputs[CLOCK_INPUT].getVoltage(), -2.0f, 6.0f));
	clock.step(delta);

	float wavem = fabs(fmodf(params[WAVE_PARAM].getValue() + inputs[WAVE_INPUT].getVoltage(), 4.0f));

	float interp = 0.0f;
	bool toss = false;

	if (wavem < 1.0f)
		interp = crossfade(oscillator.sin(), oscillator.tri(), wavem) * 5.0; 
	else if (wavem < 2.0f)
		interp = crossfade(oscillator.tri(), oscillator.saw(), wavem - 1.0f) * 5.0;
	else if (wavem < 3.0f)
		interp = crossfade(oscillator.saw(), oscillator.sqr(), wavem - 2.0f) * 5.0;
	else 
		interp = crossfade(oscillator.sqr(), oscillator.sin(), wavem - 3.0f) * 5.0;


	// Capture (pink) noise
	float noise = clamp(pink.next() * 7.5f, -5.0f, 5.0f); // -5V to 5V
	float range = params[ATTN_PARAM].getValue();

	// Shift the noise floor
	if (offset) {
		noise += 5.0f;
	} 

	float noiseLevel = clamp(params[NOISE_PARAM].getValue() + inputs[NOISE_INPUT].getVoltage(), 0.0f, 1.0f);

	// Mixed the input AM signal or noise
	if (inputs[AM_INPUT].isConnected()) {
		interp = crossfade(interp, inputs[AM_INPUT].getVoltage(), params[AM_PARAM].getValue()) * range;
	} else {
		interp *= range;
	}

	// Mix noise
	float mixedSignal = (noise * noiseLevel + interp * (1.0f - noiseLevel));

	// Process gate
	bool sampleActive = inputs[SAMPLE_INPUT].isConnected();
	float sample = inputs[SAMPLE_INPUT].getVoltage();
	bool hold = inputs[HOLD_INPUT].getVoltage() > 0.000001f;

	bool isClocked = false;

	if (!sampleActive) {
		if (clockTrigger.process(clock.sqr())) {
			isClocked = true;
		}
	} else {
		if (sampleTrigger.process(sample)) {
			isClocked = true;
		}
	}

	// If we have no input or we have been a trigger on the sample input
	if (isClocked) {

		// If we are not in a delay or gate state process the tick, otherwise eat it
		if (!delayPhase.ishigh() && !gatePhase.ishigh()) {

			// Check against prob control
			float threshold = clamp(params[PROB_PARAM].getValue() + inputs[PROB_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			toss = (random::uniform() < threshold);

			// Tick is valid
			if (toss) {

				// Determine delay time
				float dlyLen = log2(params[DELAYL_PARAM].getValue());
				float dlySpr = log2(params[DELAYS_PARAM].getValue());

				double rndD = clamp(digital::gaussrand(), -2.0f, 2.0f);
				delayTime = clamp(dlyLen + dlySpr * rndD, 0.0f, 100.0f);
				
				// Trigger the respective delay pulse generator
				delayState = true;
				delayPhase.trigger(delayTime);
			}
		} 
	}

	// In delay state and finished waiting
	if (delayState && !delayPhase.process(delta)) {

		// set the target voltage
		target = mixedSignal;

		// Determine gate time
		float gateLen = log2(params[GATEL_PARAM].getValue());
		float gateSpr = log2(params[GATES_PARAM].getValue());

		double rndG = clamp(digital::gaussrand(), -2.0f, 2.0f);
		gateTime = clamp(gateLen + gateSpr * rndG, digital::TRIGGER, 100.0f);

		// Open the gate and set flags
		gatePhase.trigger(gateTime);
		gateState = true;
		delayState = false;			
	}

	// If not held slew voltages
	if (!hold) {

		// Curve calc
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
	}

	// Quantise or not
	float out;
	if (quantise) {
		int i;
		int d;
		out = music::getPitchFromVolts(current, music::NOTE_C, music::SCALE_CHROMATIC, &i, &d);
	} else {
		out = current;
	}

	// If the gate is open, set output to high
	if (gatePhase.process(delta)) {
		outputs[GATE_OUTPUT].setVoltage(10.0f);

		lights[GATE_LIGHT].setBrightnessSmooth(1.0f);
		lights[GATE_LIGHT + 1].setBrightnessSmooth(0.0f);

	} else {
		outputs[GATE_OUTPUT].setVoltage(0.0f);
		gateState = false;

		if (delayState) {
			lights[GATE_LIGHT].setBrightnessSmooth(0.0f);
			lights[GATE_LIGHT + 1].setBrightnessSmooth(1.0f);
		} else {
			lights[GATE_LIGHT].setBrightnessSmooth(0.0f);
			lights[GATE_LIGHT + 1].setBrightnessSmooth(0.0f);
		}

	}

	outputs[OUT_OUTPUT].setVoltage(out);
	outputs[NOISE_OUTPUT].setVoltage(noise * 2.0);
	outputs[LFO_OUTPUT].setVoltage(interp);
	outputs[MIXED_OUTPUT].setVoltage(mixedSignal);
	
}

struct GenerativeWidget : ModuleWidget {
	
	GenerativeWidget(Generative *module) {
		
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Generative.svg")));

		// LFO section
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 0, 0, false, false), module, Generative::FREQ_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 1, 0, false, false), module, Generative::WAVE_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 2, 0, false, false), module, Generative::FM_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 3, 0, false, false), module, Generative::AM_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 4, 0, false, false), module, Generative::NOISE_PARAM));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 1, false, false), module, Generative::WAVE_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 1, false, false), module, Generative::FM_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 3, 1, false, false), module, Generative::AM_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 4, 1, false, false), module, Generative::NOISE_INPUT));

		// Gate Section
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 2, false, false), module, Generative::SAMPLE_INPUT));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 1, 2, false, false), module, Generative::CLOCK_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 2, 2, false, false), module, Generative::PROB_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 3, 2, false, false), module, Generative::DELAYL_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 4, 2, false, false), module, Generative::GATEL_PARAM));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 3, false, false), module, Generative::CLOCK_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 3, false, false), module, Generative::PROB_INPUT));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 3, 3, false, false), module, Generative::DELAYS_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 4, 3, false, false), module, Generative::GATES_PARAM));

		// Curve Section
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 0, 4, false, false), module, Generative::SLOPE_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 1, 4, false, false), module, Generative::SPEED_PARAM));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 4, false, false), module, Generative::HOLD_INPUT));
		addChild(createLight<MediumLight<GreenRedLight>>(gui::getPosition(gui::LIGHT, 3, 4, false, false), module, Generative::GATE_LIGHT));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 4, 4, false, false), module, Generative::ATTN_PARAM)); 

		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 5, false, false), module, Generative::LFO_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 5, false, false), module, Generative::MIXED_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 2, 5, false, false), module, Generative::NOISE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 3, 5, false, false), module, Generative::GATE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 4, 5, false, false), module, Generative::OUT_OUTPUT));

	}

	void appendContextMenu(Menu *menu) override {
			Generative *gen = dynamic_cast<Generative*>(module);
			assert(gen);

			struct GenModeItem : MenuItem {
				Generative *gen;
				void onAction(const event::Action &e) override {
					gen->quantise ^= 1;
				}
				void step() override {
					rightText = gen->quantise ? "Quantised" : "Unquantised";
					MenuItem::step();
				}
			};

			struct GenOffsetItem : MenuItem {
				Generative *gen;
				void onAction(const event::Action &e) override {
					gen->offset ^= 1;
				}
				void step() override {
					rightText = gen->offset ? "0V - 10V" : "-5V to 5V";
					MenuItem::step();
				}
			};

			menu->addChild(construct<MenuLabel>());
			menu->addChild(construct<GenModeItem>(&MenuItem::text, "Quantise", &GenModeItem::gen, gen));
			menu->addChild(construct<GenOffsetItem>(&MenuItem::text, "CV Offset", &GenOffsetItem::gen, gen));
	}
};

Model *modelGenerative = createModel<Generative, GenerativeWidget>("Generative");
