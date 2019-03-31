#include <string.h>
#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

static const int BUFFER_SIZE = 512;

using namespace ah;

std::array<NVGcolor, 16> colourMap = {
nvgRGBA(255,	0,	0, 240),
nvgRGBA(223,	0,	32, 240),
nvgRGBA(191,	0,	64, 240),
nvgRGBA(159,	0,	96, 240),
nvgRGBA(128,	0,	128, 240),
nvgRGBA(96,	0,	159, 240),
nvgRGBA(64,	0,	191, 240),
nvgRGBA(32,	0,	223, 240),
nvgRGBA(0,	32,	223, 240),
nvgRGBA(0,	64,	191, 240),
nvgRGBA(0,	96,	159, 240),
nvgRGBA(0,	128,	128, 240),
nvgRGBA(0,	159,	96, 240),
nvgRGBA(0,	191,	64, 240),
nvgRGBA(0,	223,	32, 240),
nvgRGBA(0,	255,	0, 240)};

/** 
 * PolyScope, based on Andrew Belt's Scope module.
 */
struct PolyScope : core::AHModule {
	enum ParamIds {
		SCALE_PARAM,
		SPREAD_PARAM,
		TIME_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		POLY_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	float buffer[16][BUFFER_SIZE] = {};
	int bufferIndex = 0;
	float frameIndex = 0;

	dsp::SchmittTrigger resetTrigger;

	PolyScope() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) { 
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(SCALE_PARAM, -2.0f, 2.0f, 0.0f);
		configParam(SPREAD_PARAM, 0.0f, 3.0f, 1.5f);
		configParam(TIME_PARAM, 6.0f, 16.0f, 14.0f);
	}

	void process(const ProcessArgs &args) override {

		// Compute time
		float deltaTime = std::pow(2.0f, -params[TIME_PARAM].getValue());
		int frameCount = (int) std::ceil(deltaTime * args.sampleRate);

		// Add frame to buffer
		if (bufferIndex < BUFFER_SIZE) {
			if (++frameIndex > frameCount) {
				for (int i = 0; i < 16; i++) {
					buffer[i][bufferIndex] = inputs[POLY_INPUT].getVoltage(i);
				}
				bufferIndex++;
				frameIndex = 0;
			}
		}

		// Are we waiting on the next trigger?
		if (bufferIndex >= BUFFER_SIZE) {
			// Reset the Schmitt trigger so we don't trigger immediately if the input is high
			if (frameIndex == 0) {
				resetTrigger.reset();
			}
			frameIndex++;

			// Must go below 0.1fV to trigger
			float gate = inputs[POLY_INPUT].getVoltage(0);

			// Reset if triggered
			float holdTime = 0.1f;
			float trigParam = 0.0f;
			if (resetTrigger.process(rescale(gate, trigParam - 0.1f, trigParam, 0.f, 1.f)) || (frameIndex >= args.sampleRate * holdTime)) {
				bufferIndex = 0; frameIndex = 0; return;
			}

			// Reset if we've waited too long
			if (frameIndex >= args.sampleRate * holdTime) {
				bufferIndex = 0; frameIndex = 0; return;
			}
		}
	}
};


struct PolyScopeDisplay : TransparentWidget {
	PolyScope *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	PolyScopeDisplay() { }

	void drawWaveform(const DrawArgs &args, float *valuesX) {
		if (!valuesX)
			return;
		nvgSave(args.vg);
		Rect b = Rect(Vec(0, 15), box.size.minus(Vec(0, 15*2)));
		nvgScissor(args.vg, b.pos.x, b.pos.y, b.size.x, b.size.y);
		nvgBeginPath(args.vg);
		// Draw maximum display left to right
		for (int i = 0; i < BUFFER_SIZE; i++) {
			float x, y;
			x = (float)i / (BUFFER_SIZE - 1);
			y = valuesX[i] / 2.0f + 0.5f;

			Vec p;
			p.x = b.pos.x + b.size.x * x;
			p.y = b.pos.y + b.size.y * (1.0f - y);
			if (i == 0)
				nvgMoveTo(args.vg, p.x, p.y);
			else
				nvgLineTo(args.vg, p.x, p.y);
		}
		nvgLineCap(args.vg, NVG_ROUND);
		nvgMiterLimit(args.vg, 2.0f);
		nvgStrokeWidth(args.vg, 1.25f);
		nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
		nvgStroke(args.vg);
		nvgResetScissor(args.vg);
		nvgRestore(args.vg);
	}

	void draw(const DrawArgs &args) override {
		if (!module)
			return;

		float gain = std::pow(2.0f, module->params[PolyScope::SCALE_PARAM].getValue());
		float offset = module->params[PolyScope::SPREAD_PARAM].getValue();

		float values[16][BUFFER_SIZE];
		for (int i = 0; i < BUFFER_SIZE; i++) {
			for (int k = 0; k < 16; k++) {
				values[k][i] = (module->buffer[k][i] + ((k - 8) * offset)) * gain / 10.0f;
			}
		}

		for (int i = 0; i < 16; i++) {
			nvgStrokeColor(args.vg, colourMap[i]);
			drawWaveform(args, values[i]);
		}
	}
};


struct PolyScopeWidget : ModuleWidget {
	PolyScopeWidget(PolyScope *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyScope.svg")));

		{
			PolyScopeDisplay *display = new PolyScopeDisplay();
			display->module = module;
			display->box.pos = Vec(0, 20);
			display->box.size = Vec(345, 310);
			addChild(display);
		}

		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 3, 5, true, false), module, PolyScope::SCALE_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 5, 5, true, false), module, PolyScope::SPREAD_PARAM));
		addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 7, 5, true, false), module, PolyScope::TIME_PARAM));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 5, true, false), module, PolyScope::POLY_INPUT));
	}
};


Model *modelPolyScope = createModel<PolyScope, PolyScopeWidget>("PolyScope");
