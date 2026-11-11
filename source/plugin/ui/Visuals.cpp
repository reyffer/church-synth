#include "Visuals.h"

#include "dsp/Content.h"
#include "dsp/Envelope.h"
#include "dsp/MultiFilter.h"

namespace ui
{
namespace
{
float waveValue (int wave, float ph, float width)
{
    switch (wave)
    {
        case 0: return std::sin (juce::MathConstants<float>::twoPi * ph);
        case 1: return ph < 0.25f ? 4.0f * ph : ph < 0.75f ? 2.0f - 4.0f * ph : 4.0f * ph - 4.0f;
        case 2: return 1.0f - 2.0f * ph;
        case 3: return 2.0f * ph - 1.0f;
        case 4:
        default: return ph < juce::jlimit (0.05f, 0.95f, width) ? 1.0f : -1.0f;
    }
}

void fillUnder (juce::Graphics& g, const juce::Path& line, juce::Rectangle<float> r, float baseY, juce::Colour c)
{
    juce::Path area (line);
    area.lineTo (r.getRight(), baseY);
    area.lineTo (r.getX(), baseY);
    area.closeSubPath();
    g.setGradientFill (juce::ColourGradient (c.withAlpha (0.35f), r.getCentreX(), r.getY(), c.withAlpha (0.02f), r.getCentreX(), r.getBottom(), false));
    g.fillPath (area);
}

// Travelling dots: where each sounding voice is in an envelope / LFO / function.
// kind 0 = envelope (position = stage + progress), 1 = LFO phase, 2 = function position.
struct VoiceDot
{
    float pos = 0.0f, level = 0.0f;
};
std::vector<VoiceDot> voiceDots (UiContext& ctx, int kind, int k)
{
    struct Tmp { int age; VoiceDot d; };
    std::vector<Tmp> t;
    const auto& synth = ctx.proc.getSynth();
    for (const auto& u : synth.uiVoice)
    {
        const int age = u.age.load (std::memory_order_relaxed);
        if (age < 0)
            continue;
        VoiceDot d;
        d.pos = kind == 0 ? u.env[k].load (std::memory_order_relaxed) : kind == 1 ? u.lfo[k].load (std::memory_order_relaxed)
                                                                                  : u.func[k].load (std::memory_order_relaxed);
        if (kind == 0)
            d.level = u.envLevel[k].load (std::memory_order_relaxed);
        if (d.pos < 0.0f || ! std::isfinite (d.pos))
            continue;
        t.push_back ({ age, d });
    }
    std::sort (t.begin(), t.end(), [] (const Tmp& a, const Tmp& b) { return a.age < b.age; }); // oldest first, newest last
    std::vector<VoiceDot> r;
    for (const auto& x : t)
    {
        // Global LFOs / functions report the same position on every voice: draw it once.
        bool dup = false;
        for (auto& y : r)
            if (std::abs (y.pos - x.d.pos) < 0.002f && std::abs (y.level - x.d.level) < 0.002f)
                dup = true;
        if (! dup)
            r.push_back (x.d);
    }
    return r;
}

// Newest voice: a bright white dot; older voices: fainter and smaller.
void drawDot (juce::Graphics& g, juce::Point<float> p, bool newest)
{
    const float rad = newest ? 4.5f : 3.2f;
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (p.x - rad - 1.0f, p.y - rad - 1.0f, (rad + 1.0f) * 2.0f, (rad + 1.0f) * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (newest ? 1.0f : 0.5f));
    g.fillEllipse (p.x - rad, p.y - rad, rad * 2.0f, rad * 2.0f);
}
} // namespace

void drawWave (juce::Graphics& g, juce::Rectangle<float> r, int wave, float width, juce::Colour c, float thickness, bool fill)
{
    juce::Path p;
    const int n = juce::jmax (16, (int) r.getWidth());
    for (int i = 0; i <= n; ++i)
    {
        const float ph = (float) i / (float) n;
        const float y = r.getCentreY() - waveValue (wave, ph, width) * r.getHeight() * 0.45f;
        const float x = r.getX() + ph * r.getWidth();
        if (i == 0)
            p.startNewSubPath (x, y);
        else
            p.lineTo (x, y);
    }
    if (fill)
        fillUnder (g, p, r, r.getBottom(), c);
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ===========================================================================
void OscDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float level = juce::jlimit (0.05f, 1.0f, ctx.value (vol));
    drawWave (g, r.reduced (2.0f, r.getHeight() * 0.5f * (1.0f - level) + 3.0f), ctx.intValue (wave), ctx.value (width),
              col::accent, 1.6f, true);
}

WaveSelector::WaveSelector (UiContext& c, juce::String waveId) : ctx (c), id (std::move (waveId))
{
    if (auto* p = ctx.param (id))
    {
        att = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { current = (int) std::lround (v); repaint(); });
        att->sendInitialUpdate();
    }
    setTooltip ("Waveform");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void WaveSelector::paint (juce::Graphics& g)
{
    const float w = (float) getWidth() / 5.0f;
    for (int i = 0; i < 5; ++i)
    {
        auto cell = juce::Rectangle<float> ((float) i * w, 0.0f, w, (float) getHeight()).reduced (3.0f, 2.0f);
        drawWave (g, cell, i, 0.5f, i == current ? col::accent : col::textFaint, i == current ? 1.6f : 1.2f);
    }
}

void WaveSelector::mouseDown (const juce::MouseEvent& e)
{
    if (att != nullptr)
        att->setValueAsCompleteGesture ((float) juce::jlimit (0, 4, e.x * 5 / juce::jmax (1, getWidth())));
}

// ===========================================================================
void WavetableDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::panelLo);
    g.fillRoundedRectangle (r, 5.0f);
    const auto* table = ctx.proc.currentWavetable (eng);
    if (table == nullptr || table->getNumFrames() <= 0)
    {
        g.setColour (col::textFaint);
        g.setFont (font (12.0f));
        g.drawText ("loading...", r, juce::Justification::centred);
        return;
    }
    const juce::String p = "e" + juce::String (eng + 1) + "_wt_";
    const float pos = ctx.value (p + "pos");
    const int frames = table->getNumFrames();
    const int level = 2;
    const int size = table->getLevelSize (level);
    const int shown = juce::jmin (frames, 20);
    const auto inner = r.reduced (10.0f, 10.0f);
    const float depthX = inner.getWidth() * 0.18f, depthY = inner.getHeight() * 0.32f;
    const float fw = inner.getWidth() - depthX, fh = inner.getHeight() - depthY;

    auto framePath = [&] (int idx, float ox, float oy) {
        const float* data = table->frame (level, idx);
        juce::Path path;
        const int pts = 96;
        for (int i = 0; i <= pts; ++i)
        {
            const float v = data[(i * size / pts) % size];
            const float x = inner.getX() + ox + fw * (float) i / (float) pts;
            const float y = inner.getY() + oy + fh * 0.5f - v * fh * 0.42f;
            if (i == 0)
                path.startNewSubPath (x, y);
            else
                path.lineTo (x, y);
        }
        return path;
    };
    for (int s = shown - 1; s >= 0; --s)
    {
        const float t = shown > 1 ? (float) s / (float) (shown - 1) : 0.0f;
        const int idx = juce::jlimit (0, frames - 1, (int) std::lround (t * (float) (frames - 1)));
        g.setColour (col::accent.withAlpha (0.10f + 0.12f * (1.0f - t)));
        g.strokePath (framePath (idx, depthX * t, depthY * (1.0f - t)), juce::PathStrokeType (1.0f));
    }
    const int cur = juce::jlimit (0, frames - 1, (int) std::lround (pos * (float) (frames - 1)));
    const float tc = frames > 1 ? (float) cur / (float) (frames - 1) : 0.0f;
    g.setColour (col::accent);
    g.strokePath (framePath (cur, depthX * tc, depthY * (1.0f - tc)), juce::PathStrokeType (2.0f));
    g.setColour (col::textDim);
    g.setFont (font (10.5f));
    g.drawText ("frame " + juce::String (cur + 1) + " / " + juce::String (frames), r.reduced (8.0f, 4.0f), juce::Justification::bottomRight);
}

