#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct Algorithm {

	float a;
	float b;

	virtual void reset() {};

	virtual void addSample(float _a, float _b) {
		a = _a;
		b = _b;
	};

	virtual void calculate() = 0;;

	virtual float asValue() = 0;

	virtual std::string asString() = 0;

	virtual bool isValid() {
		return true;
	};

};

struct bAlgorithm : Algorithm {

	void calculate() {}

	float asValue() override {
		return b; 
	}

	std::string asString() override {
		return std::to_string(b);
	}

};

struct sumAlgorithm : Algorithm {

	float sum;

	void calculate() {
		sum = a + b;
	}

	float asValue() override {
		return sum; 
	}

	std::string asString() override {
		return std::to_string(sum);
	}

};

struct subAlgorithm : Algorithm {

	float diff;

	void calculate() {
		diff = a - b;
	}

	float asValue() override {
		return diff; 
	}

	std::string asString() override {
		return std::to_string(diff);
	}

};

struct noteAlgorithm : Algorithm {

	float v;
	int octave;
	int semitone;
	int cents;

	std::string note;
	float quantisedNote;
	bool valid;

	void calculate() override {

		v = a + b;

		if (v < -10.0 || v > 10.0) {
			valid = false;
			return;
		}

		double octV;
		double octFrac = modf(v, &octV); 
		octave = (int)octV + 4;

		// Below 0V. modf returns negative o and frac, so actual octave is one lower
		if (octFrac < 0.0) {
			octave--;
			octFrac = octFrac + 1.0;
		}

		double st;
		double stFrac = modf(octFrac / music::SEMITONE, &st);

		// semitone 0 - 11 
		// stFrac is positive fraction of semitone 0 - 1
		if (stFrac < 0.5) {
			cents = (int)round(stFrac * 100.0);
			semitone = (int)st;
		} else {
			cents = (int)round((stFrac - 1.0) * 100.0);
			semitone = (int)st + 1;
			if (semitone == 12) {
				octave++;
				semitone = 0;
			}
		}

		quantisedNote = octV + semitone * music::SEMITONE;
		valid = true;

	}

	float asValue() override {
		return quantisedNote; 
	}

	std::string asString() override {
		if (cents == 0) {
			return music::noteNames[semitone] + std::to_string(octave);
		} else if (cents < 0) {
			return music::noteNames[semitone] + std::to_string(octave) + std::to_string(cents);
		} else {
			return music::noteNames[semitone] + std::to_string(octave) + "+" + std::to_string(cents);
		}
	}

	bool isValid() {
		return valid;
	};

};

struct PolyProbe : core::AHModule {

	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		POLYCVA_INPUT,
		POLYCVB_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		POLYALGO_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	int algo = 0;

	Algorithm *algorithms[4][16];
	bAlgorithm bAlgo[16];
	sumAlgorithm sumAlgo[16];
	subAlgorithm subAlgo[16];
	noteAlgorithm noteAlgo[16];

	int nChannels = 0;
	int nCVAChannels = 0;
	int nCVBChannels = 0;

	bool hasCVAIn = false;
	bool hasCVBIn = false;
	float cvA[16];
	float cvB[16];

	PolyProbe() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {	
		for (int i = 0; i < 16; i++) {
			algorithms[0][i] = &bAlgo[i];
		}
		for (int i = 0; i < 16; i++) {
			algorithms[1][i] = &sumAlgo[i];
		}
		for (int i = 0; i < 16; i++) {
			algorithms[2][i] = &subAlgo[i];
		}
		for (int i = 0; i < 16; i++) {
			algorithms[3][i] = &noteAlgo[i];
		}
	}

	void process(const ProcessArgs &args) override {

		AHModule::step();

		if (inputs[POLYCVB_INPUT].isConnected()) {
			hasCVBIn = true;
			nCVBChannels = inputs[POLYCVB_INPUT].getChannels();
		} else {
			hasCVBIn = false;
			nCVBChannels = 0;
		} 

		for (int i = 0; i < 16; i++) {
			cvB[i] = inputs[POLYCVB_INPUT].getVoltage(i);
		}

		if (inputs[POLYCVA_INPUT].isConnected()) {
			hasCVAIn = true;
			nCVAChannels = inputs[POLYCVA_INPUT].getChannels();
		} else {
			hasCVAIn = false;
			nCVAChannels = 0;
		}

		for (int i = 0; i < 16; i++) {
			cvA[i] = inputs[POLYCVA_INPUT].getVoltage(i);
			algorithms[algo][i]->addSample(cvA[i], cvB[i]);
			outputs[POLYALGO_OUTPUT].setVoltage(algorithms[algo][i]->asValue(), i);
		}

		nChannels = std::max(nCVAChannels,nCVBChannels);

	}
};

