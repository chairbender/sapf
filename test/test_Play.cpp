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
#include "Testability.hpp"
#include <vector>
#include <fstream>
#include <cmath>
#include <cstring>

#ifdef SAPF_AUDIOTOOLBOX
#include <AudioToolbox/AudioToolbox.h>
#else
#include <sndfile.h>
#endif

// TEST_CASE("recordPlayer function records audio data correctly") {
//     // Create a temporary file path for recording
//     std::string tempFilePath = "test_recordPlayer.wav";
//
//     // Test parameters
//     const int sampleRate = 44100;
//     const int numChannels = 2;
//     const int numFrames = 1024; // Number of frames to test
//     const int totalSamples = numFrames * numChannels;
//
//     // Generate test audio data - simple sine wave
//     std::vector<Z> testData(totalSamples);
//     const Z frequency = 440.0; // A4 note
//     const Z amplitude = 0.5;
//
//     for (int i = 0; i < numFrames; i++) {
//         Z sample = amplitude * std::sin(2.0 * M_PI * frequency * i / sampleRate);
//         for (int ch = 0; ch < numChannels; ch++) {
//             testData[i * numChannels + ch] = sample;
//         }
//     }
//
//     // TODO: create Player* and Buffers*
//
//     // TODO: invoke recordPlayer multiple times
//
//     // TODO: ensure async IO buffer is flushed (close stuff)
//
//     // TODO: Compare expected vs. actual
//
// }