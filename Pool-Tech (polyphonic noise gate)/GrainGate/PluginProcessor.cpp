#include "PluginProcessor.h"

//==============================================================================
GrainGateProcessor::GrainGateProcessor()
    : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                      // Main input bus
                      .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                      // Main output bus
                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                      // Optional sidechain input bus (index 1)
                      .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)
#endif
                      ),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

GrainGateProcessor::~GrainGateProcessor()
{
}

void GrainGateProcessor::releaseResources()
{
    // Free resources if needed
}

bool GrainGateProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // --- Original stereo main + stereo sidechain case ---
    if (layouts.inputBuses.size() == 2 && layouts.outputBuses.size() == 1)
    {
        auto mainInputChannels = layouts.getMainInputChannelSet();
        if (mainInputChannels != juce::AudioChannelSet::stereo())
        {
            DBG("REJECT: Main input must be stereo, got " << mainInputChannels.getDescription());
            return false;
        }

        auto mainOutputChannels = layouts.getMainOutputChannelSet();
        if (mainOutputChannels != juce::AudioChannelSet::stereo())
        {
            DBG("REJECT: Main output must be stereo, got " << mainOutputChannels.getDescription());
            return false;
        }

        auto sidechainChannels = layouts.getChannelSet(true, 1);
        if (sidechainChannels != juce::AudioChannelSet::stereo())
        {
            DBG("REJECT: Sidechain input must be stereo, got " << sidechainChannels.getDescription());
            return false;
        }

        DBG("ACCEPT: Stereo main in/out and stereo sidechain.");
        return true;
    }

    // --- New case: 4 discrete in, 2 discrete out (single input/output bus) ---
    if (layouts.inputBuses.size() == 1 && layouts.outputBuses.size() == 1)
    {
        auto inSet  = layouts.getMainInputChannelSet();
        auto outSet = layouts.getMainOutputChannelSet();

        if (inSet == juce::AudioChannelSet::discreteChannels(4) &&
            outSet == juce::AudioChannelSet::discreteChannels(2))
        {
            DBG("ACCEPT: 4 discrete input channels, 2 discrete output channels.");
            return true;
        }
    }

    DBG("REJECT: Unsupported bus layout.");
    return false;
}

void GrainGateProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare your DSP here

    grainGateL.prepare(sampleRate);
    grainGateR.prepare(sampleRate);
}

void GrainGateProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    jassert(numSamples > 0); // Buffer should not be empty

    // --- Clear unused output channels
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, numSamples);

    // --- Get buffer pointers
    auto mainIn  = getBusBuffer(buffer, true, 0);
    auto sideIn  = getBusBuffer(buffer, true, 1);
    auto out     = getBusBuffer(buffer, false, 0);

    // Channel sanity checks
    jassert(mainIn.getNumChannels() >= 2);  // stereo main input required
    jassert(sideIn.getNumChannels() >= 2);  // stereo sidechain required (if always expected)
    jassert(out.getNumChannels()   >= 2);   // stereo output required

    const float* mainL  = mainIn .getReadPointer(0);  jassert(mainL  != nullptr);
    const float* mainR  = mainIn .getReadPointer(1);  jassert(mainR  != nullptr);

    bool haveSidechain = sideIn.getNumChannels() >= 2;
    const float* sideL  = haveSidechain ? sideIn.getReadPointer(0) : mainIn.getReadPointer(0);
    const float* sideR  = haveSidechain ? sideIn.getReadPointer(1) : mainIn.getReadPointer(1);

    float* outL = out.getWritePointer(0);             
    jassert(outL   != nullptr);
    float* outR = out.getWritePointer(1);             
    jassert(outR   != nullptr);

    // --- Get project tempo and timeline info
    auto* playHead = getPlayHead();
    double bpm = 120.0;
    double ppq = 0.0;
    bool isPlaying = false;

    if (playHead != nullptr)
    {
        if (auto pos = playHead->getPosition())
        {
            // Bpm/Ppq may not be present even if position exists, so "value_or"
            bpm = pos->getBpm().hasValue()         ? *pos->getBpm()         : 120.0;
            ppq = pos->getPpqPosition().hasValue() ? *pos->getPpqPosition() : 0.0;
            isPlaying = pos->getIsPlaying();
        }
    }
    jassert(bpm > 10.0 && bpm < 400.0); // Catch host bugs: BPM is in a sensible range

    // --- Collect all params, including timeline/tempo
    GrainGateParams params;

    // Plugin parameters from APVTS
    
    constexpr int maxWindowTypeIndex = 3; // change to match your actual number of window types
    params.windowType = juce::jlimit(0, maxWindowTypeIndex, static_cast<int>(*apvts.getRawParameterValue("window_type")));

    params.grainSizeMs = juce::jlimit(0.5f, 2000.0f, (float) *apvts.getRawParameterValue("grain_size"));

    // Range checks for major params
    jassert(params.bpm > 10.0f && params.bpm < 999.0f);
    jassert(params.sampleRate > 6000.0 && params.sampleRate < 192000.0);
    jassert(params.grainSizeMs    > 0.5f  && params.grainSizeMs < 2000.0f);

    // -- host timeline/transport info 
    params.bpm         = bpm;
    params.currentPPQ  = ppq;
    params.isPlaying   = isPlaying;
    params.sampleRate  = getSampleRate();

    params.attackMs = juce::jlimit(1.0f, 50.0f, (float) *apvts.getRawParameterValue("grain_size"));
    params.decayMs = juce::jlimit(1.0f, 100.06f, (float) *apvts.getRawParameterValue("decayMs"));
    params.sustain = juce::jlimit(0.0f, 1.0f, (float) *apvts.getRawParameterValue("sustain"));
    params.releaseMs = juce::jlimit(1.0f, 250.0f, (float) *apvts.getRawParameterValue("releaseMs"));

    // --- Output channel buffer must be valid
    jassert(outL != nullptr && outR != nullptr);

    // --- grainGate: make sure input/output pointers are valid
    jassert(mainL != nullptr && mainR != nullptr);
    jassert(sideL != nullptr && sideR != nullptr);

    // Classic stereo: process left and right with the same plan, their own channels.
    for (int i = 0; i < numSamples; ++i) {
        outL[i] = grainGateL.process(mainL[i], sideL[i], params, mainPlan);
        outR[i] = grainGateR.process(mainR[i], sideR[i], params, mainPlan);
    }

}

void GrainGateProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save your parameters state
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void GrainGateProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Load your parameters state
    auto tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));
    if (tree.isValid())
        apvts.replaceState(tree);
}

juce::AudioProcessorEditor* GrainGateProcessor::createEditor()
{
    return nullptr; // No GUI editor
}


juce::AudioProcessorValueTreeState::ParameterLayout GrainGateProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // Randomness (0 to 1)
    params.push_back(std::make_unique<AudioParameterFloat>("randomness", "Randomness", 0.0f, 0.06f, 0.0f));

    // Stereo Correlation (boolean toggle)
    params.push_back(std::make_unique<AudioParameterBool>("stereo_correlation", "Stereo Correlation", true));

    // Window Type (combobox with several window types)
    // StringArray windowChoices { "Hann", "Blackman", "Triangle", "Rectangular" };
    StringArray windowChoices { "Hann", "Triangle" };
    params.push_back(std::make_unique<AudioParameterChoice>("window_type", "Window Type", windowChoices, 0));

    params.push_back(std::make_unique<AudioParameterBool>(
        "timebase", "Beat Sync (on) / ms (off)", false));

    params.push_back(std::make_unique<AudioParameterBool>(
        "lockToGrid", "Lock To Grid", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grain_size", "Grain Size / Window Length", 20.0f, 250.0f, 50.0f)); // ms/beat-units depending on timebase

    // Optionally, keep if still musically relevant
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "crossfade", "Crossfade", 0.0f, 1.0f, 0.0f));

    juce::StringArray beatDivisionLabels;
    for (const auto& div : kBeatDivisions)
        beatDivisionLabels.add(div.label);

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "beat_division",           // parameter ID
        "Beat Division",           // human label
        beatDivisionLabels,        // options from your table
        11));                      // default: e.g. index of "1/16" or any you prefer

    params.push_back(std::make_unique<AudioParameterFloat>("attackMs", "Attack", 1.0f, 50.0f, 2.0f));
    params.push_back(std::make_unique<AudioParameterFloat>("decayMs", "Decay", 1.0f, 0.06f, 0.0f));
    params.push_back(std::make_unique<AudioParameterFloat>("sustain", "Sustain", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<AudioParameterFloat>("releaseMs", "Release", 1.0f, 250.0f, 100.0f));

    // params.push_back(std::make_unique<AudioParameterFloat>(
    //     "overlap", "Grain Overlap",
    //     NormalisableRange<float>(0.0f, 1.0f, 0.01f),
    //     0.5f));

    return { params.begin(), params.end() };
}

//==============================================================================

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrainGateProcessor();
}
