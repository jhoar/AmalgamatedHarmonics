#include <array>
#include <string.h>
#include <osdialog.h>

#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

static const int BUFFER_SIZE = 512;

using namespace ah;

typedef std::array<NVGcolor, 16> colourMap;
std::array<colourMap,6> cMaps;

/** 
 * PolyScope, based on Andrew Belt's Scope module.
 */
struct PolyScope : core::AHModule {
	enum ParamIds {
		SCALE_PARAM,
		SPREAD_PARAM,
		TIME_PARAM,
		SHIFT_PARAM,
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
	int maxChannels;

	bool toggle = false;

	int currCMap = 1;
	std::string path;
	std::string directory;

	dsp::SchmittTrigger resetTrigger;

	void loadCMap(const char *path) {

		// Empty path, so bail 
		if (path[0] == '\0') {
			return;
		}

		FILE *file = fopen(path, "r");
		if (!file) {
			WARN("Could not load colour scheme file %s", path);
			return;
		}
		DEFER({
			fclose(file);
		});

		json_error_t error;
		json_t *rootJ = json_loadf(file, 0, &error);
		if (!rootJ) {
			std::string message = string::f("File is not a valid colour scheme file. JSON parsing error at %s %d:%d %s", error.source, error.line, error.column, error.text);
#ifdef USING_CARDINAL_NOT_RACK
			async_dialog_message(message.c_str());
#else
			osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, message.c_str());
#endif
			return;
		}
		DEFER({
			json_decref(rootJ);
		});

		this->path = path;

		for (int i = 0; i < 16; i++) {

			std::string nodeName = "userCmap" + std::to_string(i);

			json_t *cmap_array = json_object_get(rootJ, nodeName.c_str());
			if (cmap_array) {

				int r = 255;
				int g = 0;
				int b = 0;

				json_t *rJ = json_array_get(cmap_array, 0);
				if (rJ)	r = json_integer_value(rJ);

				json_t *gJ = json_array_get(cmap_array, 1);
				if (gJ)	g = json_integer_value(gJ);

				json_t *bJ = json_array_get(cmap_array, 2);
				if (bJ)	b = json_integer_value(bJ);

				cMaps[5][i] = nvgRGBA(r, g, b, 240);

			}
		}

		currCMap = 5;

	}

