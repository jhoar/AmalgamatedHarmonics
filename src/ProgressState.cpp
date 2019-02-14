#include "ProgressState.hpp"

// ProgressState
ProgressState::ProgressState() {
    for (int i = 0; i < 8; i++) {
        chords[i].setVoltages(music::ChordTable[1].root, offset);
    }
}

void ProgressState::setMode(int m) {
    if (mode != m) {
        mode = m;
        if(chordMode) { 
            for (int i = 0; i < 8; i++) {
                music::getRootFromMode(mode, key, chords[i].modeDegree, &(chords[i].rootNote), &(chords[i].quality));
            }
        }
        dirty = true;
    }
}

void ProgressState::setKey(int k) {
    if (key != k) {
        key = k;
        if(chordMode) { 
            for (int i = 0; i < 8; i++) {
                music::getRootFromMode(mode, key, chords[i].modeDegree, &(chords[i].rootNote), &(chords[i].quality));
            }
        }
        dirty = true;
    }
}
// ProgressState

// Root/Degree menu
void RootItem::onAction(const event::Action &e) {
    pChord->rootNote = root;
    pChord->dirty = true;
}

void DegreeItem::onAction(const event::Action &e) {
    pChord->modeDegree = degree;
    pChord->dirty = true;

    // Root and chord can get updated
    // From the input key, mode and degree, we can get the root chord note and quality (Major,Minor,Diminshed)
    music::getRootFromMode(pState->mode, pState->key, pChord->modeDegree, &(pChord->rootNote), &(pChord->quality));

}

void RootChoice::onAction(const event::Action &e) {
    if (!pState)
        return;

    ui::Menu *menu = createMenu();
    if (pState->chordMode) {
        menu->addChild(createMenuLabel("Degree"));
        for (int i = 0; i < music::NUM_DEGREES; i++) {
            DegreeItem *item = new DegreeItem;
            item->pState = pState;
            item->pChord = &(pState->chords[pStep]);
            item->degree = i;
            item->text = music::degreeNames[i * 3];
            menu->addChild(item);
        }
    } else {
        menu->addChild(createMenuLabel("Root Note"));
        for (int i = 0; i < music::NUM_NOTES; i++) {
            RootItem *item = new RootItem;
            item->pChord = &(pState->chords[pStep]);
            item->root = i;
            item->text = music::noteNames[i];
            menu->addChild(item);
        }
    }
}

void RootChoice::step() {
    if (!pState) {
        text = "";
        return;
    }

    ProgressChord &pC = pState->chords[pStep];
    music::InversionDefinition &inv = pState->knownChords.chords[pC.chord].inversions[pC.inversion];
    
    if(pState->nSteps > pStep) {
        color = nvgRGBA(0x00, 0xFF, 0xFF, 0xFF);
    } else {
        color = nvgRGBA(0xFF, 0x00, 0x00, 0xFF);
    }

    text = std::to_string(pStep + 1) + ": ";

    if (pState->chordMode) {
        text += inv.getName(pState->mode, pState->key, pC.modeDegree, pC.rootNote);
        text += " " + music::degreeNames[pC.modeDegree * 3 + pC.quality];
    } else {
        text += inv.getName(pC.rootNote);
    }
}
// Root/Degree menu

// Chord menu
void ChordItem::onAction(const event::Action &e)  {
    pChord->chord = chord;
    pChord->dirty = true;
}

struct ChordSubsetMenu : MenuItem {
	ProgressState *pState;
	int pStep;
    int start;
    int end;
    Menu *createChildMenu() override {
        Menu *menu = new Menu;
        for (int i = start; i < end; i++) {
            ChordItem *item = new ChordItem;
            item->pChord = &(pState->chords[pStep]);
            item->chord = i;
            item->text = music::ChordTable[i].name;
            menu->addChild(item);
        }
        return menu;
    }
};

