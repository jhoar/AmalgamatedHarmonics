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


struct AHButton : SVGSwitch, MomentarySwitch {
	AHButton() {
		addFrame(SVG::load(assetPlugin(plugin,"res/ComponentLibrary/AHButton.svg")));
	}	
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
	
	float Y_KNOB[2]    = {49.0, 56.0};
	float Y_PORT[2]    = {49.0, 56.0};
	float Y_BUTTON[2]  = {52.0, 56.0};
	float Y_LIGHT[2]   = {56.4, 56.0};
	float Y_TRIMPOT[2] = {56.4, 56.0};

	float Y_KNOB_COMPACT[2]     = {29.0, 35.0};
	float Y_PORT_COMPACT[2]     = {29.0, 35.0};
	float Y_BUTTON_COMPACT[2]   = {32.0, 35.0};
	float Y_LIGHT_COMPACT[2]    = {36.4, 35.0};
	float Y_TRIMPOT_COMPACT[2]  = {36.4, 35.0};
	
	float X_KNOB[2]     = {11.5, 48.0};
	float X_PORT[2]     = {11.5, 48.0};
	float X_BUTTON[2]   = {14.5, 48.0};
	float X_LIGHT[2]    = {18.9, 48.0};
	float X_TRIMPOT[2]  = {18.9, 48.0};

	float X_KNOB_COMPACT[2]     = {20.0, 35.0};
	float X_PORT_COMPACT[2]     = {19.0, 35.0};
	float X_BUTTON_COMPACT[2]   = {22.0, 35.0};
	float X_LIGHT_COMPACT[2]    = {26.4, 35.0};
	float X_TRIMPOT_COMPACT[2]  = {27.4, 35.0};
	

	Vec getPosition(int type, int xSlot, int ySlot, bool xDense, bool yDense);
	
	/* From the numerical key on a keyboard (0 = C, 11 = B), spacing in px between white keys and a starting x and Y coordinate for the C key (in px)
	* calculate the actual X and Y coordinate for a key, and the scale note to which that key belongs (see Midi note mapping)
	* http://www.grantmuller.com/MidiReference/doc/midiReference/ScaleReference.html */
	void calculateKeyboard(int inKey, float spacing, float xOff, float yOff, float *x, float *y, int *scale);
	
};


