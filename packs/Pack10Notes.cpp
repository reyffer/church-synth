// "How it's built" notes for Pack 10 (shown in the preset browser).

#include "Packs.h"

#include <map>

juce::String pack10Notes (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> notes {
        // ----------------------------------------------------------------- brass
        { "Noir Brass", R"(Recipe: this pack copies the late-70s two-channel flagship. Every voice is TWO complete layers: Engine 1 through Filter 1 and Engine 2 through Filter 2 (parallel routing), each with its own 12 dB filter and its own filter envelope (Env 2 and Env 3).

TOUCH is the other half: velocity sets the brightness, and aftertouch (pressing into a held key) opens both filters and adds vibrato. No aftertouch? Macro 3 (Growl) does the same.

Here: two dark saw layers, the second slightly detuned with a slower envelope, a long release into a big hall.

Try: hold a chord and press into it, or ride Macro 3.)" },
        { "Touch Brass", R"(Recipe: a saw layer and a pulse layer an octave up with a shorter filter envelope. Velocity brightens strongly, aftertouch adds more.

Listen for: two layers with different envelopes give a brass attack that has depth: the high layer bites, the low layer holds.

Try: Macro 3 (Growl) on held notes.)" },
        { "Brass Swell 80", R"(Recipe: two layers whose filters open at different speeds: the lower layer in 0.6 s, the upper layer (an octave up) in 1.8 s. The amp also swells in over 0.5 s.

Listen for: the chord gets bigger, then brighter. Two envelopes make one long, dramatic swell.

Try: Macro 4 (Attack) for a slower swell.)" },
        { "Ring Mod Brass", R"(Recipe: a brass layer (saw, filter envelope) plus a ring-modulated layer: a sine multiplied by another sine at 2.5x its pitch. Ring modulation outputs the sum and difference of the two frequencies, which aren't musical intervals: hence the metallic edge.

Try: Macro 3 (Growl), and compare by turning Engine 2 off.)" },
        { "Soft Horn Layer", R"(Recipe: a dark saw layer (450 Hz, gentle filter envelope) under a pure sine layer. The sine gives the round "body" of a horn; the saw gives its edge.

Use: under worship, sustained chords. Try: Macro 3 (Growl) for a more present horn.)" },
        // ----------------------------------------------------------------- pads and strings
        { "Twin Layer Strings", R"(Recipe: two saw layers 10 cents apart, each through its own filter with its own slow envelope (0.5 s and 1.2 s). The two layers drift in brightness against each other as well as in pitch.

Try: Macro 3 (Touch) for vibrato and more brightness.)" },
        { "Noir Pad", R"(Recipe: a dark saw layer with resonance (350 Hz) and a narrow pulse layer an octave up, both with very slow filter envelopes (2-3 s) and a 3-second release into a huge hall.

Use: dark, cinematic atmosphere. Try: Macro 3 (Growl).)" },
        { "Crystal Layer Pad", R"(Recipe: a warm saw layer (900 Hz) under a very narrow pulse layer (15 %) an octave up with its filter wide open. Narrow pulses are thin and glassy; on top of a warm layer they add sparkle without harshness.

Try: Macro 3 (Touch).)" },
        { "Breathing Strings", R"(Recipe: two detuned saw layers where aftertouch (or Macro 3) also raises Engine 1's volume. Press into the chord and it swells and opens; release the pressure and it falls back.

Use: play dynamics with your fingers instead of a pedal.)" },
        { "Cathedral Pad", R"(Recipe: a soft saw-and-pulse layer with a low filter and a pure sine layer an octave up, a slow 1.2 s attack and a big shimmer reverb.

Use: worship and prayer. Try: Macro 3 (Touch) for gentle vibrato.)" },
        // ----------------------------------------------------------------- keys and bells
        { "Ring Mod Bells", R"(Recipe: two ring-modulated sine layers (ratios 3.5 and 1.41, the second an octave up). Neither ratio is a musical interval, so the result is inharmonic: bells, gongs and spacey tones.

Try: Macro 1 (Ring) for more ring modulation, Macro 3 for more of the upper layer.)" },
        { "Layer Electric Piano", R"(Recipe: a struck sine-and-triangle layer (the body) plus a short, bright narrow-pulse layer an octave up (the tine), whose filter snaps shut in 0.25 s. Velocity opens the tine layer. High notes decay sooner.

Try: Macro 3 (Tine) for a louder tine.)" },
        { "Touch Clav", R"(Recipe: a narrow pulse layer (15 %) with resonance and a snappy filter envelope, plus a short saw layer an octave up. Velocity and aftertouch both open the filters: press into a note after striking it and it brightens.

Try: Macro 3 (Touch).)" },
        { "Glass Harp", R"(Recipe: a sine layer and a slightly detuned triangle layer an octave up, filters open, a 0.35 s attack. Simple tones, slow bow, big space.

Try: Macro 3 (Touch) adds a gentle vibrato.)" },
        // ----------------------------------------------------------------- leads
        { "Ribbon Lead", R"(Recipe: a saw layer and a detuned pulse layer, mono and legato, with the pitch bend set to 7 semitones up and down (the original had a ribbon for big slides). Aftertouch or Macro 3 adds vibrato and brightness.

Try: slide up into notes with the pitch wheel.)" },
        { "Touch Lead", R"(Recipe: two saw layers an octave apart through fairly dark, resonant filters. Velocity and aftertouch open them a long way: soft and round when you play gently, growling and singing when you press in.

Try: Macro 3 (Growl) if your keyboard has no aftertouch.)" },
        { "Soaring Sine Lead", R"(Recipe: a pure sine layer over a soft pulse layer, with a slow vibrato that fades in, legato with a long glide (80 ms) and a long echo.

Use: floating melodies over worship.)" },
        { "Sub-Osc Trill Lead", R"(Recipe: LFO 2 is a square wave at 8 Hz moving both layers' pitch by a whole tone, scaled by aftertouch (or Macro 3). Press into a held note and it trills.

Try: Macro 3 (Trill) on held notes.)" },
        // ----------------------------------------------------------------- bass and arp
        { "Twin Bass", R"(Recipe: a low layer (saw with a sine sub an octave down, low filter) and a short, bright pulse layer on top whose filter closes in 0.12 s. The top layer gives the attack, the low layer gives the weight.

Try: Macro 3 (Attack Layer) for more click.)" },
        { "Layer Arp", R"(Recipe: a saw pluck layer and a glassy narrow-pulse layer an octave up, played by the arpeggiator up and down two octaves in 16ths, with an echo.

Use: hold a chord; it follows the host tempo. Try: Macro 3 (Glass Layer).)" },
    };
    const auto it = notes.find (presetName);
    return it == notes.end() ? juce::String() : juce::String (juce::CharPointer_UTF8 (it->second));
}
