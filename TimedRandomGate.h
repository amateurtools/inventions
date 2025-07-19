/*
  ==============================================================================

  TimedRandomGate.h
  Created by AmateurTools DSP 2025

  Description:
    A time-gated random modulator for controlled probabilistic signal events.
    At each new evaluation (e.g., grain trigger), this class determines whether
    to allow randomization based on a looping one-hot vector whose loop length
    shortens with increased randomness, thus increasing the chances of allowing
    random behavior. When active, it applies a shaped randomness function for
    flipping gate/sequence values.

    Useful for granular synthesis, probabilistic sequencing, jitter modulation,
    and generative DSP applications.

  License:
    MIT License

    Copyright (c) 2025 AmateurTools DSP (aka Amateur Tools or AmateurTools)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

  Attribution:
    If used in creative or research projects, attribution is appreciated but not required:
    "[Your Name] and AI collaborative toolchain, 2025." or
    "TimedRandomGate, originally proposed by [Your Name] (2025)."

  ==============================================================================
*/


#pragma once

#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <atomic>

class TimedRandomGate
{
public:
    explicit TimedRandomGate(int sampleRate = 44100)
    {
        setSampleRate(sampleRate);
        seed(defaultSeed++);
    }

    void setSampleRate(int sr)
    {
        sampleRate = sr;
        modBuffer.resize(sr);
        fillOneHotBuffer();
    }

    void seed(unsigned int s)
    {
        rng.seed(s);
    }

    void reset()
    {
        modPosition = 0;
    }

    void setRandomness(float newAmount)
    {
        randomnessStrength = std::clamp(newAmount, 0.0f, 1.0f);
        updateLoopSize();
    }

    // Call once per sample, returns true only when real randomization is allowed
    bool shouldRandomizeThisSample()
    {
        bool trigger = (modBuffer[modPosition] == 1);
        modPosition = (modPosition + 1) % loopLength;

        return trigger;
    }

    // High-level use case: decides whether to flip gate value
    bool possiblyFlip(int gateValue)
    {
        if (!shouldRandomizeThisSample())
            return gateValue == 1;

        float shapedChance = std::pow(randomnessStrength, 1.5f); // Shape curve
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        bool flip = dist(rng) < shapedChance;

        return (flip ? !gateValue : gateValue) == 1;
    }

private:
    std::vector<int> modBuffer;
    int modPosition = 0;
    int sampleRate  = 44100;
    int loopLength  = 44100; // Starts at 1s window

    float randomnessStrength = 0.0f;

    std::mt19937 rng { std::random_device{}() };
    static inline std::atomic<unsigned> defaultSeed { 12345 };

    void fillOneHotBuffer()
    {
        std::fill(modBuffer.begin(), modBuffer.end(), 0);
        modBuffer[0] = 1;
    }

    void updateLoopSize()
    {
        loopLength = std::max(1, static_cast<int>(sampleRate * (1.0f - randomnessStrength)));
    }
};
