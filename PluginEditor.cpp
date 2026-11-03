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
                                               static_cast<float>(width), static_cast<float>(height)).reduced(5.0f);
    const float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const float outerRadius = radius * 0.88f;
    g.setColour(juce::Colour(0xff0b0d10));
    g.fillEllipse(centre.x - outerRadius - 3.0f, centre.y - outerRadius - 3.0f,
                  (outerRadius + 3.0f) * 2.0f, (outerRadius + 3.0f) * 2.0f);

    juce::Path bgArc;
    bgArc.addCentredArc(centre.x, centre.y, outerRadius + 4.0f, outerRadius + 4.0f,
                        0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff22272e));
    g.strokePath(bgArc, juce::PathStrokeType(isMainTameKnob ? 3.5f : 2.6f,
                                             juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    juce::Path valArc;
    valArc.addCentredArc(centre.x, centre.y, outerRadius + 4.0f, outerRadius + 4.0f,
                         0.0f, rotaryStartAngle, toAngle, true);
    g.setColour(isMainTameKnob ? juce::Colour(0xffff5566) : juce::Colour(0xff52b8ff));
    g.strokePath(valArc, juce::PathStrokeType(isMainTameKnob ? 3.8f : 2.8f,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    const float bodyRadius = radius * 0.74f;
    juce::ColourGradient bezelGrad(juce::Colour(0xff505760), centre.x - bodyRadius, centre.y - bodyRadius,
                                   juce::Colour(0xff181a1e), centre.x + bodyRadius, centre.y + bodyRadius, false);
    g.setGradientFill(bezelGrad);
    g.fillEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

    const float capRadius = bodyRadius * 0.84f;
    if (isMainTameKnob)
    {
        juce::ColourGradient iridGrad(juce::Colour(0xff264556), centre.x - capRadius, centre.y - capRadius,
                                      juce::Colour(0xff522439), centre.x + capRadius, centre.y + capRadius, false);
        iridGrad.addColour(0.35, juce::Colour(0xff272c33));
        iridGrad.addColour(0.68, juce::Colour(0xff364e45));
        g.setGradientFill(iridGrad);
    }
    else
    {
        juce::ColourGradient capGrad(juce::Colour(0xff38322e), centre.x, centre.y - capRadius,
                                     juce::Colour(0xff16181b), centre.x, centre.y + capRadius, false);
        g.setGradientFill(capGrad);
    }
    g.fillEllipse(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawEllipse(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.2f);

    juce::Path pointer;
    const float pointerLen = capRadius * 0.82f;
    pointer.addRoundedRectangle(-1.8f, -pointerLen, 3.6f, capRadius * 0.38f, 1.5f);
    g.setColour(isMainTameKnob ? juce::Colour(0xffff5566) : juce::Colour(0xffffb84d));
    g.fillPath(pointer, juce::AffineTransform::rotation(toAngle).translated(centre.x, centre.y));
}

void SequinLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                         const juce::Slider::SliderStyle, juce::Slider& /*slider*/)
{
    const float trackWidth = 20.0f;
    const float trackX = static_cast<float>(x) + (static_cast<float>(width) - trackWidth) * 0.5f;
    const auto trackRect = juce::Rectangle<float>(trackX, static_cast<float>(y),
                                                  trackWidth, static_cast<float>(height));

    g.setColour(juce::Colour(0xff0b0e12));
    g.fillRoundedRectangle(trackRect, 4.0f);
    g.setColour(juce::Colour(0xff343a42));
    g.drawRoundedRectangle(trackRect, 4.0f, 1.2f);

    const int numTicks = 38;
    for (int i = 0; i <= numTicks; ++i)
    {
        const float normY = static_cast<float>(i) / static_cast<float>(numTicks);
        const float ty = trackRect.getY() + normY * trackRect.getHeight();
        const bool nearThumb = std::abs(ty - sliderPos) < 32.0f;
        g.setColour(nearThumb ? juce::Colour(0xffffaa33) : juce::Colour(0xff38c878).withAlpha(0.65f));
        const float tickW = (i % 5 == 0) ? 6.0f : 3.5f;
        g.fillRect(trackRect.getX() - tickW - 3.0f, ty - 0.8f, tickW, 1.6f);
    }

    const float thumbHeight = 72.0f;
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
        const float diameter = std::min(bounds.getWidth(), bounds.getHeight()) - 8.0f;
        const auto circle = bounds.withSizeKeepingCentre(diameter, diameter);

        if (!isOn)
        {
            juce::ColourGradient glow(juce::Colour(0xffffdd88), circle.getCentreX(), circle.getCentreY(),
                                      juce::Colour(0x00ff9900), circle.getX() - 8.0f, circle.getY() - 8.0f, true);
            g.setGradientFill(glow);
            g.fillEllipse(circle.expanded(6.0f));
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
        const auto pill = bounds.withSizeKeepingCentre(42.0f, 18.0f);
        g.setColour(isOn ? juce::Colour(0xffcc7a1a) : juce::Colour(0xff14171c));
        g.fillRoundedRectangle(pill, 9.0f);
        g.setColour(juce::Colour(0xff454c56));
        g.drawRoundedRectangle(pill, 9.0f, 1.2f);

        const float knobX = isOn ? (pill.getRight() - 15.0f) : (pill.getX() + 3.0f);
        g.setColour(isOn ? juce::Colour(0xfffff0d0) : juce::Colour(0xff9aa2ad));
        g.fillEllipse(knobX, pill.getY() + 3.0f, 12.0f, 12.0f);
    }
}

void SequinLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& /*backgroundColour*/,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    const bool isAKG = (button.getButtonText() == "AKG C2000B");

    juce::Colour base = isAKG ? juce::Colour(0xff2a2115) : juce::Colour(0xff181c22);
    if (shouldDrawButtonAsDown)
        base = base.brighter(0.3f);
    else if (shouldDrawButtonAsHighlighted)
        base = base.brighter(0.15f);

    g.setColour(base);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(isAKG ? juce::Colour(0xffffaa33) : juce::Colour(0xff3d4754));
    g.drawRoundedRectangle(bounds, 4.0f, isAKG ? 1.4f : 1.0f);
}

//==============================================================================
SequinAudioProcessorEditor::SequinAudioProcessorEditor(SequinAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&sequinLnF);

    // Initialize TooltipWindow safely in constructor body
    tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 400);

    setResizable(true, true);
    setResizeLimits(570, 720, 1520, 1920);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio(760.0 / 960.0);
    setSize(760, 960);

    ColumnHistory emptyCol {};
    emptyCol.bandDb.fill(-90.0f);
    emptyCol.bandHot.fill(false);
    rxHistoryColumns.resize(numStackColumns, emptyCol);

    // Controls
    focusSlider.setSliderStyle(juce::Slider::LinearVertical);
    focusSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    focusSlider.setTooltip("Focus Frequency: Sets target center frequency (80 Hz to 20 kHz).");
    addAndMakeVisible(focusSlider);
    focusAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "focusFreq", focusSlider);

    sensitivitySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sensitivitySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sensitivitySlider.setTooltip("Sensitivity: Sets detection threshold. Higher sensitivity catches quieter harsh harmonics.");
    addAndMakeVisible(sensitivitySlider);
    sensitivityAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "sensitivity", sensitivitySlider);

    qResonanceSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    qResonanceSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    qResonanceSlider.setTooltip("Q / Resonance: Width of target zone. Minimum engages full 80 Hz to 20 kHz range.");
    addAndMakeVisible(qResonanceSlider);
    qResonanceAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "qResonance", qResonanceSlider);

    harmonicTameSlider.setName("HarmonicTame");
    harmonicTameSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    harmonicTameSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    harmonicTameSlider.setTooltip("Harmonic Tame: Subtractive gain reduction (0 dB to -24 dB) applied to excessive peaks.");
    addAndMakeVisible(harmonicTameSlider);
    harmonicTameAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "harmonicTame", harmonicTameSlider);

    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    mixSlider.setTooltip("Mix: Parallel dry/wet blend between unprocessed vocal and tamed signal.");
    addAndMakeVisible(mixSlider);
    mixAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "mix", mixSlider);

    focusAuditionButton.setName("FocusAudition");
    focusAuditionButton.setTooltip("Focus Audition: Solos only the audio within the shaded target window.");
    addAndMakeVisible(focusAuditionButton);
    focusAuditionAttachment = std::make_unique<ButtonAttachment>(audioProcessor.getAPVTS(), "focusAudition", focusAuditionButton);

    bypassButton.setName("Bypass");
    bypassButton.setTooltip("Bypass: Instantly toggles entire harmonic taming DSP on/off.");
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<ButtonAttachment>(audioProcessor.getAPVTS(), "bypass", bypassButton);

    // Preset Buttons
    auto setupPresetBtn = [this](juce::TextButton& btn, const juce::String& tip, int presetIdx)
    {
        btn.setTooltip(tip);
        btn.onClick = [this, presetIdx]() { audioProcessor.loadPreset(presetIdx); };
        addAndMakeVisible(btn);
    };

    setupPresetBtn(btnPresetAKG, "AKG C2000B De-Harsh: Tames harsh 5.5kHz - 9.5kHz condenser presence bite.", 0);
    setupPresetBtn(btnPresetSibilance, "Vocal Sibilance Tamer: Focused de-essing across 5.5kHz - 7.5kHz.", 1);
    setupPresetBtn(btnPresetBoxy, "Boxy Low-Mids Cleaner: Tight notch around 300Hz - 600Hz chest mud.", 2);
    setupPresetBtn(btnPresetAir, "Air & Sheen: Smooths top-end glare above 10kHz.", 3);
    setupPresetBtn(btnPresetFull, "Full Range Leveler: Wide net (80Hz - 20kHz) for transparent spectral leveling.", 4);

    startTimerHz(30);
}

