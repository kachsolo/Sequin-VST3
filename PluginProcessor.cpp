#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

SequinAudioProcessor::SequinAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      forwardFFT(fftOrder),
      inverseFFT(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann, false)
{
    smoothedSpectrumDb.fill(-90.0f);
    baselineFloorDb.fill(-42.0f);
    dynamicThresholdDb.fill(-24.0f);
    currentBinGain.fill(1.0f);
    detectedPeaks.reserve(64);
}

SequinAudioProcessor::~SequinAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout SequinAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto freqRange = juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f);
    freqRange.setSkewForCentre(1500.0f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "focusFreq", 1 },
        "Focus Frequency",
        freqRange,
        3450.0f));

    auto qRange = juce::NormalisableRange<float>(0.5f, 16.0f, 0.01f);
    qRange.setSkewForCentre(3.5f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "qResonance", 1 },
        "Q / Resonance",
        qRange,
        6.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sensitivity", 1 },
        "Sensitivity (Too Hot)",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        65.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "harmonicTame", 1 },
        "Harmonic Tame",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f),
        -4.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "focusAudition", 1 },
        "Focus Audition",
        false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "bypass", 1 },
        "Bypass",
        false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "viewMode", 1 },
        "View Mode (RX Stack)",
        false));

    return { params.begin(), params.end() };
}

const juce::String SequinAudioProcessor::getName() const { return JucePlugin_Name; }
bool SequinAudioProcessor::acceptsMidi() const { return false; }
bool SequinAudioProcessor::producesMidi() const { return false; }
bool SequinAudioProcessor::isMidiEffect() const { return false; }
double SequinAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int SequinAudioProcessor::getNumPrograms() { return 1; }
int SequinAudioProcessor::getCurrentProgram() { return 0; }
void SequinAudioProcessor::setCurrentProgram(int) {}
const juce::String SequinAudioProcessor::getProgramName(int) { return {}; }
void SequinAudioProcessor::changeProgramName(int, const juce::String&) {}

void SequinAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate.store(sampleRate > 0.0 ? sampleRate : 44100.0);

    for (auto& ch : stftChannels)
        ch.reset();

    currentBinGain.fill(1.0f);
    setLatencySamples(fftSize - hopSize);

    // Initialize Frequency-Calibrated Green Baseline Floor (-3 dB/oct vocal tilt reference)
    const double sr = currentSampleRate.load();
    juce::SpinLock::ScopedLockType lock(visualLock);
    for (int k = 1; k < numBins; ++k)
    {
        float freq = static_cast<float>(k * sr / static_cast<double>(fftSize));
        float octavesFrom200 = std::log2( std::max(20.0f, freq) / 200.0f );
        baselineFloorDb[static_cast<size_t>(k)] = -28.0f - 2.6f * std::max(0.0f, octavesFrom200);
    }
    baselineFloorDb[0] = -40.0f;
}

void SequinAudioProcessor::releaseResources() {}

bool SequinAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainOut == layouts.getMainInputChannelSet();
}

void SequinAudioProcessor::resetLoopTelemetry()
{
    loopTotalHarmonics.store(0);
    loopExcessiveCount.store(0);
}

void SequinAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples             = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Check DAW Playhead for Loop Telemetry reset when Cubase loops back
    if (auto* playHead = getPlayHead())
    {
        if (auto posInfo = playHead->getPosition())
        {
            if (auto ppq = posInfo->getPpqPosition())
            {
                if (lastPlayheadPpq > 0.0 && *ppq < lastPlayheadPpq - 0.25)
                    resetLoopTelemetry();
                lastPlayheadPpq = *ppq;
            }
        }
    }

    const float focusHz       = apvts.getRawParameterValue("focusFreq")->load();
    const float qVal          = apvts.getRawParameterValue("qResonance")->load();
    const float sensitivity   = apvts.getRawParameterValue("sensitivity")->load();
    const float tameDb        = apvts.getRawParameterValue("harmonicTame")->load();
    const float mixPct        = apvts.getRawParameterValue("mix")->load();
    const bool auditionMode   = apvts.getRawParameterValue("focusAudition")->load() > 0.5f;
    const bool bypassed       = apvts.getRawParameterValue("bypass")->load() > 0.5f;

    // Update Bandpass Window Bounds
    const float halfBandwidthOct = 0.65f / std::max(0.5f, qVal);
    const float fLow  = juce::jlimit(20.0f, 20000.0f, focusHz * std::pow(2.0f, -halfBandwidthOct));
    const float fHigh = juce::jlimit(20.0f, 20000.0f, focusHz * std::pow(2.0f, +halfBandwidthOct));
    lowCutFreqHz.store(fLow);
    highCutFreqHz.store(fHigh);

    const float wetGain = mixPct * 0.01f;
    const float dryGain = 1.0f - wetGain;
    const int numCh = std::min(totalNumInputChannels, 2);

    for (int chIdx = 0; chIdx < numCh; ++chIdx)
    {
        auto& stft = stftChannels[static_cast<size_t>(chIdx)];
        float* channelData = buffer.getWritePointer(chIdx);

        for (int s = 0; s < numSamples; ++s)
        {
            const float inSample = channelData[s];

            // Latency-matched dry delay line
            const int delayReadIdx = (stft.dryWriteIndex + hopSize) % fftSize;
            const float delayedDry = stft.dryDelayLine[static_cast<size_t>(delayReadIdx)];
            stft.dryDelayLine[static_cast<size_t>(stft.dryWriteIndex)] = inSample;
            stft.dryWriteIndex = (stft.dryWriteIndex + 1) % fftSize;

            // Push input into STFT circular FIFO
            stft.inputFifo[static_cast<size_t>(stft.fifoIndex)] = inSample;

            // Read reconstructed wet sample from OLA accumulator
            const float wetSample = stft.outputAccumulator[static_cast<size_t>(stft.fifoIndex)];
            stft.outputAccumulator[static_cast<size_t>(stft.fifoIndex)] = 0.0f;

            if (bypassed)
                channelData[s] = delayedDry;
            else if (auditionMode)
                channelData[s] = wetSample;
            else
                channelData[s] = (wetSample * wetGain) + (delayedDry * dryGain);

            stft.fifoIndex++;
            if (stft.fifoIndex >= fftSize)
                stft.fifoIndex = 0;

            if (stft.fifoIndex % hopSize == 0)
            {
                processSpectralFrame(stft, chIdx == 0, focusHz, qVal,
                                     sensitivity, tameDb, auditionMode);
            }
        }
    }
}

