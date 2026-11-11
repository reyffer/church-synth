#pragma once

// Modulation source generators: LFO, Function, Random and Combinator.
// All run at control rate (once per kBlock samples).

#include "MathUtil.h"
#include "Params.h"

namespace cs
{
// ---------------------------------------------------------------------------
// Tempo sync
// Divisions in quarter-note beats. Order is stored in presets: append only.
struct SyncDiv
{
    const char* name;
    double beats;
};
inline constexpr SyncDiv kSyncDivs[kNumSyncDivs] = {
    { "8/1", 32.0 },   { "4/1", 16.0 },   { "2/1", 8.0 },     { "1/1", 4.0 },
    { "1/2", 2.0 },    { "1/2 D", 3.0 },  { "1/2 T", 4.0 / 3.0 },
    { "1/4", 1.0 },    { "1/4 D", 1.5 },  { "1/4 T", 2.0 / 3.0 },
    { "1/8", 0.5 },    { "1/8 D", 0.75 }, { "1/8 T", 1.0 / 3.0 },
    { "1/16", 0.25 },  { "1/16 D", 0.375 }, { "1/16 T", 1.0 / 6.0 },
    { "1/32", 0.125 }, { "1/32 T", 1.0 / 12.0 }, { "1/64", 0.0625 },
};

struct Transport
{
    double bpm = 120.0;
    double ppq = 0.0;     // quarter notes at the start of the current block
    bool playing = false; // host transport running (sync to song position)
    bool hostTempo = true; // false: no tempo from the host (standalone) -> use the seq's own BPM
};

inline float rateToHz (const RateParams& r, const Transport& t) noexcept
{
    if (! r.sync)
        return std::max (r.hz, 0.0f);
    const auto& d = kSyncDivs[std::clamp (r.div, 0, kNumSyncDivs - 1)];
    return (float) (t.bpm / 60.0 / d.beats);
}

// Response curve for velocity / aftertouch / key: c = 0 linear, > 0 softer start.
inline float applyCurve (float x, float c) noexcept
{
    if (std::abs (c) < 1.0e-3f)
        return x;
    const float k = std::exp2 (c * 1.6f);
    return x >= 0.0f ? std::pow (x, k) : -std::pow (-x, k);
}

// Exponential segment shape shared by functions and remap curves: c in -1..1.
inline float segmentShape (float u, float c) noexcept
{
    const float k = c * 6.0f;
    if (std::abs (k) < 1.0e-3f)
        return u;
    return std::expm1 (k * u) / std::expm1 (k);
}

// Values shared by all voices for one control block.
struct GlobalMod
{
    float modWheel = 0.0f, aftertouch = 0.0f, expression = 0.0f;
    float lfo[kNumLfos] {}, func[kNumFuncs] {}, rnd[kNumRandoms] {};
    bool lfoWrap[kNumLfos] {};
    bool lfoIsGlobal[kNumLfos] {}, funcIsGlobal[kNumFuncs] {}, randIsGlobal[kNumRandoms] {};
    bool used[NumModSources] {}; // sources nothing listens to are not computed
    Transport transport {};
};

inline bool retrigIsGlobal (int r) noexcept
{
    return r == RetrigMonoKbd || r == RetrigLegatoKbd || r == RetrigFreeRun;
}

// ---------------------------------------------------------------------------
class Lfo
{
public:
    void reset (const LfoParams& p, Rng& rng) noexcept
    {
        phase = wrap01 (p.phase);
        fadeLevel = p.fade > 0.0f ? 0.0f : 1.0f;
        held = rng.bi();
        prevHeld = held;
        wrapped = true;
    }

    // Advances by n samples. phaseOverride >= 0 forces the phase (song-position sync).
    void tick (const LfoParams& p, float hz, int n, float sampleRate, Rng& rng, float phaseOverride = -1.0f) noexcept
    {
        const float dt = (float) n / sampleRate;
        wrapped = false;
        if (phaseOverride >= 0.0f)
        {
            if (phaseOverride < phase)
                wrapped = true;
            phase = phaseOverride;
        }
        else
        {
            phase += hz * dt;
            if (phase >= 1.0f)
            {
                phase -= std::floor (phase);
                wrapped = true;
            }
        }
        if (wrapped)
        {
            prevHeld = held;
            held = rng.bi();
        }

        float v = shapeAt (p, phase);
        if (p.fade > 0.0f)
        {
            fadeLevel = std::min (1.0f, fadeLevel + dt / p.fade);
            v *= fadeLevel;
        }
        if (! p.bipolar)
            v = 0.5f * (v + 1.0f);
        if (p.smooth > 0.0f)
            smoothed += (v - smoothed) * (1.0f - std::exp (-dt / p.smooth));
        else
            smoothed = v;
        value = smoothed;
    }

    float value = 0.0f;
    bool wrapped = false;
    float getPhase() const noexcept { return phase; } // 0..1 (editor)

private:
    float phase = 0.0f, fadeLevel = 1.0f, held = 0.0f, prevHeld = 0.0f, smoothed = 0.0f;

