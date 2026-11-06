// "How it's built" notes for Pack 2 (shown in the preset browser).

#include "Packs.h"

#include <map>

juce::String pack2Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- foundation pads
        { "Sanctuary Pad", R"(Recipe: the "Warm Pad" wavetable with 4 unison voices, plus a quiet saw on Engine 2 slightly detuned for body. A 24 dB lowpass at 1.8 kHz keeps it soft, a 1.2 s attack and 3 s release make chords blend into each other, and a light chorus and hall finish it.

Listen for: nothing sticks out. A good worship pad is felt more than noticed.

Try: Macro 3 (Motion) makes a slow LFO move the wavetable, so the pad gently changes colour. Macro 1 darkens it for verses.)" },
        { "Warm Blanket", R"(Recipe: a triangle (soft, few harmonics) with a little detuned saw, and a sine an octave down. The 24 dB lowpass sits very low (650 Hz), so almost all the brightness is gone.

Listen for: how warm a pad can be with almost no top end. It leaves the whole upper range for voices and speech.

Try: Macro 3 (Sub) brings up the octave below. Careful on a big PA.)" },
        { "Morning Light", R"(Recipe: the "Airy Pad" wavetable (a sine body with a breathy top), a "Glass Pad" an octave up, and two filters in series: a lowpass to tame the very top and a highpass at 250 Hz that removes the low end completely. Then shimmer reverb.

Listen for: a pad with no bass can't clash with the bass player or the left hand of the piano.

Try: Macro 3 (Air) brings up the glass octave.)" },
        { "Open Fifths", R"(Recipe: unison in Chord mode, set to Fifth + Oct: every key plays three notes, the root, the fifth and the octave. Two saws, slightly detuned, make it wide.

There's no third in the chord, so it's neither major nor minor: one finger works in any song. Play the root of each chord with your right hand.

Try: hold one note through a whole chord progression and listen to how it fits everything. Macro 3 adds a slow filter movement.)" },
        { "Octave Air", R"(Recipe: Chord-mode unison set to Octave (each key plays itself and the octave above), the whole engine moved up an octave, and a highpass at 400 Hz. A "Sparkle Air" noise adds a glittering top.

Listen for: this pad lives above the singers, so you can play it during vocals without fighting them.

Try: Macro 3 (Sparkle) for more noise on top.)" },
        { "Analog Worship Pad", R"(Recipe: a saw and a pulse wave, slightly detuned and drifting like an old analog synth, through the OTA filter (smooth 24 dB). Env 2 opens the filter slowly when you play and then settles. A slow LFO moves the pulse width (PWM), and Vintage Chorus II spreads it.

Listen for: the gentle brightening at the start of every chord.

Try: Macro 3 (Motion) makes the PWM deeper and the pad more animated.)" },
        { "Choir Pad", R"(Recipe: the "Choir Ah to Oo" wavetable (position = vowel), with a "Breathy Vox" layer and a little "Breath" noise. A slow LFO moves the vowel slightly so the choir never sounds frozen.

Try: Macro 3 (Vowel) moves from "ah" towards "oo": "oo" is darker and more intimate, "ah" is open and bright. Try "oo" for verses, "ah" for choruses.)" },
        { "Glass Worship Pad", R"(Recipe: the "Glass Pad" wavetable with 3 unison voices, a sine an octave down on Engine 2 to give it weight, a soft dotted-quarter ping-pong echo and shimmer.

Listen for: the echo makes the pad move left and right even though you're only holding chords.

Try: Macro 3 (Motion) adds slow wavetable movement.)" },
        { "Felt Pad", R"(Recipe: a triangle with a sine an octave up, barely detuned, a dark 12 dB lowpass, and a Tape Echo with a lot of wow and flutter (wobble), plus a little tape hiss.

Listen for: the slight pitch wobble on the echo. It makes the pad feel like an old recording, warm and human.

Try: Macro 3 (Hiss) up or down. A little hiss can make a quiet room feel more intimate.)" },
        { "Slow Rise Pad", R"(Recipe: saws (one an octave down) through a very low 24 dB lowpass (400 Hz). Env 2 has a 3 s attack and opens the filter while the amp fades in over 2.5 s: each chord grows from a dark hum to a full pad.

Listen for: hold a chord for at least four seconds. On short chords it stays dark.

Try: Macro 3 (Rise) makes the opening faster.)" },
        { "Worship Strings", R"(Recipe: the "String Ensemble" wavetable with 4 unison voices, blended with a quieter "Warm Pad" layer so it's smoother than real strings. A highpass at 160 Hz keeps it out of the bass, and velocity opens the lowpass a little.

The mod wheel adds vibrato (LFO 1 on the pitch, scaled by the wheel).

Try: Macro 3 (Pad) for more pad and less string. Bring the mod wheel in slowly on long notes.)" },
        // ----------------------------------------------------------------- evolving pads
        { "Tidal Pad", R"(Recipe: the "Warm Saw Sweep" wavetable (dark at the start of the table, bright at the end) moved by a very slow LFO (0.05 Hz: one cycle every 20 seconds), which also nudges the filter.

Listen for: the pad slowly opening and closing on its own. It doesn't restart with each chord because the LFO is free-running.

Try: Macro 3 makes the tide faster.)" },
        { "Heaven Wash", R"(Recipe: a 7-voice Super unison saw (very wide), an "Airy Pad" an octave up, a highpass to keep the low end clean, and a big shimmer reverb (the reverb adds an octave above itself).

Listen for: this is a big sound: use it for the climax, not under a quiet verse.

Try: Macro 3 (Shimmer) adds more of the octave-up shimmer.)" },
        { "Distant Choir", R"(Recipe: plain saws through the Formant filter. A formant filter shapes sound like a mouth does, so the saws sound like voices. A slow LFO moves the filter's vowel control, and a second lowpass and a big hall push it into the distance.

Listen for: the voice-like resonances that slowly change shape.

Try: Macro 3 (Vowel) changes the mouth shape by hand.)" },
        { "Shimmer Cloud", R"(Recipe: "Shimmer Evolve" and "Fibonacci Partials" wavetables (an octave up), both moved by a smooth Random source in opposite directions. Random never repeats, so the cloud never loops. Then a ping-pong echo and a big shimmer.

Try: Macro 3 makes the random movement faster: slow is calm, fast is restless.)" },
        { "Moving Fifths", R"(Recipe: a warm saw pad on Engine 1, and a second saw a fifth up on Engine 2. A slow triangle LFO fades Engine 2's volume in and out, so the fifth comes and goes.

Listen for: hold one chord: it changes colour by itself, from plain to "open" and back.

Try: Macro 3 changes how fast the fifth comes and goes.)" },
        { "Tape Memory", R"(Recipe: the "Vintage Saw" wavetable with a slow LFO wobbling its pitch very slightly (like a stretched tape), "Tape Hiss" and "Vinyl Dust" noise, and a Tape Echo with lots of wow.

Listen for: the wobble is tiny. Any more and it would sound out of tune with the band.

Try: Macro 3 (Age) brings up the hiss and dust.)" },
        { "Sparkle Pad", R"(Recipe: the "Warm Pad" wavetable with "Sparkle Air" noise on top. The noise is high-passed (Noise filter turned to HP), so it only adds glitter, not hiss. A dotted-eighth ping-pong echo and shimmer.

Try: Macro 3 (Sparkle) for more glitter. Macro 1 down for a warmer pad with sparkle still on top.)" },
        // ----------------------------------------------------------------- drones
        { "Root Drone", R"(Recipe: just three oscillators: a sine at the note, a sine an octave down and a quiet triangle an octave up. A slow LFO breathes the filter a little. Long attack and release.

Use: hold the key note of the song (or the set) from start to finish. Transitions between songs in the same key become seamless.

Try: Macro 3 (Octave) brings in the upper octave.)" },
        { "Fifth Drone", R"(Recipe: Chord-mode unison (Fifth + Oct) on a saw and a triangle, through a very dark lowpass (700 Hz), into shimmer. One key = root, fifth and octave.

Use: like Root Drone but richer. Hold the song's key note; it fits major and minor because there's no third.

Try: Macro 3 (Motion) adds a slow filter movement.)" },
        { "Cello Drone", R"(Recipe: the "String Ensemble" wavetable an octave down, a 24 dB lowpass at 900 Hz and a vibrato that fades in after 1.5 s (LFO fade), like a cellist starting a note straight and then adding vibrato.

Try: Macro 3 (Vibrato) makes the vibrato deeper. Hold low notes under a reading.)" },
        { "Organ Drone", R"(Recipe: drawbar organ (16', 8', some 4' and a little 2') with no key click and a slow 1.5 s swell, the Rotary speaker at half mix on Slow, and a hall.

Use: under prayers and hymns in the same key. Changing only the envelope turns an organ into a drone.

Try: Macro 3 switches the rotary to Fast.)" },
        { "Dark Drone", R"(Recipe: a saw an octave down and a square two octaves down through a resonant 24 dB lowpass at 300 Hz, the "Rumble Drone" noise, and a very slow LFO on the cutoff.

Use: tension, Good Friday, dramatic readings. It's mostly felt on a PA with subs.

Try: Macro 3 (Rumble) for more noise.)" },
        { "Air Drone", R"(Recipe: a sine with a soft triangle an octave up, plus pink noise, all through the Comb filter. A comb filter adds a delayed copy of the sound to itself; when the delay matches a note's period, it rings at that pitch. Key Track is at 100 %, so the breath is tuned to whatever note you hold.

Listen for: the noise doesn't hiss, it sings along with the tone.

Try: Macro 3 (Air) for more breath around the tone.)" },
        { "Shimmer Drone", R"(Recipe: a plain sine with a triangle an octave up, fed into the shimmer reverb at a high send. The shimmer adds octaves above, so a simple tone becomes a glowing cloud.

Listen for: the drone gets brighter over the first few seconds as the shimmer builds.

Try: Macro 3 (Shimmer) for more octave-up glow.)" },
        { "Pulsing Drone", R"(Recipe: a saw and a sine an octave down, with LFO 1 synced to quarter notes controlling the volume (Amp Mod Source = LFO 1). The drone breathes in time with the song: it follows the host tempo in MainStage.

Try: Macro 3 (Pulse) makes the pulsing deeper. At zero it's a steady drone.)" },
        // ----------------------------------------------------------------- swells and transitions
        { "Riser 2 Bars", R"(Recipe: white noise and unison saws. Function 1 is a one-shot Ramp Up synced to 2 bars: it opens the filter, raises the pitch and turns up the volume (Amp Mod Source = Function 1). Each key press starts the ramp from the beginning.

Use: press on the downbeat two bars before the drop; it follows the host tempo.

Try: Macro 4 (Pitch Rise) doubles the pitch rise to an octave. Macro 3 adds noise.)" },
        { "Riser 4 Bars", R"(Recipe: the same as Riser 2 Bars but the Function lasts 4 bars, with pink noise (darker than white) and shimmer instead of echo.

Use: for longer builds, e.g. a bridge that repeats before the last chorus.

Try: Macro 4 (Pitch Rise) for a bigger climb.)" },
        { "Reverse Swell", R"(Recipe: the "Swell" noise (a rising burst that stops dead, like a reversed cymbal) plus a pad whose envelope has a 1.5 s attack with a negative curve: it starts very slowly and rushes in at the end. Sustain is zero, so the sound stops abruptly, like a reversed recording.

Use: press about one and a half beats (at 60 BPM) before the chorus.

Try: Macro 4 (Length) makes the swell longer.)" },
        { "Down Sweep", R"(Recipe: noise and saws with Function 1 as a one-shot Ramp Down over one bar: the filter closes and the pitch falls an octave while the volume decays.

Use: the bar after a big ending, or to "drop out" the band into a quiet verse.

Try: Macro 4 changes the decay time.)" },
        { "Wind Swell", R"(Recipe: unison saws and noise through a Comb filter that follows the keyboard (so the wind is tuned to the chord), then a lowpass that Env 2 opens over 3.5 s while the volume rises. Letting go fades it out over 1.5 s.

Listen for: the "whoosh" comes from the lowpass moving; the comb makes it a chord instead of hiss.

Try: Macro 3 (Wind) for more noise and less saw. Macro 4 makes the gust longer.)" },
        { "Sub Swell", R"(Recipe: a sine with a faint triangle an octave up, a 2.5 s attack and a lowpass at 400 Hz. No reverb: low frequencies in a reverb just turn into mud.

Use: low notes (one at a time) under the last chorus. It needs a PA with subs; on small speakers Macro 3 makes it audible.

Try: Macro 3 (Overtone) for the octave up.)" },
        // ----------------------------------------------------------------- bells and mallets
        { "Worship Bells", R"(Recipe: FM. Engine 1 is a sine modulated by a sine at twice its pitch (ratio 2 = sweet, bell-like). Env 2 adds extra modulation at each strike (more with velocity), so the attack is bright and the tail is pure. Engine 2 adds a short, high "tine" (ratio 7). Then a dotted echo and shimmer.

Try: Macro 1 for a brighter or softer bell. Play simple melodies an octave above the singers.)" },
        { "Soft Chimes", R"(Recipe: the "Glass" wavetable an octave up with a long decay. Random 1 is a Voice Modulator: each new note gets a different value, which moves that note's pan. So every chime lands in a different spot, like wind chimes.

Try: Macro 3 (Colour) moves through the glass table. Play random high notes slowly with the sustain pedal.)" },
        { "Kalimba", R"(Recipe: FM with an unusual ratio (5.9) that's only heard for a split second: Env 2 decays in 0.12 s. That gives the metallic "tink" of the tine, then a pure sine. A "Pluck Noise" for the thumb.

Listen for: velocity changes the tink a lot. Play softly for a gentle, woody sound.

Try: Macro 3 (Thumb) for more pluck noise.)" },
        { "Soft Vibes", R"(Recipe: a sine plus a short overtone two octaves up (from Env 3) for the mallet, and LFO 1 at 5.2 Hz controlling the volume: that's the vibraphone's motor tremolo.

Try: Macro 3 (Motor) for deeper tremolo, or turn it all the way down for a plain mallet sound.)" },
        { "Wooden Marimba", R"(Recipe: a triangle with a quick overtone two octaves up (Env 3 decays in 0.12 s) and a "Thump" noise for the mallet. Short decay (0.7 s), as wood doesn't ring.

Listen for: compare with Soft Vibes: metal rings, wood stops.

Try: Macro 3 (Mallet) for a harder mallet.)" },
        { "Hand Bells", R"(Recipe: FM with a non-whole ratio (1.4) gives a slightly inharmonic, bell-like ring. Velocity adds brightness at the strike. A sine an octave up adds the high ring, and a long hall gives the church space.

Try: Macro 3 (Ring) for more of the upper octave. Play chords slowly, like a handbell choir.)" },
        { "Bowed Glass", R"(Recipe: the "Glass" wavetable with a 0.45 s attack (bowed, not struck) and a vibrato that fades in after the note starts.

Listen for: the same glass table as Soft Chimes. Only the envelope changed, and a struck bell became a bowed one.

Try: Macro 3 (Colour) moves through the table. Macro 4 slows the attack.)" },
        // ----------------------------------------------------------------- keys, plucks and rhythm
        { "Ambient Piano", R"(Recipe: a felt-piano recipe (a triangle, a sine an octave up and a "Thump" for the hammer, with a dark filter that velocity opens) sent into lots of shimmer and a soft quarter-note echo.

Listen for: the piano itself is small and soft; the space around it is what makes it ambient.

Try: Macro 3 (Shimmer) for more octave-up glow. Play slow, open voicings with space between them.)" },
        { "Dotted Pluck", R"(Recipe: a short saw-and-square pluck (Env 2 opens and closes the filter in 0.25 s) into a delay set to a dotted eighth, with high feedback.

Play quarter notes: the echo lands between them, and together they make the rolling rhythm you hear in a lot of worship guitar parts. The delay follows the song tempo.

Try: Macro 3 (Echo) for a louder echo.)" },
        { "Soft Synth Keys", R"(Recipe: a pulse wave (35 % width) with a triangle an octave up, through a lowpass that velocity and Env 2 open at each note, a sustain at 45 % and Vintage Chorus II.

Use: a softer, synthier alternative to an electric piano for chords.

Try: Macro 3 (Chorus) changes the chorus amount.)" },
        { "Worship Arp", R"(Recipe: the arpeggiator (Up, 2 octaves, eighth notes) playing a triangle-and-saw pluck, with a dotted-eighth ping-pong echo. The echo and the eighths cross over each other, which makes the pattern sound more complex than it is.

Hold a chord: the arp follows the host tempo.

Try: Macro 4 (Gate) for longer notes. Change the arp order on the SEQ page.)" },
        { "Sixteenth Shimmer", R"(Recipe: a quiet FM bell arpeggiated in 16ths, up and down over three octaves, with echo and shimmer. It's meant to sit low in the mix as a glittering bed, not a lead.

Try: hold a chord through a build and slowly bring up Macro 2 (Space).)" },
        { "Eighth Pulse Pad", R"(Recipe: a warm unison saw pad whose volume is chopped by Function 1 (a square shape synced to eighth notes, smoothed a little so it doesn't click). Amp Mod Source = Function 1.

Listen for: the band gets an eighth-note drive even though you only hold chords.

Try: Macro 3 (Steady) turns the pulsing down until it's a normal pad: ride it up for verses, down for choruses, or the other way round.)" },
        { "Pumping Pad", R"(Recipe: a pad whose volume follows Function 1 set to the "Duck" shape, synced to quarter notes: it dips at the start of every beat and swells back, like a pad side-chained to a kick drum.

Try: Macro 3 (Pump) for a deeper dip. Works best on songs with a steady four-on-the-floor kick.)" },
        { "Heartbeat Sub", R"(Recipe: a sine sub played by the sequencer: 16 steps of sixteenths, with notes only on step 1 and step 4 (the second one softer). That's "lub-dub", once a bar.

Use: hold one low note under a quiet moment or a prayer. It follows the song tempo.

Try: change which steps play on the SEQ page. Macro 3 (Overtone) makes it audible on small speakers.)" },
        { "Soft Sub Bass", R"(Recipe: a sine with a quiet triangle an octave up, mono with a short glide, and almost no velocity response (a sub should stay even).

Use: when there's no bass player. Play the root notes with your left hand.

Try: Macro 3 (Overtone) to hear it on small speakers. Macro 4 for a longer release between notes.)" },
        // ----------------------------------------------------------------- atmosphere
        { "Prayer Atmosphere", R"(Recipe: two soft pads ("Airy Pad" and "Warm Pad" wavetables) with a touch of "Wind" and "Air" noise, a slow LFO on the filter and a big hall.

Use: one held note or a simple chord during prayer, communion or a reading: it fills the silence without drawing attention.

Try: Macro 3 (Wind) to bring the wind forward for a more atmospheric moment.)" },
        { "Final Chorus Stack", R"(Recipe: everything at once: a 7-voice Super saw, a "String Ensemble" layer, a sine sub on the Utility oscillator, chorus and shimmer. Velocity opens the filter a little.

Listen for: it's loud and full on purpose. Save it for the last chorus and come back to a smaller pad after.

Try: Macro 3 (Sub) for more low end.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
