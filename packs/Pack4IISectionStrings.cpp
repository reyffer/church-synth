// Preset Pack 4 II: Orchestral Strings, rebuilt on the Section engine (v0.6).
//
// The same twenty sounds as Pack 4 (which stays as it was), now played by the
// Section engine: several independent players per note, each with their own tuning,
// vibrato, timing and seat, through a violin / viola / cello / bass body. Names end
// in "II" so both packs can sit side by side in the browser.
//
// Macros: M1 Brightness, M2 Space, M3 Vibrato, M4 Attack (or Length / another
// character control). The mod wheel is Dynamics everywhere, as in orchestral
// libraries: at rest the section plays mezzo-forte (or softer where noted), up = louder,
// brighter and with more vibrato.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
// Strings on engine e: body, players, seat.
void strings (B& b, int e, int body, int players, float vol, float pos, float width, float coarse = 0.0f)
{
    ensemble (b, e, SectionStrings, body, players, vol);
    sec (b, e, "pos", pos);
    sec (b, e, "width", width);
    b.set ("e" + juce::String (e + 1) + "_coarse", coarse);
}

// Violins on Engine 1 (left of centre), cellos an octave down on Engine 2 (right).
void fullSection (B& b, float violins = 0.7f, float cellos = 0.55f)
{
    strings (b, 0, SectionBodyEnsemble, 8, violins, -0.3f, 0.7f);
    strings (b, 1, SectionBodyCello, 6, cellos, 0.35f, 0.5f, -12.0f);
}

// The bow: the engine's Attack shapes each player's start; Env Amp only ends the note.
void bowed (B& b, float release, float velo = 0.25f)
{
    env (b, "env1", 0.005f, 1.0f, 1.0f, release);
    b.set ("amp_velo", velo);
}

void bothSec (B& b, bool two, const char* id, float v)
{
    sec (b, 0, id, v);
    if (two)
        sec (b, 1, id, v);
}

void wheel (B& b, bool two, float base, float range)
{
    wheelDynamics (b, 0, base, range);
    if (two)
        wheelDynamics (b, 1, base, range);
}

void orchHall (B& b, float send) { hall (b, send, 0.85f, 0.75f); }

// M1 Brightness, M2 Space, M3 Vibrato, M4 = m4 on dest by amount (both engines when two).
void orchMacros (B& b, bool two, const char* m4, const char* m4dest, float m4amt)
{
    macros (b, "Brightness", "Space", "Vibrato", m4);
    b.route ("Macro 1", "e1_sec_bright", 0.4f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_sec_vib_depth", 0.5f);
    if (two)
        b.route ("Macro 1", "e2_sec_bright", 0.4f).route ("Macro 3", "e2_sec_vib_depth", 0.5f);
    if (m4dest != nullptr)
    {
        // Section controls belong to an engine (e1_ / e2_); anything else (env2_a, fx_a1_p11) is global.
        const juce::String d (m4dest);
        const bool perEngine = d.startsWith ("sec_");
        b.route ("Macro 4", perEngine ? "e1_" + d : d, m4amt);
        if (two && perEngine)
            b.route ("Macro 4", "e2_" + d, m4amt);
    }
}

// Solo player: one player, legato with a slur, aftertouch adds vibrato.
void soloist (B& b, float glide)
{
    sec (b, 0, "players", 1.0f);
    sec (b, 0, "width", 0.0f);
    mono (b, true, glide);
    b.route ("Aftertouch", "e1_sec_vib_depth", 0.4f);
}
} // namespace

