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
	p->website = "https://github.com/jhoar/AmalgamatedHarmonics";
	p->manual = "https://github.com/jhoar/AmalgamatedHarmonics";

	// For each module, specify the ModuleWidget subclass, manufacturer slug (for saving in patches), manufacturer human-readable name, module slug, 
	// and module name
	p->addModel(createModel<ScaleQuantizerWidget>("Amalgamated Harmonics", "ScaleQuantizer", "Scale Quantizer", QUANTIZER_TAG));
	p->addModel(createModel<ArpeggiatorWidget>("Amalgamated Harmonics", "Arpeggiator", "Arpeggiator", QUANTIZER_TAG));

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
