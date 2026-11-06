// "How it's built" notes for Pack 8 (shown in the preset browser).

#include "Packs.h"

#include <map>

juce::String pack8Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- bass
        { "Classic Ladder Bass", R"(Recipe: this pack copies the classic three-oscillator mono synth. THREE oscillators (here two saws slightly detuned and a square an octave down), mixed and pushed a little into the 24 dB ladder filter (filter Drive = the original's mixer overdrive). A filter contour (Env 2) and a loudness contour (Env Amp), where the release equals the decay, like the original's "decay" switch.

Here: the filter starts low (300 Hz) and the contour opens it at each note, then settles.

Try: Macro 3 (Contour) for a longer filter decay, Macro 1 for brightness.)" },
        { "Round Soul Bass", R"(Recipe: two triangles (few harmonics, round) with a quieter square an octave down, a very low filter (260 Hz), almost no resonance and a small contour. Extra output level because a triangle is quieter than a saw.

Use: soul, gospel and worship bass that supports without growling.

Try: Macro 3 (Thump) shortens the contour for a more percussive start.)" },
        { "Pedal Bass", R"(Recipe: modelled on the classic bass-pedal synths: everything an octave down (VCO 2 and VCO 3), a very low filter (180 Hz) with some resonance, a slow 1.5 s contour, legato with a little glide.

Use: hold the root under a whole song, or play slow root movements with your left hand. It's mostly sub: it needs a PA with subs.

Try: Macro 3 (Growl) for more resonance.)" },
        { "Funk Pluck Bass", R"(Recipe: a square (40 % pulse) with a little saw and sub, resonance at 0.35, and a short, snappy contour (0.18 s). The decay switch is off, so notes stop quickly when you let go: tight and rhythmic.

Try: Macro 3 (Resonance) for more "pop", Macro 4 for a longer filter decay.)" },
        { "Overdriven Bass", R"(Recipe: the classic bass with the filter Drive pushed to 0.7. On the original, players fed the output back into the mixer to overdrive it; Drive does the same job here: thicker, grittier, still the same level.

Try: Macro 3 (Drive) for even more.)" },
        { "Glide Bass", R"(Recipe: the classic bass, legato with a long glide (0.12 s). Glide only happens when notes overlap, so you choose when to slide.

Try: play a line, then overlap two notes to slide between them.)" },
        { "Squelch Bass", R"(Recipe: one saw and the sub, high resonance (0.6) and a snappy contour, with velocity also opening the filter. The resonance turns the filter sweep into a "talking" squelch.

Try: Macro 3 (Resonance), Macro 4 (Contour). Play with dynamics.)" },
        { "Ladder Sequence 16ths", R"(Recipe: the classic bass played by the sequencer: 16 sixteenth-note steps, with rests on every fourth step and an octave jump on steps 8 and 16. Velocity blend gives the accents.

Use: hold one note; it follows the host tempo.

Try: Macro 4 (Gate) for longer or shorter steps. Edit the steps on the SEQ page.)" },
        // ----------------------------------------------------------------- leads
        { "Classic Ladder Lead", R"(Recipe: three saws, the third one detuned the other way, through the ladder with some resonance, legato with a short glide. The mod wheel adds vibrato, as on the original.

Try: Macro 3 (Resonance). Bring the mod wheel in on long notes.)" },
        { "Portamento Lead", R"(Recipe: a square and a saw with a triangle an octave up, and a long glide (0.18 s) on every note, not just legato ones. That constant slide is the singing, "crying" lead sound of the 70s.

Try: play slow melodies with wide intervals.)" },
        { "Octave Saw Lead", R"(Recipe: saws an octave apart (one up, one down) with the filter Drive at 0.45. Octaves make a lead big without making it a chord.

Try: Macro 3 (Drive).)" },
        { "Flute Lead", R"(Recipe: triangles (one an octave up), a little noise for breath, no drive, a soft attack and a vibrato that fades in after 0.6 s.

Try: Macro 3 (Breath) for a breathier flute.)" },
        { "Funky Square Lead", R"(Recipe: two pulse waves (35 % width), a resonant filter snapped open by a short contour, decay switch off for tight notes, and a short eighth-note echo.

Try: Macro 3 (Resonance) for more quack.)" },
        { "Trill Lead", R"(Recipe: LFO 1 is a square wave at 9 Hz, routed to the pitch by one semitone, but only as far as the mod wheel allows. Wheel up = the note trills rapidly with the note above it.

Try: Macro 3 (Trill Speed). Use the wheel on held notes.)" },
        { "Screaming Lead", R"(Recipe: saws with a fifth on Osc 3, high resonance (0.5) and heavy drive (0.6). Drive plus resonance makes the filter scream on its harmonics.

Try: Macro 3 (Scream) for more resonance. Mod wheel for vibrato.)" },
        { "Soft Mono Lead", R"(Recipe: a triangle with a quiet saw and a triangle an octave up, a low filter (700 Hz), no drive, a soft attack and a delayed vibrato.

Use: gentle melodies over worship. Try: Macro 1 for brightness.)" },
        // ----------------------------------------------------------------- poly
        { "Ladder Poly Brass", R"(Recipe: the ladder recipe played polyphonically, like the big American polysynth: two saws with drift and drive, a filter contour with a 50 ms attack.

Listen for: compare with the Grand Poly brass: the ladder filter is thicker and rounder in the low mids.

Try: Macro 3 (Bite) for a shorter contour.)" },
        { "Ladder Poly Pad", R"(Recipe: a saw and a pulse with lots of drift, a low ladder filter that the contour opens slowly (1.2 s), a slow attack and long release.

Try: Macro 3 (Swell) for a slower filter opening.)" },
        { "Ensemble Poly Strings", R"(Recipe: two detuned saws, filter open, a highpass at 250 Hz and a slow phaser: the 70s polyphonic string sound, swirling and bright.

Try: Macro 3 (Phase) for more phaser.)" },
        { "Ladder Pluck Keys", R"(Recipe: a saw and a pulse, resonance 0.3, a contour with no sustain (0.3 s), a 0.9 s decay. Woody and round.

Try: Macro 3 (Resonance). Good with the arpeggiator.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