const std::vector<FactoryPreset>& pack4IIPresets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ FULL SECTION
        { "Full Strings II", "Strings/Brass", "The whole section from one key: eight violins left, six cellos an octave down on the right. Mod wheel = dynamics.",
          [] (B& b) {
              fullSection (b);
              wheel (b, true, 0.55f, 0.45f);
              bothSec (b, true, "attack", 0.35f);
              bothSec (b, true, "vib_delay", 0.4f);
              bowed (b, 0.9f);
              orchHall (b, 0.4f);
              orchMacros (b, true, "Attack", "sec_attack", 0.5f);
          } },
        { "Full Strings Marcato II", "Strings/Brass", "Full section, every note dug in with the bow: a hard attack and bite, then it settles.",
          [] (B& b) {
              fullSection (b);
              wheel (b, true, 0.6f, 0.4f);
              bothSec (b, true, "attack", 0.95f);
              bothSec (b, true, "bite", 0.8f);
              bothSec (b, true, "loose", 0.2f);
              env (b, "env1", 0.003f, 0.45f, 0.6f, 0.5f);
              b.set ("amp_velo", 0.5f);
              b.route ("Velocity", "e1_sec_bite", 0.3f).route ("Velocity", "e2_sec_bite", 0.3f);
              orchHall (b, 0.38f);
              orchMacros (b, true, "Bite", "sec_bite", -0.6f);
          } },
        { "Full Strings Dynamic II", "Strings/Brass", "The full section from a whisper to fortissimo on the mod wheel. Wheel down = pianissimo.",
          [] (B& b) {
              fullSection (b, 1.0f, 0.8f);
              wheel (b, true, 0.12f, 0.88f);
              bothSec (b, true, "attack", 0.3f);
              bothSec (b, true, "vib_depth", 0.55f);
              bowed (b, 1.0f, 0.15f);
              orchHall (b, 0.42f);
              orchMacros (b, true, "Attack", "sec_attack", 0.5f);
          } },
        // ================================================================ SECTIONS
        { "Violins Ensemble II", "Strings/Brass", "Eight violins on a violin body, seated left: bright, with a vibrato that blooms on long notes.",
          [] (B& b) {
              strings (b, 0, SectionBodyViolin, 8, 0.75f, -0.35f, 0.6f);
              b.set ("e2_on", 0.0f);
              wheel (b, false, 0.55f, 0.45f);
              sec (b, 0, "vib_depth", 0.55f);
              sec (b, 0, "vib_delay", 0.5f);
              sec (b, 0, "attack", 0.4f);
              bowed (b, 0.8f);
              orchHall (b, 0.4f);
              orchMacros (b, false, "Attack", "sec_attack", 0.5f);
          } },
        { "Violins Con Sordino II", "Strings/Brass", "Muted violins: veiled, breathy and soft. Beautiful under quiet moments.",
          [] (B& b) {
              strings (b, 0, SectionBodyViolin, 8, 1.0f, -0.3f, 0.6f);
              b.set ("e2_on", 0.0f);
              wheel (b, false, 0.38f, 0.4f);
              sec (b, 0, "bright", 0.2f);
              sec (b, 0, "noise", 0.6f);
              sec (b, 0, "vib_depth", 0.35f);
              sec (b, 0, "attack", 0.2f);
              sec (b, 0, "body_mix", 0.9f);
              bowed (b, 1.0f, 0.15f);
              orchHall (b, 0.45f);
              orchMacros (b, false, "Air", "sec_noise", 0.35f);
          } },
        { "Violins Octaves II", "Strings/Brass", "First violins and seconds an octave apart, both sections seated left: soaring lines in one hand.",
          [] (B& b) {
              strings (b, 0, SectionBodyViolin, 6, 0.65f, -0.45f, 0.45f);
              strings (b, 1, SectionBodyViolin, 6, 0.55f, -0.15f, 0.45f, 12.0f);
              wheel (b, true, 0.55f, 0.45f);
              bothSec (b, true, "vib_depth", 0.55f);
              bothSec (b, true, "attack", 0.4f);
              sec (b, 1, "bright", 0.4f);
              bowed (b, 0.8f);
              orchHall (b, 0.42f);
              orchMacros (b, true, "Attack", "sec_attack", 0.5f);
          } },
        { "Violas Section II", "Strings/Brass", "Six violas in the middle of the stage: darker and warmer than violins. The inner voice.",
          [] (B& b) {
              strings (b, 0, SectionBodyViola, 6, 0.75f, 0.0f, 0.5f);
              b.set ("e2_on", 0.0f);
              wheel (b, false, 0.55f, 0.45f);
              sec (b, 0, "bright", 0.4f);
              sec (b, 0, "attack", 0.4f);
              bowed (b, 0.8f);
              orchHall (b, 0.4f);
              orchMacros (b, false, "Attack", "sec_attack", 0.5f);
          } },
        { "Cellos Section II", "Strings/Brass", "Six cellos on the right with a bit of bite. Play it in the low-mid range.",
          [] (B& b) {
              strings (b, 0, SectionBodyCello, 6, 0.75f, 0.4f, 0.5f);
              b.set ("e2_on", 0.0f);
              wheel (b, false, 0.55f, 0.45f);
              sec (b, 0, "bite", 0.45f);
              sec (b, 0, "attack", 0.45f);
              sec (b, 0, "vib_depth", 0.5f);
              bowed (b, 0.8f);
              orchHall (b, 0.38f);
              orchMacros (b, false, "Bite", "sec_bite", 0.4f);
          } },
        { "Celli and Basses II", "Strings/Brass", "Cellos with the basses an octave below: a deep, warm low end for the left hand.",
          [] (B& b) {
              strings (b, 0, SectionBodyCello, 6, 0.7f, 0.35f, 0.5f);
              strings (b, 1, SectionBodyBass, 4, 0.6f, 0.6f, 0.3f, -12.0f);
              wheel (b, true, 0.55f, 0.45f);
              bothSec (b, true, "attack", 0.4f);
              sec (b, 1, "vib_depth", 0.25f);
              bowed (b, 0.9f);
              orchHall (b, 0.38f);
              orchMacros (b, true, "Attack", "sec_attack", 0.5f);
          } },
        // ================================================================ SHORT NOTES
        { "Basses Pizzicato II", "Bass", "Plucked double basses through a bass body: round and woody. Walking or held bass lines.",
          [] (B& b) {
              strings (b, 0, SectionBodyBass, 3, 0.8f, 0.2f, 0.3f);
              b.set ("e2_on", 0.0f);
              sec (b, 0, "dyn", 0.65f);
              sec (b, 0, "attack", 1.0f);
              sec (b, 0, "bite", 0.6f);
              sec (b, 0, "vib_depth", 0.0f);
              sec (b, 0, "noise", 0.15f);
              sec (b, 0, "body_mix", 0.85f);
              sec (b, 0, "loose", 0.15f);
              env (b, "env1", 0.001f, 0.7f, 0.0f, 0.5f);
              b.set ("amp_velo", 0.6f);
              b.route ("Velocity", "e1_sec_dyn", 0.3f);
              orchHall (b, 0.25f);
              orchMacros (b, false, "Length", nullptr, 0.0f);
              b.route ("Macro 4", "env1_d", 0.35f).route ("Macro 4", "env1_r", 0.3f);
          } },
        { "Pizzicato Section II", "Pluck", "The whole section plucking: violins with cellos underneath. Light and playful.",
          [] (B& b) {
              fullSection (b, 0.75f, 0.6f);
              bothSec (b, true, "dyn", 0.6f);
              bothSec (b, true, "attack", 1.0f);
              bothSec (b, true, "bite", 0.7f);
              bothSec (b, true, "vib_depth", 0.0f);
              bothSec (b, true, "noise", 0.1f);
              bothSec (b, true, "loose", 0.3f);
              env (b, "env1", 0.001f, 0.35f, 0.0f, 0.3f);
              b.set ("amp_velo", 0.6f);
              b.route ("Velocity", "e1_sec_dyn", 0.3f).route ("Velocity", "e2_sec_dyn", 0.3f);
              orchHall (b, 0.35f);
              orchMacros (b, true, "Length", nullptr, 0.0f);
              b.route ("Macro 4", "env1_d", 0.35f).route ("Macro 4", "env1_r", 0.3f);
          } },
        { "Staccato Section II", "Strings/Brass", "Short, bouncing full-section notes for rhythmic and cinematic figures.",
          [] (B& b) {
              fullSection (b);
              bothSec (b, true, "dyn", 0.65f);
              bothSec (b, true, "attack", 1.0f);
              bothSec (b, true, "bite", 0.6f);
              bothSec (b, true, "vib_depth", 0.0f);
              bothSec (b, true, "loose", 0.15f);
              env (b, "env1", 0.002f, 0.2f, 0.0f, 0.15f);
              b.set ("amp_velo", 0.6f);
              b.route ("Velocity", "e1_sec_dyn", 0.3f).route ("Velocity", "e2_sec_dyn", 0.3f);
              orchHall (b, 0.35f);
              orchMacros (b, true, "Length", nullptr, 0.0f);
              b.route ("Macro 4", "env1_d", 0.3f).route ("Macro 4", "env1_r", 0.2f);
          } },
        // ================================================================ COLOURS
        { "Tremolo Build II", "Strings/Brass", "Bowed tremolo that grows from nothing to full over a few seconds. Hold the chord.",
          [] (B& b) {
              fullSection (b);
              bothSec (b, true, "dyn", 0.05f);
              bothSec (b, true, "vib_depth", 0.15f);
              bothSec (b, true, "noise", 0.45f);
              env (b, "env2", 4.0f, 1.0f, 1.0f, 1.0f);
              b.route ("Env 2", "e1_sec_dyn", 0.9f).route ("Env 2", "e2_sec_dyn", 0.9f);
              lfo (b, 0, LfoTriangle, 13.0f, false);
              bowed (b, 0.7f, 0.0f);
              b.set ("amp_mod_src", (float) SrcLfo1).set ("amp_velo", 0.65f);
              orchHall (b, 0.42f);
              orchMacros (b, true, "Build Time", "env2_a", -0.3f);
          } },
        { "Flautando Strings II", "Strings/Brass", "Bowed lightly over the fingerboard: soft, airy, flute-like. A whisper of strings.",
          [] (B& b) {
              fullSection (b, 0.75f, 0.5f);
              bothSec (b, true, "dyn", 0.3f);
              bothSec (b, true, "bright", 0.15f);
              bothSec (b, true, "noise", 0.75f);
              bothSec (b, true, "vib_depth", 0.25f);
              bothSec (b, true, "attack", 0.15f);
              bothSec (b, true, "body_mix", 0.5f);
              bowed (b, 1.2f, 0.1f);
              orchHall (b, 0.45f);
              orchMacros (b, true, "Air", "sec_noise", 0.25f);
          } },
        { "String Ostinato II", "Sequence", "Short strings repeating the held chord in eighths. Hold a chord; follows the tempo.",
          [] (B& b) {
              fullSection (b, 0.95f, 0.75f);
              bothSec (b, true, "dyn", 0.6f);
              bothSec (b, true, "attack", 1.0f);
              bothSec (b, true, "bite", 0.5f);
              bothSec (b, true, "vib_depth", 0.0f);
              bothSec (b, true, "loose", 0.1f);
              env (b, "env1", 0.003f, 0.22f, 0.0f, 0.15f);
              b.set ("amp_velo", 0.6f);
              b.route ("Velocity", "e1_sec_dyn", 0.3f).route ("Velocity", "e2_sec_dyn", 0.3f);
              b.set ("seq_on", 1.0f).set ("seq_mode", 0.0f).set ("seq_arp_order", (float) ArpUp).set ("seq_arp_octaves", 1.0f)
                  .set ("seq_rate", (float) kDiv8);
              b.pattern (gatePattern ({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0.4f));
              orchHall (b, 0.35f);
              orchMacros (b, true, "Length", nullptr, 0.0f);
              b.route ("Macro 4", "env1_d", 0.35f);
          } },
        // ================================================================ SOLO
        { "Solo Violin II", "Lead", "One violinist: legato slurs, a vibrato that blooms, more with aftertouch. Mod wheel = dynamics.",
          [] (B& b) {
              strings (b, 0, SectionBodyViolin, 1, 0.8f, -0.1f, 0.0f);
              b.set ("e2_on", 0.0f);
              soloist (b, 0.06f);
              wheel (b, false, 0.5f, 0.5f);
              sec (b, 0, "vib_depth", 0.6f);
              sec (b, 0, "vib_delay", 0.3f);
              sec (b, 0, "noise", 0.4f);
              sec (b, 0, "attack", 0.45f);
              sec (b, 0, "body_mix", 0.9f);
              bowed (b, 0.4f, 0.3f);
              orchHall (b, 0.35f);
              orchMacros (b, false, "Attack", "sec_attack", 0.5f);
          } },
        { "Solo Cello II", "Lead", "One cellist: warm and singing, legato slurs, aftertouch adds vibrato. Mod wheel = dynamics.",
          [] (B& b) {
              strings (b, 0, SectionBodyCello, 1, 1.0f, 0.1f, 0.0f);
              b.set ("e2_on", 0.0f);
              soloist (b, 0.07f);
              wheel (b, false, 0.5f, 0.5f);
              sec (b, 0, "vib_depth", 0.55f);
              sec (b, 0, "vib_rate", 0.4f);
              sec (b, 0, "vib_delay", 0.35f);
              sec (b, 0, "noise", 0.35f);
              sec (b, 0, "attack", 0.4f);
              sec (b, 0, "body_mix", 0.9f);
              bowed (b, 0.45f, 0.3f);
              orchHall (b, 0.35f);
              orchMacros (b, false, "Attack", "sec_attack", 0.5f);
          } },
        // ================================================================ WORSHIP / CINEMATIC
        { "Chamber Quartet II", "Strings/Brass", "A small group instead of a section: two violins left, viola and cello right. Close and intimate.",
          [] (B& b) {
              strings (b, 0, SectionBodyViolin, 2, 0.7f, -0.45f, 0.3f);
              strings (b, 1, SectionBodyCello, 2, 0.6f, 0.45f, 0.3f, -12.0f);
              wheel (b, true, 0.45f, 0.5f);
              bothSec (b, true, "into", 0.25f);
              bothSec (b, true, "noise", 0.35f);
              bothSec (b, true, "attack", 0.3f);
              bothSec (b, true, "body_mix", 0.85f);
              bowed (b, 0.9f);
              hall (b, 0.3f, 0.5f, 0.5f);
              orchMacros (b, true, "Attack", "sec_attack", 0.5f);
          } },
        { "Hymn Strings II", "Strings/Brass", "Gentle, warm full strings: slow bow, soft dynamics, long tail. Made for hymns and prayer.",
          [] (B& b) {
              fullSection (b, 0.75f, 0.6f);
              wheel (b, true, 0.4f, 0.45f);
              bothSec (b, true, "attack", 0.1f);
              bothSec (b, true, "bright", 0.4f);
              bothSec (b, true, "vib_depth", 0.4f);
              bothSec (b, true, "vib_delay", 0.8f);
              bowed (b, 2.5f, 0.1f);
              orchHall (b, 0.5f);
              orchMacros (b, true, "Attack", "sec_attack", 0.5f);
          } },
        { "Epic Low Strings II", "Strings/Brass", "Cellos and basses in octaves, swelling up slowly. Big, dark, cinematic.",
          [] (B& b) {
              strings (b, 0, SectionBodyCello, 8, 0.75f, 0.2f, 0.7f, -12.0f);
              strings (b, 1, SectionBodyBass, 6, 0.65f, 0.4f, 0.5f, -24.0f);
              bothSec (b, true, "dyn", 0.25f);
              bothSec (b, true, "attack", 0.2f);
              bothSec (b, true, "bright", 0.35f);
              env (b, "env2", 2.5f, 1.0f, 1.0f, 1.5f);
              b.route ("Env 2", "e1_sec_dyn", 0.6f).route ("Env 2", "e2_sec_dyn", 0.6f);
              b.route ("Mod Wheel", "e1_sec_dyn", 0.3f).route ("Mod Wheel", "e2_sec_dyn", 0.3f);
              bowed (b, 1.5f, 0.2f);
              orchHall (b, 0.45f);
              orchMacros (b, true, "Swell Time", "env2_a", -0.3f);
          } },
    };
    return list;
}

