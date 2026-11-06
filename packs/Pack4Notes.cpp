// "How it's built" notes for Pack 4 (shown in the preset browser).

#include "Packs.h"

#include <map>

juce::String pack4Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- full section
        { "Full Strings", R"(Recipe: three layers so one key plays the whole section. Engine 1: the "String Ensemble" wavetable with 4 unison voices (the violins: several players never perfectly in tune). Engine 2: the same table an octave down with 3 voices (the cellos). The Utility oscillator: a quiet saw two octaves down (the basses).

Then a lowpass, a Formant filter partly blended in (the wooden "body" of the instruments), a little "Breath" noise for the bow, and a vibrato that fades in after 0.8 s.

Try: Macro 3 (Vibrato) for a more romantic sound. Play harder for a faster bow.)" },
        { "Full Strings Marcato", R"(Recipe: Full Strings with a 30 ms attack, a decay to 65 % and Env 2 opening the filter for an instant at each note (more with velocity). That bump is the accent of a heavy bow stroke.

Listen for: each chord "speaks" at the start, then settles.

Try: Macro 4 (Accent) lowers the sustain, so the accent stands out more.)" },
        { "Full Strings Dynamic", R"(Recipe: Full Strings where the mod wheel controls the dynamics: it raises both engines' volumes and opens the filter together. Wheel down = soft and dark (still audible), wheel up = loud and bright. This is how orchestral sample libraries are usually played.

Try: hold a chord and move the wheel slowly up and down: a crescendo and diminuendo with one hand.)" },
        // ----------------------------------------------------------------- sections
        { "Violins Ensemble", R"(Recipe: the string table with 6 unison voices (a big section), a detuned saw on Engine 2 for extra brightness, a highpass at 250 Hz so there's no low end, and a stronger vibrato.

Listen for: violins live above the voice. Without the low end they never get in the way of the band.

Try: Macro 3 (Vibrato) for more expression on long melody notes.)" },
        { "Violins Con Sordino", R"(Recipe: con sordino means "with mute": a clip on the bridge makes violins soft and veiled. Here: a darker wavetable position, a 24 dB lowpass at 1.6 kHz, more bow noise, a slower bow (0.5 s) and a longer release.

Try: compare with Violins Ensemble: same section, muted.)" },
        { "Violins Octaves", R"(Recipe: two violin sections, Engine 2 an octave above Engine 1 and a little quieter, like first and second violins playing the melody in octaves.

Use: big melody lines with one finger.

Try: Macro 3 (Vibrato) for a more emotional line.)" },
        { "Violas Section", R"(Recipe: the string table at a slightly darker position, a lower filter (2.5 kHz) and a darker body vowel. Violas are pitched between violins and cellos and sound warmer than either.

Use: the inner notes of chords, the voice that holds harmony together.)" },
        { "Cellos Section", R"(Recipe: a darker string table position, a quiet detuned saw on Engine 2 for bite, a filter at 2 kHz with key tracking (higher notes get brighter, like a real cello) and a dark body vowel.

Use: play in the low-mid range (around the C below middle C).

Try: Macro 3 (Vibrato) on long notes.)" },
        { "Celli and Basses", R"(Recipe: cellos on Engine 1 and basses an octave lower on Engine 2, through a 24 dB lowpass at 1.2 kHz. One key gives the whole low string section.

Use: left-hand foundation under pads or piano.)" },
        // ----------------------------------------------------------------- articulations
        { "Basses Pizzicato", R"(Recipe: the string table with only two players, a "Pluck Noise" for the finger, a quick filter blip from Env 2 and a 0.7 s decay. It's mono, like a single bass line.

Try: Macro 3 (Thumb) for more pluck noise. Macro 4 (Length) for longer notes.)" },
        { "Pizzicato Section", R"(Recipe: violins plus cellos an octave down, both plucked: no sustain, a 0.45 s decay, a pluck noise and a quick filter blip. The hall gives the short notes a space to ring in.

Try: play light, bouncy rhythms. Macro 3 (Snap) for a sharper pluck.)" },
        { "Staccato Section", R"(Recipe: the full section with a very short envelope (5 ms attack, 0.25 s decay, no sustain), a short filter bump and a touch of pluck noise for the bow biting the string.

Try: Macro 4 (Length) between staccato (short) and spiccato (a little longer).)" },
        { "Tremolo Build", R"(Recipe: the full section with LFO 2 (a fast 11 Hz triangle) dipping both engines' volumes: that's bowed tremolo, the bow moving back and forth very fast. The amp envelope (2.5 s attack) and Env 2 on the filter make it grow from nothing to full.

Try: Macro 3 (Speed) for a faster tremolo. Macro 4 makes the build longer.)" },
        { "Flautando Strings", R"(Recipe: flautando means "like a flute": the bow moves lightly over the fingerboard. Here: the darkest table position, a 24 dB lowpass at 1.4 kHz, a soft sine an octave up, lots of bow air and almost no vibrato.

Use: very quiet moments, under readings or prayer.)" },
        { "String Ostinato", R"(Recipe: the staccato section played by the arpeggiator: up through the held chord in eighth notes, every fourth note accented (louder, and brighter because velocity opens the filter). It follows the host tempo.

Use: hold a chord under a cinematic build.

Try: change the arp order or rate on the SEQ page.)" },
        // ----------------------------------------------------------------- solo
        { "Solo Violin", R"(Recipe: two saws (a bowed string is close to a saw wave) through a 24 dB lowpass and a strong body formant, mono and legato with a 60 ms slide between notes. Vibrato fades in on each new phrase; aftertouch adds more.

Listen for: it's a synth violin. It shines on slow, expressive lines; fast passages give it away.

Try: play legato (overlap the notes) to hear the slides.)" },
        { "Solo Cello", R"(Recipe: a saw and a triangle through a darker 24 dB lowpass and a dark body formant, mono and legato with an 80 ms slide, vibrato that fades in, more with aftertouch.

Use: slow melodies in the lower register, the "cello solo" moment of a ballad.)" },
        // ----------------------------------------------------------------- beds
        { "Chamber Quartet", R"(Recipe: the same layers as Full Strings but only two players on each, less detune, more body and a smaller room. Fewer players = clearer, more intimate, more "real".

Try: compare with Full Strings: the size of a section is mostly the unison count and the room.)" },
        { "Hymn Strings", R"(Recipe: Full Strings made gentle: a darker filter (2.2 kHz), a slow 0.6 s bow, a long 1.5 s release so chords flow into each other, a light vibrato and a big hall.

Try: Macro 3 (Warmth) brings up the cellos for a fuller, warmer chord.)" },
        { "Epic Low Strings", R"(Recipe: the string table an octave down, a saw two octaves down and a sine sub on the Utility oscillator, through a 24 dB lowpass that Env 2 opens over 2 s.

Use: big, dark moments. On a PA with subs it's felt as much as heard.

Try: Macro 3 (Sub) for more sub.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
