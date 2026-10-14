#include "SpectrumAnalyzer.h"
using namespace juce;
using namespace fm;

namespace
{
constexpr float kMinDb = -90.0f, kMaxDb = 12.0f;
constexpr float kMinHz = 20.0f, kMaxHz = 20000.0f;
constexpr float kAxisH = 112.0f; // Hz (16) + pasek nut (18) + odstęp + panel info

const Colour kBg (0xff18181c), kCyan (0xff00e5ff), kMagenta (0xffff3d9a), kLime (0xffd4ff3a);

float hzToX (const Rectangle<float>& p, float hz)
{
    return p.getX() + p.getWidth() * std::log (hz / kMinHz) / std::log (kMaxHz / kMinHz);
}
float xToHz (const Rectangle<float>& p, float x)
{
    return kMinHz * std::pow (kMaxHz / kMinHz, (x - p.getX()) / p.getWidth());
}
float dbToY (const Rectangle<float>& p, float db)
{
    return p.getBottom() - p.getHeight() * (jlimit (kMinDb, kMaxDb, db) - kMinDb) / (kMaxDb - kMinDb);
}
String fmtHz (double hz)
{
    return hz < 10000.0 ? String (hz, 1) + " Hz" : String (hz / 1000.0, 2) + " kHz";
}
} // namespace

SpectrumAnalyzer::SpectrumAnalyzer (FreqMapProcessor& p)
    : processor (p),
      fft (fftOrder),
      window ((size_t) fftSize, dsp::WindowingFunction<float>::hann),
      ring ((size_t) fftSize, 0.0f),
      fftData ((size_t) fftSize * 2, 0.0f),
      smoothed ((size_t) fftSize / 2, kMinDb),
      scratch (4096)
{
    showPitchPage();
    startTimerHz (60);
}

// ---------------------------------------------------------------- strony
void SpectrumAnalyzer::showPitchPage()
{
    struct Z { float a, b; const char* title; const char* detail; uint32 col; };
    static const Z list[] = {
        {   20.f,   60.f, "Sub / Rumble",           "Sub-Bass: odczucie w brzuchu",                           0xff7c4dff },
        {   60.f,  150.f, "Low-End / Chest",        "Low-End: odczucie w klatce piersiowej (Chest Resonance)", 0xff536dfe },
        {  200.f,  315.f, "250 Hz  \"OO\"",         "Samogłoska „OO”: ciepło lub pudełkowatość",              0xff29b6f6 },
        {  400.f,  630.f, "500 Hz  \"OOH/AHH\"",    "Samogłoska „OOH/AHH”: ciało instrumentu",                0xff26c6da },
        {  800.f, 1250.f, "1 kHz  \"AH\"",          "Samogłoska „AH”: nosowość i środek pasma",               0xff66bb6a },
        { 2000.f, 4000.f, "2-4 kHz  \"EE\"",        "Samogłoska „EE”: atak, kąśliwość (Bite / Presence)",     0xffffca28 },
        { 6000.f,10000.f, "6-10 kHz  Air",          "Powietrze, sybilanty, jasność (Air / Sibilance)",        0xffff7043 },
    };
    zones.clear();
    for (auto& z : list)
        zones.push_back ({ z.a, z.b, U8 (z.title), U8 (z.detail), {}, {}, Colour (z.col), true });
    hoveredZone = -1;
}

void SpectrumAnalyzer::showGuide (const InstrumentGuide& g)
{
    zones.clear();
    for (auto& z : g.zones)
        zones.push_back ({ (float) z.fromHz, (float) z.toHz, z.title, {}, z.actionAdd, z.actionCut, z.colour, true });
    hoveredZone = -1;
}

void SpectrumAnalyzer::setZoneEnabled (int index, bool enabled)
{
    if (isPositiveAndBelow (index, (int) zones.size()))
        zones[(size_t) index].enabled = enabled;
}