	PolyScope() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) { 
		configParam(SCALE_PARAM, -2.0f, 2.0f, 0.0f);
		configParam(SPREAD_PARAM, 0.0f, 3.0f, 1.0f);
		configParam(TIME_PARAM, 6.0f, 16.0f, 14.0f);
		configParam(SHIFT_PARAM, -16.0f, 16.0f, 0.0f);

		cMaps[0] = { // Classic
		nvgRGBA(255,	0,		0,		240),	// 0	100		100
		nvgRGBA(223,	0,		32,		240),	// 351	100		87
		nvgRGBA(191,	0,		64,		240),	// 339	100		74
		nvgRGBA(159,	0,		96,		240),	// 323	100		62
		nvgRGBA(128,	0,		128,	240),	// 300	100		50
		nvgRGBA(96,		0,		159,	240),	// 276	100		62
		nvgRGBA(64,		0,		191,	240),	// 260	100		74
		nvgRGBA(32,		0,		223,	240),	// 248	100		91
		nvgRGBA(0,		32,		223,	240),	// 231	120		91
		nvgRGBA(0,		64,		191,	240),	// 219	100		74
		nvgRGBA(0,		96,		159,	240),	// 203	100		62
		nvgRGBA(0,		128,	128,	240),	// 180	100		50
		nvgRGBA(0,		159,	96,		240),	// 156	100		62
		nvgRGBA(0,		191,	64,		240),	// 140	100		74
		nvgRGBA(0,		223,	32,		240),	// 128	100		87
		nvgRGBA(0,		255,	0,		240)};	// 120	100		100

		cMaps[1] = { // Constant V
		nvgRGBA(255,	0,		0,		240),	// 0	100		100
		nvgRGBA(255,	0,		38,		240),	// 351	100		87
		nvgRGBA(255,	0,		89,		240),	// 339	100		74
		nvgRGBA(255,	0,		157,	240),	// 323	100		62
		nvgRGBA(255,	0,		255,	240),	// 300	100		50
		nvgRGBA(152,	0,		255,	240),	// 276	100		62
		nvgRGBA(84,		0,		255,	240),	// 260	100		74
		nvgRGBA(34,		0,		255,	240),	// 248	100		91
		nvgRGBA(0,		38,		255,	240),	// 231	100		91
		nvgRGBA(0,		89,		255,	240),	// 219	100		74
		nvgRGBA(0,		157,	159,	240),	// 203	100		62
		nvgRGBA(0,		255,	255,	240),	// 180	100		50
		nvgRGBA(0,		255,	153,	240),	// 156	100		62
		nvgRGBA(0,		255,	85,		240),	// 140	100		74
		nvgRGBA(0,		255,	33,		240),	// 128	100		87
		nvgRGBA(0,		255,	0,		240)};	// 120	100		100

		float dHue = 1.0f/16.0f;

		for (int i = 0; i < 16; i++) {
			cMaps[2][i] = nvgHSL(1 - i * dHue * 2.0f/3.0f, 1.0f, 0.7f ); // Constant L, HSL L=0.7
		}

		for (int i = 0; i < 16; i++) {
			cMaps[3][i] = nvgHSL(1 - i * dHue, 1.0f, 0.7f ); // Full Circle, HSL L=0.7
		}

		for (int i = 0; i < 16; i++) {
			cMaps[4][i] = nvgHSL(2.0f/3.0f + i * dHue * 1.0f/6.0f, 1.0f, 0.6f ); // Synthwave L=0.5
		}

		for (int i = 0; i < 16; i++) {
			cMaps[5][i] = nvgRGBf(1.0f, 1.0f, 1.0f); // User defined, start with all white
		}

	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		json_object_set_new(rootJ, "cmap", json_integer((int) currCMap));
		json_object_set_new(rootJ, "path", json_string(path.c_str()));

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		// cmap
		json_t *cMapJ = json_object_get(rootJ, "cmap");
		if (cMapJ) currCMap = json_integer_value(cMapJ);

		json_t *pathJ = json_object_get(rootJ, "path");
		if (pathJ) loadCMap(json_string_value(pathJ));

	}

	void onReset() override {
		currCMap = 1;
		path = "";
	}

	void process(const ProcessArgs &args) override {

		// Compute time
		float deltaTime = std::pow(2.0f, -params[TIME_PARAM].getValue());
		int frameCount = static_cast<int>(std::ceil(deltaTime * args.sampleRate));

		maxChannels = inputs[POLY_INPUT].getChannels();

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
				bufferIndex = 0; 
				frameIndex = 0; 
				return;
			}

			// Reset if we've waited too long
			if (frameIndex >= args.sampleRate * holdTime) {
				bufferIndex = 0; 
				frameIndex = 0; 
				return;
			}
		}
	}
};

struct Patch : Widget {

	PolyScope *module = NULL;

	void onButton(const event::Button &e) override {
		Widget::onButton(e);
		if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
			if (module) {
				module->toggle = !(module->toggle);
			}
		} 
	}

};

struct PolyScopeDisplay : TransparentWidget {
	PolyScope *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	float t = 0.0;
	float d = 0.008;

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

		if(module->toggle) {
			t = t + d;
			if ((t >= 1.0) || (t <= 0.0)) {
				d = -d;
			}
		}

		float gain = std::pow(2.0f, module->params[PolyScope::SCALE_PARAM].getValue());
		float shift = module->params[PolyScope::SHIFT_PARAM].getValue();
		float offset = module->toggle ? math::clamp(t, 0.0, 1.0) : module->params[PolyScope::SPREAD_PARAM].getValue();

