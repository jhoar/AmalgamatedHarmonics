#include "ProgressState.hpp"

// ProgressState
ProgressState::ProgressState() {
	onReset();
}

void ProgressState::onReset() {
	for (int part = 0; part < 32; part++) {
		for (int step = 0; step < 8; step++) {
			parts[part][step].reset();
		}
	}
	stateChanged = true;
}

void ProgressState::calculateVoltages(int part, int step) {
	int chordIndex = parts[part][step].chord;
	int invIndex = parts[part][step].inversion;

	music::ChordDefinition &chordDef = knownChords.chords[chordIndex];
	std::vector<int> &invDef = chordDef.inversions[invIndex].formula;
	parts[part][step].setVoltages(invDef, offset);
}

void ProgressState::update() {

	for (int step = 0; step < 8; step++) {
		if (modeChanged || stateChanged || parts[currentPart][step].dirty) {
			switch(chordMode) {
				case ChordMode::NORMAL:
					parts[currentPart][step].rootNote = parts[currentPart][step].note;
					break;
				case ChordMode::MODE:
					music::getRootFromMode(mode, key, 
					parts[currentPart][step].modeDegree,
					&(parts[currentPart][step].rootNote),
					&(parts[currentPart][step].quality));
					break;
				case ChordMode::COERCE:
					music::getRootFromMode(mode, key, 
					parts[currentPart][step].modeDegree,
					&(parts[currentPart][step].rootNote),
					&(parts[currentPart][step].quality));

					// Force chord
					switch(parts[currentPart][step].quality) {
						case music::Quality::MAJ:
							parts[currentPart][step].chord = 0;
							break;
						case music::Quality::MIN:
							parts[currentPart][step].chord = 1;
							break;
						case music::Quality::DIM:
							parts[currentPart][step].chord = 54;
							break;
					}
			}

			calculateVoltages(currentPart,step);
		}
		parts[currentPart][step].dirty = false;
	}
	stateChanged = false;
	modeChanged = false;
}

void ProgressState::copyPartFrom(int src) {
	if (src == currentPart) {
		return;
	}

	for (int step = 0; step < 8; step++) {
		parts[currentPart][step] = parts[src][step];
	}

	stateChanged = true;
}

void ProgressState::toggleGate(int part, int step) {
	parts[part][step].gate = !parts[part][step].gate;
}

bool ProgressState::gateState(int part, int step) {
	return parts[part][step].gate;
}

float *ProgressState::getChordVoltages(int part, int step) {
	return parts[part][step].outVolts;
}

ProgressChord *ProgressState::getChord(int part, int step) {
	return &(parts[part][step]);
}

void ProgressState::setMode(int m) {
	if (mode != m) {
		mode = m;
		stateChanged = true;
	}
}

void ProgressState::setKey(int k) {
	if (key != k) {
		key = k;
		stateChanged = true;
	}
}

void ProgressState::setPart(int p) {
	if (currentPart != p) {
		currentPart = p;
		stateChanged = true;
	}
}

json_t *ProgressState::toJson() {
	json_t *rootJ = json_object();

	// pChord
	json_t *rootNote_array		= json_array();
	json_t *note_array			= json_array();
	json_t *quality_array		= json_array();
	json_t *chord_array			= json_array();
	json_t *modeDegree_array	= json_array();
	json_t *inversion_array		= json_array();
	json_t *octave_array		= json_array();
	json_t *gate_array			= json_array();

	for (int part = 0; part < 32; part++) {
		for (int step = 0; step < 8; step++) {
			json_t *rootNoteJ	= json_integer(parts[part][step].rootNote);
			json_t *noteJ		= json_integer(parts[part][step].note);
			json_t *qualityJ	= json_integer(parts[part][step].quality);
			json_t *chordJ		= json_integer(parts[part][step].chord);
			json_t *modeDegreeJ	= json_integer(parts[part][step].modeDegree);
			json_t *inversionJ	= json_integer(parts[part][step].inversion);
			json_t *octaveJ		= json_integer(parts[part][step].octave);
			json_t *gateJ		= json_boolean(parts[part][step].gate);

			json_array_append_new(rootNote_array,	rootNoteJ);
			json_array_append_new(note_array,		noteJ);
			json_array_append_new(quality_array,	qualityJ);
			json_array_append_new(chord_array,		chordJ);
			json_array_append_new(modeDegree_array,	modeDegreeJ);
			json_array_append_new(inversion_array,	inversionJ);
			json_array_append_new(octave_array,		octaveJ);
			json_array_append_new(gate_array,		gateJ);
		}
	}

	json_object_set_new(rootJ, "rootnote",		rootNote_array);
	json_object_set_new(rootJ, "note",			note_array);
	json_object_set_new(rootJ, "quality",		quality_array);
	json_object_set_new(rootJ, "chord",			chord_array);
	json_object_set_new(rootJ, "modedegree",	modeDegree_array);
	json_object_set_new(rootJ, "inversion",		inversion_array);
	json_object_set_new(rootJ, "octave",		octave_array);
	json_object_set_new(rootJ, "gate",			gate_array);

	// offset
	json_t *offsetJ = json_integer((int) offset);
	json_object_set_new(rootJ, "offset", offsetJ);

	// chordMode
	json_t *chordModeJ = json_integer((int) chordMode);
	json_object_set_new(rootJ, "chordMode", chordModeJ);

	return rootJ;
}

