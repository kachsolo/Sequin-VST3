#include "PluginEditor.h"
#include <cmath>
#include <iomanip>
#include <sstream>

SequinLookAndFeel::SequinLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff7ce8ff));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void SequinLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    const bool isMainTameKnob = (slider.getName() == "HarmonicTame");
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height)).reduced(6.0f);
    const float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Outer recessed shadow ring
    const float outerRadius = radius * 0.88f;
    g.setColour(juce::Colour(0xff0b0d10));
    g.fillEllipse(centre.x - outerRadius - 3.0f, centre.y - outerRadius - 3.0f,
                  (outerRadius + 3.0f) * 2.0f, (outerRadius + 3.0f) * 2.0f);

    // Value Arc Track
    juce::Path bgArc;
    bgArc.addCentredArc(centre.x, centre.y, outerRadius + 5.0f, outerRadius + 5.0f,
                        0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff252a32));
    g.strokePath(bgArc, juce::PathStrokeType(isMainTameKnob ? 3.5f : 2.8f,
                                             juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    // Active Glowing Arc
    juce::Path valArc;
    valArc.addCentredArc(centre.x, centre.y, outerRadius + 5.0f, outerRadius + 5.0f,
                         0.0f, rotaryStartAngle, toAngle, true);
    g.setColour(isMainTameKnob ? juce::Colour(0xffe8f4ff) : juce::Colour(0xff52b8ff));
    g.strokePath(valArc, juce::PathStrokeType(isMainTameKnob ? 3.8f : 3.0f,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    // Metallic Bezel
    const float bodyRadius = radius * 0.74f;
    juce::ColourGradient bezelGrad(juce::Colour(0xff525862), centre.x - bodyRadius, centre.y - bodyRadius,
                                   juce::Colour(0xff191c20), centre.x + bodyRadius, centre.y + bodyRadius, false);
    g.setGradientFill(bezelGrad);
    g.fillEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

    // Inner Cap (Iridescent sheen on Main Harmonic Tame knob, dark bronze/gunmetal on others)
    const float capRadius = bodyRadius * 0.84f;
    if (isMainTameKnob)
    {
        juce::ColourGradient iridGrad(juce::Colour(0xff2c4d63), centre.x - capRadius, centre.y - capRadius,
                                      juce::Colour(0xff5b2a42), centre.x + capRadius, centre.y + capRadius, false);
        iridGrad.addColour(0.35, juce::Colour(0xff2d333b));
        iridGrad.addColour(0.68, juce::Colour(0xff3c584e));
        g.setGradientFill(iridGrad);
    }
    else
    {
        juce::ColourGradient capGrad(juce::Colour(0xff3d3732), centre.x, centre.y - capRadius,
                                     juce::Colour(0xff181a1d), centre.x, centre.y + capRadius, false);
        g.setGradientFill(capGrad);
    }
    g.fillEllipse(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

    // Subtle specular highlight ring
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawEllipse(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.2f);

    // Pointer Indicator Notch
    juce::Path pointer;
    const float pointerLen = capRadius * 0.82f;
    pointer.addRoundedRectangle(-1.8f, -pointerLen, 3.6f, capRadius * 0.38f, 1.5f);
    g.setColour(isMainTameKnob ? juce::Colour(0xffff5a6e) : juce::Colour(0xffffb84d));
    g.fillPath(pointer, juce::AffineTransform::rotation(toAngle).translated(centre.x, centre.y));
}

void SequinLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                         const juce::Slider::SliderStyle, juce::Slider& /*slider*/)
{
    const float trackWidth = 22.0f;
    const float trackX = static_cast<float>(x) + (static_cast<float>(width) - trackWidth) * 0.5f;
    const auto trackRect = juce::Rectangle<float>(trackX, static_cast<float>(y),
                                                  trackWidth, static_cast<float>(height));

    // Recessed Vertical Slot
    g.setColour(juce::Colour(0xff0b0e12));
    g.fillRoundedRectangle(trackRect, 4.0f);
    g.setColour(juce::Colour(0xff343a42));
    g.drawRoundedRectangle(trackRect, 4.0f, 1.2f);

    // Left LED Calibration Ladder Ticks
    const int numTicks = 42;
    for (int i = 0; i <= numTicks; ++i)
    {
        const float normY = static_cast<float>(i) / static_cast<float>(numTicks);
        const float ty = trackRect.getY() + normY * trackRect.getHeight();
        const bool nearThumb = std::abs(ty - sliderPos) < 34.0f;
        g.setColour(nearThumb ? juce::Colour(0xffffaa33) : juce::Colour(0xff38c878).withAlpha(0.65f));
        const float tickW = (i % 5 == 0) ? 6.0f : 3.5f;
        g.fillRect(trackRect.getX() - tickW - 3.0f, ty - 0.8f, tickW, 1.6f);
    }

    // Translucent Illuminated Range Thumb
    const float thumbHeight = 76.0f;
    const float thumbY = juce::jlimit(trackRect.getY(),
                                      trackRect.getBottom() - thumbHeight,
                                      sliderPos - thumbHeight * 0.5f);
    const auto thumbRect = juce::Rectangle<float>(trackRect.getX() + 2.0f, thumbY,
                                                  trackRect.getWidth() - 4.0f, thumbHeight);

    juce::ColourGradient thumbGrad(juce::Colour(0xaa988a75), thumbRect.getX(), thumbRect.getY(),
                                   juce::Colour(0xaa594e3f), thumbRect.getRight(), thumbRect.getBottom(), false);
    g.setGradientFill(thumbGrad);
    g.fillRoundedRectangle(thumbRect, 4.0f);

    g.setColour(juce::Colour(0xffffb84d).withAlpha(0.85f));
    g.drawRoundedRectangle(thumbRect, 4.0f, 1.3f);

    // Center Hairline on Thumb
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawHorizontalLine(static_cast<int>(thumbRect.getCentreY()),
                         thumbRect.getX() + 3.0f, thumbRect.getRight() - 3.0f);
}

void SequinLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                         bool /*highlighted*/, bool /*down*/)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const bool isOn = button.getToggleState();

    if (button.getName() == "Bypass")
    {
        // Round Glowing Amber Bypass Button
        const float diameter = std::min(bounds.getWidth(), bounds.getHeight()) - 10.0f;
        const auto circle = bounds.withSizeKeepingCentre(diameter, diameter);

        if (!isOn) // Active processing illuminates brightly, or illuminates when engaged
        {
            juce::ColourGradient glow(juce::Colour(0xffffdd88), circle.getCentreX(), circle.getCentreY(),
                                      juce::Colour(0x00ff9900), circle.getX() - 10.0f, circle.getY() - 10.0f, true);
            g.setGradientFill(glow);
            g.fillEllipse(circle.expanded(7.0f));
        }

        juce::ColourGradient btnFill(isOn ? juce::Colour(0xff554838) : juce::Colour(0xfffff2cc),
                                     circle.getCentreX(), circle.getY(),
                                     isOn ? juce::Colour(0xff2a241c) : juce::Colour(0xffe69525),
                                     circle.getCentreX(), circle.getBottom(), false);
        g.setGradientFill(btnFill);
        g.fillEllipse(circle);

        g.setColour(juce::Colour(0xff22262a));
        g.drawEllipse(circle, 2.0f);
    }
    else
    {
        // Focus Audition Pill Switch
        const auto pill = bounds.withSizeKeepingCentre(44.0f, 18.0f);
        g.setColour(isOn ? juce::Colour(0xffcc7a1a) : juce::Colour(0xff14171c));
        g.fillRoundedRectangle(pill, 9.0f);
        g.setColour(juce::Colour(0xff454c56));
        g.drawRoundedRectangle(pill, 9.0f, 1.2f);

        const float knobX = isOn ? (pill.getRight() - 15.0f) : (pill.getX() + 3.0f);
        g.setColour(isOn ? juce::Colour(0xfffff0d0) : juce::Colour(0xff9aa2ad));
        g.fillEllipse(knobX, pill.getY() + 3.0f, 12.0f, 12.0f);
    }
}

//==============================================================================
SequinAudioProcessorEditor::SequinAudioProcessorEditor(SequinAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&sequinLnF);
    setSize(760, 960);

    // Initialize RX History Columns
    ColumnHistory emptyCol {};
    emptyCol.bandDb.fill(-90.0f);
    emptyCol.bandHot.fill(false);
    rxHistoryColumns.resize(numStackColumns, emptyCol);

    // 1. Focus Vertical Slider
    focusSlider.setSliderStyle(juce::Slider::LinearVertical);
    focusSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(focusSlider);
    focusAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "focusFreq", focusSlider);

    // 2. Sensitivity Knob
    sensitivitySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sensitivitySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(sensitivitySlider);
    sensitivityAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "sensitivity", sensitivitySlider);

    // 3. Q / Resonance Knob
    qResonanceSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    qResonanceSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(qResonanceSlider);
    qResonanceAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "qResonance", qResonanceSlider);

    // 4. Harmonic Tame Main Knob
    harmonicTameSlider.setName("HarmonicTame");
    harmonicTameSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    harmonicTameSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(harmonicTameSlider);
    harmonicTameAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "harmonicTame", harmonicTameSlider);

    // 5. Mix Knob
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(mixSlider);
    mixAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "mix", mixSlider);

    // 6. Focus Audition Switch
    focusAuditionButton.setName("FocusAudition");
    addAndMakeVisible(focusAuditionButton);
    focusAuditionAttachment = std::make_unique<ButtonAttachment>(audioProcessor.getAPVTS(), "focusAudition", focusAuditionButton);

    // 7. Bypass Button
    bypassButton.setName("Bypass");
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<ButtonAttachment>(audioProcessor.getAPVTS(), "bypass", bypassButton);

    startTimerHz(30);
}