		float values[16][BUFFER_SIZE];
		for (int i = 0; i < BUFFER_SIZE; i++) {
			for (int k = 0; k < 16; k++) {
				values[k][i] = ((module->buffer[k][i] + ((k - 8) * offset)) + shift) * gain / 10.0f;
			}
		}

		for (int i = 0; i < module->maxChannels; i++) {
			nvgStrokeColor(args.vg, cMaps[module->currCMap][i]);
			drawWaveform(args, values[i]);
		}
	}
};

static void cmapPathSelected(PolyScope *module, char* path) {
	if (path) {
		module->loadCMap(path);
		free(path);
	}
}

static void loadCMap(PolyScope *module) {

	std::string dir;
	std::string filename;
	if (module->path != "") {
		dir = system::getDirectory(module->path);
		filename = system::getFilename(module->path);
	}
	else {
		dir = asset::user("");
		filename = "colourmap.json";
	}

#ifdef USING_CARDINAL_NOT_RACK
	async_dialog_filebrowser(false, dir.c_str(), "Load colour scheme", [module](char* path) {
		cmapPathSelected(module, path);
	});
#else
	char *path = osdialog_file(OSDIALOG_OPEN, dir.c_str(), filename.c_str(), NULL);
	cmapPathSelected(module, path);
#endif
}

struct PolyScopeWidget : ModuleWidget {

	std::vector<MenuOption<int>> cmapOptions;

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

		{
			Patch *patch = new Patch();
			patch->module = module;
			patch->box.pos = Vec(155, 355);
			patch->box.size = Vec(30, 20);
			addChild(patch);
		}

		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(233.448, 340.079), module, PolyScope::TIME_PARAM));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(111.552, 340.162), module, PolyScope::SCALE_PARAM));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(152.184, 340.162), module, PolyScope::SPREAD_PARAM));
		addParam(createParamCentered<gui::AHKnobNoSnap>(Vec(192.816, 340.162), module, PolyScope::SHIFT_PARAM));

		addInput(createInputCentered<gui::AHPort>(Vec(37.414, 340.658), module, PolyScope::POLY_INPUT));

		cmapOptions.emplace_back("Classic", 0);
		cmapOptions.emplace_back("Constant V", 1);
		cmapOptions.emplace_back("Constant L", 2);
		cmapOptions.emplace_back("Full Circle", 3);
		cmapOptions.emplace_back("Synthwave", 4);
		cmapOptions.emplace_back("User", 5);

	}

	void appendContextMenu(Menu *menu) override {

		PolyScope *scope = dynamic_cast<PolyScope*>(module);
		assert(scope);

		struct PolyScopeMenu : MenuItem {
			PolyScope *module;
			PolyScopeWidget *parent;
		};

		struct PathItem : PolyScopeMenu {
			void onAction(const event::Action &e) override {
				loadCMap(module);
			}
		};

		struct ColourItem : PolyScopeMenu {
			int cMap;
			void onAction(const rack::event::Action &e) override {
				module->currCMap = cMap;
			}
		};

		struct ColourMenu : PolyScopeMenu {
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				for (auto opt: parent->cmapOptions) {
					ColourItem *item = createMenuItem<ColourItem>(opt.name, CHECKMARK(module->currCMap == opt.value));
					item->module = module;
					item->cMap = opt.value;
					menu->addChild(item);
				}
				return menu;
			}
		};

		ColourMenu *cMapItem = createMenuItem<ColourMenu>("Colour Schemes");
		cMapItem->module = scope;
		cMapItem->parent = this;
		menu->addChild(cMapItem);

		PathItem *pathItem = new PathItem;
		pathItem->text = "Load colour scheme";
		pathItem->module = scope;
		menu->addChild(pathItem);

	 }

};

Model *modelPolyScope = createModel<PolyScope, PolyScopeWidget>("PolyScope");