void ProgressState::fromJson(json_t *rootJ) {

	// rootNote
	json_t *rootNote_array = json_object_get(rootJ, "rootnote");
	if (rootNote_array) {
		for (int part = 0; part < 32; part++) {
			for (int step = 0; step < 8; step++) {
				json_t *rootNoteJ = json_array_get(rootNote_array, part * 8 + step);
				if (rootNoteJ) parts[part][step].rootNote = json_integer_value(rootNoteJ);
			}
		}
	}

	// rootNote
	json_t *note_array = json_object_get(rootJ, "note");
	if (note_array) {
		for (int part = 0; part < 32; part++) {
			for (int step = 0; step < 8; step++) {
				json_t *noteJ = json_array_get(note_array, part * 8 + step);
				if (noteJ) parts[part][step].note = json_integer_value(noteJ);
			}
		}
	}

	// quality
	json_t *quality_array = json_object_get(rootJ, "quality");
	if (quality_array) {
		for (int part = 0; part < 32; part++) {
			for (int step = 0; step < 8; step++) {
				json_t *qualityJ = json_array_get(quality_array, part * 8 + step);
				if (qualityJ) parts[part][step].quality = json_integer_value(qualityJ);
			}
		}
	}

	// chord
	json_t *chord_array = json_object_get(rootJ, "chord");
	if (chord_array) {
		for (int part = 0; part < 32; part++) {
			for (int step = 0; step < 8; step++) {
				json_t *chordJ = json_array_get(chord_array, part * 8 + step);
				if (chordJ)	parts[part][step].chord = json_integer_value(chordJ);
			}
		}
	}

	// modeDegree
	json_t *modeDegree_array = json_object_get(rootJ, "modedegree");
	if (modeDegree_array) {
		for (int part = 0; part < 32; part++) {
			for (int step = 0; step < 8; step++) {
				json_t *modeDegreeJ = json_array_get(modeDegree_array, part * 8 + step);
				if (modeDegreeJ) parts[part][step].modeDegree = json_integer_value(modeDegreeJ);
			}
		}
	}

	// inversion
	json_t *inversion_array = json_object_get(rootJ, "inversion");
	if (inversion_array) {
		for (int part = 0; part < 32; part++) {
			for (int step = 0; step < 8; step++) {
				json_t *inversionJ = json_array_get(inversion_array, part * 8 + step);
				if (inversionJ)	parts[part][step].inversion = json_integer_value(inversionJ);
			}
		}
	}

	// octave
	json_t *octave_array = json_object_get(rootJ, "octave");
	if (octave_array) {
		for (int part = 0; part < 32; part++) {
			for (int step = 0; step < 8; step++) {
				json_t *octaveJ = json_array_get(octave_array, part * 8 + step);
				if (octaveJ) parts[part][step].octave = json_integer_value(octaveJ);
			}
		}
	}

	// gates
	json_t *gate_array = json_object_get(rootJ, "gate");
	if (gate_array) {
		for (int part = 0; part < 32; part++) {
			for (int step = 0; step < 8; step++) {
				json_t *gateJ = json_array_get(gate_array, part * 8 + step);
				if (gateJ) parts[part][step].gate = json_boolean_value(gateJ);
			}
		}
	}

	// offset
	json_t *offsetJ = json_object_get(rootJ, "offset");
	if (offsetJ) offset = json_integer_value(offsetJ);

	// chordMode
	json_t *chordModeJ = json_object_get(rootJ, "chordMode");
	if (chordModeJ) chordMode = (ChordMode)json_integer_value(chordModeJ);

}

// ProgressState

// Root menu
void RootItem::onAction(const rack::event::Action &e) {
	pChord->note = root;
	pChord->dirty = true;
}

void RootChoice::onAction(const rack::event::Action &e) {
	if (!pState)
		return;

	ProgressChord *pChord = pState->getChord(pState->currentPart, pStep);

	ui::Menu *menu = createMenu();
	menu->addChild(createMenuLabel("Root Note"));
	for (int i = 0; i < music::Notes::NUM_NOTES; i++) {
		RootItem *item = new RootItem;
		item->pChord = pChord;
		item->root = i;
		item->text = music::noteNames[i];
		menu->addChild(item);
	}
}

