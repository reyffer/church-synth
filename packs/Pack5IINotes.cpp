// "How it's built" notes for Pack 5 II (Brass on the Section engine).

#include "Packs.h"

#include <map>

juce::String pack5IINotes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        { "RnB Brass Stab II", R"(Recipe: the Section engine in its Brass family: four players through the Trumpet bell. Attack high, a short Blat (a bright burst at the start of loud notes) and a small Scoop (each note starts a little flat and rises into pitch). The Env Amp decays to a lower sustain for a stab.

How brass gets brighter: in the Brass family, Dynamics opens each player's tone much faster than it raises the level, and a soft saturator adds upper harmonics. Velocity adds to Dynamics, so hard notes are not just louder but much brighter, like a real horn player blowing harder.

Try: Macro 4 (Blat). Play the same chord at velocity 60 and 120.)" },
        { "Velvet RnB Brass II", R"(Recipe: six players on the Horn bell (darker than the trumpet), soft Dynamics, a slow Attack and a little scoop, chorus and hall. The mod wheel swells the held chord.

Use: 90s R&B ballads, under a vocal. Swell into the chord with the wheel instead of hitting the keys.

Try: Macro 4 (Scoop) for more of the "lean" into each note.)" },
        { "Soul Brass Swell II", R"(Recipe: trumpets (left) and trombones an octave down (right). Dynamics starts low (0.15) and Env 2, with a 1.8 s attack, raises it while you hold: the section crescendos, getting much brighter as it gets louder.

Lesson: that brightness change is the bloom of real brass. A synth brass filter sweep imitates it; here it comes from Dynamics.

Try: Macro 4 (Swell Time).)" },
        { "Funk Brass Hits II", R"(Recipe: trumpets and trombones, Attack at the top (sforzando: the level overshoots and drops back), strong Blat, a tiny scoop, no vibrato, a very short Env Amp. Looseness is low so the hits are tight.

Use: funk and gospel punches, short and on the beat.

Try: Macro 4 (Length) for slightly longer hits.)" },
        { "Bright Synth Brass II", R"(Recipe: six players, Brightness high, Dynamics high, less body (0.45) so the raw players come through, and chorus: the 80s synth-brass sound, but with the movement of independent players.

Try: Macro 4 (Body) adds the trumpet bell back for a more acoustic sound.)" },
        { "Octave Synth Brass II", R"(Recipe: four trumpets on Engine 1, four trombones an octave below on Engine 2: one hand plays a full horn line in octaves. A small scoop on both.

Try: lower Engine 2's volume for a lighter doubling.)" },
        { "Gospel Brass II", R"(Recipe: five trumpets and three trombones, a strong Scoop (0.55), Blat and a high Attack. Velocity adds Dynamics, so hard hits are bright and blaring.

Use: gospel shouts and hits. Lean into the keys.

Try: Macro 4 (Scoop) more or less.)" },
        { "Sharp Brass Stab II", R"(Recipe: four trumpets with Brightness, Attack and Blat at maximum: the stab cuts through any band. No vibrato, short decay.

Try: Macro 4 takes the blat back for a cleaner, still bright stab.)" },
        { "Brass Fall II", R"(Recipe: trumpets tuned 5 semitones down (Coarse −5). Env 3 lifts the pitch back up straight away and holds it while the key is down; when you let go, Env 3 releases over 0.4 s, so the pitch falls: the big-band fall.

Lesson: an envelope on pitch can work "backwards": up while held, falling on release.

Try: Macro 4 (Fall) for a longer, slower fall.)" },
        { "Brass Sfz Swell II", R"(Recipe: trumpets and horns, Dynamics starting low. Two envelopes push Dynamics: Env 2 (a fast hit that dies in 0.35 s) and Env 3 (a slow 2.5 s swell). Together: hit hard, drop back, then crescendo, the dramatic sforzando-crescendo of film brass.

Try: Macro 4 (Swell Time).)" },
        { "Staccato Brass II", R"(Recipe: trumpets and trombones, very short Env Amp, a high Attack and some Blat, no vibrato, low Looseness. Velocity adds Dynamics for accents.

Try: Macro 4 (Length).)" },
        { "Brass Blast II", R"(Recipe: the biggest section: eight trumpets and six trombones, Dynamics near the top, strong attack and blat, and a light Distortion (FX A1) for the slightly overdriven blare of a full brass section at maximum.

Use: final chords and big endings. Watch your level.

Try: Macro 4 (Grit) for more distortion.)" },
        { "Trumpet Section II", R"(Recipe: four trumpets on the Trumpet bell, a scoop of 0.3 and more breath Noise than the stabs. Velocity and the mod wheel add Dynamics.

Lesson: the Trumpet bell has a broad peak around 1.3 kHz: the "ring" of a trumpet. Compare with the Horn bell, whose peak is much lower.)" },
        { "Trombone Section II", R"(Recipe: four players on the Trombone bell, seated right. Play from C2 to C4. Blat gives the buzzy edge on loud notes.

Try: Macro 4 (Blat). Play soft and loud to hear the brass bloom.)" },
        { "Horn Section II", R"(Recipe: three trumpets (left) and three trombones an octave down (right), the soul horn section in one patch. A scoop and some blat on both.

Use: soul, Motown, gospel lines and stabs. Play in octaves or close chords.)" },
        { "French Horns II", R"(Recipe: four players on the Horn bell, which faces backwards on a real horn: dark and round, with a soft Attack. A bigger hall.

Use: film moments, hymns, noble chords. Swell with the mod wheel.

Try: Macro 4 (Attack) for a firmer entry.)" },
        { "French Horns Soft II", R"(Recipe: six horns, soft Dynamics (0.25), a very soft attack, a long release and a big hall: a warm bed under worship. The mod wheel brings them up.)" },
        { "Solo Trumpet II", R"(Recipe: one player, Mono Legato with a short glide: legato notes slur (a quick dip in brightness and level) instead of re-attacking. A scoop on each new note, breath noise, vibrato that fades in, aftertouch for more. Velocity and the mod wheel add Dynamics.

Lesson: play phrases legato and only lift between phrases, like a trumpeter breathing.)" },
        { "Muted Trumpet II", R"(Recipe: one player through the Muted bell: a strong nasal peak around 1.7 kHz with the lows cut, like a mute in the bell. Mono Legato, a little scoop, breath noise.

Use: late-night jazz phrases, soft and close.)" },
        { "Flugelhorn II", R"(Recipe: one player through the Horn bell (dark), Brightness lower and lots of breath: the flugelhorn's soft, round tone. Mono Legato with a slightly slower glide.

Use: ballads, intros. Try Macro 4 (Air) for more breath.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