SequinAudioProcessorEditor::~SequinAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void SequinAudioProcessorEditor::timerCallback()
{
    audioProcessor.getVisualSnapshot(spectrumDb, baselineDb, thresholdDb, activePeaks);

    // Smoothly animate Dual View Mode morph
    const bool targetRxMode = audioProcessor.getAPVTS().getRawParameterValue("viewMode")->load() > 0.5f;
    const float targetMorph = targetRxMode ? 1.0f : 0.0f;
    viewMorphProgress += (targetMorph - viewMorphProgress) * 0.22f;

    // Push latest spectral column into RX Harmonic Stack rolling history
    ColumnHistory col {};
    const double sr = audioProcessor.getCurrentSampleRate();
    const float binHz = static_cast<float>(sr / static_cast<double>(SequinAudioProcessor::fftSize));

    for (int b = 0; b < numHarmonicBands; ++b)
    {
        const float normMin = static_cast<float>(b) / static_cast<float>(numHarmonicBands);
        const float normMax = static_cast<float>(b + 1) / static_cast<float>(numHarmonicBands);
        const float f0 = 20.0f * std::pow(1000.0f, normMin);
        const float f1 = 20.0f * std::pow(1000.0f, normMax);

        const int k0 = std::clamp(static_cast<int>(f0 / binHz), 1, SequinAudioProcessor::numBins - 1);
        const int k1 = std::clamp(static_cast<int>(f1 / binHz), k0, SequinAudioProcessor::numBins - 1);

        float maxDb = -96.0f;
        bool hot = false;
        for (int k = k0; k <= k1; ++k)
        {
            maxDb = std::max(maxDb, spectrumDb[static_cast<size_t>(k)]);
            if (spectrumDb[static_cast<size_t>(k)] > thresholdDb[static_cast<size_t>(k)])
                hot = true;
        }
        col.bandDb[static_cast<size_t>(b)]  = maxDb;
        col.bandHot[static_cast<size_t>(b)] = hot;
    }

    rxHistoryColumns.pop_front();
    rxHistoryColumns.push_back(col);

    repaint();
}

