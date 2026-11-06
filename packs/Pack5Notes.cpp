// "How it's built" notes for Pack 5 (shown in the preset browser).

#include "Packs.h"

#include <map>

juce::String pack5Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- R&B and soul synth brass
        { "RnB Brass Stab", R"(Recipe: every brass sound in this pack starts from the same idea. Two saws (a brass instrument's tone is close to a saw) through a 24 dB lowpass that opens with Env 2: half of the opening always happens, half depends on velocity. So louder notes are brighter, exactly like real brass. Env 3 bends the pitch up into each note by half a semitone (the "scoop") and a little "Breath" noise adds air.

Here: a quick filter opening that falls back to 35 %, some analog drift and Vintage Chorus II.

Try: Macro 3 (Bite) shortens the filter decay for punchier stabs.)" },
        { "Velvet RnB Brass", R"(Recipe: the brass recipe slowed down: a 0.18 s attack, a filter that stays fairly open (70 %), three unison players, drift and Vintage Chorus I + II, plus a hall.

Listen for: it's brass by its tone, but it moves like a pad. That's the 90s R&B ballad sound.

Try: Macro 3 (Swell) slows the filter opening.)" },
        { "Soul Brass Swell", R"(Recipe: a 0.6 s amp attack and Env 2 opening the filter over 1.2 s from very dark (400 Hz). The chord grows in volume and in brightness.

Listen for: the brightness keeps rising after the volume has arrived. That's what makes a swell sound exciting.

Try: Macro 3 (Swell) for an even slower opening.)" },
        { "Funk Brass Hits", R"(Recipe: the brass recipe with no sustain: a 3 ms attack, a 0.18 s decay and a filter that snaps shut. Velocity changes both the level and the brightness of each hit.

Try: Macro 3 (Bite) adds resonance for a sharper edge. Macro 4 (Length) for longer hits.)" },
        { "Bright Synth Brass", R"(Recipe: three unison players through the OTA filter (smooth and creamy, the 80s polysynth filter) at a higher cutoff, with chorus. No breath noise: this is unashamedly a synth.

Try: Macro 3 (Bite) for shorter filter decay. Use it for stabs or held pads.)" },
        { "Octave Synth Brass", R"(Recipe: the brass recipe with a third saw an octave below (Osc 3) and Vintage Chorus I. One finger plays the line in octaves, like trumpets with trombones underneath.

Try: Macro 3 (Octave) for a louder low octave.)" },
        { "Gospel Brass", R"(Recipe: three players, a brighter filter, filter drive for grit, and a deeper scoop (a full semitone): every note bends up into place like a gospel horn shout.

Try: Macro 3 (Scoop) makes the scoop longer. Play hard: velocity opens the filter.)" },
        // ----------------------------------------------------------------- sharp brass
        { "Sharp Brass Stab", R"(Recipe: a very fast attack (3 ms), a filter that opens a long way (50 %) and falls back quickly, and resonance (0.3). The resonance boosts the frequency at the cutoff as it sweeps: that's the "blat".

Try: Macro 3 (Blat) for more resonance.)" },
        { "Brass Fall", R"(Recipe: the engine is tuned 5 semitones low, and Env 3 lifts it back to the right pitch while you hold the key (attack 0, sustain 100 %). When you let go, Env 3's release (0.4 s) lets the pitch fall, while the amp's release fades it out: the big-band "fall".

Try: Macro 3 (Fall) makes the fall slower. Release the key on purpose, on the beat.)" },
        { "Brass Sfz Swell", R"(Recipe: two envelopes. The amp hits hard and drops to 30 % (the sforzando). Env 3 then rises over 1.8 s, raising Engine 1's volume and opening the filter (the crescendo).

Listen for: hit, back off, grow. Hold the chord for at least two seconds.

Try: Macro 3 (Swell Time) for a slower crescendo.)" },
        { "Staccato Brass", R"(Recipe: three players with a very short envelope (4 ms attack, 0.22 s decay, no sustain) and a filter that closes completely.

Try: Macro 4 (Length) between tight staccato and a slightly longer note.)" },
        { "Brass Blast", R"(Recipe: four unison players, an octave below on Osc 3, a deeper scoop and a Tube distortion on bus A. Big, slightly rough, loud.

Use: final chords and endings.

Try: Macro 3 (Drive) for more tube.)" },
        // ----------------------------------------------------------------- horns
        { "Trumpet Section", R"(Recipe: three players with a bright filter, a strong velocity opening, a scoop and breath, plus a highpass at 300 Hz (trumpets have little low end).

Try: Macro 3 (Scoop) for a longer bend into each note. Play lines with clear, separate notes.)" },
        { "Trombone Section", R"(Recipe: a saw and a square (the square adds a hollow, buzzy tone), three players, a lower filter (500 Hz) and a slower attack. Trombones speak a little later than trumpets.

Use: the low-mid range. Try: Macro 3 (Buzz) adds filter drive for a rougher, louder trombone.)" },
        { "Horn Section", R"(Recipe: a saw (trumpet), a square (sax-like reed) and a saw an octave down, slightly detuned (trombone), with the brass filter and scoop. Three instruments from one patch.

Try: Macro 3 (Low Horns) for more of the low octave.)" },
        { "French Horns", R"(Recipe: a saw with a triangle (soft, few harmonics), a very low filter (450 Hz) with little resonance, a slower attack (80 ms) and a large hall. French horns are round and dark because the player's hand sits inside the bell.

Try: Macro 3 (Breath) for more air.)" },
        { "French Horns Soft", R"(Recipe: even darker (350 Hz), a slow 0.25 s attack, no scoop, less velocity response and a bigger hall.

Use: a warm bed under worship, or the last held chord of a hymn.)" },
        { "Solo Trumpet", R"(Recipe: one player (no unison), mono and legato with a 40 ms slide, a deeper scoop, breath and a vibrato that fades in, more with aftertouch.

Listen for: overlap the notes to slide between them; separate them to re-articulate each one.

Try: Macro 3 (Scoop) for jazzier bends.)" },
        { "Muted Trumpet", R"(Recipe: Solo Trumpet followed by a bandpass filter at 1.2 kHz that removes the low and the very high end. That narrow, nasal band is what a mute in the bell does.

Try: Macro 3 (Mute) adds resonance to the band for a more "wah-like" mute.)" },
        { "Flugelhorn", R"(Recipe: a saw with a triangle, a dark filter (600 Hz), a softer attack, more breath and a gentle vibrato. A flugelhorn is a trumpet with a wider tube: rounder and darker.

Try: compare with Solo Trumpet: same playing, softer instrument.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