// ---------------------------------------------------------------- DSP
void SpectrumAnalyzer::timerCallback()
{
    analyse();
    findPeaks();
    repaint();
}

void SpectrumAnalyzer::analyse()
{
    const double sr = processor.getSampleRate();
    sampleRate = sr > 0.0 ? (float) sr : 44100.0f;

    int got;
    while ((got = processor.pullSamples (scratch.data(), (int) scratch.size())) > 0)
        for (int i = 0; i < got; ++i)
        {
            ring[(size_t) writePos] = scratch[(size_t) i];
            writePos = (writePos + 1) & (fftSize - 1);
        }

    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = ring[(size_t) ((writePos + i) & (fftSize - 1))];
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    // Hann: sinus 0 dBFS daje |X| ~ N/4  ->  0 dB na wykresie
    const float norm = 1.0f / ((float) fftSize * 0.25f);
    for (size_t k = 0; k < smoothed.size(); ++k)
    {
        const float db = Decibels::gainToDecibels (fftData[k] * norm, kMinDb - 20.0f);
        float& s = smoothed[k];
        s += (db - s) * (db > s ? 0.5f : 0.15f); // szybki atak, wolniejsze opadanie
    }
}

void SpectrumAnalyzer::findPeaks()
{
    peaks.clear();
    const float binHz = sampleRate / (float) fftSize;
    const int k0 = jmax (2, (int) std::ceil (30.0f / binHz));
    const int k1 = jmin ((int) smoothed.size() - 3, (int) std::floor (8000.0f / binHz));

    std::vector<Peak> cand;
    for (int k = k0; k <= k1; ++k)
    {
        const float v = smoothed[(size_t) k];
        if (v < -60.0f) continue;
        if (v > smoothed[(size_t) k - 1] && v >= smoothed[(size_t) k + 1]
            && v > smoothed[(size_t) k - 2] && v >= smoothed[(size_t) k + 2])
        {
            const float a = smoothed[(size_t) k - 1], c = smoothed[(size_t) k + 1];
            const float denom = a - 2.0f * v + c;
            const float delta = denom != 0.0f ? 0.5f * (a - c) / denom : 0.0f; // interpolacja paraboliczna
            cand.push_back ({ ((float) k + delta) * binHz, v - 0.25f * (a - c) * delta });
        }
    }
    if (cand.empty()) return;

    std::sort (cand.begin(), cand.end(), [] (const Peak& x, const Peak& y) { return x.db > y.db; });
    const float top = cand.front().db;

    for (auto& c : cand)
    {
        if (peaks.size() >= 3 || c.db < top - 30.0f) break;
        bool distinct = true;
        for (auto& p : peaks)
            if (std::abs (12.0f * std::log2 (c.hz / p.hz)) < 0.7f) { distinct = false; break; }
        if (distinct) peaks.push_back (c);
    }
}

float SpectrumAnalyzer::valueAt (float hz0, float hz1) const
{
    const float binHz = sampleRate / (float) fftSize;
    const float p0 = hz0 / binHz, p1 = hz1 / binHz;
    const int maxBin = (int) smoothed.size() - 2;

    if (p1 - p0 < 1.0f) // niskie częstotliwości: interpolacja liniowa między binami
    {
        const float p = jlimit (1.0f, (float) maxBin, 0.5f * (p0 + p1));
        const int i = (int) p;
        const float f = p - (float) i;
        return smoothed[(size_t) i] + (smoothed[(size_t) i + 1] - smoothed[(size_t) i]) * f;
    }
    const int b0 = jmax (1, (int) std::ceil (p0));
    const int b1 = jmin (maxBin, (int) std::floor (p1));
    float m = kMinDb;
    for (int b = b0; b <= b1; ++b) m = jmax (m, smoothed[(size_t) b]); // wysokie: maksimum z binów
    return m;
}

