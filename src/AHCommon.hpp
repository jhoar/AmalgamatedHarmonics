#pragma once

#include <iostream>

#include "AH.hpp"
#include "componentlibrary.hpp"

namespace ah {

namespace core {

const double PI = 3.14159265358979323846264338327950288;

struct ParamEvent {

	ParamEvent(int t, int i, float v) : pType(t), pId(i), value(v) {}

	int pType;
	int pId;
	float value;

};

struct AHModule : rack::Module {

	AHModule(int numParams, int numInputs, int numOutputs, int numLights = 0) {
		config(numParams, numInputs, numOutputs, numLights);
	}

	int stepX = 0;

	bool debugFlag = false;

	inline bool debugEnabled() {
		return debugFlag;
	}

	inline bool debugEnabled(int poll) {
		if (debugFlag && stepX % poll == 0) {
			return true;
		} else {
			return false;
		}
	}

	bool receiveEvents = false;
	int keepStateDisplay = 0;
	std::string paramState = ">";

	virtual void receiveEvent(ParamEvent e) {
		paramState = ">";
		keepStateDisplay = 0;
	}

	void step() override {

		stepX++;

		// Once we start stepping, we can process events
		receiveEvents = true;
		// Timeout for display
		keepStateDisplay++;
		if (keepStateDisplay > 50000) {
			paramState = ">"; 
		}

	}

};

} // namespace core

namespace gui {

struct AHChoice : LedDisplayChoice {
	AHChoice();

	float fontSize;

	virtual void draw(const DrawArgs& args) override;
};

struct StateDisplay : TransparentWidget {

	core::AHModule *module;
	std::shared_ptr<Font> font;

	StateDisplay() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/EurostileBold.ttf"));
	}

	virtual void draw(const DrawArgs& args) override {

		Vec pos = Vec(0, 15);

		nvgFontSize(args.vg, 16);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, -1);

		nvgFillColor(args.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));

		char text[128];
		snprintf(text, sizeof(text), "%s", module->paramState.c_str());
		nvgText(args.vg, pos.x + 10, pos.y + 5, text, NULL);

	}

};

struct AHParamWidget { // it's a mix-in

	int pType = -1; // Should be set by step<>(), but if not this allows us to catch pwidgets we are not interested in
	int pId;
	core::AHModule *mod = NULL;

	virtual core::ParamEvent generateEvent(float value) {
		return core::ParamEvent(pType,pId,value);
	};

	template <typename T = AHParamWidget>
	static void set(T *param, int pType, int pId) {
		param->pType = pType;
		param->pId = pId;
	}

};

// Not going to monitor buttons
struct AHButton : SvgSwitch {
	AHButton() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ComponentLibrary/AHButton.svg")));
	}	
};

struct AHKnob : RoundKnob, AHParamWidget {
	void onChange(const rack::event::Change & e) override { 
		// One off cast, don't want to subclass from ParamWidget, so have to grab it here
		if (!AHParamWidget::mod) {
			AHParamWidget::mod = static_cast<core::AHModule *>(paramQuantity->module);
		}
		AHParamWidget::mod->receiveEvent(generateEvent(paramQuantity->getValue()));
		RoundKnob::onChange(e);
	}
};

struct AHKnobSnap : AHKnob {
	AHKnobSnap() {
		snap = true;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ComponentLibrary/AHKnob.svg")));
	}
};

struct AHKnobNoSnap : AHKnob {
	AHKnobNoSnap() {
		snap = false;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ComponentLibrary/AHKnob.svg")));
	}
};

struct AHBigKnobNoSnap : AHKnob {
	AHBigKnobNoSnap() {
		snap = false;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ComponentLibrary/AHBigKnob.svg")));
	}
};

struct AHBigKnobSnap : AHKnob {
	AHBigKnobSnap() {
		snap = true;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ComponentLibrary/AHBigKnob.svg")));
	}
};

