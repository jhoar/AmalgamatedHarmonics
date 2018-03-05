#pragma once

#include <iostream>

#include "AH.hpp"
#include "componentlibrary.hpp"



struct ParamEvent  {

	enum Format {
		INT = 0,
		FP
	};
	
	ParamEvent(std::string n, Format s, float v) : name(n), spec(s), value(v) {}
	
	std::string name;
	Format spec;
	float value;

};

struct AHParam {
	
	std::string paramName = "NONAME";
	ParamEvent::Format spec = ParamEvent::INT;
	
	virtual ParamEvent generateEvent() = 0;
	
	template <typename T = AHParam>
	static void set(T *param, std::string n, ParamEvent::Format s) {
		param->paramName = n;
		param->spec = s;
	}

};

struct AHModule : Module {

	float delta;

	AHModule(int numParams, int numInputs, int numOutputs, int numLights = 0) : Module(numParams, numInputs, numOutputs, numLights) {
		delta = 1.0 / engineGetSampleRate();
	}

	void onSampleRateChange() override { 
		delta = 1.0 / engineGetSampleRate();
	}

	int stepX;
	
	bool debugFlag = false;
	
	inline bool debug() {
		return debugFlag;
	}
	
	virtual void receiveEvent(ParamEvent e) { }

};

struct AHButton : SVGSwitch, MomentarySwitch, AHParam {
	AHButton() {
		addFrame(SVG::load(assetPlugin(plugin,"res/ComponentLibrary/AHButton.svg")));
	}
	
	void onChange(EventChange &e) override { // buttons excluded
		MomentarySwitch::onChange(e);
	}

	ParamEvent generateEvent() override {
		return ParamEvent(paramName,spec,value);
	};
	
};

struct AHKnobSnap : RoundKnob, AHParam {
		
	AHKnobSnap() {
		snap = true;
		setSVG(SVG::load(assetPlugin(plugin,"res/ComponentLibrary/AHKnob.svg")));
	}

	void onChange(EventChange &e) override { 
		AHModule *m = static_cast<AHModule *>(this->module);
		m->receiveEvent(generateEvent());
		RoundKnob::onChange(e);
	}

	ParamEvent generateEvent() override {
		return ParamEvent(paramName,spec,value);
	};

};

struct AHKnobNoSnap : RoundKnob, AHParam {
	AHKnobNoSnap() {
		snap = false;
		setSVG(SVG::load(assetPlugin(plugin,"res/ComponentLibrary/AHKnob.svg")));
	}
	
	void onChange(EventChange &e) override { 
		AHModule *m = static_cast<AHModule *>(this->module);
		m->receiveEvent(generateEvent());
		RoundKnob::onChange(e);
	}

	ParamEvent generateEvent() override {
		return ParamEvent(paramName,spec,value);
	};
		
};


struct AHTrimpotSnap : RoundKnob, AHParam {
	AHTrimpotSnap() {
		snap = true;
		setSVG(SVG::load(assetPlugin(plugin,"res/ComponentLibrary/AHTrimpot.svg")));
	}
	
	void onChange(EventChange &e) override { 
		AHModule *m = static_cast<AHModule *>(this->module);
		m->receiveEvent(generateEvent());
		RoundKnob::onChange(e);
	}

	ParamEvent generateEvent() override {
		return ParamEvent(paramName,spec,value);
	};
	
};

struct AHTrimpotNoSnap : RoundKnob, AHParam {
	AHTrimpotNoSnap() {
		snap = false;
		setSVG(SVG::load(assetPlugin(plugin,"res/ComponentLibrary/AHTrimpot.svg")));
	}
	
	void onChange(EventChange &e) override { 
		AHModule *m = static_cast<AHModule *>(this->module);
		m->receiveEvent(generateEvent());
		RoundKnob::onChange(e);
	}

	ParamEvent generateEvent() override {
		return ParamEvent(paramName,spec,value);
	};
	
};


