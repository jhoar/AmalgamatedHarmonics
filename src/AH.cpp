#include "AH.hpp"

// The plugin-wide instance of the Plugin class
Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	// This is the unique identifier for your plugin
	p->slug = "AmalgamatedHarmonics";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif
	p->website = "https://github.com/jhoar/AmalgamatedHarmonics/wiki";
	p->manual = "https://github.com/jhoar/AmalgamatedHarmonics/wiki";

	// For each module, specify the ModuleWidget subclass, manufacturer slug (for saving in patches), manufacturer human-readable name, module slug, 
	// and module name
	p->addModel(createModel<ScaleQuantizerWidget>("Amalgamated Harmonics", "ScaleQuantizer", "Scale Quantizer", QUANTIZER_TAG));
	p->addModel(createModel<ScaleQuantizer2Widget>("Amalgamated Harmonics", "ScaleQuantizer2", "Scale Quantizer MkII", QUANTIZER_TAG));
	p->addModel(createModel<ArpeggiatorWidget>("Amalgamated Harmonics", "Arpeggiator", "Arpeggiator", SEQUENCER_TAG));
	p->addModel(createModel<ProgressWidget>("Amalgamated Harmonics", "Progress", "Progress", SEQUENCER_TAG));
	p->addModel(createModel<CircleWidget>("Amalgamated Harmonics", "Circle", "Fifths and Fourths", SEQUENCER_TAG));
        p->addModel(createModel<ImperfectWidget>("Amalgamated Harmonics", "Imperfect", "Imperfect", UTILITY_TAG));


	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