// ---------------------------------------------------------------- mysz
Rectangle<float> SpectrumAnalyzer::plotArea() const
{
    auto b = getLocalBounds().toFloat();
    b.removeFromLeft (38.0f);
    b.removeFromRight (8.0f);
    b.removeFromTop (6.0f);
    b.removeFromBottom (kAxisH);
    return b;
}

void SpectrumAnalyzer::mouseMove (const MouseEvent& e)
{
    mousePos = e.position;
    const auto plot = plotArea();
    mouseInside = plot.contains (mousePos);
    hoveredZone = -1;
    if (! mouseInside) return;

    const float f = xToHz (plot, mousePos.x);
    float bestWidth = 1.0e9f;
    for (size_t i = 0; i < zones.size(); ++i)
    {
        auto& z = zones[i];
        if (z.enabled && f >= z.from && f <= z.to && (z.to - z.from) < bestWidth)
        {
            bestWidth = z.to - z.from;
            hoveredZone = (int) i;
        }
    }
}

void SpectrumAnalyzer::mouseExit (const MouseEvent&)
{
    mouseInside = false;
    hoveredZone = -1;
}

// ---------------------------------------------------------------- rysowanie
void SpectrumAnalyzer::paint (Graphics& g)
{
    g.fillAll (kBg);
    const auto plot = plotArea();
    drawKeyBands (g, plot);
    drawZones (g, plot);
    drawGrid (g, plot);
    drawSpectrum (g, plot);
    drawPeaks (g, plot);
    drawAxes (g, plot);
    drawInfo (g, plot);
}

void SpectrumAnalyzer::drawKeyBands (Graphics& g, const Rectangle<float>& plot)
{
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (plot.toNearestInt());

    const int n0 = (int) std::ceil (midiFromHz (kMinHz)), n1 = (int) std::floor (midiFromHz (kMaxHz));
    for (int n = n0; n <= n1; ++n)
    {
        const float x0 = hzToX (plot, (float) hzFromMidi (n - 0.5));
        const float x1 = hzToX (plot, (float) hzFromMidi (n + 0.5));
        if (isBlackKey (n)) // czarne klawisze: delikatne pasmo
        {
            g.setColour (Colours::white.withAlpha (0.03f));
            g.fillRect (x0, plot.getY(), x1 - x0, plot.getHeight());
        }
        if (n % 12 == 0) // linia na każdym C
        {
            g.setColour (Colours::white.withAlpha (0.10f));
            g.drawVerticalLine ((int) hzToX (plot, (float) hzFromMidi (n)), plot.getY(), plot.getBottom());
        }
    }
}

void SpectrumAnalyzer::drawZones (Graphics& g, const Rectangle<float>& plot)
{
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (plot.toNearestInt());
    g.setFont (11.0f);

    for (size_t i = 0; i < zones.size(); ++i)
    {
        auto& z = zones[i];
        if (! z.enabled) continue;

        const float x0 = hzToX (plot, z.from), x1 = hzToX (plot, z.to);
        const bool hov = (int) i == hoveredZone;

        g.setColour (z.colour.withAlpha (hov ? 0.22f : 0.09f));
        g.fillRect (x0, plot.getY(), x1 - x0, plot.getHeight());
        g.setColour (z.colour.withAlpha (hov ? 0.8f : 0.35f));
        g.drawVerticalLine ((int) x0, plot.getY(), plot.getBottom());
        g.drawVerticalLine ((int) x1, plot.getY(), plot.getBottom());

        const float ty = plot.getBottom() - 16.0f - (float) (i % 4) * 14.0f; // 4 rzędy etykiet
        g.setColour (z.colour.brighter (0.3f).withAlpha (0.95f));
        g.drawText (z.title, Rectangle<float> (x0 + 3.0f, ty, 220.0f, 13.0f), Justification::centredLeft, false);
    }
}

