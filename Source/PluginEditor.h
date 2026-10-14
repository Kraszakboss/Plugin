#pragma once
#include "Common.h"
#include "GuideData.h"
#include "PluginProcessor.h"
#include "SpectrumAnalyzer.h"

class FreqMapEditor : public juce::AudioProcessorEditor
{
public:
    explicit FreqMapEditor (FreqMapProcessor&);
    ~FreqMapEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void rebuildTabs();
    void selectTab (int index);   // 0 = Nuty & Częstotliwości, 1.. = poradniki
    void openGuideFile();

    FreqMapProcessor& proc;
    SpectrumAnalyzer analyzer;

    std::vector<InstrumentGuide> guides;           // tab i+1 <-> guides[i]
    juce::OwnedArray<juce::TextButton> tabButtons;
    juce::TextButton addButton { "+" };
    juce::OwnedArray<juce::ToggleButton> zoneToggles;
    juce::Rectangle<int> panelBounds;
    int currentTab = 0;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreqMapEditor)
};
