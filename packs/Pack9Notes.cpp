// "How it's built" notes for Pack 9 (shown in the preset browser).

#include "Packs.h"

#include <map>

juce::String pack9Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- pads
        { "Vintage Poly Pad 5", R"(Recipe: this pack copies the late-70s five-voice classic. Two oscillators per voice (here Osc 1 plays the original's "Osc B" and Osc 2 its "Osc A"), with analog drift, a warm resonant 24 dB lowpass (Classic, Analog on), a filter envelope and an amp envelope.

Here: a saw and a 40 % pulse, the filter opened slowly by Env 2. The mod wheel brings in PWM (the original's "wheel-mod").

Try: Macro 3 (Detune), mod wheel for movement.)" },
        { "Poly-Mod Pad", R"(Recipe: poly-mod, the original's signature. A silent copy of Osc B (on Osc 3, a fifth up) frequency-modulates Osc A a little, and the filter envelope adds a bit more. FM between the oscillators adds a glassy, slightly metallic shimmer that a plain detune can't.

Try: Macro 3 (Poly-Mod) for more. A little goes a long way.)" },
        { "Analog Strings 5", R"(Recipe: two saws detuned by 12 cents, extra drift, the filter fairly open, a highpass at 200 Hz and a slow attack. The mod wheel adds vibrato.

Try: Macro 3 (Detune) for a wider, more chorused string section.)" },
        { "Dark Sweep Pad", R"(Recipe: a saw and a pulse through a dark, resonant filter (350 Hz, resonance 0.45). Env 2 has a 2.5 s attack, so every chord sweeps open slowly.

Try: Macro 3 (Resonance), Macro 4 (Sweep Time). Hold chords for a few seconds.)" },
        { "Hollow Pulse Pad", R"(Recipe: two narrow pulses (20 % and 25 %): narrow pulses are hollow and reedy. The mod wheel moves Osc A's width with the LFO (PWM).

Try: Macro 3 (Width) changes Osc B's width: hear how the tone changes from reedy to full.)" },
        // ----------------------------------------------------------------- brass
        { "Vintage Brass 5", R"(Recipe: two saws, a dark filter (650 Hz) opened by Env 2 with an 80 ms attack and settling to 55 %, velocity adding brightness, drift for warmth.

Listen for: the slight instability between voices is part of the charm.

Try: Macro 3 (Bite) for shorter stabs.)" },
        { "Soft Brass Pad", R"(Recipe: the brass recipe slowed down: a 0.5 s amp attack, a 1 s filter attack and a long release. Brass colour, pad behaviour.

Use: under worship, especially big hymns. Try: Macro 3 (Swell) for a slower opening.)" },
        { "Poly-Mod Brass", R"(Recipe: brass where the filter envelope also drives poly-mod (FM from Osc B into Osc A). At each note the tone gets a buzzy, almost vocal burst that fades as the envelope settles.

Try: Macro 3 (Poly-Mod) for a stronger burst.)" },
        // ----------------------------------------------------------------- keys and plucks
        { "Vintage Pluck Keys", R"(Recipe: a saw and a 30 % pulse, resonance 0.2, the filter snapped open by a fast envelope that settles low, a decaying amp envelope with a little sustain, light chorus.

Use: the singer-songwriter synth comp. Try: Macro 3 (Chorus).)" },
        { "Poly-Mod Bells", R"(Recipe: two triangles. Osc B, tuned an octave and a fifth up (19 semitones) on silent Osc 3, frequency-modulates Osc A; the filter envelope adds a strong burst of it at each strike. Non-octave FM ratios = inharmonic, bell-like overtones.

Try: Macro 3 (Clang) for more metal.)" },
        { "Sync Sweep Keys", R"(Recipe: the original's most famous trick. Osc A is synced to Osc B (Osc B silent), and the filter envelope sweeps Osc A's pitch at each note (poly-mod to frequency). You hear the sweep as a tearing, vocal change of tone, not of pitch.

Try: Macro 3 (Sweep) for a bigger sweep.)" },
        { "Synth Harpsichord", R"(Recipe: two very narrow pulses (12 % and 20 %), one an octave up, a fast bright pluck and no sustain, shorter on high notes.

Try: Macro 3 (Octave) for more of the upper pulse.)" },
        { "Glass Keys", R"(Recipe: a triangle (Osc B) and a narrow pulse an octave up (Osc A), with a small poly-mod burst from the filter envelope at each strike. Clear, glassy, a little bell on the attack.

Try: Macro 3 (Glass) for more.)" },
        // ----------------------------------------------------------------- leads
        { "Sync Sweep Lead", R"(Recipe: Osc A an octave up and synced to Osc B, swept by the filter envelope at each note, AND by the mod wheel. Osc B is mixed in underneath for body.

Listen for: hold a note and ride the wheel: the tone screams while the pitch stays.

Try: Macro 3 (Sweep) for a longer envelope sweep.)" },
        { "Unison Saw Lead", R"(Recipe: Unison: five stacked, detuned voices on one note, legato with a short glide. The mod wheel adds vibrato.

Try: Macro 3 (Spread) for more detune.)" },
        { "Wheel-Mod Lead", R"(Recipe: a saw and a pulse through a resonant filter (0.4). The mod wheel brings LFO 1 (4 Hz) onto the cutoff: a talking, wah-like wobble.

Try: Macro 3 (Wobble Speed). Use the wheel like a wah pedal.)" },
        // ----------------------------------------------------------------- bass and arp
        { "Vintage Poly Bass", R"(Recipe: a saw and a pulse an octave down, a low filter (300 Hz) with a punchy envelope.

Try: Macro 3 (Punch) for a longer filter decay.)" },
        { "Unison Bass 5", R"(Recipe: Unison on a bass: four stacked voices (two oscillators each, one an octave down) with the stereo spread kept narrow so the bass stays centred.

Try: Macro 3 (Spread) for more detune.)" },
        { "Vintage Arp", R"(Recipe: the pluck recipe played by the arpeggiator, up two octaves in 16ths, with a dotted-eighth echo.

Use: hold a chord; it follows the host tempo. Try: Macro 3 (Resonance).)" },
        { "Wheel-Mod Strings", R"(Recipe: analog strings (a saw and a square, filter open, highpass at 220 Hz) where the mod wheel brings in slow PWM on Osc A. Wheel down = still and clean; wheel up = shimmering, like a chorus.

Try: ride the wheel slowly through a song.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