// ===========================================================================
void FilterDisplay::refreshView()
{
    const juce::String p = "f" + juce::String (index + 1) + "_";
    juce::String key;
    for (auto* s : { "on", "type", "mode", "cutoff", "reso", "drive", "analog", "a", "b", "c", "submode" })
        key << juce::String (ctx.value (p + s), 3) << ",";
    if (key == lastKey)
        return;
    lastKey = key;
    active = ctx.boolValue (p + "on");

    static auto filter = std::make_unique<cs::MultiFilter>();
    constexpr float sr = 48000.0f;
    cs::FilterParams fp;
    fp.on = true;
    fp.type = ctx.intValue (p + "type");
    fp.mode = ctx.intValue (p + "mode");
    fp.cutoff = ctx.value (p + "cutoff");
    fp.reso = ctx.value (p + "reso");
    fp.drive = ctx.value (p + "drive");
    fp.analog = ctx.boolValue (p + "analog");
    fp.a = ctx.value (p + "a");
    fp.b = ctx.value (p + "b");
    fp.c = ctx.value (p + "c");
    fp.submode = ctx.intValue (p + "submode");
    filter->setSampleRate (sr);
    filter->reset();
    for (int i = 0; i < 400; ++i) // let control-rate state (e.g. the LPG vactrol) settle
        filter->setup (fp, fp.cutoff);

    constexpr int n = 4096;
    std::vector<float> ir (n);
    for (int i = 0; i < n; ++i)
    {
        if (i % cs::kBlock == 0)
            filter->setup (fp, fp.cutoff);
        float l = i == 0 ? 0.25f : 0.0f, r = l;
        filter->process (l, r, 0.0f);
        ir[(size_t) i] = std::isfinite (l) ? l * 4.0f : 0.0f;
    }
    constexpr int bins = 120;
    mags.assign (bins, -60.0f);
    for (int b = 0; b < bins; ++b)
    {
        const double f = 20.0 * std::pow (1000.0, (double) b / (bins - 1));
        const double w = juce::MathConstants<double>::twoPi * f / sr;
        double re = 0.0, im = 0.0;
        for (int i = 0; i < n; ++i)
        {
            re += ir[(size_t) i] * std::cos (w * i);
            im -= ir[(size_t) i] * std::sin (w * i);
        }
        mags[(size_t) b] = (float) juce::jlimit (-60.0, 30.0, 20.0 * std::log10 (std::sqrt (re * re + im * im) + 1.0e-9));
    }
    repaint();
}

void FilterDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::panelLo);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (col::border.withAlpha (0.5f));
    for (float f : { 100.0f, 1000.0f, 10000.0f })
    {
        const float x = r.getX() + r.getWidth() * std::log (f / 20.0f) / std::log (1000.0f);
        g.drawVerticalLine ((int) x, r.getY() + 2.0f, r.getBottom() - 2.0f);
    }
    if (mags.empty())
        return;
    const float top = 18.0f, bottom = -36.0f; // dB range shown
    juce::Path p;
    for (size_t i = 0; i < mags.size(); ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) (mags.size() - 1);
        const float y = r.getY() + r.getHeight() * (top - juce::jlimit (bottom, top, mags[i])) / (top - bottom);
        if (i == 0)
            p.startNewSubPath (x, y);
        else
            p.lineTo (x, y);
    }
    const auto c = active ? col::accent : col::textFaint;
    fillUnder (g, p, r, r.getBottom(), c);
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (1.6f));
    if (! active)
    {
        g.setColour (col::textDim);
        g.setFont (font (11.0f, true));
        g.drawText ("BYPASSED", r, juce::Justification::centred);
    }
}

// ===========================================================================
EnvDisplay::EnvDisplay (UiContext& c, juce::String prefix, juce::Colour colour) : ctx (c), p (std::move (prefix)), colourUsed (colour)
{
    setTooltip ("Drag the points: attack, decay and sustain, release. Double-click a point to reset it");
}

juce::StringArray EnvDisplay::paramsFor (int point) const
{
    if (point == 0) return { p + "_a" };
    if (point == 1) return { p + "_d", p + "_s" };
    return { p + "_r" };
}

int EnvDisplay::hitPoint (juce::Point<float> pos) const
{
    int best = -1;
    float bestD = 12.0f;
    for (int i = 0; i < 3; ++i)
        if (const float d = points[i].getDistanceFrom (pos); d < bestD)
        {
            bestD = d;
            best = i;
        }
    return best;
}