SequinAudioProcessorEditor::~SequinAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void SequinAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.isDawPlaying.load())
    {
        audioProcessor.getVisualSnapshot(spectrumDb, baselineDb, thresholdDb, activePeaks);

        ColumnHistory col {};
        const double sr = audioProcessor.getCurrentSampleRate();
        const float binHz = static_cast<float>(sr / static_cast<double>(SequinAudioProcessor::fftSize));

        for (int b = 0; b < numHarmonicBands; ++b)
        {
            const float normMin = static_cast<float>(b) / static_cast<float>(numHarmonicBands);
            const float normMax = static_cast<float>(b + 1) / static_cast<float>(numHarmonicBands);
            const float f0 = 80.0f * std::pow(250.0f, normMin);
            const float f1 = 80.0f * std::pow(250.0f, normMax);

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
    }

    const bool targetRxMode = audioProcessor.getAPVTS().getRawParameterValue("viewMode")->load() > 0.5f;
    const float targetMorph = targetRxMode ? 1.0f : 0.0f;
    viewMorphProgress += (targetMorph - viewMorphProgress) * 0.22f;

    repaint();
}

float SequinAudioProcessorEditor::freqToNormX(float freqHz) const
{
    const float clamped = juce::jlimit(80.0f, 20000.0f, freqHz);
    return std::log10(clamped / 80.0f) / std::log10(20000.0f / 80.0f);
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

    g.setColour(juce::Colours::white.withAlpha(0.015f));
    for (int y = 0; y < bounds.getHeight(); y += 3)
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(bounds.getWidth()));

    g.setColour(juce::Colour(0xff484f5a));
    g.drawRect(bounds, 2);
}

