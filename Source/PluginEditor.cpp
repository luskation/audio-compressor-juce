#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
// Look & Feel customizado para os knobs
// ─────────────────────────────────────────────────────────────────────────────
struct CompressorLookAndFeel : public juce::LookAndFeel_V4
{
    CompressorLookAndFeel()
    {
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(130, 80, 220));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(50, 40, 70));
        setColour(juce::Slider::thumbColourId, juce::Colours::white);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle,
        float rotaryEndAngle, juce::Slider& slider) override
    {
        const float radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
        const float centreX = (float)x + (float)width * 0.5f;
        const float centreY = (float)y + (float)height * 0.5f;
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Anel de fundo (trilha)
        const float trackW = 4.0f;
        juce::Path bgArc;
        bgArc.addCentredArc(centreX, centreY, radius, radius, 0,
            rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(50, 40, 70));
        g.strokePath(bgArc, juce::PathStrokeType(trackW, juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

        // Anel de valor (preenchido)
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius, radius, 0,
            rotaryStartAngle, angle, true);
        juce::ColourGradient grad(juce::Colour(100, 60, 200), centreX - radius, centreY,
            juce::Colour(180, 100, 255), centreX + radius, centreY, false);
        g.setGradientFill(grad);
        g.strokePath(valueArc, juce::PathStrokeType(trackW, juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

        // Círculo central (corpo do knob)
        const float knobR = radius - trackW - 3.0f;
        juce::ColourGradient knobGrad(juce::Colour(65, 55, 90),
            centreX - knobR * 0.3f, centreY - knobR * 0.3f,
            juce::Colour(28, 22, 42),
            centreX + knobR * 0.3f, centreY + knobR * 0.3f, false);
        g.setGradientFill(knobGrad);
        g.fillEllipse(centreX - knobR, centreY - knobR, knobR * 2.0f, knobR * 2.0f);

        // Borda do knob
        g.setColour(juce::Colour(90, 70, 130));
        g.drawEllipse(centreX - knobR, centreY - knobR, knobR * 2.0f, knobR * 2.0f, 1.0f);

        // Ponteiro
        const float pointerLen = knobR * 0.55f;
        const float pointerX = centreX + std::sin(angle) * pointerLen;
        const float pointerY = centreY - std::cos(angle) * pointerLen;
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawLine(centreX, centreY, pointerX, pointerY, 2.0f);

        // Ponto brilhante no centro
        g.setColour(juce::Colour(200, 160, 255).withAlpha(0.4f));
        g.fillEllipse(centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);

        juce::ignoreUnused(slider);
    }
};

static CompressorLookAndFeel gLookAndFeel;

// ─────────────────────────────────────────────────────────────────────────────
CompressorAudioAudioProcessorEditor::CompressorAudioAudioProcessorEditor(CompressorAudioAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Aplica look and feel customizado a cada slider
    auto setupSlider = [&](LabeledKnob& k)
        {
            k.slider.setLookAndFeel(&gLookAndFeel);
            // Atualiza o repaint do componente quando o valor muda
            k.slider.onValueChange = [&k] { k.repaint(); };
            addAndMakeVisible(k);
        };

    setupSlider(thresholdKnob);
    setupSlider(ratioKnob);
    setupSlider(attackKnob);
    setupSlider(releaseKnob);
    setupSlider(makeupKnob);
    setupSlider(kneeKnob);

    // Attachments APVTS
    a1 = std::make_unique<Attachment>(audioProcessor.apvts, "threshold", thresholdKnob.slider);
    a2 = std::make_unique<Attachment>(audioProcessor.apvts, "ratio", ratioKnob.slider);
    a3 = std::make_unique<Attachment>(audioProcessor.apvts, "attack", attackKnob.slider);
    a4 = std::make_unique<Attachment>(audioProcessor.apvts, "release", releaseKnob.slider);
    a5 = std::make_unique<Attachment>(audioProcessor.apvts, "makeup", makeupKnob.slider);
    a6 = std::make_unique<Attachment>(audioProcessor.apvts, "knee", kneeKnob.slider);

    // Medidor GR
    grMeter.sourceDb = &audioProcessor.gainReductionDb;
    addAndMakeVisible(grMeter);

    setSize(620, 280);
    startTimerHz(30);
}

CompressorAudioAudioProcessorEditor::~CompressorAudioAudioProcessorEditor()
{
    // Remove LookAndFeel antes de destruir
    auto clearLAF = [](LabeledKnob& k) { k.slider.setLookAndFeel(nullptr); };
    clearLAF(thresholdKnob); clearLAF(ratioKnob);
    clearLAF(attackKnob);    clearLAF(releaseKnob);
    clearLAF(makeupKnob);    clearLAF(kneeKnob);
    stopTimer();
}

// ─────────────────────────────────────────────────────────────────────────────
void CompressorAudioAudioProcessorEditor::timerCallback()
{
    // Força repaint dos knobs a cada frame (para atualizar labels de valor)
    thresholdKnob.repaint();
    ratioKnob.repaint();
    attackKnob.repaint();
    releaseKnob.repaint();
    makeupKnob.repaint();
    kneeKnob.repaint();
}

// ─────────────────────────────────────────────────────────────────────────────
void CompressorAudioAudioProcessorEditor::drawBackground(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Gradiente de fundo: deep purple-black
    juce::ColourGradient bg(juce::Colour(18, 12, 32), 0.0f, 0.0f,
        juce::Colour(28, 20, 48), b.getWidth(), b.getHeight(), false);
    g.setGradientFill(bg);
    g.fillAll();

    // Linhas de grid sutis
    g.setColour(juce::Colour(255, 255, 255).withAlpha(0.025f));
    const int step = 24;
    for (int x = 0; x < (int)b.getWidth(); x += step)
        g.drawVerticalLine(x, 0.0f, b.getHeight());
    for (int y = 0; y < (int)b.getHeight(); y += step)
        g.drawHorizontalLine(y, 0.0f, b.getWidth());

    // Separador entre header e knobs
    g.setColour(juce::Colour(130, 80, 220).withAlpha(0.4f));
    g.drawHorizontalLine(48, 12.0f, b.getWidth() - 12.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
void CompressorAudioAudioProcessorEditor::paint(juce::Graphics& g)
{
    drawBackground(g);

    // ── Header ──────────────────────────────────────────────────────────────
    // Brilho lateral esquerdo (acento roxo)
    g.setColour(juce::Colour(130, 80, 220).withAlpha(0.8f));
    g.fillRect(0, 0, 4, 48);

    // Nome do plugin
    g.setFont(juce::Font("Arial", 22.0f, juce::Font::bold));
    juce::ColourGradient titleGrad(juce::Colour(200, 160, 255), 20.0f, 14.0f,
        juce::Colour(130, 80, 220), 200.0f, 14.0f, false);
    g.setGradientFill(titleGrad);
    g.drawText("COMPRESSOR", juce::Rectangle<float>(16.0f, 10.0f, 220.0f, 28.0f),
        juce::Justification::centredLeft);

    // Sub-label
    g.setColour(juce::Colour(120, 100, 160));
    g.setFont(juce::Font("Arial", 10.0f, juce::Font::plain));
    g.drawText("Dynamics Processor  |  Linked Stereo  |  Soft Knee",
        juce::Rectangle<float>(16.0f, 30.0f, 400.0f, 14.0f),
        juce::Justification::centredLeft);

    // Versão
    g.setColour(juce::Colour(80, 70, 100));
    g.setFont(juce::Font("Arial", 9.0f, juce::Font::plain));
    g.drawText("v2.0", getLocalBounds().toFloat().reduced(12.0f, 6.0f),
        juce::Justification::topRight);
}

// ─────────────────────────────────────────────────────────────────────────────
void CompressorAudioAudioProcessorEditor::resized()
{
    const int headerH = 52;
    const int padding = 14;
    const int meterW = 52;

    auto area = getLocalBounds().reduced(padding);
    area.removeFromTop(headerH - padding);

    // Medidor GR à direita
    auto meterArea = area.removeFromRight(meterW);
    meterArea.removeFromTop(4);
    grMeter.setBounds(meterArea);

    area.removeFromRight(8); // gap entre knobs e meter

    // 6 knobs distribuídos igualmente
    const int numKnobs = 6;
    const int knobW = area.getWidth() / numKnobs;

    thresholdKnob.setBounds(area.removeFromLeft(knobW));
    ratioKnob.setBounds(area.removeFromLeft(knobW));
    attackKnob.setBounds(area.removeFromLeft(knobW));
    releaseKnob.setBounds(area.removeFromLeft(knobW));
    kneeKnob.setBounds(area.removeFromLeft(knobW));
    makeupKnob.setBounds(area.removeFromLeft(knobW));
}