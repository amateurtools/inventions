constexpr int fftOrder = 9; // 512-point FFT
constexpr int fftSize = 1 << fftOrder;
constexpr int scopeSize = 128; // Number of bands in your display

juce::dsp::FFT fft { fftOrder };
juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };
float fifo[fftSize];
float fftData[2 * fftSize];
float scopeData[scopeSize];

void pushNextSampleIntoFifo(float sample) {
    // Fill FIFO, run FFT when full
    fifo[fifoIndex++] = sample;
    if (fifoIndex == fftSize) {
        std::copy(fifo, fifo+fftSize, fftData);
        window.multiplyWithWindowingTable(fftData, fftSize);
        fft.performFrequencyOnlyForwardTransform(fftData);

        // Fill scopeData
        for (int i = 0; i < scopeSize; ++i) {
            // Simple mapping: log/linear, or averaging bins
            int bin = i * (fftSize/2) / scopeSize;
            float mag = juce::Decibels::gainToDecibels(fftData[bin]);
            // normalize to 0...1 for drawing (map appropriate range)
            scopeData[i] = juce::jmap(mag, -100.0f, 0.0f, 0.0f, 1.0f);
        }

        fifoIndex = 0;
        nextFFTBlockReady = true;
    }
}