void EnvDisplay::mouseMove (const juce::MouseEvent& e)
{
    const int h = hitPoint (e.position);
    if (h != hoverPoint)
    {
        hoverPoint = h;
        setMouseCursor (h >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void EnvDisplay::mouseDown (const juce::MouseEvent& e)
{
    dragPoint = hitPoint (e.position);
    if (dragPoint < 0)
        return;
    const auto ids = paramsFor (dragPoint);
    for (int i = 0; i < ids.size(); ++i)
        if (auto* prm = ctx.param (ids[i]))
        {
            startNorm[i] = prm->getValue();
            prm->beginChangeGesture();
        }
}

void EnvDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (dragPoint < 0)
        return;
    const auto ids = paramsFor (dragPoint);
    // Time: relative horizontal drag (the curve rescales as times change); shift = fine.
    const float speed = e.mods.isShiftDown() ? 1.0f / 1200.0f : 1.0f / 300.0f;
    if (auto* prm = ctx.param (ids[0]))
        prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, startNorm[0] + (float) e.getDistanceFromDragStartX() * speed));
    // Sustain: absolute level.
    if (dragPoint == 1 && ! ctx.boolValue (p + "_adr"))
        if (auto* prm = ctx.param (ids[1]))
        {
            const auto r = getLocalBounds().toFloat().reduced (2.0f, 4.0f);
            const float level = juce::jlimit (0.0f, 1.0f, (r.getBottom() - e.position.y) / r.getHeight());
            prm->setValueNotifyingHost (prm->convertTo0to1 (prm->convertFrom0to1 (0.0f) + level * (prm->convertFrom0to1 (1.0f) - prm->convertFrom0to1 (0.0f))));
        }
    repaint();
}

void EnvDisplay::mouseUp (const juce::MouseEvent&)
{
    if (dragPoint >= 0)
        for (const auto& id : paramsFor (dragPoint))
            if (auto* prm = ctx.param (id))
                prm->endChangeGesture();
    dragPoint = -1;
}

void EnvDisplay::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int h = hitPoint (e.position);
    if (h < 0)
        return;
    for (const auto& id : paramsFor (h))
        if (auto* prm = ctx.param (id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->getDefaultValue());
            prm->endChangeGesture();
        }
}

void EnvDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f, 4.0f);
    const float a = ctx.value (p + "_a"), d = ctx.value (p + "_d"), s = ctx.value (p + "_s"), rel = ctx.value (p + "_r");
    const float ac = ctx.value (p + "_acurve"), dc = ctx.value (p + "_dcurve");
    const bool adr = ctx.boolValue (p + "_adr");
    // Segment widths grow with sqrt(time) so short and long envelopes both read well.
    auto w = [] (float t) { return std::sqrt (juce::jmax (0.001f, t)) + 0.05f; };
    const float wa = w (a), wd = w (d), wh = adr ? 0.0f : 0.9f, wr = w (rel);
    const float total = wa + wd + wh + wr;
    const float sx = r.getWidth() / total;
    auto yOf = [&] (float level) { return r.getBottom() - level * r.getHeight(); };
    auto shape = [] (float t, float curve) {
        const float k = curve * 0.3f;
        return std::abs (k) < 1.0e-3f ? t : std::expm1 (-k * t) / std::expm1 (-k);
    };
    juce::Path path;
    path.startNewSubPath (r.getX(), r.getBottom());
    const int steps = 40;
    float x = r.getX();
    for (int i = 1; i <= steps; ++i)
    {
        const float t = (float) i / steps;
        path.lineTo (x + t * wa * sx, yOf (shape (t, -ac)));
    }
    x += wa * sx;
    const float sus = adr ? 0.0f : s;
    for (int i = 1; i <= steps; ++i)
    {
        const float t = (float) i / steps;
        path.lineTo (x + t * wd * sx, yOf (sus + (1.0f - sus) * (1.0f - shape (t, dc))));
    }
    x += wd * sx;
    if (! adr)
    {
        path.lineTo (x + wh * sx, yOf (sus));
        x += wh * sx;
    }
    for (int i = 1; i <= steps; ++i)
    {
        const float t = (float) i / steps;
        path.lineTo (x + t * wr * sx, yOf (sus * (1.0f - shape (t, dc))));
    }
    fillUnder (g, path, r, r.getBottom(), colourUsed);
    g.setColour (colourUsed);
    g.strokePath (path, juce::PathStrokeType (1.6f));
    // Travelling dots: where each sounding note is in this envelope.
    {
        const int k = juce::jlimit (0, 2, p.getTrailingIntValue() - 1);
        const auto dots = voiceDots (ctx, 0, k);
        for (size_t i = 0; i < dots.size(); ++i)
        {
            const float pos = dots[i].pos;
            const int stage = (int) std::floor (pos);
            const float t = pos - (float) stage;
            float dx = r.getX();
            if (stage == 0)
                dx += t * wa * sx;
            else if (stage == 1)
                dx += (wa + t * wd) * sx;
            else if (stage == 2)
                dx += (wa + wd + 0.5f * wh) * sx;
            else
                dx += (wa + wd + wh + t * wr) * sx;
            drawDot (g, { dx, yOf (juce::jlimit (0.0f, 1.0f, dots[i].level)) }, i + 1 == dots.size());
        }
    }
    // Points (draggable)
    points[0] = { r.getX() + wa * sx, yOf (1.0f) };
    points[1] = { r.getX() + (wa + wd) * sx, yOf (sus) };
    points[2] = { r.getRight(), r.getBottom() };
    for (int i = 0; i < 3; ++i)
    {
        const auto pt = points[i];
        const bool hot = i == hoverPoint || i == dragPoint;
        const float rad = hot ? 4.5f : 3.0f;
        g.setColour (hot ? colourUsed.brighter (0.5f) : colourUsed.brighter (0.3f));
        if (hot)
            g.fillEllipse (pt.x - rad, pt.y - rad, rad * 2.0f, rad * 2.0f);
        else
            g.drawEllipse (pt.x - rad, pt.y - rad, rad * 2.0f, rad * 2.0f, 1.4f);
    }
}

