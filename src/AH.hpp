#pragma once

#include "rack.hpp"

using namespace rack;

extern Plugin *plugin;

////////////////////
// module widgets
////////////////////

struct ScaleQuantizerWidget : ModuleWidget {
	ScaleQuantizerWidget();
};

struct ScaleQuantizer2Widget : ModuleWidget {
	ScaleQuantizer2Widget();
};

struct ArpeggiatorWidget : ModuleWidget {
	ArpeggiatorWidget();
};

struct Arpeggiator2Widget : ModuleWidget {
	Arpeggiator2Widget();
	Menu *createContextMenu() override;
};


struct ProgressWidget : ModuleWidget {
	ProgressWidget();
	Menu *createContextMenu() override;
};

struct CircleWidget : ModuleWidget {
	CircleWidget();
};

struct ImperfectWidget : ModuleWidget {
	ImperfectWidget();
};



