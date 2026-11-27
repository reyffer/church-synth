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
        { "Tonewheel Init", R"(Recipe: the Organ engine, a model of a tonewheel organ. Each drawbar adds one harmonic of the key you play, made by its own spinning tonewheel: 16' (an octave down), 5 1/3' (a fifth above that), 8' (the note itself), then 4', 2 2/3', 2', 1 3/5', 1 1/3' and 1' going up. Here: 888 000 000, the first three bars full.

The signal then goes through the Organ Scanner (FX A1, mode C3: the organ's own chorus) and the rotary speaker (FX A2). The mod wheel switches the rotary to Fast once it passes half way; let go and it slows down again, the bass drum more slowly than the horn, as on a real cabinet.

Playing: no velocity (an organ key is on or off). The Expression pedal (CC11) is the swell, through Amp Mod. Without a pedal the organ plays at full level. Each key click comes from the drawbar contacts closing a fraction of a millisecond apart.

Try: Registrations... above the drawbars for classic settings. Pull out the 4' and 2' for more edge. For a split or pedal bass, use a second instance (see Tonewheel Bass).)" },
        { "Tonewheel Gospel", R"(Recipe: 888 800 008 on the Organ engine: the three lower bars and 4' full, plus the 1' for the sizzle on top. A bit more leakage and Wheel Character than the Init preset for the slightly dirty sound of an old instrument, and a harder rotary Drive (FX A2).

Why it works in gospel: the 16' gives weight under the band, the 1' cuts through cymbals, and the driven rotary turns chords into a roar when you hit the fast speed.

Listen for: the swirl changing when you push the mod wheel past half way (fast) and back (slow); the speed ramps instead of jumping.

Try: Macro 1 pulls out the middle bars (2 2/3', 2', 1 3/5') for the full "all out" shout. Macro 3 adds drive. Use the Expression pedal for dynamics, not velocity.)" },
        { "Tonewheel Jazz", R"(Recipe: 888 000 000 with percussion: Third harmonic, Fast decay, Normal level. The percussion adds a short "ping" an octave and a fifth above each note (the 2 2/3' pitch) that dies away quickly.

How the percussion behaves, as on the original: it only plays when you start from all keys up. Notes played legato, while another key is still down, get no new ping. Jazz players use this: detached notes ping, legato runs stay smooth. With percussion on, the 1' drawbar is silent and the drawbars drop a little in level (Normal).

Listen for: the difference between a staccato line (every note pings) and a legato line (only the first note does).

Try: Second harmonic for a softer, rounder ping. Slow decay for a bell-like tail. Macro 4 sets the key click.)" },
        { "Tonewheel Ballad", R"(Recipe: 008 500 000: just the 8' (the note itself) and a softer 4' an octave up. With no 16' it stays out of the bass player's way; with no high bars it is round and soft. Chorus C2 (FX A1) and the slow rotary give movement; a little hall.

Key click is low (0.2), so notes start smoothly, and leakage is low for a clean tone.

Why it works for quiet moments: a pure, soft organ is one of the most supportive sounds under a voice, and it never gets in the way of the words.

Try: Macro 1 adds the 16' and 2' for a fuller chorus. Swell with the Expression pedal rather than playing harder.)" },
        { "Worship Tonewheel", R"(Recipe: 838 000 000: full 16' and 8' with the 5 1/3' only at 3, so the bass is warm without the hollow "quint" sound getting heavy. Chorus C3, slow rotary with very little drive, a little hall.

This is the organ for sitting under a worship band: wide, warm and steady. Hold chords with your left hand and leave the melody to the singers.

Listen for: how the slow rotary makes held chords move without any obvious wobble.

Try: Macro 1 adds the 4' and 2' as the song builds. The mod wheel (fast rotary) for the last chorus.)" },
        { "Tonewheel Flutes", R"(Recipe: 006 040 000: the 8' at 6 and the 2 2/3' at 4 only. That is the classic "flute" colour: hollow and a little reedy because the 2 2/3' is a twelfth (not an octave). Wheel Character is low, so the wheels are almost pure sines.

The Organ Scanner is on V3: vibrato only (no dry signal), the deepest setting, so the pitch wobbles like a theatre organ flute. Compare with the C modes, which mix in the dry signal and sound like a chorus instead.

Try: switch the scanner (FX A1) between V1, V2 and V3 to hear the depth change. Macro 1 adds the 4'.)" },
        { "Tonewheel Lead", R"(Recipe: 800 000 888: the 16' plus the three highest bars. With nothing in the middle the sound is thin and piercing, made for single-note lines. Percussion: Second harmonic, Fast, and here it does not mute the 1' (Mutes the 1' is off).

Drive is high on the rotary (FX A2), so the sound bites when you play harder chords; the mod wheel switches the rotary to fast for long held notes.

Try: play a line legato and then detached to hear the percussion only on detached notes. Macro 1 adds the 8' for body.)" },
        { "Tonewheel Choir", R"(Recipe: 006 876 540: the bars rise and then fall away towards the top, so the harmonics are balanced like a soft string section or a choir "ooh". No 16', so it floats above the band.

Chorus C2 (FX A1) and the slow rotary, a little hall. Key click is low for smooth entries.

Listen for: how many drawbars blend into one smooth tone when their levels form a curve like this.

Try: Macro 1 adds the 16' for a fuller ending. Move single drawbars one step at a time and listen to how each harmonic changes the colour.)" },
        { "Tonewheel All Out", R"(Recipe: 888 888 888: every drawbar fully out. More leakage (the faint sound of neighbouring wheels), more Wheel Character (grittier wheels) and a key click that cuts through. The rotary Drive (FX A2) is pushed hard.

This is the loudest organ registration; on a real instrument it is used for climaxes and rock. Watch your level.

Try: Macro 2 for even more drive, Macro 3 for more leakage dirt. Pull single bars back in to see how much each harmonic carries.)" },
        { "Tonewheel Rock", R"(Recipe: 888 600 000 with high Wheel Character and leakage, a strong key click and a lot of rotary drive (FX A2): the crunchy rock organ. The 4' at 6 gives edge without the fizz of the top bars.

Rock players switch the rotary to fast for solos and big chords: the mod wheel does it here. The drive comes before the rotary, as with a real cabinet's preamp.

Try: Macro 1 adds the 2' and 1' for a screaming top. Macro 3 for more drive.)" },
        { "Tonewheel Bass", R"(Recipe: 808 000 000: the 16' and 8' only, for left-hand bass lines. A strong, dark key click (click tone low) gives the "thump" of organ bass. Four voices.

The rotary (FX A1) is set mostly to the bass drum (Horn/Drum at 0.3), which turns slowly and adds only gentle movement. Mod wheel = fast.

How to use it: one Church Synth instance per manual. In MainStage, put this preset on a second channel strip and limit its key range to the lower part of the keyboard (or a pedalboard); play the upper manual with another tonewheel preset.

Try: Macro 1 adds the 5 1/3' for a growlier bass. Macro 4 takes the rotary out for a tighter low end.)" },
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
