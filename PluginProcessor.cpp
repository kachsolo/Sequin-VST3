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
    baselineFloorDb.fill(-40.0f);
    dynamicThresholdDb.fill(-24.0f);
    currentBinGain.fill(1.0f);
    detectedPeaks.reserve(64);
}

SequinAudioProcessor::~SequinAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout SequinAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Focus Range: 80 Hz to 20 kHz
    auto freqRange = juce::NormalisableRange<float>(80.0f, 20000.0f, 1.0f);
    freqRange.setSkewForCentre(1500.0f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "focusFreq", 1 },
        "Focus",
        freqRange,
        7200.0f));

    // Q/Resonance: 0.1 to 10.0 (0.1 engages full 80 Hz - 20 kHz range)
    auto qRange = juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f);
    qRange.setSkewForCentre(2.5f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "qResonance", 1 },
        "Q / Resonance",
        qRange,
        4.5f));

    // Sensitivity: 0 to 100%
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sensitivity", 1 },
        "Sensitivity",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        78.0f));

    // Harmonic Tame: 0.0 dB to 24.0 dB of subtractive gain
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "harmonicTame", 1 },
        "Harmonic Tame",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f),
        8.5f));

    // Mix: 0 to 100%
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
        "View Mode",
        false));

    return { params.begin(), params.end() };
}

const juce::String SequinAudioProcessor::getName() const { return JucePlugin_Name; }
bool SequinAudioProcessor::acceptsMidi() const { return false; }
bool SequinAudioProcessor::producesMidi() const { return false; }
bool SequinAudioProcessor::isMidiEffect() const { return false; }
double SequinAudioProcessor::getTailLengthSeconds() const { return 0.0; }

// DAW Programs & Hardware Presets
int SequinAudioProcessor::getNumPrograms() { return 5; }
int SequinAudioProcessor::getCurrentProgram() { return currentProgram; }

void SequinAudioProcessor::setCurrentProgram(int index)
{
    loadPreset(index);
}

const juce::String SequinAudioProcessor::getProgramName(int index)
{
    switch (index)
    {
        case 0: return "AKG C2000B De-Harsh (5k-10k)";
        case 1: return "Vocal Sibilance Tamer";
        case 2: return "Boxy Low-Mids Cleaner";
        case 3: return "Air & High Sheen";
        case 4: return "Full Range Vocal Leveler";
        default: return "Default";
    }
}

void SequinAudioProcessor::changeProgramName(int, const juce::String&) {}

void SequinAudioProcessor::loadPreset(int presetIndex)
{
    currentProgram = juce::jlimit(0, 4, presetIndex);

    struct PresetValues { float freq; float q; float sens; float tame; float mix; };
    PresetValues preset;

    switch (currentProgram)
    {
        case 0: // AKG C2000B Condenser Mic Presence/Bite Tamer
            preset = { 7200.0f, 4.5f, 78.0f, 8.5f, 100.0f };
            break;
        case 1: // General Sibilance / De-Essing
            preset = { 6200.0f, 4.0f, 74.0f, 7.5f, 100.0f };
            break;
        case 2: // Boxy Low-Mids / Mud
            preset = { 420.0f, 5.5f, 52.0f, 5.0f, 100.0f };
            break;
        case 3: // Air & High Sheen
            preset = { 11500.0f, 2.2f, 65.0f, 4.0f, 100.0f };
            break;
        case 4: // Full Range Leveling (Min Q = 80Hz - 20kHz net)
            preset = { 3200.0f, 0.1f, 60.0f, 4.5f, 100.0f };
            break;
        default:
            preset = { 7200.0f, 4.5f, 78.0f, 8.5f, 100.0f };
            break;
    }

    if (auto* p = apvts.getParameter("focusFreq"))
        p->setValueNotifyingHost(p->convertTo0to1(preset.freq));
    if (auto* p = apvts.getParameter("qResonance"))
        p->setValueNotifyingHost(p->convertTo0to1(preset.q));
    if (auto* p = apvts.getParameter("sensitivity"))
        p->setValueNotifyingHost(p->convertTo0to1(preset.sens));
    if (auto* p = apvts.getParameter("harmonicTame"))
        p->setValueNotifyingHost(p->convertTo0to1(preset.tame));
    if (auto* p = apvts.getParameter("mix"))
        p->setValueNotifyingHost(p->convertTo0to1(preset.mix));
}

void SequinAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate.store(sampleRate > 0.0 ? sampleRate : 44100.0);

    for (auto& ch : stftChannels)
        ch.reset();

    currentBinGain.fill(1.0f);
    setLatencySamples(fftSize - hopSize);

    const double sr = currentSampleRate.load();
    juce::SpinLock::ScopedLockType lock(visualLock);
    for (int k = 1; k < numBins; ++k)
    {
        float freq = static_cast<float>(k * sr / static_cast<double>(fftSize));
        float octavesFrom200 = std::log2(std::max(80.0f, freq) / 200.0f);
        baselineFloorDb[static_cast<size_t>(k)] = -28.0f - 2.8f * std::max(0.0f, octavesFrom200);
    }
    baselineFloorDb[0] = -42.0f;
}

void SequinAudioProcessor::releaseResources() {}

bool SequinAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainOut == layouts.getMainInputChannelSet();
}

void SequinAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples             = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Track Transport Play/Pause State in Cubase (Fixed C4458 & C2100)
    bool playing = true;
    if (auto* ph = getPlayHead())
    {
        if (auto posInfo = ph->getPosition())
            playing = posInfo->getIsPlaying();
    }
    isDawPlaying.store(playing);

    if (playing && !wasDawPlaying)
    {
        sessionPeakHarmonics = 0;
        sessionPeakTooHot = 0;
    }
    wasDawPlaying = playing;

    const float focusHz     = apvts.getRawParameterValue("focusFreq")->load();
    const float qVal        = apvts.getRawParameterValue("qResonance")->load();
    const float sensitivity = apvts.getRawParameterValue("sensitivity")->load();
    const float tameAmount  = apvts.getRawParameterValue("harmonicTame")->load();
    const float mixPct      = apvts.getRawParameterValue("mix")->load();
    const bool auditionMode = apvts.getRawParameterValue("focusAudition")->load() > 0.5f;
    const bool bypassed     = apvts.getRawParameterValue("bypass")->load() > 0.5f;

    // Calibrated Q: At min Q (<= 0.15), plugin works on full range (80 Hz to 20 kHz)
    float fLow = 80.0f;
    float fHigh = 20000.0f;
    if (qVal > 0.15f)
    {
        const float maxOctSpan = std::log2(20000.0f / 80.0f);
        const float minOctSpan = 0.15f;
        const float normQ = (qVal - 0.15f) / (10.0f - 0.15f);
        const float octSpan = juce::jmap(std::pow(normQ, 0.55f), maxOctSpan, minOctSpan);

        fLow  = juce::jlimit(80.0f, 20000.0f, focusHz * std::pow(2.0f, -octSpan * 0.5f));
        fHigh = juce::jlimit(80.0f, 20000.0f, focusHz * std::pow(2.0f, +octSpan * 0.5f));
    }
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

            const int delayReadIdx = (stft.dryWriteIndex + hopSize) % fftSize;
            const float delayedDry = stft.dryDelayLine[static_cast<size_t>(delayReadIdx)];
            stft.dryDelayLine[static_cast<size_t>(stft.dryWriteIndex)] = inSample;
            stft.dryWriteIndex = (stft.dryWriteIndex + 1) % fftSize;

            stft.inputFifo[static_cast<size_t>(stft.fifoIndex)] = inSample;

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
                                     sensitivity, tameAmount, auditionMode);
            }
        }
    }
}

