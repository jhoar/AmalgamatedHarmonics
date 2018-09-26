#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "dsp/noise.hpp"

#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"

// TODO delay control, UI, random gate length, wave range map to -5-5v

// Andrew Belt's LFO-2 code
struct LowFrequencyOscillator {
	float phase = 0.0f;
	float pw = 0.5f;
	float freq = 1.0f;
	bool offset = false;
	bool invert = false;
	SchmittTrigger resetTrigger;

	LowFrequencyOscillator() {}
	void setPitch(float pitch) {
		pitch = fminf(pitch, 10.0f);
		freq = powf(2.0f, pitch);
	}
	void setPulseWidth(float pw_) {
		const float pwMin = 0.01f;
		pw = clamp(pw_, pwMin, 1.0f - pwMin);
	}
	void setReset(float reset) {
		if (resetTrigger.process(reset / 0.01f)) {
			phase = 0.0f;
		}
	}
	void step(float dt) {
		float deltaPhase = fminf(freq * dt, 0.5f);
		phase += deltaPhase;
		if (phase >= 1.0f)
			phase -= 1.0f;
	}
	float sin() {
		if (offset)
			return 1.0f - cosf(2*M_PI * phase) * (invert ? -1.0f : 1.0f);
		else
			return sinf(2*M_PI * phase) * (invert ? -1.0f : 1.0f);
	}
	float tri(float x) {
		return 4.0f * fabsf(x - roundf(x));
	}
	float tri() {
		if (offset)
			return tri(invert ? phase - 0.5f : phase);
		else
			return -1.0f + tri(invert ? phase - 0.25f : phase - 0.75f);
	}
	float saw(float x) {
		return 2.0f * (x - roundf(x));
	}
	float saw() {
		if (offset)
			return invert ? 2.0f * (1.0f - phase) : 2.0f * phase;
		else
			return saw(phase) * (invert ? -1.0f : 1.0f);
	}
	float sqr() {
		float sqr = (phase < pw) ^ invert ? 1.0f : -1.0f;
		return offset ? sqr + 1.0f : sqr;
	}
	float light() {
		return sinf(2*M_PI * phase);
	}
};
struct Generative : AHModule {

	enum ParamIds {
		FREQ_PARAM,
		FM_PARAM,
		AM_PARAM,
		WAVE_PARAM,
		PROB_PARAM,
		SLOPE_PARAM,
		SPEED_PARAM,
		RANGE_PARAM,
		DELAY_PARAM,
		GATE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		SAMPLE_INPUT,
		PROB_INPUT,
		HOLD_INPUT,
		FM_INPUT,
		AM_INPUT,
		WAVE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CURVE_OUTPUT,
		NOISE_OUTPUT,
		LFO_OUTPUT,
		GATE_OUTPUT,
		BLEND_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Generative() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;

	Core core;

	SchmittTrigger sampleTrigger;
	SchmittTrigger holdTrigger;
	bogaudio::dsp::PinkNoiseGenerator pink;
	LowFrequencyOscillator oscillator;
	AHPulseGenerator delayPhase;
	AHPulseGenerator gatePhase;

	float target = 0.0f;
	float current = 0.0f;
	bool quantise = false;
	bool offset = false;
	bool gate = false;
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

	float wave = clamp(params[WAVE_PARAM].value + inputs[WAVE_INPUT].value, 0.0f, 3.0f);

	float interp = 0.0f;
	bool toss = false;

	if (wave < 1.0f)
		interp = crossfade(oscillator.sin(), oscillator.tri(), wave) * 5.0; 
	else if (wave < 2.0f)
		interp = crossfade(oscillator.tri(), oscillator.saw(), wave - 1.0f) * 5.0;
	else
		interp = crossfade(oscillator.saw(), oscillator.sqr(), wave - 2.0f) * 5.0;

	// Capture (pink) noise
	float noise = clamp(pink.next() * 7.5f, -5.0f, 5.0f);
	float range = params[RANGE_PARAM].value;

	// Mix AM (noise if no AM input) with LFO and scale
	float mixedSignal;
	float noiseOffset = 0.0f;

	// Shift the noise floor
	if (offset) {
		noiseOffset = 5.0f;
	} 

	// Mixed the input AM signal or noise
	if (inputs[AM_INPUT].active) {
	 	mixedSignal = crossfade(interp, inputs[AM_INPUT].value, params[AM_PARAM].value) * range;
	} else {
	 	mixedSignal = ((noise + noiseOffset) * params[AM_PARAM].value + interp * (1.0f - params[AM_PARAM].value)) * range;
	}

	// Process gate
	bool sampleActive = inputs[SAMPLE_INPUT].active;
	float sample = inputs[SAMPLE_INPUT].value;
	bool hold = inputs[HOLD_INPUT].value > 0.000001f;

	// Curve calc
	float shape = params[SLOPE_PARAM].value;
	float speed = params[SPEED_PARAM].value;	
	float slew = slewMax * powf(slewRatio, speed);

