#include "common.hpp"

#include "dsp/noise.hpp"

#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"
#include "VCO.hpp"

struct Generative : AHModule {

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

	Generative() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) { 

		// LFO section
		params[FREQ_PARAM].config(-8.0f, 10.0f, 1.0f);
		params[WAVE_PARAM].config(0.0f, 4.0f, 1.5f);
		params[FM_PARAM].config(0.0f, 1.0f, 0.5f);
		params[AM_PARAM].config(0.0f, 1.0f, 0.5f);
		params[NOISE_PARAM].config(0.0f, 1.0f, 0.5f);
		params[CLOCK_PARAM].config(-2.0, 6.0, 1.0);
		params[PROB_PARAM].config(0.0, 1.0, 1.0);
		params[DELAYL_PARAM].config(1.0f, 2.0f, 1.0f);
		params[GATEL_PARAM].config(1.0f, 2.0f, 1.0f);
		params[DELAYS_PARAM].config(1.0f, 2.0f, 1.0f);
		params[GATES_PARAM].config(1.0f, 2.0f, 1.0f);
		params[SLOPE_PARAM].config(0.0, 1.0, 0.0);
		params[SPEED_PARAM].config(0.0, 1.0, 0.0);
		params[ATTN_PARAM].config(0.0, 1.0, 1.0); 

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

	Core core;

	dsp::SchmittTrigger sampleTrigger;
	dsp::SchmittTrigger holdTrigger;
	dsp::SchmittTrigger clockTrigger;
	bogaudio::dsp::PinkNoiseGenerator pink;
	LowFrequencyOscillator oscillator;
	LowFrequencyOscillator clock;
	AHPulseGenerator delayPhase;
	AHPulseGenerator gatePhase;

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

	oscillator.setPitch(params[FREQ_PARAM].value + params[FM_PARAM].value * inputs[FM_INPUT].value);
	oscillator.offset = offset;
	oscillator.step(delta);

	clock.setPitch(clamp(params[CLOCK_PARAM].value + inputs[CLOCK_INPUT].value, -2.0f, 6.0f));
	clock.step(delta);

	float wavem = fabs(fmodf(params[WAVE_PARAM].value + inputs[WAVE_INPUT].value, 4.0f));

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
	float range = params[ATTN_PARAM].value;

	// Shift the noise floor
	if (offset) {
		noise += 5.0f;
	} 

	float noiseLevel = clamp(params[NOISE_PARAM].value + inputs[NOISE_INPUT].value, 0.0f, 1.0f);

	// Mixed the input AM signal or noise
	if (inputs[AM_INPUT].active) {
		interp = crossfade(interp, inputs[AM_INPUT].value, params[AM_PARAM].value) * range;
	} else {
		interp *= range;
	}

	// Mix noise
	float mixedSignal = (noise * noiseLevel + interp * (1.0f - noiseLevel));

	// Process gate
	bool sampleActive = inputs[SAMPLE_INPUT].active;
	float sample = inputs[SAMPLE_INPUT].value;
	bool hold = inputs[HOLD_INPUT].value > 0.000001f;

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
			float threshold = clamp(params[PROB_PARAM].value + inputs[PROB_INPUT].value / 10.f, 0.f, 1.f);
			toss = (random::uniform() < threshold);

			// Tick is valid
			if (toss) {

				// Determine delay time
				float dlyLen = log2(params[DELAYL_PARAM].value);
				float dlySpr = log2(params[DELAYS_PARAM].value);

				double rndD = clamp(core.gaussrand(), -2.0f, 2.0f);
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
		float gateLen = log2(params[GATEL_PARAM].value);
		float gateSpr = log2(params[GATES_PARAM].value);

		double rndG = clamp(core.gaussrand(), -2.0f, 2.0f);
		gateTime = clamp(gateLen + gateSpr * rndG, Core::TRIGGER, 100.0f);

		// Open the gate and set flags
		gatePhase.trigger(gateTime);
		gateState = true;
		delayState = false;			
	}

	// If not held slew voltages
	if (!hold) {

		// Curve calc
		float shape = params[SLOPE_PARAM].value;
		float speed = params[SPEED_PARAM].value;	
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
		out = CoreUtil().getPitchFromVolts(current, Core::NOTE_C, Core::SCALE_CHROMATIC, &i, &d);
	} else {
		out = current;
	}

	// If the gate is open, set output to high
	if (gatePhase.process(delta)) {
		outputs[GATE_OUTPUT].value = 10.0f;

		lights[GATE_LIGHT].setBrightnessSmooth(1.0f);
		lights[GATE_LIGHT + 1].setBrightnessSmooth(0.0f);

	} else {
		outputs[GATE_OUTPUT].value = 0.0f;
		gateState = false;

		if (delayState) {
			lights[GATE_LIGHT].setBrightnessSmooth(0.0f);
			lights[GATE_LIGHT + 1].setBrightnessSmooth(1.0f);
		} else {
			lights[GATE_LIGHT].setBrightnessSmooth(0.0f);
			lights[GATE_LIGHT + 1].setBrightnessSmooth(0.0f);
		}

	}

	outputs[OUT_OUTPUT].value = out;
	outputs[NOISE_OUTPUT].value = noise * 2.0;
	outputs[LFO_OUTPUT].value = interp;
	outputs[MIXED_OUTPUT].value = mixedSignal;
	
}

struct GenerativeWidget : ModuleWidget {
	
	GenerativeWidget(Generative *module) {
		
		setModule(module);
		setPanel(SVG::load(asset::plugin(pluginInstance, "res/Generative.svg")));
		UI ui;

		// LFO section
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 0, 0, false, false), module, Generative::FREQ_PARAM));
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 1, 0, false, false), module, Generative::WAVE_PARAM));
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 2, 0, false, false), module, Generative::FM_PARAM));
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 3, 0, false, false), module, Generative::AM_PARAM));
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 4, 0, false, false), module, Generative::NOISE_PARAM));
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 1, 1, false, false), module, Generative::WAVE_INPUT));
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 1, false, false), module, Generative::FM_INPUT));
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 3, 1, false, false), module, Generative::AM_INPUT));
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 1, false, false), module, Generative::NOISE_INPUT));

		// Gate Section
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 2, false, false), module, Generative::SAMPLE_INPUT));
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 1, 2, false, false), module, Generative::CLOCK_PARAM));
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 2, 2, false, false), module, Generative::PROB_PARAM));
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 3, 2, false, false), module, Generative::DELAYL_PARAM));
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 4, 2, false, false), module, Generative::GATEL_PARAM));

		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 1, 3, false, false), module, Generative::CLOCK_INPUT));
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 3, false, false), module, Generative::PROB_INPUT));
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 3, 3, false, false), module, Generative::DELAYS_PARAM));
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 4, 3, false, false), module, Generative::GATES_PARAM));

		// Curve Section
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 0, 4, false, false), module, Generative::SLOPE_PARAM));
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 1, 4, false, false), module, Generative::SPEED_PARAM));
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 4, false, false), module, Generative::HOLD_INPUT));
		addChild(createLight<MediumLight<GreenRedLight>>(ui.getPosition(UI::LIGHT, 3, 4, false, false), module, Generative::GATE_LIGHT));
		addParam(createParam<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 4, 4, false, false), module, Generative::ATTN_PARAM)); 

		addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 5, false, false), module, Generative::LFO_OUTPUT));
		addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 1, 5, false, false), module, Generative::MIXED_OUTPUT));
		addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 5, false, false), module, Generative::NOISE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 3, 5, false, false), module, Generative::GATE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 4, 5, false, false), module, Generative::OUT_OUTPUT));

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
