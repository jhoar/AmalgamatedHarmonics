# Amalgamated Harmonics

## Scale Quantiser

Scared by the anarchy of unconstrained tone? Want to come back to that familiar octave of your youth? Here at Amalgamated Harmonics, our engineers and scientists provide the best possible set of semitones, fresh from the laboratory. 

* V/OCT: Input signal
* KEY: Root note of the scale
* SCALE: In order of appearance:
	* Chromatic,
	* Ionian a.k.a. Major
	* Dorian
	* Phrygian
	* Lydian
	* Mixolydian
	* Aeolian a.k.a. (Natural) Minor
	* Locrian 
	* Major Pentonic 
	* Minor Pentatonic
	* Harmonic Minor
	* Blues
* TRIG: Gate fired quantized pitch changes
* OUT: Quantised note (V/OCT)
* Top row of outputs: Gates triggered from the scale degree of the note quantised

## Arpeggiator

Need some more predictable variety in your patches? Why not try the Arpeggiator from Amalgamated Harmonics? Guaranteed 17.5% more music per hour.

* V/OCT: Input signal
* CLOCK: Input clock for the sequenced notes
* GATE: Output gate fired for each sequenced note. This could be slaved to the source for the TRIG input, or maybe not...
* OUT: Output sequenced note (V/OCT)
* STEPS: Input number of steps in the sequence (max 16)
* DIST: Number of semi-tones for each step (max 10)
* TRIG: Input gate to trigger the sequence
* EOS: Output gate fired when the sequence ends

Bit of a work-in-progress this one. It just runs up or down the scale at the moment, this will be fixed with something much more interesting. There's also some oddness on the first cycle to be fixed.

### Notes

These modules are primarily intended as a vehicle for me to learn how to create modules for VCV Rack.   

After watching a cool video from KlirrFaktor, I set out to build a scale based quantiser as there did not seem to be one around and this was a great opportuntity to learn how to build modules. I did not spot jeremywen's module in development, but wasn't going to let redundancy get in the way of good idea. I thought I'd at least add something (scale degree-based gates) that may or may not be usedful for someone.  

ScaleQuantiser: Input/Output from Rack, Lights system, UI design and how to align SVG with UI elements defined in the code (screenshots, transparency and object proxies). 
Arpeggiator: Working with clocks (in progress)

### Credits and Thanks

* SVG widgets adapted from the VCV Rack standard library provide by Grayscale
* jeremywen for the neat quantisation code, saved me a great deal of time
* The KlirrFaktor for his great video on building a sequencer: https://www.youtube.com/watch?v=RFyC4II1kRw   
