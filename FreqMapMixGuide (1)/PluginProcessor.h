#pragma once
#include "Common.h"

// Wtyczka przepuszcza audio bez zmian; kopia sygnału (mono) trafia do FIFO,
// z którego korzysta analizator w edytorze.
class FreqMapProcessor : public juce::AudioProcessor
{
public:
    FreqMapProcessor();
    ~FreqMapProcessor() override = default;

    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Wątek GUI: pobiera dostępne próbki (zwraca liczbę próbek).
    int pullSamples (float* dest, int maxNum);

    // Ścieżki wczytanych przez użytkownika plików JSON (zapisywane w projekcie DAW).
    juce::StringArray customGuideFiles;

private:
    static constexpr int fifoSize = 1 << 16;
    juce::AbstractFifo fifo { fifoSize };
    std::vector<float> fifoData;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreqMapProcessor)
};
