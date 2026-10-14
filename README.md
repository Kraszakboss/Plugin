# FreqMap & MixGuide VST3

Wtyczka-analizator (JUCE 8, C++17) do Studio One Pro. Audio przechodzi bez zmian.

## Budowanie
Wymagane: CMake >= 3.22, kompilator C++17 (VS 2022 / Xcode / gcc).

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

VST3 ląduje w `build/FreqMapMixGuide_artefacts/Release/VST3/`
(`COPY_PLUGIN_AFTER_BUILD` kopiuje go też do systemowego folderu VST3; w Windows wymaga to
uprawnień administratora, w razie błędu skopiuj ręcznie do `C:\Program Files\Common Files\VST3`).
W Studio One: Options > Locations > VST Plug-ins > Reset Blacklist / Rescan.
Wariant Standalone służy do szybkiego testu bez DAW.

## Własne poradniki JSON
Przycisk "+" wczytuje plik .json (pojedynczy obiekt lub tablica obiektów). Wzór: `Resources/example_custom_keys.json`.
Pola strefy: from_hz, to_hz, title, action_add, action_cut, opcjonalnie color (hex RRGGBB).
Pole instrumentu: instrument, opcjonalnie label (nazwa zakładki).
Ścieżki wczytanych plików zapisują się w projekcie DAW.

## Struktura
    Source/PluginProcessor.*   pass-through + FIFO próbek (mono) dla GUI
    Source/SpectrumAnalyzer.*  FFT 16384, wykres log, siatka nut, piki, strefy, hover
    Source/PluginEditor.*      zakładki, panel przełączników stref, wczytywanie JSON
    Source/GuideData.*         parser JSON + wbudowane poradniki (gitara, bas, perkusja, wokal)
