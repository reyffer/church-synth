// "How it's built" notes for Pack 15 (shown in the preset browser).
// Each note is a small sound-design lesson: how far synthesis gets towards a real piano.

#include "Packs.h"

#include <map>

juce::String pack15Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- grands
        { "Concert Grand", R"(Recipe: every piano in this pack is built from the same five ingredients.
1. The string: a wavetable between a sine and a saw (Engine 1). Three unison voices, a hair apart (about 1 cent), all starting in phase, like the three strings a hammer hits at once. They drift apart as the note rings, which gives the "bloom" and slow beating of a real piano.
2. The strike: Engine 2 is a short, bright FM tone that dies in a third of a second (Env 3, scaled by velocity). A real piano note has this "double decay": a loud, bright start, then a quieter tail that rings for a long time.
3. The hammer: a short Key Click noise on the Utility engine.
4. Velocity does three jobs: louder (Amp Mod), brighter filter, and a richer string (it moves the wavetable towards the saw).
5. No sustain on the amp envelope, with the steepest decay curve so it fades evenly in decibels like a real string; Keytrack shortens the decay as you go up.

Lesson: a piano isn't one sound but a hammer, a strike and a long ring, and the way they change with velocity and pitch.

Try: Macro 1 (Tone), Macro 2 (Space), Macro 3 (Hammer), Macro 4 (Decay). Play very softly, then hard.)" },
        { "Studio Grand", R"(Recipe: the same piano as Concert Grand, a little darker at rest, a slightly shorter decay and a small room instead of a hall.

Lesson: for playing in a band, less reverb and a slightly shorter ring keep a piano clear; the hall belongs to solo moments.

Try: Macro 3 (Hammer) for more or less attack noise.)" },
        { "Warm Grand", R"(Recipe: the string wavetable sits nearer the sine (fewer harmonics), the filter rests low (400 Hz), the strike layer is gentler and the hammer is a soft Thump instead of a click.

Lesson: "warm" usually means fewer upper harmonics at rest while velocity can still open the tone. Play hard and it still brightens.

Try: Macro 1 (Tone) to bring a little more top back.)" },
        { "Bright Pop Grand", R"(Recipe: the string wavetable sits near the saw, the filter rests high, the FM strike layer is loud, and a compressor on FX A evens the dynamics so the piano stays in front.

Lesson: pop pianos are recorded bright and compressed so they cut through drums and guitars. A compressor also makes the hammer attack feel punchier.

Try: Macro 3 (Sparkle) adds FM to the strike layer: a glassier attack.)" },
        { "Soft Grand", R"(Recipe: a mellow string, a low filter, a tiny Soft Click hammer, and less velocity on the volume (Amp Mod 0.55), so even soft playing stays audible while the tone still changes with touch.

Lesson: lowering velocity-to-volume and keeping velocity-to-brightness is how you make a gentle, even piano that still responds.

Try: hold chords and let the long tail ring into the hall.)" },
        { "Ballad Grand", R"(Recipe: Concert Grand with a long release (0.5 s, so lifted chords don't stop abruptly), a soft dotted-quarter echo on FX A and a big hall on the Aux bus.

Lesson: a tempo-synced echo, set low, makes a piano sound bigger without washing it out.

Try: Macro 2 (Space).)" },
        { "Rock Grand", R"(Recipe: a near-saw string, a bright filter, a loud strike layer and a hard hammer, with Drive on Filter 1 to grit the attack.

Lesson: rock piano is bright, short and a bit driven. Drive before the filter adds harmonics, and the filter then shapes them.

Try: Macro 3 (Drive).)" },
        { "Worship Grand", R"(Recipe: a slightly warm string and filter, a moderate strike, a gentle hammer, a long decay and release, and a big hall.

Lesson: for worship the piano needs to be soft-edged under singing but still clear when you dig in. That is mostly the resting filter (low) versus velocity brightness (high).

Try: Macro 2 (Space), Macro 4 (Decay) for even longer chords.)" },
        // ----------------------------------------------------------------- soul / gospel
        { "Soul Ballad Piano", R"(Recipe: the R&B / neo-soul ballad piano: a warm, rounded string, a low resting filter with plenty of velocity brightness, a soft Thump hammer, and a compressor that holds it present and close. A small room, not a hall.

Lesson: this style of piano is recorded close and compressed: warm, intimate and upfront rather than big and roomy. Most of its character is in how it is played: rich voicings, low octaves, soft dynamics.

Try: Macro 3 (Warmth) rounds the string off further.)" },
        { "Neo-Soul Piano", R"(Recipe: a dark string and filter, a soft Thump hammer, a slow free-running LFO that wobbles the pitch of both engines by a couple of cents (tape wow), and a faint Tape Echo underneath.

Lesson: Macro 3 doesn't move a knob here: it is the sidechain of the two LFO routes, so it scales how deep the wobble is. Any route can be put on a macro this way.

Try: Macro 3 (Wobble) from a hint of wow to a seasick old tape.)" },
        { "Gospel Grand", R"(Recipe: a bright, saw-like string, a punchy strike, a strong hammer and a compressor, with the most velocity response on the filter.

Lesson: gospel playing uses the whole dynamic range, from whispered fills to shout chords. A wide velocity-to-brightness range is what makes that come alive.

Try: play soft runs, then hit a big chord.)" },
        // ----------------------------------------------------------------- uprights
        { "Upright Piano", R"(Recipe: the grand recipe with a shorter decay (6.5 s), a stronger hammer, and Filter 2 as a highpass at 150 Hz in series, thinning the bass.

Lesson: an upright's shorter strings and smaller soundboard give less low end and a shorter, boxier tone. A highpass plus a shorter decay gets you most of the way.

Try: Macro 3 (Hammer).)" },
        { "Honky-Tonk", R"(Recipe: the upright, but the three unison "strings" are detuned about +/-12 cents instead of 1, so each note beats and chorus-wobbles.

Lesson: that wobble IS the honky-tonk sound: on an old saloon piano the strings of each note have drifted apart. It's the same unison control, just pushed much further.

Try: Macro 3 (Detune).)" },
        { "Old Upright", R"(Recipe: a dark upright, strings slightly out of tune (about +/-4 cents), a Tape Echo for wow and flutter, and a little tape hiss on Noise 2.

Lesson: "old" is a combination of small faults: dull, slightly out of tune, slightly wobbly, slightly noisy. Any one alone sounds like a mistake; together they sound like a recording.

Try: Macro 3 (Hiss).)" },
        // ----------------------------------------------------------------- character
        { "Muted Piano", R"(Recipe: a very dark string and filter, a Thump hammer and a very short decay (1.6 s), like a piano with the felt practice rail down.

Lesson: decay length changes what instrument we hear. The same piano with a short decay becomes intimate and percussive.

Try: Macro 4 (Decay), Macro 3 (Thump).)" },
        { "Lo-Fi Piano", R"(Recipe: a dull piano, one free-running slow LFO wobbling the pitch of both engines a little (wow), a Tape Echo, and Vinyl Dust noise underneath.

Lesson: the LFO is set to Free Running so the wobble keeps going across notes, like a tape that's slightly stretched, rather than restarting with each key.

Try: Macro 3 (Dust).)" },
        { "Dark Cinematic Piano", R"(Recipe: the darkest string and filter, a slow decay (14 s) and release (1 s), a ping-pong echo and a very large hall.

Lesson: distance is darkness plus reverb. Low notes and single lines work best; dense chords turn to mush in this much space.

Try: Macro 3 (Echo).)" },
        { "Toy Piano", R"(Recipe: not strings at all. A toy piano has metal rods, so this is an FM bell: a sine modulated at 3.5x (an inharmonic ratio, like a struck rod), with Env 2 adding a burst of FM on the attack, a Click noise, an octave up, and a short decay.

Lesson: a "piano" sound is really about the source. Change the strings to rods and you need FM's inharmonic partials instead of a saw.

Try: Macro 1 (Tone), Macro 3 (Clunk).)" },
        // ----------------------------------------------------------------- layers
        { "Piano and Pad", R"(Recipe: the warm grand on Filter 1, plus the Utility saw through its own darker Filter 2 (parallel routing). Function 1 is a Swell shape in Envelope mode that fades the saw in over a couple of seconds while you hold the chord.

Lesson: the pad uses the piano's amp envelope, so it fades away with the piano's long decay: it fills the middle of a held chord rather than droning forever.

Try: Macro 3 (Pad) for more pad.)" },
        { "Piano and Strings", R"(Recipe: the grand, plus the Utility saw through a brighter Filter 2, faded in by Function 1 (Swell, Envelope mode), with a gentle delayed vibrato (LFO 1 with fade-in on the saw's pitch) and a chorus to widen it into a section.

Lesson: two layers that start at different times sound like two instruments. The piano speaks first, the strings answer behind it.

Try: Macro 3 (Strings).)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
