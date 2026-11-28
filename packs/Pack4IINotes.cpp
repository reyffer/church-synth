// "How it's built" notes for Pack 4 II (Orchestral Strings on the Section engine).

#include "Packs.h"

#include <map>

juce::String pack4IINotes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        { "Full Strings II", R"(Recipe: two Section engines. Engine 1 = eight violin-range players on the Ensemble body, seated left of centre; Engine 2 = six players an octave down on the Cello body, seated right. One key = the whole section.

What the Section engine does that a synth string can't: each player has their own tuning (Intonation), slow drift, vibrato speed and depth, entry time (Looseness) and seat. That is why the chord sounds like many people, not like one copied oscillator. Then the sum goes through a body: fixed resonances, like the wood of a real instrument. When the vibrato moves the harmonics through those fixed peaks, the tone shimmers.

The mod wheel is Dynamics: louder, brighter and more vibrato together, the orchestral-library way. At rest it plays mezzo-forte.

Try: Macro 1 (Brightness), Macro 3 (Vibrato), Macro 4 (Attack). Compare with "Full Strings" in Pack 4, the oscillator version.)" },
        { "Full Strings Marcato II", R"(Recipe: Full Strings II with Attack near the top (a marcato bow stroke) and lots of Bite: each note starts with a burst of brightness and bow noise, then settles to a lower sustain (the Env Amp decays to 60 %). Velocity adds more bite. Looseness is low so the section attacks together.

Lesson: an accent is not just "louder": it is brighter and noisier at the very start, then quieter. Bite does exactly that.

Try: Macro 4 takes the bite back for a softer accent. Play repeated chords at different velocities.)" },
        { "Full Strings Dynamic II", R"(Recipe: Full Strings II with the mod wheel covering the whole range of Dynamics: wheel down = pianissimo (soft, dark, little vibrato), wheel up = fortissimo (loud, bright, wide vibrato). Velocity is almost off: the wheel does the expression.

How to play it: hold the chord and move the wheel slowly, like a conductor bringing the strings in. A real section changes colour as it gets louder, not only level: here the brightness opens with Dynamics.

Try: assign an expression pedal to the mod wheel in MainStage so your foot does the dynamics.)" },
        { "Violins Ensemble II", R"(Recipe: one Section engine, eight players on the Violin body, seated left of centre (Position −0.35). Vibrato is a bit deeper and fades in after half a second, so short notes stay clean and long notes bloom.

The Violin body has a strong "bridge hill" around 2–3 kHz: that is where a violin section's brilliance lives.

Try: Macro 3 for more vibrato. Lower Intonation for a tighter, more professional section; raise it for a community orchestra.)" },
        { "Violins Con Sordino II", R"(Recipe: violins played with the mute on: Brightness low (0.2), lots of bow Noise, a gentle Attack, softer Dynamics and the body almost fully on. The mute takes the top off and lets the air of the bow come through.

Lesson: soft string sounds are mostly about less brightness and more noise, not only less level.

Try: Macro 4 (Air) adds more bow noise. Move the mod wheel for a slightly fuller sound.)" },
        { "Violins Octaves II", R"(Recipe: two violin sections, the second an octave up (Engine 2 Coarse +12) and a little brighter. Both are seated left, the upper one a little closer to the middle.

Use: melody lines in one hand that sound like the first and second violins doubling in octaves, a classic film-score sound.

Try: lower Engine 2's volume for a more subtle doubling.)" },
        { "Violas Section II", R"(Recipe: six players on the Viola body, centred. The viola body's resonances sit about a fifth lower than the violin's and roll off the top a little, so the same notes sound darker and warmer.

Lesson: the body, not the oscillator, is what makes violin, viola and cello different colours. Switch the Body menu between Violin, Viola and Cello on the same chord to hear it.)" },
        { "Cellos Section II", R"(Recipe: six players on the Cello body, seated right (Position 0.4), with a little Bite. The cello body's main resonances are low, so the bottom of the instrument is full and the top is soft.

Play it in the low-mid range (C2 to C4). Above that it gets thin, as a real cello section would.

Try: Macro 4 for more bite on rhythmic parts.)" },
        { "Celli and Basses II", R"(Recipe: cellos on Engine 1 (right of centre), basses an octave below on Engine 2 (further right, four players on the Bass body). The basses have less vibrato, as real players do.

Use: the left hand under a pad or piano; a deep, warm floor that moves like a real section.)" },
        { "Basses Pizzicato II", R"(Recipe: three bass players, Attack at the top and Bite for the pluck, then the Env Amp decays with no sustain: a plucked string. The Bass body does most of the work: it gives the woody "thump". No vibrato on a pluck. Velocity adds Dynamics, so harder notes are louder and brighter.

Try: Macro 4 (Length) for longer, ringing notes in a slow ballad.)" },
        { "Pizzicato Section II", R"(Recipe: the full section plucking: violins and cellos, Attack and Bite high, no vibrato, a short decay. Looseness is higher here (0.3): a section of pizzicato players is never exactly together, and that spread is what makes it sound like a group.

Try: set Looseness to 0 on both engines to hear how mechanical it becomes.)" },
        { "Staccato Section II", R"(Recipe: full section, Attack at the top, Bite 0.6, no vibrato, and a short Env Amp (0.2 s decay, no sustain). Velocity adds Dynamics, so accents are louder and brighter.

Try: Macro 4 (Length) between spiccato (very short) and a longer staccato.)" },
        { "Tremolo Build II", R"(Recipe: full section starting almost silent (Dynamics 0.05). Env 2 with a 4-second attack raises Dynamics to the top, so the section grows from nothing to full while you hold the chord. LFO 1 (13 Hz triangle) on Amp Mod is the bow moving quickly back and forth: tremolo. More bow noise than usual.

Try: Macro 4 shortens the build. Let go early to stop the swell half way.)" },
        { "Flautando Strings II", R"(Recipe: the bow lightly over the fingerboard: Brightness very low, a lot of Noise, soft Dynamics, a gentle Attack and less body. The result is airy and flute-like.

Use: very quiet moments, spoken prayer, the start of a song before the band comes in.

Try: Macro 4 (Air) for even more breath.)" },
        { "String Ostinato II", R"(Recipe: the full section as short staccato notes, played by the sequencer: it repeats the held chord in eighth notes and follows the tempo. Attack and Bite high, no vibrato.

How to use: hold a chord and the strings play the rhythm. Change chord on the beat.

Try: Macro 4 (Length) for longer notes; change the Seq rate on the Seq page for sixteenths.)" },
        { "Solo Violin II", R"(Recipe: one player (Players 1) on the Violin body, Play Mode Mono Legato with a short glide. When you play legato (the next key before letting go of the last), the Section engine slurs: a quick dip in brightness and level, like a bow change, while the pitch glides.

The vibrato fades in after a third of a second and aftertouch adds more. The mod wheel is Dynamics.

Lesson: a solo instrument needs no Intonation spread and no width; it needs expression: dynamics, vibrato and the slur.)" },
        { "Solo Cello II", R"(Recipe: one player on the Cello body, Mono Legato with a slightly slower glide, a slower vibrato (cellists vibrate wider and slower) that fades in, aftertouch for more. The mod wheel is Dynamics.

Play it from C2 to G4. Swell into long notes with the wheel.)" },
        { "Chamber Quartet II", R"(Recipe: a small group instead of a section: two violins on the left, two lower players (Cello body, an octave down) on the right. Fewer players and less Intonation spread make it intimate; more bow noise and body make it close.

Lesson: Players is the size of the group. 1 = soloist, 2–3 = chamber, 6–8 = section. The same notes feel completely different.)" },
        { "Hymn Strings II", R"(Recipe: the full section with a very slow, soft Attack, Dynamics a little below the middle, Brightness slightly down, a long Env Amp release (2.5 s) and a big hall. The vibrato fades in late (0.8 s) so moving hymn chords stay calm.

Use: hymns, communion, prayer. The mod wheel brings it up for the last verse.)" },
        { "Epic Low Strings II", R"(Recipe: cellos an octave down and basses two octaves down, eight and six players, starting quietly. Env 2 (2.5 s attack) swells Dynamics up while you hold, and the mod wheel adds more on top.

Use: cinematic builds, the bridge of a big worship song. Hold a low fifth and let it grow.

Try: Macro 4 shortens or lengthens the swell.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
