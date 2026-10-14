#include "GuideData.h"
using namespace juce;

namespace
{
const char* kGuitar = R"json({
  "instrument": "Guitar",
  "label": "Gitara Elektryczna / Akustyczna",
  "zones": [
    { "from_hz": 80,   "to_hz": 120,  "title": "High-Pass Filter",
      "action_add": "",
      "action_cut": "Odetnij, aby zwolnić miejsce dla basu i stopy." },
    { "from_hz": 200,  "to_hz": 400,  "title": "Ciało / Mud",
      "action_add": "Dodaje ciepła i pełni, gdy gitara jest cienka.",
      "action_cut": "Odejmij, jeśli gitara brzmi zamulenie / pudełkowo." },
    { "from_hz": 1500, "to_hz": 3500, "title": "Atak piórka / Czytelność",
      "action_add": "+1-3 dB, jeśli gitara ginie w miksie.",
      "action_cut": "Cofnij, jeśli gitara wyskakuje przed wokal." },
    { "from_hz": 5000, "to_hz": 8000, "title": "Jad / Harshness",
      "action_add": "Dodaje blasku i szkliwa (ostrożnie).",
      "action_cut": "Wytnij wąskim filtrem notch, jeśli góra kłuje w uszy." }
  ]
})json";

const char* kBass = R"json({
  "instrument": "Bass",
  "label": "Bas",
  "zones": [
    { "from_hz": 30,   "to_hz": 60,   "title": "Sub",
      "action_add": "Fundament i waga; uważaj na konflikt ze stopą.",
      "action_cut": "Highpass lub cięcie, jeśli miks dudni na dużych systemach." },
    { "from_hz": 60,   "to_hz": 120,  "title": "Podstawa / Punch",
      "action_add": "Masa i pewność dołu.",
      "action_cut": "Odejmij, gdy bas i stopa się zlewają." },
    { "from_hz": 200,  "to_hz": 400,  "title": "Mud",
      "action_add": "",
      "action_cut": "Wytnij, jeśli bas brzmi zamulenie." },
    { "from_hz": 700,  "to_hz": 1200, "title": "Growl / Czytelność",
      "action_add": "Dodaje charakteru, bas słychać na małych głośnikach.",
      "action_cut": "Odejmij, jeśli brzmi nosowo." },
    { "from_hz": 2000, "to_hz": 3500, "title": "Atak / Palce-Kostka",
      "action_add": "Czytelność ataku (slap, pick).",
      "action_cut": "Odejmij, gdy bas klika lub szura." }
  ]
})json";

const char* kDrums = R"json({
  "instrument": "Drums",
  "label": "Perkusja / Werbel",
  "zones": [
    { "from_hz": 50,    "to_hz": 100,   "title": "Stopa: dół / Thump",
      "action_add": "Uderzenie w klatkę piersiową.",
      "action_cut": "Odejmij, jeśli stopa walczy z basem." },
    { "from_hz": 150,   "to_hz": 250,   "title": "Werbel: ciało / Fatness",
      "action_add": "Pełnia i waga werbla.",
      "action_cut": "Odejmij, jeśli werbel brzmi tępo." },
    { "from_hz": 300,   "to_hz": 500,   "title": "Pudło / Boxiness",
      "action_add": "",
      "action_cut": "Wytnij dla czystości stopy i tomów." },
    { "from_hz": 2000,  "to_hz": 5000,  "title": "Werbel: atak / Crack",
      "action_add": "Trzask, przebija się przez gitary.",
      "action_cut": "Odejmij, jeśli werbel kłuje." },
    { "from_hz": 8000,  "to_hz": 14000, "title": "Blachy / Air",
      "action_add": "Powietrze i blask hi-hatów oraz overheadów.",
      "action_cut": "Odejmij, jeśli blachy syczą." }
  ]
})json";