struct PolyProbeDisplay : TransparentWidget {

	PolyProbe *module;
	std::shared_ptr<Font> font;
	int refresh = 0;

	PolyProbeDisplay() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/EurostileBold.ttf"));
	}

	void draw(const DrawArgs &ctx) override {

		nvgFontSize(ctx.vg, 14);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgTextLetterSpacing(ctx.vg, -1);

		char text[128];
		char text1[128];

		int j = 0;

		nvgTextAlign(ctx.vg, NVG_ALIGN_LEFT);
		if (module->hasCVAIn) {
			nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
			snprintf(text, sizeof(text), "CV A In: %d", module->nCVAChannels);
		} else {
			nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0x6F));
			snprintf(text, sizeof(text), "No CV A in");
		}
		nvgText(ctx.vg, box.pos.x + 5, box.pos.y + j * 16, text, NULL);
		j++;

		if (module->hasCVBIn) {
			nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
			snprintf(text, sizeof(text), "CV B in: %d", module->nCVBChannels);
		} else {
			nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0x6F));
			snprintf(text, sizeof(text), "No CV B in");
		}
		nvgText(ctx.vg, box.pos.x + 5, box.pos.y + j * 16, text, NULL);
		j = j + 2;

		for (int i = 0; i < 16; i++)  {
			if (i >= module->nCVAChannels) {
				nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0x6F));
				snprintf(text, sizeof(text), "%02d --", i + 1);
			} else {
				nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
				snprintf(text, sizeof(text), "%02d %f", i + 1, module->cvA[i]);
			}
			nvgText(ctx.vg, box.pos.x + 5, box.pos.y + i * 16 + j * 16, text, NULL);		

			module->algorithms[module->algo][i]->calculate();

			if (module->algorithms[module->algo][i]->isValid()) {
				nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
				snprintf(text1, sizeof(text1), "%s", module->algorithms[module->algo][i]->asString().c_str());
			} else {
				nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0x6F));
				snprintf(text1, sizeof(text1), "--");
			}
			nvgText(ctx.vg, box.pos.x + 110, box.pos.y + i * 16 + j * 16, text1, NULL);
		}
	}

};

struct PolyProbeWidget : ModuleWidget {

	PolyProbeWidget(PolyProbe *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyProbe.svg")));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 4, true, true), module, PolyProbe::POLYCVA_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 6, true, true), module, PolyProbe::POLYCVB_INPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 5, 8, true, true), module, PolyProbe::POLYALGO_OUTPUT));

		if (module != NULL) {
			PolyProbeDisplay *displayW = createWidget<PolyProbeDisplay>(Vec(0, 20));
			displayW->box.size = Vec(240, 230);
			displayW->module = module;
			addChild(displayW);
		}

	}

	void appendContextMenu(Menu *menu) override {

		PolyProbe *probe = dynamic_cast<PolyProbe*>(module);
		assert(probe);

		struct AlgoItem : MenuItem {
			PolyProbe *module;
			int algo;
			void onAction(const rack::event::Action &e) override {
				module->algo = algo;
			}
		};

		struct AlgoMenu : MenuItem {
			PolyProbe *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<int> algo = {0, 1, 2, 3};
				std::vector<std::string> names = {"B", "A+B", "A-B", "Note(A+B)"};
				for (size_t i = 0; i < algo.size(); i++) {
					AlgoItem *item = createMenuItem<AlgoItem>(names[i], CHECKMARK(module->algo == algo[i]));
					item->module = module;
					item->algo = algo[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());
		AlgoMenu *algoItem = createMenuItem<AlgoMenu>("Algorithm");
		algoItem->module = probe;
		menu->addChild(algoItem);

	}

};

Model *modelPolyProbe = createModel<PolyProbe, PolyProbeWidget>("PolyProbe");
