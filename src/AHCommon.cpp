#include "AHCommon.hpp"

#include <sstream>

namespace ah {

namespace gui {

AHChoice::AHChoice() {
    box.size = mm2px(math::Vec(0, 28.0 / 3)); // FIX
    font = APP->window->loadFont(asset::plugin(pluginInstance, "res/EurostileBold.ttf")); // FIX
    color = nvgRGB(0x00, 0xFF, 0xFF);
    bgColor = nvgRGBAf(30, 30, 30, 0); 
    textOffset = math::Vec(10, 18);	// FIX?	
	fontSize = 12.0;
}

void AHChoice::draw(NVGcontext *vg) {
    nvgScissor(vg, 0, 0, box.size.x, box.size.y);

    if (font->handle >= 0) {
        nvgFillColor(vg, color);
        nvgFontFaceId(vg, font->handle);
        nvgTextLetterSpacing(vg, 0.0);

        nvgFontSize(vg, fontSize); // FIX
        nvgText(vg, textOffset.x, textOffset.y, text.c_str(), NULL);
    }

    nvgResetScissor(vg);
}

float Y_KNOB[2]    = {50.8, 56.0}; // w.r.t 22 = 28.8 from bottom
float Y_PORT[2]    = {49.7, 56.0}; // 27.7
float Y_BUTTON[2]  = {53.3, 56.0}; // 31.3 
float Y_LIGHT[2]   = {57.7, 56.0}; // 35.7
float Y_TRIMPOT[2] = {52.8, 56.0}; // 30.8

float Y_KNOB_COMPACT[2]     = {30.1, 35.0}; // Calculated relative to  PORT=29 and the deltas above
float Y_PORT_COMPACT[2]     = {29.0, 35.0};
float Y_BUTTON_COMPACT[2]   = {32.6, 35.0};
float Y_LIGHT_COMPACT[2]    = {37.0, 35.0};
float Y_TRIMPOT_COMPACT[2]  = {32.1, 35.0};

float X_KNOB[2]     = {12.5, 48.0}; // w.r.t 6.5 = 6 from left
float X_PORT[2]     = {11.5, 48.0}; // 5
float X_BUTTON[2]   = {14.7, 48.0}; // 8.2
float X_LIGHT[2]    = {19.1, 48.0}; // 12.6
float X_TRIMPOT[2]  = {14.7, 48.0}; // 8.2

float X_KNOB_COMPACT[2]     = {21.0, 35.0}; // 15 + 6, see calc above
float X_PORT_COMPACT[2]     = {20.0, 35.0}; // 15 + 5
float X_BUTTON_COMPACT[2]   = {23.2, 35.0}; // 15 + 8.2
float X_LIGHT_COMPACT[2]    = {27.6, 35.0}; // 15 + 12.6
float X_TRIMPOT_COMPACT[2]  = {23.2, 35.0}; // 15 + 8.2

std::string KeyParamQuantity::getDisplayValueString() {
	int k = (int)getValue();
	return music::noteNames[k];
};

std::string ModeParamQuantity::getDisplayValueString() {
	int k = (int)getValue();
	return music::modeNames[k];
};

Vec getPosition(int type, int xSlot, int ySlot, bool xDense, bool yDense, float xDelta, float yDelta) {

	float *xArray;
	float *yArray;
	
	switch(type) {
		case KNOB: 
		if (xDense) { xArray = X_KNOB_COMPACT; } else { xArray = X_KNOB; }
		if (yDense) { yArray = Y_KNOB_COMPACT; } else { yArray = Y_KNOB; }
		break;
		case PORT: 
		if (xDense) { xArray = X_PORT_COMPACT; } else { xArray = X_PORT; }
		if (yDense) { yArray = Y_PORT_COMPACT; } else { yArray = Y_PORT; }
		break;
		case BUTTON:
		if (xDense) { xArray = X_BUTTON_COMPACT; } else { xArray = X_BUTTON; }
		if (yDense) { yArray = Y_BUTTON_COMPACT; } else { yArray = Y_BUTTON; }
		break;
		case LIGHT: 
		if (xDense) { xArray = X_LIGHT_COMPACT; } else { xArray = X_LIGHT; }
		if (yDense) { yArray = Y_LIGHT_COMPACT; } else { yArray = Y_LIGHT; }
		break;
		case TRIMPOT: 
		if (xDense) { xArray = X_TRIMPOT_COMPACT; } else { xArray = X_TRIMPOT; }
		if (yDense) { yArray = Y_TRIMPOT_COMPACT; } else { yArray = Y_TRIMPOT; }
		break;
		default:
		if (xDense) { xArray = X_KNOB_COMPACT; } else { xArray = X_KNOB; }
		if (yDense) { yArray = Y_KNOB_COMPACT; } else { yArray = Y_KNOB; }
	}

	return Vec(xDelta + xArray[0] + xArray[1] * xSlot, yDelta + yArray[0] + yArray[1] * ySlot);

}

/* From the numerical key on a keyboard (0 = C, 11 = B), spacing in px between white keys and a starting x and Y coordinate for the C key (in px)
* calculate the actual X and Y coordinate for a key, and the scale note to which that key belongs (see Midi note mapping)
* http://www.grantmuller.com/MidiReference/doc/midiReference/ScaleReference.html */
void calculateKeyboard(int inKey, float spacing, float xOff, float yOff, float *x, float *y, int *scale) {
  
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

} // namespace gui

namespace digital {

double gaussrand() {
 	static double U, V;
 	static int phase = 0;
 	double Z;

 	if(phase == 0) {
 		U = (rand() + 1.) / (RAND_MAX + 2.);
 		V = rand() / (RAND_MAX + 1.);
 		Z = sqrt(-2 * log(U)) * sin(2 * core::PI * V);
 	} else
 		Z = sqrt(-2 * log(U)) * cos(2 * core::PI * V);

 	phase = 1 - phase;

 	return Z;
}

}

namespace music {

Chord::Chord() : rootNote(0), quality(0), chord(1), modeDegree(0), inversion(0) {
	setVoltages(defaultChord.formula, 12);
}

void Chord::setVoltages(int *chordArray, int offset) {
	for (int j = 0; j < 6; j++) {
		if (chordArray[j] < 0) {
			int off = offset;
			if (offset == 0) { // if offset = 0, randomise offset per note
				off = (rand() % 3 + 1) * 12;
			}
			outVolts[j] = getVoltsFromPitch(chordArray[j] + off,rootNote);			
		} else {
			outVolts[j] = getVoltsFromPitch(chordArray[j],      rootNote);
		}	
	}
}

void Chord::setVoltages(std::vector<int> &chordArray, int offset) {
	for (int j = 0; j < 6; j++) {
		if (chordArray[j] < 0) {
			int off = offset;
			if (offset == 0) { // if offset = 0, randomise offset per note
				off = (rand() % 3 + 1) * 12;
			}
			outVolts[j] = getVoltsFromPitch(chordArray[j] + off,rootNote);			
		} else {
			outVolts[j] = getVoltsFromPitch(chordArray[j],      rootNote);
		}	
	}
}


ChordDef ChordTable[NUM_CHORDS] {
	{	0	,"None",{	-24	,	-24	,	-24	,	-24	,	-24	,	-24	},{	-24	,	-24	,	-24	,	-24	,	-24	,	-24	},{	-24	,	-24	,	-24	,	-24	,	-24	,	-24	}},
	{	1	,"M",{	0	,	4	,	7	,	-24	,	-20	,	-17	},{	12	,	4	,	7	,	-12	,	-20	,	-17	},{	12	,	16	,	7	,	-12	,	-8	,	-17	}},
	{	2	,"M#5",{	0	,	4	,	8	,	-24	,	-20	,	-16	},{	12	,	4	,	8	,	-12	,	-20	,	-16	},{	12	,	16	,	8	,	-12	,	-8	,	-16	}},
	{	3	,"M#5add9",{	0	,	4	,	8	,	14	,	-24	,	-20	},{	12	,	4	,	8	,	14	,	-24	,	-20	},{	12	,	16	,	8	,	14	,	-12	,	-20	}},
	{	4	,"M13",{	0	,	4	,	7	,	11	,	14	,	21	},{	12	,	4	,	7	,	11	,	14	,	21	},{	12	,	16	,	7	,	11	,	14	,	21	}},
	{	5	,"M6",{	0	,	4	,	7	,	21	,	-24	,	-20	},{	12	,	4	,	7	,	21	,	-24	,	-20	},{	12	,	16	,	7	,	21	,	-12	,	-20	}},
	{	6	,"M6#11",{	0	,	4	,	7	,	9	,	18	,	-24	},{	12	,	4	,	7	,	9	,	18	,	-24	},{	12	,	16	,	7	,	9	,	18	,	-24	}},
	{	7	,"M6/9",{	0	,	4	,	7	,	9	,	14	,	-24	},{	12	,	4	,	7	,	9	,	14	,	-24	},{	12	,	16	,	7	,	9	,	14	,	-24	}},
	{	8	,"M6/9#11",{	0	,	4	,	7	,	9	,	14	,	18	},{	12	,	4	,	7	,	9	,	14	,	18	},{	12	,	16	,	7	,	9	,	14	,	18	}},
	{	9	,"M7#11",{	0	,	4	,	7	,	11	,	18	,	-24	},{	12	,	4	,	7	,	11	,	18	,	-24	},{	12	,	16	,	7	,	11	,	18	,	-24	}},
	{	10	,"M7#5",{	0	,	4	,	8	,	11	,	-24	,	-20	},{	12	,	4	,	8	,	11	,	-24	,	-20	},{	12	,	16	,	8	,	11	,	-12	,	-20	}},
	{	11	,"M7#5sus4",{	0	,	5	,	8	,	11	,	-24	,	-19	},{	12	,	5	,	8	,	11	,	-24	,	-19	},{	12	,	17	,	8	,	11	,	-12	,	-19	}},
	{	12	,"M7#9#11",{	0	,	4	,	7	,	11	,	15	,	18	},{	12	,	4	,	7	,	11	,	15	,	18	},{	12	,	16	,	7	,	11	,	15	,	18	}},
	{	13	,"M7add13",{	0	,	4	,	7	,	9	,	11	,	14	},{	12	,	4	,	7	,	9	,	11	,	14	},{	12	,	16	,	7	,	9	,	11	,	14	}},
	{	14	,"M7b5",{	0	,	4	,	6	,	11	,	-24	,	-20	},{	12	,	4	,	6	,	11	,	-24	,	-20	},{	12	,	16	,	6	,	11	,	-12	,	-20	}},
	{	15	,"M7b6",{	0	,	4	,	8	,	11	,	-24	,	-20	},{	12	,	4	,	8	,	11	,	-24	,	-20	},{	12	,	16	,	8	,	11	,	-12	,	-20	}},
	{	16	,"M7b9",{	0	,	4	,	7	,	11	,	13	,	-20	},{	12	,	4	,	7	,	11	,	13	,	-20	},{	12	,	16	,	7	,	11	,	13	,	-20	}},
	{	17	,"M7sus4",{	0	,	5	,	7	,	11	,	-24	,	-19	},{	12	,	5	,	7	,	11	,	-24	,	-19	},{	12	,	17	,	7	,	11	,	-12	,	-19	}},
	{	18	,"M9",{	0	,	4	,	7	,	11	,	14	,	-24	},{	12	,	4	,	7	,	11	,	14	,	-24	},{	12	,	16	,	7	,	11	,	14	,	-24	}},
	{	19	,"M9#11",{	0	,	4	,	7	,	11	,	14	,	18	},{	12	,	4	,	7	,	11	,	14	,	18	},{	12	,	16	,	7	,	11	,	14	,	18	}},
	{	20	,"M9#5",{	0	,	4	,	8	,	11	,	14	,	-24	},{	12	,	4	,	8	,	11	,	14	,	-24	},{	12	,	16	,	8	,	11	,	14	,	-24	}},
	{	21	,"M9#5sus4",{	0	,	5	,	8	,	11	,	14	,	-24	},{	12	,	5	,	8	,	11	,	14	,	-24	},{	12	,	17	,	8	,	11	,	14	,	-24	}},
	{	22	,"M9b5",{	0	,	4	,	6	,	11	,	14	,	-24	},{	12	,	4	,	6	,	11	,	14	,	-24	},{	12	,	16	,	6	,	11	,	14	,	-24	}},
	{	23	,"M9sus4",{	0	,	5	,	7	,	11	,	14	,	-24	},{	12	,	5	,	7	,	11	,	14	,	-24	},{	12	,	17	,	7	,	11	,	14	,	-24	}},
	{	24	,"Madd9",{	0	,	4	,	7	,	14	,	-24	,	-20	},{	12	,	4	,	7	,	14	,	-24	,	-20	},{	12	,	16	,	7	,	14	,	-12	,	-20	}},
	{	25	,"Maj7",{	0	,	4	,	7	,	11	,	-24	,	-20	},{	12	,	4	,	7	,	11	,	-24	,	-20	},{	12	,	16	,	7	,	11	,	-12	,	-20	}},
	{	26	,"Mb5",{	0	,	4	,	6	,	-24	,	-20	,	-18	},{	12	,	4	,	6	,	-12	,	-20	,	-18	},{	12	,	16	,	6	,	-12	,	-8	,	-18	}},
	{	27	,"Mb6",{	0	,	4	,	20	,	-24	,	-20	,	-4	},{	12	,	4	,	20	,	-12	,	-20	,	-4	},{	12	,	16	,	20	,	-12	,	-8	,	-4	}},
	{	28	,"Msus2",{	0	,	2	,	7	,	-24	,	-22	,	-17	},{	12	,	2	,	7	,	-12	,	-22	,	-17	},{	12	,	14	,	7	,	-12	,	-10	,	-17	}},
	{	29	,"Msus4",{	0	,	5	,	7	,	-24	,	-19	,	-17	},{	12	,	5	,	7	,	-12	,	-19	,	-17	},{	12	,	17	,	7	,	-12	,	-7	,	-17	}},
	{	30	,"Maddb9",{	0	,	4	,	7	,	13	,	-24	,	-20	},{	12	,	4	,	7	,	13	,	-24	,	-20	},{	12	,	16	,	7	,	13	,	-12	,	-20	}},
	{	31	,"7",{	0	,	4	,	7	,	10	,	-24	,	-20	},{	12	,	4	,	7	,	10	,	-24	,	-20	},{	12	,	16	,	7	,	10	,	-12	,	-20	}},
	{	32	,"9",{	0	,	4	,	7	,	10	,	14	,	-24	},{	12	,	4	,	7	,	10	,	14	,	-24	},{	12	,	16	,	7	,	10	,	14	,	-24	}},
	{	33	,"11",{	0	,	7	,	10	,	14	,	17	,	-24	},{	12	,	7	,	10	,	14	,	17	,	-24	},{	12	,	19	,	10	,	14	,	17	,	-24	}},
	{	34	,"13",{	0	,	4	,	7	,	10	,	14	,	21	},{	12	,	4	,	7	,	10	,	14	,	21	},{	12	,	16	,	7	,	10	,	14	,	21	}},
	{	35	,"11b9",{	0	,	7	,	10	,	13	,	17	,	-24	},{	12	,	7	,	10	,	13	,	17	,	-24	},{	12	,	19	,	10	,	13	,	17	,	-24	}},
	{	36	,"13#9",{	0	,	4	,	7	,	10	,	15	,	21	},{	12	,	4	,	7	,	10	,	15	,	21	},{	12	,	16	,	7	,	10	,	15	,	21	}},
	{	37	,"13b5",{	0	,	4	,	6	,	9	,	10	,	14	},{	12	,	4	,	6	,	9	,	10	,	14	},{	12	,	16	,	6	,	9	,	10	,	14	}},
	{	38	,"13b9",{	0	,	4	,	7	,	10	,	13	,	21	},{	12	,	4	,	7	,	10	,	13	,	21	},{	12	,	16	,	7	,	10	,	13	,	21	}},
	{	39	,"13no5",{	0	,	4	,	10	,	14	,	21	,	-24	},{	12	,	4	,	10	,	14	,	21	,	-24	},{	12	,	16	,	10	,	14	,	21	,	-24	}},
	{	40	,"13sus4",{	0	,	5	,	7	,	10	,	14	,	21	},{	12	,	5	,	7	,	10	,	14	,	21	},{	12	,	17	,	7	,	10	,	14	,	21	}},
	{	41	,"69#11",{	0	,	4	,	7	,	9	,	14	,	18	},{	12	,	4	,	7	,	9	,	14	,	18	},{	12	,	16	,	7	,	9	,	14	,	18	}},
	{	42	,"7#11",{	0	,	4	,	7	,	10	,	18	,	-24	},{	12	,	4	,	7	,	10	,	18	,	-24	},{	12	,	16	,	7	,	10	,	18	,	-24	}},
	{	43	,"7#11b13",{	0	,	4	,	7	,	10	,	18	,	20	},{	12	,	4	,	7	,	10	,	18	,	20	},{	12	,	16	,	7	,	10	,	18	,	20	}},
	{	44	,"7#5",{	0	,	4	,	8	,	10	,	-24	,	-20	},{	12	,	4	,	8	,	10	,	-24	,	-20	},{	12	,	16	,	8	,	10	,	-12	,	-20	}},
	{	45	,"7#5#9",{	0	,	4	,	8	,	10	,	15	,	-24	},{	12	,	4	,	8	,	10	,	15	,	-24	},{	12	,	16	,	8	,	10	,	15	,	-24	}},
	{	46	,"7#5b9",{	0	,	4	,	8	,	10	,	13	,	-24	},{	12	,	4	,	8	,	10	,	13	,	-24	},{	12	,	16	,	8	,	10	,	13	,	-24	}},
	{	47	,"7#5b9#11",{	0	,	4	,	8	,	10	,	13	,	18	},{	12	,	4	,	8	,	10	,	13	,	18	},{	12	,	16	,	8	,	10	,	13	,	18	}},
	{	48	,"7#5sus4",{	0	,	5	,	8	,	10	,	-24	,	-19	},{	12	,	5	,	8	,	10	,	-24	,	-19	},{	12	,	17	,	8	,	10	,	-12	,	-19	}},
	{	49	,"7#9",{	0	,	4	,	7	,	10	,	15	,	-24	},{	12	,	4	,	7	,	10	,	15	,	-24	},{	12	,	16	,	7	,	10	,	15	,	-24	}},
	{	50	,"7#9#11",{	0	,	4	,	7	,	10	,	15	,	18	},{	12	,	4	,	7	,	10	,	15	,	18	},{	12	,	16	,	7	,	10	,	15	,	18	}},
	{	51	,"7#9b13",{	0	,	4	,	7	,	10	,	15	,	20	},{	12	,	4	,	7	,	10	,	15	,	20	},{	12	,	16	,	7	,	10	,	15	,	20	}},
	{	52	,"7add6",{	0	,	4	,	7	,	10	,	21	,	-24	},{	12	,	4	,	7	,	10	,	21	,	-24	},{	12	,	16	,	7	,	10	,	21	,	-24	}},
	{	53	,"7b13",{	0	,	4	,	10	,	20	,	-24	,	-20	},{	12	,	4	,	10	,	20	,	-24	,	-20	},{	12	,	16	,	10	,	20	,	-12	,	-20	}},
	{	54	,"7b5",{	0	,	4	,	6	,	10	,	-24	,	-20	},{	12	,	4	,	6	,	10	,	-24	,	-20	},{	12	,	16	,	6	,	10	,	-12	,	-20	}},
	{	55	,"7b6",{	0	,	4	,	7	,	8	,	10	,	-24	},{	12	,	4	,	7	,	8	,	10	,	-24	},{	12	,	16	,	7	,	8	,	10	,	-24	}},
	{	56	,"7b9",{	0	,	4	,	7	,	10	,	13	,	-24	},{	12	,	4	,	7	,	10	,	13	,	-24	},{	12	,	16	,	7	,	10	,	13	,	-24	}},
	{	57	,"7b9#11",{	0	,	4	,	7	,	10	,	13	,	18	},{	12	,	4	,	7	,	10	,	13	,	18	},{	12	,	16	,	7	,	10	,	13	,	18	}},
	{	58	,"7b9#9",{	0	,	4	,	7	,	10	,	13	,	15	},{	12	,	4	,	7	,	10	,	13	,	15	},{	12	,	16	,	7	,	10	,	13	,	15	}},
	{	59	,"7b9b13",{	0	,	4	,	7	,	10	,	13	,	20	},{	12	,	4	,	7	,	10	,	13	,	20	},{	12	,	16	,	7	,	10	,	13	,	20	}},
	{	60	,"7no5",{	0	,	4	,	10	,	-24	,	-20	,	-14	},{	12	,	4	,	10	,	-12	,	-20	,	-14	},{	12	,	16	,	10	,	-12	,	-8	,	-14	}},
	{	61	,"7sus4",{	0	,	5	,	7	,	10	,	-24	,	-19	},{	12	,	5	,	7	,	10	,	-24	,	-19	},{	12	,	17	,	7	,	10	,	-12	,	-19	}},
	{	62	,"7sus4b9",{	0	,	5	,	7	,	10	,	13	,	-24	},{	12	,	5	,	7	,	10	,	13	,	-24	},{	12	,	17	,	7	,	10	,	13	,	-24	}},
	{	63	,"7sus4b9b13",{	0	,	5	,	7	,	10	,	13	,	20	},{	12	,	5	,	7	,	10	,	13	,	20	},{	12	,	17	,	7	,	10	,	13	,	20	}},
	{	64	,"9#11",{	0	,	4	,	7	,	10	,	14	,	18	},{	12	,	4	,	7	,	10	,	14	,	18	},{	12	,	16	,	7	,	10	,	14	,	18	}},
	{	65	,"9#5",{	0	,	4	,	8	,	10	,	14	,	-24	},{	12	,	4	,	8	,	10	,	14	,	-24	},{	12	,	16	,	8	,	10	,	14	,	-24	}},
	{	66	,"9#5#11",{	0	,	4	,	8	,	10	,	14	,	18	},{	12	,	4	,	8	,	10	,	14	,	18	},{	12	,	16	,	8	,	10	,	14	,	18	}},
	{	67	,"9b13",{	0	,	4	,	10	,	14	,	20	,	-24	},{	12	,	4	,	10	,	14	,	20	,	-24	},{	12	,	16	,	10	,	14	,	20	,	-24	}},
	{	68	,"9b5",{	0	,	4	,	6	,	10	,	14	,	-24	},{	12	,	4	,	6	,	10	,	14	,	-24	},{	12	,	16	,	6	,	10	,	14	,	-24	}},
	{	69	,"9no5",{	0	,	4	,	10	,	14	,	-24	,	-20	},{	12	,	4	,	10	,	14	,	-24	,	-20	},{	12	,	16	,	10	,	14	,	-12	,	-20	}},
	{	70	,"9sus4",{	0	,	5	,	7	,	10	,	14	,	-24	},{	12	,	5	,	7	,	10	,	14	,	-24	},{	12	,	17	,	7	,	10	,	14	,	-24	}},
	{	71	,"m",{	0	,	3	,	7	,	-24	,	-21	,	-17	},{	12	,	3	,	7	,	-12	,	-21	,	-17	},{	12	,	15	,	7	,	-12	,	-9	,	-17	}},
	{	72	,"m#5",{	0	,	3	,	8	,	-24	,	-21	,	-16	},{	12	,	3	,	8	,	-12	,	-21	,	-16	},{	12	,	15	,	8	,	-12	,	-9	,	-16	}},
	{	73	,"m11",{	0	,	3	,	7	,	10	,	14	,	17	},{	12	,	3	,	7	,	10	,	14	,	17	},{	12	,	15	,	7	,	10	,	14	,	17	}},
	{	74	,"m11A5",{	0	,	3	,	8	,	10	,	14	,	17	},{	12	,	3	,	8	,	10	,	14	,	17	},{	12	,	15	,	8	,	10	,	14	,	17	}},
	{	75	,"m11b5",{	0	,	3	,	10	,	14	,	17	,	18	},{	12	,	3	,	10	,	14	,	17	,	18	},{	12	,	15	,	10	,	14	,	17	,	18	}},
	{	76	,"m6",{	0	,	3	,	5	,	7	,	21	,	-24	},{	12	,	3	,	5	,	7	,	21	,	-24	},{	12	,	15	,	5	,	7	,	21	,	-24	}},
	{	77	,"m69",{	0	,	3	,	7	,	9	,	14	,	-24	},{	12	,	3	,	7	,	9	,	14	,	-24	},{	12	,	15	,	7	,	9	,	14	,	-24	}},
	{	78	,"m7",{	0	,	3	,	7	,	10	,	-24	,	-21	},{	12	,	3	,	7	,	10	,	-24	,	-21	},{	12	,	15	,	7	,	10	,	-12	,	-21	}},
	{	79	,"m7#5",{	0	,	3	,	8	,	10	,	-24	,	-21	},{	12	,	3	,	8	,	10	,	-24	,	-21	},{	12	,	15	,	8	,	10	,	-12	,	-21	}},
	{	80	,"m7add11",{	0	,	3	,	7	,	10	,	17	,	-24	},{	12	,	3	,	7	,	10	,	17	,	-24	},{	12	,	15	,	7	,	10	,	17	,	-24	}},
	{	81	,"m7b5",{	0	,	3	,	6	,	10	,	-24	,	-21	},{	12	,	3	,	6	,	10	,	-24	,	-21	},{	12	,	15	,	6	,	10	,	-12	,	-21	}},
	{	82	,"m9",{	0	,	3	,	7	,	10	,	14	,	-24	},{	12	,	3	,	7	,	10	,	14	,	-24	},{	12	,	15	,	7	,	10	,	14	,	-24	}},
	{	83	,"#5",{	0	,	3	,	8	,	10	,	14	,	-24	},{	12	,	3	,	8	,	10	,	14	,	-24	},{	12	,	15	,	8	,	10	,	14	,	-24	}},
	{	84	,"m9b5",{	0	,	3	,	10	,	14	,	18	,	-24	},{	12	,	3	,	10	,	14	,	18	,	-24	},{	12	,	15	,	10	,	14	,	18	,	-24	}},
	{	85	,"mMaj7",{	0	,	3	,	7	,	11	,	-24	,	-21	},{	12	,	3	,	7	,	11	,	-24	,	-21	},{	12	,	15	,	7	,	11	,	-12	,	-21	}},
	{	86	,"mMaj7b6",{	0	,	3	,	7	,	8	,	11	,	-24	},{	12	,	3	,	7	,	8	,	11	,	-24	},{	12	,	15	,	7	,	8	,	11	,	-24	}},
	{	87	,"mM9",{	0	,	3	,	7	,	11	,	14	,	-24	},{	12	,	3	,	7	,	11	,	14	,	-24	},{	12	,	15	,	7	,	11	,	14	,	-24	}},
	{	88	,"mM9b6",{	0	,	3	,	7	,	8	,	11	,	14	},{	12	,	3	,	7	,	8	,	11	,	14	},{	12	,	15	,	7	,	8	,	11	,	14	}},
	{	89	,"mb6M7",{	0	,	3	,	8	,	11	,	-24	,	-21	},{	12	,	3	,	8	,	11	,	-24	,	-21	},{	12	,	15	,	8	,	11	,	-12	,	-21	}},
	{	90	,"mb6b9",{	0	,	3	,	8	,	13	,	-24	,	-21	},{	12	,	3	,	8	,	13	,	-24	,	-21	},{	12	,	15	,	8	,	13	,	-12	,	-21	}},
	{	91	,"dim",{	0	,	3	,	6	,	-24	,	-21	,	-18	},{	12	,	3	,	6	,	-12	,	-21	,	-18	},{	12	,	15	,	6	,	-12	,	-9	,	-18	}},
	{	92	,"dim7",{	0	,	3	,	6	,	21	,	-24	,	-21	},{	12	,	3	,	6	,	21	,	-24	,	-21	},{	12	,	15	,	6	,	21	,	-12	,	-21	}},
	{	93	,"dim7M7",{	0	,	3	,	6	,	9	,	11	,	-24	},{	12	,	3	,	6	,	9	,	11	,	-24	},{	12	,	15	,	6	,	9	,	11	,	-24	}},
	{	94	,"dimM7",{	0	,	3	,	6	,	11	,	-24	,	-21	},{	12	,	3	,	6	,	11	,	-24	,	-21	},{	12	,	15	,	6	,	11	,	-12	,	-21	}},
	{	95	,"sus24",{	0	,	2	,	5	,	7	,	-24	,	-22	},{	12	,	2	,	5	,	7	,	-24	,	-22	},{	12	,	14	,	5	,	7	,	-12	,	-22	}},
	{	96	,"augadd#9",{	0	,	4	,	8	,	15	,	-24	,	-20	},{	12	,	4	,	8	,	15	,	-24	,	-20	},{	12	,	16	,	8	,	15	,	-12	,	-20	}},
	{	97	,"madd4",{	0	,	3	,	5	,	7	,	-24	,	-21	},{	12	,	3	,	5	,	7	,	-24	,	-21	},{	12	,	15	,	5	,	7	,	-12	,	-21	}},
	{	98	,"madd9",{	0	,	3	,	7	,	14	,	-24	,	-21	},{	12	,	3	,	7	,	14	,	-24	,	-21	},{	12	,	15	,	7	,	14	,	-12	,	-21	}},		
};

std::vector<ChordFormula> BasicChordSet {
	{"M",			{	0	,	4	,	7}},
	{"m",			{	0	,	3	,	7}},
	{"5",			{	0	,	7	,	12}},
	{"Msus2",		{	0	,	2	,	7}},
	{"Msus4",		{	0	,	5	,	7}},
	{"sus24",		{	0	,	2	,	5	,	7}},
	{"7",			{	0	,	4	,	7	,	10}},
	{"M7",			{	0	,	4	,	7	,	11}},
	{"M7add13",		{	0	,	4	,	7	,	9	,	11	,	14	}},
	{"M7#9#11",		{	0	,	4	,	7	,	11	,	15	,	18	}},
	{"M7#11",		{	0	,	4	,	7	,	11	,	18}},
	{"M7b9",		{	0	,	4	,	7	,	11	,	13}},
	{"M7b5",		{	0	,	4	,	6	,	11}},
	{"7#5",			{	0	,	4	,	8	,	10}},
	{"7#5b9",		{	0	,	4	,	8	,	10	,	13}},
	{"7#5#9",		{	0	,	4	,	8	,	10	,	15}},
	{"7#5b9#11",	{	0	,	4	,	8	,	10	,	13	,	18	}},
	{"M7b6",		{	0	,	4	,	8	,	11}},
	{"M7#5",		{	0	,	4	,	8	,	11}},
	{"M7sus4",		{	0	,	5	,	7	,	11}},
	{"M7#5sus4",	{	0	,	5	,	8	,	11}},
	{"7#9",			{	0	,	4	,	7	,	10	,	15}},
	{"7#9#11",		{	0	,	4	,	7	,	10	,	15	,	18	}},
	{"7#9b13",		{	0	,	4	,	7	,	10	,	15	,	20	}},
	{"7#11",		{	0	,	4	,	7	,	10	,	18}},
	{"7#11b13",		{	0	,	4	,	7	,	10	,	18	,	20	}},
	{"7add6",		{	0	,	4	,	7	,	10	,	21}},
	{"7b5",			{	0	,	4	,	6	,	10}},
	{"7b6",			{	0	,	4	,	7	,	8	,	10}},
	{"7b9",			{	0	,	4	,	7	,	10	,	13}},
	{"7b13",		{	0	,	4	,	10	,	20}},
	{"7b9#9",		{	0	,	4	,	7	,	10	,	13	,	15	}},
	{"7b9#11",		{	0	,	4	,	7	,	10	,	13	,	18	}},
	{"7b9b13",		{	0	,	4	,	7	,	10	,	13	,	20	}},
	{"7no5",		{	0	,	4	,	10}},
	{"7sus4",		{	0	,	5	,	7	,	10}},
	{"7sus4b9",		{	0	,	5	,	7	,	10	,	13}},
	{"7sus4b9b13",	{	0	,	5	,	7	,	10	,	13	,	20	}},
	{"m7",			{	0	,	3	,	7	,	10}},
	{"mMaj7",		{	0	,	3	,	7	,	11}},
	{"mMaj7b6",		{	0	,	3	,	7	,	8	,	11}},
	{"m7add11",		{	0	,	3	,	7	,	10	,	17}},
	{"m7#5",		{	0	,	3	,	8	,	10}},
	{"m7b5",		{	0	,	3	,	6	,	10}},
	{"9",			{	0	,	4	,	7	,	10	,	14}},
	{"M9",			{	0	,	4	,	7	,	11	,	14}},
	{"M9#11",		{	0	,	4	,	7	,	11	,	14	,	18	}},
	{"Maddb9",		{	0	,	4	,	7	,	13}},
	{"Madd9",		{	0	,	4	,	7	,	14}},
	{"M9#5",		{	0	,	4	,	8	,	11	,	14}},
	{"M9b5",		{	0	,	4	,	6	,	11	,	14}},
	{"M9#5sus4",	{	0	,	5	,	8	,	11	,	14}},
	{"M9sus4",		{	0	,	5	,	7	,	11	,	14}},
	{"11",			{	0	,	7	,	10	,	14	,	17}},
	{"dim",			{	0	,	3	,	6}},
	{"dim7M7",		{	0	,	3	,	6	,	9	,	11}},
	{"dimM7",		{	0	,	3	,	6	,	11}},
	{"dim7",		{	0	,	3	,	6	,	21}},
	{"9#11",		{	0	,	4	,	7	,	10	,	14	,	18	}},
	{"9#5",			{	0	,	4	,	8	,	10	,	14}},
	{"9#5#11",		{	0	,	4	,	8	,	10	,	14	,	18	}},
	{"9b5",			{	0	,	4	,	6	,	10	,	14}},
	{"9b13",		{	0	,	4	,	10	,	14	,	20}},
	{"9no5",		{	0	,	4	,	10	,	14}},
	{"9sus4",		{	0	,	5	,	7	,	10	,	14}},
	{"11b9",		{	0	,	7	,	10	,	13	,	17}},
	{"13",			{	0	,	4	,	7	,	10	,	14	,	21}},
	{"M13",			{	0	,	4	,	7	,	11	,	14	,	21	}},
	{"13b9",		{	0	,	4	,	7	,	10	,	13	,	21	}},
	{"13#9",		{	0	,	4	,	7	,	10	,	15	,	21	}},
	{"13b5",		{	0	,	4	,	6	,	9	,	10	,	14	}},
	{"13no5",		{	0	,	4	,	10	,	14	,	21}},
	{"13sus4",		{	0	,	5	,	7	,	10	,	14	,	21	}},
	{"M#5",			{	0	,	4	,	8}},
	{"M#5add9",		{	0	,	4	,	8	,	14}},
	{"Mb5",			{	0	,	4	,	6}},
	{"M6",			{	0	,	4	,	7	,	21}},
	{"Mb6",			{	0	,	4	,	20}},
	{"69#11",		{	0	,	4	,	7	,	9	,	14	,	18	}},
	{"M6#11",		{	0	,	4	,	7	,	9	,	18}},
	{"M6/9",		{	0	,	4	,	7	,	9	,	14}},
	{"M6/9#11",		{	0	,	4	,	7	,	9	,	14	,	18	}},
	{"mM9b6",		{	0	,	3	,	7	,	8	,	11	,	14	}},
	{"m#5",			{	0	,	3	,	8}},
	{"#5",			{	0	,	3	,	8	,	10	,	14}},
	{"m6",			{	0	,	3	,	5	,	7	,	21}},
	{"m69",			{	0	,	3	,	7	,	9	,	14}},
	{"mb6M7",		{	0	,	3	,	8	,	11}},
	{"mb6b9",		{	0	,	3	,	8	,	13}},
	{"m9",			{	0	,	3	,	7	,	10	,	14}},
	{"m9b5",		{	0	,	3	,	10	,	14	,	18}},
	{"mM9",			{	0	,	3	,	7	,	11	,	14}},
	{"m11",			{	0	,	3	,	7	,	10	,	14	,	17	}},
	{"m11A5",		{	0	,	3	,	8	,	10	,	14	,	17	}},
	{"m11b5",		{	0	,	3	,	10	,	14	,	17	,	18	}},
	{"augadd#9",	{	0	,	4	,	8	,	15}},
	{"madd4",		{	0	,	3	,	5	,	7}},
	{"madd9",		{	0	,	3	,	7	,	14}},
};

InversionDefinition defaultChord {0, {0, 4, 7, 0, 4, 7}, "M"};

std::string noteNames[12] = {
	"C",
	"C#/Db",
	"D",
	"D#/Eb",
	"E",
	"F",
	"F#/Gb",
	"G",
	"G#/Ab",
	"A",
	"A#/Bb",
	"B",
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

std::string intervalNames[13] {
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

std::string modeNames[7] {
	"Ionian (M)",
	"Dorian",
	"Phrygian",
	"Lydian",
	"Mixolydian",
	"Aeolian (m)",
	"Locrian"
};

std::string degreeNames[21] { // Degree * 3 + Quality
	"I",
	"i",
	"i°",
	"II",
	"ii",
	"ii°",
	"III",
	"iii",
	"iii°",
	"IV",
	"iv",
	"iv°",
	"V",
	"v",
	"v°",
	"VI",
	"vi",
	"vi°",
	"VII",
	"vii",
	"vii°"
};
	
std::string inversionNames[3] {
	"(R)",
	"(1)",
	"(2)"
};

std::string qualityNames[3] {
	"Maj",
	"Min",
	"Dim"
};

std::string NoteDegreeModeNames[12][7][7] = { // Note, Degree, Mode
{{"C","C","C","C","C","C","C"},
{"D","D ","Db","D","D","D","Db"},
{"E","Eb","Eb","E","E","Eb","Eb"},
{"F","F","F","F#","F","F","F"},
{"G","G","G","G","G","G","Gb"},
{"A","A","Ab","A","A","Ab","Ab"},
{"B","Bb","Bb","B","Bb","Bb","Bb"}},
														
{{"C#","C#","C#","Db","C#","C#","C#"},
{"D#","D#","D","Eb","D#","D#","D"},
{"E#","E","E","F","E#","E","E"},
{"F#","F#","F#","G","F#","F#","F#"},
{"G#","G#","G#","Ab","G#","G#","G"},
{"A#","A#","A","Bb","A#","A","A"},
{"B#","B","B","C","B","B","B"}},
														
{{"D","D","D","D","D","D","D"},
{"E","E","Eb","E","E","E","Eb"},
{"F#","F","F","F#","F#","F","F"},
{"G","G","G","G#","G","G","G"},
{"A","A","A","A","A","A","Ab"},
{"B","B","Bb","B","B","Bb","Bb"},
{"C#","C","C","C#","C","C","C"}},
														
{{"Eb","Eb","D#","Eb","Eb","Eb","D#"},
{"F","F","E","F","F","F","E"},
{"G","Gb","F#","G","G","Gb","F#"},
{"Ab","Ab","G#","A","Ab","Ab","G#"},
{"Bb","Bb","A#","Bb","Bb","Bb","A"},
{"C","C","B","C","C","Cb","B"},
{"D","Db","C#","D","Db","Db","C#"}},
														
{{"E","E","E","E","E","E","E"},
{"F#","F#","F","F#","F#","F#","F"},
{"G#","G","G","G#","G#","G","G"},
{"A","A","A","A#","A","A","A"},
{"B","B","B","B","B","B","Bb"},
{"C#","C#","C","C#","C#","C","C"},
{"D#","D","D","D#","D","D","D"}},
														
{{"F","F","F","F","F","F","F"},
{"G","G","Gb","G","G","G","Gb"},
{"A","Ab","Ab","A","A","Ab","Ab"},
{"Bb","Bb","Bb","B","Bb","Bb","Bb"},
{"C","C","C","C","C","C","Cb"},
{"D","D","Db","D","D","Db","Db"},
{"E","Eb","Eb","E","Eb","Eb","Eb"}},
														
{{"F#","F#","F#","F#","F#","F#","F#"},
{"G#","G#","G","G#","G#","G#","G"},
{"A#","A","A","A#","A#","A","A"},
{"B","B","B","B#","B","B","B"},
{"C#","C#","C#","C#","C#","C#","C"},
{"D#","D#","D","D#","D#","D","D"},
{"E#","E","E","E#","E","E","E"}},
														
{{"G","G","G","G","G","G","G"},
{"A","A","Ab","A","A","A","Ab"},
{"B","Bb","Bb","B","B","Bb","Bb"},
{"C","C","C","C#","C","C","C"},
{"D","D","D","D","D","D","Db"},
{"E","E","Eb","E","E","Eb","Eb"},
{"F#","F","F","F#","F","F","F"}},
														
{{"Ab","G#","G#","Ab","Ab","G#","G#"},
{"Bb","A#","A","Bb","Bb","A#","A"},
{"C","B","B","C","C","B","B"},
{"Db","C#","C#","D","Db","C#","C#"},
{"Eb","D#","D#","Eb","Eb","D#","D"},
{"F","E#","E","F","F","E","E"},
{"G","F#","F#","G","Gb","F#","F#"}},
														
{{"A","A","A","A","A","A","A"},
{"B","B","Bb","B","B","B","Bb"},
{"C#","C","C","C#","C#","C","C"},
{"D","D","D","D#","D","D","D"},
{"E","E","E","E","E","E","Eb"},
{"F#","F#","F","F#","F#","F","F"},
{"G#","G","G","G#","G","G","G"}},
														
{{"Bb","Bb","Bb","Bb","Bb","Bb","A#"},
{"C","C","Cb","C","C","C","B"},
{"D","Db","Db","D","D","Db","C#"},
{"Eb","Eb","Eb","E","Eb","Eb","D#"},
{"F","F","F","F","F","F","E"},
{"G","G","Gb","G","G","Gb","F#"},
{"A","Ab","Ab","A","Ab","Ab","G#"}},
														
{{"B","B","B","B","B","B","B"},
{"C#","C#","C","C#","C#","C#","C"},
{"D#","D","D","D#","D#","D","D"},
{"E","E","E","E#","E","E","E"},
{"F#","F#","F#","F#","F#","F#","F"},
{"G#","G#","G","G#","G#","G","G"},
{"A#","A","A","A#","A","A","A"}}};

int ModeQuality[7][7] {
	{MAJ,MIN,MIN,MAJ,MAJ,MIN,DIM}, // Ionian
	{MIN,MIN,MAJ,MAJ,MIN,DIM,MAJ}, // Dorian
	{MIN,MAJ,MAJ,MIN,DIM,MAJ,MIN}, // Phrygian
	{MAJ,MAJ,MIN,DIM,MAJ,MIN,MIN}, // Lydian
	{MAJ,MIN,DIM,MAJ,MIN,MIN,MAJ}, // Mixolydian
	{MIN,DIM,MAJ,MIN,MIN,MAJ,MAJ}, // Aeolian
	{DIM,MAJ,MIN,MIN,MAJ,MAJ,MIN}  // Locrian
};

int ModeOffset[7][7] {
	{0,0,0,0,0,0,0},     // Ionian
	{0,0,-1,0,0,0,-1},  // Dorian
	{0,-1,-1,0,0,-1,-1}, // Phrygian
	{0,0,0,1,0,0,0},     // Lydian
	{0,0,0,0,0,0,-1},    // Mixolydian
	{0,0,-1,0,0,-1,-1},  // Aeolian
	{0,-1,-1,0,-1,-1,-1} // Locrian
};

//0	1	2	3	4	5	6	7	8	9	10	11	12
int tonicIndex[13] {1, 3, 5, 0, 2, 4, 6, 1, 3, 5, 0, 2, 4};
int scaleIndex[7] {5, 3, 1, 6, 4, 2, 0};
int noteIndex[13] { 
	NOTE_G_FLAT,
	NOTE_D_FLAT,
	NOTE_A_FLAT,
	NOTE_E_FLAT,
	NOTE_B_FLAT,
	NOTE_F,
	NOTE_C,
	NOTE_G,
	NOTE_D,
	NOTE_A,
	NOTE_E,		
	NOTE_B,
	NOTE_G_FLAT};

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
int ASCALE_CHROMATIC      [13]= {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; 	// All of the notes
int ASCALE_IONIAN         [8] = {0, 2, 4, 5, 7, 9, 11, 12};			// 1,2,3,4,5,6,7
int ASCALE_DORIAN         [8] = {0, 2, 3, 5, 7, 9, 10, 12};			// 1,2,b3,4,5,6,b7
int ASCALE_PHRYGIAN       [8] = {0, 1, 3, 5, 7, 8, 10, 12};			// 1,b2,b3,4,5,b6,b7
int ASCALE_LYDIAN         [8] = {0, 2, 4, 6, 7, 9, 10, 12};			// 1,2,3,#4,5,6,7
int ASCALE_MIXOLYDIAN     [8] = {0, 2, 4, 5, 7, 9, 10, 12};			// 1,2,3,4,5,6,b7 
int ASCALE_AEOLIAN        [8] = {0, 2, 3, 5, 7, 8, 10, 12};			// 1,2,b3,4,5,b6,b7
int ASCALE_LOCRIAN        [8] = {0, 1, 3, 5, 6, 8, 10, 12};			// 1,b2,b3,4,b5,b6,b7
int ASCALE_MAJOR_PENTA    [6] = {0, 2, 4, 7, 9, 12};				// 1,2,3,5,6
int ASCALE_MINOR_PENTA    [6] = {0, 3, 5, 7, 10, 12};				// 1,b3,4,5,b7
int ASCALE_HARMONIC_MINOR [8] = {0, 2, 3, 5, 7, 8, 11, 12};			// 1,2,b3,4,5,b6,7
int ASCALE_BLUES          [7] = {0, 3, 5, 6, 7, 10, 12};			// 1,b3,4,b5,5,b7

/*
* Convert a root note (relative to C, C=0) and positive semi-tone offset from that root to a voltage (1V/OCT, 0V = C4 (or 3??))
*/
float getVoltsFromPitch(int inRoot, int inNote){	
	return (inRoot + inNote) * music::SEMITONE;
}

float getVoltsFromScale(int scale) {
	return rack::math::rescale(scale, 0.0f, music::NUM_SCALES - 1, 0.0f, 10.0f);
}

int getScaleFromVolts(float volts) {
	float v = rack::math::clamp(fabs(volts), 0.0, 10.0f);
	return round(rack::math::rescale(v, 0.0f, 10.0f, 0.0f, NUM_SCALES - 1));
}

int getModeFromVolts(float volts) {
	float v = rack::math::clamp(fabs(volts), 0.0, 10.0f);
	int mode = round(rack::math::rescale(v, 0.0f, 10.0f, 0.0f, NUM_SCALES - 1));
	return rack::math::clamp(mode - 1, 0, 6);
}

float getVoltsFromMode(int mode) {
	// Mode 0 = IONIAN, MODE 6 = LOCRIAN -> Scale 1 - 7
	return rack::math::rescale(mode + 1, 0.0f, NUM_SCALES - 1, 0.0f, 10.0f);
}

float getVoltsFromKey(int key) {
	return rack::math::rescale(key, 0.0f, NUM_NOTES - 1, 0.0f, 10.0f);
}

int getKeyFromVolts(float volts) {
	float v = rack::math::clamp(fabs(volts), 0.0, 10.0f);
	return round(rack::math::rescale(v, 0.0f, 10.0f, 0.0f, NUM_NOTES - 1));
}

float getPitchFromVolts(float inVolts, int currRoot, int currScale, int *outNote, int *outDegree) {
	
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

	// if (debug && stepX % poll == 0) {
	// 	std::cout << "QUANT Octave: " << octave << " Scale: " << scaleNames[currScale] << " Root: " << noteNames[currRoot] << std::endl;
	// }

	float octaveOffset = 0;
	if (currRoot != 0) {
		octaveOffset = (12 - currRoot) / 12.0;
	}
	
	// if (debug && stepX % poll == 0) {
	// 	std::cout << "QUANT Octave: " << octave << " currRoot: " << currRoot << " -> Offset: " << octaveOffset << " inVolts: " << inVolts << std::endl;
	// }
		
	float fOctave = (float)octave - octaveOffset;	
	int scaleIndex = 0;
	int searchOctave = 0;
	
	do { 

		int degree = curScaleArr[scaleIndex]; // 0 - 11!
		float fVoltsAboveOctave = searchOctave + degree / 12.0;
		float fScaleNoteInVolts = fOctave + fVoltsAboveOctave;
		float distAway = fabs(inVolts - fScaleNoteInVolts);

		// if (debug && stepX % poll == 0) {
		// 	std::cout << "QUANT input: " << inVolts
		// 	<< " index: " << scaleIndex
		// 	<< " root: " << currRoot
		// 	<< " octave: " << fOctave
		// 	<< " degree: " << degree
		// 	<< " V above O: " << fVoltsAboveOctave
		// 	<< " note in V: " << fScaleNoteInVolts
		// 	<< " distance: " << distAway
		// 	<< std::endl;
		// }

		// Assume that the list of notes is ordered, so there is an single inflection point at the minimum value
		if (distAway >= closestDist){
			break;
		} else {
			// Let's remember this
			closestVal = fScaleNoteInVolts;
			closestDist = distAway;
		}
		
		scaleIndex++;
		
		if (scaleIndex == notesInScale - 1) {
			scaleIndex = 0;
			searchOctave++;
		}

	} while (true);

	if (outNote != NULL && outDegree != NULL) {

		if(scaleIndex == 0) {
			noteFound = notesInScale - 2; // NIS is a count, not index
		} else {
			noteFound = scaleIndex - 1;
		}

		// if (debug && stepX % poll == 0) {
		// 	std::cout << "QUANT NIS: " << notesInScale <<  " scaleIndex: " << scaleIndex << " NF: " << noteFound << std::endl;
		// }
		
		int currNote = (currRoot + curScaleArr[noteFound]) % 12; // So this is the nth note of the scale; 
		// case in point, V=0, Scale = F#m returns the 6th note, which should be C#

		// if (debug && stepX % poll == 0) {
		// 	// Dump the note and degree, mod the size in case where we have wrapped round

		// 	std::cout << "QUANT Found index in scale: " << noteFound << ", currNote: "  << currNote;
		// 	std::cout << " This is scale note: "  << curScaleArr[noteFound] << " (Interval: " << intervalNames[curScaleArr[noteFound]] << ")";
		// 	std::cout << ": " << inVolts << " -> " << closestVal << std::endl;

		// }

		*outNote = currNote;
		*outDegree = curScaleArr[noteFound];
	}

	return closestVal;
 
}

void getRootFromMode(int inMode, int inRoot, int inTonic, int *currRoot, int *quality) {
	
	*quality = ModeQuality[inMode][inTonic];

	int positionRelativeToStartOfScale = tonicIndex[inMode + inTonic];
	int positionStartOfScale = scaleIndex[inMode];
	
	int root = inRoot + noteIndex[positionStartOfScale + positionRelativeToStartOfScale]; 
	*currRoot = eucMod(root, 12);

	// Quantizer q;
	//
	// std::cout << "Mode: " << inMode
	// 	<< " Root: " << q.noteNames[inRoot]
	// 	<< " Tonic: " << q.tonicNames[inTonic]
	// 	<< " Scale Pos: " << positionStartOfScale
	// 	<< " Rel Pos: " <<  positionRelativeToStartOfScale
	// 	<< " Note Index: " << positionStartOfScale + positionRelativeToStartOfScale
	// 	<< " Note: " << noteIndex[positionStartOfScale + positionRelativeToStartOfScale]
	// 	<< " Offset: " << ModeOffset[inMode][inTonic]
	// 	<< " Output: " << *currRoot
	// 	<< " " << q.noteNames[*currRoot]
	// 	<< std::endl;
}
 

std::string InversionDefinition::getName(int rootNote) {
	if (inversion > 0) { 
		int bassNote = (rootNote + formula[0]) % 12;
		return music::noteNames[rootNote] + baseName + "/" + music::noteNames[bassNote];
	} else {
		return music::noteNames[rootNote] + baseName;
	}
}

std::string InversionDefinition::getName(int mode, int key, int degree, int root) {
	if (inversion > 0) { 
		int bassNote = (root + formula[0]) % 12;
		return music::NoteDegreeModeNames[key][degree][mode] + baseName + "/" + music::noteNames[bassNote];
	} else {
		return music::NoteDegreeModeNames[key][degree][mode] + baseName;
	}
}

void ChordDefinition::generateInversions() {
	for(size_t i = 0; i < formula.size(); i++) {
		// i is number of inversions
		InversionDefinition inv;
		inv.inversion = i;
		inv.baseName = name;

		calculateInversion(formula, inv.formula, i);
		inversions.push_back(inv);
	}
}

void ChordDefinition::calculateInversion(std::vector<int> &inputF, std::vector<int> &outputF, int inv) {
	outputF = inputF;
	for (int i = 0; i < inv; i++) {
		outputF[i] += 12; 
	}
	std::sort(outputF.begin(), outputF.end());

	// Fill in missing notes
	size_t nNotes = outputF.size();
	for (size_t j = 0; j < (6 - nNotes); j++) {
		outputF.push_back(-24 + outputF[j]); 
	}
}

KnownChords::KnownChords() {
	for(size_t i = 0; i < BasicChordSet.size(); i++) {
		ChordDefinition def;
		def.id = i;
		def.name = BasicChordSet[i].name;
		def.formula = BasicChordSet[i].root;
		def.generateInversions();
		chords.push_back(def);
	}
}

void KnownChords::dump() {
	for(ChordDefinition chord: chords) {
		std::cout << chord.id << " = " << chord.name << std::endl;
		for(InversionDefinition inv: chord.inversions) {
			std::stringstream ss;
			for(size_t i = 0; i < inv.formula.size(); i++) {
				if(i != 0) {
				    ss << ",";
				}
  				ss << inv.formula[i];
			}
			std::cout << inv.inversion << "(" << inv.formula.size() <<  ") = " << ss.str() << std::endl;
		}
	}
}

}

}