juce::StringArray pack4IITags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "Full Strings II", "Strings, Orchestral, Full Section, Section Engine" },
        { "Full Strings Marcato II", "Strings, Orchestral, Full Section, Short, Section Engine" },
        { "Full Strings Dynamic II", "Strings, Orchestral, Full Section, Expressive, Section Engine" },
        { "Violins Ensemble II", "Strings, Orchestral, Violins, Section Engine" },
        { "Violins Con Sordino II", "Strings, Orchestral, Violins, Soft, Section Engine" },
        { "Violins Octaves II", "Strings, Orchestral, Violins, Cinematic, Section Engine" },
        { "Violas Section II", "Strings, Orchestral, Violas, Section Engine" },
        { "Cellos Section II", "Strings, Orchestral, Cellos, Section Engine" },
        { "Celli and Basses II", "Strings, Orchestral, Cellos, Basses, Section Engine" },
        { "Basses Pizzicato II", "Strings, Orchestral, Basses, Pizzicato, Bass, Section Engine" },
        { "Pizzicato Section II", "Strings, Orchestral, Pizzicato, Short, Section Engine" },
        { "Staccato Section II", "Strings, Orchestral, Short, Section Engine" },
        { "Tremolo Build II", "Strings, Orchestral, Tremolo, Cinematic, Swell, Section Engine" },
        { "Flautando Strings II", "Strings, Orchestral, Soft, Section Engine" },
        { "String Ostinato II", "Strings, Orchestral, Short, Arp, Cinematic, Section Engine" },
        { "Solo Violin II", "Strings, Solo, Violins, Expressive, Section Engine" },
        { "Solo Cello II", "Strings, Solo, Cellos, Expressive, Section Engine" },
        { "Chamber Quartet II", "Strings, Orchestral, Worship, Section Engine" },
        { "Hymn Strings II", "Strings, Orchestral, Worship, Section Engine" },
        { "Epic Low Strings II", "Strings, Orchestral, Cinematic, Cellos, Basses, Section Engine" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
