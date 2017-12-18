#pragma once

#include "AH.hpp"

#include <iostream>

struct Quantizer {

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
	int SCALE_CHROMATIC      [13]= {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; 	// All of the notes
	int SCALE_IONIAN         [8] = {0, 2, 4, 5, 7, 9, 11, 12};			// 1,2,3,4,5,6,7
	int SCALE_DORIAN         [8] = {0, 2, 3, 5, 7, 9, 10, 12};			// 1,2,b3,4,5,6,b7
	int SCALE_PHRYGIAN       [8] = {0, 1, 3, 5, 7, 8, 10, 12};			// 1,b2,b3,4,5,b6,b7
	int SCALE_LYDIAN         [8] = {0, 2, 4, 6, 7, 9, 10, 12};			// 1,2,3,#4,5,6,7
	int SCALE_MIXOLYDIAN     [8] = {0, 2, 4, 5, 7, 9, 10, 12};			// 1,2,3,4,5,6,b7 
	int SCALE_AEOLIAN        [8] = {0, 2, 3, 5, 7, 8, 10, 12};			// 1,2,b3,4,5,b6,b7
	int SCALE_LOCRIAN        [8] = {0, 1, 3, 5, 6, 8, 10, 12};			// 1,b2,b3,4,b5,b6,b7
	int SCALE_MAJOR_PENTA    [6] = {0, 2, 4, 7, 9, 12};					// 1,2,3,5,6
	int SCALE_MINOR_PENTA    [6] = {0, 3, 5, 7, 10, 12};				// 1,b3,4,5,b7
	int SCALE_HARMONIC_MINOR [8] = {0, 2, 3, 5, 7, 8, 11, 12};			// 1,2,b3,4,5,b6,7
	int SCALE_BLUES          [7] = {0, 3, 5, 6, 7, 10, 12};				// 1,b3,4,b5,5,b7

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

	int CIRCLE_FIFTHS [12] = {
		NOTE_C,
		NOTE_G,
		NOTE_D,
		NOTE_A,
		NOTE_E, 
		NOTE_B,
		NOTE_G_FLAT,
		NOTE_D_FLAT,
		NOTE_A_FLAT,
		NOTE_E_FLAT,
		NOTE_B_FLAT,
		NOTE_F
	};


	std::string noteNames[12] = {
		"C",
		"Db",
		"D",
		"Eb",
		"E",
		"F",
		"Gb",
		"G",
		"Ab",
		"A",
		"Bb",
		"B",
	};

	Notes root = NOTE_C;

	enum Scales {
		CHROMATIC = 0,
 		IONIAN,
		DORIAN,
		PHRYGIAN,
		LYDIAN,
		MIXOLYDIAN,
		AEOLIAN,
		LOCRIAN,
		MAJOR_PENTA,
		MINOR_PENTA,
		HARMONIC_MINOR,
		BLUES,
		NUM_SCALES
	};

	std::string scaleNames[12] = {
		"Chromatic",
		"Ionian (Major)",
		"Dorian",
		"Phrygian",
		"Lydian",
		"Mixolydian",
		"Aeolian (Natural Minor)",
		"Locrian",
		"Major Pentatonic",
		"Minor Pentatonic",
		"Harmonic Minor",
		"Blues"
	};

	enum Degrees {
		FIRST,
		FLAT_SECOND,
		SECOND,
		FLAT_THIRD,
		THIRD,
		FOURTH,
		FLAT_FIFTH,
		FIFTH,
		FLAT_SIXTH,
		SIXTH,
		FLAT_SEVENTH,
		SEVENTH,
		OCTAVE
	};

	std::string degreeNames[13] {
		"1",
		"b2",
		"2",
		"b3",
		"3",
		"4",
		"b5",
		"5",
		"b6",
		"6",
		"b7",
		"7",
		"O"
	};

	bool debug = false;
	int poll = 5000;
	int stepX = 0;

	/*
	* Convert a V/OCT voltage to a quantized pitch, key and scale, and calculate various information about the quantised note.
	*/
	
	float getPitchFromVolts(float inVolts, int inRoot, int inScale, int *outNote, int *outDegree);

	float getPitchFromVolts(float inVolts, float inRoot, float inScale, int *outRoot, int *outScale, int *outNote, int *outDegree);
	
	/*
	 * Convert a root note (relative to C, C=0) and positive semi-tone offset from that root to a voltage (1V/OCT, 0V = C4 (or 3??))
	 * IMPORTANT: If inNote is negative, this funciton will return -10V. This convention modules such as Arp to detect connected but invalid inputs
	 * and modules using this function to use fix sized arrays to store variable length chords.
	 * DO NOT TRY TO DO CLEVER THINGS WITH THIS FUNCTION
	 */
	float getVoltsFromPitch(int inNote, int inRoot);
	
	/* From the numerical key on a keyboard (0 = C, 11 = B), spacing in px between white keys and a starting x and Y coordinate for the C key (in px)
	* calculate the actual X and Y coordinate for a key, and the scale note to which that key belongs (see Midi note mapping)
	* http://www.grantmuller.com/MidiReference/doc/midiReference/ScaleReference.html */
	void calculateKey(int inKey, float spacing, float xOff, float yOff, float *x, float *y, int *scale); 
	
	
	float getVoltsFromScale(int scale) {
		return rescalef(scale, 0, Quantizer::NUM_SCALES - 1, 0.0, 10.0);
	}
	
	int getScaleFromVolts(float volts) {
		return round(rescalef(fabs(volts), 0.0, 10.0, 0, Quantizer::NUM_SCALES - 1));
	}

	float getVoltsFromKey(int key) {
		return rescalef(key, 0, Quantizer::NUM_NOTES - 1, 0.0, 10.0);
	}
	
	int getKeyFromVolts(float volts) {
		return round(rescalef(fabs(volts), 0.0, 10.0, 0, Quantizer::NUM_NOTES - 1));
	}
	
};

struct Chord {
	
	int CHORD_MAJOR		[6] = {0,4,7,-1,-1,-1};
	int CHORD_MINOR		[6] = {0,3,7,-1,-1,-1};
	
	enum CHORDS {
		MAJOR,
		MINOR,
		NUM_CHORDS
	};
	
	std::string ChordNames [2] {
		"Major",
		"Minor"
	};

};