float SequinAudioProcessorEditor::freqToNormX(float freqHz) const
{
    const float clamped = juce::jlimit(20.0f, 20000.0f, freqHz);
    return std::log10(clamped / 20.0f) / 3.0f; // log10(20000/20) = 3.0
}

float SequinAudioProcessorEditor::dbToNormY(float db) const
{
    const float clamped = juce::jlimit(-84.0f, 0.0f, db);
    return 1.0f - ((clamped + 84.0f) / 84.0f);
}

juce::String SequinAudioProcessorEditor::formatFrequency(float hz)
{
    std::ostringstream oss;
    if (hz >= 1000.0f)
        oss << std::fixed << std::setprecision(1) << (hz / 1000.0f) << "kHz";
    else
        oss << static_cast<int>(std::round(hz)) << "Hz";
    return oss.str();
}

void SequinAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (viewToggleHitBox.contains(event.position))
    {
        if (auto* param = audioProcessor.getAPVTS().getParameter("viewMode"))
        {
            const bool current = param->getValue() > 0.5f;
            param->setValueNotifyingHost(current ? 0.0f : 1.0f);
        }
    }
}

void SequinAudioProcessorEditor::drawBrushedMetalBackground(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    juce::ColourGradient chassisGrad(juce::Colour(0xff2a2e35), 0.0f, 0.0f,
                                     juce::Colour(0xff171a1f), 0.0f, static_cast<float>(bounds.getHeight()), false);
    chassisGrad.addColour(0.5, juce::Colour(0xff22262c));
    g.setGradientFill(chassisGrad);
    g.fillRect(bounds);

    // Subtle horizontal brushed-metal texture lines
    g.setColour(juce::Colours::white.withAlpha(0.015f));
    for (int y = 0; y < bounds.getHeight(); y += 3)
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(bounds.getWidth()));

    // Outer metallic bevel border
    g.setColour(juce::Colour(0xff484f5a));
    g.drawRect(bounds, 2);
}