struct AHTrimpotSnap : AHKnob {
	AHTrimpotSnap() {
		snap = true;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ComponentLibrary/AHTrimpot.svg")));
	}
};

struct AHTrimpotNoSnap : AHKnob {
	AHTrimpotNoSnap() {
		snap = false;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ComponentLibrary/AHTrimpot.svg")));
	}
};

struct KeyParamQuantity : engine::ParamQuantity {
	std::string getDisplayValueString() override;
};

struct ModeParamQuantity : engine::ParamQuantity {
	std::string getDisplayValueString() override;
};

enum UIElement {
	KNOB = 0,
	PORT,
	BUTTON,
	LIGHT,
	TRIMPOT
};

extern float Y_KNOB[2];
extern float Y_PORT[2];
extern float Y_BUTTON[2]; 
extern float Y_LIGHT[2];
extern float Y_TRIMPOT[2];

extern float Y_KNOB_COMPACT[2];
extern float Y_PORT_COMPACT[2];
extern float Y_BUTTON_COMPACT[2];
extern float Y_LIGHT_COMPACT[2];
extern float Y_TRIMPOT_COMPACT[2];

extern float X_KNOB[2];
extern float X_PORT[2];
extern float X_BUTTON[2];
extern float X_LIGHT[2];
extern float X_TRIMPOT[2];

extern float X_KNOB_COMPACT[2];
extern float X_PORT_COMPACT[2];
extern float X_BUTTON_COMPACT[2];
extern float X_LIGHT_COMPACT[2];
extern float X_TRIMPOT_COMPACT[2];

// Template this
Vec getPosition(int type, int xSlot, int ySlot, bool xDense, bool yDense, float xDelta = 0.0f, float yDelta = 0.0f);

/* From the numerical key on a keyboard (0 = C, 11 = B), spacing in px between white keys and a starting x and Y coordinate for the C key (in px)
* calculate the actual X and Y coordinate for a key, and the scale note to which that key belongs (see Midi note mapping)
* http://www.grantmuller.com/MidiReference/doc/midiReference/ScaleReference.html */
void calculateKeyboard(int inKey, float spacing, float xOff, float yOff, float *x, float *y, int *scale);

} // namespace gui

namespace digital {

static constexpr float TRIGGER = 1e-3f;

struct AHPulseGenerator {
	float time = 0.f;
	float pulseTime = 0.f;

	bool ishigh() {
			return time < pulseTime;
	}

	bool process(float deltaTime) {
		time += deltaTime;
		return time < pulseTime;
	}

	bool trigger(float pulseTime) {
		// Keep the previous pulseTime if the existing pulse would be held longer than the currently requested one.
		if (time + pulseTime >= this->pulseTime) {
			time = 0.f;
			this->pulseTime = pulseTime;
			return true;
		} else {
			return false;
		}
	}
};

struct BpmCalculator {

	float timer = 0.0f;
	int misses = 0;
	float seconds = 0;
	rack::dsp::SchmittTrigger gateTrigger;

	inline bool checkBeat(int mult) {
		return ( ((timer - mult * seconds) * (timer - mult * seconds) / (seconds * seconds) < 0.2f ) && misses < 4);
	}

	float calculateBPM(float delta, float input) {

		if (gateTrigger.process(input) ) {

			if (timer > 0) {

				float new_seconds;
				bool found = false;

				for(int mult = 1; !found && mult < 20; mult++ )  {

					if (checkBeat(mult)) {
						new_seconds = timer / mult;
						if (mult == 1) {
							misses = 0;
						} else {
							misses++;
						}
					
						found = true;
					};
				
				};

				if (!found) {
					// std::cerr << "default. misses = " << misses << "\n";
					new_seconds = timer;
					misses = 0;
				}

				float a = 0.5f; 
				seconds = ((1.0f - a) * seconds + a * new_seconds);
				timer -= seconds;

			}

		};

		timer += delta;
		if (seconds < 2.0e-05) {
			return 0.0f;
		} else {
			return 60.0f / seconds;
		}
	};

};

} // namespace digital

