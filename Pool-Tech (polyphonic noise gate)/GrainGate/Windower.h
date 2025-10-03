#pragma once
#include <juce_core/juce_core.h>
#include <cmath>

struct WindowerParams
{
    float grainSizeMs = 100.0f;
    int windowType = 0; // If >= 10, use ADSR mode (see below)
    // --- ADSR parameters ---
    float attackMs = 2.0f;
    float decayMs  = 10.0f;
    float sustain  = 0.8f;
    float releaseMs = 20.0f;

    float bpm = 120.0f;
    int beat_division = 0;
    double currentPPQ = 0.0;
    double grainStartPPQ = 0.0;
    double sampleRate = 44100.0;
    bool isPlaying = true;
    float randomness = 0.0f;
    bool useBeats = false;
    bool stereoCorrelation = false;
    float crossfade = 1.0f;
    bool lockToGrid = false;
};

class Windower
{
public:
    Windower() = default;

    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
    }

    void reset()
    {
        active = false;
        sampleIndex = 0;
        length = 512;
        windowType = 0;
        // For ADSR
        envState = EnvState::Idle;
    }

    // Now takes full WindowerParams so ADSR params can be set
    void startNewGrain(int offsetSamples, int windowTypeToUse, int windowLengthSamples, const WindowerParams& params = {})
    {
        sampleIndex = juce::jlimit(0, windowLengthSamples - 1, offsetSamples);
        length      = windowLengthSamples;
        windowType  = windowTypeToUse;
        envParams   = params;
        active      = true;

        if (windowTypeToUse >= 10)
        {
            setADSRStages(params, windowLengthSamples);
            envSample = 0;
            envState  = EnvState::Attack;
        }
    }

    bool isActive() const { return active; }

    float process(float input)
    {
        if (!active)
            return 0.0f;

        float gain = 1.0f;

        if (windowType >= 10)
            gain = processADSR();
        else
            gain = evaluateWindow(sampleIndex, length, windowType);

        ++sampleIndex;
        if (sampleIndex >= length || (windowType >= 10 && envState == EnvState::Idle))
        {
            active = false;
            return 0.0f;
        }
        return input * gain;
    }

    // For GUI/debugging
    static juce::String getWindowName(int type)
    {
        switch (type)
        {
            case 0: return "Hann (cos)";
            case 1: return "Triangle";
            case 2: return "Blackman";
            case 3: return "Rectangular";
            case 4: return "Exponential";
            case 10: return "ADSR";
            default: return "Flat";
        }
    }

private:
    double sampleRate = 44100.0;
    int sampleIndex = 0;
    int length = 512;
    int windowType = 0;
    bool active = false;

    // ADSR structures
    enum class EnvState { Idle, Attack, Decay, Sustain, Release };
    EnvState envState = EnvState::Idle;
    int envSample = 0;
    int attackSamples = 1, decaySamples = 1, releaseSamples = 1, sustainSamples = 1;
    float sustainLevel = 0.8f;
    WindowerParams envParams;

    void setADSRStages(const WindowerParams& params, int totalLength)
    {
        // Convert ms to samples, clamp to totalLength
        attackSamples  = juce::jlimit(1, totalLength, int(params.attackMs  * 0.001 * sampleRate));
        decaySamples   = juce::jlimit(1, totalLength-attackSamples, int(params.decayMs * 0.001 * sampleRate));
        releaseSamples = juce::jlimit(1, totalLength-attackSamples-decaySamples, int(params.releaseMs * 0.001 * sampleRate));
        sustainLevel   = params.sustain;

        // Remaining samples become sustain
        int used = attackSamples + decaySamples + releaseSamples;
        sustainSamples = std::max(totalLength - used, 1);
    }

    float processADSR()
    {
        float value = 0.0f;
        switch (envState)
        {
            case EnvState::Attack:
                value = float(envSample) / float(std::max(1, attackSamples));
                if (++envSample >= attackSamples)
                {
                    envSample = 0;
                    envState  = EnvState::Decay;
                }
                break;
            case EnvState::Decay:
                value = 1.0f - (1.0f - sustainLevel) * float(envSample) / float(std::max(1, decaySamples));
                if (++envSample >= decaySamples)
                {
                    envSample = 0;
                    envState = EnvState::Sustain;
                }
                break;
            case EnvState::Sustain:
                value = sustainLevel;
                if (++envSample >= sustainSamples)
                {
                    envSample = 0;
                    envState  = EnvState::Release;
                }
                break;
            case EnvState::Release:
                value = sustainLevel * (1.0f - float(envSample) / float(std::max(1, releaseSamples)));
                if (++envSample >= releaseSamples)
                    envState = EnvState::Idle;
                break;
            default:
                envState = EnvState::Idle; value = 0.0f;
                break;
        }
        return value;
    }

    // Window-based envelopes
    float evaluateWindow(int sampleIdx, int length, int type) const
    {
        jassert(sampleIdx >= 0 && sampleIdx < length);
        float phase = sampleIdx / float(length - 1);

        switch (type)
        {
            case 0: // Hann
                return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
            case 1: // Triangle
                return 1.0f - std::abs(2.0f * phase - 1.0f);
            case 2: // Blackman
                return 0.42f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * phase)
                             + 0.08f * std::cos(2.0f * juce::MathConstants<float>::twoPi * phase);
            case 3: // Rectangular
                return 1.0f;
            case 4: // Exponential
                return phase <= 1.0f ? std::exp(-4.0f * (1.0f - phase)) : 0.0f;
            default:
                return 1.0f;
        }
    }
};
