#pragma once

#include <JuceHeader.h>

class CompressorAudioAudioProcessor : public juce::AudioProcessor
{
public:
    CompressorAudioAudioProcessor();
    ~CompressorAudioAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

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

    juce::AudioProcessorValueTreeState apvts;

    // Expõe GR para o editor desenhar o medidor
    std::atomic<float> gainReductionDb{ 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    // Envelope de detecção (único, linked stereo)
    float envelope = 0.0f;
    float sampleRate = 44100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorAudioAudioProcessor)
};