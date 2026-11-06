// "How it's built" notes for Pack 3 (shown in the preset browser).

#include "Packs.h"

#include <map>

juce::String pack3Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- tine pianos
        { "Tine I Stage", R"(Recipe: every tine piano in this pack has three parts. (1) The body: a sine with a little phase modulation from a sine at the same pitch (Engine 1). (2) The bark: Env 2 adds more of that modulation at the strike, scaled by velocity, so hard notes growl. (3) The tine: Engine 2, a sine modulated at 14x the pitch, heard for only 0.2 s. Velocity also adds filter drive (pickup grit).

"Tine I" is the earlier, barkier kind: more bark, a darker filter (3.2 kHz).

Try: play one note from very soft to very hard. Macro 3 (Grit) adds drive.)" },
        { "Tine I Suitcase", R"(Recipe: Tine I Stage through a Panner (the stereo auto-pan of the suitcase amp) and a light Tube distortion for the amp's warmth.

Listen for: the sound moves left and right. With the MONO button on (top bar) the pan becomes a volume tremolo, which is how it reaches a mono PA.

Try: Macro 3 (Pan) for a deeper pan.)" },
        { "Tine I Ballad", R"(Recipe: Tine I with less bark (0.2), a darker filter (2.4 kHz) and less grit, plus a light chorus and a hall.

Listen for: compare with Tine I Stage. The same piano with the edges taken off sits behind a singer instead of next to them.

Try: Macro 3 (Chorus) for more shimmer.)" },
        { "Tine I Growl", R"(Recipe: Tine I with the bark and the grit pushed up (0.5 and 0.6), then an Overdrive on FX bus A.

Listen for: at soft velocities it's still clean. The growl only appears when you dig in, so your playing controls the distortion.

Try: Macro 3 (Drive) for more overdrive.)" },
        { "Tine Lo-Fi", R"(Recipe: a dark Tine I (2.2 kHz) through a Tape Echo with lots of wow and flutter, plus a little "Tape Hiss" noise.

Listen for: the echo wobbles in pitch. That small instability is what makes it sound like an old recording.

Try: Macro 3 (Hiss) up or down.)" },
        { "Tine II Stage", R"(Recipe: the same three-part tine recipe as Tine I, set the way the later pianos sound: less bark (0.22), a louder and longer tine (0.5 for 0.3 s), a more open filter (7 kHz) and less grit. Cleaner and more bell-like.

Listen for: A/B it with Tine I Stage. Tine I growls; Tine II rings.

Try: Macro 1 (Bark) up to move it towards Tine I.)" },
        { "Tine II Phaser", R"(Recipe: Tine II through a slow phaser (rate low, depth half, mix 45 %).

Listen for: the slow "swoosh" through the chords. A phaser on a clean tine piano is the 70s soul and jazz-funk sound.

Try: Macro 3 (Phase) for a deeper phaser.)" },
        { "Tine II Chorus", R"(Recipe: Tine II with a chorus (45 %), a soft dotted-eighth ping-pong echo and a little hall.

Use: modern pop and worship ballads, where the electric piano is wide and glossy.

Try: Macro 3 (Chorus) changes the chorus amount.)" },
        { "Tine II Bell", R"(Recipe: Tine II with the tine louder (0.75) and longer (0.6 s), so the bell on top of each note rings out.

Listen for: high voicings sparkle; low notes still have body.

Try: Macro 3 (Bell) adds more modulation to the tine, making it brighter and more metallic.)" },
        // ----------------------------------------------------------------- FM pianos
        { "FM Ballad EP", R"(Recipe: the classic 80s FM electric piano is two pairs of sine waves. Engine 1: a carrier with a modulator at the same pitch (1:1) for the body; Env 2 adds a velocity "bark". Engine 2: a carrier with a modulator at 14x the pitch for the glassy tine, faded quickly by Env 3. Engine 2 is detuned 7 cents, so the two pairs beat gently.

Listen for: the beating gives FM pianos their shimmer even without chorus.

Try: Macro 3 (Chorus) on top of that.)" },
        { "FM Glass EP", R"(Recipe: the FM piano with the tine louder (0.65) and much longer (0.8 s), a little more detune (8 cents), a ping-pong echo and a hall.

Listen for: the glassy ring that hangs over each chord.

Try: Macro 3 (Glass) makes the tine ring even longer.)" },
        { "FM Soft EP", R"(Recipe: the FM piano with very little modulation (the body is almost a pure sine), a quiet tine and a low filter (3.5 kHz).

Use: under singers in quiet songs. Less modulation = fewer harmonics = softer.

Try: Macro 1 (Bark) slowly up to hear the FM come in.)" },
        { "FM Bright EP", R"(Recipe: the FM piano with more modulation everywhere (body 0.18, bark 0.35, tine 0.55) and the filter wide open.

Use: when the electric piano has to cut through a full band.

Try: Macro 1 (Bark) down for verses.)" },
        { "FM Bell EP", R"(Recipe: the tine pair (Engine 2) uses a 3.5 ratio instead of 14. Non-whole ratios make inharmonic overtones, which our ears hear as "bell". It also rings longer (1.4 s).

Listen for: a piano and a bell struck together.

Try: Macro 3 (Bell) for a louder, brighter bell.)" },
        { "FM Wide EP", R"(Recipe: the FM Ballad piano with more detune (10 cents), Vintage Chorus I + II and a dotted-eighth ping-pong echo.

Listen for: very wide in headphones. On a mono PA it's still solid because the chorus width is held at 80 %.

Try: Macro 3 (Chorus) for the chorus mix.)" },
        { "FM Keys and Pad", R"(Recipe: here the amp envelope sustains, and the piano's own decay comes from Env 2 on Engine 1's volume (4.5 s). That leaves room for a "Warm Pad" on Engine 2, which Env 3 fades in over 1.8 s.

Listen for: tap short chords for piano; hold them and the pad rises underneath.

Try: Macro 3 (Pad) for a louder pad.)" },
        { "FM Punch EP", R"(Recipe: the FM piano with a very fast, strong bark (Env 2 decays in 0.6 s) and a high velocity range (Amp Mod 85 %).

Listen for: soft notes are gentle and round; hard notes jump out with a bright attack. Your touch is the main control.

Try: Macro 3 (Dynamics) for even more velocity range.)" },
        { "FM Velvet EP", R"(Recipe: the FM piano with a triangle modulator instead of a sine (Engine 1's modulator wave). A triangle has a few more harmonics, so the body is slightly richer but still soft. A darker filter (4 kHz) and a slow stereo Panner.

Try: Macro 3 (Pan) for a deeper auto-pan.)" },
        { "FM Soul EP", R"(Recipe: the FM piano through three effects in a row on bus A: chorus, a gentle Panner and a little Tube distortion.

Listen for: the tube adds warmth more than distortion. Three subtle effects can add up to a big character change.

Try: Macro 3 (Pan) for more movement.)" },
        { "FM Hollow EP", R"(Recipe: the body pair uses a 3:1 modulator instead of 1:1. That changes which harmonics appear and gives a hollow, reedy body. A slow phaser on top.

Try: compare with FM Ballad EP: one number changed (the body ratio) and the whole character moved.)" },
        // ----------------------------------------------------------------- reed pianos
        { "Reed Classic", R"(Recipe: a triangle plus a narrow pulse (28 % width), the nasal reed tone. A 12 dB lowpass at 1.8 kHz that Env 2 and velocity open at each strike, and velocity-driven filter drive for the raspy bite.

Listen for: soft = sweet and round, hard = nasal and gritty. That change is the reed piano's character.

Try: Macro 3 (Grit) for more rasp at every velocity.)" },
        { "Reed Tremolo", R"(Recipe: Reed Classic with its built-in tremolo: LFO 1 (5.6 Hz) dips Engine 1's volume. Because the tremolo is on the engine volume (not the Amp Mod), velocity still controls the loudness.

Try: Macro 3 (Tremolo) for a deeper tremolo.)" },
        { "Reed Driven", R"(Recipe: a slightly brighter reed with more grit (0.6) into an Overdrive on bus A.

Use: rock and blues rhythm parts. Play with a hard touch.

Try: Macro 3 (Drive) for more overdrive.)" },
        { "Reed Soft", R"(Recipe: a reed with less pulse (more triangle), a dark filter (1.2 kHz), almost no grit, a light chorus and a hall.

Use: quiet songs, where a reed piano adds warmth without the bite.

Try: Macro 1 (Brightness) up for verses that need a bit more presence.)" },
        { "Reed Phaser", R"(Recipe: Reed Classic with a slight tremolo and a slow phaser.

Try: Macro 3 (Phase) for a deeper phaser.)" },
        // ----------------------------------------------------------------- clav
        { "Clav Funk", R"(Recipe: a very narrow pulse wave (15 % width) with a little saw. Narrow pulses are missing some harmonics, the same way a pickup placed along a string misses some, which gives the thin, "picky" clav tone. A "Pluck Noise" for the hammer, a snappy filter envelope (Env 2, velocity-scaled), a highpass to keep it thin, and a 50 ms release so notes stop sharply.

Try: play short, rhythmic sixteenths. Macro 3 (Bite) adds filter resonance.)" },
        { "Clav Muted", R"(Recipe: Clav Funk with a 25 % pulse and no sustain: each note dies in about a quarter of a second, like a muted string.

Try: Macro 4 (Length) for longer notes.)" },
        { "Clav Wah", R"(Recipe: the clav through a bandpass filter (instead of a lowpass) with resonance. Env 2 sweeps the band up at each note and back down: an "envelope wah", like a guitar auto-wah pedal. Velocity scales the sweep.

Listen for: every note says "wow". Harder notes say it louder.

Try: Macro 3 (Wah) for more resonance.)" },
        { "Clav Phaser", R"(Recipe: Clav Funk through a deep phaser (depth 60 %, mix 50 %).

Try: Macro 3 (Phase) for more swirl.)" },
        { "Clav Mellow", R"(Recipe: the clav with a wider pulse (40 %, closer to a square, so rounder), a darker filter, less envelope and a light chorus.

Use: rhythm parts in softer songs, where a bright clav would be too much.

Try: compare the pulse width with Clav Funk (15 %) by ear: that one number changes the tone a lot.)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
