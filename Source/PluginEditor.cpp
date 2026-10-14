#include "PluginEditor.h"
using namespace juce;

FreqMapEditor::FreqMapEditor (FreqMapProcessor& p)
    : AudioProcessorEditor (&p), proc (p), analyzer (p)
{
    setResizable (true, true);
    setResizeLimits (760, 460, 2400, 1400);
    setSize (1040, 660);

    String err;
    for (auto& json : GuideData::builtinJson())
        for (auto& g : GuideData::parse (json, err))
            guides.push_back (g);

    for (auto& path : proc.customGuideFiles)
    {
        File f (path);
        if (f.existsAsFile())
            for (auto& g : GuideData::parse (f.loadFileAsString(), err))
                guides.push_back (g);
    }

    addAndMakeVisible (analyzer);

    addButton.setColour (TextButton::buttonColourId, Colour (0xff24242c));
    addButton.setColour (TextButton::textColourOffId, Colour (0xffd4ff3a));
    addButton.onClick = [this] { openGuideFile(); };
    addAndMakeVisible (addButton);

    rebuildTabs();
    selectTab (0);
}

void FreqMapEditor::rebuildTabs()
{
    tabButtons.clear();

    auto addTab = [this] (const String& name, int index)
    {
        auto* b = tabButtons.add (new TextButton (name));
        b->setClickingTogglesState (true);
        b->setRadioGroupId (1001);
        b->setColour (TextButton::buttonColourId,   Colour (0xff24242c));
        b->setColour (TextButton::buttonOnColourId, Colour (0xff00b8d4));
        b->setColour (TextButton::textColourOffId,  Colours::lightgrey);
        b->setColour (TextButton::textColourOnId,   Colours::black);
        b->onClick = [this, index] { selectTab (index); };
        addAndMakeVisible (b);
    };

    addTab (fm::U8 ("Nuty & Częstotliwości"), 0);
    for (size_t i = 0; i < guides.size(); ++i)
        addTab (guides[i].label, (int) i + 1);

    resized();
}

void FreqMapEditor::selectTab (int index)
{
    currentTab = jlimit (0, (int) guides.size(), index);
    if (isPositiveAndBelow (currentTab, tabButtons.size()))
        tabButtons[currentTab]->setToggleState (true, dontSendNotification);

    zoneToggles.clear();
    if (currentTab == 0)
    {
        analyzer.showPitchPage();
    }
    else
    {
        const auto& g = guides[(size_t) currentTab - 1];
        analyzer.showGuide (g);

        for (size_t j = 0; j < g.zones.size(); ++j)
        {
            const auto& z = g.zones[j];
            auto* t = zoneToggles.add (new ToggleButton (z.title + "  (" + fm::fmtRange (z.fromHz, z.toHz) + ")"));
            t->setToggleState (true, dontSendNotification);
            t->setColour (ToggleButton::textColourId, Colours::lightgrey);
            t->setColour (ToggleButton::tickColourId, z.colour);
            t->setColour (ToggleButton::tickDisabledColourId, Colours::grey);
            t->onClick = [this, j, t] { analyzer.setZoneEnabled ((int) j, t->getToggleState()); };
            addAndMakeVisible (t);
        }
    }
    resized();
    repaint();
}

void FreqMapEditor::openGuideFile()
{
    chooser = std::make_unique<FileChooser> (fm::U8 ("Wczytaj plik JSON z poradami"), File(), "*.json");
    Component::SafePointer<FreqMapEditor> safe (this);

    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
        [safe] (const FileChooser& fc)
        {
            if (safe == nullptr) return;
            const auto file = fc.getResult();
            if (! file.existsAsFile()) return;

            String err;
            auto parsed = GuideData::parse (file.loadFileAsString(), err);
            if (parsed.empty())
            {
                AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "JSON", err);
                return;
            }

            safe->proc.customGuideFiles.addIfNotAlreadyThere (file.getFullPathName());
            const int first = (int) safe->guides.size() + 1;
            for (auto& g : parsed) safe->guides.push_back (g);
            safe->rebuildTabs();
            safe->selectTab (first);
        });
}

void FreqMapEditor::paint (Graphics& g)
{
    g.fillAll (Colour (0xff131316));
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (43, 0.0f, (float) getWidth());

    if (currentTab > 0 && ! panelBounds.isEmpty())
    {
        auto pb = panelBounds;
        g.setColour (Colour (0xff00b8d4));
        g.setFont (12.0f);
        g.drawText (fm::U8 ("STREFY EQ — ") + guides[(size_t) currentTab - 1].label,
                    pb.removeFromTop (20), Justification::centredLeft, true);
    }
}

void FreqMapEditor::resized()
{
    auto r = getLocalBounds();
    auto tabRow = r.removeFromTop (44).reduced (8, 7);

    for (auto* b : tabButtons)
    {
        const int w = b->getButtonText().length() * 7 + 26;
        b->setBounds (tabRow.removeFromLeft (w));
        tabRow.removeFromLeft (4);
    }
    addButton.setBounds (tabRow.removeFromLeft (34));

    panelBounds = {};
    if (currentTab > 0)
    {
        panelBounds = r.removeFromRight (250).reduced (10);
        auto pb = panelBounds;
        pb.removeFromTop (28);
        for (auto* t : zoneToggles)
            t->setBounds (pb.removeFromTop (26));
    }
    analyzer.setBounds (r.reduced (8, 0).withTrimmedBottom (8));
}