    float shapeAt (const LfoParams& p, float ph) const noexcept
    {
        const float s = std::clamp (p.symmetry, 0.02f, 0.98f);
        const float q = ph < s ? 0.5f * ph / s : 0.5f + 0.5f * (ph - s) / (1.0f - s);
        switch (p.wave)
        {
            case LfoSine: return SineTable::get() (q);
            case LfoTriangle: return q < 0.25f ? 4.0f * q : q < 0.75f ? 2.0f - 4.0f * q : 4.0f * q - 4.0f;
            case LfoSquare: return q < 0.5f ? 1.0f : -1.0f;
            case LfoSaw: return 1.0f - 2.0f * q;
            case LfoRamp: return 2.0f * q - 1.0f;
            case LfoSampleHold:
            default: return held;
        }
    }
};

// ---------------------------------------------------------------------------
struct FunctionShape
{
    static constexpr int kMaxPoints = 64;
    int count = 2;
    float x[kMaxPoints] { 0.0f, 1.0f };
    float y[kMaxPoints] { 0.0f, 1.0f };
    float curve[kMaxPoints] {}; // curve of the segment starting at each point
    int sustain = -1;           // point index (envelope modes)
    int loopStart = 0;          // point index (envelope-loop mode)

    float eval (float t) const noexcept
    {
        if (count < 2)
            return count == 1 ? y[0] : 0.0f;
        t = std::clamp (t, 0.0f, 1.0f);
        int i = 0;
        while (i < count - 2 && t > x[i + 1])
            ++i;
        const float w = x[i + 1] - x[i];
        const float u = w > 1.0e-6f ? (t - x[i]) / w : 1.0f;
        return y[i] + (y[i + 1] - y[i]) * segmentShape (std::clamp (u, 0.0f, 1.0f), curve[i]);
    }
};

// Built-in shapes for Functions and Combinator Remap. Order is stored in presets: append only.
constexpr int kNumFunctionPresets = 16;
const char* functionPresetName (int index) noexcept;
const FunctionShape& functionPreset (int index) noexcept;
constexpr int kNumRemapShapes = 10;
const char* remapShapeName (int index) noexcept;
const FunctionShape& remapShape (int index) noexcept;

class FunctionGen
{
public:
    void reset() noexcept
    {
        pos = 0.0f;
        released = false;
        wrapped = true;
    }

    void release() noexcept { released = true; }

    void tick (const FunctionParams& p, float hz, bool gate, int n, float sampleRate) noexcept
    {
        const FunctionShape& s = p.shape != nullptr ? *p.shape : functionPreset (0);
        const float dt = (float) n / sampleRate;
        const float step = hz * dt;
        wrapped = false;
        if (! gate)
            released = true;

        switch (p.mode)
        {
            case FuncLoop:
                pos += step;
                if (pos >= 1.0f)
                {
                    pos -= std::floor (pos);
                    wrapped = true;
                }
                break;
            case FuncOneShot:
                pos = std::min (1.0f, pos + step);
                break;
            case FuncEnvelope:
            case FuncEnvLoop:
            {
                const float susX = s.sustain >= 0 && s.sustain < s.count ? s.x[s.sustain] : 1.0f;
                if (! released && pos <= susX)
                {
                    pos += step;
                    if (pos >= susX)
                    {
                        if (p.mode == FuncEnvLoop && s.loopStart >= 0 && s.loopStart < s.count && s.x[s.loopStart] < susX)
                        {
                            pos = s.x[s.loopStart] + (pos - susX);
                            wrapped = true;
                        }
                        else
                            pos = susX; // hold at the sustain point
                    }
                }
                else
                    pos = std::min (1.0f, pos + step);
                break;
            }
            default:
                break;
        }

        float v = s.eval (pos);
        if (p.bipolar)
            v = 2.0f * v - 1.0f;
        v *= p.scale;
        if (p.smooth > 0.0f)
            smoothed += (v - smoothed) * (1.0f - std::exp (-dt / p.smooth));
        else
            smoothed = v;
        value = smoothed;
    }

    float value = 0.0f;
    bool wrapped = false;
    float getPosition() const noexcept { return pos; } // 0..1 along the shape (editor)

private:
    float pos = 0.0f, smoothed = 0.0f;
    bool released = false;
};

// ---------------------------------------------------------------------------
class RandomGen
{
public:
    void reset (Rng& rng) noexcept
    {
        stepPos = 1.0f; // generate a value on the next tick
        current = target = previous = 0.0f;
        for (auto& b : turing)
            b = rng.uni();
        turingIndex = 0;
        bit = 0.0f;
    }

