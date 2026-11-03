#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <atomic>
#include <memory>
#include <functional>

struct HarmonicPeakInfo
{
    float frequencyHz = 0.0f;
    float magnitudeDb = -100.0f;
    float attenuationDb = 0.0f;
    bool isTooHot = false;
};

class SequinAudioProcessor : public juce::AudioProcessor
{
public:
    SequinAudioProcessor();
    ~SequinAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder; // 2048
    static constexpr int numBins  = fftSize / 2 + 1; // 1025
    static constexpr int hopSize  = fftSize / 4; // 512

    void getVisualSnapshot(std::array<float, numBins>& outSpectrumDb,
                           std::array<float, numBins>& outBaselineDb,
                           std::array<float, numBins>& outThresholdDb,
                           std::vector<HarmonicPeakInfo>& outPeaks) const;

    double getCurrentSampleRate() const noexcept { return currentSampleRate.load(); }

    void loadPreset(int presetIndex);

    // Telemetry & Transport State
    std::atomic<bool> isDawPlaying         { false };
    std::atomic<int>  harmonicsDetected    { 0 };
    std::atomic<int>  tooHotHarmonicsCount { 0 };
    std::atomic<bool> warningLightActive   { false };
    std::atomic<float> lowCutFreqHz        { 80.0f };
    std::atomic<float> highCutFreqHz       { 20000.0f };

private:
    juce::AudioProcessorValueTreeState apvts;

    juce::dsp::FFT forwardFFT;
    juce::dsp::FFT inverseFFT;
    juce::dsp::WindowingFunction<float> window;

    std::atomic<double> currentSampleRate { 44100.0 };

    struct ChannelSTFT
    {
        std::array<float, fftSize> inputFifo {};
        std::array<float, fftSize> outputAccumulator {};
        std::array<float, fftSize> dryDelayLine {};
        std::array<float, fftSize * 2> fftWorkBuffer {};
        int fifoIndex = 0;
        int dryWriteIndex = 0;

        void reset()
        {
            inputFifo.fill(0.0f);
            outputAccumulator.fill(0.0f);
            dryDelayLine.fill(0.0f);
            fftWorkBuffer.fill(0.0f);
            fifoIndex = 0;
            dryWriteIndex = 0;
        }
    };

    std::array<ChannelSTFT, 2> stftChannels;
    std::array<float, numBins> currentBinGain {};

    mutable juce::SpinLock visualLock;
    std::array<float, numBins> smoothedSpectrumDb {};
    std::array<float, numBins> baselineFloorDb {};
    std::array<float, numBins> dynamicThresholdDb {};
    std::vector<HarmonicPeakInfo> detectedPeaks;

    int currentProgram = 0;
    bool wasDawPlaying = false;
    int sessionPeakHarmonics = 0;
    int sessionPeakTooHot = 0;

    void processSpectralFrame(ChannelSTFT& ch, bool isPrimaryAnalysisChannel,
                              float focusHz, float qVal, float sensitivityVal,
                              float tameAmount, bool auditionMode);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequinAudioProcessor)
};