void SequinAudioProcessorEditor::drawTopHeader(juce::Graphics& g)
{
    // Metallic Header Plate
    juce::Rectangle<float> headerBounds(16.0f, 10.0f, 728.0f, 70.0f);
    juce::ColourGradient headerGrad(juce::Colour(0xff323740), 380.0f, 10.0f,
                                    juce::Colour(0xff181b20), 380.0f, 80.0f, false);
    g.setGradientFill(headerGrad);
    g.fillRoundedRectangle(headerBounds, 6.0f);

    // Gold Metallic Title "SEQUIN"
    juce::ColourGradient goldGrad(juce::Colour(0xffffecb3), 380.0f, 16.0f,
                                  juce::Colour(0xffb5842b), 380.0f, 52.0f, false);
    g.setGradientFill(goldGrad);
    g.setFont(juce::Font(34.0f, juce::Font::bold));
    g.drawText("SEQUIN", 0, 14, getWidth(), 36, juce::Justification::centred);

    g.setColour(juce::Colour(0xffa9b2bd));
    g.setFont(juce::Font(14.5f, juce::Font::plain));
    g.drawText("Harmonic Vocal Processor", 0, 48, getWidth(), 22, juce::Justification::centred);

    // Top-Right Telemetry Readout Pill ("Harmonics: 14 / Too Hot: 2" & "Loop: 14 / 2 excessive")
    juce::Rectangle<float> telemetryBox(532.0f, 18.0f, 198.0f, 48.0f);
    g.setColour(juce::Colour(0xff0d1117));
    g.fillRoundedRectangle(telemetryBox, 6.0f);
    g.setColour(juce::Colour(0xff3a4654));
    g.drawRoundedRectangle(telemetryBox, 6.0f, 1.3f);

    const int harmCount = audioProcessor.activeHarmonicsCount.load();
    const int hotCount  = audioProcessor.tooHotHarmonicsCount.load();
    const int loopTotal = audioProcessor.loopTotalHarmonics.load();
    const int loopHot   = audioProcessor.loopExcessiveCount.load();

    g.setColour(juce::Colour(0xff9ad8ff));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("Harmonics: " + juce::String(harmCount) + " / Too Hot: " + juce::String(hotCount),
               telemetryBox.getX() + 8.0f, telemetryBox.getY() + 6.0f,
               telemetryBox.getWidth() - 16.0f, 18.0f, juce::Justification::centred);

    g.setColour(juce::Colour(0xff7aa5c4));
    g.setFont(juce::Font(12.5f, juce::Font::plain));
    g.drawText("Loop: " + juce::String(loopTotal) + " / " + juce::String(loopHot) + " excessive",
               telemetryBox.getX() + 8.0f, telemetryBox.getY() + 24.0f,
               telemetryBox.getWidth() - 16.0f, 18.0f, juce::Justification::centred);
}

