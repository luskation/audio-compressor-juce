#include "PluginProcessor.h"
#include "PluginEditor.h"

CompressorAudioAudioProcessor::CompressorAudioAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMS", createParameters())
{
}

CompressorAudioAudioProcessor::~CompressorAudioAudioProcessor() {}

bool CompressorAudioAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* CompressorAudioAudioProcessor::createEditor()
{
    return new CompressorAudioAudioProcessorEditor(*this);
}

const juce::String CompressorAudioAudioProcessor::getName() const { return JucePlugin_Name; }
bool CompressorAudioAudioProcessor::acceptsMidi()  const { return false; }
bool CompressorAudioAudioProcessor::producesMidi() const { return false; }
bool CompressorAudioAudioProcessor::isMidiEffect() const { return false; }
double CompressorAudioAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int  CompressorAudioAudioProcessor::getNumPrograms() { return 1; }
int  CompressorAudioAudioProcessor::getCurrentProgram() { return 0; }
void CompressorAudioAudioProcessor::setCurrentProgram(int) {}
const juce::String CompressorAudioAudioProcessor::getProgramName(int) { return {}; }
void CompressorAudioAudioProcessor::changeProgramName(int, const juce::String&) {}

// ─────────────────────────────────────────────────────────────────────────────
void CompressorAudioAudioProcessor::prepareToPlay(double sr, int)
{
    sampleRate = (float)sr;
    envelope = 0.0f;
    gainReductionDb.store(0.0f);
}

void CompressorAudioAudioProcessor::releaseResources() {}

// ─────────────────────────────────────────────────────────────────────────────
// processBlock — detecção linked-stereo, ganho aplicado a todos os canais
// ─────────────────────────────────────────────────────────────────────────────
void CompressorAudioAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Lê parâmetros uma vez por bloco (thread-safe via atomic)
    const float threshold = apvts.getRawParameterValue("threshold")->load();
    const float ratio = apvts.getRawParameterValue("ratio")->load();
    const float attackMs = apvts.getRawParameterValue("attack")->load();
    const float releaseMs = apvts.getRawParameterValue("release")->load();
    const float makeupDb = apvts.getRawParameterValue("makeup")->load();
    const float kneeDb = apvts.getRawParameterValue("knee")->load();

    const float makeupGain = juce::Decibels::decibelsToGain(makeupDb);
    const float attackCoeff = std::exp(-1.0f / (0.001f * attackMs * sampleRate));
    const float releaseCoeff = std::exp(-1.0f / (0.001f * releaseMs * sampleRate));

    float peakGrDb = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // ── Detector: pico linked (máximo dos canais) ───────────────────────
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            peak = std::max(peak, std::abs(buffer.getReadPointer(ch)[i]));

        const float inputDb = juce::Decibels::gainToDecibels(peak + 1e-9f);

        // ── Cálculo da redução de ganho (soft knee) ─────────────────────────
        float grDb = 0.0f;
        const float halfKnee = kneeDb * 0.5f;

        if (kneeDb > 0.0f && inputDb >= (threshold - halfKnee) && inputDb <= (threshold + halfKnee))
        {
            // Zona de knee suave
            const float x = inputDb - threshold + halfKnee;
            grDb = (1.0f / ratio - 1.0f) * (x * x) / (2.0f * kneeDb);
        }
        else if (inputDb > threshold + halfKnee)
        {
            grDb = (threshold - inputDb) * (1.0f - 1.0f / ratio);
        }

        // grDb é negativo (redução) ou zero
        const float targetGain = juce::Decibels::decibelsToGain(grDb);

        // ── Envelope de ataque/release ────────────────────────────────────────
        const float coeff = (targetGain < envelope) ? attackCoeff : releaseCoeff;
        envelope = coeff * envelope + (1.0f - coeff) * targetGain;

        // Ganho total a aplicar no sample
        const float totalGain = envelope * makeupGain;

        // ── Aplica ganho em todos os canais ──────────────────────────────────
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer(ch)[i] *= totalGain;

        // Rastreia pico de GR para o medidor da UI (em dB, valor positivo = redução)
        const float grNow = juce::Decibels::gainToDecibels(envelope);
        if (grNow < peakGrDb) peakGrDb = grNow;
    }

    // Expõe GR como valor positivo para o medidor
    gainReductionDb.store(-peakGrDb);
}

// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
CompressorAudioAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "threshold", "Threshold", -60.0f, 0.0f, -18.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "ratio", 1 }, "Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), 4.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "attack", "Attack", 1.0f, 200.0f, 10.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release", 10.0f, 2000.0f, 150.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "makeup", "Makeup", 0.0f, 24.0f, 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "knee", "Knee", 0.0f, 12.0f, 6.0f));

    return { p.begin(), p.end() };
}

// ─────────────────────────────────────────────────────────────────────────────
// Persistência de estado (preset save/load)
// ─────────────────────────────────────────────────────────────────────────────
void CompressorAudioAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CompressorAudioAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorAudioAudioProcessor();
}