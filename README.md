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

## Arpeggiator

Need some more predictable variety in your patches? Why not try the Arpeggiator from Amalgamated Harmonics? Guaranteed 17.5% more music per hour.

How it is (supposed to) work is the following:

* The Arpeggiator plays a sequence, composed on 1 or more arpeggios, each arpeggio consisting of up to 6 notes.
* At the bottom there are 6 inputs which represent up to 6 pitches (1V/OCT) which will played in an arpeggios. 
* The ARP switch controls whether the pitch are played left-to-right (L-R), right-to-left (R-L) or a random selection of the pitches and selected (RND). In the RND position pitches can be repeated. Empty inputs are ignored.
* A arpeggio will repeated a number of times according to the STEPS input.
* Each subsequent arpeggio after the first will be shifted by the number of semi-tones controlled by the DIST input. The shift is either up or down depending on the setting of the SEQ switch; ASC shifts the arpeggio up, DSC shifts the arpeggio down and RND shifts the arpeggio up or down 50/50; in this position it is a random walk
* A sequence is triggered from a gate on the TRIG input.
* The sequence is... err... sequenced from the CLOCK input, so the CLOCK source should be running faster the TRIG source.
* The output pitch is on the OUT output (1V/OCT), with a gate fired on the GATE. 
* When an arpeggio completes, a gate is fired on the EOC output, and similarly when the whole sequence completes there is gate on the EOS output
* The LOCK button locks the pitches, SEQ and ARP settings to the value in the current arpeggio, both within the running sequence and after. When unlocked the pitches will be scanned at the start of every cycle. 

A small display shows the active STEPS and DIST in the currently active sequence, with the values being read from the inputs shown in square brackets. The active values of the SEQ and ARP parameters are alos shown

In summary:

* P1 - P6: Input pitches; these will be scanned left to right and form the notes to be played in each cycle. Empty inputs are ignored
* STEPS: Number of repetitions of a cycle (max 16)
* DIST: Number of semi-tones used for shifting pitches between cycles (max 10)
* ARP: Scan the pitches left-to-right (L-R) or right-to-left (R-L), or a random selection of pitches  (RND)
* SEQ: Shifts the arpeggio up (ASC), down (DSC) or randonly up or down (RND)
* CLOCK: Input clock for the arpeggio
* TRIG: Input gate to trigger the sequence
* LOCK: Lock the pitches, SEQ and ARP setting, including the randomised notes

* OUT: Output sequenced note (1V/OCT)
* GATE: Output gate fired for each sequenced note. 
* EOC: Output gate fired when a cycle ends
* EOS: Output gate fired when the whole series of cycles is finished

This is complete for the time being. There are a few new ideas which might materialise in the future.

### Notes

These modules are primarily intended as a vehicle for me to learn how to create modules for VCV Rack.   

After watching a cool video from KlirrFaktor, I set out to build a scale based quantiser as there did not seem to be one around and this was a great opportuntity to learn how to build modules. I did not spot jeremywen's module in development, but wasn't going to let redundancy get in the way of good idea. I thought I'd at least add something (scale degree-based gates) that may or may not be usedful for someone.  

ScaleQuantiser: Input/Output from Rack, Lights system, UI design and how to align SVG with UI elements defined in the code (screenshots, transparency and object proxies). 
Arpeggiator: Working with clocks (in progress)

### Credits and Thanks

* SVG widgets adapted from the VCV Rack standard library provide by Grayscale
* jeremywen for the neat quantisation code, saved me a great deal of time
* The KlirrFaktor for his great video on building a sequencer: https://www.youtube.com/watch?v=RFyC4II1kRw   