void SequinAudioProcessorEditor::drawSpectrumView(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.saveState();
    g.reduceClipRegion(bounds.toNearestInt());

    // Dark CRT/Analyzer Backing
    juce::ColourGradient bgGrad(juce::Colour(0xff0a0e13), bounds.getX(), bounds.getY(),
                                juce::Colour(0xff0f171e), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(bgGrad);
    g.fillRect(bounds);

    // Subtle Grid Lines & Frequency Labels
    const float gridFreqs[] = { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f };
    const char* gridLabels[] = { "20Hz", "50Hz", "100Hz", "200Hz", "500Hz", "1kHz", "5kHz", "10kHz" };

    g.setFont(juce::Font(11.5f, juce::Font::plain));
    for (int i = 0; i < 8; ++i)
    {
        const float nx = freqToNormX(gridFreqs[i]);
        const float gx = bounds.getX() + nx * bounds.getWidth();
        g.setColour(juce::Colour(0xff1c2630));
        g.drawVerticalLine(static_cast<int>(gx), bounds.getY(), bounds.getBottom() - 20.0f);
        g.setColour(juce::Colour(0xff7f8c99));
        g.drawText(gridLabels[i], static_cast<int>(gx) - 18, static_cast<int>(bounds.getBottom()) - 19,
                   48, 16, juce::Justification::left);
    }

    for (int row = 1; row < 5; ++row)
    {
        const float gy = bounds.getY() + (static_cast<float>(row) / 5.0f) * (bounds.getHeight() - 22.0f);
        g.setColour(juce::Colour(0xff18212a));
        g.drawHorizontalLine(static_cast<int>(gy), bounds.getX(), bounds.getRight());
    }

    const float plotBottom = bounds.getBottom() - 22.0f;
    const float plotHeight = plotBottom - bounds.getY();
    const double sr = audioProcessor.getCurrentSampleRate();
    const float binHz = static_cast<float>(sr / static_cast<double>(SequinAudioProcessor::fftSize));

    // 1. Live Spectrum Curve + Translucent Green Fill
    juce::Path specPath;
    juce::Path specFill;
    bool started = false;

    for (int k = 1; k < SequinAudioProcessor::numBins; ++k)
    {
        const float freq = k * binHz;
        if (freq < 20.0f || freq > 20000.0f)
            continue;

        const float x = bounds.getX() + freqToNormX(freq) * bounds.getWidth();
        const float y = bounds.getY() + dbToNormY(spectrumDb[static_cast<size_t>(k)]) * plotHeight;

        if (!started)
        {
            specPath.startNewSubPath(x, y);
            specFill.startNewSubPath(x, plotBottom);
            specFill.lineTo(x, y);
            started = true;
        }
        else
        {
            specPath.lineTo(x, y);
            specFill.lineTo(x, y);
        }
    }

    if (started)
    {
        specFill.lineTo(bounds.getRight(), plotBottom);
        specFill.closeSubPath();

        juce::ColourGradient fillGrad(juce::Colour(0x6638d878), bounds.getX(), bounds.getY() + 30.0f,
                                      juce::Colour(0x08103820), bounds.getX(), plotBottom, false);
        g.setGradientFill(fillGrad);
        g.fillPath(specFill);

        g.setColour(juce::Colour(0xff65e892));
        g.strokePath(specPath, juce::PathStrokeType(1.5f));
    }

    // 2. Green Frequency-Calibrated Baseline Floor (Healthy structural vocal reference)
    juce::Path baselinePath;
    bool baseStarted = false;
    for (int k = 1; k < SequinAudioProcessor::numBins; k += 4)
    {
        const float freq = k * binHz;
        if (freq < 20.0f || freq > 20000.0f)
            continue;
        const float x = bounds.getX() + freqToNormX(freq) * bounds.getWidth();
        const float y = bounds.getY() + dbToNormY(baselineDb[static_cast<size_t>(k)]) * plotHeight;
        if (!baseStarted) { baselinePath.startNewSubPath(x, y); baseStarted = true; }
        else { baselinePath.lineTo(x, y); }
    }
    g.setColour(juce::Colour(0xff2aff55));
    g.strokePath(baselinePath, juce::PathStrokeType(2.0f));

    // 3. Dynamic Dashed Red "Too Hot" Threshold Line
    const float sensVal = audioProcessor.getAPVTS().getRawParameterValue("sensitivity")->load();
    const float threshVisualDb = -16.0f - (sensVal * 0.32f);
    const float threshY = bounds.getY() + dbToNormY(threshVisualDb) * plotHeight;

    juce::Path dashedRed;
    dashedRed.startNewSubPath(bounds.getX(), threshY);
    dashedRed.lineTo(bounds.getRight(), threshY);
    const float dashLengths[] = { 6.0f, 4.0f };
    juce::Path strokedDash;
    juce::PathStrokeType(1.4f).createDashedStroke(strokedDash, dashedRed, dashLengths, 2);
    g.setColour(juce::Colour(0xffff4242).withAlpha(0.85f));
    g.fillPath(strokedDash);

    // 4. Interactive Bandpass Shading Window
    const float fLow  = audioProcessor.lowCutFreqHz.load();
    const float fHigh = audioProcessor.highCutFreqHz.load();
    const float xLow  = bounds.getX() + freqToNormX(fLow) * bounds.getWidth();
    const float xHigh = bounds.getX() + freqToNormX(fHigh) * bounds.getWidth();

    juce::Rectangle<float> bandRect(xLow, bounds.getY(), std::max(4.0f, xHigh - xLow), plotHeight);
    juce::ColourGradient bandGrad(juce::Colour(0x55ff9d2e), bandRect.getCentreX(), bandRect.getY(),
                                  juce::Colour(0x22ff7700), bandRect.getCentreX(), bandRect.getBottom(), false);
    g.setGradientFill(bandGrad);
    g.fillRect(bandRect);

    g.setColour(juce::Colour(0xffffaa3b).withAlpha(0.85f));
    g.drawVerticalLine(static_cast<int>(bandRect.getX()), bandRect.getY(), bandRect.getBottom());
    g.drawVerticalLine(static_cast<int>(bandRect.getRight()), bandRect.getY(), bandRect.getBottom());

    // 5. Pinned Harmonic Attenuation Pills (e.g., -3.2 dB, -1.1 dB) above targeted red harmonic spikes
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    int pillsDrawn = 0;
    float lastPillX = -100.0f;

    for (const auto& peak : activePeaks)
    {
        if (!peak.isTooHot || peak.attenuationDb >= -0.15f)
            continue;

        const float px = bounds.getX() + freqToNormX(peak.frequencyHz) * bounds.getWidth();
        const float py = bounds.getY() + dbToNormY(peak.magnitudeDb) * plotHeight;

        // Highlight targeted red harmonic spike
        g.setColour(juce::Colour(0xffff3838));
        g.drawLine(px, py, px, plotBottom, 2.2f);
        g.fillEllipse(px - 3.0f, py - 3.0f, 6.0f, 6.0f);

        if (std::abs(px - lastPillX) > 48.0f && pillsDrawn < 4)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << peak.attenuationDb << " dB";
            const juce::String pillText = oss.str();

            juce::Rectangle<float> pillBox(px - 24.0f, std::max(bounds.getY() + 4.0f, py - 24.0f), 48.0f, 17.0f);
            g.setColour(juce::Colour(0xdd2a0c10));
            g.fillRoundedRectangle(pillBox, 4.0f);
            g.setColour(juce::Colour(0xffff4d4d));
            g.drawRoundedRectangle(pillBox, 4.0f, 1.0f);

            g.setColour(juce::Colours::white);
            g.drawText(pillText, pillBox.toNearestInt(), juce::Justification::centred);

            lastPillX = px;
            pillsDrawn++;
        }
    }

    g.restoreState();
}

