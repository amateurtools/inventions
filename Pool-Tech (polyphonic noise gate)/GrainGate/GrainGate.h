#pragma once
#include "Windower.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <random>
#include <array>

enum class EnvelopeState { Inactive, Active, Dying };

struct GrainGate
{
    static constexpr int grainsInPool = 32;
    int nextGrainIndex = 0;
    std::mt19937 rng { std::random_device{}() };

    struct PoolGrain
    {
        Windower window;
        bool useInputB = false;
        EnvelopeState state = EnvelopeState::Inactive;
        int dyingCounter = 0;              // Remaining fade-out samples
        int initialDyingCounter = 0;       // Used for scaling
        static constexpr int dyingFadeMs = 8; // Fast fade, tune to taste
        bool wasActive = false;

        void trigger(int windowType, int length, bool isB)
        {
            window.startNewGrain(0, windowType, length);
            useInputB = isB;
            state = EnvelopeState::Active;
            dyingCounter = 0;
            wasActive = true;
        }

        // Call when the pool is full and this voice is about to be recycled
        void markDying(double sampleRate)
        {
            if (state == EnvelopeState::Active)
            {
                initialDyingCounter = dyingCounter = std::max(4, int(dyingFadeMs * 0.001 * sampleRate));
                state = EnvelopeState::Dying;
            }
        }

        bool isActive() const { return state != EnvelopeState::Inactive; }

        float process(float inA, float inB)
        {
            if (!window.isActive() && state != EnvelopeState::Dying)
            {
                state = EnvelopeState::Inactive;
                wasActive = false;
                return 0.0f;
            }

            float val = window.process(useInputB ? inB : inA);

            if (state == EnvelopeState::Dying)
            {
                if (dyingCounter > 0 && initialDyingCounter > 0)
                {
                    float fadeMult = dyingCounter / float(initialDyingCounter);
                    val *= fadeMult;
                    if (--dyingCounter <= 0 || !window.isActive())
                    {
                        state = EnvelopeState::Inactive;
                        wasActive = false;
                    }
                }
                else
                {
                    state = EnvelopeState::Inactive;
                    wasActive = false;
                }
            }
            else if (!window.isActive())
            {
                state = EnvelopeState::Inactive;
                wasActive = false;
            }

            return val;
        }
    };

    std::array<PoolGrain, grainsInPool> pool;

    void prepare(double sampleRate)
    {
        for (auto& grain : pool)
            grain.window.prepare(sampleRate);
        reset();
    }

    void reset()
    {
        for (auto& grain : pool)
            grain = PoolGrain();
        nextGrainIndex = 0;
    }

    // FIFO graceful stealing: marks oldest Active grain as Dying if needed, allocates next
    void triggerGrain(int windowType, int windowLength, bool useInputB, double sampleRate)
    {
        // Try to find an inactive grain
        for (int tries = 0; tries < grainsInPool; ++tries)
        {
            int idx = (nextGrainIndex + tries) % grainsInPool;
            if (!pool[idx].isActive())
            {
                pool[idx].trigger(windowType, windowLength, useInputB);
                nextGrainIndex = (idx + 1) % grainsInPool;
                return;
            }
        }
        // All busy: gracefully mark the oldest as dying and immediately re-use
        int oldestIdx = findOldestActive();
        pool[oldestIdx].markDying(sampleRate);
        pool[oldestIdx].trigger(windowType, windowLength, useInputB);
        nextGrainIndex = (oldestIdx + 1) % grainsInPool;
    }

    // Find grain to be stolen (e.g. the one that's been Active the longest)
    int findOldestActive() const
    {
        // Simple FIFO: the nextGrainIndex* is always oldest, unless you want to add time stamps for true "longest"
        // Here, we steal nextGrainIndex itself, but you could search for the one that's been Active the longest.
        return nextGrainIndex;
    }

    float process(float inputA, float inputB)
    {
        float out = 0.0f;
        for (auto& grain : pool)
            out += grain.process(inputA, inputB);
        return out;
    }
};
