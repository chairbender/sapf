#pragma once

#ifndef SAPF_AUDIOTOOLBOX
#include "PortableBuffers.hpp"

#include <memory>
#include <vector>
#include <sndfile.h>
#include <CDSPResampler.h>

class SndfileSoundFile {
public:
	// maxBufLen should be the max size (in samples) of the PortableBuffers that will be passed to pull.
	SndfileSoundFile(SNDFILE *inSndfile, int inNumChannels, double inFileSampleRate, double inThreadSampleRate, int maxBufLen);
	~SndfileSoundFile();

	uint32_t numChannels() const;

	// buffers must have:
	// 1. numChannels buffers
	// 2. each buffer has a size of framesRead.
	// Note framesRead is updated based on the actual number of frames that were read
	// (which would be less than requested in the event we reach the end of file for example).
	int pull(uint32_t *framesRead, PortableBuffers& buffers);
	// write to file synchronously (blocking)
	// bufs is expected to contain only a single buffer with the specified number of channels
	// and the buffer data already interleaved (for wav output), as floats. It should have
	// the exact amount of frames as indicated by numFrames.
	void write(int numFrames, const PortableBuffers& bufs) const;

	static std::unique_ptr<SndfileSoundFile> open(const char *path, double threadSampleRate, int maxBufLen);
	static std::unique_ptr<SndfileSoundFile> create(const char *path, int numChannels, double threadSampleRate, double fileSampleRate, bool interleaved, int maxBufLen);
private:
	void readUntilResamplerOutput(double *interleaved);
	void endOfInputFile();

	void pullWithoutResampling(uint32_t *framesRead, PortableBuffers &buffers, int requestedOutputFrames) const;
	void pullWithResampling(uint32_t *framesRead, PortableBuffers &buffers, int requestedOutputFrames);

	uint32_t readAndResample(PortableBuffers &buffers, int requestedOutputFrames,
	                         double *interleaved);
	uint32_t resampleTail(uint32_t framesAlreadyOutput, PortableBuffers &buffers, int requestedOutputFrames);

	static void deInterleaveAudio(const double *interleavedData, double *channelBuffer, int numFrames, int numChannels,
	                              int channelIndex);
	static int readFramesFromFile(SNDFILE *sndfile, double *interleavedBuffer, int framesToRead);
	static std::vector<std::unique_ptr<r8b::CDSPResampler>> initResamplers(int numChannels, double fileSampleRate,
	                                                                double threadSampleRate, int resamplerInputBufLen);

	static std::vector<std::vector<double>> initResamplerInputs(int numChannels, double fileSampleRate,
	                                                     double threadSampleRate,
	                                                     int resamplerInputBufLen);

	SNDFILE* const mSndfile;
	const int mNumChannels;
	const double mDestToSrcSampleRateRatio;
	int mResamplerInputBufLen;
	sf_count_t mFileFramesRead;
	sf_count_t mTotalFramesOutput;
	sf_count_t mExpectedTotalFramesOutput;
	bool mAtEndOfFile;

	// we need one per channel because the resampler is stateful
	const std::vector<std::unique_ptr<r8b::CDSPResampler>> mResamplers;
	// holds the de-interleaved channel input for feeding into the resampler.
	// Since we need to read more (or less) frames of input in order to produce
	// a specific number of output frames, this isn't always going to be the same size as
	// the PortableBuffers
	std::vector<std::vector<double>> mResamplerInputs;

	// can only calculate this AFTER setting up mResamplers
	int mResampleSamplesBeforeOutput;
};
#endif // SAPF_AUDIOTOOLBOX