void SequinAudioProcessor::processSpectralFrame(ChannelSTFT& ch, bool isPrimaryAnalysisChannel,
                                                float /*focusHz*/, float /*qVal*/, float sensitivityVal,
                                                float tameAmount, bool auditionMode)
{
    ch.fftWorkBuffer.fill(0.0f);

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

        const float thresholdOffsetDb = 16.0f - (sensitivityVal * 0.145f);

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

        currentBinGain.fill(1.0f);

        int harmonicsInZone = 0;
        int tooHotInZone = 0;
        std::vector<HarmonicPeakInfo> framePeaks;
        framePeaks.reserve(32);

        for (int k = 2; k < numBins - 2; ++k)
        {
            const float freq = k * binHz;
            if (freq < 80.0f || freq > 20000.0f)
                continue;

            const bool insideFocus = (freq >= fLow && freq <= fHigh);

            if (auditionMode && !insideFocus)
            {
                currentBinGain[static_cast<size_t>(k)] = 0.0f;
                continue;
            }

            const float magDb = frameMagDb[static_cast<size_t>(k)];
            const float refBaseDb = std::max(localAvgDb[static_cast<size_t>(k)],
                                             baselineFloorDb[static_cast<size_t>(k)] - 10.0f);

            // Psychoacoustic Frequency-Weighting (Method 2)
            float freqTiltDb = 0.0f;
            if (freq > 3000.0f)
            {
                const float octAbove3k = std::log2(freq / 3000.0f);
                freqTiltDb = -std::min(4.5f, octAbove3k * 1.8f);
            }
            else if (freq < 700.0f)
            {
                const float octBelow700 = std::log2(700.0f / std::max(80.0f, freq));
                freqTiltDb = +std::min(3.0f, octBelow700 * 1.2f);
            }

            const float threshDb = refBaseDb + thresholdOffsetDb + freqTiltDb;
            dynamicThresholdDb[static_cast<size_t>(k)] = threshDb;

            const bool isLocalPeak = (magDb > frameMagDb[static_cast<size_t>(k - 1)]) &&
                                     (magDb >= frameMagDb[static_cast<size_t>(k + 1)]) &&
                                     (magDb > -65.0f) &&
                                     (magDb > localAvgDb[static_cast<size_t>(k)] + 1.6f);

            if (insideFocus && isLocalPeak)
            {
                harmonicsInZone++;
                const float excessDb = magDb - threshDb;
                const bool isHot = (excessDb > 0.0f);

                float appliedCutDb = 0.0f;
                if (isHot && tameAmount > 0.05f)
                {
                    tooHotInZone++;
                    const float macroScale = tameAmount / 24.0f;
                    appliedCutDb = -std::min(tameAmount, (excessDb * 1.8f + 2.5f) * macroScale);
                    const float linGain = juce::Decibels::decibelsToGain(appliedCutDb);

                    currentBinGain[static_cast<size_t>(k)] = std::min(currentBinGain[static_cast<size_t>(k)], linGain);
                    currentBinGain[static_cast<size_t>(k - 1)] = std::min(currentBinGain[static_cast<size_t>(k - 1)],
                                                                          juce::jmap(0.4f, linGain, 1.0f));
                    currentBinGain[static_cast<size_t>(k + 1)] = std::min(currentBinGain[static_cast<size_t>(k + 1)],
                                                                          juce::jmap(0.4f, linGain, 1.0f));
                }

                if (framePeaks.size() < 24)
                    framePeaks.push_back({ freq, magDb, appliedCutDb, isHot });
            }
        }

        if (auditionMode)
        {
            currentBinGain[0] = 0.0f;
            currentBinGain[1] = 0.0f;
            currentBinGain[numBins - 1] = 0.0f;
        }

        if (isDawPlaying.load())
        {
            sessionPeakHarmonics = std::max(sessionPeakHarmonics, harmonicsInZone);
            sessionPeakTooHot    = std::max(sessionPeakTooHot, tooHotInZone);
            harmonicsDetected.store(sessionPeakHarmonics);
            tooHotHarmonicsCount.store(sessionPeakTooHot);
        }
        warningLightActive.store(tooHotInZone > 0);

        {
            juce::SpinLock::ScopedTryLockType lock(visualLock);
            if (lock.isLocked())
            {
                for (int k = 0; k < numBins; ++k)
                {
                    const float inDb = frameMagDb[static_cast<size_t>(k)];
                    const float prevDb = smoothedSpectrumDb[static_cast<size_t>(k)];
                    smoothedSpectrumDb[static_cast<size_t>(k)] = (inDb > prevDb)
                        ? (0.55f * inDb + 0.45f * prevDb)
                        : (0.18f * inDb + 0.82f * prevDb);
                }
                detectedPeaks = std::move(framePeaks);
            }
        }
    }

    for (int k = 0; k < numBins; ++k)
    {
        const float g = currentBinGain[static_cast<size_t>(k)];
        ch.fftWorkBuffer[static_cast<size_t>(k * 2)]     *= g;
        ch.fftWorkBuffer[static_cast<size_t>(k * 2 + 1)] *= g;
    }

    inverseFFT.performRealOnlyInverseTransform(ch.fftWorkBuffer.data());
    window.multiplyWithWindowingTable(ch.fftWorkBuffer.data(), fftSize);

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
