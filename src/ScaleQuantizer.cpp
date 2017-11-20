#include "AH.hpp"

#include <iostream>

struct ScaleQuantizer : Module {

	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		IN_INPUT,
		KEY_INPUT,
		SCALE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		GATE_OUTPUT,
		NUM_OUTPUTS = GATE_OUTPUT + 12
	};
	enum LightIds {
		NOTE_LIGHT,
                KEY_LIGHT = NOTE_LIGHT + 12,
		SCALE_LIGHT = KEY_LIGHT + 12,
		DEGREE_LIGHT = SCALE_LIGHT + 12,
		NUM_LIGHTS = DEGREE_LIGHT + 12
	};

	// Reference, midi note to scale
	// 0	1
	// 1	b2 (#1)
	// 2	2
	// 3	b3 (#2)
	// 4	3
	// 5	4
	// 6	#4 (b5)
	// 7	5
	// 8	b6 (#5)
	// 9 	6
	// 10	b7 (#6)
	// 11	7

	//copied from http://www.grantmuller.com/MidiReference/doc/midiReference/ScaleReference.html
        int SCALE_CHROMATIC      [13]= {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; 	// All of the notes
	int SCALE_IONIAN         [8] = {0, 2, 4, 5, 7, 9, 11, 12};			// 1,2,3,4,5,6,7
        int SCALE_DORIAN         [8] = {0, 2, 3, 5, 7, 9, 10, 12};			// 1,2,b3,4,5,6,b7
        int SCALE_PHRYGIAN       [8] = {0, 1, 3, 5, 7, 8, 10, 12};			// 1,b2,b3,4,5,b6,b7
        int SCALE_LYDIAN         [8] = {0, 2, 4, 6, 7, 9, 10, 12};			// 1,2,3,#4,5,6,7
	int SCALE_MIXOLYDIAN     [8] = {0, 2, 4, 5, 7, 9, 10, 12};			// 1,2,3,4,5,6,b7 
        int SCALE_AEOLIAN        [8] = {0, 2, 3, 5, 7, 8, 10, 12};                  	// 1,2,b3,4,5,b6,b7
        int SCALE_LOCRIAN        [8] = {0, 1, 3, 5, 6, 8, 10, 12};			// 1,b2,b3,4,b5,b6,b7
        int SCALE_MAJOR_PENTA    [6] = {0, 2, 4, 7, 9, 12};				// 1,2,3,5,6
        int SCALE_MINOR_PENTA    [6] = {0, 3, 5, 7, 10, 12};				// 1,b3,4,5,b7
        int SCALE_HARMONIC_MINOR [8] = {0, 2, 3, 5, 7, 8, 11, 12};			// 1,2,b3,4,5,b6,7
        int SCALE_BLUES          [7] = {0, 3, 5, 6, 7, 10, 12};				// 1,b3,4,b5,5,b7

	enum Notes {
		NOTE_C,
		NOTE_D_FLAT,
		NOTE_D,
		NOTE_E_FLAT,
		NOTE_E,
		NOTE_F,
		NOTE_G_FLAT,
		NOTE_G,
		NOTE_A_FLAT,
		NOTE_A,
		NOTE_B_FLAT,
		NOTE_B,
		NUM_NOTES
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
		CHROMATIC,
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

	int currNote = 0;
	int currScale = 0;
	int currRoot = 0;
	int currDegree = 0;
	int lastScale = 0;
	int lastRoot = 0;

	bool debug = false;
	bool firstStep = true;
        int poll = 50000;
        int stepX = 0;

	ScaleQuantizer() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	float getPitchFromVolts(float inVolts, float inRoot, float inScale) {

		lastScale = currScale;
		lastRoot = currRoot;

		// get the root note and scale
		currRoot = round(rescalef(fabs(inRoot), 0, 10, 0, ScaleQuantizer::NUM_NOTES));
                currScale = round(rescalef(fabs(inScale), 0, 10, 0, ScaleQuantizer::NUM_SCALES));

		int *curScaleArr;
		int notesInScale = 0;
		switch(currScale){
			case CHROMATIC:		curScaleArr = SCALE_CHROMATIC;		notesInScale=LENGTHOF(SCALE_CHROMATIC); break;
                        case IONIAN:          	curScaleArr = SCALE_IONIAN;		notesInScale=LENGTHOF(SCALE_IONIAN); break;
                        case DORIAN:          	curScaleArr = SCALE_DORIAN;		notesInScale=LENGTHOF(SCALE_DORIAN); break;
                        case PHRYGIAN:         	curScaleArr = SCALE_PHRYGIAN;		notesInScale=LENGTHOF(SCALE_PHRYGIAN); break;
                        case LYDIAN:          	curScaleArr = SCALE_LYDIAN;		notesInScale=LENGTHOF(SCALE_LYDIAN); break;
                        case MIXOLYDIAN:	curScaleArr = SCALE_MIXOLYDIAN;		notesInScale=LENGTHOF(SCALE_MIXOLYDIAN); break;
                        case AEOLIAN:		curScaleArr = SCALE_AEOLIAN;		notesInScale=LENGTHOF(SCALE_AEOLIAN); break;
                        case LOCRIAN:		curScaleArr = SCALE_LOCRIAN;		notesInScale=LENGTHOF(SCALE_LOCRIAN); break;
                        case MAJOR_PENTA:	curScaleArr = SCALE_MAJOR_PENTA;	notesInScale=LENGTHOF(SCALE_MAJOR_PENTA); break;
                        case MINOR_PENTA:	curScaleArr = SCALE_MINOR_PENTA;	notesInScale=LENGTHOF(SCALE_MINOR_PENTA); break;
                        case HARMONIC_MINOR:	curScaleArr = SCALE_HARMONIC_MINOR;	notesInScale=LENGTHOF(SCALE_HARMONIC_MINOR); break;
                        case BLUES:		curScaleArr = SCALE_BLUES;		notesInScale=LENGTHOF(SCALE_BLUES); break;
			default: 		curScaleArr = SCALE_CHROMATIC;		notesInScale=LENGTHOF(SCALE_CHROMATIC);
		}

                // get the octave
                int octave = floor(inVolts);
                float closestVal = 10.0;
                float closestDist = 10.0;

		int noteFound = 0;
		int degreeFound = 0;

	        if (debug && stepX % poll == 0) {
       			std::cout << "Octave: " << octave << " Scale: " << scaleNames[currScale] << " Root: " << noteNames[currRoot] << std::endl;
        	}

		for (int i = 0; i < notesInScale; i++) {

			float fOctave = (float)octave;
			int iDegree = curScaleArr[i];
			float fVoltsAboveOctave = iDegree / 12.0;
			float fScaleNoteInVolts = fOctave + fVoltsAboveOctave;
	                float distAway = fabs(inVolts - fScaleNoteInVolts);

                        if (debug && stepX % poll == 0) {
                                std::cout << "input: " << inVolts 
					<< " index: " << i 
					<< " root: " << currRoot
					<< " octave: " << fOctave
					<< " degree: " << iDegree
					<< " V above O: " << fVoltsAboveOctave
					<< " note in V: " << fScaleNoteInVolts
					<< " distance: " << distAway
                                        << std::endl;
                        }

			// Assume that the list of notes is ordered, so there is an single inflection point at the minimum value 
			if(distAway > closestDist){
				noteFound = i - 1; // We break here because the previous note was closer, all subsequent notes are father away
				degreeFound = iDegree;
				break;
			} else {
				// Let's remember this
                                closestVal = fScaleNoteInVolts;
                                closestDist = distAway;
			}
		}

		// Dump the note and degree, mod the size in case where we have wrapped round
		currNote = (currRoot + curScaleArr[noteFound]) % 12;
		currDegree = degreeFound % 12;

                if (debug && stepX % poll == 0) {
                        std::cout << "Found note in scale: " << noteFound 
				<< " Scale degree: " << curScaleArr[noteFound] << " " << degreeNames[curScaleArr[noteFound]] 
				<< " Note: " << currNote << " " << noteNames[currNote] 
				<< std::endl;
                }

		return closestVal;
 
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void ScaleQuantizer::step() {

	stepX++;

	// Get the input pitch
	float volts = inputs[IN_INPUT].value;
	float root =  inputs[KEY_INPUT].value;
	float scale = inputs[SCALE_INPUT].value;

	// Calculate output pitch from raw voltage
	float pitch = getPitchFromVolts(volts, root, scale);

	// Set the value
	outputs[OUT_OUTPUT].value = pitch;

	// update tone lights
	for (int i = 0; i < ScaleQuantizer::NUM_NOTES; i++) {
		lights[NOTE_LIGHT + i].value = 0.0;
	}
	lights[NOTE_LIGHT + currNote].value = 1.0;

	// update degree lights
	for (int i = 0; i < ScaleQuantizer::NUM_NOTES; i++) {
		lights[DEGREE_LIGHT + i].value = 0.0;
		outputs[GATE_OUTPUT + i].value = 0.0;
	}
	lights[DEGREE_LIGHT + currDegree].value = 1.0;
        outputs[GATE_OUTPUT + currDegree].value = 10.0;

	if (lastScale != currScale || firstStep) {
	        for (int i = 0; i < ScaleQuantizer::NUM_NOTES; i++) {
        	        lights[SCALE_LIGHT + i].value = 0.0;
        	}
        	lights[SCALE_LIGHT + currScale].value = 1.0;
	} 

	if (lastRoot != currRoot || firstStep) {
                for (int i = 0; i < ScaleQuantizer::NUM_NOTES; i++) {
                        lights[KEY_LIGHT + i].value = 0.0;
                }
                lights[KEY_LIGHT + currRoot].value = 1.0;

	} 

	firstStep = false;

}

ScaleQuantizerWidget::ScaleQuantizerWidget() {
	ScaleQuantizer *module = new ScaleQuantizer();
	setModule(module);
	box.size = Vec(240, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/ScaleQuantizer.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

	addInput(createInput<PJ301MPort>(Vec(18, 329), module, ScaleQuantizer::IN_INPUT));
        addInput(createInput<PJ301MPort>(Vec(78, 329), module, ScaleQuantizer::KEY_INPUT));
        addInput(createInput<PJ301MPort>(Vec(138, 329), module, ScaleQuantizer::SCALE_INPUT));

	addOutput(createOutput<PJ301MPort>(Vec(198, 329), module, ScaleQuantizer::OUT_OUTPUT));

        float xOffset = 18.0;
        float xSpace = 21.0;
	float xPos = 0.0;
	float yPos = 0.0;
	int scale = 0;

        for (int i = 0; i < 12; i++) {
                addChild(createLight<SmallLight<GreenLight>>(Vec(xOffset + i * 18.0, 280.0), module, ScaleQuantizer::SCALE_LIGHT + i));
        }
 
	for (int i = 0; i < 12; i++) {
		calculateKey(i, xSpace, xOffset, 230.0, &xPos, &yPos, &scale);
		addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, ScaleQuantizer::KEY_LIGHT + scale));

                calculateKey(i, xSpace, xOffset + 72.0, 165.0, &xPos, &yPos, &scale);
                addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, ScaleQuantizer::NOTE_LIGHT + scale));

                calculateKey(i, 30.0, xOffset + 9.5, 110.0, &xPos, &yPos, &scale);
                addChild(createLight<SmallLight<GreenLight>>(Vec(xPos, yPos), module, ScaleQuantizer::DEGREE_LIGHT + scale));

                calculateKey(i, 30.0, xOffset, 85.0, &xPos, &yPos, &scale);
                addOutput(createOutput<PJ301MPort>(Vec(xPos, yPos), module, ScaleQuantizer::GATE_OUTPUT + scale));
        }

}

