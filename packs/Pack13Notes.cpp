// "How it's built" notes for Pack 13 (shown in the preset browser).
// Timbres only: our own recipes, no copied patches, no riffs.

#include "Packs.h"

#include <map>

juce::String pack13Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- brass and stabs
        { "Countdown Brass", R"(Recipe: saws with three unison voices plus a square an octave up, a fairly open filter that a quick envelope pushes further, and Vintage Chorus II. The octave-up square is what makes it shine.

Try: Macro 3 (Bite) for shorter, punchier stabs.)" },
        { "Jump Poly", R"(Recipe: two saws detuned by 10 cents with analog drift, a bright filter with a snappy envelope, no chorus. Big 80s polysynths got their width from detuned oscillators, not effects.

Try: play wide, open chords and hit them hard.)" },
        { "Separate Brass", R"(Recipe: a saw and a 40 % pulse with filter drive (0.35) and a very fast attack. The drive adds the edge of an amp-driven rock keyboard.

Try: Macro 3 (Drive).)" },
        { "Runaway Poly", R"(Recipe: a pulse and a saw through a resonant filter (0.3) that bites at the start and settles to half, with chorus I.

Try: Macro 3 (Resonance) for more bite.)" },
        { "Party Stab 99", R"(Recipe: two detuned saws with drift and a short, punchy filter envelope (0.25 s) that settles low. Short, fat, funky.

Try: play short chords on the off-beats.)" },
        { "Want Me Brass", R"(Recipe: a saw and a narrow pulse, little drift (cold and precise), and a highpass at 300 Hz that takes the warmth away. Early synth-pop was thin on purpose.

Try: compare with Jump Poly: same family, opposite temperature.)" },
        { "Chariot Brass", R"(Recipe: two detuned saws through a warm, resonant 24 dB filter with a slow 0.3 s opening, a soft attack, a long release and a vast hall. Aftertouch (or Macro 3) opens the filter and adds vibrato: the famous "growl" of the two-layer flagship synth.

Try: hold a chord and press into it.)" },
        { "Replicant Brass", R"(Recipe: the same brass, darker (400 Hz), with a 2.5 s release, a slow vibrato that fades in after a second, and an even bigger hall. Long tails are what make it sound like rain at night.

Try: Macro 3 (Growl) for more presence.)" },
        { "Orchestra Hit 80s", R"(Recipe: Chord unison set to Fifth + Octave (one key = root, fifth and octave), a sub, a burst of noise and a short decay into a hall. The famous sampler orchestra hit, made from synth parts.

Try: Macro 3 (Noise) for more "crash".)" },
        // ----------------------------------------------------------------- leads
        { "Take On Lead", R"(Recipe: a square and a saw, a quick filter blip, a decay to a 60 % sustain and Vintage Chorus II. Short notes sound plucky, long ones sing.

Try: Macro 3 (Chorus).)" },
        { "Axel Lead", R"(Recipe: a square with a little saw, resonance and a snappy filter envelope, legato with a tiny glide and a short eighth echo. Punchy and bouncy.

Try: Macro 3 (Resonance).)" },
        { "Vice Lead", R"(Recipe: two detuned saws, legato with glide, a 5-semitone pitch bend, a delayed vibrato, a dotted echo and a big hall. Made for bending.

Try: bend up into long notes.)" },
        { "G-Funk Whistle", R"(Recipe: a pure sine (with a whisper of triangle), transposed up an octave, with a long portamento on every note and a vibrato that fades in. Mostly glide and vibrato: the tone is simple.

Try: Macro 3 (Vibrato) for a faster wobble.)" },
        { "Talk Box Lead", R"(Recipe: saws through a Formant (vowel) filter. Env 3 opens the vowel at each note ("wah") and the mod wheel moves it too, like a mouth shaping a guitar or synth through a tube.

Try: ride the mod wheel while you play.)" },
        { "Popcorn Pluck", R"(Recipe: a square through a resonant Ladder filter (0.45) snapped open by a 0.15 s envelope with no sustain. The resonance makes each note "pop" like a bubble.

Try: Macro 3 (Resonance).)" },
        { "Enough Lead", R"(Recipe: two pulses an octave apart, a quick filter blip and a short decay to half sustain. Bright and bouncy, with a short echo.

Try: Macro 3 (Octave).)" },
        { "Sandstorm Lead", R"(Recipe: very detuned saws with unison, the filter open, short notes (0.08 s release) so fast repeated notes stay crisp. Buzzy and relentless.

Try: Macro 3 (Width).)" },
        { "Levels Lead", R"(Recipe: a 7-voice supersaw with a small pitch drop into each note (Env 3 starts it a semitone high), a dotted echo and reverb.

Try: Macro 3 (Width).)" },
        { "Animal Drop", R"(Recipe: a pulse and a very detuned saw with a sub, filter drive and an Overdrive on bus A. Hollow, loud and simple: drops are about one huge note.

Try: Macro 3 (Drive).)" },
        { "Town Riff Lead", R"(Recipe: a saw and a narrow pulse through a resonant filter (0.35) that snaps open and settles low. Short notes pop; the resonance gives the disco "quack".

Try: Macro 3 (Resonance).)" },
        { "Robot Love Lead", R"(Recipe: a square and a triangle an octave up, no drift at all (perfectly stable = robotic), a fairly open filter and a plain (not ping-pong) echo.

Try: Macro 3 (Octave).)" },
        { "Sawyer Growl", R"(Recipe: saws with a sub through a resonant, driven Ladder filter. The filter envelope opens it and then sweeps slowly down over 1.2 s on every note: the resonance turns that sweep into a growl.

Try: Macro 3 (Growl), Macro 4 (Sweep Time).)" },
        // ----------------------------------------------------------------- keys, bells, plucks
        { "Mirror Bells", R"(Recipe: two FM pairs. Engine 1: a soft 1:1 body with velocity bark. Engine 2: a bell pair with a 3.5 ratio (inharmonic) that rings for 1.2 s. Chorus and a hall.

Try: Macro 3 (Bell) for a brighter bell.)" },
        { "Tainted Bink", R"(Recipe: a square with an 80 ms decay and a tiny pitch drop at the start (Env 3, two semitones in 40 ms). It's almost all attack: a "bink".

Try: play it high, on repeated notes.)" },
        { "Rains Pluck", R"(Recipe: FM with a 5:1 modulator that only sounds for 0.1 s, then a pure tone that decays in under a second. Like a kalimba.

Try: Macro 3 (Tink).)" },
        { "Rule the World Keys", R"(Recipe: a saw with a narrow pulse an octave up, a struck filter envelope and a long decay to a low sustain, Vintage Chorus II and a dotted-eighth echo. The shimmer comes from the chorus plus the echo.

Try: Macro 3 (Echo).)" },
        { "Riley Organ Arp", R"(Recipe: an organ tone (a sine with a square an octave up and a quiet partial at the octave-and-fifth) with a short, percussive envelope, played by the arpeggiator in sixteenths in the order you play the notes. Hold a chord and it repeats it like the old home-organ "repeat" effect.

Try: Macro 4 (Decay) for longer notes.)" },
        { "Insomnia Pluck", R"(Recipe: detuned saws through a resonant filter (0.3) snapped open with no sustain, into a long dotted echo with high feedback.

Try: Macro 3 (Resonance).)" },
        { "Dream Piano 96", R"(Recipe: a synth-built piano (a triangle, a sine an octave up and a hint of saw, a felt "thump" and a velocity filter) into a huge hall. It's a synth piano, not a sampled one: the reverb does most of the dreaming.

Try: Macro 3 (Hammer).)" },
        { "Strobe Pluck", R"(Recipe: soft saws through a resonant filter with a slow-ish pluck (0.4 s), into a long dotted-quarter echo with high feedback and a big hall. Patient and spacious.

Try: Macro 3 (Resonance).)" },
        { "Beat Gong", R"(Recipe: a sine ring-modulated by a sine at 1.41x (an inharmonic ratio), an octave down, with Env 2 adding extra ring-mod at the strike, a "Thump" noise and a 6-second decay into a big hall.

Try: play a single low note.)" },
        // ----------------------------------------------------------------- pads and strings
        { "Tonight Air Pad", R"(Recipe: a saw and a pulse through a warm, resonant filter, a slow attack and a very slow LFO breathing the filter. Dark and patient.

Try: Macro 3 (Motion).)" },
        { "Oxygen Strings", R"(Recipe: two saws an octave apart with unison, the filter open, a highpass at 250 Hz and a deep, slow phaser (mix 55 %). The phaser is the sound.

Try: Macro 3 (Phase).)" },
        { "Silence Pad", R"(Recipe: two detuned saws with unison, a low filter that opens slowly, chorus I and a hall. Melancholy comes from darkness plus movement.

Try: Macro 3 (Chorus).)" },
        { "Cars Vox Pad", R"(Recipe: a narrow pulse with a saw an octave up, through a partly blended Formant filter (a vocal colour) and a phaser. Cold, buzzy and slightly human.

Try: Macro 3 (Vox) for more vowel.)" },
        { "Detroit Strings", R"(Recipe: bright detuned saws with a highpass, Vintage Chorus I + II and a short release: urgent chords that stop when you lift.

Try: Macro 3 (Thin).)" },
        { "Billie Pad", R"(Recipe: a square and a saw, a soft filter swell and chorus I, in a small room. Quiet, warm, it supports a groove without taking space.

Try: Macro 3 (Chorus).)" },
        { "Vienna Strings", R"(Recipe: two detuned saws with unison, a slow 0.6 s attack, a long release, a gentle phaser and a large hall. Grand and dramatic.

Try: Macro 3 (Phase).)" },
        // ----------------------------------------------------------------- bass and sequences
        { "Monday Bass", R"(Recipe: a pulse and a saw with a sub, a punchy filter blip, played by the sequencer in eighths with octave jumps on two steps. Hold one note.

Try: change the pattern on the SEQ page.)" },
        { "Sweet Dream Seq", R"(Recipe: a saw and a square with a sub, resonance 0.3, in a sixteenth pattern with rests and two octave jumps, a touch of chorus. Smooth and hypnotic.

Try: Macro 3 (Resonance).)" },
        { "Feel Love Seq", R"(Recipe: a Ladder bass in straight sixteenths with every fourth step an octave up, plus a dotted echo that thickens the pulse. The late-70s modular sequencer sound.

Try: Macro 3 (Echo).)" },
        { "Thrill Bass", R"(Recipe: a saw, a square and a sub through the Ladder filter with a punchy envelope, velocity-sensitive. Played by hand: tight, syncopated lines.

Try: Macro 3 (Punch).)" },
        { "Jean Bass", R"(Recipe: a square with a sub through the Ladder filter, a short filter blip and a short decay. Dry and steady: no effects at all.

Try: Macro 3 (Punch).)" },
        { "Chameleon Bass", R"(Recipe: a saw and a square through a warm 24 dB filter with high resonance (0.55) and a strong envelope, legato with a short glide, velocity opening the filter. Squelchy jazz-funk.

Try: Macro 3 (Squelch).)" },
        { "Boogie Moog Bass", R"(Recipe: saws and a sub through a slightly driven Ladder filter with a 30 ms envelope attack: each note goes "wow" like a wah. Legato with glide.

Try: Macro 3 (Wah).)" },
        { "Acid Line", R"(Recipe: one saw through the Ladder filter with very high resonance (0.7) and drive, played by the sequencer with ACCENTS (louder, and brighter because velocity opens the filter) and SLIDES (glide into the next note). That's the whole acid sound.

Try: edit accents (velocity) and slides on the SEQ page.)" },
        { "FM Slap Bass 80s", R"(Recipe: a 1:1 FM pair with a strong, fast burst of modulation at the strike (Env 2, 0.25 s), plus a sine an octave down for weight. The burst is the "slap".

Try: Macro 3 (Slap).)" },
        // ----------------------------------------------------------------- arps and special
        { "Rio Arp", R"(Recipe: a saw and a pulse with a quick pluck, arpeggiated up and down three octaves in sixteenths, with a soft echo.

Try: hold four-note chords.)" },
        { "Upside Arp", R"(Recipe: a resonant, slightly driven Ladder saw arpeggiated up two octaves in eighths, a very slow filter wave and a big, dark hall.

Try: Macro 3 (Resonance).)" },
        { "Hoover Rave", R"(Recipe: a saw and a PWM pulse with four detuned unison voices, chorus I + II, and a pitch swoop DOWN into every note (Env 3 starts it 3 semitones high). Legato with a long glide.

Try: Macro 3 (Swoop) for a longer swoop.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
