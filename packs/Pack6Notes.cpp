// "How it's built" notes for Pack 6 (shown in the preset browser).

#include "Packs.h"

#include <map>

juce::String pack6Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- pads
        { "PWM Strings", R"(Recipe: the whole pack copies one famous 80s polysynth: ONE oscillator per voice (a saw, a pulse, or both), a square sub an octave down, a highpass, a smooth 4-pole lowpass (the OTA model), one envelope for both filter and amp, and a stereo chorus.

Here: only the pulse wave, with LFO 1 slowly changing its width (PWM). PWM makes one oscillator sound like several drifting against each other. A thin highpass, a slow attack and Chorus II.

Try: Macro 3 (PWM Speed). Faster PWM = more "shimmer".)" },
        { "Saw Strings", R"(Recipe: only the saw, the filter fairly open, no filter envelope, the thinnest highpass setting (720 Hz) and Chorus I + II.

Listen for: the highpass is what makes it "strings" rather than "pad": no low end, just the bright, buzzy top.

Try: Macro 3 (Thin) moves the highpass even higher.)" },
        { "Dream Pad", R"(Recipe: saw, pulse (with gentle PWM) and sub, a dark filter at 900 Hz with a slow LFO wave on the cutoff, a slow attack and release, Chorus I.

Try: Macro 3 (Motion) makes the filter wave faster. Macro 1 down for verses.)" },
        { "Night Drive Pad", R"(Recipe: saw plus a strong sub, a very low filter (400 Hz) with some resonance, and the envelope opening it slowly (1.2 s attack) then settling back (sustain 60 %).

Listen for: each chord blooms and then darkens slightly. That's one envelope doing two jobs.

Try: Macro 3 (Sub) for more weight.)" },
        { "Breathing Pad", R"(Recipe: mostly pulse with PWM, plus a little saw, and LFO 1 also moving the filter. One LFO, two jobs: the pad changes width and brightness together, like breathing.

Try: Macro 3 (Breath Speed) faster or slower.)" },
        { "Sweep Pad", R"(Recipe: saw and sub through a resonant filter (0.5) with no envelope, swept up and down by a slow, free-running LFO (one cycle every 8 seconds). Chorus I + II.

Listen for: the resonance makes the sweep "talk" as it passes through the harmonics.

Try: Macro 3 (Sweep Speed), Macro 4 (Resonance).)" },
        // ----------------------------------------------------------------- keys, stabs, plucks
        { "Chorus Brass", R"(Recipe: saw with a little sub, a dark filter (600 Hz) and ONE envelope (60 ms attack, 0.5 s decay, 60 % sustain) moving both the filter and the volume. That's how brass works on this kind of synth: the sound gets brighter exactly as it gets louder.

Try: Macro 3 (Bite) shortens the decay for punchier stabs.)" },
        { "Poly Stab", R"(Recipe: saw and a 35 % pulse, a resonant filter that the envelope snaps open and shut (no sustain), Chorus II and a dotted-eighth echo.

Use: short dance-pop chords on the off-beats.

Try: Macro 3 (Echo) for more echo, Macro 4 for a longer decay.)" },
        { "Chorus Keys", R"(Recipe: mostly pulse with slow PWM, a quick envelope (3 ms attack) that decays to a soft 40 % sustain, Chorus II.

Use: the bread-and-butter 80s comping sound under a vocal.

Try: Macro 3 (Chorus) changes the chorus mix.)" },
        { "Pulse Organ", R"(Recipe: a square wave and the sub, with the amp in "gate" mode (on/off, no envelope shape), Chorus I + II, and a vibrato that fades in after 0.6 s (the LFO delay).

Listen for: squares have only odd harmonics, which is why they sound hollow and organ-like.

Try: Macro 3 (Sub) for a fuller organ.)" },
        { "Poly Pluck", R"(Recipe: saw and a narrow pulse, the filter closed (500 Hz) and snapped open by a 0.25 s envelope with no sustain, resonance for snap, Chorus I and a dotted-eighth echo.

Try: Macro 3 (Resonance) for a sharper "pew". Works great with the arpeggiator on (SEQ page).)" },
        { "Resonant Bell", R"(Recipe: a very narrow pulse (15 %) through the filter with high resonance (0.7) and full key tracking. The resonance rings on the harmonics near the cutoff, and the narrow pulse has a lot of them: the result rings like a bell.

Try: Macro 3 (Ring) for more resonance.)" },
        { "Chord Memory Stab", R"(Recipe: unison in Chord mode set to Minor 7: each key plays a whole minor-seventh chord, the "chord memory" trick of house and garage records. A short stab envelope, Chorus II and an echo.

Use: play a melody with one finger and it becomes a chord line. Keep to the song's scale.

Try: change the chord type on Engine 1 (Unison > Chord).)" },
        // ----------------------------------------------------------------- bass
        { "Square Sub Bass", R"(Recipe: the pulse wave and a loud sub, a low filter with a short blip from the envelope, and a little chorus with the width kept narrow (bass should stay centred).

Use: round, solid house and pop bass. Try: Macro 3 (Sub) for more weight on big systems.)" },
        { "Saw Bass Punch", R"(Recipe: saw and sub, no chorus, the filter opened hard and fast by the envelope (0.25 s decay to 30 %).

Listen for: the "punch" is the filter, not the volume.

Try: Macro 3 (Resonance) for a squelchier bass.)" },
        { "Rubber Bass", R"(Recipe: pulse and sub through a resonant filter (0.55) and a very fast envelope (0.18 s). High resonance plus a fast sweep = a bouncy, "boing" quality.

Try: Macro 3 (Rubber) for even more resonance.)" },
        { "Arp Bass 16ths", R"(Recipe: the saw-and-sub bass played by the arpeggiator: up two octaves in sixteenths. Hold one note and it bounces between the note and its octave, in time with the song.

Try: change the octave range on the SEQ page.)" },
        // ----------------------------------------------------------------- leads and arps
        { "Chorus Lead", R"(Recipe: saw and pulse, mono and legato with a short glide, a vibrato that only fades in after half a second (the LFO delay) and Chorus I with an echo.

Listen for: short notes stay straight; long notes start to sing. That's the LFO delay, and it makes a lead feel played.

Try: Macro 3 (Vibrato Speed).)" },
        { "Resonant Whistle", R"(Recipe: very high resonance (0.92) on a filter that tracks the keyboard 100 %, with only a little saw going in. The resonance picks out the harmonics near the cutoff and rings on them, so most of what you hear is the filter itself singing.

Try: Macro 3 (Body) brings up the saw underneath for a fuller tone.)" },
        { "Chorus Arp", R"(Recipe: pulse with PWM and a little saw, a snappy envelope, played by the arpeggiator up and down two octaves in sixteenths, with Chorus II and a dotted-eighth echo.

Use: hold a chord and let it run; it follows the host tempo.

Try: Macro 3 (Echo) and the arp settings on the SEQ page.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
