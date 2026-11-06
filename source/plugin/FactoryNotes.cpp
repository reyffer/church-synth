// "How it's built" notes for the factory presets, shown in the preset browser.
// Written for players learning sound design by ear: the recipe, what each part
// does to the sound, and something to try. Keep them in step with FactoryPresets.cpp.

#include "PresetManager.h"

#include <map>

juce::String factoryPresetNotes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        { "Sanctuary Pad", R"(Recipe: both engines play saw waves with 4-voice unison (thickness), plus a second oscillator per engine (a square an octave down in Engine 1, a slightly detuned saw in Engine 2). Engine 2 is detuned a few cents against Engine 1, and Drift adds the slow pitch wander of old analog synths.

Filter 1 is a 24 dB lowpass at 1.6 kHz: it takes the fizz off the saws. LFO 1 (triangle, very slow) moves the cutoff a little so the pad breathes. Slow attack (0.9 s), long release (2.6 s), Vintage Chorus mode I and a hall.

Listen for: the slow beating between the detuned saws. That is what makes it feel alive.

Try: turn Drift to 0 and hear how static it gets. Turn Macro 1 (Brightness) up for a band, down for a quiet verse.)" },
        { "Warm Bed", R"(Recipe: the "Warm Pad" wavetable with 4-voice unison on Engine 1, and a pure sine an octave down on Engine 2 as a sub. A 12 dB lowpass at 2.4 kHz keeps it soft.

LFO 1 slowly moves the wavetable position: the tone changes very gently over time without any obvious wobble.

Why it sits under a band: no bright top end and no busy movement. The sine sub gives weight without mud, because a sine has no harmonics.

Try: Macro 3 moves the wavetable position by hand; find the spot you like. Lower Engine 2's volume if the bass player is busy.)" },
        { "Glass Cathedral", R"(Recipe: two wavetables, "Glass Pad" and "Airy Pad" (an octave up), each slowly scanned by its own LFO at a different rate (0.06 and 0.11 Hz), so the two layers drift in and out of step.

A 12 dB lowpass at 5.2 kHz, a slow attack and a long 4 s release, into the Shimmer reverb (a reverb that adds an octave above, the "angelic" tail).

Listen for: the shimmer tail rising above the chord when you let go.

Try: Macro 3 changes how fast the glass moves. Turn the shimmer send (Macro 2) down for a more intimate sound.)" },
        { "Choir Air", R"(Recipe: the "Choir Ah to Oo" wavetable (its position morphs between vowels) with 4-voice unison, plus the "Breath" noise for air. 12 dB lowpass at 3.2 kHz.

The mod wheel moves the wavetable position: wheel down = "ah", wheel up = "oo". A slow LFO adds a tiny vowel movement so it never sounds frozen.

Try: ride the mod wheel slowly while holding a chord, like a choir changing vowel. Macro 3 also moves the vowel.)" },
        { "Strings Pad", R"(Recipe: the "String Ensemble" wavetable with 5-voice unison, layered with a unison saw on Engine 2 for extra body. 24 dB lowpass at 2.8 kHz; velocity opens the filter a little, so harder playing is brighter.

Slow-ish attack (0.6 s) like a bow, chorus for width, hall.

Try: play softly, then firmly, and listen to the brightness change. Macro 4 (Swell) lengthens the attack for very slow entries.)" },
        { "Shimmer Swell", R"(Recipe: one saw with Super unison (7 voices), a 24 dB lowpass starting low (900 Hz), and Env 2 opening the filter over 3.5 s. The amp attack is 2.5 s, so the sound fades in and brightens at the same time.

The Shimmer reverb adds the octave-up tail.

Listen for: two slow movements at once, volume and brightness. That double swell is the "bloom".

Try: shorten Env 2's attack to hear the difference between a fade-in and a bloom.)" },
        { "Evolving Light", R"(Recipe: two wavetables ("Shimmer Evolve" and "Airy Pad"). A smooth Random source moves Engine 1's wavetable position and nudges the filter; a slow Function (a drawn shape) moves Engine 2's position.

Because random and function never line up, the sound keeps changing without repeating.

Try: Macro 3 changes the random speed. Set it to 0 and the pad becomes calm; turn it up for a restless texture.)" },
        { "Soft Organ Pad", R"(Recipe: the "Drawbars" wavetable (each position is a different drawbar registration) with 2-voice unison, a 12 dB lowpass, a quick-ish attack and Vintage Chorus I + II for the swirl.

It's a pad, not a real organ: the attack is softer and the release longer, so it blends under singers.

Try: Macro 3 changes the registration (wavetable position). For a real tonewheel organ with a rotary speaker, see the Organ presets in the pack.)" },
        { "Ambient Fifths", R"(Recipe: a triangle and a sine an octave up, with Unison set to Chord mode: every key plays the root and the fifth. That's why one finger sounds like a chord.

12 dB lowpass at 2.6 kHz, very slow attack and a 5 s release, into the Shimmer reverb.

Try: play single notes in the bass and let them ring. Play two notes a fourth apart and listen to the open, "modern worship" voicing.)" },
        { "Dark Drone", R"(Recipe: two saws (one an octave down), 3-voice unison, plus the "Rumble Drone" noise. A 24 dB lowpass at only 520 Hz with some drive keeps everything dark but thick. A very slow LFO moves the cutoff.

Try: Macro 1 (Brightness) slowly up across a transition, then back down. The Drive knob on Filter 1 adds grit without getting brighter.)" },
        { "Electric Tines", R"(Recipe: a sine (the body of the note) frequency-modulated by Oscillator 3, a sine set 14 semitones up. FM adds the metallic "bark" of a tine piano. Env 2 makes the bark short (it fades in 0.4 s), and velocity adds more: soft = round, hard = bell-like bark.

Instant attack (0 ms), a long decay to a low sustain, a Panner for tremolo.

Try: Macro 1 (Bark) up for a brighter, 80s piano. Change Osc 3's pitch to 12 or 19 semitones and hear how the bark's character changes.)" },
        { "Soft Keys", R"(Recipe: a triangle with a sine an octave up, through a 24 dB lowpass at 2.6 kHz. Env 2 opens the filter a little at the start of each note and velocity adds a little more, so notes have a soft "front".

Short-ish decay into a 35 % sustain, chorus and a hall.

Try: Macro 1 for brightness, Macro 4 for a shorter or longer note. A good starting point to make your own simple keys.)" },
        { "Stage Keys", R"(Recipe: a saw plus a narrow square an octave up, through a 24 dB lowpass at 1.8 kHz. Env 2 opens the filter quickly at each note (the "pluck" at the front), and the "Key Click" noise adds a mechanical attack.

Instant attack, decay to 40 % sustain.

Why it cuts through: the narrow square adds bright upper harmonics, and the filter envelope makes every note start bright.

Try: Macro 3 turns the click up or down.)" },
        { "Gospel Organ", R"(Recipe: the "Drawbars" wavetable plus a sine an octave down for the bass drawbar. The mod wheel moves the wavetable to a fuller registration (more drawbars). Fast attack, short release, Vintage Chorus I + II.

Try: ride the mod wheel into the chorus of a song. For a rotary speaker (the real gospel sound), use the Tonewheel organs in the pack.)" },
        { "Pipe Organ", R"(Recipe: the "Pipe Organ" wavetable (principal pipes) with a "Flute Pipe" rank an octave up on Engine 2. No velocity (a real pipe organ isn't touch-sensitive), a slow 30 ms attack like air filling a pipe, and a large, long reverb.

Try: Macro 3 brings the flute rank up or down. Macro 2 changes the room.)" },
        { "Glass Keys", R"(Recipe: the "Glass" wavetable alone. Velocity moves the wavetable position, so harder notes are glassier. Instant attack, a long decay to silence (no sustain), a quarter-note echo and a hall.

Try: Macro 1 moves the wavetable for a darker or brighter glass. Play slow melodies and let the echo fill the gaps.)" },
        { "Worship Pluck", R"(Recipe: two saws (one detuned) with unison through a 24 dB lowpass set very low (400 Hz). Env 2 opens the filter for a fraction of a second at each note: that quick opening and closing IS the pluck. Velocity opens it a little more.

Then a dotted-eighth ping-pong delay (the classic worship-guitar rhythm) and a hall.

Try: Macro 4 changes the pluck length (Env 2 decay). Macro 3 sets how many echoes you get.)" },
        { "Bell Pluck", R"(Recipe: the "FM Ratio 3" wavetable, whose positions go from a pure tone to a bright FM bell. Env 2 moves the position at each note: bright at the strike, mellow as it rings. Instant attack, long decay, dotted-eighth echo.

Try: Macro 1 sets how bright the bell is overall.)" },
        { "Soft Mallet", R"(Recipe: a triangle plus a sine 19 semitones up (an octave and a fifth, a typical mallet overtone), with a "Soft Click" noise for the stick. Filter 1 is a Lowpass Gate: a filter and volume control in one, opened briefly by Env 2 at each note. That's what gives the rounded "bonk".

Try: Macro 4 changes how quickly the gate closes (longer = more ring). Macro 3 brings out the overtone.)" },
        { "Thumb Piano", R"(Recipe: a sine with a quiet sine two octaves up, a "Pluck Noise" for the woody click, a lowpass at 4.5 kHz, instant attack and a short decay.

Try: Macro 3 turns the high overtone up for a brighter, more metallic tine.)" },
        { "Ambient Pluck", R"(Recipe: a triangle with a quiet saw an octave up, a 24 dB lowpass that Env 2 opens briefly, an eighth-note ping-pong delay with lots of feedback, and a big hall.

The pluck itself is short; the delay and reverb make the long, ambient trail.

Try: play sparse notes and leave space. Macro 3 sets the number of echoes.)" },
        { "Soft Lead", R"(Recipe: a square (at 40 % pulse width) with a quiet detuned saw, mono with glide (notes slide into each other when you play legato). 24 dB lowpass at 2.2 kHz.

The mod wheel adds vibrato: LFO 1 (5.5 Hz) moves the pitch, but only as much as the wheel allows.

Try: play legato (overlap the notes) to hear the glide; play detached for no glide.)" },
        { "Saw Lead", R"(Recipe: one saw with Super unison (7 voices), mono, through a 24 dB lowpass at 3.8 kHz, with an eighth-note ping-pong echo.

Try: Macro 3 (Width) changes the unison detune: wider = bigger and more "trance", narrower = focused.)" },
        { "Breath Flute", R"(Recipe: the "Flute Pipe" wavetable with "Breath" noise, mono with glide, a 60 ms attack (a flute takes a moment to speak), and a vibrato that fades in after the note starts (LFO fade-in), like a real player.

Try: Macro 3 changes the amount of breath. Hold long notes to hear the delayed vibrato arrive.)" },
        { "Sync Lead", R"(Recipe: hard sync. Oscillator 2 is reset by Oscillator 1 at Osc 1's pitch, so you always hear Osc 1's note, but Osc 2's pitch changes the TONE (that tearing, vocal sound). Osc 1 itself is silent; it only drives the sync.

Env 2 sweeps Osc 2's pitch down at each note, and the mod wheel pushes it further.

Try: ride the mod wheel slowly on a held note to hear the sync sweep.)" },
        { "Sub Bass", R"(Recipe: a pure sine with a quiet triangle an octave up, mono with a little glide, a gentle lowpass at 900 Hz.

The sine is the "felt" part; the triangle gives just enough harmonics to be heard on small speakers.

Try: Macro 3 brings in more of the triangle if the bass disappears on a small PA.)" },
        { "Warm Bass", R"(Recipe: a saw and a square an octave down, mono, through a 24 dB lowpass at 500 Hz with a little drive. Env 2 opens the filter briefly at each note (the punch), velocity adds more.

Try: Macro 3 (Punch) changes how long the filter stays open. Filter Drive up for a dirtier bass.)" },
        { "Pluck Bass", R"(Recipe: a saw with a sine an octave down, polyphonic (so octave lines can overlap), through a 24 dB lowpass at 300 Hz that Env 2 opens very briefly. Instant attack, short decay, no sustain.

Try: play octave lines. Macro 3 changes the pluck length.)" },
        { "Drone Bass", R"(Recipe: a saw and a sine an octave down through a very low 24 dB lowpass (260 Hz) with drive, a slow attack, and a slow LFO on the cutoff.

Try: hold one low note under a whole section; Macro 1 slowly up for a build.)" },
        { "Synth Strings", R"(Recipe: the 70s string machine: two engines of saws (one an octave up) with unison, a 12 dB lowpass and a 12 dB highpass in series (the highpass removes the low mud), a slow bow attack, and Vintage Chorus mode II. The chorus does most of the work: it turns plain saws into an ensemble.

Try: switch the Vintage Chorus off on the FX page and hear how plain the saws are without it.)" },
        { "Soft Brass", R"(Recipe: two detuned saws with unison through a 24 dB lowpass at 700 Hz. Env 2 opens the filter with a short attack (0.12 s), like a brass player's breath building up. Velocity opens it more.

Try: Macro 3 (Bite) shortens the filter attack for sharper stabs; Macro 4 lengthens the amp attack for swells.)" },
        { "Cello Swell", R"(Recipe: the "String Ensemble" wavetable plus a saw, mono with glide, a 24 dB lowpass, a slow attack. The mod wheel adds vibrato to one engine and aftertouch to the other, so you can play it like a solo string player.

Try: play a slow melody with the mod wheel slightly up, and press harder into held notes.)" },
        { "Rain Texture", R"(Recipe: no oscillators at all: just the "Rain" and "Wind" noises through a bandpass filter that a slow LFO moves. Every key plays the same texture.

Try: hold one key for a background bed between songs. Macro 1 moves the bandpass to a darker or brighter spot.)" },
        { "Ocean Swell", R"(Recipe: the "Ocean" noise over a soft sine pad with a little unison, slow attack and release, a big reverb.

Try: lower Engine 1's volume for just the waves.)" },
        { "Vinyl Air", R"(Recipe: the "Airy Pad" wavetable with "Vinyl Crackle" noise, a lowpass, and a Bitcrusher on FX bus A to make it slightly grainy.

Try: switch the Bitcrusher off to hear the clean pad, then back on: that's the lo-fi part.)" },
        { "Pulse Arp", R"(Recipe: a saw and a narrow square an octave up, a filter that Env 2 plucks, and the arpeggiator: Up order, 2 octaves, 16ths. Hold a chord and the arp plays its notes in turn. A dotted-eighth echo doubles the rhythm.

Try: change the Arp order (Up & Down, Random) and the octaves on the SEQ page.)" },
        { "Eighth Note Gate", R"(Recipe: a two-engine saw pad played by the sequencer in Seq mode: the whole held chord is re-triggered in an eighth-note rhythm (the pattern has gaps, which make the groove).

Try: change the steps on the SEQ page, or press generate for a new rhythm.)" },
        { "Ambient Arp", R"(Recipe: the "Glass Pad" wavetable, arpeggiated up and down over 2 octaves in eighths, with a dotted-quarter echo into the shimmer reverb.

Try: hold a big chord with lots of notes; the arp gets longer and more interesting.)" },
        { "Generative Bells", R"(Recipe: an FM bell (the "FM Ratio 2" wavetable) played by the sequencer. The pitch track is randomised (Random 80 %) and snapped to the Major Pentatonic scale, so any random note fits. Auto Regen picks new random values every two bars.

Try: hold one note and let it play. Change the scale on the SEQ page and hear the mood change.)" },
        { "Rhythmic Pad 16ths", R"(Recipe: the "Warm Pad" wavetable played in a swung 16th pattern with accents (the louder steps) and two octave jumps.

Swing (58 %) delays every second 16th a little, which makes it groove instead of tick.

Try: the Swing knob on the SEQ page: 50 % is straight, 66 % is a triplet feel.)" },
        { "Riser", R"(Recipe: a Super unison saw plus white noise through a 24 dB lowpass at 300 Hz. Function 1 is a one-shot ramp lasting eight seconds: it opens the filter and lifts the pitch, then stays at the top.

Try: start it eight seconds before the drop or the chorus. Macro 3 sets how much noise.)" },
        { "Transition Swell", R"(Recipe: the "Airy Pad" wavetable with a 3 s attack and the ADR envelope mode (it fades in, then straight back out, even if you keep holding). Env 2 opens the filter in the same shape. The shimmer reverb carries it out.

That swell-then-vanish shape sounds like a reversed sound.

Try: play it on the last beat before a new song.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