namespace music {

const static int NUM_CHORDS = 99; // FIXME Remove this

static constexpr float SEMITONE = 1.0 / 12.0;

struct Chord {
	int rootNote;
	int quality;
	int chord;
	int modeDegree;
	int inversion;
	int octave;

	float outVolts[6];

	Chord();
	void reset() {
		rootNote = 0;
		quality = 0;
		chord = 0;
		modeDegree = 0;
		inversion = 0;
		octave = 0;
	}

	void setVoltages(const std::vector<int> &chordArray, int offset);

};

struct ChordDef {
	int number;
	std::string name;
	int	root[6];
	int	first[6];
	int	second[6];
};

extern ChordDef ChordTable[NUM_CHORDS];

struct ChordFormula {
	std::string name;
	std::vector<int> root;
};

extern std::vector<ChordFormula> BasicChordSet;

enum Notes {
	NOTE_C = 0,
	NOTE_D_FLAT, // C Sharp
	NOTE_D,
	NOTE_E_FLAT, // D Sharp
	NOTE_E,
	NOTE_F,
	NOTE_G_FLAT, //F Sharp
	NOTE_G,
	NOTE_A_FLAT, // G Sharp
	NOTE_A,
	NOTE_B_FLAT, // A Sharp
	NOTE_B,
	NUM_NOTES
};

enum Scales {
	SCALE_CHROMATIC = 0,
	SCALE_IONIAN,
	SCALE_DORIAN,
	SCALE_PHRYGIAN,
	SCALE_LYDIAN,
	SCALE_MIXOLYDIAN,
	SCALE_AEOLIAN,
	SCALE_LOCRIAN,
	SCALE_MAJOR_PENTA,
	SCALE_MINOR_PENTA,
	SCALE_HARMONIC_MINOR,
	SCALE_BLUES,
	NUM_SCALES
};

enum Modes {
	MODE_IONIAN = 0,
	MODE_DORIAN,
	MODE_PHRYGIAN,
	MODE_LYDIAN,
	MODE_MIXOLYDIAN,
	MODE_AEOLIAN,
	MODE_LOCRIAN,
	NUM_MODES
};

enum Degrees {
	DEGREE_I = 0,
	DEGREE_II,
	DEGREE_III,
	DEGREE_IV,
	DEGREE_V,
	DEGREE_VI,
	DEGREE_VII,
	NUM_DEGREES
};

enum Inversion {
	ROOT,
	FIRST_INV,
	SECOND_INV,
	NUM_INV
};

enum Quality {
	MAJ = 0,
	MIN,
	DIM,
	NUM_QUALITY
};

enum RootScaling {
	CIRCLE,
	VOCT
};

/*
* Convert a V/OCT voltage to a quantized pitch, key and scale, and calculate various information about the quantised note.
*/

float getPitchFromVolts(float inVolts, int inRoot, int inScale, int *outNote = NULL, int *outDegree = NULL);

float getPitchFromVolts(float inVolts, float inRoot, float inScale, int *outRoot, int *outScale, int *outNote, int *outDegree);

/*
* Convert a root note (relative to C, C=0) and positive semi-tone offset from that root to a voltage (1V/OCT, 0V = C4 (or 3??))
*/
float getVoltsFromPitch(int inRoot, int inNote);

float getVoltsFromScale(int scale);

int getScaleFromVolts(float volts);

int getModeFromVolts(float volts);

float getVoltsFromMode(int mode);

float getVoltsFromKey(int key);

int getKeyFromVolts(float volts);

void getRootFromMode(int inMode, int inRoot, int inTonic, int *currRoot, int *quality);

extern int ModeQuality[7][7];

extern int ModeOffset[7][7];

extern std::string DegreeString[7][7];

// NOTE_C = 0,
// NOTE_D_FLAT, // C Sharp
// NOTE_D,
// NOTE_E_FLAT, // D Sharp
// NOTE_E,
// NOTE_F,
// NOTE_G_FLAT, //F Sharp
// NOTE_G,
// NOTE_A_FLAT, // G Sharp
// NOTE_A,
// NOTE_B_FLAT, // A Sharp
// NOTE_B,

//0	1	2	3	4	5	6	7	8	9	10	11	12
extern int tonicIndex[13];
extern int scaleIndex[7];
extern int noteIndex[13];
			
// Reference, midi note to scale
// 0	1
// 1	b2 (#1)
// 2	2
// 3	b3 (#2)
// 4	3
// 5	4
// 6	b5 (#4)
// 7	5
// 8	b6 (#5)
// 9 	6
// 10	b7 (#6)
// 11	7

// http://www.grantmuller.com/MidiReference/doc/midiReference/ScaleReference.html
// Although their definition of the Blues scale is wrong
// Added the octave note to ensure that the last note is correctly processed
extern int ASCALE_CHROMATIC			[13];
extern int ASCALE_IONIAN			[8];
extern int ASCALE_DORIAN			[8];
extern int ASCALE_PHRYGIAN			[8];
extern int ASCALE_LYDIAN			[8];
extern int ASCALE_MIXOLYDIAN		[8];
extern int ASCALE_AEOLIAN			[8];
extern int ASCALE_LOCRIAN			[8];
extern int ASCALE_MAJOR_PENTA		[6];
extern int ASCALE_MINOR_PENTA		[6];
extern int ASCALE_HARMONIC_MINOR	[8];
extern int ASCALE_BLUES				[7];

extern int CIRCLE_FIFTHS [12];

extern std::string noteNames[12];

extern std::string scaleNames[12];

extern std::string intervalNames[13];

extern std::string modeNames[7];

extern std::string inversionNames[3];

extern std::string qualityNames[3];

extern std::string NoteDegreeModeNames[12][7][7];

struct InversionDefinition {
	int inversion;
	std::vector<int> formula;
	std::string baseName;

