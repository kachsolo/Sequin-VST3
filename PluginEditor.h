#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <deque>

class SequinLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SequinLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class SequinAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit SequinAudioProcessorEditor(SequinAudioProcessor&);
    ~SequinAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    void timerCallback() override;

    void drawBrushedMetalBackground(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawTopHeader(juce::Graphics& g);
    void drawSpectrumView(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawViewModeDivider(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawRXHarmonicStackView(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawControlDeckLabels(juce::Graphics& g);

    float freqToNormX(float freqHz) const;
    float dbToNormY(float db) const;
    static juce::String formatFrequency(float hz);

    SequinAudioProcessor& audioProcessor;
    SequinLookAndFeel sequinLnF;

    // Controls
    juce::Slider focusSlider;
    juce::Slider sensitivitySlider;
    juce::Slider qResonanceSlider;
    juce::Slider harmonicTameSlider;
    juce::Slider mixSlider;

    juce::ToggleButton focusAuditionButton;
    juce::ToggleButton bypassButton;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> focusAttachment;
    std::unique_ptr<SliderAttachment> sensitivityAttachment;
    std::unique_ptr<SliderAttachment> qResonanceAttachment;
    std::unique_ptr<SliderAttachment> harmonicTameAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<ButtonAttachment> focusAuditionAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    // Display Data & Animated Morph State
    std::array<float, SequinAudioProcessor::numBins> spectrumDb {};
    std::array<float, SequinAudioProcessor::numBins> baselineDb {};
    std::array<float, SequinAudioProcessor::numBins> thresholdDb {};
    std::vector<HarmonicPeakInfo> activePeaks;

    static constexpr int numStackColumns = 44;
    static constexpr int numHarmonicBands = 32;
    struct ColumnHistory
    {
        std::array<float, numHarmonicBands> bandDb {};
        std::array<bool, numHarmonicBands> bandHot {};
    };
    std::deque<ColumnHistory> rxHistoryColumns;

    float viewMorphProgress = 0.0f; // 0.0 = Dual/Spectrum priority, 1.0 = Full RX Harmonic Stack morph
    juce::Rectangle<float> viewToggleHitBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequinAudioProcessorEditor)
};