// ===========================================================================
void LfoDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f, 6.0f);
    const juce::String p = "lfo" + juce::String (k + 1);
    cs::LfoParams lp;
    lp.wave = ctx.intValue (p + "_wave");
    lp.symmetry = ctx.value (p + "_sym");
    const bool bipolar = ctx.boolValue (p + "_bipolar");
    const float sym = juce::jlimit (0.02f, 0.98f, lp.symmetry);
    juce::Random rng (1234 + k);
    float held = 0.0f;
    juce::Path path;
    const int n = 160;
    float ys[n + 1] {};
    for (int i = 0; i <= n; ++i)
    {
        const float ph = (float) i / n;
        const float q = ph < sym ? 0.5f * ph / sym : 0.5f + 0.5f * (ph - sym) / (1.0f - sym);
        float v = 0.0f;
        switch (lp.wave)
        {
            case cs::LfoSine: v = std::sin (juce::MathConstants<float>::twoPi * q); break;
            case cs::LfoTriangle: v = q < 0.25f ? 4.0f * q : q < 0.75f ? 2.0f - 4.0f * q : 4.0f * q - 4.0f; break;
            case cs::LfoSquare: v = q < 0.5f ? 1.0f : -1.0f; break;
            case cs::LfoSaw: v = 1.0f - 2.0f * q; break;
            case cs::LfoRamp: v = 2.0f * q - 1.0f; break;
            default:
                if (i % 20 == 0)
                    held = rng.nextFloat() * 2.0f - 1.0f;
                v = held;
                break;
        }
        if (! bipolar)
            v = 0.5f + 0.5f * v;
        const float y = bipolar ? r.getCentreY() - v * r.getHeight() * 0.48f : r.getBottom() - v * r.getHeight() * 0.96f;
        ys[i] = y;
        const float x = r.getX() + ph * r.getWidth();
        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }
    g.setColour (col::border.withAlpha (0.6f));
    g.drawHorizontalLine ((int) (bipolar ? r.getCentreY() : r.getBottom()), r.getX(), r.getRight());
    fillUnder (g, path, r, bipolar ? r.getCentreY() : r.getBottom(), col::modLfo);
    g.setColour (col::modLfo);
    g.strokePath (path, juce::PathStrokeType (1.6f));
    // Travelling dots (one per sounding note, or one for a shared LFO); otherwise the live value.
    const auto dots = voiceDots (ctx, 1, k);
    for (size_t i = 0; i < dots.size(); ++i)
    {
        const float ph = juce::jlimit (0.0f, 1.0f, dots[i].pos);
        drawDot (g, { r.getX() + ph * r.getWidth(), ys[juce::jlimit (0, n, (int) std::lround (ph * n))] }, i + 1 == dots.size());
    }
    if (dots.empty())
    {
        const float live = ctx.proc.getSynth().uiSource[cs::SrcLfo1 + k].load (std::memory_order_relaxed);
        const float ly = bipolar ? r.getCentreY() - live * r.getHeight() * 0.48f : r.getBottom() - live * r.getHeight() * 0.96f;
        g.setColour (col::modLfo.brighter (0.4f));
        g.fillEllipse (r.getRight() - 5.0f, ly - 3.0f, 6.0f, 6.0f);
    }
}

// ===========================================================================
cs::FunctionShape FunctionEditor::current() const
{
    const int idx = ctx.intValue ("func" + juce::String (k + 1) + "_shape");
    if (idx >= cs::kNumFunctionPresets)
        return ctx.proc.getCustomShape (k);
    return cs::functionPreset (idx);
}

