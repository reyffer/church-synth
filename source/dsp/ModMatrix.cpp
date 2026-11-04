#include "ModMatrix.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace cs
{
float ModDest::toNorm (float v) const noexcept
{
    if (logScale)
        return std::log (std::max (v, min) / min) / std::log (max / min);
    return (v - min) / (max - min);
}

float ModDest::fromNorm (float n) const noexcept
{
    n = std::clamp (n, 0.0f, 1.0f);
    if (logScale)
        return min * std::exp (n * std::log (max / min));
    return min + n * (max - min);
}

namespace
{
std::vector<ModDest> build()
{
    std::vector<ModDest> d;
    auto add = [&d] (const std::string& id, const std::string& name, float mn, float mx, bool lg,
                     std::function<float& (SynthParams&)> ref) {
        // IDs must outlive the table: keep them in static storage.
        static std::vector<std::unique_ptr<std::string>> idStore;
        idStore.push_back (std::make_unique<std::string> (id));
        d.push_back ({ idStore.back()->c_str(), name, mn, mx, lg, std::move (ref) });
    };

    for (size_t e = 0; e < 2; ++e)
    {
        const std::string p = "e" + std::to_string (e + 1) + "_", n = "Engine " + std::to_string (e + 1) + " ";
        add (p + "coarse", n + "Coarse", -60, 60, false, [e] (SynthParams& s) -> float& { return s.engine[e].coarse; });
        add (p + "fine", n + "Fine", -1, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].fine; });
        add (p + "vol", n + "Volume", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].volume; });
        add (p + "fmix", n + "Filter Mix", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].filterMix; });
        for (size_t o = 0; o < 3; ++o)
        {
            const std::string op = p + "o" + std::to_string (o + 1) + "_", on = n + "Osc " + std::to_string (o + 1) + " ";
            add (op + "width", on + "Width", 0, 1, false, [e, o] (SynthParams& s) -> float& { return s.engine[e].osc[o].width; });
            add (op + "vol", on + "Volume", 0, 1, false, [e, o] (SynthParams& s) -> float& { return s.engine[e].osc[o].volume; });
            add (op + "coarse", on + "Coarse", -36, 36, false, [e, o] (SynthParams& s) -> float& { return s.engine[e].osc[o].coarse; });
            if (o > 0)
                add (op + "fine", on + "Fine", -1, 1, false, [e, o] (SynthParams& s) -> float& { return s.engine[e].osc[o].fine; });
            if (o < 2)
                add (op + "fm", on + "FM", 0, 1, false, [e, o] (SynthParams& s) -> float& { return s.engine[e].osc[o].fm; });
        }
        add (p + "noise_color", n + "Noise Colour", -1, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].noiseColor; });
        add (p + "noise_vol", n + "Noise Volume", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].noiseVolume; });
        add (p + "mod_src", n + "Mod Osc3/Noise", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].modSource; });
        add (p + "uni_detune", n + "Unison Detune", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].detune; });
        add (p + "uni_stereo", n + "Unison Stereo", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].stereo; });
        add (p + "uni_mix", n + "Super Mix", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].superMix; });

        const std::string w = p + "wt_", wn = n + "WT ";
        add (w + "pos", wn + "Position", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].wt.position; });
        add (w + "vol", wn + "Wavetable Volume", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].wt.wtVolume; });
        add (w + "fm", wn + "FM / Ring", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].wt.fmAmount; });
        add (w + "pm", wn + "Phase Mod", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].wt.pmAmount; });
        add (w + "pd", wn + "Phase Transform", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].wt.pdAmount; });
        add (w + "pd_mod", wn + "Phase Transform Mod", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].wt.pdMod; });
        add (w + "fold", wn + "Fold", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].wt.foldAmount; });
        add (w + "fold_mod", wn + "Fold Mod", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].wt.foldMod; });
        add (w + "mod_coarse", wn + "Mod Osc Coarse", -48, 48, false, [e] (SynthParams& s) -> float& { return s.engine[e].wt.modCoarse; });
        add (w + "mod_ratio", wn + "Mod Osc Ratio", 0.125f, 16, true, [e] (SynthParams& s) -> float& { return s.engine[e].wt.modRatio; });
        add (w + "mod_hz", wn + "Mod Osc Fixed Hz", 0.1f, 10000, true, [e] (SynthParams& s) -> float& { return s.engine[e].wt.modHz; });
        add (w + "mod_fine", wn + "Mod Osc Fine", -1, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].wt.modFine; });
        add (w + "mod_vol", wn + "Mod Osc Volume", 0, 1, false, [e] (SynthParams& s) -> float& { return s.engine[e].wt.modVolume; });
    }

    for (size_t k = 0; k < 2; ++k)
    {
        const std::string p = "u_n" + std::to_string (k + 1) + "_", n = "Noise " + std::to_string (k + 1) + " ";
        add (p + "tune", n + "Tune", -36, 36, false, [k] (SynthParams& s) -> float& { return s.utility.noise[k].tune; });
        add (p + "filter", n + "Filter", -1, 1, false, [k] (SynthParams& s) -> float& { return s.utility.noise[k].filter; });
        add (p + "len", n + "Length", 0, 1, false, [k] (SynthParams& s) -> float& { return s.utility.noise[k].length; });
        add (p + "fmix", n + "Filter Mix", 0, 1, false, [k] (SynthParams& s) -> float& { return s.utility.noise[k].filterMix; });
        add (p + "vol", n + "Volume", 0, 1, false, [k] (SynthParams& s) -> float& { return s.utility.noise[k].volume; });
    }
    add ("u_osc_width", "Utility Osc Width", 0, 1, false, [] (SynthParams& s) -> float& { return s.utility.oscWidth; });
    add ("u_osc_coarse", "Utility Osc Coarse", -36, 36, false, [] (SynthParams& s) -> float& { return s.utility.oscCoarse; });
    add ("u_osc_fine", "Utility Osc Fine", -1, 1, false, [] (SynthParams& s) -> float& { return s.utility.oscFine; });
    add ("u_osc_fmix", "Utility Osc Filter Mix", 0, 1, false, [] (SynthParams& s) -> float& { return s.utility.oscFilterMix; });
    add ("u_osc_vol", "Utility Osc Volume", 0, 1, false, [] (SynthParams& s) -> float& { return s.utility.oscVolume; });

    for (size_t f = 0; f < 2; ++f)
    {
        const std::string p = "f" + std::to_string (f + 1) + "_", n = "Filter " + std::to_string (f + 1) + " ";
        add (p + "cutoff", n + "Cutoff", 20, 20000, true, [f] (SynthParams& s) -> float& { return s.filter[f].cutoff; });
        add (p + "reso", n + "Resonance", 0, 1, false, [f] (SynthParams& s) -> float& { return s.filter[f].reso; });
        add (p + "drive", n + "Drive", 0, 1, false, [f] (SynthParams& s) -> float& { return s.filter[f].drive; });
        add (p + "kbd", n + "Key Track", 0, 1, false, [f] (SynthParams& s) -> float& { return s.filter[f].keytrack; });
        add (p + "vol", n + "Volume", 0, 1, false, [f] (SynthParams& s) -> float& { return s.filter[f].volume; });
        add (p + "pan", n + "Pan", -1, 1, false, [f] (SynthParams& s) -> float& { return s.filter[f].pan; });
        add (p + "a", n + "Control A", 0, 1, false, [f] (SynthParams& s) -> float& { return s.filter[f].a; });
        add (p + "b", n + "Control B", 0, 1, false, [f] (SynthParams& s) -> float& { return s.filter[f].b; });
        add (p + "c", n + "Control C", 0, 1, false, [f] (SynthParams& s) -> float& { return s.filter[f].c; });
        add (p + "fm", n + "FM Amount", 0, 1, false, [f] (SynthParams& s) -> float& { return s.filter[f].fm; });
    }
    add ("f_routing", "Filter Routing", 0, 1, false, [] (SynthParams& s) -> float& { return s.filterRouting; });
    add ("amp_velo", "Amp Mod Amount", 0, 1, false, [] (SynthParams& s) -> float& { return s.ampVelocity; });
    add ("voice_pan", "Voice Pan", -1, 1, false, [] (SynthParams& s) -> float& { return s.voicePan; });
    add ("fine_tune", "Fine Tune", -1, 1, false, [] (SynthParams& s) -> float& { return s.transposeFine; });

    auto addEnv = [&] (const std::string& p, const std::string& n, std::function<EnvParams& (SynthParams&)> env) {
        add (p + "_a", n + " Attack", 0.001f, 20, true, [env] (SynthParams& s) -> float& { return env (s).attack; });
        add (p + "_d", n + " Decay", 0.001f, 20, true, [env] (SynthParams& s) -> float& { return env (s).decay; });
        add (p + "_s", n + " Sustain", 0, 1, false, [env] (SynthParams& s) -> float& { return env (s).sustain; });
        add (p + "_r", n + " Release", 0.001f, 20, true, [env] (SynthParams& s) -> float& { return env (s).release; });
    };
    addEnv ("env1", "Env Amp", [] (SynthParams& s) -> EnvParams& { return s.ampEnv; });
    addEnv ("env2", "Env 2", [] (SynthParams& s) -> EnvParams& { return s.env2; });
    addEnv ("env3", "Env 3", [] (SynthParams& s) -> EnvParams& { return s.env3; });

    for (size_t k = 0; k < 3; ++k)
    {
        const std::string i = std::to_string (k + 1);
        add ("lfo" + i + "_rate", "LFO " + i + " Rate", 0.01f, 50, true, [k] (SynthParams& s) -> float& { return s.lfo[k].rate.hz; });
        add ("lfo" + i + "_sym", "LFO " + i + " Symmetry", 0, 1, false, [k] (SynthParams& s) -> float& { return s.lfo[k].symmetry; });
        add ("func" + i + "_rate", "Function " + i + " Rate", 0.01f, 50, true, [k] (SynthParams& s) -> float& { return s.func[k].rate.hz; });
        add ("func" + i + "_scale", "Function " + i + " Scale", -1, 1, false, [k] (SynthParams& s) -> float& { return s.func[k].scale; });
        add ("rand" + i + "_rate", "Random " + i + " Rate", 0.01f, 50, true, [k] (SynthParams& s) -> float& { return s.random[k].rate.hz; });
        add ("rand" + i + "_smooth", "Random " + i + " Smooth", 0, 1, false, [k] (SynthParams& s) -> float& { return s.random[k].smooth; });
        add ("rand" + i + "_dist", "Random " + i + " Distance", 0, 1, false, [k] (SynthParams& s) -> float& { return s.random[k].distance; });
        add ("comb" + i + "_amount", "Combinator " + i + " Amount", 0, 1, false, [k] (SynthParams& s) -> float& { return s.comb[k].amount; });
    }
    // FX section (global destinations).
    const size_t firstFx = d.size();
    static const char* const kSlotNames[9] = { "a1", "a2", "a3", "b1", "b2", "b3", "x1", "x2", "x3" };
    static const char* const kSlotLabels[9] = { "FX A1", "FX A2", "FX A3", "FX B1", "FX B2", "FX B3", "Aux 1", "Aux 2", "Aux 3" };
    for (size_t sl = 0; sl < 9; ++sl)
        for (size_t k = 0; k < (size_t) fx::kFxParams; ++k)
            add (std::string ("fx_") + kSlotNames[sl] + "_p" + std::to_string (k + 1),
                 std::string (kSlotLabels[sl]) + " Param " + std::to_string (k + 1), 0, 1, false,
                 [sl, k] (SynthParams& s) -> float& { return s.fx.slot[sl].p[k]; });
    add ("fx_a_vol", "FX Bus A Volume", 0, 1.5f, false, [] (SynthParams& s) -> float& { return s.fx.busVolume[0]; });
    add ("fx_b_vol", "FX Bus B Volume", 0, 1.5f, false, [] (SynthParams& s) -> float& { return s.fx.busVolume[1]; });
    add ("fx_aux_send", "Aux Send", 0, 1, false, [] (SynthParams& s) -> float& { return s.fx.auxSend; });
    add ("fx_aux_return", "Aux Return", 0, 1, false, [] (SynthParams& s) -> float& { return s.fx.auxReturn; });
    // Sequencer (global).
    add ("seq_transpose", "Seq Transpose", -24, 24, false, [] (SynthParams& s) -> float& { return s.seq.transpose; });
    add ("seq_swing", "Seq Swing", 0.5f, 0.75f, false, [] (SynthParams& s) -> float& { return s.seq.swing; });
    add ("seq_vel_blend", "Seq Velocity Blend", 0, 1, false, [] (SynthParams& s) -> float& { return s.seq.velBlend; });
    for (size_t i = firstFx; i < d.size(); ++i)
        d[i].global = true;
    return d;
}