void SequinAudioProcessorEditor::drawViewModeDivider(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour(juce::Colour(0xff12171e));
    g.fillRect(bounds);

    // Selective Warning Indicator (Left Pill — stays dark normally, flashes brightly when "Too Hot" spikes)
    juce::Rectangle<float> warnPill(bounds.getX() + 12.0f, bounds.getY() + 4.0f, 88.0f, bounds.getHeight() - 8.0f);
    const bool warnOn = audioProcessor.warningLightActive.load();

    g.setColour(juce::Colour(0xff1e242c));
    g.fillRoundedRectangle(warnPill, 7.0f);
    g.setColour(juce::Colour(0xff38404a));
    g.drawRoundedRectangle(warnPill, 7.0f, 1.0f);

    if (warnOn)
    {
        juce::ColourGradient warnGlow(juce::Colour(0xffffcc44), warnPill.getRight() - 12.0f, warnPill.getCentreY(),
                                      juce::Colour(0xffcc2200), warnPill.getX() + 10.0f, warnPill.getCentreY(), false);
        g.setGradientFill(warnGlow);
        g.fillRoundedRectangle(warnPill.reduced(2.0f), 5.0f);
        g.setColour(juce::Colour(0xfffffae0));
        g.fillEllipse(warnPill.getRight() - 16.0f, warnPill.getCentreY() - 5.0f, 10.0f, 10.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff3b2c28));
        g.fillEllipse(warnPill.getRight() - 16.0f, warnPill.getCentreY() - 5.0f, 10.0f, 10.0f);
    }

    // Center Interactive Toggle: VIEW MODE: SPECTRUM | SPECTROGRAM
    viewToggleHitBox = juce::Rectangle<float>(bounds.getCentreX() - 140.0f, bounds.getY(), 280.0f, bounds.getHeight());
    const bool isRxMode = audioProcessor.getAPVTS().getRawParameterValue("viewMode")->load() > 0.5f;

    g.setFont(juce::Font(12.5f, juce::Font::bold));
    g.setColour(isRxMode ? juce::Colour(0xff7890a8) : juce::Colour(0xff52c8ff));
    g.drawText("VIEW MODE: SPECTRUM", static_cast<int>(bounds.getCentreX() - 145.0f),
               static_cast<int>(bounds.getY()), 145, static_cast<int>(bounds.getHeight()),
               juce::Justification::centredRight);

    g.setColour(juce::Colour(0xff6c7a89));
    g.drawText("|", static_cast<int>(bounds.getCentreX() - 2.0f),
               static_cast<int>(bounds.getY()), 10, static_cast<int>(bounds.getHeight()),
               juce::Justification::centred);

    g.setColour(isRxMode ? juce::Colour(0xff52c8ff) : juce::Colour(0xff7890a8));
    g.drawText("SPECTROGRAM", static_cast<int>(bounds.getCentreX() + 10.0f),
               static_cast<int>(bounds.getY()), 115, static_cast<int>(bounds.getHeight()),
               juce::Justification::centredLeft);

    // Right Loop Telemetry Summary
    const int loopTotal = audioProcessor.loopTotalHarmonics.load();
    const int loopHot   = audioProcessor.loopExcessiveCount.load();
    g.setColour(juce::Colour(0xff9ab0c4));
    g.setFont(juce::Font(12.0f, juce::Font::plain));
    g.drawText("Loop: " + juce::String(loopTotal) + " / " + juce::String(loopHot) + " excessive   >",
               static_cast<int>(bounds.getRight() - 185.0f), static_cast<int>(bounds.getY()),
               175, static_cast<int>(bounds.getHeight()), juce::Justification::centredRight);
}

void SequinAudioProcessorEditor::drawRXHarmonicStackView(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.saveState();
    g.reduceClipRegion(bounds.toNearestInt());

    g.setColour(juce::Colour(0xff070a0f));
    g.fillRect(bounds);

    // RX Harmonic Stacks: Overtones appear as horizontal lines forming vertical columns,
    // with natural thickness reduction as you move higher up the frequency spectrum.
    const float colWidth = bounds.getWidth() / static_cast<float>(numStackColumns);
    const float bandHeight = bounds.getHeight() / static_cast<float>(numHarmonicBands);

    for (size_t c = 0; c < rxHistoryColumns.size(); ++c)
    {
        const auto& col = rxHistoryColumns[c];
        const float x = bounds.getX() + static_cast<float>(c) * colWidth;

        for (int b = 0; b < numHarmonicBands; ++b)
        {
            const float db = col.bandDb[static_cast<size_t>(b)];
            const float intensity = juce::jlimit(0.0f, 1.0f, (db + 78.0f) / 66.0f);
            if (intensity < 0.08f)
                continue;

            const float normFreqHeight = static_cast<float>(b) / static_cast<float>(numHarmonicBands - 1);
            const float y = bounds.getBottom() - (static_cast<float>(b + 1) * bandHeight);

            // Natural thickness reduction higher up the frequency spectrum
            const float maxThickness = juce::jmap(normFreqHeight, 0.0f, 1.0f, bandHeight * 0.88f, bandHeight * 0.28f);
            const float lineThickness = std::max(1.0f, maxThickness * (0.5f + 0.5f * intensity));

            juce::Colour barColour;
            if (col.bandHot[static_cast<size_t>(b)] || intensity > 0.82f)
                barColour = juce::Colour(0xffff5522).interpolatedWith(juce::Colour(0xffffdd44), intensity);
            else if (intensity > 0.45f)
                barColour = juce::Colour(0xff28c870).interpolatedWith(juce::Colour(0xffffaa22), (intensity - 0.45f) / 0.37f);
            else
                barColour = juce::Colour(0xff103868).interpolatedWith(juce::Colour(0xff28c870), intensity / 0.45f);

            g.setColour(barColour.withAlpha(0.25f + 0.75f * intensity));
            g.fillRect(x + 1.0f, y + (bandHeight - lineThickness) * 0.5f, colWidth - 2.0f, lineThickness);
        }
    }

    // Highlight active Focus range on RX Spectrogram
    const float fLow  = audioProcessor.lowCutFreqHz.load();
    const float fHigh = audioProcessor.highCutFreqHz.load();
    const float xLow  = bounds.getX() + freqToNormX(fLow) * bounds.getWidth();
    const float xHigh = bounds.getX() + freqToNormX(fHigh) * bounds.getWidth();
    g.setColour(juce::Colour(0x28ffaa33));
    g.fillRect(xLow, bounds.getY(), std::max(3.0f, xHigh - xLow), bounds.getHeight());

    g.restoreState();
}