juce::Point<float> FunctionEditor::toScreen (float x, float y) const
{
    const auto r = plot();
    return { r.getX() + x * r.getWidth(), r.getBottom() - juce::jlimit (0.0f, 1.0f, y) * r.getHeight() };
}

juce::Point<float> FunctionEditor::fromScreen (juce::Point<float> p) const
{
    const auto r = plot();
    return { juce::jlimit (0.0f, 1.0f, (p.x - r.getX()) / r.getWidth()), juce::jlimit (0.0f, 1.0f, (r.getBottom() - p.y) / r.getHeight()) };
}

void FunctionEditor::paint (juce::Graphics& g)
{
    const auto r = plot();
    g.setColour (col::panelLo);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
    g.setColour (col::border.withAlpha (0.5f));
    for (int i = 1; i < 4; ++i)
        g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) i / 4.0f), r.getY(), r.getBottom());
    const auto s = current();
    juce::Path path;
    const int n = juce::jmax (64, (int) r.getWidth());
    for (int i = 0; i <= n; ++i)
    {
        const float t = (float) i / (float) n;
        const auto pt = toScreen (t, s.eval (t));
        if (i == 0)
            path.startNewSubPath (pt);
        else
            path.lineTo (pt);
    }
    fillUnder (g, path, r, r.getBottom(), col::modFunc);
    g.setColour (col::modFunc);
    g.strokePath (path, juce::PathStrokeType (1.6f));
    for (int i = 0; i < s.count; ++i)
    {
        const auto pt = toScreen (s.x[i], s.y[i]);
        const bool special = i == s.sustain;
        g.setColour (special ? col::gold : col::modFunc.brighter (0.4f));
        g.drawEllipse (pt.x - 3.5f, pt.y - 3.5f, 7.0f, 7.0f, 1.5f);
    }
    const int mode = ctx.intValue ("func" + juce::String (k + 1) + "_mode");
    if (mode >= 2 && s.sustain >= 0)
    {
        g.setColour (col::gold.withAlpha (0.6f));
        const auto pt = toScreen (s.x[s.sustain], 0.0f);
        g.drawVerticalLine ((int) pt.x, r.getY(), r.getBottom());
    }
    // Travelling dots (one per sounding note, or one for a shared function); otherwise the output level.
    const auto dots = voiceDots (ctx, 2, k);
    for (size_t i = 0; i < dots.size(); ++i)
    {
        const float x = juce::jlimit (0.0f, 1.0f, dots[i].pos);
        drawDot (g, toScreen (x, s.eval (x)), i + 1 == dots.size());
    }
    if (dots.empty())
    {
        const float live = ctx.proc.getSynth().uiSource[cs::SrcFunc1 + k].load (std::memory_order_relaxed);
        g.setColour (col::modFunc.brighter (0.5f));
        g.fillEllipse (r.getRight() - 4.0f, r.getBottom() - juce::jlimit (0.0f, 1.0f, live) * r.getHeight() - 3.0f, 6.0f, 6.0f);
    }
}

void FunctionEditor::makeCustom (const cs::FunctionShape& s, bool record)
{
    const juce::String id = "func" + juce::String (k + 1) + "_shape";
    ctx.proc.setCustomShape (k, s);
    if (ctx.intValue (id) != cs::kNumFunctionPresets)
        if (auto* p = ctx.param (id))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) cs::kNumFunctionPresets));
    if (record)
        ctx.proc.checkpoint ("Function " + juce::String (k + 1) + " shape");
    repaint();
}

void FunctionEditor::mouseDown (const juce::MouseEvent& e)
{
    working = current();
    dragPoint = dragSegment = -1;
    const auto pos = e.position;
    for (int i = 0; i < working.count; ++i)
        if (toScreen (working.x[i], working.y[i]).getDistanceFrom (pos) < 8.0f)
            dragPoint = i;
    if (e.mods.isPopupMenu() && dragPoint >= 0)
    {
        const int pt = dragPoint;
        juce::PopupMenu m;
        m.addItem (1, "Delete point", pt > 0 && pt < working.count - 1);
        m.addItem (2, "Sustain point (envelope modes)", true, working.sustain == pt);
        m.addItem (3, "Loop start (envelope loop mode)", true, working.loopStart == pt);
        m.addItem (4, "Straight segment");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, pt] (int r) {
            auto s = current();
            if (r == 1 && pt > 0 && pt < s.count - 1)
            {
                for (int i = pt; i < s.count - 1; ++i)
                {
                    s.x[i] = s.x[i + 1];
                    s.y[i] = s.y[i + 1];
                    s.curve[i] = s.curve[i + 1];
                }
                --s.count;
                if (s.sustain >= s.count) s.sustain = s.count - 1;
                if (s.loopStart >= s.count) s.loopStart = 0;
            }
            else if (r == 2)
                s.sustain = s.sustain == pt ? -1 : pt;
            else if (r == 3)
                s.loopStart = pt;
            else if (r == 4)
                s.curve[pt] = 0.0f;
            else
                return;
            makeCustom (s, true);
        });
        dragPoint = -1;
        return;
    }
    if (dragPoint < 0)
    {
        const float t = fromScreen (pos).x;
        for (int i = 0; i < working.count - 1; ++i)
            if (t >= working.x[i] && t <= working.x[i + 1])
                dragSegment = i;
        if (dragSegment >= 0)
            dragCurveStart = working.curve[dragSegment];
    }
}

void FunctionEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragPoint >= 0)
    {
        auto p = fromScreen (e.position);
        const int i = dragPoint;
        if (i == 0)
            p.x = 0.0f;
        else if (i == working.count - 1)
            p.x = 1.0f;
        else
            p.x = juce::jlimit (working.x[i - 1], working.x[i + 1], p.x);
        working.x[i] = p.x;
        working.y[i] = std::round (p.y * 100.0f) / 100.0f;
        makeCustom (working, false);
    }
    else if (dragSegment >= 0)
    {
        // Vertical drag bends the segment.
        const float up = working.y[dragSegment + 1] >= working.y[dragSegment] ? 1.0f : -1.0f;
        working.curve[dragSegment] = juce::jlimit (-1.0f, 1.0f, dragCurveStart + up * (float) e.getDistanceFromDragStartY() / 100.0f);
        makeCustom (working, false);
    }
}

void FunctionEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragPoint >= 0 || dragSegment >= 0)
        ctx.proc.checkpoint ("Function " + juce::String (k + 1) + " shape");
    dragPoint = dragSegment = -1;
}

void FunctionEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto s = current();
    for (int i = 1; i < s.count - 1; ++i)
        if (toScreen (s.x[i], s.y[i]).getDistanceFrom (e.position) < 8.0f)
        {
            for (int j = i; j < s.count - 1; ++j)
            {
                s.x[j] = s.x[j + 1];
                s.y[j] = s.y[j + 1];
                s.curve[j] = s.curve[j + 1];
            }
            --s.count;
            makeCustom (s, true);
            return;
        }
    if (s.count >= cs::FunctionShape::kMaxPoints)
        return;
    const auto p = fromScreen (e.position);
    int at = 1;
    while (at < s.count && s.x[at] < p.x)
        ++at;
    for (int j = s.count; j > at; --j)
    {
        s.x[j] = s.x[j - 1];
        s.y[j] = s.y[j - 1];
        s.curve[j] = s.curve[j - 1];
    }
    s.x[at] = p.x;
    s.y[at] = p.y;
    s.curve[at] = 0.0f;
    ++s.count;
    if (s.sustain >= at) ++s.sustain;
    if (s.loopStart >= at && s.loopStart > 0) ++s.loopStart;
    makeCustom (s, true);
}

// ===========================================================================
CurveEditor::CurveEditor (UiContext& c, juce::String paramId, juce::String t, juce::Colour colour)
    : ctx (c), id (std::move (paramId)), title (std::move (t)), colourUsed (colour)
{
    if (auto* p = ctx.param (id))
    {
        att = std::make_unique<juce::ParameterAttachment> (*p, [this] (float) { repaint(); });
        att->sendInitialUpdate();
    }
    setTooltip (title + " curve: drag up/down to bend, double-click for linear");
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void CurveEditor::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::panelLo);
    g.fillRoundedRectangle (r, 4.0f);
    auto plotR = r.reduced (10.0f, 0.0f).withTrimmedTop (24.0f).withTrimmedBottom (20.0f);
    g.setColour (col::text);
    g.setFont (font (11.5f, true));
    g.drawText (title.toUpperCase(), r.reduced (8.0f, 4.0f).removeFromTop (14.0f), juce::Justification::centredLeft);
    const float c = ctx.value (id);
    juce::Path p;
    for (int i = 0; i <= 64; ++i)
    {
        const float x = (float) i / 64.0f;
        const float y = cs::applyCurve (x, c);
        const juce::Point<float> pt (plotR.getX() + x * plotR.getWidth(), plotR.getBottom() - y * plotR.getHeight());
        if (i == 0)
            p.startNewSubPath (pt);
        else
            p.lineTo (pt);
    }
    g.setColour (col::border);
    g.drawRect (plotR, 1.0f);
    fillUnder (g, p, plotR, plotR.getBottom(), colourUsed);
    g.setColour (colourUsed);
    g.strokePath (p, juce::PathStrokeType (1.8f));
    g.setColour (col::textDim);
    g.setFont (font (10.5f));
    g.drawText (std::abs (c) < 0.01f ? juce::String ("linear") : juce::String (c > 0 ? "soft " : "hard ") + juce::String (juce::roundToInt (std::abs (c) * 100.0f)) + " %",
                r.reduced (8.0f, 4.0f).removeFromBottom (14.0f), juce::Justification::centredRight);
}

