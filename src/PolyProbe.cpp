#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct PolyProbe : core::AHModule {

	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		POLYCV_INPUT,
		POLYGATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	int nGateChannels = 0;
	int nCVChannels = 0;
	bool hasGateIn = false;
	bool hasCVIn = false;
	bool gate[16];
	float cv[16];

	PolyProbe() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {	}

	void step() override {
	
		AHModule::step();

		nGateChannels = 0;
		nCVChannels = 0;
		hasGateIn = false;
		hasCVIn = false;
		for (int i = 0; i < 16; i++) {
			gate[i] = false;
			cv[i] = 0.0;
		}

		if (inputs[POLYGATE_INPUT].isConnected()) {
			hasGateIn = true;
			nGateChannels = inputs[POLYGATE_INPUT].getChannels();
			for (int i = 0; i < nGateChannels; i++) {
				if (inputs[POLYGATE_INPUT].getNormalVoltage(0.0, i) > 0.0f) {
					gate[i] = true;
				} else {
					gate[i] = false;
				}
			}
		} else {
			hasGateIn = false;
		}

		// Process poly input
		if (inputs[POLYCV_INPUT].isConnected()) {
			hasCVIn = true;
			nCVChannels = inputs[POLYCV_INPUT].getChannels();
			for (int i = 0; i < nCVChannels; i++) {
				cv[i] = inputs[POLYCV_INPUT].getVoltage(i);
			}
		} else {
			hasCVIn = false;
		}
	}

};

struct PolyProbeDisplay : TransparentWidget {
	
	PolyProbe *module;
	std::shared_ptr<Font> font;

	PolyProbeDisplay() {
		font = Font::load(asset::plugin(pluginInstance, "res/EurostileBold.ttf"));
	}

	void draw(const DrawContext &ctx) override {
	
		nvgFontSize(ctx.vg, 14);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgTextLetterSpacing(ctx.vg, -1);

		char text[128];

		int j = 0;

		nvgTextAlign(ctx.vg, NVG_ALIGN_LEFT);
		if (module->hasCVIn) {
			if (module->nCVChannels == module->nGateChannels) {
				nvgFillColor(ctx.vg, nvgRGBA(0, 255, 0, 0xff));
				snprintf(text, sizeof(text), "CV In: %d", module->nCVChannels);
			} else {
				if (module->hasGateIn) {
					nvgFillColor(ctx.vg, nvgRGBA(255, 0, 0, 0xff));
					snprintf(text, sizeof(text), "CV In: %d (channel count mismatch)", module->nCVChannels);
				} else {
					nvgFillColor(ctx.vg, nvgRGBA(0, 255, 0, 0xff));
					snprintf(text, sizeof(text), "CV In: %d", module->nCVChannels);
				}
			}
		} else {
			nvgFillColor(ctx.vg, nvgRGBA(255, 0, 0, 0xff));
			snprintf(text, sizeof(text), "No CV in");
		}
		nvgText(ctx.vg, box.pos.x + 5, box.pos.y + j * 16, text, NULL);
		j++;

		if (module->hasGateIn) {
			nvgFillColor(ctx.vg, nvgRGBA(0, 255, 0, 0xff));
			snprintf(text, sizeof(text), "Gate in: %d", module->nGateChannels);
		} else {
			nvgFillColor(ctx.vg, nvgRGBA(255, 0, 0, 0xff));
			snprintf(text, sizeof(text), "No gate in");
		}
		nvgText(ctx.vg, box.pos.x + 5, box.pos.y + j * 16, text, NULL);
		j = j + 2;

		for (int i = 0; i < 16; i++)  {
			if (i >= module->nCVChannels) {
				nvgFillColor(ctx.vg, nvgRGBA(255, 0, 0, 0xff));
				snprintf(text, sizeof(text), "--");
			} else {
				if (module->hasGateIn) {
					if (module->gate[i]) {
						nvgFillColor(ctx.vg, nvgRGBA(0, 255, 0, 0xff));
						snprintf(text, sizeof(text), "%02d GATE        %f", i, module->cv[i]);
					} else {
						nvgFillColor(ctx.vg, nvgRGBA(255, 0, 0, 0xff));
						snprintf(text, sizeof(text), "%02d NOGATE   %f", i, module->cv[i]);
					}
				} else {
					nvgFillColor(ctx.vg, nvgRGBA(255, 165, 0, 0xff));
					snprintf(text, sizeof(text), "%02d NOGATE   %f", i, module->cv[i]);
				}
			}
			nvgText(ctx.vg, box.pos.x + 5, box.pos.y + i * 16 + j * 16, text, NULL);
		}
	}
	
};

struct PolyProbeWidget : ModuleWidget {

	PolyProbeWidget(PolyProbe *module) {
		
		setModule(module);
		setPanel(SVG::load(asset::plugin(pluginInstance, "res/PolyProbe.svg")));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, 0, false, false), module, PolyProbe::POLYCV_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, 0, false, false), module, PolyProbe::POLYGATE_INPUT));

		if (module != NULL) {
			PolyProbeDisplay *displayW = createWidget<PolyProbeDisplay>(Vec(0, 20));
			displayW->box.size = Vec(240, 230);
			displayW->module = module;
			addChild(displayW);
		}

	}

};

Model *modelPolyProbe = createModel<PolyProbe, PolyProbeWidget>("PolyProbe");
