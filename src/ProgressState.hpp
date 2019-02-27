#pragma once

#include "AHCommon.hpp"

using namespace ah;

struct ProgressChord : music::Chord {

	bool gate;
	bool dirty;
	int  note;

	void reset() {
		music::Chord::reset();
		gate = true;
		dirty = true;
		note = 0;
	}

};

struct ProgressState {

	int chordMode = 0;  // 0 == Chord, 1 = Mode
	int offset = 24; 	// Repeated notes in chord and expressed in the chord definition as being transposed 2 octaves lower. 
						// When played this offset needs to be removed (or the notes removed, or the notes transposed to an octave higher)
	
	music::KnownChords knownChords;

	ProgressChord parts[32][8];

	ProgressState();
	json_t *toJson();
	void fromJson(json_t *pStateJ);

	void onReset();
	void update();
	
	void toggleGate(int part, int step);
	bool gateState(int part, int step);
	void calculateVoltages(int part, int step);
	float *getChordVoltages(int part, int step);
	ProgressChord *getChord(int part, int step);

	void copyPartFrom(int src);

	void setMode(int m);
	void setKey(int k);
	void setPart(int p);

	int mode = 0;
	int key = 0;
	int currentPart = 0;
	int nSteps = 1;

	bool stateChanged;
	bool modeChanged;

};

// Menu Items
struct RootItem : ui::MenuItem {
	ProgressChord *pChord;
	int root;

	void onAction(const ActionEvent &e) override;
};

struct DegreeItem : ui::MenuItem {
	ProgressChord *pChord;
	ProgressState *pState;
	int degree;

	void onAction(const ActionEvent &e) override;
};

struct ChordItem : ui::MenuItem {
	ProgressChord *pChord;
	int chord;

	void onAction(const ActionEvent &e) override;
};

struct OctaveItem : ui::MenuItem {
	ProgressChord *pChord;
	ProgressState *pState;
	int octave;

	void onAction(const ActionEvent &e) override;
};

struct InversionItem : ui::MenuItem {
	ProgressChord *pChord;
	int inversion;

	void onAction(const ActionEvent &e) override;
};

// Menus
struct RootChoice : gui::AHChoice {
	ProgressState *pState;
	int pStep;

	void onAction(const ActionEvent &e) override;
	void step() override;
};

struct DegreeChoice : gui::AHChoice {
	ProgressState *pState;
	int pStep;

	void onAction(const ActionEvent &e) override;
	void step() override;
};

struct ChordChoice : gui::AHChoice {
	ProgressState *pState;
	int pStep;

	void onAction(const ActionEvent &e) override;
	void step() override;
};

// Chord submenu
struct ChordSubsetMenu : MenuItem {
	ProgressState *pState;
	int pStep;
    int start;
    int end;
    Menu *createChildMenu() override;
};

struct OctaveChoice : gui::AHChoice {
	ProgressState *pState;
	int pStep;

	void onAction(const ActionEvent &e) override;
	void step() override;
};

struct InversionChoice : gui::AHChoice {
	ProgressState *pState;
	int pStep;

	void onAction(const ActionEvent &e) override;
	void step() override;
};

struct StatusBox : gui::AHChoice {
	ProgressState *pState;

	void step() override;
};

struct ProgressStepWidget : LedDisplay {

	ChordChoice *chordChooser;
	RootChoice *rootChooser;
	DegreeChoice *degreeChooser;
	InversionChoice *inversionChooser;
	OctaveChoice *octaveChooser;

	void setPState(ProgressState *pState, int pStep);
};

struct ProgressStateWidget : LedDisplay {
	ProgressStepWidget *stepConfig[8];
	ProgressState *pState;	

	void setPState(ProgressState *pState);
};