void CurveEditor::mouseDown (const juce::MouseEvent&)
{
    startValue = ctx.value (id);
    if (att != nullptr)
        att->beginGesture();
}

void CurveEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (att != nullptr)
        att->setValueAsPartOfGesture (juce::jlimit (-1.0f, 1.0f, startValue + (float) e.getDistanceFromDragStartY() / 120.0f));
}

void CurveEditor::mouseUp (const juce::MouseEvent&)
{
    if (att != nullptr)
        att->endGesture();
}

void CurveEditor::mouseDoubleClick (const juce::MouseEvent&)
{
    if (att != nullptr)
        att->setValueAsCompleteGesture (0.0f);
}

// ===========================================================================
void Scope::push (float v)
{
    buf[(size_t) pos] = std::isfinite (v) ? v : 0.0f;
    pos = (pos + 1) % (int) buf.size();
    repaint();
}

void Scope::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f, 3.0f);
    juce::Path p;
    const int n = (int) buf.size();
    for (int i = 0; i < n; ++i)
    {
        const float v = buf[(size_t) ((pos + i) % n)];
        const float y = bipolar ? r.getCentreY() - juce::jlimit (-1.0f, 1.0f, v) * r.getHeight() * 0.5f
                                : r.getBottom() - juce::jlimit (0.0f, 1.0f, std::abs (v)) * r.getHeight();
        const float x = r.getX() + r.getWidth() * (float) i / (float) (n - 1);
        if (i == 0)
            p.startNewSubPath (x, y);
        else
            p.lineTo (x, y);
    }
    g.setColour (colourUsed);
    g.strokePath (p, juce::PathStrokeType (1.4f));
}

// ===========================================================================
NoteMask::NoteMask (UiContext& c, juce::String paramId) : ctx (c), id (std::move (paramId))
{
    if (auto* p = ctx.param (id))
    {
        att = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { mask = (int) std::lround (v); repaint(); });
        att->sendInitialUpdate();
    }
    setTooltip ("Mod Quantize: modulated Coarse snaps to the lit intervals");
}

juce::Rectangle<float> NoteMask::keyRect (int note) const
{
    static const int whiteIndex[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
    static const float blackPos[12] = { 0, 0.7f, 0, 1.7f, 0, 0, 3.7f, 0, 4.7f, 0, 5.7f, 0 };
    const float w = (float) getWidth() / 7.0f, h = (float) getHeight();
    if (whiteIndex[note] >= 0)
        return { (float) whiteIndex[note] * w, 0.0f, w, h };
    return { blackPos[note] * w, 0.0f, w * 0.6f, h * 0.6f };
}

void NoteMask::paint (juce::Graphics& g)
{
    for (int pass = 0; pass < 2; ++pass)
        for (int n = 0; n < 12; ++n)
        {
            const bool black = n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
            if (black != (pass == 1))
                continue;
            const auto r = keyRect (n).reduced (0.5f);
            const bool on = (mask >> n) & 1;
            g.setColour (on ? col::accent.withAlpha (black ? 0.9f : 0.7f) : (black ? col::bg : col::panelHi));
            g.fillRect (r);
            g.setColour (col::border);
            g.drawRect (r, 1.0f);
        }
}

void NoteMask::mouseDown (const juce::MouseEvent& e)
{
    int hit = -1;
    for (int n : { 1, 3, 6, 8, 10 })
        if (keyRect (n).contains (e.position))
            hit = n;
    if (hit < 0)
        for (int n : { 0, 2, 4, 5, 7, 9, 11 })
            if (keyRect (n).contains (e.position))
                hit = n;
    if (hit < 0 || att == nullptr)
        return;
    int m = mask ^ (1 << hit);
    if (m == 0)
        m = 1;
    att->setValueAsCompleteGesture ((float) m);
}
} // namespace ui
