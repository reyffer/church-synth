// "How it's built" notes for Pack 1 (shown in the preset browser).

#include "Packs.h"

#include <map>

juce::String pack1Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- electric pianos
        { "Tine Piano", R"(Recipe: FM, the way the classic digital electric pianos work. Engine 1 is a sine phase-modulated by another sine at the same pitch (Wavetable engine, Phase Mod): that's the body. Env 2 adds extra modulation at the start of each note, scaled by velocity, so hard notes "bark".

Engine 2 is the tine: a sine modulated at 14x its pitch, heard only for a moment (Env 3 decays in 0.2 s). That bright ping on top is what makes it sound like a tine piano.

Try: Macro 1 (Bark). Play very softly, then very hard, and listen to how the tone changes, not just the volume.)" },
        { "Suitcase Keys", R"(Recipe: an Analog-engine take on the tine piano: a sine with a little FM from Osc 3 (same pitch), plus a quiet sine an octave up. Env 2 makes the FM bark short and velocity-sensitive.

The character comes from the effects: a Panner with a lot of depth (the famous stereo "suitcase" vibrato), and a touch of Tube distortion for amp grit.

Try: Macro 3 changes the pan depth, Macro 4 the grit. With MONO on, the panner becomes a volume tremolo.)" },
        { "Reed Piano", R"(Recipe: a triangle plus a narrow pulse (28 % width), which gives the reedy, nasal tone. Env 2 opens the filter at each note (more with velocity), and an Overdrive adds bite when you play hard.

The tremolo is amplitude, not panning: LFO 1 (5.6 Hz) drives the amp, like the built-in tremolo of a reed electric piano.

Try: Macro 3 sets the tremolo depth. Play hard to hear the overdrive "growl".)" },
        { "Eighties Tines", R"(Recipe: the same two-part FM piano as Tine Piano (body + tine at 14x) but pushed brighter: more base phase modulation, a louder and longer tine, the filter fully open, then chorus and a dotted-eighth echo.

That's the 80s ballad piano: bright, glassy, chorused.

Try: compare it with Tine Piano. Same recipe, different amounts, a completely different era.)" },
        { "Dreamy EP", R"(Recipe: a soft FM piano (little modulation, so it's round), a long 5 s decay, then three effects in a row: Vintage Chorus II, a dotted-quarter ping-pong echo and the shimmer reverb.

Most of the sound is effects. The piano itself is simple.

Try: switch the FX off with the FX power button to hear the dry piano, then back on.)" },
        { "Electric Grand", R"(Recipe: a saw (for the bright hammer) and a triangle an octave up, through a 24 dB lowpass with strong key tracking, so high notes stay bright. Env 2 and velocity open the filter at each strike, the "Key Click" noise adds the mechanism, and a light chorus adds the slightly detuned shimmer of an electric grand.

Try: Macro 3 sets the hammer click.)" },
        { "Felt Piano", R"(Recipe: a triangle with a quiet sine an octave up, 2-voice unison barely detuned (a piano's strings are never perfectly in tune), and the "Thump" noise for the felt hammer. A 12 dB lowpass at 900 Hz keeps it soft; velocity opens it.

No sustain: notes decay to silence over about 4 s, like a real piano.

Try: Macro 3 (Hammer) up for more felt thump. Play very softly: that's where it's most beautiful.)" },
        { "Felt Lullaby", R"(Recipe: a sine with a triangle an octave up and a faint 19-semitone overtone, "Soft Click" for the hammer, a little "Tape Hiss", a very dark filter (700 Hz) and a Tape Echo with wobble.

The hiss and wobble make it feel like an old recording.

Try: Macro 3 turns the hiss up or down.)" },
        { "Felt Upright", R"(Recipe: like Felt Piano but brighter: a triangle plus a detuned saw (more harmonics), a 24 dB filter at 1.2 kHz, and both "Thump" and "Key Click".

Brighter sounds sit better in a band; softer ones work alone.

Try: A/B it against Felt Piano in a band mix.)" },
        { "Keys and Pad", R"(Recipe: an FM piano on Engine 1 plus a unison saw pad on Engine 2. The pad's volume is controlled by Env 3, which has a slow 1.8 s attack: when you hold a chord, the pad fades in under the piano.

The two engines go to separate filters (parallel routing), so the pad can be darker than the piano.

Try: Macro 3 sets how loud the pad gets. Tap short chords (piano only) vs hold (piano then pad).)" },
        // ----------------------------------------------------------------- strings
        { "Chamber Strings", R"(Recipe: the "String Ensemble" wavetable with 6 unison voices, plus a unison saw for body. A 12 dB lowpass (darker = more distant) and a highpass to clear the low mud. Attack 0.35 s, like a bow.

The mod wheel adds vibrato (LFO 1 moving the pitch of both engines). Velocity opens the filter.

Try: hold a chord and slowly bring in the mod wheel: that's the moment strings come alive.)" },
        { "Ensemble Strings", R"(Recipe: the 70s string machine: two engines of saws an octave apart, a 12 dB lowpass, a highpass at 220 Hz (string machines are thin and bright, not bassy), Vintage Chorus I + II and a slow phaser.

Try: switch the phaser off (FX page) and listen to how much it adds.)" },
        { "Slow Strings Swell", R"(Recipe: strings plus a saw an octave down, a dark 24 dB lowpass at 600 Hz, and Env 2 with a 2.2 s attack opening the filter AND moving the wavetable position. The amp also fades in over 1.4 s.

So every note grows louder and brighter at the same time.

Try: Macro 3 (Bloom) changes how fast it opens.)" },
        { "Pizzicato Strings", R"(Recipe: the string wavetable with a very short envelope (no sustain, 0.38 s decay), a "Pluck Noise" for the finger, and a quick filter blip from Env 2.

Short sounds need reverb to feel natural: the hall does that.

Try: Macro 4 changes the length.)" },
        { "Tremolo Strings", R"(Recipe: strings with the amp modulated by a fast triangle LFO (9 Hz): that's bowed tremolo, the nervous shimmer in film scores.

Try: Macro 3 changes the speed. Slow it right down for a gentle pulse.)" },
        { "Silk Strings", R"(Recipe: strings an octave up, with an "Airy Pad" layer, a highpass at 300 Hz (no low end at all), a slow attack and a delayed vibrato (the LFO fades in).

High strings with no bass sit above a band without clashing.

Try: Macro 3 brings in more of the airy layer.)" },
        // ----------------------------------------------------------------- bells
        { "Tubular Bells", R"(Recipe: FM with a non-whole ratio. The modulator runs at 3.5x the carrier, and non-integer ratios create inharmonic overtones: that's what makes bells sound like bells. A sine an octave down adds the low hum of a big bell.

Try: Macro 3 changes the hum level.)" },
        { "Music Box", R"(Recipe: three sines: the note, two octaves up, and an octave and a fifth above that, transposed up an octave overall. A tiny "Pluck Noise", a short decay and a soft echo.

Try: Macro 1 changes the high overtone level.)" },
        { "Glockenspiel", R"(Recipe: a sine plus two overtones at inharmonic ratios (2.76x and about 5.4x, the real overtones of a metal bar), each faded out quickly by Env 3, with a "Click" for the mallet.

The overtones last only a moment; the pure tone rings on. That's the glockenspiel shimmer.

Try: Macro 3 (Metal) brings the overtones forward.)" },
        { "Crystal Bells", R"(Recipe: the "Fibonacci Partials" wavetable (overtones at unusual ratios), with Env 2 sweeping its position at each strike, into a dotted echo and shimmer.

Try: Macro 1 for brighter or darker crystal.)" },
        { "Celesta", R"(Recipe: FM with a 4x modulator (an octave-and-two above: bright but still musical), a short burst of modulation at each note, "Soft Click", an octave up.

Try: compare with Tubular Bells: a whole-number ratio (4) gives a sweeter bell than 3.5.)" },
        { "Bells into Pad", R"(Recipe: an FM bell wavetable with a decay to 55 % sustain on Engine 1, and a "Warm Pad" on Engine 2 whose volume Env 3 fades in over 2 s.

Two envelopes with different timing make one sound that changes character as you hold it.

Try: Macro 3 sets how loud the pad becomes.)" },
        // ----------------------------------------------------------------- evolving pads
        { "Harmonic Drift", R"(Recipe: "Harmonic Scan" and "Vowels" wavetables. A slow Function moves the first one's position; a smooth Random moves the second and nudges the filter. Neither repeats in step with the other, so the pad never loops audibly.

Try: Macro 3 changes the Function's speed.)" },
        { "Breathing Vowels", R"(Recipe: "Vowels" and "Breathy Vox" wavetables with "Air" noise. Two LFOs at different slow rates (0.07 and 0.11 Hz) move the vowels and the filter.

Slow LFOs at unrelated rates are an easy way to make movement that doesn't sound mechanical.

Try: Macro 3 changes the main LFO speed.)" },
        { "Glass Tides", R"(Recipe: "Glass Pad" and "Shimmer Evolve" (an octave up), moved by two LFOs in opposite directions, through a slow phaser and a large hall.

Try: Macro 3 changes the drift speed.)" },
        { "Slow Bloom", R"(Recipe: the "Warm Saw Sweep" wavetable starts at its darkest position. Env 2 (6 s attack) moves the position and opens the filter together, with a sine sub underneath and a big shimmer.

Try: Macro 3 shortens the bloom.)" },
        { "Spectral Motion", R"(Recipe: "Odd Harmonics" and "Digital Harmonics" wavetables, moved in opposite directions by a stepped random source (Turing type), through a phaser and a dotted echo.

Stepped random gives rhythm-like movement without a fixed pattern.

Try: Macro 3 changes the step speed.)" },
        { "Aurora", R"(Recipe: a 7-voice Super unison saw with a triangle an octave up, a 24 dB lowpass, and a slow looping Function (a rise-and-fall shape) on the cutoff, into a big shimmer.

Try: Macro 3 changes the speed of the rise and fall.)" },
        // ----------------------------------------------------------------- polysynth pads
        { "Poly Brass Pad", R"(Recipe: two detuned saws with 2-voice unison through the SVF Morph filter (12 dB, like the classic state-variable polysynths). Env 2 opens it with a 0.25 s attack: that's the brassy swell.

Try: Macro 3 (Swell) changes the filter attack. Short = brass, long = pad.)" },
        { "Vintage Poly Pad", R"(Recipe: a saw and a pulse (35 % width) slightly detuned, Drift on for analog instability, the OTA 4-Pole filter (smooth 24 dB), a slow filter envelope, and LFO 1 slowly moving the pulse width (PWM).

PWM makes a single oscillator sound like several: the "moving" quality of late-70s pads.

Try: Macro 3 changes the PWM speed.)" },
        { "Wide Saw Pad", R"(Recipe: a 7-voice Super unison saw with a square sub an octave down, through the Multimode 24 dB lowpass, with chorus.

Try: Macro 3 changes the unison detune. Less = focused, more = huge.)" },
        { "Analog Sweep Pad", R"(Recipe: two detuned saws through the Ladder filter with fairly high resonance (0.45), and a slow LFO sweeping the cutoff.

Resonance makes the sweep audible: it boosts the frequencies right at the cutoff as it moves.

Try: Macro 4 changes the resonance, Macro 3 the sweep speed.)" },
        { "Expressive Poly", R"(Recipe: two detuned saws with lots of Drift, a 12 dB lowpass (Analog on) and a highpass. Velocity AND aftertouch open the filter, and aftertouch also adds vibrato.

Try: press harder into held chords. It responds like a performer's instrument.)" },
        // ----------------------------------------------------------------- 80s chorus polysynth
        { "Chorus Poly Pad", R"(Recipe: the classic 80s one-oscillator polysynth: a saw plus a pulse wave with slow PWM (LFO 1), a square sub an octave down, a gentle highpass, a 24 dB lowpass (Analog on), and Vintage Chorus II. Notice how little resonance: these sounds are warm, not squelchy.

The chorus is half the sound.

Try: switch the chorus between I, II and I + II on the FX page.)" },
        { "Chorus Poly Strings", R"(Recipe: the same polysynth with only the saw, the filter open higher (3.6 kHz), a highpass at 240 Hz and chorus I + II.

Strings on these synths are just a bright saw plus chorus: simple, but it works.

Try: compare with Ensemble Strings (two engines) to hear what one oscillator can do.)" },
        { "Chorus Poly Brass", R"(Recipe: saw plus sub, a low filter (700 Hz) that Env 2 opens quickly (0.04 s attack) and closes to 45 %. That fast open-close is the brass "blat".

Try: Macro 3 (Bite) shortens the filter decay for sharper stabs.)" },
        { "Chorus Poly Keys", R"(Recipe: mostly the pulse (PWM), a low filter with a quick Env 2 pluck, a decaying amp envelope, chorus II and an echo.

Try: Macro 4 changes the note length.)" },
        { "Chorus Poly Bass", R"(Recipe: square plus sub, mono with a little glide, a low filter with a short pluck, chorus I at low depth and width (bass should stay centred).

Try: turn the chorus width up and hear the bass spread: nice in headphones, risky on a PA.)" },
        { "Chorus Poly Arp", R"(Recipe: the pulse polysynth through the arpeggiator (Up, 2 octaves, 16ths), with a quick filter pluck, chorus II and a dotted-eighth echo.

Try: change the arp order on the SEQ page.)" },
        // ----------------------------------------------------------------- ladder leads
        { "Ladder Lead", R"(Recipe: three oscillators (two detuned saws and a square an octave down), mono with glide, through the Ladder filter with some drive. Env 2 opens it at each note.

That's the classic three-oscillator mono lead.

Try: play legato for glide. Macro 3 adds resonance.)" },
        { "Fat Mono Lead", R"(Recipe: saw, square and a saw an octave down, lots of Ladder drive, Tube distortion on FX bus A, and more glide.

Drive in two places (filter and FX) is what makes it thick.

Try: Macro 3 changes the tube drive.)" },
        { "Soft Ladder Lead", R"(Recipe: a square with a triangle an octave up, a low cutoff with little resonance, a slow attack (30 ms), and vibrato that fades in (LFO fade 0.7 s), like a flute player.

Try: hold long notes to hear the vibrato arrive.)" },
        { "Resonant Sweep Lead", R"(Recipe: two saws through the Ladder filter with high resonance (0.62). Env 2 sweeps the cutoff at each note: the resonance makes that sweep "talk".

Try: Macro 4 (Resonance) up and down while playing.)" },
        { "Fifth Ladder Lead", R"(Recipe: a saw with a second saw a fifth up (7 semitones) and a sub, with a long glide (0.15 s).

The fifth makes every note a power chord: big and anthemic.

Try: Macro 3 lowers the fifth to hear the plain lead.)" },
        // ----------------------------------------------------------------- pulsing basses
        { "Pulse Bass 8ths", R"(Recipe: saw plus sub through the Ladder filter (+6 dB filter volume to make up level), played by the sequencer in eighth notes. Env 2 gives each step a short filter pluck.

Hold one note: the sequencer repeats it in time with the song.

Try: Macro 4 (Gate) changes how much of each step sustains: low = short and punchy, high = legato.)" },
        { "Octave Pulse Bass", R"(Recipe: 16th notes where every second step jumps up an octave (the Octave track on the SEQ page).

Try: change which steps jump on the SEQ page.)" },
        { "Rolling Bass 16ths", R"(Recipe: a 3-3-2 rhythm in 16ths with accents (louder steps) and an octave jump on the last step.

Accents are just higher velocity on some steps; with velocity opening the filter, accented steps are also brighter.

Try: press generate on the SEQ page for a new rhythm.)" },
        { "Gated Sub Pulse", R"(Recipe: a sine sub with a quiet triangle an octave up, re-triggered in eighths with long gates.

Under pads, a pulsing sub adds energy without adding notes.

Try: Macro 3 brings up the overtone so it's heard on small speakers.)" },
        { "Dark Pulse Bass", R"(Recipe: 16th notes through the OTA filter, with a very slow LFO (0.08 Hz) moving the cutoff over several bars.

A slow LFO on a fast pattern is an easy way to build tension.

Try: Macro 3 changes the speed of the sweep.)" },
        { "Synthwave Bass", R"(Recipe: two detuned saws and a sub in 16ths through the OTA filter, with Vintage Chorus I.

Try: switch the chorus off to hear the plain bass.)" },
        // ----------------------------------------------------------------- organs
        { "Gospel Drawbars", R"(Recipe: an organ built from sine waves like real drawbars: 16', 5 1/3' and 8' fully out (888000000), plus percussion (a short extra harmonic at each note), key click, and the Rotary speaker on FX bus A.

The mod wheel and Macro 3 switch the rotary to Fast. Let go and it slows down gradually.

Try: hold a chord and flick the mod wheel up and down: the speed-up and slow-down is the gospel organ sound.)" },
        { "Full Drawbars", R"(Recipe: all drawbars out, more drive in the filter and the rotary's preamp. The full, overdriven rock and gospel organ.

Try: Macro 4 (Drive) changes the rotary drive.)" },
        { "Jazz Organ", R"(Recipe: just 8' and 4' drawbars with second-harmonic percussion and a gentle rotary. Fewer drawbars = rounder and softer.

Try: compare with Full Drawbars: same instrument, different registration.)" },
        { "Chapel Organ", R"(Recipe: gentle drawbars (8' with a little 4' and 2'), a slightly slower attack, a half-wet rotary and a hall.

Try: keep the rotary on Slow for hymns.)" },
        { "Organ Swell Pad", R"(Recipe: drawbars with a slow attack (0.9 s) instead of the instant organ attack, a slow rotary and a big hall.

Changing only the envelope turns an organ into a pad.

Try: Macro 3 for a Fast rotary on a held chord.)" },
        { "Full Pipe Organ", R"(Recipe: the "Pipe Organ" wavetable plus an octave rank and a quiet "mixture" (a fifth above, on the utility oscillator), a 40 ms attack, no velocity, and a long cathedral reverb.

Try: Macro 3 changes the mixture: more = brighter, more "full organ".)" },
        { "Soft Flute Stop", R"(Recipe: the "Flute Pipe" wavetable with a little "Air" noise, a slow-ish attack and a very slight wobble.

Try: Macro 3 changes the breath.)" },
        { "Chorus Poly Organ", R"(Recipe: the 80s polysynth used as an organ: square plus sub, no PWM, a straight on/off envelope, chorus II.

Try: Macro 3 changes the sub level.)" },
        // ----------------------------------------------------------------- more strings
        { "Expressive Strings", R"(Recipe: strings whose volume and brightness follow the mod wheel: wheel down = soft and dark, up = loud and bright. Like a bow pressing harder.

Try: hold a chord and ride the wheel slowly up and down.)" },
        { "Octave Strings", R"(Recipe: two string layers an octave apart. One finger plays a full octave line.

Try: Macro 3 lowers the upper octave.)" },
        { "Low Strings", R"(Recipe: strings with a saw an octave down, a dark 24 dB filter (900 Hz) that Env 2 opens slowly, and a 0.45 s attack.

Try: hold low notes under a build. Macro 3 adds vibrato speed.)" },
        { "Spiccato Strings", R"(Recipe: very short bow strokes: an 8 ms attack, 0.22 s decay, a quick filter blip and a little pluck noise.

Try: Macro 4 changes the note length.)" },
        { "Strings and Choir", R"(Recipe: strings with a "Choir Ah to Oo" layer; the mod wheel changes the vowel.

Try: Macro 3 brings the choir up.)" },
        { "Poly Strings", R"(Recipe: a unison saw through the OTA filter (12 dB mode), a highpass, a slow attack and chorus.

Try: compare with Chorus Poly Strings: different filter, different era.)" },
        // ----------------------------------------------------------------- grand poly
        { "Grand Poly Pad", R"(Recipe: two detuned saws and a square sub, 2-voice unison, Drift, through the OTA 4-Pole filter (smooth 24 dB), with a slow filter envelope and a light chorus.

Two oscillators per voice and a creamy 4-pole filter: the 80s flagship pad.

Try: Macro 1 for brightness, Macro 4 for a slower swell.)" },
        { "Grand Poly Brass", R"(Recipe: two detuned saws through the OTA filter, which Env 2 opens strongly (60 ms attack) and partly closes. Velocity adds brightness.

Try: Macro 3 (Bite) for shorter, punchier stabs.)" },
        { "Grand Poly Sweep", R"(Recipe: saw and pulse through a resonant OTA filter (0.5), swept by a slow triangle LFO.

Try: Macro 3 changes the sweep speed.)" },
        { "Grand Poly Arp", R"(Recipe: a saw and a pulse an octave up through the OTA filter (+4.6 dB filter volume), arpeggiated up and down in 16ths, with a dotted echo.

Try: hold four-note chords and change them every bar.)" },
        { "Grand Poly Bells", R"(Recipe: cross-modulation (FM) on the Analog engine: a sine modulated by a sine 19 semitones up, with a burst at each note, plus chorus.

Try: Macro 1 (Metal) changes the FM amount.)" },
        // ----------------------------------------------------------------- more chorus poly
        { "Chorus Poly Sweep", R"(Recipe: saw, PWM and sub with a resonant lowpass (0.4) swept slowly by LFO 2, through chorus I + II.

Try: Macro 3 changes the sweep speed.)" },
        { "Chorus Poly Soft Pad", R"(Recipe: a dark (900 Hz) saw and PWM pad with chorus I, slow attack.

Try: Macro 1 slowly up into a chorus of a song.)" },
        // ----------------------------------------------------------------- vintage poly
        { "Vintage Poly Brass", R"(Recipe: a saw and a pulse, slightly unstable (Drift), through the OTA filter, with a softer filter swell (0.15 s) than the Grand Poly brass.

Try: Macro 3 changes the swell.)" },
        { "Vintage Poly Keys", R"(Recipe: saw plus narrow pulse, a quick filter pluck and a decaying amp envelope, light chorus.

Try: Macro 4 changes the decay.)" },
        { "Vintage Poly Sync", R"(Recipe: hard sync: Osc 2 is reset by Osc 1 (which you don't hear). Env 2 sweeps Osc 2's pitch at each note, which changes the tone, not the note: that's the "tearing" sync sweep. The mod wheel adds more.

Try: ride the mod wheel on a held note.)" },
        { "Vintage Poly Strings", R"(Recipe: saw plus pulse with slow PWM, the OTA filter, a slow attack and chorus.

Try: Macro 3 changes the PWM speed.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
