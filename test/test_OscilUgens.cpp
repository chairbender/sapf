//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "Object.hpp"
#include "doctest.h"
#include "ArrHelpers.hpp"

// non-vectorized version for comparison
void sinosc_calc(Z phase, Z freqmul, int n, Z* out, Z* freq, int freqStride) {
	for (int i = 0; i < n; ++i) {
		out[i] = phase;
		phase += *freq * freqmul;
		freq += freqStride;
		if (phase >= kTwoPi) phase -= kTwoPi;
		else if (phase < 0.) phase += kTwoPi;
	}
	LOOP(i,n) { out[i] = sin(out[i]); }
}

TEST_CASE("SinOsc") {
	const int n = 100;
	Z out[n];
	Z freq[n];
	Z expected[n];
	Z startFreq = 400;
	Z freqmul = 1;
	Z iphase;
	int freqStride;
	Thread th;
	th.rate.radiansPerSample = freqmul;
	LOOP(i,n) { freq[i] = sin(i/(double)n)*400 + 200; }

	SUBCASE("") {
		iphase = 0;
		freqStride = 1;
	}

	SUBCASE("") {
		iphase = .3;
		freqStride = 1;
	}

	SUBCASE("") {
		iphase = 0;
		freqStride = 0;
	}

	SUBCASE("") {
		iphase = .3;
		freqStride = 0;
	}
	CAPTURE(iphase);
	CAPTURE(freqStride);

	Z phase = sc_wrap(iphase, 0., 1.) * kTwoPi;

	SinOsc osc(th, startFreq, iphase);
	osc.calc(n, out, freq, freqStride);
	sinosc_calc(phase, freqmul, n, expected, freq, freqStride);
	CHECK_ARR(expected, out, n);
}

// non-vectorized version for comparison
void sinoscpm_calc(Z phase, Z freqmul, int n, Z* out, Z* freq, Z* phasemod, int freqStride, int phasemodStride) {
	for (int i = 0; i < n; ++i) {
		out[i] = phase + *phasemod * kTwoPi;
		phase += *freq * freqmul;
		freq += freqStride;
		phasemod += phasemodStride;
		if (phase >= kTwoPi) phase -= kTwoPi;
		else if (phase < 0.) phase += kTwoPi;
	}
	LOOP(i,n) { out[i] = sin(out[i]); }
}

TEST_CASE("SinOscPM") {
	const int n = 100;
	Z out[n];
	Z freq[n];
	Z expected[n];
	Z startFreq = 400;
	Z freqmul = 1;
	Z iphase;
	int freqStride;
	Thread th;
	th.rate.radiansPerSample = freqmul;
	LOOP(i,n) { freq[i] = sin(i/(double)n)*400 + 200; }

	SUBCASE("") {
		iphase = 0;
		freqStride = 1;
	}

	SUBCASE("") {
		iphase = .3;
		freqStride = 1;
	}

	SUBCASE("") {
		iphase = 0;
		freqStride = 0;
	}

	SUBCASE("") {
		iphase = .3;
		freqStride = 0;
	}
	CAPTURE(iphase);
	CAPTURE(freqStride);

	Z phase = sc_wrap(iphase, 0., 1.) * kTwoPi;

	SinOscPM osc(th, startFreq, iphase);
	osc.calc(n, out, freq, freqStride);
	sinoscpm_calc(phase, freqmul, n, expected, freq, freqStride);
	CHECK_ARR(expected, out, n);
}