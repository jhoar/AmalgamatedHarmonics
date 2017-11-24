# Amalgamated Harmonics

## Scale Quantiser

Scared by the anarchy of unconstrained tone? Want to come back to that familiar octave of your youth? Here at Amalgamated Harmonics, our engineers and scientists provide the best possible set of semitones, fresh from the laboratory. 

* V/OCT: Input pitch, in 1V/OCT
* KEY: Root note of the scale
* SCALE: Scale; In order of appearance:
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
* TRIG: Gate fired when the quantized pitch changes
* OUT: Quantised note (1V/OCT)
* Top row of outputs: Gates triggered according to the scale degree of the note quantised

This is largely done, will probably do some internal plumbing to use PulseGenerators in the future.
## Arpeggiator

Need some more predictable variety in your patches? Why not try the Arpeggiator from Amalgamated Harmonics? Guaranteed 17.5% more music per hour.

* P1 - P5: Input pitches; these will be scanned left to right and form the notes to be played in each cycle. Empty inputs are ignored
* STEPS: Number of repetitions of a cycle (max 16)
* DIST: Number of semi-tones used for shifting pitches (max 10)
* DIR: The notes in the cycles shift up or down according to the direction.  
* CLOCK: Input clock for the sequenced notes. This could be slaved to the source for the TRIG input, or maybe not...
* TRIG: Input gate to trigger the sequence

* OUT: Output sequenced note (1V/OCT)
* GATE: Output gate fired for each sequenced note. 
* EOC: Output gate fired when a cycle ends
* EOS: Output gate fired when the whole series of cycles is finished

Almost something usable. The gaping void in the middle of the panel is where the selection mechanism for patterns will go. At the moment it just goes up or down.

### Notes

These modules are primarily intended as a vehicle for me to learn how to create modules for VCV Rack.   

After watching a cool video from KlirrFaktor, I set out to build a scale based quantiser as there did not seem to be one around and this was a great opportuntity to learn how to build modules. I did not spot jeremywen's module in development, but wasn't going to let redundancy get in the way of good idea. I thought I'd at least add something (scale degree-based gates) that may or may not be usedful for someone.  

ScaleQuantiser: Input/Output from Rack, Lights system, UI design and how to align SVG with UI elements defined in the code (screenshots, transparency and object proxies). 
Arpeggiator: Working with clocks (in progress)

### Credits and Thanks

* SVG widgets adapted from the VCV Rack standard library provide by Grayscale
* jeremywen for the neat quantisation code, saved me a great deal of time
* The KlirrFaktor for his great video on building a sequencer: https://www.youtube.com/watch?v=RFyC4II1kRw   
