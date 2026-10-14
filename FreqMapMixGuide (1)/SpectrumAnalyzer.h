#pragma once
#include "Common.h"
#include "GuideData.h"
#include "PluginProcessor.h"

class SpectrumAnalyzer : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumAnalyzer (FreqMapProcessor&);
    ~SpectrumAnalyzer() override { stopTimer(); }

    void showPitchPage();                       // Tab 1: strefy odczuć słuchowych
    void showGuide (const InstrumentGuide&);    // Tab 2+: strefy EQ z JSON
    void setZoneEnabled (int index, bool enabled);

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent& e) override { mouseMove (e); }
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    struct DisplayZone
    {
        float from, to;
        juce::String title, detail, add, cut;
        juce::Colour colour;
        bool enabled;
    };
    struct Peak { float hz, db; };

    static constexpr int fftOrder = 14;               // 16384 punktów: ~2.7 Hz/bin przy 44.1 kHz
    static constexpr int fftSize  = 1 << fftOrder;

    void timerCallback() override;
    void analyse();
    void findPeaks();
    float valueAt (float hz0, float hz1) const;

    juce::Rectangle<float> plotArea() const;
    void drawKeyBands (juce::Graphics&, const juce::Rectangle<float>&);
    void drawZones    (juce::Graphics&, const juce::Rectangle<float>&);
    void drawGrid     (juce::Graphics&, const juce::Rectangle<float>&);
    void drawSpectrum (juce::Graphics&, const juce::Rectangle<float>&);
    void drawPeaks    (juce::Graphics&, const juce::Rectangle<float>&);
    void drawAxes     (juce::Graphics&, const juce::Rectangle<float>&);
    void drawInfo     (juce::Graphics&, const juce::Rectangle<float>&);

    FreqMapProcessor& processor;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::vector<float> ring, fftData, smoothed, scratch;
    int writePos = 0;
    float sampleRate = 44100.0f;

    std::vector<Peak> peaks;
    std::vector<DisplayZone> zones;
    int hoveredZone = -1;
    bool mouseInside = false;
    juce::Point<float> mousePos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzer)
};
