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
	float gateCV[16];

	PolyProbe() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {	}

	void step() override {
	
		AHModule::step();

		if (!inputs[POLYGATE_INPUT].isConnected()) {
			hasGateIn = false;
			nGateChannels = 0;
			for (int i = 0; i < 16; i++) {
				gate[i] = false;
			}
		}

		if (!inputs[POLYCV_INPUT].isConnected()) {
			nCVChannels = 0;
			hasCVIn = false;
			for (int i = 0; i < 16; i++) {
				cv[i] = 0.0;
			}
		}

		if (inputs[POLYGATE_INPUT].isConnected()) {
			hasGateIn = true;
			nGateChannels = inputs[POLYGATE_INPUT].getChannels();
			for (int i = 0; i < nGateChannels; i++) {
				float cvg = inputs[POLYGATE_INPUT].getVoltage(i);
				if (cvg > 0.0f) {
					gate[i] = true;
					gateCV[i] = cvg;
				} 
			}
		} 

		// Process poly input
		if (inputs[POLYCV_INPUT].isConnected()) {
			hasCVIn = true;
			nCVChannels = inputs[POLYCV_INPUT].getChannels();
			for (int i = 0; i < nCVChannels; i++) {
				cv[i] = inputs[POLYCV_INPUT].getVoltage(i);
			}
		} 
	}
};

struct PolyProbeDisplay : TransparentWidget {
	
	PolyProbe *module;
	std::shared_ptr<Font> font;

	PolyProbeDisplay() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/EurostileBold.ttf"));
	}

	void draw(const DrawContext &ctx) override {
	
		nvgFontSize(ctx.vg, 14);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgTextLetterSpacing(ctx.vg, -1);

		char text[128];
		char text1[128];

		float scaleMin = 100.0f;
		float scaleRange = 255.0f - scaleMin;

		int j = 0;

		nvgTextAlign(ctx.vg, NVG_ALIGN_LEFT);
		if (module->hasCVIn) {
			if (module->nCVChannels == module->nGateChannels) {
				nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
				snprintf(text, sizeof(text), "CV In: %d", module->nCVChannels);
			} else {
				if (module->hasGateIn) {
					nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0x6F));
					snprintf(text, sizeof(text), "CV In: %d (channel count mismatch)", module->nCVChannels);
				} else {
					nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
					snprintf(text, sizeof(text), "CV In: %d", module->nCVChannels);
				}
			}
		} else {
			nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0x6F));
			snprintf(text, sizeof(text), "No CV in");
		}
		nvgText(ctx.vg, box.pos.x + 5, box.pos.y + j * 16, text, NULL);
		j++;

		if (module->hasGateIn) {
			nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
			snprintf(text, sizeof(text), "Gate in: %d", module->nGateChannels);
		} else {
			nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0x6F));
			snprintf(text, sizeof(text), "No Gate in");
		}
		nvgText(ctx.vg, box.pos.x + 5, box.pos.y + j * 16, text, NULL);
		j = j + 2;

		for (int i = 0; i < 16; i++)  {
			if (i >= module->nCVChannels) {
				nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0x6F));
				snprintf(text, sizeof(text), "--");
				snprintf(text1, sizeof(text), "--");
				nvgText(ctx.vg, box.pos.x + 5, box.pos.y + i * 16 + j * 16, text, NULL);
				nvgText(ctx.vg, box.pos.x + 90, box.pos.y + i * 16 + j * 16, text1, NULL);
			} else {
				if (module->hasGateIn) {
					if (module->gate[i]) {
						float scale = (scaleRange * fabs(module->gateCV[i]) / 10.0) + scaleMin;
						nvgFillColor(ctx.vg, nvgRGBA(0x00, (int)scale, int(scale), 0xFF));
						snprintf(text,  sizeof(text), "%02d GATE", i);
						nvgText(ctx.vg, box.pos.x + 5, box.pos.y + i * 16 + j * 16, text, NULL);
						
						nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
						snprintf(text1, sizeof(text), "%f", module->cv[i]);
						nvgText(ctx.vg, box.pos.x + 90, box.pos.y + i * 16 + j * 16, text1, NULL);
					} else {
						nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0x6F));
						snprintf(text,  sizeof(text), "%02d NOGATE", i);
						snprintf(text1, sizeof(text), "%f", module->cv[i]);
						nvgText(ctx.vg, box.pos.x + 5, box.pos.y + i * 16 + j * 16, text, NULL);
						nvgText(ctx.vg, box.pos.x + 90, box.pos.y + i * 16 + j * 16, text1, NULL);
					}
				} else {
					nvgFillColor(ctx.vg, nvgRGBA(255, 165, 0, 0xff));
					nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0x6F));
					nvgText(ctx.vg, box.pos.x + 5, box.pos.y + i * 16 + j * 16, text, NULL);
					nvgText(ctx.vg, box.pos.x + 90, box.pos.y + i * 16 + j * 16, text1, NULL);
				}
			}
		}
	}
	
};

struct PolyProbeWidget : ModuleWidget {

	PolyProbeWidget(PolyProbe *module) {
		
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyProbe.svg")));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 6, true, true), module, PolyProbe::POLYCV_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 8, true, true), module, PolyProbe::POLYGATE_INPUT));

		if (module != NULL) {
			PolyProbeDisplay *displayW = createWidget<PolyProbeDisplay>(Vec(0, 20));
			displayW->box.size = Vec(240, 230);
			displayW->module = module;
			addChild(displayW);
		}

	}

};

Model *modelPolyProbe = createModel<PolyProbe, PolyProbeWidget>("PolyProbe");
