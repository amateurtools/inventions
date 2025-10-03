#pragma once
#include <juce_dsp/juce_dsp.h>

class SimpleBandpass
{
public:
    SimpleBandpass() {}

    void prepare(double sampleRate, int maxBlockSize)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = maxBlockSize;
        spec.numChannels = 1;
        filter.prepare(spec);
        reset();
    }

    // Call this if changing frequency/Q at runtime
    void setParams(float centerHz, float Q)
    {
        juce::dsp::IIR::Coefficients<float>::Ptr coeffs =
            juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, centerHz, Q);
        *filter.state = *coeffs;
    }

    void reset()
    {
        filter.reset();
    }

    float processSample(float x)
    {
        return filter.processSample(x);
    }

    void setSampleRate(double sr)
    {
        sampleRate = sr;
    }

private:
    juce::dsp::IIR::Filter<float> filter;
    double sampleRate = 44100.0;
};
