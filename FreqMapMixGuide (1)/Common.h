#pragma once
#include <cmath>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace fm
{
// Literały ze znakami spoza ASCII (polskie litery) zawsze przez U8().
inline juce::String U8 (const char* s) { return juce::String::fromUTF8 (s); }

// f = 440 * 2^((n-69)/12)   <->   n = 69 + 12 * log2(f/440)
inline double hzFromMidi (double n) { return 440.0 * std::pow (2.0, (n - 69.0) / 12.0); }
inline double midiFromHz (double hz) { return 69.0 + 12.0 * std::log2 (hz / 440.0); }

inline bool isBlackKey (int n)
{
    const int pc = ((n % 12) + 12) % 12;
    return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
}

inline juce::String noteName (int n)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (names[((n % 12) + 12) % 12]) + juce::String (n / 12 - 1); // C4 = MIDI 60
}

struct NoteInfo { int midi; double cents; juce::String name; };

inline NoteInfo describeHz (double hz)
{
    const double m = midiFromHz (hz);
    const int n = (int) std::lround (m);
    return { n, (m - n) * 100.0, noteName (n) };
}

inline juce::String fmtShort (double hz)
{
    if (hz >= 1000.0)
    {
        const double k = hz / 1000.0;
        return juce::String (k, std::abs (k - std::round (k)) < 0.05 ? 0 : 1) + "k";
    }
    return juce::String ((int) std::lround (hz));
}

inline juce::String fmtRange (double from, double to)
{
    return fmtShort (from) + U8 ("–") + fmtShort (to) + " Hz";
}
} // namespace fm
