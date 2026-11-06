// "How it's built" notes for Pack 14 (shown in the preset browser).
// Each note is a small sound-design lesson.

#include "Packs.h"

#include <map>

juce::String pack14Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- nature
        { "Thunder", R"(Recipe: no oscillators at all, only noise. Brown noise (deep) through a lowpass that Env 2 opens wide at the start and closes over 1.5 s: the crack becomes a rumble. White noise is faded in only for the first quarter-second by Env 3 (the crack). Random 1, smooth and fast, wobbles the volume so the rumble rolls unevenly.

Lesson: one envelope on a filter turns a burst into a distant roll.

Try: Macro 3 (Rumble), Macro 4 (Length).)" },
        { "Rain", R"(Recipe: the "Rain" noise sample, with a little wide pink noise underneath for body, through a gentle lowpass. A slow fade in and out.

Lesson: real-world textures are usually two layers: the detail (drops) and the bed (wash).

Try: Macro 3 (Downpour) brings up the wash.)" },
        { "Howling Wind", R"(Recipe: "Wind" and white noise through a resonant bandpass that follows the keyboard 100 %, so the note you play sets the whistle's pitch. A slow, smooth Random source moves the band and the volume: gusts.

Lesson: resonance on a bandpass turns noise into a pitched whistle.

Try: Macro 3 (Whistle), Macro 4 (Gust Speed). Play different notes.)" },
        { "Ocean Waves", R"(Recipe: "Ocean" and pink noise through a lowpass. One slow LFO (one cycle every 8 s) opens the filter AND raises the volume at the same time: a wave arriving, then pulling back.

Lesson: moving brightness and loudness together sounds physical.

Try: Macro 4 (Wave Speed).)" },
        { "Campfire", R"(Recipe: the "Fire" noise for the roar and "Vinyl Crackle" for the pops, through a gentle lowpass.

Lesson: crackle noise isn't only for lo-fi; it's great for any sound with random clicks.

Try: Macro 3 (Crackle).)" },
        { "Distant Storm", R"(Recipe: rain on Noise 1 and the "Rumble Drone" on Noise 2. A very slow Random source raises and lowers the rumble's volume, so it comes and goes like distant thunder.

Try: Macro 3 (Rumble).)" },
        // ----------------------------------------------------------------- sci-fi
        { "Laser Zap", R"(Recipe: a saw and a pulse an octave up, with a PITCH ENVELOPE: Env 3 starts the pitch three octaves higher and brings it down in 0.18 s. A fast pitch drop is the "pew"; a short echo adds the ricochet.

Lesson: pitch envelopes are behind lasers, kicks, toms and zaps.

Try: Macro 3 (Dive) for a slower dive.)" },
        { "UFO Hover", R"(Recipe: a sine ring-modulated by a sine 1.5x its frequency (metallic), high up, with a fast 7 Hz vibrato and a very slow LFO drifting the ring-mod ratio. A phaser on top.

Lesson: ring modulation + vibrato = instant 1950s sci-fi.

Try: Macro 3 (Wobble), Macro 4 (Metal).)" },
        { "Alarm Siren", R"(Recipe: a saw and a square whose pitch is moved by a slow triangle LFO, up a fifth and back (0.4 Hz).

Lesson: an LFO on pitch with a big depth isn't vibrato any more: it's a siren.

Try: Macro 3 (Wail Speed). Change the LFO to Square for a two-tone siren.)" },
        { "Robot Chatter", R"(Recipe: Random 1 and Random 2 in Sample & Hold mode at 9 Hz: every 110 ms they jump to a new value. One moves the pitch, the other moves a vowel (Formant) filter. Stepped pitch + stepped vowels = a robot "talking".

Lesson: Sample & Hold is the sound of 70s computers.

Try: Macro 3 (Talk Speed).)" },
        { "Teleport", R"(Recipe: sines and triangles with unison and noise, a fast shimmering vibrato, and Function 1 as a one-shot ramp lifting the pitch two octaves and opening the filter over 2 s, into a shimmer reverb.

Lesson: a one-shot Function is a long, custom envelope.

Try: Macro 4 (Rise Time).)" },
        { "Computer Bleeps", R"(Recipe: a square wave. Random 1 in Sample & Hold, synced to sixteenths, jumps the pitch around, and Engine 1's Quantize keyboard only allows C, E and G, so the random notes always form a major chord. Function 1 ("Trance Gate") chops the volume into beeps.

Lesson: quantising random pitch makes it musical.

Try: change the quantize notes on Engine 1.)" },
        { "Power Down", R"(Recipe: a one-shot Function ramps DOWN two octaves of pitch and closes the filter over 1.5 s while the volume fades linearly. Like a machine losing power.

Try: Macro 3 (Drop) for a slower wind-down.)" },
        // ----------------------------------------------------------------- transitions and impacts
        { "Sub Drop Impact", R"(Recipe: a sine an octave down with a pitch envelope that starts two octaves higher and dives over half a second, plus a "Thump" noise for the hit, into a huge reverb. The cinematic "boom".

Lesson: the same trick as a kick drum, slowed down.

Try: Macro 3 (Punch) for more thump. Needs subs.)" },
        { "Whoosh Pass", R"(Recipe: white noise through a bandpass. The amp and filter envelopes rise with an accelerating curve, then fall: loud and bright as it passes, quieter and darker as it goes. A one-shot Function pans it from left to right.

Lesson: pan + brightness + volume moving together = motion past you.

Try: Macro 3 (Whistle), Macro 4 (Length).)" },
        { "Reverse Crash", R"(Recipe: white noise through a highpass with an attack that starts slowly and rushes at the end (negative attack curve), then stops in 50 ms. The "Swell" noise adds body.

Lesson: a reversed sound is mostly a curved attack and a sudden end.

Try: Macro 4 (Length).)" },
        { "Noise Sweep Up", R"(Recipe: white noise through a resonant highpass whose cutoff is swept up by a one-shot Function synced to two bars, with the volume rising too. An eighth-note echo thickens it.

Try: Macro 3 (Resonance) for a whistling riser.)" },
        // ----------------------------------------------------------------- oddities
        { "Clock Ticks", R"(Recipe: the "Click" transient through a resonant bandpass, played by the sequencer on quarter notes: step 1 is the tick, step 2 is a fourth higher and softer (the tock). The click follows the keyboard, so higher keys = smaller clock.

Lesson: the sequencer isn't only for basslines.

Try: Macro 3 (Wood) for a duller, wooden tick.)" },
        { "Radio Tuning", R"(Recipe: "Static" and "Digital Static" noise through a bandpass, plus a quiet sine. One smooth Random source moves both the bandpass and the sine's pitch, like turning the dial: the static changes colour and whistles drift through.

Try: Macro 3 (Tuning Speed), Macro 4 (Whistle).)" },
        { "Underwater Bubbles", R"(Recipe: a sine with a tiny upward pitch blip on each note (Env 3, a 60 ms sweep up from an octave below), played by the arpeggiator in RANDOM order, with steps that only play 70 % of the time (probability on the SEQ page). A dark filter and a chorus make it watery.

Lesson: probability and random order make a pattern sound natural.

Try: hold a chord; Macro 3 (Pop) changes the bubble size.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
