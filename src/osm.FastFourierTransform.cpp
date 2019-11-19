#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include "osm.FastFourierTransform.h"

namespace osm {

// Implemention of Fast Fourier Transform
void FastFourierTransform(std::vector<Sample> samples, float* spectrums, int32_t samplingRate, FFTWindow window)
{
    // "samplingRate" must be the power of 2.
    if(samplingRate & (samplingRate - 1)) return;

    // Allocate & initialize memroy
    std::vector<float> spectrumReal;
    spectrumReal.reserve(samplingRate);
    spectrumReal.resize(samplingRate);
    std::vector<float> spectrumImag;
    spectrumImag.reserve(samplingRate);
    spectrumImag.resize(samplingRate);
    for(int i = 0; i < samplingRate; ++i)
    {
        // Move wave data
        Sample sample = samples[BitReverse(i, samplingRate)];
        spectrumReal[i] = (sample.Left + sample.Right);
        spectrumImag[i] = 0.0;

        // Apply window function
        switch(window)
        {
            case FFTWindow::Rectangular:
                break;

            case FFTWindow::Triangle:
                spectrumReal[i] *= Triangle((float)i / (float)samplingRate);
                break;

            case FFTWindow::Hamming:
                spectrumReal[i] *= Hamming((float)i / (float)samplingRate);
                break;

            case FFTWindow::Hanning:
                spectrumReal[i] *= Hanning((float)i / (float)samplingRate);
                break;

            case FFTWindow::Blackman:
                spectrumReal[i] *= Blackman((float)i / (float)samplingRate);
                break;

            case FFTWindow::BlackmanHarris:
                spectrumReal[i] *= BlackmanHarris((float)i / (float)samplingRate);
                break;
        }
    }

    // Fast Fourier Transform
    for(int i = 1; i < samplingRate; i *= 2)
        for(int j = 0; j < samplingRate; j += i * 2)
            for(int k = 0; k < i; ++k)
            {
                int m = j + k;
                int n = m + i;
                double tr = cos(-M_PI / (double)i * (double)k);
                double ti = sin(-M_PI / (double)i * (double)k);
                double mr = spectrumReal[m] + spectrumReal[n] * tr - spectrumImag[n] * ti;
                double mi = spectrumImag[m] + spectrumImag[n] * tr + spectrumReal[n] * ti;
                double nr = spectrumReal[m] - spectrumReal[n] * tr + spectrumImag[n] * ti;
                double ni = spectrumImag[m] - spectrumImag[n] * tr - spectrumReal[n] * ti;
                spectrumReal[m] = mr; spectrumImag[m] = mi;
                spectrumReal[n] = nr; spectrumImag[n] = ni;
            }

    // Return spectrum data as real value
    for(int i = 0; i < samplingRate; ++i)
        spectrums[i] = sqrt(pow(spectrumReal[i], 2) + pow(spectrumImag[i], 2));
}

// Bit Reverse
static int BitReverse(int32_t x, int32_t x_max)
{
    int bit_rev = 0;
    for(int i = x_max / 2; i >= 1; i /= 2)
    {
        bit_rev += i * (x % 2);
        x /= 2;
    }
    return bit_rev;
}

// Window Functions
static float Triangle(float x) { return 1 - abs(2 * x - 1); }

static float Hamming(float x) { return 0.54f - 0.46f * cosf(2 * M_PI * x); }

static float Hanning(float x) { return 0.5f - 0.5f * cosf(2 * M_PI * x); }

static float Blackman(float x) { return 0.42f - 0.5f * cosf(2 * M_PI * x) + 0.08f * cosf(4 * M_PI * x); }

static float BlackmanHarris(float x) { return 0.35875f - 0.48829f * cosf(2 * M_PI * x) + 0.14128f * cosf(4 * M_PI * x) - 0.012604f * cosf(6 * M_PI * x); }

}