void ScaleQuantizerWidget::calculateKey(int inKey, float spacing, float xOff, float yOff, float *x, float *y, int *scale) {

	// Sanitise input
	int key = inKey % 12;

        // White
        int whiteO [7] = {0, 1, 2, 3, 4, 5, 6};   // 'spaces' occupied by keys
        int whiteK [7] = {0, 2, 4, 5, 7, 9, 11};  // scale mapped to key
        // Black
        int blackO [5] = {0, 1, 3, 4, 5};         // 'spaces' occupied by keys
        int blackK [5] = {1, 3, 6, 8, 10};        // scale mapped to keys

        float bOffset = xOff + (spacing / 2);
        float bSpace = spacing;
        // Make an equilateral triangle pattern, tri height = base * (sqrt(3) / 2)
        float bDelta = bSpace * sqrt(3) * 0.5;

	// Are we black or white key?, check black keys
	for(int i = 0; i < 5; i++) {
		if (blackK[i] == key) {
               		*x = bOffset + bSpace * blackO[i];
			*y = yOff - bDelta;
			*scale = blackK[i];
			return;
		}
	}

	int wOffset = xOff;
	int wSpace = spacing;
	
        for(int i = 0; i < 7; i++) {
                if (whiteK[i] == key) {
                        *x = wOffset + wSpace * whiteO[i];
                        *y = yOff;
                        *scale = whiteK[i];
                        return;
                }
        }
}