const char* const kSourceNames[NumModSources] = {
    "Velocity", "Aftertouch", "Mod Wheel", "Keytrack", "Expression",
    "Env Amp", "Env 2", "Env 3",
    "LFO 1", "LFO 2", "LFO 3",
    "Function 1", "Function 2", "Function 3",
    "Random 1", "Random 2", "Random 3",
    "Combinator 1", "Combinator 2", "Combinator 3",
    "Macro 1", "Macro 2", "Macro 3", "Macro 4",
};
} // namespace

const std::vector<ModDest>& modDestinations()
{
    static const std::vector<ModDest> table = build();
    return table;
}

int findModDest (const std::string& id) noexcept
{
    const auto& t = modDestinations();
    for (size_t i = 0; i < t.size(); ++i)
        if (id == t[i].id)
            return (int) i;
    return -1;
}

const char* modSourceName (int s) noexcept
{
    return (s >= 0 && s < NumModSources) ? kSourceNames[s] : "None";
}

int findModSource (const std::string& name) noexcept
{
    for (int i = 0; i < NumModSources; ++i)
        if (name == kSourceNames[i])
            return i;
    return -1;
}

bool isGlobalSource (int s) noexcept
{
    return s == SrcModWheel || s == SrcExpression || s == SrcAftertouch || (s >= SrcMacro1 && s <= SrcMacro4);
}

void ModRouting::prepare() noexcept
{
    numTouched = 0;
    const int numDests = (int) modDestinations().size();
    for (int r = 0; r < count; ++r)
    {
        const auto& rt = routes[r];
        slotOf[r] = -1;
        if (rt.source < 0 || rt.source >= NumModSources || rt.dest < 0 || rt.dest >= numDests)
            continue;
        int slot = -1;
        for (int i = 0; i < numTouched; ++i)
            if (touched[i] == rt.dest)
                slot = i;
        if (slot < 0 && numTouched < kMaxRoutes)
        {
            slot = numTouched++;
            touched[slot] = rt.dest;
        }
        slotOf[r] = slot;
    }
}
} // namespace cs