void SequinAudioProcessor::processSpectralFrame(ChannelSTFT& ch, bool isPrimaryAnalysisChannel,
                                                float /*focusHz*/, float /*qVal*/, float sensitivityVal,
                                                float tameDb, bool auditionMode)
{
    ch.fftWorkBuffer.fill(0.0f);

    // Copy unwrapped frame from circular FIFO
    int readIdx = ch.fifoIndex;
    for (int i = 0; i < fftSize; ++i)
    {
        ch.fftWorkBuffer[static_cast<size_t>(i)] = ch.inputFifo[static_cast<size_t>(readIdx)];
        readIdx = (readIdx + 1) & (fftSize - 1);
    }

    window.multiplyWithWindowingTable(ch.fftWorkBuffer.data(), fftSize);
    forwardFFT.performRealOnlyForwardTransform(ch.fftWorkBuffer.data(), true);

    if (isPrimaryAnalysisChannel)
    {
        const double sr = currentSampleRate.load();
        const float binHz = static_cast<float>(sr / static_cast<double>(fftSize));
        const float fLow  = lowCutFreqHz.load();
        const float fHigh = highCutFreqHz.load();

        std::array<float, numBins> frameMagDb {};
        for (int k = 0; k < numBins; ++k)
        {
            const float re = ch.fftWorkBuffer[static_cast<size_t>(k * 2)];
            const float im = ch.fftWorkBuffer[static_cast<size_t>(k * 2 + 1)];
            const float mag = std::sqrt(re * re + im * im) / static_cast<float>(fftSize * 0.25f);
            frameMagDb[static_cast<size_t>(k)] = juce::Decibels::gainToDecibels(mag, -96.0f);
        }

        // Compute Local Running Average & Dynamic Threshold
        // Higher sensitivity lowers the "Too Hot" threshold closer to the local average
        const float thresholdOffsetDb = 18.0f - (sensitivityVal * 0.155f); // +18 dB down to +2.5 dB

        std::array<float, numBins> localAvgDb {};
        for (int k = 1; k < numBins - 1; ++k)
        {
            const int radius = std::clamp(k / 6, 3, 28);
            const int startBin = std::max(1, k - radius);
            const int endBin   = std::min(numBins - 2, k + radius);
            float sum = 0.0f;
            for (int j = startBin; j <= endBin; ++j)
                sum += frameMagDb[static_cast<size_t>(j)];
            localAvgDb[static_cast<size_t>(k)] = sum / static_cast<float>(endBin - startBin + 1);
        }

        // Reset all target gains to 1.0 (instant event-based release the microsecond energy subsides)
        currentBinGain.fill(1.0f);

        int harmonicsInZone = 0;
        int tooHotInZone = 0;
        std::vector<HarmonicPeakInfo> framePeaks;
        framePeaks.reserve(32);

        for (int k = 2; k < numBins - 2; ++k)
        {
            const float freq = k * binHz;
            const bool insideFocus = (freq >= fLow && freq <= fHigh);

            if (auditionMode && !insideFocus)
            {
                currentBinGain[static_cast<size_t>(k)] = 0.0f;
                continue;
            }

            const float magDb = frameMagDb[static_cast<size_t>(k)];
            const float refBaseDb = std::max(localAvgDb[static_cast<size_t>(k)],
                                             baselineFloorDb[static_cast<size_t>(k)] - 12.0f);
            const float threshDb = refBaseDb + thresholdOffsetDb;
            dynamicThresholdDb[static_cast<size_t>(k)] = threshDb;

            // Detect distinct spectral harmonic peak
            const bool isLocalPeak = (magDb > frameMagDb[static_cast<size_t>(k - 1)]) &&
                                     (magDb >= frameMagDb[static_cast<size_t>(k + 1)]) &&
                                     (magDb > -68.0f) &&
                                     (magDb > localAvgDb[static_cast<size_t>(k)] + 1.8f);

            if (insideFocus && isLocalPeak)
            {
                harmonicsInZone++;
                const float excessDb = magDb - threshDb;
                const bool isHot = (excessDb > 0.0f);

                float appliedCutDb = 0.0f;
                if (isHot)
                {
                    tooHotInZone++;
                    // Independent Overtone Leveling scaled by Macro Harmonic Tame (0 to -12 dB)
                    const float macroScale = std::abs(tameDb) / 12.0f;
                    appliedCutDb = -std::min(12.0f, excessDb * 0.85f * macroScale + (macroScale * 1.5f));
                    const float linGain = juce::Decibels::decibelsToGain(appliedCutDb);

                    // Apply microsecond clamp-on to peak and immediate skirts
                    currentBinGain[static_cast<size_t>(k)]     = std::min(currentBinGain[static_cast<size_t>(k)], linGain);
                    currentBinGain[static_cast<size_t>(k - 1)] = std::min(currentBinGain[static_cast<size_t>(k - 1)],
                                                                          juce::jmap(0.5f, linGain, 1.0f));
                    currentBinGain[static_cast<size_t>(k + 1)] = std::min(currentBinGain[static_cast<size_t>(k + 1)],
                                                                          juce::jmap(0.5f, linGain, 1.0f));
                }

                if (framePeaks.size() < 24)
                {
                    framePeaks.push_back({ freq, magDb, appliedCutDb, isHot });
                }
            }
        }

        if (auditionMode)
        {
            currentBinGain[0] = 0.0f;
            currentBinGain[1] = 0.0f;
            currentBinGain[numBins - 1] = 0.0f;
        }

        activeHarmonicsCount.store(harmonicsInZone);
        tooHotHarmonicsCount.store(tooHotInZone);
        warningLightActive.store(tooHotInZone > 0);

        if (harmonicsInZone > loopTotalHarmonics.load())
            loopTotalHarmonics.store(harmonicsInZone);
        if (tooHotInZone > loopExcessiveCount.load())
            loopExcessiveCount.store(tooHotInZone);

        // Update UI visual arrays under spinlock
        {
            juce::SpinLock::ScopedTryLockType lock(visualLock);
            if (lock.isLocked())
            {
                for (int k = 0; k < numBins; ++k)
                {
                    const float inDb = frameMagDb[static_cast<size_t>(k)];
                    const float prevDb = smoothedSpectrumDb[static_cast<size_t>(k)];
                    // Fast attack, smooth visual decay
                    smoothedSpectrumDb[static_cast<size_t>(k)] = (inDb > prevDb)
                        ? (0.55f * inDb + 0.45f * prevDb)
                        : (0.18f * inDb + 0.82f * prevDb);
                }
                detectedPeaks = std::move(framePeaks);
            }
        }
    }

    // Apply per-bin harmonic attenuation gains to complex spectrum
    for (int k = 0; k < numBins; ++k)
    {
        const float g = currentBinGain[static_cast<size_t>(k)];
        ch.fftWorkBuffer[static_cast<size_t>(k * 2)]     *= g;
        ch.fftWorkBuffer[static_cast<size_t>(k * 2 + 1)] *= g;
    }

    inverseFFT.performRealOnlyInverseTransform(ch.fftWorkBuffer.data());
    window.multiplyWithWindowingTable(ch.fftWorkBuffer.data(), fftSize);

    // Hann^2 with 4x overlap sums to 1.5 -> scale by 2/3
    constexpr float olaNorm = 2.0f / 3.0f;
    int writeIdx = ch.fifoIndex;
    for (int i = 0; i < fftSize; ++i)
    {
        ch.outputAccumulator[static_cast<size_t>(writeIdx)] += ch.fftWorkBuffer[static_cast<size_t>(i)] * olaNorm;
        writeIdx = (writeIdx + 1) & (fftSize - 1);
    }
}

void SequinAudioProcessor::getVisualSnapshot(std::array<float, numBins>& outSpectrumDb,
                                             std::array<float, numBins>& outBaselineDb,
                                             std::array<float, numBins>& outThresholdDb,
                                             std::vector<HarmonicPeakInfo>& outPeaks) const
{
    juce::SpinLock::ScopedLockType lock(visualLock);
    outSpectrumDb  = smoothedSpectrumDb;
    outBaselineDb  = baselineFloorDb;
    outThresholdDb = dynamicThresholdDb;
    outPeaks       = detectedPeaks;
}

bool SequinAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* SequinAudioProcessor::createEditor()
{
    return new SequinAudioProcessorEditor(*this);
}

void SequinAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SequinAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SequinAudioProcessor();
}
