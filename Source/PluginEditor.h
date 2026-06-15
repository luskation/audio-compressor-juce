#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// ─────────────────────────────────────────────────────────────────────────────
// Knob customizado com label, valor e estilo escuro
// ─────────────────────────────────────────────────────────────────────────────
class LabeledKnob : public juce::Component
{
public:
    juce::Slider slider;

    LabeledKnob(const juce::String& labelText, const juce::String& unit)
        : label(labelText), unitStr(unit)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // Label em cima
        g.setColour(juce::Colour(160, 160, 180));
        g.setFont(juce::Font("Arial", 10.5f, juce::Font::bold));
        g.drawText(label, b.removeFromTop(16.0f), juce::Justification::centred);

        b.removeFromBottom(16.0f); // reserva espaço para o valor

        // Valor em baixo
        auto valueArea = getLocalBounds().toFloat().removeFromBottom(16.0f);
        g.setColour(juce::Colour(220, 200, 255));
        g.setFont(juce::Font("Arial", 10.0f, juce::Font::plain));

        juce::String valStr = formatValue(slider.getValue());
        g.drawText(valStr + " " + unitStr, valueArea, juce::Justification::centred);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        b.removeFromTop(16);
        b.removeFromBottom(16);
        slider.setBounds(b);
    }

private:
    juce::String label, unitStr;

    juce::String formatValue(double v) const
    {
        if (std::abs(v) < 10.0)  return juce::String(v, 1);
        return juce::String(juce::roundToInt(v));
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Medidor de Gain Reduction vertical
// ─────────────────────────────────────────────────────────────────────────────
class GRMeter : public juce::Component, public juce::Timer
{
public:
    std::atomic<float>* sourceDb = nullptr;

    GRMeter() { startTimerHz(30); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // Fundo
        g.setColour(juce::Colour(25, 20, 38));
        g.fillRoundedRectangle(b, 4.0f);

        g.setColour(juce::Colour(50, 40, 70));
        g.drawRoundedRectangle(b.reduced(0.5f), 4.0f, 1.0f);

        // Barra (redução de ganho → de cima para baixo)
        constexpr float maxDb = 30.0f;
        const float gr = juce::jlimit(0.0f, maxDb, displayGr);
        const float proportion = gr / maxDb;

        if (proportion > 0.001f)
        {
            auto meterArea = b.reduced(3.0f);
            auto fill = meterArea;
            fill.setHeight(meterArea.getHeight() * proportion);

            // Gradiente verde → amarelo → vermelho
            juce::ColourGradient grad(
                juce::Colour(80, 220, 140), fill.getTopLeft(),
                juce::Colour(220, 60, 80), fill.getBottomLeft(), false);
            grad.addColour(0.5, juce::Colour(230, 190, 50));

            g.setGradientFill(grad);
            g.fillRoundedRectangle(fill, 2.0f);
        }

        // Label GR
        g.setColour(juce::Colour(160, 160, 180));
        g.setFont(juce::Font("Arial", 9.0f, juce::Font::bold));
        g.drawText("GR", b.removeFromBottom(14.0f), juce::Justification::centred);

        // Valor numérico
        g.setColour(juce::Colour(220, 200, 255));
        g.setFont(juce::Font("Arial", 9.5f, juce::Font::plain));
        juce::String grStr = "-" + juce::String(displayGr, 1) + "dB";
        g.drawText(grStr, juce::Rectangle<float>(b.getX(), b.getY(), b.getWidth(), 14.0f),
            juce::Justification::centred);
    }

    void timerCallback() override
    {
        if (sourceDb == nullptr) return;
        const float raw = sourceDb->load();
        // Peak hold simples
        if (raw > displayGr)       displayGr = raw;
        else                       displayGr = displayGr * 0.93f + raw * 0.07f;
        repaint();
    }

private:
    float displayGr = 0.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
// Editor principal
// ─────────────────────────────────────────────────────────────────────────────
class CompressorAudioAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::Timer
{
public:
    CompressorAudioAudioProcessorEditor(CompressorAudioAudioProcessor&);
    ~CompressorAudioAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    CompressorAudioAudioProcessor& audioProcessor;

    LabeledKnob thresholdKnob{ "THRESHOLD", "dB" };
    LabeledKnob ratioKnob{ "RATIO",     ":1" };
    LabeledKnob attackKnob{ "ATTACK",    "ms" };
    LabeledKnob releaseKnob{ "RELEASE",   "ms" };
    LabeledKnob makeupKnob{ "MAKEUP",    "dB" };
    LabeledKnob kneeKnob{ "KNEE",      "dB" };

    GRMeter grMeter;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> a1, a2, a3, a4, a5, a6;

    // Pintamos um grid de linhas sutis no fundo
    void drawBackground(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorAudioAudioProcessorEditor)
};