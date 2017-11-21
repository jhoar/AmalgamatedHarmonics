#include "Core.hpp"

#include <iostream>

float Quantizer::getPitchFromVolts(float inVolts, float inRoot, float inScale, int *outRoot, int *outScale, int *outNote, int *outDegree) {
	
	stepX++;
		
	// get the root note and scale
	int currRoot = round(rescalef(fabs(inRoot), 0, 10, 0, Quantizer::NUM_NOTES));
	int currScale = round(rescalef(fabs(inScale), 0, 10, 0, Quantizer::NUM_SCALES));

	int *curScaleArr;
	int notesInScale = 0;
	switch (currScale){
		case CHROMATIC:			curScaleArr = SCALE_CHROMATIC;		notesInScale=LENGTHOF(SCALE_CHROMATIC); break;
		case IONIAN:			curScaleArr = SCALE_IONIAN;			notesInScale=LENGTHOF(SCALE_IONIAN); break;
		case DORIAN:			curScaleArr = SCALE_DORIAN;			notesInScale=LENGTHOF(SCALE_DORIAN); break;
		case PHRYGIAN:			curScaleArr = SCALE_PHRYGIAN;		notesInScale=LENGTHOF(SCALE_PHRYGIAN); break;
		case LYDIAN:			curScaleArr = SCALE_LYDIAN;			notesInScale=LENGTHOF(SCALE_LYDIAN); break;
		case MIXOLYDIAN:		curScaleArr = SCALE_MIXOLYDIAN;		notesInScale=LENGTHOF(SCALE_MIXOLYDIAN); break;
		case AEOLIAN:			curScaleArr = SCALE_AEOLIAN;		notesInScale=LENGTHOF(SCALE_AEOLIAN); break;
		case LOCRIAN:			curScaleArr = SCALE_LOCRIAN;		notesInScale=LENGTHOF(SCALE_LOCRIAN); break;
		case MAJOR_PENTA:		curScaleArr = SCALE_MAJOR_PENTA;	notesInScale=LENGTHOF(SCALE_MAJOR_PENTA); break;
		case MINOR_PENTA:		curScaleArr = SCALE_MINOR_PENTA;	notesInScale=LENGTHOF(SCALE_MINOR_PENTA); break;
		case HARMONIC_MINOR:	curScaleArr = SCALE_HARMONIC_MINOR;	notesInScale=LENGTHOF(SCALE_HARMONIC_MINOR); break;
		case BLUES:				curScaleArr = SCALE_BLUES;			notesInScale=LENGTHOF(SCALE_BLUES); break;
		default: 				curScaleArr = SCALE_CHROMATIC;		notesInScale=LENGTHOF(SCALE_CHROMATIC);
	}

	// get the octave
	int octave = floor(inVolts);
	float closestVal = 10.0;
	float closestDist = 10.0;
	int noteFound = 0;

	if (debug && stepX % poll == 0) {
		std::cout << "Octave: " << octave << " Scale: " << scaleNames[currScale] << " Root: " << noteNames[currRoot] << std::endl;
	}

	for (int i = 0; i < notesInScale; i++) {

		float fOctave = (float)octave;
		int degree = curScaleArr[i]; // 0 - 11!
		float fVoltsAboveOctave = degree / 12.0;
		float fScaleNoteInVolts = fOctave + fVoltsAboveOctave;
		float distAway = fabs(inVolts - fScaleNoteInVolts);

		if (debug && stepX % poll == 0) {
			std::cout << "input: " << inVolts 
			<< " index: " << i 
			<< " root: " << currRoot
			<< " octave: " << fOctave
			<< " degree: " << degree
			<< " V above O: " << fVoltsAboveOctave
			<< " note in V: " << fScaleNoteInVolts
			<< " distance: " << distAway
			<< std::endl;
		}

		// Assume that the list of notes is ordered, so there is an single inflection point at the minimum value 
		if (distAway > closestDist){
			noteFound = i - 1; // We break here because the previous note was closer, all subsequent notes are farther away
			break;
		} else {
			// Let's remember this
			closestVal = fScaleNoteInVolts;
			closestDist = distAway;
		}
	}



	int currNote = (currRoot + curScaleArr[noteFound]) % 12;
	if (debug && stepX % poll == 0) {
		// Dump the note and degree, mod the size in case where we have wrapped round

		std::cout << "Found index in scale: " << noteFound << ", currNote: "  << currNote <<  " (Name: " << noteNames[currNote] << ")" << std::endl;
		std::cout << "This is scale note: "  << curScaleArr[noteFound] << " (Name: " << degreeNames[curScaleArr[noteFound]] << ")" << std::endl;
	}

	*outRoot = currRoot;
	*outScale = currScale;
	*outNote = currNote;
	*outDegree = curScaleArr[noteFound];
	
	return closestVal;
 
}

void Quantizer::calculateKey(int inKey, float spacing, float xOff, float yOff, float *x, float *y, int *scale) {

	// Sanitise input
	int key = inKey % 12;

	// White
	int whiteO [7] = {0, 1, 2, 3, 4, 5, 6};		// 'spaces' occupied by keys
	int whiteK [7] = {0, 2, 4, 5, 7, 9, 11};	// scale mapped to key
	// Black
	int blackO [5] = {0, 1, 3, 4, 5};			// 'spaces' occupied by keys
	int blackK [5] = {1, 3, 6, 8, 10};			// scale mapped to keys

	float bOffset = xOff + (spacing / 2);
	float bSpace = spacing;
	// Make an equilateral triangle pattern, tri height = base * (sqrt(3) / 2)
	float bDelta = bSpace * sqrt(3) * 0.5;

	// Are we black or white key?, check black keys
	for (int i = 0; i < 5; i++) {
		if (blackK[i] == key) {
			*x = bOffset + bSpace * blackO[i];
			*y = yOff - bDelta;
			*scale = blackK[i];
			return;
		}
	}

	int wOffset = xOff;
	int wSpace = spacing;
	
	for (int i = 0; i < 7; i++) {
		if (whiteK[i] == key) {
			*x = wOffset + wSpace * whiteO[i];
			*y = yOff;
			*scale = whiteK[i];
			return;
		}
	}
}