void RootChoice::step() {
	if (!pState) {
		text = "";
		return;
	}

	ProgressChord *pC = pState->getChord(pState->currentPart, pStep);
	
	if(!pState->chordMode && pState->nSteps > pStep) {
		color = nvgRGBA(0x00, 0xFF, 0xFF, 0xFF);
	} else {
		color = nvgRGBA(0x00, 0xFF, 0xFF, 0x6F);
	}

	text = std::string("◊ ") + music::noteNames[pC->note];
	
}
// Root 

// Degree
void DegreeItem::onAction(const rack::event::Action &e) {
	pChord->modeDegree = degree;
	pChord->dirty = true;
}

void DegreeChoice::onAction(const rack::event::Action &e) {
		if (!pState)
		return;

	ProgressChord *pChord = pState->getChord(pState->currentPart, pStep);

	ui::Menu *menu = createMenu();
	menu->addChild(createMenuLabel("Degree"));
	for (int i = 0; i < music::Degrees::NUM_DEGREES; i++) {
		DegreeItem *item = new DegreeItem;
		item->pState = pState;
		item->pChord = pChord;
		item->degree = i;
		item->text = music::DegreeString[pState->mode][i];
		menu->addChild(item);
	}
}

void DegreeChoice::step() {
	if (!pState) {
		text = "";
		return;
	}

	ProgressChord *pC = pState->getChord(pState->currentPart, pStep);

	if(pState->chordMode && pState->nSteps > pStep) {
		color = nvgRGBA(0x00, 0xFF, 0xFF, 0xFF);
	} else {
		color = nvgRGBA(0x00, 0xFF, 0xFF, 0x6F);
	}

	text = std::string("◊ ") + music::DegreeString[pState->mode][pC->modeDegree];

}
// Degree

// Chord 
void ChordItem::onAction(const rack::event::Action &e)  {
	pChord->chord = chord;
	pChord->dirty = true;
}

Menu *ChordSubsetMenu::createChildMenu() {

	ProgressChord *pChord = pState->getChord(pState->currentPart, pStep);

	Menu *menu = new Menu;
	for (int i = start; i <= end; i++) {
		ChordItem *item = new ChordItem;
		item->pChord = pChord;
		item->chord = i;
		item->text = music::BasicChordSet[i].name;
		menu->addChild(item);
	}
	return menu;
}

void ChordChoice::onAction(const rack::event::Action &e) {
	if (!pState)
		return;

	size_t maxChords = music::BasicChordSet.size();

	ui::Menu *menu = createMenu();
	menu->addChild(createMenuLabel("Chord"));
	for(size_t i = 0; i < maxChords; i += 9) {

		int endNum = std::min(i + 9, maxChords - 1);

		std::string startName = music::BasicChordSet[i].name;
		std::string endName = music::BasicChordSet[endNum].name;

		ChordSubsetMenu *item = createMenuItem<ChordSubsetMenu>(startName + " - " + endName);
		item->pState = pState;
		item->pStep = pStep;
		item->start = i;
		item->end = endNum;
		menu->addChild(item);

	}

}

void ChordChoice::step() {
	if (!pState) {
		text = "";
		return;
	}

	ProgressChord *pC = pState->getChord(pState->currentPart, pStep);
	music::InversionDefinition &inv = pState->knownChords.chords[pC->chord].inversions[pC->inversion];

	if(pState->nSteps > pStep) {
		color = nvgRGBA(0x00, 0xFF, 0xFF, 0xFF);
	} else {
		color = nvgRGBA(0x00, 0xFF, 0xFF, 0x6F);
	}

	text = std::to_string(pStep + 1) + std::string(": ◊ ");

	if (pState->chordMode) {
		text += inv.getName(pState->mode, pState->key, pC->modeDegree, pC->rootNote);
	} else {
		text += inv.getName(pC->rootNote);
	}

}

// Chord menu

// Octave
void OctaveItem::onAction(const rack::event::Action &e) {
	pChord->octave = octave;
	pChord->dirty = true;
}

void OctaveChoice::onAction(const rack::event::Action &e) {
	if (!pState)
		return;

	ProgressChord *pChord = pState->getChord(pState->currentPart, pStep);

	ui::Menu *menu = createMenu();
	menu->addChild(createMenuLabel("Octave"));
	for (int i = -5; i < 6; i++) {
		OctaveItem *item = new OctaveItem;
		item->pChord = pChord;
		item->octave = i;
		item->text = std::to_string(i);
		menu->addChild(item);
	}
}

void OctaveChoice::step() {
	if (!pState) {
		text = "";
		return;
	}

	if(pState->nSteps > pStep) {
		color = nvgRGBA(0x00, 0xFF, 0xFF, 0xFF);
	} else {
		color = nvgRGBA(0x00, 0xFF, 0xFF, 0x6F);
	}

	ProgressChord *pChord = pState->getChord(pState->currentPart, pStep);

	text = std::string("◊ ") + std::to_string(pChord->octave);

}
// Octave 

