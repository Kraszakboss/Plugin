#pragma once
#include "Common.h"

struct GuideZone
{
    double fromHz = 0.0, toHz = 0.0;
    juce::String title, actionAdd, actionCut;
    juce::Colour colour;
};

struct InstrumentGuide
{
    juce::String instrument; // pole "instrument"
    juce::String label;      // opcjonalne pole "label" (nazwa zakładki), domyślnie = instrument
    std::vector<GuideZone> zones;
};

namespace GuideData
{
    // Przyjmuje pojedynczy obiekt {...} albo tablicę obiektów [{...}, {...}].
    // Przy błędzie zwraca pustą listę i opis w `error`.
    std::vector<InstrumentGuide> parse (const juce::String& jsonText, juce::String& error);

    // Wbudowane poradniki (Gitara, Bas, Perkusja/Werbel, Wokal) - w tej kolejności.
    const std::vector<juce::String>& builtinJson();
}