void SequinAudioProcessorEditor::drawTopHeader(juce::Graphics& g, float sx, float sy)
{
    juce::Rectangle<float> headerBounds(16.0f * sx, 10.0f * sy, 728.0f * sx, 64.0f * sy);
    juce::ColourGradient headerGrad(juce::Colour(0xff323740), 380.0f * sx, 10.0f * sy,
                                    juce::Colour(0xff181b20), 380.0f * sx, 74.0f * sy, false);
    g.setGradientFill(headerGrad);
    g.fillRoundedRectangle(headerBounds, 6.0f);

    juce::ColourGradient goldGrad(juce::Colour(0xffffecb3), 380.0f * sx, 14.0f * sy,
                                  juce::Colour(0xffb5842b), 380.0f * sx, 48.0f * sy, false);
    g.setGradientFill(goldGrad);
    g.setFont(juce::Font(32.0f * sy, juce::Font::bold));
    g.drawText("SEQUIN", 0, static_cast<int>(12 * sy), getWidth(), static_cast<int>(34 * sy), juce::Justification::centred);

    g.setColour(juce::Colour(0xffa9b2bd));
    g.setFont(juce::Font(13.5f * sy, juce::Font::plain));
    g.drawText("Harmonic Vocal Processor", 0, static_cast<int>(44 * sy), getWidth(), static_cast<int>(20 * sy), juce::Justification::centred);

    // Top-Right Telemetry Readout Pill
    juce::Rectangle<float> telemetryBox(488.0f * sx, 18.0f * sy, 242.0f * sx, 42.0f * sy);
    g.setColour(juce::Colour(0xff0d1117));
    g.fillRoundedRectangle(telemetryBox, 5.0f);
    g.setColour(juce::Colour(0xff3a4654));
    g.drawRoundedRectangle(telemetryBox, 5.0f, 1.2f);

    const int harmCount = audioProcessor.harmonicsDetected.load();
    const int hotCount  = audioProcessor.tooHotHarmonicsCount.load();

    g.setColour(juce::Colour(0xff9ad8ff));
    g.setFont(juce::Font(13.0f * sy, juce::Font::bold));
    g.drawText("Harmonics detected: " + juce::String(harmCount) + ", Too loud: " + juce::String(hotCount),
               telemetryBox.toNearestInt(), juce::Justification::centred);
}