// Inversion 
void InversionItem::onAction(const rack::event::Action &e) {
	pChord->inversion = inversion;
	pChord->dirty = true;
}

void InversionChoice::onAction(const rack::event::Action &e) {
	if (!pState)
		return;

	ProgressChord *pChord = pState->getChord(pState->currentPart, pStep);

	ui::Menu *menu = createMenu();
	menu->addChild(createMenuLabel("Inversion"));
	for (int i = 0; i < music::Inversion::NUM_INV; i++) {
		InversionItem *item = new InversionItem;
		item->pChord = pChord;
		item->inversion = i;
		item->text = music::inversionNames[i];
		menu->addChild(item);
	}
}

void InversionChoice::step() {
	if (!pState) {
		text = "";
		return;
	}

	if(pState->nSteps > pStep) {
		color = nvgRGBA(0x00, 0xFF, 0xFF, 0xFF);
	} else {
		color = nvgRGBA(0x00, 0xFF, 0xFF, 0x6F);
	}

	ProgressChord *pChord = pState->getChord(pState->currentPart, pStep);

	text = std::string("◊ ") + music::inversionNames[pChord->inversion];

}
// Inversion 

void StatusBox::step() {
	if (!pState) {
		text = "";
		return;
	}

	if(pState->chordMode) {
		color = nvgRGBA(0x00, 0xFF, 0xFF, 0xFF);
	} else {
		color = nvgRGBA(0x00, 0xFF, 0xFF, 0x6F);
	}

	text = "Part " + std::to_string(pState->currentPart) + " " + music::NoteDegreeModeNames[pState->key][0][pState->mode] + " " + music::modeNames[pState->mode];

}

// ProgressStepWidget
void ProgressStepWidget::setPState(ProgressState *pState, int pStep) {

	clearChildren();

	math::Vec pos;

	ChordChoice *chordChoice = createWidget<ChordChoice>(pos);
	chordChoice->box.size.x = 155.0;
	chordChoice->textOffset.y = 10.0;
	chordChoice->pState = pState;
	chordChoice->pStep = pStep;
	addChild(chordChoice);
	pos = chordChoice->box.getTopRight();
	this->chordChooser = chordChoice;

	RootChoice *rootChoice = createWidget<RootChoice>(pos);
	rootChoice->box.size.x = 35.0;
	rootChoice->textOffset.y = 10.0;
	rootChoice->pState = pState;
	rootChoice->pStep = pStep;
	addChild(rootChoice);
	pos = rootChoice->box.getTopRight();
	this->rootChooser = rootChoice;

	DegreeChoice *degreeChoice = createWidget<DegreeChoice>(pos);
	degreeChoice->box.size.x = 30.0;
	degreeChoice->textOffset.y = 10.0;
	degreeChoice->pState = pState;
	degreeChoice->pStep = pStep;
	addChild(degreeChoice);
	pos = degreeChoice->box.getTopRight();
	this->degreeChooser = degreeChoice;

	InversionChoice *inversionChoice = createWidget<InversionChoice>(pos);
	inversionChoice->box.size.x = 35.0;
	inversionChoice->textOffset.y = 10.0;
	inversionChoice->pState = pState;
	inversionChoice->pStep = pStep;
	addChild(inversionChoice);
	pos = inversionChoice->box.getTopRight();
	this->inversionChooser = inversionChoice;

	OctaveChoice *octaveChoice = createWidget<OctaveChoice>(pos);
	octaveChoice->box.size.x = 35.0;
	octaveChoice->textOffset.y = 10.0;
	octaveChoice->pState = pState;
	octaveChoice->pStep = pStep;
	addChild(octaveChoice);
	pos = octaveChoice->box.getTopRight();
	this->octaveChooser = octaveChoice;

}
// ProgressStepWidget

// ProgressStateWidget
void ProgressStateWidget::setPState(ProgressState *pState) {
	clearChildren();
	math::Vec pos;

	StatusBox *statusBox  = createWidget<StatusBox>(pos);
	statusBox->box.size.x = 170.0;
	statusBox->pState = pState;
	addChild(statusBox);
	pos = statusBox->box.getBottomLeft();

	for (int i = 0; i < 8; i++) {
		ProgressStepWidget *pWidget = createWidget<ProgressStepWidget>(pos);
		pWidget->box.size.x = box.size.x - 5;
		pWidget->box.size.y = box.size.y / 9.0;
		pWidget->setPState(pState, i);
		addChild(pWidget);
		pos = pWidget->box.getBottomLeft();
		this->stepConfig[i] = pWidget;
	}
}
// ProgressStateWidget

