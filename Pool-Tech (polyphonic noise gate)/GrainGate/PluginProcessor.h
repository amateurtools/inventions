#pragma once

#include <JuceHeader.h>
#include "Windower.h"
#include "GrainGate.h"
#include "BeatDivisionTable.h" 

//==============================================================================
/**
    Represents the main audio processor for GrainGate, a polyphonic noise gate.
    Handles buffer processing, parameter state, routing configuration, and plugin interfacing.
*/
class GrainGateProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    GranularCrossfaderProcessor();
    ~GranularCrossfaderProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #if ! JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override  { return true; }

    //==============================================================================
    const juce::String getName() const override          { return JucePlugin_Name; }
    bool acceptsMidi() const override                    { return false; }
    bool producesMidi() const override                   { return false; }
    bool isMidiEffect() const override                   { return false; }
    double getTailLengthSeconds() const override         { return 0.0; }

    //==============================================================================
    int getNumPrograms() override                        { return 1; }
    int getCurrentProgram() override                     { return 0; }
    void setCurrentProgram (int) override                {}
    const juce::String getProgramName (int) override     { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Accessor for AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Factory for parameter layout setup
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    //==============================================================================

    juce::AudioProcessorValueTreeState apvts;

    static constexpr int NUM_WINDOW_TYPES = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularCrossfaderProcessor)
};
