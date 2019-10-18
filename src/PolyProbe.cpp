#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct Operator {

	float a;
	float b;
	float outV;
	bool valid = true;

	virtual float asValue() {
		return outV; 
	}

	virtual std::string asString() {
		return std::to_string(outV);
	}

	virtual bool isValid() {
		return valid;
	}

	virtual void addSample(float _a, float _b) {
		a = _a;
		b = _b;
	}

	virtual void reset() {};

	virtual void calculate() = 0;

	virtual ~Operator() = default;

};

struct AddOperator : Operator {

	void calculate() override {
		outV = a + b;
		valid = true;
	}

};

struct SubOperator : Operator {

	void calculate() override {
		outV = a - b;	
		valid = true;
	}

};

struct NoteOperator : Operator {

	int octave = 0;
	int semitone = 0;
	int cents = 0;

	std::string asString() override {
		if (cents == 0) {
			return music::noteNames[semitone] + std::to_string(octave);
		} else if (cents < 0) {
			return music::noteNames[semitone] + std::to_string(octave) + std::to_string(cents);
		} else {
			return music::noteNames[semitone] + std::to_string(octave) + "+" + std::to_string(cents);
		}
	}

	void calculate() override {

		float v = a + b;

		if (v < -10.0 || v > 10.0) {
			valid = false;
			octave = 0;
			semitone = 0;
			cents = 0;
			return;
		}

		double octV;
		double octFrac = modf(v, &octV); 
		octave = (int)octV + 4; // 0V is 4th Octave

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

		outV = (octave - 4) + semitone * music::SEMITONE;

		valid = true;

	}

};

struct PolyProbe : core::AHModule {

	enum Algorithms {
		SUM,
		DIFF,
		NOTE
	};

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

	Operator * oper[3][16];
	Algorithms currAlgo = SUM;

	int nChannels = 0;
	int nCVAChannels = 0;
	int nCVBChannels = 0;
	int frame = 0;

	bool hasCVAIn = false;
	bool hasCVBIn = false;
	float cvA[16];
	float cvB[16];

	PolyProbe() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		for (int i = 0; i < 16; i++) {
			oper[0][i] = new AddOperator;
			oper[1][i] = new SubOperator;
			oper[2][i] = new NoteOperator;
		}
	}

	~PolyProbe() {
		for (int i = 0; i < 16; i++) {
			delete oper[0][i];
			delete oper[1][i];
			delete oper[2][i];
		}
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// algo
		json_t *algoJ = json_integer((int) currAlgo);
		json_object_set_new(rootJ, "algo", algoJ);

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {

		// algo
		json_t *algoJ = json_object_get(rootJ, "algo");
		if (algoJ) currAlgo = (Algorithms)json_integer_value(algoJ);

	}

	void process(const ProcessArgs &args) override {

		AHModule::step();

		if (inputs[POLYCVA_INPUT].isConnected()) {
			hasCVAIn = true;
			nCVAChannels = inputs[POLYCVA_INPUT].getChannels();
		} else {
			hasCVAIn = false;
			nCVAChannels = 0;
		}

		if (inputs[POLYCVB_INPUT].isConnected()) {
			hasCVBIn = true;
			nCVBChannels = inputs[POLYCVB_INPUT].getChannels();
		} else {
			hasCVBIn = false;
			nCVBChannels = 0;
		} 

		nChannels = std::max(nCVAChannels,nCVBChannels);

		outputs[POLYALGO_OUTPUT].setChannels(nChannels);
		for (int i = 0; i < nChannels; i++) {
			cvA[i] = inputs[POLYCVA_INPUT].getVoltage(i);
			cvB[i] = inputs[POLYCVB_INPUT].getVoltage(i);

			oper[currAlgo][i]->addSample(cvA[i], cvB[i]);
			oper[currAlgo][i]->calculate();

			outputs[POLYALGO_OUTPUT].setVoltage(oper[currAlgo][i]->asValue(), i);
		}

		for (int i = nChannels; i < 16; i++) {
			oper[currAlgo][i]->valid = false;
		}

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

			if (i >= module->nCVBChannels) {
				nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0x6F));
				snprintf(text, sizeof(text), "%02d --", i + 1);
			} else {
				nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
				snprintf(text, sizeof(text), "%02d %f", i + 1, module->cvB[i]);
			}
			nvgText(ctx.vg, box.pos.x + 110, box.pos.y + i * 16 + j * 16, text, NULL);		

			if (module->oper[module->currAlgo][i]->isValid()) {
				nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
				snprintf(text, sizeof(text), "%s", module->oper[module->currAlgo][i]->asString().c_str());
			} else {
				nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0x6F));
				snprintf(text, sizeof(text), "--");
			}
			nvgText(ctx.vg, box.pos.x + 215, box.pos.y + i * 16 + j * 16, text, NULL);
		}
	}

};

struct PolyProbeWidget : ModuleWidget {

	std::vector<MenuOption<PolyProbe::Algorithms>> algoOptions;

	PolyProbeWidget(PolyProbe *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyProbe.svg")));

		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 8, 4, true, true), module, PolyProbe::POLYCVA_INPUT));
		addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 8, 6, true, true), module, PolyProbe::POLYCVB_INPUT));
		addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 8, 8, true, true), module, PolyProbe::POLYALGO_OUTPUT));

		if (module != NULL) {
			PolyProbeDisplay *displayW = createWidget<PolyProbeDisplay>(Vec(0, 20));
			displayW->box.size = Vec(240, 230);
			displayW->module = module;
			addChild(displayW);
		}

		algoOptions.emplace_back(std::string("A+B"), PolyProbe::Algorithms::SUM);
		algoOptions.emplace_back(std::string("A-B"), PolyProbe::Algorithms::DIFF);
		algoOptions.emplace_back(std::string("Note(A+B)"), PolyProbe::Algorithms::NOTE);

	}

	void appendContextMenu(Menu *menu) override {

		PolyProbe *probe = dynamic_cast<PolyProbe*>(module);
		assert(probe);

		struct PolyProbeMenu : MenuItem {
			PolyProbe *module;
			PolyProbeWidget *parent;
		};

		struct AlgoItem : PolyProbeMenu {
			PolyProbe::Algorithms algo;
			void onAction(const rack::event::Action &e) override {
				module->currAlgo = algo;
			}
		};

		struct AlgoMenu : PolyProbeMenu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->algoOptions) {
					AlgoItem *item = createMenuItem<AlgoItem>(opt.name, CHECKMARK(module->currAlgo == opt.value));
					item->module = module;
					item->algo = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());
		AlgoMenu *algoItem = createMenuItem<AlgoMenu>("Operation");
		algoItem->module = probe;
		algoItem->parent = this;
		menu->addChild(algoItem);

	}

};

Model *modelPolyProbe = createModel<PolyProbe, PolyProbeWidget>("PolyProbe");
