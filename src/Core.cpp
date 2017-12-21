#include "Core.hpp"

#include <iostream>

float Quantizer::getPitchFromVolts(float inVolts, float inRoot, float inScale, int *outRoot, int *outScale, int *outNote, int *outDegree) {
	
	// get the root note and scale
	int currRoot = getKeyFromVolts(inRoot);
	int currScale = getScaleFromVolts(inScale);
		
	if (debug && stepX % poll == 0) {
		std::cout << stepX << " Root in: " << inRoot << " Root out: " << currRoot<< " Scale in: " << inScale << " Scale out: " << currScale << std::endl;
	}	
		
	int *curScaleArr;
	int notesInScale = 0;
	switch (currScale){
		case SCALE_CHROMATIC:		curScaleArr = ASCALE_CHROMATIC;			notesInScale=LENGTHOF(ASCALE_CHROMATIC); break;
		case SCALE_IONIAN:			curScaleArr = ASCALE_IONIAN;			notesInScale=LENGTHOF(ASCALE_IONIAN); break;
		case SCALE_DORIAN:			curScaleArr = ASCALE_DORIAN;			notesInScale=LENGTHOF(ASCALE_DORIAN); break;
		case SCALE_PHRYGIAN:		curScaleArr = ASCALE_PHRYGIAN;			notesInScale=LENGTHOF(ASCALE_PHRYGIAN); break;
		case SCALE_LYDIAN:			curScaleArr = ASCALE_LYDIAN;			notesInScale=LENGTHOF(ASCALE_LYDIAN); break;
		case SCALE_MIXOLYDIAN:		curScaleArr = ASCALE_MIXOLYDIAN;		notesInScale=LENGTHOF(ASCALE_MIXOLYDIAN); break;
		case SCALE_AEOLIAN:			curScaleArr = ASCALE_AEOLIAN;			notesInScale=LENGTHOF(ASCALE_AEOLIAN); break;
		case SCALE_LOCRIAN:			curScaleArr = ASCALE_LOCRIAN;			notesInScale=LENGTHOF(ASCALE_LOCRIAN); break;
		case SCALE_MAJOR_PENTA:		curScaleArr = ASCALE_MAJOR_PENTA;		notesInScale=LENGTHOF(ASCALE_MAJOR_PENTA); break;
		case SCALE_MINOR_PENTA:		curScaleArr = ASCALE_MINOR_PENTA;		notesInScale=LENGTHOF(ASCALE_MINOR_PENTA); break;
		case SCALE_HARMONIC_MINOR:	curScaleArr = ASCALE_HARMONIC_MINOR;	notesInScale=LENGTHOF(ASCALE_HARMONIC_MINOR); break;
		case SCALE_BLUES:			curScaleArr = ASCALE_BLUES;				notesInScale=LENGTHOF(ASCALE_BLUES); break;
		default: 					curScaleArr = ASCALE_CHROMATIC;			notesInScale=LENGTHOF(ASCALE_CHROMATIC);
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

float Quantizer::getPitchFromVolts(float inVolts, int inRoot, int inScale, int *outNote, int *outDegree) {
	
	int currScale = inScale;
	int currRoot = inRoot;
		
	if (debug && stepX % poll == 0) {
		std::cout << "Root float: " << inRoot << " Scale float: " << inScale << std::endl;
	}	
		
	int *curScaleArr;
	int notesInScale = 0;
	switch (currScale){
		case SCALE_CHROMATIC:			curScaleArr = ASCALE_CHROMATIC;			notesInScale=LENGTHOF(ASCALE_CHROMATIC); break;
		case SCALE_IONIAN:			curScaleArr = ASCALE_IONIAN;			notesInScale=LENGTHOF(ASCALE_IONIAN); break;
		case SCALE_DORIAN:			curScaleArr = ASCALE_DORIAN;			notesInScale=LENGTHOF(ASCALE_DORIAN); break;
		case SCALE_PHRYGIAN:			curScaleArr = ASCALE_PHRYGIAN;			notesInScale=LENGTHOF(ASCALE_PHRYGIAN); break;
		case SCALE_LYDIAN:			curScaleArr = ASCALE_LYDIAN;			notesInScale=LENGTHOF(ASCALE_LYDIAN); break;
		case SCALE_MIXOLYDIAN:		curScaleArr = ASCALE_MIXOLYDIAN;		notesInScale=LENGTHOF(ASCALE_MIXOLYDIAN); break;
		case SCALE_AEOLIAN:			curScaleArr = ASCALE_AEOLIAN;			notesInScale=LENGTHOF(ASCALE_AEOLIAN); break;
		case SCALE_LOCRIAN:			curScaleArr = ASCALE_LOCRIAN;			notesInScale=LENGTHOF(ASCALE_LOCRIAN); break;
		case SCALE_MAJOR_PENTA:		curScaleArr = ASCALE_MAJOR_PENTA;		notesInScale=LENGTHOF(ASCALE_MAJOR_PENTA); break;
		case SCALE_MINOR_PENTA:		curScaleArr = ASCALE_MINOR_PENTA;		notesInScale=LENGTHOF(ASCALE_MINOR_PENTA); break;
		case SCALE_HARMONIC_MINOR:	curScaleArr = ASCALE_HARMONIC_MINOR;	notesInScale=LENGTHOF(ASCALE_HARMONIC_MINOR); break;
		case SCALE_BLUES:				curScaleArr = ASCALE_BLUES;				notesInScale=LENGTHOF(ASCALE_BLUES); break;
		default: 				curScaleArr = ASCALE_CHROMATIC;			notesInScale=LENGTHOF(ASCALE_CHROMATIC);
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


float Quantizer::getVoltsFromPitch(int inNote, int inRoot) {
	
	float semiTone = 1.0 / 12.0;
	return (inRoot + inNote) * semiTone;
	
}


