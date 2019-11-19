

#pragma once

#include "OpenSoundMixer.h"
#include "osm.Sound_Impl.h"

namespace osm {

// Implemention of Fast Fourier Transform
void FastFourierTransform(std::vector<Sample> samples, float* spectrums, int32_t samplingRate, FFTWindow window);

// Bit Reverse
static int BitReverse(int32_t x, int32_t x_max);

// Window Functions
static float Triangle(float x);

static float Hamming(float x);

static float Hanning(float x);

static float Blackman(float x);

static float BlackmanHarris(float x);

}