	std::string getName(int rootNote) const;
	std::string getName(int mode, int key, int degree, int rootNote) const;
};

struct ChordDefinition {
	int id;
	std::string name;
	std::vector<int> formula;
	std::vector<InversionDefinition> inversions;

	void generateInversions();
	void calculateInversion(std::vector<int> &inputF, std::vector<int> &outputF, int inv, int rootOffset);
};

struct KnownChords {
	std::vector<ChordDefinition> chords;

	KnownChords();
	void dump();
	const InversionDefinition &getChord(Chord chord);
};

extern InversionDefinition defaultChord;

} // namespace music

} // namespace ah

template <class T> class MenuOption {
	public: 
		std::string name;
		T value;
		MenuOption(std::string _name, T _value) : name(_name), value(_value) {}
};

struct ImperfectSetting {
	float dlyLen;
	float dlySpr;
	float gateLen;
	float gateSpr;

	int division;
	float prob;
};

struct ImperfectState {
	bool delayState;
	bool gateState;
	float delayTime;
	float gateTime;
	ah::digital::AHPulseGenerator delayPhase;
	ah::digital::AHPulseGenerator gatePhase;
	float bpm;

	void reset() {
		delayState = false;
		gateState = false;
		delayTime = 0.0;
		gateTime = 0.0;
		bpm = 0.0;
	}

	void jitter(ImperfectSetting &setting) {
		// Determine delay and gate times for all active outputs
		double rndD = clamp(random::normal(), -2.0f, 2.0f);
		delayTime = clamp(setting.dlyLen + setting.dlySpr * rndD, 0.0f, 100.0f);

		// The modified gate time cannot be earlier than the start of the delay
		double rndG = clamp(random::normal(), -2.0f, 2.0f);
		gateTime = clamp(setting.gateLen + setting.gateSpr * rndG, ah::digital::TRIGGER, 100.0f);
	}

	void fixed(float delay, float gate) {
		delayTime = delay;
		gateTime = gate;
	}

};