void SequinAudioProcessorEditor::drawControlDeckLabels(juce::Graphics& g)
{
    // Lower Control Panel Divider Lines
    g.setColour(juce::Colour(0xff14171c));
    g.drawVerticalLine(390, 480.0f, 930.0f);
    g.drawHorizontalLine(472, 24.0f, 736.0f);

    // Column Headers
    g.setColour(juce::Colour(0xffd0d7e0));
    g.setFont(juce::Font(15.0f, juce::Font::bold));
    g.drawText("FOCUS", 42, 488, 150, 20, juce::Justification::centred);
    g.drawText("SENSITIVITY", 215, 488, 150, 20, juce::Justification::centred);
    g.drawText("HARMONIC TAME", 405, 488, 165, 20, juce::Justification::centred);
    g.drawText("MIX", 580, 488, 140, 20, juce::Justification::centred);

    g.setColour(juce::Colour(0xff9aa4b0));
    g.setFont(juce::Font(13.0f, juce::Font::plain));
    g.drawText("(FREQUENCY)", 42, 506, 150, 18, juce::Justification::centred);
    g.drawText("Too Hot", 215, 506, 150, 18, juce::Justification::centred);
    g.drawText("(Main)", 405, 506, 165, 18, juce::Justification::centred);
    g.drawText("(%)", 580, 506, 140, 18, juce::Justification::centred);

    // Focus Slider Scale Labels
    g.setColour(juce::Colour(0xffc8d0d8));
    g.setFont(juce::Font(12.5f, juce::Font::bold));
    g.drawText("20kHz", 44, 535, 48, 16, juce::Justification::right);
    g.drawText("5.7", 44, 650, 48, 16, juce::Justification::right);
    g.drawText("1.8", 44, 710, 48, 16, juce::Justification::right);
    g.drawText("3.2", 44, 780, 48, 16, juce::Justification::right);
    g.drawText("20Hz", 44, 855, 48, 16, juce::Justification::right);

    // Orange Bracket & Readouts next to Focus Slider
    const float fLow  = audioProcessor.lowCutFreqHz.load();
    const float fHigh = audioProcessor.highCutFreqHz.load();
    const float sliderTop = 540.0f;
    const float sliderBot = 865.0f;
    const float thumbY = sliderTop + (1.0f - static_cast<float>(focusSlider.valueToProportionOfLength(focusSlider.getValue()))) * (sliderBot - sliderTop);
    const float bracketTop = juce::jlimit(sliderTop, sliderBot - 60.0f, thumbY - 42.0f);
    const float bracketBot = juce::jlimit(bracketTop + 30.0f, sliderBot, thumbY + 42.0f);

    g.setColour(juce::Colour(0xffff9d2e));
    g.drawLine(134.0f, bracketTop, 141.0f, bracketTop, 2.0f);
    g.drawLine(141.0f, bracketTop, 141.0f, bracketBot, 2.0f);
    g.drawLine(134.0f, bracketBot, 141.0f, bracketBot, 2.0f);

    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText(formatFrequency(fHigh), 146, static_cast<int>(bracketTop) - 8, 68, 18, juce::Justification::left);
    g.drawText(formatFrequency(fLow), 146, static_cast<int>(bracketBot) - 8, 68, 18, juce::Justification::left);

    // Bottom Left Banner: CURRENT RANGE: 3.2kHz - 3.7kHz
    juce::Rectangle<float> rangeBox(42.0f, 885.0f, 255.0f, 34.0f);
    g.setColour(juce::Colour(0xff14110e));
    g.fillRoundedRectangle(rangeBox, 4.0f);
    g.setColour(juce::Colour(0xff8c5820));
    g.drawRoundedRectangle(rangeBox, 4.0f, 1.2f);
    g.setColour(juce::Colour(0xffffb84d));
    g.setFont(juce::Font(14.5f, juce::Font::bold));
    g.drawText("CURRENT RANGE: " + formatFrequency(fLow) + " - " + formatFrequency(fHigh),
               rangeBox.toNearestInt(), juce::Justification::centred);

    // Digital Readout Pill under Sensitivity Knob
    juce::Rectangle<float> sensPill(252.0f, 646.0f, 76.0f, 22.0f);
    g.setColour(juce::Colour(0xff0d131a));
    g.fillRoundedRectangle(sensPill, 4.0f);
    g.setColour(juce::Colour(0xff7ce8ff));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    const float sensVal = audioProcessor.getAPVTS().getRawParameterValue("sensitivity")->load();
    g.drawText(juce::String(sensVal, 1) + "%", sensPill.toNearestInt(), juce::Justification::centred);

    // Q / RESONANCE & FOCUS AUDITION Labels
    g.setColour(juce::Colour(0xffc0c8d2));
    g.setFont(juce::Font(13.5f, juce::Font::bold));
    g.drawText("Q/RESONANCE", 215, 792, 150, 20, juce::Justification::centred);
    g.drawText("FOCUS AUDITION", 215, 862, 150, 20, juce::Justification::centred);

    // Main Harmonic Tame Scale & Readout Pill (0 dB to -12 dB)
    g.setColour(juce::Colour(0xffb8c2cc));
    g.setFont(juce::Font(12.0f, juce::Font::plain));
    g.drawText("0", 478, 524, 20, 16, juce::Justification::centred);
    g.drawText("0 dB", 412, 668, 42, 16, juce::Justification::centred);
    g.drawText("-12 dB", 522, 668, 48, 16, juce::Justification::centred);

    juce::Rectangle<float> tamePill(452.0f, 675.0f, 72.0f, 24.0f);
    g.setColour(juce::Colour(0xff0d131a));
    g.fillRoundedRectangle(tamePill, 4.0f);
    g.setColour(juce::Colour(0xff2c4558));
    g.drawRoundedRectangle(tamePill, 4.0f, 1.0f);
    g.setColour(juce::Colour(0xff7ce8ff));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    const float tameVal = audioProcessor.getAPVTS().getRawParameterValue("harmonicTame")->load();
    g.drawText(juce::String(tameVal, 1) + " dB", tamePill.toNearestInt(), juce::Justification::centred);

    // Mix Readout Pill (0% to 100%)
    g.setColour(juce::Colour(0xffb8c2cc));
    g.setFont(juce::Font(12.0f, juce::Font::plain));
    g.drawText("0%", 595, 632, 32, 16, juce::Justification::centred);
    g.drawText("100%", 676, 632, 40, 16, juce::Justification::centred);

    juce::Rectangle<float> mixPill(616.0f, 646.0f, 68.0f, 22.0f);
    g.setColour(juce::Colour(0xff0d131a));
    g.fillRoundedRectangle(mixPill, 4.0f);
    g.setColour(juce::Colour(0xff7ce8ff));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    const float mixVal = audioProcessor.getAPVTS().getRawParameterValue("mix")->load();
    g.drawText(juce::String( static_cast<int>(std::round(mixVal)) ) + "%",
               mixPill.toNearestInt(), juce::Justification::centred);

    // Bypass Label
    g.setColour(juce::Colour(0xffb8c2cc));
    g.setFont(juce::Font(13.5f, juce::Font::bold));
    g.drawText("BYPASS", 580, 882, 140, 20, juce::Justification::centred);
}

