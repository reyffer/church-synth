// "How it's built" notes for Pack 11 (shown in the preset browser).

#include "Packs.h"

#include <map>

juce::String pack11Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- bass
        { "Outrun Bass", R"(Recipe: two saws and a square sub through the smooth OTA filter, with a short filter envelope, played by the sequencer in straight sixteenths. Every note gets the same little "blip" of brightness: that's the engine of synthwave.

Use: hold the root of each chord; it follows the host tempo.

Try: Macro 3 (Punch) for a longer filter blip, Macro 4 (Gate) for longer notes.)" },
        { "Octave Runner Bass", R"(Recipe: the outrun bass with every second step jumped up an octave (the Octave lane on the SEQ page). Low-high-low-high in sixteenths: instant energy.

Try: Macro 3 (Resonance) for more squelch.)" },
        { "Galloping Bass", R"(Recipe: a 16-step pattern with a rest on every fourth step: "one-and-a, two-and-a". The gaps are what makes it gallop.

Try: edit which steps rest on the SEQ page to invent your own rhythm.)" },
        { "Analog Saw Bass", R"(Recipe: saws and a sub through the Ladder filter with some drive, played by hand (no sequencer). Fatter and rounder than the OTA basses.

Try: Macro 3 (Drive).)" },
        { "Sub Pulse Bass", R"(Recipe: a pulse and a loud sub in eighth notes with long gates (80 %). Mostly sub: it pushes air under pads.

Try: Macro 3 (Sub) on big systems.)" },
        { "Dark Synth Bass", R"(Recipe: a saw and a saw an octave down through a driven, resonant ladder filter in a broken sixteenth rhythm, with a very slow LFO moving the cutoff over several bars.

Listen for: the slow filter wave builds tension without anyone touching a knob.

Try: Macro 3 (Drive).)" },
        { "Funk Bass 80s", R"(Recipe: a 35 % pulse with saw and sub, resonance 0.35 and a snappy filter envelope, velocity-sensitive. Played by hand.

Try: Macro 3 (Resonance) for more "pop".)" },
        { "Triplet Bass", R"(Recipe: the outrun bass in sixteenth-note triplets (12 steps per bar), with an octave jump on every third step. Triplets against a straight beat feel cinematic and rolling.

Try: Macro 3 (Punch).)" },
        // ----------------------------------------------------------------- sequences and arps
        { "Night City Arp", R"(Recipe: a saw and a PWM pulse, a short filter blip, the arpeggiator going up two octaves in sixteenths, Vintage Chorus II and a dotted-eighth ping-pong echo.

Listen for: the dotted echo lands between the sixteenths and makes the pattern sound twice as busy.

Try: Macro 3 (Echo).)" },
        { "Neon Up Arp", R"(Recipe: brighter saws, arpeggiated up three octaves in sixteenths, chorus and echo.

Try: change the arp order on the SEQ page (Up & Down sounds calmer).)" },
        { "Arpeggio Bells", R"(Recipe: an FM bell (ratio 3.5, inharmonic) arpeggiated up and down in eighths, with a long dotted echo.

Try: Macro 3 (Metal) for brighter bells.)" },
        { "Pluck Sequence 16ths", R"(Recipe: a pluck played by the sequencer (not the arpeggiator): a fixed 16-step rhythm with rests and octave jumps. Hold ONE note and it plays the riff on that note.

Try: hold different notes to follow the chords.)" },
        { "Gated Chord Seq", R"(Recipe: a sustained pad whose volume is chopped by Function 1 (the "Trance Gate" shape over one bar), synced to the tempo. Hold a chord and it becomes a rhythm.

Try: Macro 3 (Gate Depth) turns the chopping down towards a plain pad.)" },
        { "Octave Arp 8ths", R"(Recipe: a warm square pluck arpeggiated up two octaves in eighths, chorus I, a dotted-quarter echo. Slower arps leave space for vocals.

Try: Macro 3 (Echo).)" },
        { "Triplet Arp", R"(Recipe: arpeggio in sixteenth triplets, up and down two octaves. Triplets against straight drums give a swirling, cinematic motion.

Try: Macro 3 (Echo).)" },
        { "Dark Arp", R"(Recipe: a driven, resonant ladder pluck arpeggiated DOWN in sixteenths, with a very slow LFO on the cutoff. Downward arps feel darker than upward ones.

Try: Macro 3 (Resonance).)" },
        { "Shimmer Arp", R"(Recipe: a soft triangle with a narrow pulse an octave up, arpeggiated up and down three octaves in sixteenths, into a dotted echo and the shimmer reverb.

Use: quiet glittering motion behind a verse.)" },
        { "Pulse Pad Seq", R"(Recipe: a unison saw pad whose volume follows Function 1's "Duck" shape every beat: the side-chain pump that's everywhere in synthwave, without needing a kick drum.

Try: Macro 3 (Pump) for a deeper pump.)" },
        // ----------------------------------------------------------------- plucks
        { "Neon Pluck", R"(Recipe: two saws, a resonant filter snapped open by a 0.2 s envelope with no sustain, Vintage Chorus II and a dotted-eighth echo. The default synthwave pluck.

Try: Macro 3 (Echo).)" },
        { "Glass Pluck 80s", R"(Recipe: FM with a 4:1 modulator that only sounds at the strike (Env 2 decays in 0.15 s), so you hear a bright "tink" and then a pure tone. Chorus and echo.

Try: Macro 3 (Tink).)" },
        { "Retro Pluck", R"(Recipe: a square-and-saw pluck through a Tape Echo with wow and flutter: each echo is slightly out of tune, like an old tape delay.

Try: Macro 3 (Wobble) for more tape movement.)" },
        { "Echo Pluck", R"(Recipe: a very short pluck into a dotted-eighth echo with high feedback (50 %) and a loud mix. Play quarter notes: the echo fills in the sixteenths.

Try: Macro 3 (Echo).)" },
        { "Square Pluck", R"(Recipe: a square wave (hollow: odd harmonics only) with a quick filter pluck, chorus I and an echo.

Try: Macro 3 (Width) narrows the pulse: from hollow square to thin and nasal.)" },
        { "Chorus Pluck", R"(Recipe: a soft saw-and-pulse pluck drenched in Vintage Chorus I + II at 55 % mix.

Try: Macro 3 (Chorus).)" },
        { "Muted Pluck", R"(Recipe: a very short envelope (0.12 s) and filter blip: tight and percussive, for fast rhythmic parts.

Try: Macro 4 (Length).)" },
        { "Synth Marimba", R"(Recipe: FM with a 4:1 modulator that sounds only for 80 ms, and a short 0.5 s decay. The 80s digital marimba.

Try: Macro 3 (Mallet).)" },
        // ----------------------------------------------------------------- pads
        { "Sunset Pad", R"(Recipe: a 7-voice Super unison saw (very wide), a slow filter swell, chorus I and a big hall.

Try: Macro 3 (Width) for more detune.)" },
        { "Midnight Highway Pad", R"(Recipe: saw, pulse and sub with a dark, resonant filter, a slow filter swell, and an LFO gently moving the cutoff.

Try: Macro 3 (Motion) changes the LFO speed.)" },
        { "Neon Haze", R"(Recipe: two pulses whose widths move in opposite directions (PWM from one LFO), through a Tape Echo with a lot of wobble.

Try: Macro 3 (PWM Speed).)" },
        { "Retro Strings", R"(Recipe: two detuned saws with two unison voices each, filter wide open, a highpass at 300 Hz and Vintage Chorus I + II. 80s polysynth strings.

Try: Macro 3 (Thin) moves the highpass higher.)" },
        { "VHS Pad", R"(Recipe: a saw pad with an LFO wobbling the pitch slightly (like a stretched tape), tape hiss, a dark filter and a wobbly Tape Echo.

Try: Macro 3 (Hiss).)" },
        { "Analog Dream", R"(Recipe: a saw with a triangle an octave up, lots of drift, a slow filter swell and chorus. Gentle enough for worship.

Try: Macro 3 (Swell).)" },
        { "Starfield Pad", R"(Recipe: the "Glass Pad" wavetable moved slowly by an LFO, a "Sparkle Air" noise on top, a dotted echo and shimmer.

Try: Macro 3 (Sparkle).)" },
        { "Choir Haze", R"(Recipe: the "Choir Ah to Oo" and "Breathy Vox" wavetables behind Vintage Chorus II and a long hall.

Try: Macro 3 (Vowel) moves towards "oo".)" },
        { "Warm Saw Bed", R"(Recipe: two saws with a little sub through a low filter (700 Hz), chorus and a hall. Deliberately simple: it fills without competing.

Try: Macro 3 (Sub).)" },
        { "Ocean Drive Pad", R"(Recipe: saws through a resonant filter (0.4) swept up and down by a slow LFO (one cycle every 10 s), with chorus I + II. The resonance makes each sweep sound like a wave.

Try: Macro 3 (Wave Speed), Macro 4 (Resonance).)" },
        // ----------------------------------------------------------------- leads
        { "Hero Lead", R"(Recipe: a 7-voice Super unison saw, legato with glide, vibrato on the mod wheel, a dotted echo and a hall. The big chorus-melody lead.

Try: Macro 3 (Width).)" },
        { "Sync Lead 84", R"(Recipe: hard sync: Osc 2 (a fifth up) locked to Osc 1, its pitch swept by Env 2 and by the mod wheel. The tone tears and screams without the pitch changing.

Try: ride the mod wheel on held notes.)" },
        { "Square Lead 80s", R"(Recipe: two squares an octave apart, legato with a little glide, a delayed vibrato and chorus I.

Try: Macro 3 (Octave) for more of the upper square.)" },
        { "Brass Lead 80s", R"(Recipe: two saws with a brass-style filter swell on every note (60 ms attack), a short glide, vibrato on the wheel.

Try: Macro 3 (Bite).)" },
        { "Whistle Lead", R"(Recipe: a sine with a quiet triangle an octave up, a long portamento (0.15 s) on every note, a delayed vibrato and a long echo.

Try: Macro 3 (Breath) adds a little noise.)" },
        { "Chorus Saw Lead", R"(Recipe: one saw (plus a detuned one) through deep Vintage Chorus I + II: smooth and wide without unison.

Try: Macro 3 (Chorus).)" },
        { "Octave Lead 80s", R"(Recipe: saws an octave apart with a sub, through a driven ladder filter. Bold and cutting.

Try: Macro 3 (Drive).)" },
        { "Talking Lead", R"(Recipe: saws through a Formant (vowel) filter. Env 3 moves the vowel at each note, and the mod wheel moves it too: the lead "says" a vowel, like a talk box.

Try: Macro 3 (Vowel), or ride the mod wheel.)" },
        // ----------------------------------------------------------------- keys and stabs
        { "FM Keys 84", R"(Recipe: the 80s FM electric piano: a 1:1 FM body with velocity bark (Engine 1) plus a 14:1 tine pair that fades in 0.3 s (Engine 2), slightly detuned. Vintage Chorus II and an echo.

Try: Macro 3 (Bark).)" },
        { "Synth Brass Stabs", R"(Recipe: two saws with unison, a fast filter envelope that settles to 35 %, chorus I.

Try: Macro 3 (Bite) for shorter stabs.)" },
        { "Neon Chord Stab", R"(Recipe: Chord-mode unison (minor 7): one key plays a whole chord. A short stab envelope, Chorus II and an echo.

Use: play the melody of the stab with one finger; it becomes a chord line.)" },
        { "Retro EP", R"(Recipe: a tine electric piano (1:1 FM body with velocity bark, 14:1 tine) through a wide stereo auto-pan and chorus.

Try: Macro 3 (Pan).)" },
        { "Poly Keys 80s", R"(Recipe: a pulse with slow PWM and a little saw, a decaying envelope to a 40 % sustain, Vintage Chorus II.

Try: Macro 3 (Chorus).)" },
        { "Neon Keys", R"(Recipe: a soft saw with a very narrow pulse an octave up: the pulse adds a bell-like edge. Chorus and a soft echo.

Try: Macro 3 (Bell) for more of the upper pulse.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