    // srcValues: current modulation source values (for Sample & Hold sampling).
    // voiceStep: the step chosen for this voice (Voice Modulator).
    void tick (const RandomParams& p, float hz, int n, float sampleRate, Rng& rng,
               const float* srcValues, int voiceStep) noexcept
    {
        const float dt = (float) n / sampleRate;
        wrapped = false;

        if (p.type == RandVoiceMod)
        {
            const int steps = std::clamp (p.vmVoices, 1, 8);
            float v = p.vmSteps[(size_t) (std::max (voiceStep, 0) % steps)];
            value = p.bipolar ? v : 0.5f * (v + 1.0f);
            return;
        }

        stepPos += hz * dt * jitterScale;
        if (stepPos >= 1.0f)
        {
            stepPos -= std::floor (stepPos);
            wrapped = true;
            jitterScale = 1.0f + p.jitter * (rng.uni() - 0.5f);
            previous = current;
            target = nextValue (p, rng, srcValues);
        }

        if (p.type == RandSampleHold)
        {
            const float rise = p.shRise, fall = p.shLink ? p.shRise : p.shFall;
            const float t = target > current ? rise : fall;
            current = t > 0.0f ? current + (target - current) * (1.0f - std::exp (-dt / t)) : target;
        }
        else if (p.smooth > 0.0f && p.type == RandRandom)
        {
            const float u = std::min (1.0f, stepPos / std::max (p.smooth, 1.0e-3f));
            current = previous + (target - previous) * (0.5f - 0.5f * SineTable::get() (0.25f + 0.5f * u));
        }
        else
            current = target;

        value = p.bipolar ? current : 0.5f * (current + 1.0f);
    }

    float value = 0.0f;
    bool wrapped = false;

private:
    float stepPos = 1.0f, current = 0.0f, target = 0.0f, previous = 0.0f, jitterScale = 1.0f;
    float turing[32] {};
    int turingIndex = 0;
    float bit = 0.0f;

    // Returns a bipolar value -1..1.
    float nextValue (const RandomParams& p, Rng& rng, const float* srcValues) noexcept
    {
        switch (p.type)
        {
            case RandSampleHold:
                if (p.shSource >= 0 && p.shSource < NumModSources && srcValues != nullptr)
                    return std::clamp (srcValues[p.shSource], -1.0f, 1.0f);
                return rng.bi();
            case RandTuring:
            {
                const int len = std::clamp (p.length, 1, 32);
                turingIndex = (turingIndex + 1) % len;
                float& b = turing[turingIndex];
                const float flip = std::clamp (p.flip, 0.0f, 1.0f);
                const float pNew = 1.0f - 2.0f * std::abs (flip - 0.5f);
                const float pInv = flip > 0.5f ? 2.0f * (flip - 0.5f) : 0.0f;
                const float r = rng.uni();
                if (r < pNew)
                    b = rng.uni();
                else if (r < pNew + pInv)
                    b = 1.0f - b;
                return 2.0f * b - 1.0f;
            }
            case RandBinary:
                if (rng.uni() >= std::clamp (p.correlation, 0.0f, 1.0f))
                    bit = rng.uni() < std::clamp (p.probability, 0.0f, 1.0f) ? 1.0f : 0.0f;
                return 2.0f * bit - 1.0f;
            case RandRandom:
            default:
                return rng.bi() * std::clamp (p.distance, 0.0f, 1.0f);
        }
    }
};

// ---------------------------------------------------------------------------
// Combinator maths. lagState / followLevel are per-voice state.
inline float combine (const CombParams& p, float s, float m, float& lagState, float followLevel, float dt) noexcept
{
    const float a = std::clamp (p.amount, 0.0f, 1.0f);
    float out;
    switch (p.type)
    {
        case CombSum: out = s + m * a; break;
        case CombDiff: out = s - m * a; break;
        case CombMultiply: out = s * m * a + s * (1.0f - a); break;
        case CombDivide:
        {
            float d = a + m;
            if (std::abs (d) < 0.05f)
                d = d < 0.0f ? -0.05f : 0.05f;
            out = s / d;
            break;
        }
        case CombCrossfade: out = s + (m - s) * a; break;
        case CombLag:
        {
            // ~5 s to reach 99 % at amount 1, instant at 0.
            const float tau = a * 5.0f / 4.6f;
            lagState = tau > 1.0e-4f ? lagState + (s - lagState) * (1.0f - std::exp (-dt / tau)) : s;
            out = lagState;
            break;
        }
        case CombThreshold:
        {
            const float t = -1.0f + 2.0f * a;
            out = s > t ? s : t;
            break;
        }
        case CombOffset: out = s + 2.0f * a; break;
        case CombRemap:
        {
            const float u = 0.5f * (std::clamp (s, -1.0f, 1.0f) + 1.0f);
            const float r = 2.0f * remapShape (p.remapShape).eval (u) - 1.0f;
            out = s + (r - s) * a;
            break;
        }
        case CombEnvFollow:
        default:
            out = followLevel;
            break;
    }
    return std::clamp (out, -1.0f, 1.0f);
}
} // namespace cs
