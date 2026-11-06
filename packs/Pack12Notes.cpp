// "How it's built" notes for Pack 12 (shown in the preset browser).

#include "Packs.h"

#include <map>

juce::String pack12Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- plucks
        { "Progressive Pluck", R"(Recipe: a 7-voice Super unison saw (the "supersaw": seven detuned saws spread across the stereo field), a filter snapped open by a 0.25 s envelope with no sustain, a dotted-eighth ping-pong echo and a big hall.

Listen for: the reverb and echo are half the sound. Dry, it's a short blip.

Try: Macro 3 (Echo), Macro 4 (Decay).)" },
        { "Tropical Pluck", R"(Recipe: FM with a 3:1 modulator that sounds only for 80 ms at the strike, then a pure, short sine: soft, woody and marimba-like. A dotted echo for the bounce.

Try: Macro 3 (Mallet) for a harder strike.)" },
        { "Future Pluck", R"(Recipe: Chord-mode unison set to Major 7: one key plays a whole major-seventh chord. Env 3 bends the pitch up from a semitone below at each note (the "pitch rise" of future bass), with chorus and echo.

Use: play melodies with one finger; they come out as lush chords.)" },
        { "Trance Pluck", R"(Recipe: a supersaw with resonance (0.3) and a very fast filter snap (0.15 s). Short and bright for fast arpeggios.

Try: turn on the arpeggiator (SEQ page) and hold a chord.)" },
        { "Bell Drop Pluck", R"(Recipe: an FM bell (2:1 body) with a short, bright 7:1 layer on top, a 1.2 s decay and a long dotted echo.

Try: Macro 3 (Metal) for a brighter bell.)" },
        { "Pumping Pluck", R"(Recipe: a saw pluck with a little sustain, whose volume ducks on every beat (Function 1 "Duck", synced to quarter notes, on the amp). That's what a side-chain compressor does to synths in a dance track.

Try: Macro 3 (Pump) for a deeper duck.)" },
        { "Dark Techno Pluck", R"(Recipe: a saw and a saw an octave down through a driven, resonant Ladder filter (resonance 0.45) with a short envelope. Dark and hypnotic.

Try: Macro 3 (Resonance) slowly up through a build.)" },
        // ----------------------------------------------------------------- stabs
        { "Big Room Stab", R"(Recipe: a very detuned supersaw with a sub, a bright filter, a short decay to 30 %, and a little Tube distortion. Huge, simple, loud.

Try: Macro 3 (Drive).)" },
        { "Future Bass Chords", R"(Recipe: supersaw chords with three tricks at once. (1) A pitch rise into each chord (Env 3, two semitones in 80 ms). (2) A side-chain pump on every beat. (3) Macro 3 brings in an eighth-note LFO on the filter: the "wobble".

Try: hold big chords and ride Macro 3.)" },
        { "House Organ Stab", R"(Recipe: three sines: the note, an octave up and an octave-and-a-fifth up (organ partials), through a slightly driven filter, with a quick decay to a low sustain. The percussive 90s house organ.

Try: Macro 3 (Click) brings up the top partial for a sharper stab.)" },
        { "Rave Stab", R"(Recipe: Chord-mode unison set to Minor: one key plays a minor triad. Resonant saws with a snappy filter envelope. The 90s rave stab.

Try: Macro 3 (Resonance).)" },
        { "Piano House Stab", R"(Recipe: a bright FM piano (1:1 with strong velocity bark, plus a short 14:1 tine), a 1.5 s decay. Punchy enough to play off-beat chords against a kick.

Try: Macro 3 (Bark) for more brightness.)" },
        { "Pumping Saw Stab", R"(Recipe: sustained saw chords with a deep side-chain pump (70 %). Hold the chord: the rhythm comes from the pump.

Try: Macro 3 (Pump).)" },
        { "Brass Drop Stab", R"(Recipe: saw brass with a pitch rise into each stab, a brass filter envelope and some drive.

Try: Macro 3 (Bite) for shorter stabs.)" },
        // ----------------------------------------------------------------- leads
        { "Trance Supersaw Lead", R"(Recipe: a 7-voice supersaw with the filter wide open, legato with a short glide, vibrato on the mod wheel, a long echo and reverb.

Try: Macro 3 (Width) for more detune.)" },
        { "Festival Lead", R"(Recipe: two very detuned saws with unison and a sub, a little drive, and the pitch bend set to a full octave for big dive-bombs and rises.

Try: bend up an octave into the drop.)" },
        { "Future Bass Lead", R"(Recipe: a square and a saw with a pitch rise into every note, a long glide and a quick vibrato that fades in, with chorus and echo.

Try: Macro 3 (Vibrato speed).)" },
        { "Sync Screech Lead", R"(Recipe: hard sync (Osc 2 an octave up, locked to Osc 1) swept by Env 2 and by the mod wheel, through a driven filter. Edgy and cutting.

Try: Macro 3 (Screech) or the mod wheel.)" },
        { "Vocal Chop Lead", R"(Recipe: saws through a Formant (vowel) filter. Random 1 is a Voice Modulator: every new note gets the next value from a list, which moves the vowel. Each note sounds like a different syllable, like chopped vocal samples.

Try: Macro 3 (Vowel) moves all the vowels together.)" },
        { "Drop Pluck Lead", R"(Recipe: a saw lead whose filter plucks on every note (0.25 s) but keeps a little sustain, mono with a short glide, echo and reverb.

Try: Macro 3 (Resonance).)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