	// If we have no input or we have been a trigger on the sample input
	if (!sampleActive || sampleTrigger.process(sample)) {

		// If we are not in a delay state
		if (!delayPhase.ishigh()) {

			float dlySpr = log2(params[DELAY_PARAM].value);
			float gateSpr = log2(params[GATE_PARAM].value);

			// Determine delay time
			double rndD = fabs(clamp(core.gaussrand(), -2.0f, 2.0f));
			if (sampleActive) {
				delayTime = clamp(dlySpr * rndD, 0.0f, 100.0f);
			} else {
				delayTime = clamp(dlySpr * rndD, Core::TRIGGER, 100.0f); // Have a minumum delay time if no input on SAMPLE
			}
			
			// Determine gate time
			if (gate) {
				double rndG = clamp(core.gaussrand(), -2.0f, 2.0f);
				gateTime = clamp(gateSpr * rndG, Core::TRIGGER, 100.0f);
			} else {
				gateTime = Core::TRIGGER;
			}

			// Trigger the respective delay pulse generator
			delayState = true;
			delayPhase.trigger(delayTime);

		} 
	}

	// In delay state and finished waiting
	if (delayState && !delayPhase.process(delta)) {
		float threshold = clamp(params[PROB_PARAM].value + inputs[PROB_INPUT].value / 10.f, 0.f, 1.f);
		toss = (randomUniform() < threshold);

		if (toss) {
			target = mixedSignal;
			gatePhase.trigger(gateTime);
			gateState = true;
		}

		// Whatever happens, end delay phase
		delayState = false;			

	}

	// If not held slew voltages
	if (!hold) {
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
	} else {
		outputs[GATE_OUTPUT].value = 0.0f;
		gateState = false;
	}

	outputs[CURVE_OUTPUT].value = out;
	outputs[NOISE_OUTPUT].value = noise * 2.0;
	outputs[LFO_OUTPUT].value = interp;
	outputs[BLEND_OUTPUT].value = mixedSignal;
	
}

struct GenerativeWidget : ModuleWidget {
	
	GenerativeWidget(Generative *module) : ModuleWidget(module) {
		
		UI ui;
		
		box.size = Vec(525, 380);

		{
			SVGPanel *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/Generative.svg")));
			addChild(panel);
		}

		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 0, 0, false, false), module, Generative::FREQ_PARAM, -8.0f, 10.0f, 1.0f));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 1, 0, false, false), module, Generative::WAVE_PARAM, 0.0f, 3.0f, 1.5f));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 2, 0, false, false), module, Generative::FM_PARAM, 0.0f, 1.0f, 0.5f));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 3, 0, false, false), module, Generative::AM_PARAM, 0.0f, 1.0f, 0.5f));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 5, 0, false, false), module, Generative::PROB_PARAM, 0.0, 1.0, 0.5));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 6, 0, false, false), module, Generative::DELAY_PARAM, 1.0f, 2.0f, 1.0f));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 7, 0, false, false), module, Generative::GATE_PARAM, 1.0f, 2.0f, 1.0f));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 8, 0, false, false), module, Generative::SLOPE_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 9, 0, false, false), module, Generative::SPEED_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 10, 0, false, false), module, Generative::RANGE_PARAM, 0.0, 1.0, 1.0)); 

		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 1, 1, false, false), Port::INPUT, module, Generative::WAVE_INPUT));
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 2, 1, false, false), Port::INPUT, module, Generative::FM_INPUT));
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 3, 1, false, false), Port::INPUT, module, Generative::AM_INPUT));
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 5, 1, false, false), Port::INPUT, module, Generative::PROB_INPUT));

		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 0, 2, false, false), Port::INPUT, module, Generative::SAMPLE_INPUT));
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 1, 2, false, false), Port::INPUT, module, Generative::HOLD_INPUT));
		
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 0, 3, false, false), Port::OUTPUT, module, Generative::LFO_OUTPUT));
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 1, 3, false, false), Port::OUTPUT, module, Generative::BLEND_OUTPUT));
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 2, 3, false, false), Port::OUTPUT, module, Generative::NOISE_OUTPUT));
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 5, 3, false, false), Port::OUTPUT, module, Generative::GATE_OUTPUT));
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 10, 3, false, false), Port::OUTPUT, module, Generative::CURVE_OUTPUT));

	}

	void appendContextMenu(Menu *menu) override {
			Generative *gen = dynamic_cast<Generative*>(module);
			assert(gen);

			struct GenModeItem : MenuItem {
				Generative *gen;
				void onAction(EventAction &e) override {
					gen->quantise ^= 1;
				}
				void step() override {
					rightText = gen->quantise ? "Quantised" : "Unquantised";
					MenuItem::step();
				}
			};

			struct GenOffsetItem : MenuItem {
				Generative *gen;
				void onAction(EventAction &e) override {
					gen->offset ^= 1;
				}
				void step() override {
					rightText = gen->offset ? "0V - 10V" : "-5V to 5V";
					MenuItem::step();
				}
			};

			struct GenGateItem : MenuItem {
				Generative *gen;
				void onAction(EventAction &e) override {
					gen->gate ^= 1;
				}
				void step() override {
					rightText = gen->gate ? "Gate" : "Trigger";
					MenuItem::step();
				}
			};

			menu->addChild(construct<MenuLabel>());
			menu->addChild(construct<GenModeItem>(&MenuItem::text, "Quantise", &GenModeItem::gen, gen));
			menu->addChild(construct<GenOffsetItem>(&MenuItem::text, "CV Offset", &GenOffsetItem::gen, gen));
			menu->addChild(construct<GenGateItem>(&MenuItem::text, "Gate", &GenGateItem::gen, gen));
	}
};

Model *modelGenerative = Model::create<Generative, GenerativeWidget>( "Amalgamated Harmonics", "Generative", "Generative", NOISE_TAG);
