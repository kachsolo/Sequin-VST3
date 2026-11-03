#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <deque>
#include <memory>

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

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
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
    void drawTopHeader(juce::Graphics& g, float sx, float sy);
    void drawSpectrumView(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawViewModeDivider(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawRXHarmonicStackView(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawControlDeckLabels(juce::Graphics& g, float sx, float sy);

    float freqToNormX(float freqHz) const;
    float dbToNormY(float db) const;
    static juce::String formatFrequency(float hz);

    SequinAudioProcessor& audioProcessor;
    SequinLookAndFeel sequinLnF;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    // Sliders & Toggles
    juce::Slider focusSlider;
    juce::Slider sensitivitySlider;
    juce::Slider qResonanceSlider;
    juce::Slider harmonicTameSlider;
    juce::Slider mixSlider;

    juce::ToggleButton focusAuditionButton;
    juce::ToggleButton bypassButton;

    // Preset Quick Buttons
    juce::TextButton btnPresetAKG        { "AKG C2000B" };
    juce::TextButton btnPresetSibilance  { "Sibilance" };
    juce::TextButton btnPresetBoxy       { "Boxy Mids" };
    juce::TextButton btnPresetAir        { "Air & Sheen" };
    juce::TextButton btnPresetFull       { "Full Range" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> focusAttachment;
    std::unique_ptr<SliderAttachment> sensitivityAttachment;
    std::unique_ptr<SliderAttachment> qResonanceAttachment;
    std::unique_ptr<SliderAttachment> harmonicTameAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<ButtonAttachment> focusAuditionAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    std::array<float, SequinAudioProcessor::numBins> spectrumDb {};
    std::array<float, SequinAudioProcessor::numBins> baselineDb {};
    std::array<float, SequinAudioProcessor::numBins> thresholdDb {};
    std::vector<HarmonicPeakInfo> activePeaks;

    static constexpr int numStackColumns = 48;
    static constexpr int numHarmonicBands = 36;
    struct ColumnHistory
    {
        std::array<float, numHarmonicBands> bandDb {};
        std::array<bool, numHarmonicBands> bandHot {};
    };
    std::deque<ColumnHistory> rxHistoryColumns;

    float viewMorphProgress = 0.0f;
    juce::Rectangle<float> viewToggleHitBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequinAudioProcessorEditor)
};
