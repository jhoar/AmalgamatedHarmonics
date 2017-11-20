#include "rack.hpp"


using namespace rack;


extern Plugin *plugin;

////////////////////
// module widgets
////////////////////

struct ScaleQuantizerWidget : ModuleWidget {
	ScaleQuantizerWidget();
	void calculateKey(int key, float spacing, float xOff, float yOff, float *x, float *y, int *scale);
};