void SequinAudioProcessorEditor::paint(juce::Graphics& g)
{
    drawBrushedMetalBackground(g, getLocalBounds());
    drawTopHeader(g);

    // Main Display Recessed Frame
    const juce::Rectangle<float> displayFrame(26.0f, 88.0f, 708.0f, 372.0f);
    g.setColour(juce::Colour(0xff07090c));
    g.fillRoundedRectangle(displayFrame.expanded(3.0f), 6.0f);
    g.setColour(juce::Colour(0xff46505c));
    g.drawRoundedRectangle(displayFrame.expanded(3.0f), 6.0f, 2.0f);

    // Animated Dual View Split / Morph Calculation
    const float dividerH = 24.0f;
    const float specRatio = juce::jmap(viewMorphProgress, 0.0f, 1.0f, 0.64f, 0.25f);
    const float specH = (displayFrame.getHeight() - dividerH) * specRatio;
    const float rxH   = displayFrame.getHeight() - dividerH - specH;

    const juce::Rectangle<float> specBounds(displayFrame.getX(), displayFrame.getY(),
                                            displayFrame.getWidth(), specH);
    const juce::Rectangle<float> divBounds(displayFrame.getX(), specBounds.getBottom(),
                                           displayFrame.getWidth(), dividerH);
    const juce::Rectangle<float> rxBounds(displayFrame.getX(), divBounds.getBottom(),
                                          displayFrame.getWidth(), rxH);

    drawSpectrumView(g, specBounds);
    drawViewModeDivider(g, divBounds);
    drawRXHarmonicStackView(g, rxBounds);

    drawControlDeckLabels(g);
}

void SequinAudioProcessorEditor::resized()
{
    focusSlider.setBounds(92, 536, 44, 332);
    sensitivitySlider.setBounds(235, 530, 110, 110);
    qResonanceSlider.setBounds(235, 682, 110, 106);
    focusAuditionButton.setBounds(255, 832, 70, 26);

    harmonicTameSlider.setBounds(406, 532, 164, 142);
    mixSlider.setBounds(595, 530, 110, 110);
    bypassButton.setBounds(610, 795, 80, 80);
}
