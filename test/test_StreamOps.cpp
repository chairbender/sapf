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

#include "Object.hpp"
#include "VM.hpp"
#include "doctest.h"
#include "ArrHelpers.hpp"
#include "ZArr.hpp"

// non-vectorized version for comparison
void hann_calc(Z* out, int n) {
	for (int i = 0; i < n; i++) {
		out[i] = 0.5 * (1 - cos(2*M_PI*i/(n-1)));
	}
}

void hanning_(Thread& th, Prim* prim);

TEST_CASE("hanning simd") {
	const int n = 100;
	Z expected[n];
	Thread th;
	th.push(n);

	hann_calc(expected, n);
	hanning_(th, nullptr);
	P<List> outZList = th.popZList("hanning");
	Z* out = outZList->mArray->z();

	CHECK_ARR(expected, out, n);
}

void hamm_calc(Z* out, int n) {
	for (int i = 0; i < n; i++) {
		out[i] = 0.54 - .46 * cos(2*M_PI*i/(n-1));
	}
}

void hamming_(Thread& th, Prim* prim);

TEST_CASE("hamming simd") {
	const int n = 100;
	Z expected[n];
	Thread th;
	th.push(n);

	hamm_calc(expected, n);
	hamming_(th, nullptr);
	P<List> outZList = th.popZList("hamming");
	Z* out = outZList->mArray->z();

	CHECK_ARR(expected, out, n);
}

void blackman_calc(Z* out, int n) {
	for (int i = 0; i < n; i++) {
		out[i] = 0.42
			- .5 * cos(2*M_PI*i/(n-1))
			+ .08 * cos(4*M_PI*i/(n-1));
	}
}

void blackman_(Thread& th, Prim* prim);

TEST_CASE("blackman simd") {
	const int n = 100;
	Z expected[n];
	Thread th;
	th.push(n);

	blackman_calc(expected, n);
	blackman_(th, nullptr);
	P<List> outZList = th.popZList("blackman");
	Z* out = outZList->mArray->z();

	CHECK_ARR(expected, out, n);
}

struct WinSegment : public Gen
{
	ZIn in_;
	BothIn hop_;
	P<Array> window_;
#ifndef SAPF_ACCELERATE
	ZArr windowzarr_;
#endif
	int length_;
	int offset;
	Z fracsamp_;
	Z sr_;
	WinSegment(Thread& th, Arg in, Arg hop, P<Array> const& window);
	virtual const char* TypeName();
	virtual void pull(Thread& th);
};

// void winseg_pull_calc(Z* in, ...)
//
// TEST_CASE("WinSegment pull simd") {
//
// }
