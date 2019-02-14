#pragma once

#include "AHCommon.hpp"

#include "component.hpp"

using namespace ah;

struct ProgressChord : music::Chord {

	bool gate;
	bool dirty;

};

struct ProgressState {

	int chordMode = 0;  // 0 == Chord, 1 = Mode
	int offset = 24; 	// Repeated notes in chord and expressed in the chord definition as being transposed 2 octaves lower. 
						// When played this offset needs to be removed (or the notes removed, or the notes transposed to an octave higher)
	
	ProgressChord chords[8];
	music::KnownChords knownChords;

	ProgressState();

	void setMode(int m);
	void setKey(int k);

	int mode;
	int key;
	int nSteps;

	bool dirty = true; // read on first run through
	bool settingChanged = false;

};

struct RootItem : ui::MenuItem {
	ProgressChord *pChord;
	int root;

	void onAction(const event::Action &e) override;
};

struct DegreeItem : ui::MenuItem {
	ProgressChord *pChord;
	ProgressState *pState;
	int degree;

	void onAction(const event::Action &e) override;
};

struct RootChoice : gui::AHChoice {
	ProgressState *pState;
	int pStep;

	void onAction(const event::Action &e) override;
	void step() override;
};

struct ChordItem : ui::MenuItem {
	ProgressChord *pChord;
	int chord;

	void onAction(const event::Action &e) override;
};

struct ChordChoice : gui::AHChoice {
	ProgressState *pState;
	int pStep;

	void onAction(const event::Action &e) override;
	void step() override;
};

struct InversionItem : ui::MenuItem {
	ProgressChord *pChord;
	int inversion;

	void onAction(const event::Action &e) override;
};

struct InversionChoice : gui::AHChoice {
	ProgressState *pState;
	int pStep;

	void onAction(const event::Action &e) override;
	void step() override;
};


struct ProgressStepWidget : LedDisplay {

	RootChoice *rootChooser;
	ChordChoice *chordChooser;
	InversionChoice *inversionChooser;
	LedDisplaySeparator *separators[2];

	void setPState(ProgressState *pState, int pStep);
};

struct ProgressStateWidget : LedDisplay {
	ProgressStepWidget *stepConfig[8];
	ProgressState *pState;	

	void setPState(ProgressState *pState);
};

