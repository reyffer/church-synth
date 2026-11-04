#include "SeqArp.h"

#include <algorithm>
#include <cmath>

namespace cs
{
const SeqScale kSeqScales[kNumSeqScales] = {
    { "Chromatic",        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
    { "Major",            { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 } },
    { "Natural Minor",    { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0 } },
    { "Harmonic Minor",   { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1 } },
    { "Melodic Minor",    { 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1 } },
    { "Dorian",           { 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0 } },
    { "Phrygian",         { 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0 } },
    { "Lydian",           { 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1 } },
    { "Mixolydian",       { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0 } },
    { "Locrian",          { 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0 } },
    { "Major Pentatonic", { 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0 } },
    { "Minor Pentatonic", { 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0 } },
    { "Blues",            { 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0 } },
    { "Fifth",            { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    // Generative: every note of the scale can play, chord tones more often.
    { "Major (Chord Tones)", { 4, 0, 1, 0, 3, 1, 0, 3, 0, 1, 0, 1 } },
    { "Minor (Chord Tones)", { 4, 0, 1, 3, 0, 1, 0, 3, 1, 0, 1, 0 } },
    { "Root & Fifth Heavy",  { 6, 0, 1, 0, 1, 1, 0, 4, 0, 1, 0, 0 } },
    { "Custom",           { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 } }, // weights come from the pattern
};

namespace
{
uint32_t mix32 (uint32_t x) noexcept
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

// Deterministic 0..1 value for (seed, column, track, which).
float hashUni (uint32_t seed, int column, int track, int which) noexcept
{
    const uint32_t h = mix32 (seed * 0x9E3779B1u ^ mix32 ((uint32_t) column * 0x85EBCA77u + (uint32_t) track * 0xC2B2AE3Du
                                                          + (uint32_t) which * 0x27D4EB2Fu + 0x165667B1u));
    return (float) (h >> 8) * (1.0f / 16777216.0f);
}

int64_t floorDiv (int64_t a, int64_t b) noexcept
{
    int64_t q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0)))
        --q;
    return q;
}
int64_t posMod (int64_t a, int64_t b) noexcept
{
    const int64_t m = a % b;
    return m < 0 ? m + b : m;
}

const std::array<float, 12>& scaleWeights (const SeqPattern& pat, int scale) noexcept
{
    if (scale == kScaleCustom)
        return pat.customWeights;
    return kSeqScales[std::clamp (scale, 0, kNumSeqScales - 1)].weights;
}

// Random pitch 0..12 picked by the scale weights (the octave counts as the root).
int weightedPitch (const std::array<float, 12>& w, float u) noexcept
{
    float total = 0.0f;
    for (int i = 0; i <= 12; ++i)
        total += std::max (0.0f, w[(size_t) (i % 12)]);
    if (total <= 0.0f)
        return 0;
    float acc = 0.0f;
    const float target = u * total;
    for (int i = 0; i <= 12; ++i)
    {
        acc += std::max (0.0f, w[(size_t) (i % 12)]);
        if (target < acc)
            return i;
    }
    return 12;
}
} // namespace

uint32_t seqNextSeed (uint32_t seed) noexcept
{
    const uint32_t s = mix32 (seed + 0x6D2B79F5u);
    return s == 0 ? 1u : s;
}

float seqSlideSeconds (float slide) noexcept
{
    return 0.02f * std::pow (25.0f, std::clamp (slide, 0.0f, 1.0f)); // 20 ms .. 500 ms
}

int seqSnapToScale (int semis, const std::array<float, 12>& weights) noexcept
{
    // Nearest note with a non-zero weight (ties go down).
    if (weights[(size_t) posMod (semis, 12)] > 0.0f)
        return semis;
    for (int d = 1; d < 12; ++d)
    {
        if (weights[(size_t) posMod (semis - d, 12)] > 0.0f)
            return semis - d;
        if (weights[(size_t) posMod (semis + d, 12)] > 0.0f)
            return semis + d;
    }
    return semis;
}

SeqResolved seqResolve (const SeqPattern& pat, const SeqParams& p, const int* columns, uint32_t liveSeed) noexcept
{
    auto seedAt = [&] (int col) { const auto ls = pat.steps[(size_t) col].lockSeed; return ls != 0 ? ls : liveSeed; };
    auto u = [&] (int track, int which) {
        const int col = columns[track];
        return hashUni (seedAt (col), col, track, which);
    };
    const auto& tr = pat.tracks;
    SeqResolved r {};

    // Steps: on/off (random flips some) + probability.
    {
        const auto& st = pat.steps[(size_t) columns[TrackSteps]];
        r.on = true;
        r.prob = 1.0f;
        if (tr[TrackSteps].enabled)
        {
            const float amt = tr[TrackSteps].random;
            r.on = st.on != (u (TrackSteps, 0) < amt * 0.5f);
            r.prob = std::clamp (st.prob, 0.0f, 1.0f);
        }
    }
    // Pitch (Seq mode only; the arp ignores it).
    {
        const auto& st = pat.steps[(size_t) columns[TrackPitch]];
        const auto& w = scaleWeights (pat, p.scale);
        int pitch = 0;
        if (tr[TrackPitch].enabled)
        {
            pitch = std::clamp (st.pitch, 0, 12);
            if (u (TrackPitch, 0) < tr[TrackPitch].random)
                pitch = weightedPitch (w, u (TrackPitch, 1));
            if (p.snap)
                pitch = seqSnapToScale (pitch, w);
        }
        r.pitch = pitch;
    }
    {
        const auto& st = pat.steps[(size_t) columns[TrackVelocity]];
        r.velocity = -1.0f; // -1 = use the played velocity
        if (tr[TrackVelocity].enabled)
            r.velocity = std::clamp (st.velocity + (2.0f * u (TrackVelocity, 0) - 1.0f) * tr[TrackVelocity].random, 0.0f, 1.0f);
    }
    {
        const auto& st = pat.steps[(size_t) columns[TrackOctave]];
        r.octave = 0;
        if (tr[TrackOctave].enabled)
            r.octave = std::clamp (st.octave + (int) std::lround ((2.0f * u (TrackOctave, 0) - 1.0f) * 2.0f * tr[TrackOctave].random), -3, 3);
    }
    {
        const auto& st = pat.steps[(size_t) columns[TrackGate]];
        r.gate = 0.5f;
        if (tr[TrackGate].enabled)
            r.gate = std::clamp (st.gate * std::exp2 ((2.0f * u (TrackGate, 0) - 1.0f) * 2.0f * tr[TrackGate].random), 0.05f, 4.0f);
    }
    {
        const auto& st = pat.steps[(size_t) columns[TrackSlide]];
        r.slide = 0.0f;
        if (tr[TrackSlide].enabled)
        {
            r.slide = std::clamp (st.slide, 0.0f, 1.0f);
            if (u (TrackSlide, 0) < tr[TrackSlide].random * 0.5f)
                r.slide = r.slide > 0.0f ? 0.0f : 0.3f;
        }
    }
    return r;
}

void seqRotate (SeqPattern& pat, int track, int direction) noexcept
{
    auto rotateTrack = [&] (int t) {
        const int len = std::clamp (pat.tracks[(size_t) t].length, 1, kSeqSteps);
        std::array<SeqStep, kSeqSteps> old = pat.steps;
        for (int i = 0; i < len; ++i)
        {
            const int from = (int) posMod (i - direction, len);
            auto& dst = pat.steps[(size_t) i];
            const auto& src = old[(size_t) from];
            switch (t)
            {
                case TrackSteps: dst.on = src.on; dst.prob = src.prob; break;
                case TrackPitch: dst.pitch = src.pitch; break;
                case TrackVelocity: dst.velocity = src.velocity; break;
                case TrackOctave: dst.octave = src.octave; break;
                case TrackGate: dst.gate = src.gate; break;
                case TrackSlide: dst.slide = src.slide; break;
                default: break;
            }
        }
    };
    if (track >= 0 && track < NumSeqTracks)
        rotateTrack (track);
    else
        for (int t = 0; t < NumSeqTracks; ++t)
            rotateTrack (t);
}

void seqApplyRandom (SeqPattern& pat, const SeqParams& p, uint32_t liveSeed) noexcept
{
    SeqPattern out = pat;
    for (int c = 0; c < kSeqSteps; ++c)
    {
        int cols[NumSeqTracks];
        for (auto& x : cols)
            x = c;
        const auto r = seqResolve (pat, p, cols, liveSeed);
        auto& st = out.steps[(size_t) c];
        if (pat.tracks[TrackSteps].enabled) st.on = r.on;
        if (pat.tracks[TrackPitch].enabled) st.pitch = r.pitch;
        if (pat.tracks[TrackVelocity].enabled) st.velocity = r.velocity;
        if (pat.tracks[TrackOctave].enabled) st.octave = std::clamp (r.octave, -2, 2);
        if (pat.tracks[TrackGate].enabled) st.gate = r.gate;
        if (pat.tracks[TrackSlide].enabled) st.slide = r.slide;
        st.lockSeed = 0;
    }
    for (auto& t : out.tracks)
        t.random = 0.0f;
    pat = out;
}

void seqClearRandom (SeqPattern& pat) noexcept
{
    for (auto& t : pat.tracks)
        t.random = 0.0f;
    for (auto& s : pat.steps)
        s.lockSeed = 0;
}

// ---------------------------------------------------------------------------
float SeqArp::rnd() noexcept
{
    rngState = rngState * 1664525u + 1013904223u;
    return (float) (rngState >> 8) * (1.0f / 16777216.0f);
}

void SeqArp::reset() noexcept
{
    keyCount = 0;
    playingCount = 0;
    lastNoteCount = 0;
    running = false;
    restartPending = false;
    wasHostPlaying = false;
    phase = 0.0;
    lastStepBeats = 0.0;
    nextStep = 0;
    arpIndex = 0;
    uiStep.store (-1, std::memory_order_relaxed);
}

void SeqArp::setPattern (const SeqPattern& p) noexcept
{
    pattern = p;
    if (p.seed != lastPatternSeed) // a new seed from the editor (Regen) or a loaded pattern
    {
        seed = p.seed == 0 ? 1u : p.seed;
        lastPatternSeed = p.seed;
        uiSeed.store (seed, std::memory_order_relaxed);
    }
}

void SeqArp::setChord (const int* intervals, int count) noexcept
{
    chordCount = std::clamp (count, 0, kMaxUnison);
    for (int i = 0; i < chordCount; ++i)
        chord[i] = intervals[i];
}

// --- keys --------------------------------------------------------------------
void SeqArp::removeKeyAt (int i) noexcept
{
    for (int j = i; j < keyCount - 1; ++j)
        keys[(size_t) j] = keys[(size_t) j + 1];
    --keyCount;
}

void SeqArp::purgeReleasedKeys (bool hold, bool pedal) noexcept
{
    if (hold || pedal)
        return;
    for (int i = keyCount - 1; i >= 0; --i)
        if (! keys[(size_t) i].physical)
            removeKeyAt (i);
}

void SeqArp::keyDown (int note, float velocity, bool hold, bool pedal) noexcept
{
    bool anyPhysical = false;
    for (int i = 0; i < keyCount; ++i)
        anyPhysical = anyPhysical || keys[(size_t) i].physical;
    // Hold (latch): a new chord after letting go replaces the latched one.
    if (hold && ! pedal && ! anyPhysical)
        keyCount = 0;
    for (int i = 0; i < keyCount; ++i)
        if (keys[(size_t) i].note == note)
        {
            removeKeyAt (i);
            break;
        }
    if (keyCount == 0)
        restartPending = ! running; // first key: start the pattern (unless it is still running)
    if (keyCount < (int) keys.size())
        keys[(size_t) keyCount++] = { note, velocity, true };
}

void SeqArp::keyUp (int note, bool hold, bool pedal) noexcept
{
    for (int i = 0; i < keyCount; ++i)
        if (keys[(size_t) i].note == note)
        {
            keys[(size_t) i].physical = false;
            if (! hold && ! pedal)
                removeKeyAt (i);
            break;
        }
}

void SeqArp::pedalChanged (bool hold, bool pedal) noexcept
{
    purgeReleasedKeys (hold, pedal);
}

// --- sounding notes -----------------------------------------------------------
int SeqArp::findPlaying (int note) const noexcept
{
    for (int i = 0; i < playingCount; ++i)
        if (playing[(size_t) i].note == note)
            return i;
    return -1;
}

void SeqArp::removePlayingAt (int i) noexcept
{
    playing[(size_t) i] = playing[(size_t) (playingCount - 1)];
    --playingCount;
}

void SeqArp::addPlaying (int note, double offTime, Sink& sink) noexcept
{
    if (playingCount >= (int) playing.size())
    {
        // Full: end the note that would finish first.
        int first = 0;
        for (int i = 1; i < playingCount; ++i)
            if (playing[(size_t) i].offTime < playing[(size_t) first].offTime)
                first = i;
        sink.seqNoteOff (playing[(size_t) first].note);
        removePlayingAt (first);
    }
    playing[(size_t) playingCount++] = { note, offTime };
}

void SeqArp::stop (Sink& sink) noexcept
{
    for (int i = 0; i < playingCount; ++i)
        sink.seqNoteOff (playing[(size_t) i].note);
    playingCount = 0;
    lastNoteCount = 0;
    running = false;
    restartPending = keyCount > 0;
    uiStep.store (-1, std::memory_order_relaxed);
}

// --- timing ----------------------------------------------------------------------
double SeqArp::stepTime (int64_t k, float swing) noexcept
{
    const double late = (posMod (k, 2) == 1) ? (double) (std::clamp (swing, 0.5f, 0.75f) - 0.5f) * 2.0 : 0.0;
    return (double) k + late;
}

void SeqArp::columnsFor (int64_t k, const SeqParams& p, int* columns) const noexcept
{
    const int64_t kk = p.resetSteps > 0 ? posMod (k, p.resetSteps) : k;
    for (int t = 0; t < NumSeqTracks; ++t)
    {
        const auto& tr = pattern.tracks[(size_t) t];
        const int len = std::clamp (p.polymetry ? tr.length : p.length, 1, kSeqSteps);
        const int64_t c = floorDiv (kk, std::max (1, tr.divider));
        int64_t pos = 0;
        switch (p.direction)
        {
            case SeqBackward: pos = len - 1 - posMod (c, len); break;
            case SeqPingPong:
                if (len > 1)
                {
                    const int64_t i = posMod (c, 2 * len - 2);
                    pos = i < len ? i : 2 * len - 2 - i;
                }
                break;
            case SeqRandomDir:
                pos = (int64_t) (hashUni (dirSalt, (int) posMod (c, 1 << 20), t, 7) * (float) len);
                break;
            case SeqForward:
            default: pos = posMod (c, len); break;
        }
        columns[t] = (int) std::clamp<int64_t> (pos, 0, len - 1);
    }
}

int SeqArp::buildArpList (const SeqParams& p, int* out, float* vel) noexcept
{
    constexpr int kMax = 128;
    // Expand keys by the chord (chord arp) and octaves, keeping play order.
    int n = 0;
    const int octaves = std::clamp (p.arpOctaves, 1, 4);
    const int nChord = chordCount > 0 ? chordCount : 1;
    for (int o = 0; o < octaves; ++o)
        for (int k = 0; k < keyCount; ++k)
            for (int c = 0; c < nChord && n < kMax; ++c)
            {
                out[n] = keys[(size_t) k].note + 12 * o + (chordCount > 0 ? chord[c] : 0);
                vel[n] = keys[(size_t) k].vel;
                ++n;
            }
    if (p.arpOrder == ArpAsPlayed || p.arpOrder == ArpRandom || n <= 1)
        return n;

    // Sorted orders: sort by pitch, drop duplicates.
    for (int i = 1; i < n; ++i)
        for (int j = i; j > 0 && out[j - 1] > out[j]; --j)
        {
            std::swap (out[j - 1], out[j]);
            std::swap (vel[j - 1], vel[j]);
        }
    int m = 0;
    for (int i = 0; i < n; ++i)
        if (m == 0 || out[i] != out[m - 1])
        {
            out[m] = out[i];
            vel[m] = vel[i];
            ++m;
        }
    n = m;
    if (p.arpOrder == ArpDown)
    {
        std::reverse (out, out + n);
        std::reverse (vel, vel + n);
    }
    else if (p.arpOrder == ArpUpDownIncl || p.arpOrder == ArpUpDownExcl)
    {
        const bool incl = p.arpOrder == ArpUpDownIncl;
        const int from = incl ? n - 1 : n - 2, to = incl ? 0 : 1;
        for (int i = from; i >= to && n < kMax * 2 - 1; --i)
        {
            out[n] = out[i];
            vel[n] = vel[i];
            ++n;
        }
    }
    return n;
}

void SeqArp::fireStep (int64_t k, const SeqParams& p, double stepBeats, Sink& sink) noexcept
{
    // Auto-regen: a new seed at each period boundary.
    if (p.autoRegen > 0 && p.autoRegen < kNumAutoRegen)
    {
        const double beats = (double) k * stepBeats;
        const int64_t idx = (int64_t) std::floor (beats / kAutoRegenBeats[p.autoRegen] + 1.0e-9);
        if (lastRegenIndex >= 0 && idx != lastRegenIndex)
        {
            seed = seqNextSeed (seed);
            uiSeed.store (seed, std::memory_order_relaxed);
        }
        lastRegenIndex = idx;
    }

    int cols[NumSeqTracks];
    columnsFor (k, p, cols);
    uiStep.store (cols[TrackSteps], std::memory_order_relaxed);
    const auto r = seqResolve (pattern, p, cols, seed);
    if (! r.on || keyCount == 0)
        return;
    if (r.prob < 1.0f && rnd() >= r.prob)
        return;

    // If the next step slides, keep this one sounding until it starts.
    int nextCols[NumSeqTracks];
    columnsFor (k + 1, p, nextCols);
    const auto rn = seqResolve (pattern, p, nextCols, seed);
    const bool nextSlides = rn.on && rn.slide > 0.0f;

    const double t0 = stepTime (k, p.swing);
    double offTime = t0 + (double) r.gate;
    if (nextSlides)
        offTime = std::max (offTime, stepTime (k + 1, p.swing) + 0.02);

    const int transpose = (int) std::lround (p.transpose);
    int notes[32];
    float vels[32];
    int count = 0;
    if (p.mode == SeqModeSeq)
    {
        for (int i = 0; i < keyCount && count < 32; ++i)
        {
            notes[count] = keys[(size_t) i].note + r.pitch + 12 * r.octave + transpose;
            vels[count] = keys[(size_t) i].vel;
            ++count;
        }
    }
    else
    {
        int list[256];
        float lv[256];
        const int n = buildArpList (p, list, lv);
        if (n <= 0)
            return;
        const int i = p.arpOrder == ArpRandom ? (int) (rnd() * (float) n) % n : arpIndex % n;
        ++arpIndex;
        notes[0] = list[i] + 12 * r.octave + transpose;
        vels[0] = lv[i];
        count = 1;
    }

    const bool slide = r.slide > 0.0f;
    const float slideSec = seqSlideSeconds (r.slide);
    int newNotes[32];
    int newCount = 0;
    for (int i = 0; i < count; ++i)
    {
        const int note = std::clamp (notes[i], 0, 127);
        bool dup = false;
        for (int j = 0; j < newCount; ++j)
            dup = dup || newNotes[j] == note;
        if (dup)
            continue;
        float vel = vels[i];
        if (r.velocity >= 0.0f)
            vel = vel + std::clamp (p.velBlend, 0.0f, 1.0f) * (r.velocity - vel);
        vel = std::clamp (vel, 0.01f, 1.0f);

        int from = -1;
        if (slide && lastNoteCount > 0)
        {
            const int cand = lastNotes[(size_t) (newCount % lastNoteCount)];
            const int pi = findPlaying (cand);
            if (pi >= 0)
            {
                from = cand;
                removePlayingAt (pi); // the voice carries on with the new note
            }
        }
        if (from < 0)
        {
            const int pi = findPlaying (note);
            if (pi >= 0) // same note still sounding: end it first
            {
                sink.seqNoteOff (note);
                removePlayingAt (pi);
            }
        }
        else if (from != note)
        {
            const int pi = findPlaying (note);
            if (pi >= 0)
            {
                sink.seqNoteOff (note);
                removePlayingAt (pi);
            }
        }
        sink.seqNoteOn (note, vel, from, slideSec);
        addPlaying (note, offTime, sink);
        newNotes[newCount++] = note;
    }
    lastNoteCount = newCount;
    for (int i = 0; i < newCount; ++i)
        lastNotes[(size_t) i] = newNotes[i];
}

int SeqArp::run (int maxSamples, const SeqParams& p, const Transport& t, Sink& sink) noexcept
{
    maxSamples = std::max (1, maxSamples);
    const double stepBeats = kSyncDivs[std::clamp (p.rateDiv, 0, kNumSyncDivs - 1)].beats;
    const double bpm = std::clamp (t.hostTempo ? t.bpm : (double) p.bpm, 20.0, 400.0);
    const double stepsPerSample = bpm / 60.0 / (double) sampleRate / stepBeats;

    // Rate changed: keep our place (the same beat position) in the new step units.
    if (lastStepBeats > 0.0 && std::abs (stepBeats - lastStepBeats) > 1.0e-9)
    {
        const double scale = lastStepBeats / stepBeats;
        phase *= scale;
        for (int i = 0; i < playingCount; ++i)
            playing[(size_t) i].offTime *= scale;
        nextStep = (int64_t) std::ceil (phase - 1.0e-9);
        lastRegenIndex = -1;
    }
    lastStepBeats = stepBeats;

    // Follow the host's song position while it plays.
    if (t.playing)
    {
        const double target = t.ppq / stepBeats;
        if (! wasHostPlaying || std::abs (target - phase) > 0.05)
        {
            phase = target;
            if (running)
                nextStep = (int64_t) std::ceil (phase - 1.0e-9);
        }
        else
            phase = target;
    }
    wasHostPlaying = t.playing;

    if (keyCount == 0 && running)
    {
        running = false;
        uiStep.store (-1, std::memory_order_relaxed);
    }

    // Start: first key down.
    if (restartPending && keyCount > 0)
    {
        restartPending = false;
        running = true;
        arpIndex = 0;
        lastNoteCount = 0;
        lastRegenIndex = -1;
        dirSalt = mix32 (dirSalt + 0x9E3779B9u);
        if (t.playing)
        {
            // On the grid. A key pressed a little late still plays the step it missed.
            const int64_t kc = (int64_t) std::floor (phase);
            const double tk = stepTime (kc, p.swing);
            nextStep = (tk > phase || phase - tk <= 0.25) ? kc : kc + 1;
        }
        else
        {
            phase = 0.0;
            nextStep = 0;
        }
    }

    constexpr double eps = 1.0e-9;
    // Note-offs that are due.
    for (int i = playingCount - 1; i >= 0; --i)
        if (playing[(size_t) i].offTime <= phase + eps)
        {
            sink.seqNoteOff (playing[(size_t) i].note);
            removePlayingAt (i);
        }

    // Steps that are due (if we fell behind, only the latest one plays).
    if (running && stepTime (nextStep, p.swing) <= phase + eps)
    {
        while (stepTime (nextStep + 1, p.swing) <= phase + eps)
            ++nextStep;
        fireStep (nextStep, p, stepBeats, sink);
        ++nextStep;
    }

    // Time to the next event.
    double next = 1.0e300;
    if (running)
        next = stepTime (nextStep, p.swing);
    for (int i = 0; i < playingCount; ++i)
        next = std::min (next, playing[(size_t) i].offTime);

    int n = maxSamples;
    if (next < 1.0e299)
    {
        const double samples = std::ceil ((next - phase) / stepsPerSample - 1.0e-6);
        n = (int) std::clamp (samples, 1.0, (double) maxSamples);
    }
    phase += (double) n * stepsPerSample;
    return n;
}
} // namespace cs