struct UI {

	enum UIElement {
		KNOB = 0,
		PORT,
		BUTTON,
		LIGHT,
		TRIMPOT
	};
	
	float Y_KNOB[6]    = {49.0, 105.0, 161.0, 217.0, 273.0, 329.0};
	float Y_PORT[6]    = {49.0, 105.0, 161.0, 217.0, 273.0, 329.0};
	float Y_BUTTON[6]  = {52.0, 108.0, 164.0, 220.0, 276.0, 332.0};
	float Y_LIGHT[6]   = {56.4, 112.4, 168.4, 224.4, 280.4, 336.4};
	float Y_TRIMPOT[6] = {56.4, 112.4, 168.4, 224.4, 280.4, 336.4};

	float Y_KNOB_COMPACT[9]     = {29.0, 64.0, 99.0, 134.0, 169.0, 204.0, 239.0, 274.0, 309.0};
	float Y_PORT_COMPACT[9]     = {29.0, 64.0, 99.0, 134.0, 169.0, 204.0, 239.0, 274.0, 309.0};
	float Y_BUTTON_COMPACT[9]   = {32.0, 67.0, 102.0, 137.0, 172.0, 207.0, 242.0, 277.0, 312.0};
	float Y_LIGHT_COMPACT[9]    = {36.4, 71.4, 106.4, 141.4, 176.4, 211.4, 246.4, 281.4, 316.4};
	float Y_TRIMPOT_COMPACT[9]  = {36.4, 71.4, 106.4, 141.4, 176.4, 211.4, 246.4, 281.4, 316.4};
	
	float X_KNOB[9]     = {11.5, 59.5, 107.5, 155.5, 203.5, 251.5, 299.5, 347.5, 395.5};
	float X_PORT[9]     = {11.5, 59.5, 107.5, 155.5, 203.5, 251.5, 299.5, 347.5, 395.5};
	float X_BUTTON[9]   = {14.5, 62.5, 110.5, 158.5, 206.5, 254.5, 302.5, 350.5, 398.5};
	float X_LIGHT[9]    = {18.9, 66.9, 114.9, 162.9, 210.9, 258.9, 306.9, 354.9, 402.9};
	float X_TRIMPOT[9]  = {18.9, 66.9, 114.9, 162.9, 210.9, 258.9, 306.9, 354.9, 402.9};

	float X_KNOB_COMPACT[12]     = {20.0, 55.0, 90.0, 125.0, 160.0, 195.0, 230.0, 265.0, 300.0, 335.0, 370.0, 405.0};
	float X_PORT_COMPACT[12]     = {20.0, 55.0, 90.0, 125.0, 160.0, 195.0, 230.0, 265.0, 300.0, 335.0, 370.0, 405.0};
	float X_BUTTON_COMPACT[12]   = {23.0, 58.0, 93.0, 128.0, 163.0, 198.0, 233.0, 268.0, 303.0, 338.0, 373.0, 408.0};
	float X_LIGHT_COMPACT[12]    = {27.4, 62.4, 97.4, 132.4, 167.4, 202.4, 237.4, 272.4, 307.4, 342.4, 377.4, 412.4};
	float X_TRIMPOT_COMPACT[12]  = {27.4, 62.4, 97.4, 132.4, 167.4, 202.4, 237.4, 272.4, 307.4, 342.4, 377.4, 412.4};
	

	Vec getPosition(int type, int xSlot, int ySlot, bool xDense, bool yDense);
	
	/* From the numerical key on a keyboard (0 = C, 11 = B), spacing in px between white keys and a starting x and Y coordinate for the C key (in px)
	* calculate the actual X and Y coordinate for a key, and the scale note to which that key belongs (see Midi note mapping)
	* http://www.grantmuller.com/MidiReference/doc/midiReference/ScaleReference.html */
	void calculateKeyboard(int inKey, float spacing, float xOff, float yOff, float *x, float *y, int *scale);
	
};