void ChordChoice::onAction(const event::Action &e) {
    if (!pState)
        return;

    ui::Menu *menu = createMenu();
    ChordSubsetMenu *majorItem = createMenuItem<ChordSubsetMenu>("Major");
    majorItem->pState = pState;
    majorItem->pStep = pStep;
    majorItem->start = 1;
    majorItem->end = 70;
    menu->addChild(majorItem);

    ChordSubsetMenu *minorItem = createMenuItem<ChordSubsetMenu>("Minor");
    minorItem->pState = pState;
    minorItem->pStep = pStep;
    minorItem->start = 71;
    minorItem->end = 90;
    menu->addChild(minorItem);

    ChordSubsetMenu *otherItem = createMenuItem<ChordSubsetMenu>("Other");
    otherItem->pState = pState;
    otherItem->pStep = pStep;
    otherItem->start = 91;
    otherItem->end = 99;
    menu->addChild(otherItem);

}

void ChordChoice::step() {
    if (!pState) {
        text = "";
        return;
    }

    if(pState->nSteps > pStep) {
        color = nvgRGBA(0x00, 0xFF, 0xFF, 0xFF);
    } else {
        color = nvgRGBA(0xFF, 0x00, 0x00, 0xFF);
    }

    text = music::ChordTable[pState->chords[pStep].chord].name;

}
// Chord menu

// Inversion menu
void InversionItem::onAction(const event::Action &e) {
    pChord->inversion = inversion;
    pChord->dirty = true;
}

void InversionChoice::onAction(const event::Action &e) {
    if (!pState)
        return;

    ui::Menu *menu = createMenu();
    menu->addChild(createMenuLabel("Inversion"));
    for (int i = 0; i < music::NUM_INV; i++) {
        InversionItem *item = new InversionItem;
        item->pChord = &(pState->chords[pStep]);
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
        color = nvgRGBA(0xFF, 0x00, 0x00, 0xFF);
    }

    text = music::inversionNames[pState->chords[pStep].inversion];

}
// Inversion menu

void KeyModeBox::step() {
    if (!pState) {
        text = "";
        return;
    }

    text = music::NoteDegreeModeNames[pState->key][0][pState->mode] + " " + music::modeNames[pState->mode];

}

// ProgressStepWidget
void ProgressStepWidget::setPState(ProgressState *pState, int pStep) {

    clearChildren();

    math::Vec pos;

    RootChoice *rootChoice = createWidget<RootChoice>(pos);
    rootChoice->box.size.x = 170.0;
    rootChoice->textOffset.y = 10.0;
    rootChoice->pState = pState;
    rootChoice->pStep = pStep;
    addChild(rootChoice);
    pos = rootChoice->box.getTopRight();
    this->rootChooser = rootChoice;

    this->separators[0] = createWidget<LedDisplaySeparator>(pos);
    this->separators[0]->box.size.x = box.size.x / 3.0;
    addChild(this->separators[0]);

    ChordChoice *chordChoice = createWidget<ChordChoice>(pos);
    chordChoice->box.size.x = 100.0;
    chordChoice->textOffset.y = 10.0;
    chordChoice->pState = pState;
    chordChoice->pStep = pStep;
    addChild(chordChoice);
    pos = chordChoice->box.getTopRight();
    this->chordChooser = chordChoice;

    this->separators[1] = createWidget<LedDisplaySeparator>(pos);
    this->separators[1]->box.size.x = box.size.x / 3.0;
    addChild(this->separators[1]);

    InversionChoice *inversionChoice = createWidget<InversionChoice>(pos);
    inversionChoice->box.size.x = 50.0;
    inversionChoice->textOffset.y = 10.0;
    inversionChoice->pState = pState;
    inversionChoice->pStep = pStep;
    addChild(inversionChoice);
    pos = inversionChoice->box.getTopRight();
    this->inversionChooser = inversionChoice;

}
// ProgressStepWidget

// ProgressStateWidget
void ProgressStateWidget::setPState(ProgressState *pState) {
    clearChildren();
    math::Vec pos;

    KeyModeBox *keyModeBox  = createWidget<KeyModeBox>(pos);
    keyModeBox->box.size.x = 170.0;
    keyModeBox->pState = pState;
    addChild(keyModeBox);
    pos = keyModeBox->box.getBottomLeft();

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