void SequinAudioProcessorEditor::drawSpectrumView(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.saveState();
    g.reduceClipRegion(bounds.toNearestInt());

    juce::ColourGradient bgGrad(juce::Colour(0xff0a0e13), bounds.getX(), bounds.getY(),
                                juce::Colour(0xff0f171e), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(bgGrad);
    g.fillRect(bounds);

    const float gridFreqs[] = { 80.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    const char* gridLabels[] = { "80Hz", "200Hz", "500Hz", "1kHz", "2kHz", "5kHz", "10kHz", "20kHz" };

    g.setFont(juce::Font(11.0f, juce::Font::plain));
    for (int i = 0; i < 8; ++i)
    {
        const float nx = freqToNormX(gridFreqs[i]);
        const float gx = bounds.getX() + nx * bounds.getWidth();
        g.setColour(juce::Colour(0xff1c2630));
        g.drawVerticalLine(static_cast<int>(gx), bounds.getY(), bounds.getBottom() - 18.0f);
        g.setColour(juce::Colour(0xff7f8c99));
        const int labelOffset = (i == 7) ? -38 : ((i == 0) ? 2 : -16);
        g.drawText(gridLabels[i], static_cast<int>(gx) + labelOffset, static_cast<int>(bounds.getBottom()) - 17,
                   42, 14, juce::Justification::left);
    }

    for (int row = 1; row < 5; ++row)
    {
        const float gy = bounds.getY() + (static_cast<float>(row) / 5.0f) * (bounds.getHeight() - 20.0f);
        g.setColour(juce::Colour(0xff18212a));
        g.drawHorizontalLine(static_cast<int>(gy), bounds.getX(), bounds.getRight());
    }

    const float plotBottom = bounds.getBottom() - 19.0f;
    const float plotHeight = plotBottom - bounds.getY();
    const double sr = audioProcessor.getCurrentSampleRate();
    const float binHz = static_cast<float>(sr / static_cast<double>(SequinAudioProcessor::fftSize));

    juce::Path specPath;
    juce::Path specFill;
    bool started = false;

    for (int k = 1; k < SequinAudioProcessor::numBins; ++k)
    {
        const float freq = k * binHz;
        if (freq < 80.0f || freq > 20000.0f)
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

    juce::Path baselinePath;
    bool baseStarted = false;
    for (int k = 1; k < SequinAudioProcessor::numBins; k += 4)
    {
        const float freq = k * binHz;
        if (freq < 80.0f || freq > 20000.0f)
            continue;
        const float x = bounds.getX() + freqToNormX(freq) * bounds.getWidth();
        const float y = bounds.getY() + dbToNormY(baselineDb[static_cast<size_t>(k)]) * plotHeight;
        if (!baseStarted) { baselinePath.startNewSubPath(x, y); baseStarted = true; }
        else { baselinePath.lineTo(x, y); }
    }
    g.setColour(juce::Colour(0xff2aff55));
    g.strokePath(baselinePath, juce::PathStrokeType(1.8f));

    const float sensVal = audioProcessor.getAPVTS().getRawParameterValue("sensitivity")->load();
    const float threshVisualDb = -18.0f - (sensVal * 0.32f);
    const float threshY = bounds.getY() + dbToNormY(threshVisualDb) * plotHeight;

    juce::Path dashedRed;
    dashedRed.startNewSubPath(bounds.getX(), threshY);
    dashedRed.lineTo(bounds.getRight(), threshY);
    const float dashLengths[] = { 6.0f, 4.0f };
    juce::Path strokedDash;
    juce::PathStrokeType(1.3f).createDashedStroke(strokedDash, dashedRed, dashLengths, 2);
    g.setColour(juce::Colour(0xffff4242).withAlpha(0.85f));
    g.fillPath(strokedDash);

    const float fLow  = audioProcessor.lowCutFreqHz.load();
    const float fHigh = audioProcessor.highCutFreqHz.load();
    const float xLow  = bounds.getX() + freqToNormX(fLow) * bounds.getWidth();
    const float xHigh = bounds.getX() + freqToNormX(fHigh) * bounds.getWidth();

    juce::Rectangle<float> bandRect(xLow, bounds.getY(), std::max(4.0f, xHigh - xLow), plotHeight);
    juce::ColourGradient bandGrad(juce::Colour(0x50ff9d2e), bandRect.getCentreX(), bandRect.getY(),
                                  juce::Colour(0x20ff7700), bandRect.getCentreX(), bandRect.getBottom(), false);
    g.setGradientFill(bandGrad);
    g.fillRect(bandRect);

    g.setColour(juce::Colour(0xffffaa3b).withAlpha(0.85f));
    g.drawVerticalLine(static_cast<int>(bandRect.getX()), bandRect.getY(), bandRect.getBottom());
    g.drawVerticalLine(static_cast<int>(bandRect.getRight()), bandRect.getY(), bandRect.getBottom());

    g.setFont(juce::Font(11.0f, juce::Font::bold));
    int pillsDrawn = 0;
    float lastPillX = -100.0f;

    for (const auto& peak : activePeaks)
    {
        if (!peak.isTooHot || peak.attenuationDb >= -0.15f)
            continue;

        const float px = bounds.getX() + freqToNormX(peak.frequencyHz) * bounds.getWidth();
        const float py = bounds.getY() + dbToNormY(peak.magnitudeDb) * plotHeight;

        g.setColour(juce::Colour(0xffff3838));
        g.drawLine(px, py, px, plotBottom, 2.0f);
        g.fillEllipse(px - 3.0f, py - 3.0f, 6.0f, 6.0f);

        if (std::abs(px - lastPillX) > 48.0f && pillsDrawn < 4)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << peak.attenuationDb << " dB";
            const juce::String pillText = oss.str();

            juce::Rectangle<float> pillBox(px - 25.0f, std::max(bounds.getY() + 4.0f, py - 24.0f), 50.0f, 17.0f);
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

    juce::Rectangle<float> warnPill(bounds.getX() + 12.0f, bounds.getY() + 4.0f, 80.0f, bounds.getHeight() - 8.0f);
    const bool warnOn = audioProcessor.warningLightActive.load();

    g.setColour(juce::Colour(0xff1e242c));
    g.fillRoundedRectangle(warnPill, 6.0f);
    g.setColour(juce::Colour(0xff38404a));
    g.drawRoundedRectangle(warnPill, 6.0f, 1.0f);

    if (warnOn)
    {
        juce::ColourGradient warnGlow(juce::Colour(0xffffcc44), warnPill.getRight() - 12.0f, warnPill.getCentreY(),
                                      juce::Colour(0xffcc2200), warnPill.getX() + 10.0f, warnPill.getCentreY(), false);
        g.setGradientFill(warnGlow);
        g.fillRoundedRectangle(warnPill.reduced(2.0f), 4.0f);
        g.setColour(juce::Colour(0xfffffae0));
        g.fillEllipse(warnPill.getRight() - 16.0f, warnPill.getCentreY() - 5.0f, 10.0f, 10.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff3b2c28));
        g.fillEllipse(warnPill.getRight() - 16.0f, warnPill.getCentreY() - 5.0f, 10.0f, 10.0f);
    }

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
}

void SequinAudioProcessorEditor::drawRXHarmonicStackView(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.saveState();
    g.reduceClipRegion(bounds.toNearestInt());

    g.setColour(juce::Colour(0xff070a0f));
    g.fillRect(bounds);

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

    const float fLow  = audioProcessor.lowCutFreqHz.load();
    const float fHigh = audioProcessor.highCutFreqHz.load();
    const float xLow  = bounds.getX() + freqToNormX(fLow) * bounds.getWidth();
    const float xHigh = bounds.getX() + freqToNormX(fHigh) * bounds.getWidth();
    g.setColour(juce::Colour(0x28ffaa33));
    g.fillRect(xLow, bounds.getY(), std::max(3.0f, xHigh - xLow), bounds.getHeight());

    g.restoreState();
}

void SequinAudioProcessorEditor::drawControlDeckLabels(juce::Graphics& g, float sx, float sy)
{
    g.setColour(juce::Colour(0xff14171c));
    g.drawVerticalLine(static_cast<int>(390 * sx), 512.0f * sy, 930.0f * sy);
    g.drawHorizontalLine(static_cast<int>(504 * sy), 24.0f * sx, 736.0f * sx);

    // Labels
    g.setColour(juce::Colour(0xffd0d7e0));
    g.setFont(juce::Font(15.0f * sy, juce::Font::bold));
    g.drawText("FOCUS", static_cast<int>(42 * sx), static_cast<int>(516 * sy), static_cast<int>(150 * sx), static_cast<int>(20 * sy), juce::Justification::centred);
    g.drawText("SENSITIVITY", static_cast<int>(215 * sx), static_cast<int>(516 * sy), static_cast<int>(150 * sx), static_cast<int>(20 * sy), juce::Justification::centred);
    g.drawText("HARMONIC TAME", static_cast<int>(405 * sx), static_cast<int>(516 * sy), static_cast<int>(165 * sx), static_cast<int>(20 * sy), juce::Justification::centred);
    g.drawText("MIX", static_cast<int>(580 * sx), static_cast<int>(516 * sy), static_cast<int>(140 * sx), static_cast<int>(20 * sy), juce::Justification::centred);

    // Frequency Scale
    g.setColour(juce::Colour(0xffc8d0d8));
    g.setFont(juce::Font(12.0f * sy, juce::Font::bold));
    g.drawText("20kHz", static_cast<int>(40 * sx), static_cast<int>(545 * sy), static_cast<int>(48 * sx), static_cast<int>(16 * sy), juce::Justification::right);
    g.drawText("5.0k",  static_cast<int>(40 * sx), static_cast<int>(635 * sy), static_cast<int>(48 * sx), static_cast<int>(16 * sy), juce::Justification::right);
    g.drawText("1.5k",  static_cast<int>(40 * sx), static_cast<int>(715 * sy), static_cast<int>(48 * sx), static_cast<int>(16 * sy), juce::Justification::right);
    g.drawText("300",   static_cast<int>(40 * sx), static_cast<int>(795 * sy), static_cast<int>(48 * sx), static_cast<int>(16 * sy), juce::Justification::right);
    g.drawText("80Hz",  static_cast<int>(40 * sx), static_cast<int>(865 * sy), static_cast<int>(48 * sx), static_cast<int>(16 * sy), juce::Justification::right);

    const float fLow  = audioProcessor.lowCutFreqHz.load();
    const float fHigh = audioProcessor.highCutFreqHz.load();
    const float sliderTop = 550.0f * sy;
    const float sliderBot = 875.0f * sy;

    // Fixed: Public parameter range normalization avoiding protected slider method
    const float normVal = audioProcessor.getAPVTS().getParameterRange("focusFreq").convertTo0to1(static_cast<float>(focusSlider.getValue()));
    const float thumbY = sliderTop + (1.0f - normVal) * (sliderBot - sliderTop);
    const float bracketTop = juce::jlimit(sliderTop, sliderBot - 50.0f * sy, thumbY - 38.0f * sy);
    const float bracketBot = juce::jlimit(bracketTop + 24.0f * sy, sliderBot, thumbY + 38.0f * sy);

    g.setColour(juce::Colour(0xffff9d2e));
    g.drawLine(134.0f * sx, bracketTop, 141.0f * sx, bracketTop, 2.0f);
    g.drawLine(141.0f * sx, bracketTop, 141.0f * sx, bracketBot, 2.0f);
    g.drawLine(134.0f * sx, bracketBot, 141.0f * sx, bracketBot, 2.0f);

    g.setFont(juce::Font(12.5f * sy, juce::Font::bold));
    g.drawText(formatFrequency(fHigh), static_cast<int>(146 * sx), static_cast<int>(bracketTop) - 8, static_cast<int>(68 * sx), 18, juce::Justification::left);
    g.drawText(formatFrequency(fLow), static_cast<int>(146 * sx), static_cast<int>(bracketBot) - 8, static_cast<int>(68 * sx), 18, juce::Justification::left);

    // Current Range Box
    juce::Rectangle<float> rangeBox(42.0f * sx, 895.0f * sy, 255.0f * sx, 34.0f * sy);
    g.setColour(juce::Colour(0xff14110e));
    g.fillRoundedRectangle(rangeBox, 4.0f);
    g.setColour(juce::Colour(0xff8c5820));
    g.drawRoundedRectangle(rangeBox, 4.0f, 1.2f);
    g.setColour(juce::Colour(0xffffb84d));
    g.setFont(juce::Font(14.0f * sy, juce::Font::bold));
    g.drawText("CURRENT RANGE: " + formatFrequency(fLow) + " - " + formatFrequency(fHigh),
               rangeBox.toNearestInt(), juce::Justification::centred);

    // Sensitivity Readout Pill
    juce::Rectangle<float> sensPill(252.0f * sx, 656.0f * sy, 76.0f * sx, 22.0f * sy);
    g.setColour(juce::Colour(0xff0d131a));
    g.fillRoundedRectangle(sensPill, 4.0f);
    g.setColour(juce::Colour(0xff7ce8ff));
    g.setFont(juce::Font(13.0f * sy, juce::Font::bold));
    const float sensVal = audioProcessor.getAPVTS().getRawParameterValue("sensitivity")->load();
    g.drawText(juce::String(sensVal, 1) + "%", sensPill.toNearestInt(), juce::Justification::centred);

    g.setColour(juce::Colour(0xffc0c8d2));
    g.setFont(juce::Font(13.5f * sy, juce::Font::bold));
    g.drawText("Q/RESONANCE", static_cast<int>(215 * sx), static_cast<int>(802 * sy), static_cast<int>(150 * sx), static_cast<int>(20 * sy), juce::Justification::centred);
    g.drawText("FOCUS AUDITION", static_cast<int>(215 * sx), static_cast<int>(872 * sy), static_cast<int>(150 * sx), static_cast<int>(20 * sy), juce::Justification::centred);

    // Harmonic Tame Graduations
    g.setColour(juce::Colour(0xffb8c2cc));
    g.setFont(juce::Font(12.0f * sy, juce::Font::plain));
    g.drawText("0 dB", static_cast<int>(406 * sx), static_cast<int>(678 * sy), static_cast<int>(44 * sx), 16, juce::Justification::centred);
    g.drawText("-24 dB", static_cast<int>(524 * sx), static_cast<int>(678 * sy), static_cast<int>(50 * sx), 16, juce::Justification::centred);

    juce::Rectangle<float> tamePill(452.0f * sx, 685.0f * sy, 74.0f * sx, 24.0f * sy);
    g.setColour(juce::Colour(0xff0d131a));
    g.fillRoundedRectangle(tamePill, 4.0f);
    g.setColour(juce::Colour(0xff2c4558));
    g.drawRoundedRectangle(tamePill, 4.0f, 1.0f);
    g.setColour(juce::Colour(0xff7ce8ff));
    g.setFont(juce::Font(13.0f * sy, juce::Font::bold));
    const float tameVal = audioProcessor.getAPVTS().getRawParameterValue("harmonicTame")->load();
    const juce::String tameStr = (tameVal > 0.05f ? "-" : "") + juce::String(tameVal, 1) + " dB";
    g.drawText(tameStr, tamePill.toNearestInt(), juce::Justification::centred);

    // Mix Readout Pill
    g.setColour(juce::Colour(0xffb8c2cc));
    g.setFont(juce::Font(12.0f * sy, juce::Font::plain));
    g.drawText("0%", static_cast<int>(595 * sx), static_cast<int>(642 * sy), static_cast<int>(32 * sx), 16, juce::Justification::centred);
    g.drawText("100%", static_cast<int>(676 * sx), static_cast<int>(642 * sy), static_cast<int>(40 * sx), 16, juce::Justification::centred);

    juce::Rectangle<float> mixPill(616.0f * sx, 656.0f * sy, 68.0f * sx, 22.0f * sy);
    g.setColour(juce::Colour(0xff0d131a));
    g.fillRoundedRectangle(mixPill, 4.0f);
    g.setColour(juce::Colour(0xff7ce8ff));
    g.setFont(juce::Font(13.0f * sy, juce::Font::bold));
    const float mixVal = audioProcessor.getAPVTS().getRawParameterValue("mix")->load();
    g.drawText(juce::String(static_cast<int>(std::round(mixVal))) + "%",
               mixPill.toNearestInt(), juce::Justification::centred);

    // Bypass Label
    g.setColour(juce::Colour(0xffb8c2cc));
    g.setFont(juce::Font(13.5f * sy, juce::Font::bold));
    g.drawText("BYPASS", static_cast<int>(580 * sx), static_cast<int>(892 * sy), static_cast<int>(140 * sx), static_cast<int>(20 * sy), juce::Justification::centred);
}

void SequinAudioProcessorEditor::paint(juce::Graphics& g)
{
    const float sx = static_cast<float>(getWidth()) / 760.0f;
    const float sy = static_cast<float>(getHeight()) / 960.0f;

    drawBrushedMetalBackground(g, getLocalBounds());
    drawTopHeader(g, sx, sy);

    const juce::Rectangle<float> displayFrame(26.0f * sx, 80.0f * sy, 708.0f * sx, 350.0f * sy);
    g.setColour(juce::Colour(0xff07090c));
    g.fillRoundedRectangle(displayFrame.expanded(3.0f), 6.0f);
    g.setColour(juce::Colour(0xff46505c));
    g.drawRoundedRectangle(displayFrame.expanded(3.0f), 6.0f, 2.0f);

    const float dividerH = 24.0f * sy;
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

    drawControlDeckLabels(g, sx, sy);
}

void SequinAudioProcessorEditor::resized()
{
    const float sx = static_cast<float>(getWidth()) / 760.0f;
    const float sy = static_cast<float>(getHeight()) / 960.0f;

    // Preset Selector Bar (Quick 1-Click Access)
    const int btnY = static_cast<int>(444 * sy);
    const int btnH = static_cast<int>(26 * sy);
    btnPresetAKG.setBounds(static_cast<int>(26 * sx), btnY, static_cast<int>(134 * sx), btnH);
    btnPresetSibilance.setBounds(static_cast<int>(168 * sx), btnY, static_cast<int>(130 * sx), btnH);
    btnPresetBoxy.setBounds(static_cast<int>(306 * sx), btnY, static_cast<int>(130 * sx), btnH);
    btnPresetAir.setBounds(static_cast<int>(444 * sx), btnY, static_cast<int>(130 * sx), btnH);
    btnPresetFull.setBounds(static_cast<int>(582 * sx), btnY, static_cast<int>(152 * sx), btnH);

    // Controls
    focusSlider.setBounds(static_cast<int>(92 * sx),  static_cast<int>(546 * sy), static_cast<int>(44 * sx),  static_cast<int>(332 * sy));
    sensitivitySlider.setBounds(static_cast<int>(235 * sx), static_cast<int>(540 * sy), static_cast<int>(110 * sx), static_cast<int>(110 * sy));
    qResonanceSlider.setBounds(static_cast<int>(235 * sx),  static_cast<int>(692 * sy), static_cast<int>(110 * sx), static_cast<int>(106 * sy));
    focusAuditionButton.setBounds(static_cast<int>(255 * sx), static_cast<int>(842 * sy), static_cast<int>(70 * sx),  static_cast<int>(26 * sy));

    harmonicTameSlider.setBounds(static_cast<int>(406 * sx), static_cast<int>(542 * sy), static_cast<int>(164 * sx), static_cast<int>(142 * sy));
    mixSlider.setBounds(static_cast<int>(595 * sx), static_cast<int>(540 * sy), static_cast<int>(110 * sx), static_cast<int>(110 * sy));
    bypassButton.setBounds(static_cast<int>(610 * sx), static_cast<int>(805 * sy), static_cast<int>(80 * sx),  static_cast<int>(80 * sy));
}
