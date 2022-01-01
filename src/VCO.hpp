#include "AHCommon.hpp"

using namespace ah;

// Andrew Belt's LFO-2 code
struct LowFrequencyOscillator {
	float phase = 0.0f;
	float pw = 0.5f;
	float freq = 1.0f;
	bool offset = false;
	bool invert = false;
	rack::dsp::SchmittTrigger resetTrigger;

	LowFrequencyOscillator() {}
	void setPitch(float pitch) {
		pitch = fminf(pitch, 10.0f);
		freq = powf(2.0f, pitch);
	}
	void setPulseWidth(float pw_) {
		const float pwMin = 0.01f;
		pw = clamp(pw_, pwMin, 1.0f - pwMin);
	}
	void setReset(float reset) {
		if (resetTrigger.process(reset / 0.01f)) {
			phase = 0.0f;
		}
	}
	void step(float dt) {
		float deltaPhase = fminf(freq * dt, 0.5f);
		phase += deltaPhase;
		if (phase >= 1.0f)
			phase -= 1.0f;
	}
	float sin() {
		if (offset)
			return 1.0f - cosf(2 * core::PI * phase) * (invert ? -1.0f : 1.0f);
		else
			return sinf(2 * core::PI * phase) * (invert ? -1.0f : 1.0f);
	}
	float tri(float x) {
		return 4.0f * fabsf(x - roundf(x));
	}
	float tri() {
		if (offset)
			return tri(invert ? phase - 0.5f : phase);
		else
			return -1.0f + tri(invert ? phase - 0.25f : phase - 0.75f);
	}
	float saw(float x) {
		return 2.0f * (x - roundf(x));
	}
	float saw() {
		if (offset)
			return invert ? 2.0f * (1.0f - phase) : 2.0f * phase;
		else
			return saw(phase) * (invert ? -1.0f : 1.0f);
	}
	float sqr() {
		float sqr = (phase < pw) ^ invert ? 1.0f : -1.0f;
		return offset ? sqr + 1.0f : sqr;
	}
	float light() {
		return sinf(2 * core::PI * phase);
	}
};

// A 'portable' version of Andrew Belt's EvenVCO code, which is much less CPU intensive than VCO-1 or -2
struct EvenVCO {

	float phase = 0.0;
	/** The value of the last sync input */
	float sync = 0.0;
	/** The outputs */
	float tri = 0.0;
	/** Whether we are past the pulse width already */
	bool halfPhase = false;

	dsp::MinBlepGenerator<16, 32> triSquareMinBLEP;
	dsp::MinBlepGenerator<16, 32> triMinBLEP;
	dsp::MinBlepGenerator<16, 32> sineMinBLEP;
	dsp::MinBlepGenerator<16, 32> doubleSawMinBLEP;
	dsp::MinBlepGenerator<16, 32> sawMinBLEP;
	dsp::MinBlepGenerator<16, 32> squareMinBLEP;

	dsp::RCFilter triFilter;

    float pw;

   	float sine;
	float doubleSaw;
	float even;
	float saw;
	float square;

	EvenVCO() {	}

	void reset() {
		phase = 0.0;
		sync = 0.0;
		tri = 0.0;
		halfPhase = false;
	}

	void step(float delta, float pitch) {
		// Compute frequency, pitch is 1V/oct
		float freq = dsp::FREQ_C4 * powf(2.0, pitch);
		freq = rack::clamp(freq, 0.0f, 20000.0f);

		// Pulse width
		const float minPw = 0.05;
		pw = rack::rescale(rack::clamp(pw, -1.0f, 1.0f), -1.0f, 1.0f, minPw, 1.0f - minPw);

		// Advance phase
		float deltaPhase = rack::clamp(freq * delta, 1e-6f, 0.5f);
		float oldPhase = phase;
		phase += deltaPhase;

		if (oldPhase < 0.5 && phase >= 0.5) {
			float crossing = -(phase - 0.5) / deltaPhase;
			triSquareMinBLEP.insertDiscontinuity(crossing, 2.0);
			doubleSawMinBLEP.insertDiscontinuity(crossing, -2.0);
		}

		if (!halfPhase && phase >= pw) {
			float crossing  = -(phase - pw) / deltaPhase;
			squareMinBLEP.insertDiscontinuity(crossing, 2.0);
			halfPhase = true;
		}

		// Reset phase if at end of cycle
		if (phase >= 1.0) {
			phase -= 1.0;
			float crossing = -phase / deltaPhase;
			triSquareMinBLEP.insertDiscontinuity(crossing, -2.0);
			doubleSawMinBLEP.insertDiscontinuity(crossing, -2.0);
			squareMinBLEP.insertDiscontinuity(crossing, -2.0);
			sawMinBLEP.insertDiscontinuity(crossing, -2.0);
			halfPhase = false;
		}

		// Outputs
		float triSquare = (phase < 0.5) ? -1.0 : 1.0;
		triSquare += triSquareMinBLEP.process();

		// Integrate square for triangle
		tri += 4.0 * triSquare * freq * delta;
		tri *= (1.0 - 40.0 * delta);

		sine = -cosf(2* core::PI * phase);
		doubleSaw = (phase < 0.5) ? (-1.0 + 4.0*phase) : (-1.0 + 4.0*(phase - 0.5));
		doubleSaw += doubleSawMinBLEP.process();
		even = 0.55 * (doubleSaw + 1.27 * sine);
		saw = -1.0 + 2.0*phase;
		saw += sawMinBLEP.process();
		square = (phase < pw) ? -1.0 : 1.0;
		square += squareMinBLEP.process();
	}
};