void SpectrumAnalyzer::drawGrid (Graphics& g, const Rectangle<float>& plot)
{
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (plot.toNearestInt());
        for (float decade : { 10.0f, 100.0f, 1000.0f, 10000.0f })
            for (int m = 1; m <= 9; ++m)
            {
                const float f = decade * (float) m;
                if (f < kMinHz || f > kMaxHz) continue;
                const bool major = (m == 1 || m == 2 || m == 5);
                g.setColour (Colours::white.withAlpha (major ? 0.10f : 0.045f));
                g.drawVerticalLine ((int) hzToX (plot, f), plot.getY(), plot.getBottom());
            }
        for (int db = 12; db > (int) kMinDb; db -= 12)
        {
            g.setColour (Colours::white.withAlpha (db == 0 ? 0.20f : 0.06f));
            g.drawHorizontalLine ((int) dbToY (plot, (float) db), plot.getX(), plot.getRight());
        }
    }

    g.setFont (10.0f);
    g.setColour (Colours::grey);
    for (int db = 12; db > (int) kMinDb; db -= 12)
        g.drawText (String (db), Rectangle<float> (0.0f, dbToY (plot, (float) db) - 7.0f, 34.0f, 14.0f),
                    Justification::centredRight, false);
}

void SpectrumAnalyzer::drawSpectrum (Graphics& g, const Rectangle<float>& plot)
{
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (plot.toNearestInt());

    Path line;
    const int w = (int) plot.getWidth();
    for (int px = 0; px <= w; ++px)
    {
        const float x = plot.getX() + (float) px;
        const float y = dbToY (plot, valueAt (xToHz (plot, x), xToHz (plot, x + 1.0f)));
        if (px == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
    }

    Path fill (line);
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.lineTo (plot.getX(), plot.getBottom());
    fill.closeSubPath();

    ColourGradient grad = ColourGradient::horizontal (kCyan, plot.getX(), kMagenta, plot.getRight());
    ColourGradient fillGrad (grad);
    fillGrad.multiplyOpacity (0.22f);

    g.setGradientFill (fillGrad);
    g.fillPath (fill);
    g.setGradientFill (grad);
    g.strokePath (line, PathStrokeType (1.6f, PathStrokeType::curved));
}

void SpectrumAnalyzer::drawPeaks (Graphics& g, const Rectangle<float>& plot)
{
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (plot.toNearestInt());
    g.setFont (11.0f);

    for (size_t i = 0; i < peaks.size(); ++i)
    {
        const auto& p = peaks[i];
        const float x = hzToX (plot, p.hz), y = dbToY (plot, p.db);
        const auto info = describeHz (p.hz);
        const int cents = (int) std::lround (info.cents);
        const String text = info.name + " " + (cents >= 0 ? "+" : "") + String (cents) + "c  (" + fmtHz (p.hz) + ")";

        const float w = (float) text.length() * 6.4f + 14.0f;
        const float lx = jlimit (plot.getX() + 2.0f, plot.getRight() - w - 2.0f, x - w * 0.5f);
        const float ly = plot.getY() + 4.0f + (float) i * 20.0f;

        if (y > ly + 16.0f)
        {
            g.setColour (kLime.withAlpha (0.35f));
            g.drawLine (x, ly + 16.0f, x, y, 1.0f);
        }
        g.setColour (kLime);
        g.fillEllipse (x - 3.0f, y - 3.0f, 6.0f, 6.0f);

        g.setColour (Colour (0xdd101014));
        g.fillRoundedRectangle (lx, ly, w, 16.0f, 4.0f);
        g.setColour (kLime);
        g.drawRoundedRectangle (lx, ly, w, 16.0f, 4.0f, 1.0f);
        g.drawText (text, Rectangle<float> (lx, ly, w, 16.0f), Justification::centred, false);
    }
}

void SpectrumAnalyzer::drawAxes (Graphics& g, const Rectangle<float>& plot)
{
    // etykiety Hz
    g.setFont (10.0f);
    g.setColour (Colours::grey);
    for (float decade : { 10.0f, 100.0f, 1000.0f, 10000.0f })
        for (int m : { 1, 2, 5 })
        {
            const float f = decade * (float) m;
            if (f < kMinHz || f > kMaxHz) continue;
            g.drawText (fmtShort (f), Rectangle<float> (hzToX (plot, f) - 20.0f, plot.getBottom() + 1.0f, 40.0f, 14.0f),
                        Justification::centred, false);
        }

    // pasek nut pod osią Hz (strój A4 = 440 Hz)
    const Rectangle<float> strip (plot.getX(), plot.getBottom() + 16.0f, plot.getWidth(), 18.0f);
    g.setColour (Colour (0xff0d0d11));
    g.fillRect (strip);

    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (strip.toNearestInt());
    g.setFont (9.0f);

    const int n0 = (int) std::ceil (midiFromHz (kMinHz)), n1 = (int) std::floor (midiFromHz (kMaxHz));
    for (int n = n0; n <= n1; ++n)
    {
        const float x0 = jmax (plot.getX(),     hzToX (plot, (float) hzFromMidi (n - 0.5)));
        const float x1 = jmin (plot.getRight(), hzToX (plot, (float) hzFromMidi (n + 0.5)));
        if (x1 <= x0) continue;

        g.setColour (isBlackKey (n) ? Colour (0xff060608) : Colour (0xff2b2b33));
        g.fillRect (x0 + 0.5f, strip.getY() + 1.0f, jmax (0.5f, x1 - x0 - 1.0f), strip.getHeight() - 2.0f);

        const bool isC = (n % 12) == 0;
        if (isC || (x1 - x0) >= 22.0f) // przy małej szerokości tylko C1, C2, ...
        {
            g.setColour (isC ? kCyan : Colours::lightgrey.withAlpha (0.8f));
            g.drawText (noteName (n), Rectangle<float> (x0 - 10.0f, strip.getY(), (x1 - x0) + 20.0f, strip.getHeight()),
                        Justification::centred, false);
        }
    }
}

void SpectrumAnalyzer::drawInfo (Graphics& g, const Rectangle<float>& plot)
{
    if (mouseInside)
    {
        g.setColour (Colours::white.withAlpha (0.25f));
        g.drawLine (mousePos.x, plot.getY(), mousePos.x, plot.getBottom(), 1.0f);
    }

    const Rectangle<float> box (plot.getX(), plot.getBottom() + 38.0f, plot.getWidth(), kAxisH - 38.0f - 4.0f);
    g.setColour (Colour (0xff202028));
    g.fillRoundedRectangle (box, 6.0f);
    auto inner = box.reduced (10.0f, 5.0f).toNearestInt();

    String line1;
    if (mouseInside)
    {
        const double hz = xToHz (plot, mousePos.x);
        const auto n = describeHz (hz);
        const int c = (int) std::lround (n.cents);
        line1 = fmtHz (hz) + U8 ("   ≈   ") + n.name + " " + (c >= 0 ? "+" : "") + String (c) + "c";
    }
    else
    {
        line1 = U8 ("Najedź kursorem na wykres: częstotliwość, nuta i opis strefy");
    }

    g.setFont (12.0f);
    g.setColour (Colours::white.withAlpha (0.9f));
    g.drawText (line1, inner.removeFromTop (16), Justification::centredLeft, true);

    if (isPositiveAndBelow (hoveredZone, (int) zones.size()))
    {
        const auto& z = zones[(size_t) hoveredZone];
        String text;
        if (z.detail.isNotEmpty())
            text = z.title + U8 (" — ") + z.detail;
        else
        {
            text = z.title + "  [" + fmtRange (z.from, z.to) + "]";
            if (z.add.isNotEmpty()) text << "\n+ Dodaj: " << z.add;
            if (z.cut.isNotEmpty()) text << "\n- Odejmij: " << z.cut;
        }
        g.setFont (11.0f);
        g.setColour (z.colour.brighter (0.4f));
        g.drawFittedText (text, inner, Justification::topLeft, 3);
    }
}
