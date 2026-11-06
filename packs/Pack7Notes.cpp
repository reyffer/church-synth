// "How it's built" notes for Pack 7 (shown in the preset browser).

#include "Packs.h"

#include <map>

juce::String pack7Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- pads and strings
        { "Grand Strings", R"(Recipe: this pack copies the 80s flagship polysynth: TWO oscillators per voice, slightly detuned and drifting, a non-resonant highpass, a 4-pole lowpass (OTA model) that can switch to 2-pole, and TWO envelopes (Env 2 for the filter, Env Amp for the volume).

Here: two saws detuned against each other, a highpass at 200 Hz, a slow attack. No chorus at all.

Listen for: compare with PWM Strings in the Chorus Poly pack. That one's width comes from a chorus; this one's comes from two oscillators beating against each other.

Try: Macro 3 (Detune) for more beating.)" },
        { "Grand Warm Pad", R"(Recipe: a saw and a pulse (40 % width) through the filter switched to 2-pole (12 dB). A 2-pole filter is gentler: more harmonics get through, so it sounds smoother and more open even at a low cutoff. Env 2 opens it slowly (1 s attack).

Try: Macro 3 (Filter Swell) for a slower opening.)" },
        { "Envelope PWM Pad", R"(Recipe: two pulse waves whose widths are moved by Env 2 (the filter envelope) in opposite directions, instead of by an LFO. Each chord starts full and its tone thins out as the envelope settles to 30 %.

Listen for: the movement happens once per chord, not in a loop. Envelope PWM sounds played; LFO PWM sounds animated.

Try: Macro 3 (PWM Depth).)" },
        { "Dual Strings and Bells", R"(Recipe: two patches layered, like the flagship's Dual mode. Lower layer (Engine 1): detuned saw strings through Filter 1. Upper layer (Engine 2): a sine an octave up, frequency-modulated by a silent sine 19 semitones up, with Env 3 adding an FM burst and volume at each strike, through its own Filter 2 (parallel routing).

Try: Macro 3 (Bells) for more bell.)" },
        { "Resonant Sweep Strings", R"(Recipe: detuned saws through a resonant filter (0.45) starting very low (500 Hz). Env 2 has a 2-second attack, so every chord sweeps up slowly through its harmonics; the resonance makes the sweep audible.

Try: Macro 3 (Resonance), Macro 4 (Sweep Time).)" },
        { "Glass Pulse Pad", R"(Recipe: two narrow pulses (20 % and 25 %), a high highpass (600 Hz) and an open filter, with a dotted-quarter echo and a hall. Narrow pulses are thin and bright; with no low end, the pad floats above everything.

Try: Macro 3 (Thin) moves the highpass higher.)" },
        // ----------------------------------------------------------------- brass
        { "Grand Brass", R"(Recipe: two detuned saws, a dark filter (700 Hz) that Env 2 snaps open (50 ms attack) and lets settle to 55 %, velocity adding a little extra brightness. The amp envelope is separate and stays up while you hold.

Listen for: two envelopes = the brightness can fall back while the volume stays. That's the flagship-brass "punch then hold".

Try: Macro 3 (Bite) for a shorter filter decay.)" },
        { "Brass Swell Stack", R"(Recipe: the brass recipe with VCO 2 an octave down, a slow amp attack (0.5 s) and an even slower filter attack (0.9 s), so the brightness keeps rising after the volume arrives.

Try: Macro 3 (Swell) for a slower filter.)" },
        { "Unison Brass Stab", R"(Recipe: Unison mode: the synth stacks five detuned copies of the voice on one note, and plays one note at a time. Huge and thick, but monophonic.

Use: single-note brass hits and riffs, not chords.

Try: Macro 3 (Spread) for more detune between the stacked voices.)" },
        // ----------------------------------------------------------------- keys and bells
        { "Cross-Mod Bells", R"(Recipe: cross-mod. VCO 1 is a sine whose pitch is wobbled at audio rate by a sine 19 semitones above (Osc 3, silent). Env 2 adds more cross-mod at the strike, scaled by velocity. That makes inharmonic overtones: metal and glass. VCO 2 adds a quiet sine an octave up.

Try: Macro 3 (Metal) for more cross-mod all the time.)" },
        { "Sync Keys", R"(Recipe: hard sync. VCO 2 restarts its cycle every time VCO 1 does, so it always plays VCO 1's pitch, but Env 2 sweeps VCO 2's own tuning at each note. You hear that as a sweeping, vocal "tearing" tone, not a pitch change. VCO 1 itself is silent.

Try: Macro 3 (Sync Depth) for a bigger sweep.)" },
        { "Bright Poly Pluck", R"(Recipe: a 35 % pulse and a saw, a resonant filter snapped open by a 0.22 s envelope with no sustain, a dotted-eighth echo.

Try: Macro 3 (Resonance) for more snap. Great with the arpeggiator.)" },
        { "Synth Piano 80s", R"(Recipe: a saw with a narrow pulse an octave up, a struck envelope (no sustain, 2.5 s decay, shorter on high notes) and velocity on both level and brightness. A light chorus.

Listen for: not a real piano and not trying to be: the 80s pop "synth piano" was its own instrument.

Try: Macro 3 (Chorus).)" },
        { "Grand Arpeggio", R"(Recipe: a saw-and-pulse pluck through the arpeggiator: up and down three octaves in 16ths, with a dotted-eighth echo crossing the rhythm.

Use: hold a chord; it follows the host tempo.

Try: change the order and octaves on the SEQ page.)" },
        // ----------------------------------------------------------------- leads
        { "Unison Lead", R"(Recipe: Unison mode (five stacked, detuned voices), legato with a short glide, a vibrato on the mod wheel that fades in after half a second, and an echo.

Try: Macro 3 (Spread) for more detune. Mod wheel up for vibrato.)" },
        { "Sync Lead", R"(Recipe: hard sync again, with VCO 2 tuned a fifth up and swept by Env 2 at each note AND by the mod wheel. VCO 1 is mixed in quietly underneath for body.

Listen for: ride the mod wheel while holding a note: the tone screams without the pitch moving.

Try: Macro 3 (Sync Sweep) for a longer sweep.)" },
        { "Soft Square Lead", R"(Recipe: two squares an octave apart through the gentle 2-pole filter, legato with a short glide and a delayed vibrato.

Try: Macro 3 (Octave) for more of the upper square.)" },
        // ----------------------------------------------------------------- bass
        { "Unison Bass", R"(Recipe: a saw and a saw an octave down, stacked three times in Unison mode with the stereo spread kept narrow (bass should stay centred), a low filter and a short envelope blip.

Try: Macro 3 (Punch) for a longer filter decay.)" },
        { "Sync Bass", R"(Recipe: hard sync on a bass: VCO 2 tuned a fourth up and swept by Env 2 at each note, with VCO 1 underneath for the fundamental.

Try: Macro 3 (Sync Depth) for more growl.)" },
        // ----------------------------------------------------------------- dual
        { "Dual Brass and Strings", R"(Recipe: Dual mode again: brass on Engine 1 through Filter 1 (with its punchy filter envelope), detuned strings on Engine 2 through their own, open Filter 2 (parallel routing).

Listen for: the brass gives the attack, the strings give the sustain.

Try: Macro 3 (Strings) for more of the string layer.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
