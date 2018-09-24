#include "util/common.hpp"
#include "dsp/digital.hpp"

#include "dsp/noise.hpp"

#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"


struct LowFrequencyOscillator {
	float phase = 0.0f;
	float pw = 0.5f;
	float freq = 1.0f;

	LowFrequencyOscillator() {}
	void setPitch(float pitch) {
		pitch = fminf(pitch, 10.0f);
		freq = powf(2.0f, pitch);
	}
	void setPulseWidth(float pw_) {
		const float pwMin = 0.01f;
		pw = clamp(pw_, pwMin, 1.0f - pwMin);
	}
	void step(float dt) {
		float deltaPhase = fminf(freq * dt, 0.5f);
		phase += deltaPhase;
		if (phase >= 1.0f)
			phase -= 1.0f;
	}
	float sin() {
		return sinf(2*M_PI * phase);
	}
	float tri(float x) {
		return 4.0f * fabsf(x - roundf(x));
	}
	float tri() {
		return -1.0f + tri(phase - 0.75f);
	}
	float saw(float x) {
		return 2.0f * (x - roundf(x));
	}
	float saw() {
		return saw(phase);
	}
	float sqr() {
		return (phase < pw) ? 1.0f : -1.0f;
	}
	float light() {
		return sinf(2*M_PI * phase);
	}
};

struct Generative : AHModule {

	enum ParamIds {
		FREQ_PARAM,
		FM_PARAM,
		WAVE_PARAM,
		PROB_PARAM,
		BLEND_PARAM,
		SLOPE_PARAM,
		SPEED_PARAM,
		RANGE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		SAMPLE_INPUT,
		PROB_INPUT,
		HOLD_INPUT,
		FM_INPUT,
		WAVE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CURVE_OUTPUT,
		NOISE_OUTPUT,
		LFO_OUTPUT,
		TOSS_OUTPUT,
		BLEND_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Generative() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

	}
	
	void step() override;

	Core core;

	SchmittTrigger sampleTrigger;
	SchmittTrigger holdTrigger;
	bogaudio::dsp::PinkNoiseGenerator pink;
	LowFrequencyOscillator oscillator;
	PulseGenerator tossPulse;

	float target = 0.0f;
	float current = 0.0f;
	bool quantise = false;
	
	// minimum and maximum slopes in volts per second
	const float slewMin = 0.1;
	const float slewMax = 10000.0;
	
	const float slewRatio = slewMin / slewMax;
	
	// Amount of extra slew per voltage difference
	const float shapeScale = 1.0/10.0;
	
};

void Generative::step() {
	
	AHModule::step();

	oscillator.setPitch(params[FREQ_PARAM].value + params[FM_PARAM].value * inputs[FM_INPUT].value);
	oscillator.step(delta);

	float wave = clamp(params[WAVE_PARAM].value + inputs[WAVE_INPUT].value, 0.0f, 3.0f);

	float interp;
	bool toss = false;

	if (wave < 1.0f)
		interp = crossfade(oscillator.sin(), oscillator.tri(), wave);
	else if (wave < 2.0f)
		interp = crossfade(oscillator.tri(), oscillator.saw(), wave - 1.0f);
	else
		interp = crossfade(oscillator.saw(), oscillator.sqr(), wave - 2.0f);

	// Capture (pink) noise
	float noise = clamp(pink.next() * 1.5f, -1.0f, 1.0f);

	// Mix noise with LFO and scale
	float blend = params[BLEND_PARAM].value;
	float range = params[RANGE_PARAM].value;
	float curve = (noise * blend + (1.0f - blend) * interp) * 5.0 * range;

	// Process gate
	float sample = inputs[SAMPLE_INPUT].value;
	float hold = inputs[HOLD_INPUT].value;

	// Curve calc
	float shape = params[SLOPE_PARAM].value;
	float speed = params[SPEED_PARAM].value;	
	float slew = slewMax * powf(slewRatio, speed);

	if (!(hold > 1.0f)) {
		if (sampleTrigger.process(sample)) {
			float threshold = clamp(params[PROB_PARAM].value + inputs[PROB_INPUT].value / 10.f, 0.f, 1.f);
			toss = randomUniform() < threshold;

			if (toss) {
				target = curve;
				tossPulse.trigger(Core::TRIGGER);
			}
		}

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

	float out;
	if (quantise) {
		int i;
		int d;
		out = CoreUtil().getPitchFromVolts(current, Core::NOTE_C, Core::SCALE_CHROMATIC, &i, &d);
	} else {
		out = current;
	}


	bool tPulse = tossPulse.process(delta);

	outputs[CURVE_OUTPUT].value = out;
	outputs[NOISE_OUTPUT].value = noise * 10.0;	
	outputs[LFO_OUTPUT].value = interp * 5.0f;
	outputs[TOSS_OUTPUT].value = tPulse ? 10.0f : 0.0f;
	outputs[BLEND_OUTPUT].value = curve;
	
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
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 4, 0, false, false), module, Generative::PROB_PARAM, 0.0, 1.0, 0.5));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 6, 0, false, false), module, Generative::BLEND_PARAM, 0.0, 1.0, 0.5)); 
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 8, 0, false, false), module, Generative::SLOPE_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 9, 0, false, false), module, Generative::SPEED_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<AHKnobNoSnap>(ui.getPosition(UI::KNOB, 10, 0, false, false), module, Generative::RANGE_PARAM, 0.0, 1.0, 1.0)); 

		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 1, 1, false, false), Port::INPUT, module, Generative::WAVE_INPUT));
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 2, 1, false, false), Port::INPUT, module, Generative::FM_INPUT));
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 4, 1, false, false), Port::INPUT, module, Generative::PROB_INPUT));

		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 0, 2, false, false), Port::INPUT, module, Generative::SAMPLE_INPUT));
		addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 1, 2, false, false), Port::INPUT, module, Generative::HOLD_INPUT));
		
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 0, 3, false, false), Port::OUTPUT, module, Generative::CURVE_OUTPUT));
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 1, 3, false, false), Port::OUTPUT, module, Generative::NOISE_OUTPUT));
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 2, 3, false, false), Port::OUTPUT, module, Generative::LFO_OUTPUT));
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 3, 3, false, false), Port::OUTPUT, module, Generative::TOSS_OUTPUT));
		addOutput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 4, 3, false, false), Port::OUTPUT, module, Generative::BLEND_OUTPUT));

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
					rightText = gen->quantise ? "Quantise" : "Unquantised";
					MenuItem::step();
				}
			};

			menu->addChild(construct<MenuLabel>());
			menu->addChild(construct<GenModeItem>(&MenuItem::text, "Quantise", &GenModeItem::gen, gen));
	}
};

Model *modelGenerative = Model::create<Generative, GenerativeWidget>( "Amalgamated Harmonics", "Generative", "Generative", NOISE_TAG);