const char* kVocals = R"json({
  "instrument": "Vocals",
  "label": "Wokal",
  "zones": [
    { "from_hz": 80,    "to_hz": 120,   "title": "Low Cut",
      "action_add": "",
      "action_cut": "Usuwa dudnienie, pop i szum." },
    { "from_hz": 200,   "to_hz": 350,   "title": "Ciepło / Mud",
      "action_add": "Ciało i ciepło głosu.",
      "action_cut": "Wytnij, jeśli wokal brzmi zamulenie, jak w beczce." },
    { "from_hz": 800,   "to_hz": 1500,  "title": "Nosowość",
      "action_add": "",
      "action_cut": "Odejmij przy nosowym, telefonicznym brzmieniu." },
    { "from_hz": 2000,  "to_hz": 5000,  "title": "Czytelność / Presence",
      "action_add": "Zrozumiałość tekstu i obecność.",
      "action_cut": "Odejmij, jeśli wokal męczy." },
    { "from_hz": 5000,  "to_hz": 9000,  "title": "Sybilanty",
      "action_add": "",
      "action_cut": "De-esser lub wycięcie, gdy s i sz kłują." },
    { "from_hz": 10000, "to_hz": 16000, "title": "Air",
      "action_add": "Powietrze i drogi blask.",
      "action_cut": "Odejmij, jeśli szumi lub brzmi sztucznie jasno." }
  ]
})json";

Colour parseColour (String s, Colour fallback)
{
    s = s.trim().trimCharactersAtStart ("#");
    if (s.length() == 6) s = "ff" + s;
    if (s.length() != 8 || ! s.containsOnly ("0123456789abcdefABCDEF")) return fallback;
    return Colour::fromString (s);
}

bool parseGuide (const var& v, InstrumentGuide& out, String& err)
{
    auto* obj = v.getDynamicObject();
    if (obj == nullptr)
    {
        err = fm::U8 ("Oczekiwano obiektu JSON z polami \"instrument\" i \"zones\".");
        return false;
    }

    out.instrument = obj->getProperty ("instrument").toString();
    if (out.instrument.isEmpty())
    {
        err = fm::U8 ("Brak pola \"instrument\".");
        return false;
    }
    out.label = obj->hasProperty ("label") ? obj->getProperty ("label").toString() : out.instrument;

    const var zones = obj->getProperty ("zones");
    if (! zones.isArray())
    {
        err = fm::U8 ("Brak tablicy \"zones\".");
        return false;
    }

    static const Colour palette[] = { Colour (0xff00e5ff), Colour (0xffff3d9a), Colour (0xffb2ff59),
                                      Colour (0xffffab40), Colour (0xff7c4dff), Colour (0xff40c4ff) };
    int i = 0;
    for (auto& zv : *zones.getArray())
    {
        auto* zo = zv.getDynamicObject();
        if (zo == nullptr) continue;

        GuideZone z;
        z.fromHz    = (double) zo->getProperty ("from_hz");
        z.toHz      = (double) zo->getProperty ("to_hz");
        z.title     = zo->getProperty ("title").toString();
        z.actionAdd = zo->getProperty ("action_add").toString();
        z.actionCut = zo->getProperty ("action_cut").toString();

        if (! (z.fromHz > 0.0 && z.toHz > z.fromHz))
        {
            err = fm::U8 ("Strefa nr ") + String (i + 1) + fm::U8 (": nieprawidłowy zakres from_hz / to_hz.");
            return false;
        }
        z.colour = parseColour (zo->getProperty ("color").toString(), palette[i % 6]);
        out.zones.push_back (z);
        ++i;
    }
    return true;
}
} // namespace

std::vector<InstrumentGuide> GuideData::parse (const String& text, String& error)
{
    std::vector<InstrumentGuide> result;
    const var v = JSON::parse (text);
    if (v.isVoid())
    {
        error = fm::U8 ("Niepoprawna składnia JSON.");
        return result;
    }

    auto handle = [&] (const var& item)
    {
        InstrumentGuide g;
        if (! parseGuide (item, g, error)) return false;
        result.push_back (std::move (g));
        return true;
    };

    if (auto* arr = v.getArray())
    {
        for (auto& item : *arr)
            if (! handle (item)) { result.clear(); return result; }
    }
    else if (! handle (v))
    {
        result.clear();
    }
    return result;
}

const std::vector<String>& GuideData::builtinJson()
{
    static const std::vector<String> list { fm::U8 (kGuitar), fm::U8 (kBass), fm::U8 (kDrums), fm::U8 (kVocals) };
    return list;
}
