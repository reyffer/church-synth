// Preset Pack 5 II: Brass, rebuilt on the Section engine (v0.6).
//
// The same twenty sounds as Pack 5 (which stays as it was), now played by the
// Section engine in its Brass family: independent players per note, a brightness
// that grows much faster than the level as you play louder (the brass "bloom"),
// scoop, blat and sforzando attacks, and a trumpet / trombone / horn / muted bell.
// Names end in "II" so both packs can sit side by side in the browser.
//
// Macros: M1 Brightness, M2 Space, M3 Vibrato, M4 a character control (Scoop, Blat,
// Length...). Velocity plays louder and brighter (it adds to Dynamics); the mod wheel
// swells the held notes (Dynamics again), except where noted.

#include "Packs.h"

#include "plugin/PresetHelpers.h"

#include <map>

using namespace presethelpers;

namespace
{
// Brass on engine e: bell, players, seat.
void horns (B& b, int e, int body, int players, float vol, float pos, float width, float coarse = 0.0f)
{
    ensemble (b, e, SectionBrass, body, players, vol);
    sec (b, e, "pos", pos);
    sec (b, e, "width", width);
    b.set ("e" + juce::String (e + 1) + "_coarse", coarse);
}

void both (B& b, bool two, const char* id, float v)
{
    sec (b, 0, id, v);
    if (two)
        sec (b, 1, id, v);
}

// Played like a brass player: velocity adds Dynamics, the wheel swells the held note.
void touch (B& b, bool two, float base, float velocity, float wheel = 0.35f)
{
    for (int e = 0; e < (two ? 2 : 1); ++e)
    {
        const juce::String d = "e" + juce::String (e + 1) + "_sec_dyn";
        sec (b, e, "dyn", base);
        b.route ("Velocity", d, velocity);
        if (wheel != 0.0f)
            b.route ("Mod Wheel", d, wheel);
    }
    b.set ("amp_velo", 0.2f);
}

void blown (B& b, float decay, float sustain, float release)
{
    env (b, "env1", 0.003f, decay, sustain, release);
}

// M1 Brightness, M2 Space, M3 Vibrato, M4 = m4 on dest.
void brassMacros (B& b, bool two, const char* m4, const char* m4dest, float m4amt)
{
    macros (b, "Brightness", "Space", "Vibrato", m4);
    b.route ("Macro 1", "e1_sec_bright", 0.4f).route ("Macro 2", "fx_aux_send", 0.5f).route ("Macro 3", "e1_sec_vib_depth", 0.6f);
    if (two)
        b.route ("Macro 1", "e2_sec_bright", 0.4f).route ("Macro 3", "e2_sec_vib_depth", 0.6f);
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

void soloHorn (B& b, float glide)
{
    sec (b, 0, "players", 1.0f);
    sec (b, 0, "width", 0.0f);
    mono (b, true, glide);
    b.route ("Aftertouch", "e1_sec_vib_depth", 0.5f);
}
} // namespace

const std::vector<FactoryPreset>& pack5IIPresets()
{
    static const std::vector<FactoryPreset> list {
        // ================================================================ SYNTH-STYLE / SOUL
        { "RnB Brass Stab II", "Strings/Brass", "Punchy, warm brass for R&B and soul stabs: four trumpets, a quick scoop and a short blat.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 4, 0.75f, 0.0f, 0.6f);
              b.set ("e2_on", 0.0f);
              touch (b, false, 0.45f, 0.45f);
              both (b, false, "attack", 0.85f);
              both (b, false, "bite", 0.5f);
              both (b, false, "scoop", 0.2f);
              both (b, false, "vib_depth", 0.1f);
              blown (b, 0.4f, 0.45f, 0.25f);
              room (b, 0.2f);
              brassMacros (b, false, "Blat", "sec_bite", 0.4f);
          } },
        { "Velvet RnB Brass II", "Strings/Brass", "Soft, swelling horn-bell brass for 90s R&B ballads. Lush under a vocal.",
          [] (B& b) {
              horns (b, 0, SectionBodyHorn, 6, 0.75f, 0.0f, 0.8f);
              b.set ("e2_on", 0.0f);
              touch (b, false, 0.35f, 0.25f, 0.4f);
              both (b, false, "attack", 0.25f);
              both (b, false, "scoop", 0.12f);
              both (b, false, "vib_depth", 0.3f);
              blown (b, 1.0f, 1.0f, 0.6f);
              chorus (b, 0, 0.25f);
              hall (b, 0.25f);
              brassMacros (b, false, "Scoop", "sec_scoop", 0.4f);
          } },
        { "Soul Brass Swell II", "Strings/Brass", "Brass that swells up while you hold the chord, then opens wide. The soul-ballad crescendo.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 4, 0.7f, -0.2f, 0.6f);
              horns (b, 1, SectionBodyTrombone, 4, 0.6f, 0.3f, 0.5f, -12.0f);
              touch (b, true, 0.15f, 0.2f, 0.0f);
              both (b, true, "attack", 0.3f);
              both (b, true, "vib_depth", 0.35f);
              env (b, "env2", 1.8f, 1.0f, 1.0f, 0.5f);
              b.route ("Env 2", "e1_sec_dyn", 0.75f).route ("Env 2", "e2_sec_dyn", 0.75f);
              blown (b, 1.0f, 1.0f, 0.5f);
              hall (b, 0.22f);
              brassMacros (b, true, "Swell Time", "env2_a", -0.3f);
          } },
        { "Funk Brass Hits II", "Strings/Brass", "Very short, bright hits for funk and gospel punches: full sforzando and a blat.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 4, 0.75f, -0.15f, 0.6f);
              horns (b, 1, SectionBodyTrombone, 3, 0.55f, 0.3f, 0.5f, -12.0f);
              touch (b, true, 0.6f, 0.35f, 0.0f);
              both (b, true, "attack", 1.0f);
              both (b, true, "bite", 0.8f);
              both (b, true, "scoop", 0.1f);
              both (b, true, "vib_depth", 0.0f);
              both (b, true, "loose", 0.15f);
              blown (b, 0.14f, 0.0f, 0.1f);
              room (b, 0.15f);
              brassMacros (b, true, "Length", nullptr, 0.0f);
              b.route ("Macro 4", "env1_d", 0.3f).route ("Macro 4", "env1_r", 0.2f);
          } },
        { "Bright Synth Brass II", "Strings/Brass", "Bright, wide 80s-style brass with chorus, but played by six real-feeling players.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 6, 0.7f, 0.0f, 0.9f);
              b.set ("e2_on", 0.0f);
              touch (b, false, 0.65f, 0.3f);
              both (b, false, "bright", 0.7f);
              both (b, false, "attack", 0.7f);
              both (b, false, "body_mix", 0.45f);
              both (b, false, "vib_depth", 0.0f);
              blown (b, 1.0f, 1.0f, 0.4f);
              chorus (b, 0, 0.35f);
              hall (b, 0.2f);
              brassMacros (b, false, "Body", "sec_body_mix", 0.5f);
          } },
        { "Octave Synth Brass II", "Strings/Brass", "Trumpets with trombones an octave below: one hand sounds like a full horn line.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 4, 0.7f, -0.25f, 0.5f);
              horns (b, 1, SectionBodyTrombone, 4, 0.6f, 0.3f, 0.5f, -12.0f);
              touch (b, true, 0.5f, 0.35f);
              both (b, true, "attack", 0.7f);
              both (b, true, "scoop", 0.15f);
              both (b, true, "vib_depth", 0.15f);
              blown (b, 1.0f, 1.0f, 0.35f);
              room (b, 0.2f);
              brassMacros (b, true, "Scoop", "sec_scoop", 0.4f);
          } },
        { "Gospel Brass II", "Strings/Brass", "Big, bright brass with a strong scoop and blat: gospel shouts and hits.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 5, 0.75f, -0.15f, 0.7f);
              horns (b, 1, SectionBodyTrombone, 3, 0.55f, 0.3f, 0.5f, -12.0f);
              touch (b, true, 0.55f, 0.4f);
              both (b, true, "attack", 0.85f);
              both (b, true, "bite", 0.6f);
              both (b, true, "scoop", 0.55f);
              both (b, true, "vib_depth", 0.2f);
              blown (b, 0.6f, 0.7f, 0.3f);
              room (b, 0.2f);
              brassMacros (b, true, "Scoop", "sec_scoop", 0.4f);
          } },
        // ================================================================ SHARP / BIG
        { "Sharp Brass Stab II", "Strings/Brass", "Very bright, cutting stab with a hard blat. Cuts through a loud band.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 4, 0.75f, 0.0f, 0.6f);
              b.set ("e2_on", 0.0f);
              touch (b, false, 0.65f, 0.35f, 0.0f);
              both (b, false, "bright", 0.8f);
              both (b, false, "attack", 1.0f);
              both (b, false, "bite", 1.0f);
              both (b, false, "vib_depth", 0.0f);
              blown (b, 0.3f, 0.3f, 0.2f);
              room (b, 0.15f);
              brassMacros (b, false, "Blat", "sec_bite", -0.5f);
          } },
        { "Brass Fall II", "Strings/Brass", "Brass that falls off in pitch when you let go: the big-band fall.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 4, 0.75f, 0.0f, 0.6f, -5.0f);
              b.set ("e2_on", 0.0f);
              touch (b, false, 0.6f, 0.3f);
              both (b, false, "attack", 0.8f);
              both (b, false, "scoop", 0.0f);
              both (b, false, "vib_depth", 0.1f);
              // Pitch sits 5 semitones low; Env 3 lifts it while the key is held, so on release it falls.
              env (b, "env3", 0.0f, 0.07f, 1.0f, 0.4f);
              b.route ("Env 3", "e1_coarse", 5.0f / 120.0f);
              blown (b, 1.0f, 1.0f, 0.45f);
              room (b, 0.22f);
              brassMacros (b, false, "Fall", nullptr, 0.0f);
              b.route ("Macro 4", "env3_r", 0.3f);
          } },
        { "Brass Sfz Swell II", "Strings/Brass", "Hit hard, drop back, then swell up again: the dramatic sforzando-crescendo.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 4, 0.7f, -0.2f, 0.6f);
              horns (b, 1, SectionBodyHorn, 4, 0.6f, 0.3f, 0.6f, -12.0f);
              both (b, true, "dyn", 0.15f);
              both (b, true, "attack", 1.0f);
              both (b, true, "bite", 0.6f);
              both (b, true, "vib_depth", 0.3f);
              env (b, "env2", 0.0f, 0.35f, 0.0f, 0.3f); // the hit
              env (b, "env3", 2.5f, 1.0f, 1.0f, 0.5f);  // the swell
              for (const char* d : { "e1_sec_dyn", "e2_sec_dyn" })
                  b.route ("Env 2", d, 0.7f).route ("Env 3", d, 0.75f);
              blown (b, 1.0f, 1.0f, 0.5f);
              b.set ("amp_velo", 0.3f);
              hall (b, 0.28f);
              brassMacros (b, true, "Swell Time", "env3_a", -0.3f);
          } },
        { "Staccato Brass II", "Strings/Brass", "Tight, short section notes for rhythmic figures.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 4, 0.75f, -0.15f, 0.6f);
              horns (b, 1, SectionBodyTrombone, 3, 0.55f, 0.3f, 0.5f, -12.0f);
              touch (b, true, 0.5f, 0.4f, 0.0f);
              both (b, true, "attack", 0.95f);
              both (b, true, "bite", 0.5f);
              both (b, true, "vib_depth", 0.0f);
              both (b, true, "loose", 0.15f);
              blown (b, 0.18f, 0.0f, 0.12f);
              room (b, 0.18f);
              brassMacros (b, true, "Length", nullptr, 0.0f);
              b.route ("Macro 4", "env1_d", 0.3f);
          } },
        { "Brass Blast II", "Strings/Brass", "A huge brass section at full force, slightly overdriven. Final chords and big endings.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 8, 0.75f, -0.15f, 0.9f);
              horns (b, 1, SectionBodyTrombone, 6, 0.6f, 0.25f, 0.7f, -12.0f);
              touch (b, true, 0.8f, 0.2f);
              both (b, true, "attack", 0.95f);
              both (b, true, "bite", 0.7f);
              both (b, true, "vib_depth", 0.15f);
              blown (b, 1.0f, 1.0f, 0.6f);
              b.fx (0, FxDistortion, { { 0, 0.15f }, { 1, 0.25f }, { 10, 0.4f } });
              hall (b, 0.3f);
              brassMacros (b, true, "Grit", "fx_a1_p11", 0.4f);
          } },
        // ================================================================ HORNS
        { "Trumpet Section II", "Strings/Brass", "A bright trumpet section with a scoop and breath. Lines and stabs.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 4, 0.75f, -0.1f, 0.6f);
              b.set ("e2_on", 0.0f);
              touch (b, false, 0.5f, 0.4f);
              both (b, false, "attack", 0.75f);
              both (b, false, "scoop", 0.3f);
              both (b, false, "noise", 0.4f);
              both (b, false, "vib_depth", 0.2f);
              blown (b, 1.0f, 1.0f, 0.3f);
              room (b, 0.22f);
              brassMacros (b, false, "Scoop", "sec_scoop", 0.4f);
          } },
        { "Trombone Section II", "Strings/Brass", "Warm, buzzy trombones on the right. Play in the low-mid range.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrombone, 4, 0.75f, 0.35f, 0.5f);
              b.set ("e2_on", 0.0f);
              touch (b, false, 0.5f, 0.4f);
              both (b, false, "attack", 0.6f);
              both (b, false, "bite", 0.4f);
              both (b, false, "scoop", 0.15f);
              both (b, false, "vib_depth", 0.15f);
              blown (b, 1.0f, 1.0f, 0.35f);
              room (b, 0.22f);
              brassMacros (b, false, "Blat", "sec_bite", 0.4f);
          } },
        { "Horn Section II", "Strings/Brass", "Trumpets and trombones together: the soul horn section in one patch.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 3, 0.7f, -0.35f, 0.4f);
              horns (b, 1, SectionBodyTrombone, 3, 0.6f, 0.35f, 0.4f, -12.0f);
              touch (b, true, 0.5f, 0.4f);
              both (b, true, "attack", 0.8f);
              both (b, true, "bite", 0.4f);
              both (b, true, "scoop", 0.25f);
              both (b, true, "vib_depth", 0.15f);
              blown (b, 0.8f, 0.8f, 0.3f);
              room (b, 0.2f);
              brassMacros (b, true, "Scoop", "sec_scoop", 0.4f);
          } },
        { "French Horns II", "Strings/Brass", "Round, noble French horns with a soft attack. Film and hymn moments.",
          [] (B& b) {
              horns (b, 0, SectionBodyHorn, 4, 0.75f, -0.1f, 0.7f);
              b.set ("e2_on", 0.0f);
              touch (b, false, 0.5f, 0.25f, 0.4f);
              both (b, false, "attack", 0.25f);
              both (b, false, "bright", 0.35f);
              both (b, false, "scoop", 0.05f);
              both (b, false, "vib_depth", 0.15f);
              blown (b, 1.0f, 1.0f, 0.7f);
              hall (b, 0.32f, 0.8f, 0.7f);
              brassMacros (b, false, "Attack", "sec_attack", 0.5f);
          } },
        { "French Horns Soft II", "Strings/Brass", "Very soft, distant French horns. A warm bed under worship.",
          [] (B& b) {
              horns (b, 0, SectionBodyHorn, 6, 0.8f, 0.0f, 0.8f);
              b.set ("e2_on", 0.0f);
              touch (b, false, 0.25f, 0.15f, 0.4f);
              both (b, false, "attack", 0.1f);
              both (b, false, "bright", 0.3f);
              both (b, false, "noise", 0.2f);
              both (b, false, "vib_depth", 0.1f);
              blown (b, 1.0f, 1.0f, 1.2f);
              hall (b, 0.4f, 0.85f, 0.75f);
              brassMacros (b, false, "Attack", "sec_attack", 0.5f);
          } },
        // ================================================================ SOLO
        { "Solo Trumpet II", "Lead", "One trumpeter: scoops, legato slurs, vibrato with aftertouch. Velocity and the wheel play louder.",
          [] (B& b) {
              horns (b, 0, SectionBodyTrumpet, 1, 0.8f, 0.0f, 0.0f);
              b.set ("e2_on", 0.0f);
              soloHorn (b, 0.04f);
              touch (b, false, 0.45f, 0.35f);
              both (b, false, "attack", 0.7f);
              both (b, false, "scoop", 0.35f);
              both (b, false, "noise", 0.45f);
              both (b, false, "vib_depth", 0.3f);
              both (b, false, "vib_delay", 0.35f);
              blown (b, 1.0f, 1.0f, 0.25f);
              room (b, 0.28f);
              brassMacros (b, false, "Scoop", "sec_scoop", 0.4f);
          } },
        { "Muted Trumpet II", "Lead", "Nasal, intimate muted trumpet for late-night jazz phrases.",
          [] (B& b) {
              horns (b, 0, SectionBodyMuted, 1, 0.85f, 0.0f, 0.0f);
              b.set ("e2_on", 0.0f);
              soloHorn (b, 0.04f);
              touch (b, false, 0.5f, 0.3f);
              both (b, false, "attack", 0.55f);
              both (b, false, "scoop", 0.2f);
              both (b, false, "noise", 0.5f);
              both (b, false, "vib_depth", 0.25f);
              both (b, false, "body_mix", 0.95f);
              blown (b, 1.0f, 1.0f, 0.25f);
              room (b, 0.25f);
              brassMacros (b, false, "Scoop", "sec_scoop", 0.4f);
          } },
        { "Flugelhorn II", "Lead", "Warm, dark, breathy flugelhorn: a softer cousin of the trumpet for ballads.",
          [] (B& b) {
              horns (b, 0, SectionBodyHorn, 1, 0.85f, 0.0f, 0.0f);
              b.set ("e2_on", 0.0f);
              soloHorn (b, 0.05f);
              touch (b, false, 0.4f, 0.3f);
              both (b, false, "attack", 0.35f);
              both (b, false, "bright", 0.3f);
              both (b, false, "scoop", 0.15f);
              both (b, false, "noise", 0.6f);
              both (b, false, "vib_depth", 0.3f);
              blown (b, 1.0f, 1.0f, 0.35f);
              hall (b, 0.3f);
              brassMacros (b, false, "Air", "sec_noise", 0.3f);
          } },
    };
    return list;
}

juce::StringArray pack5IITags (const juce::String& presetName)
{
    static const std::map<juce::String, const char*> tags {
        { "RnB Brass Stab II", "Brass, RnB, Stab, Section Engine" }, { "Velvet RnB Brass II", "Brass, RnB, Swell, Section Engine" },
        { "Soul Brass Swell II", "Brass, Soul, Swell, Section Engine" }, { "Funk Brass Hits II", "Brass, Funk, Stab, Short, Section Engine" },
        { "Bright Synth Brass II", "Brass, Synth Brass, Polysynth, Section Engine" }, { "Octave Synth Brass II", "Brass, Synth Brass, Section Engine" },
        { "Gospel Brass II", "Brass, Gospel, Stab, Section Engine" }, { "Sharp Brass Stab II", "Brass, Sharp, Stab, Section Engine" },
        { "Brass Fall II", "Brass, Sharp, Fall, Section Engine" }, { "Brass Sfz Swell II", "Brass, Sharp, Swell, Cinematic, Section Engine" },
        { "Staccato Brass II", "Brass, Sharp, Short, Section Engine" }, { "Brass Blast II", "Brass, Sharp, Big, Cinematic, Section Engine" },
        { "Trumpet Section II", "Brass, Horns, Trumpet, Section Engine" }, { "Trombone Section II", "Brass, Horns, Trombone, Section Engine" },
        { "Horn Section II", "Brass, Horns, Soul, Section Engine" }, { "French Horns II", "Brass, Horns, French Horn, Cinematic, Section Engine" },
        { "French Horns Soft II", "Brass, Horns, French Horn, Worship, Section Engine" },
        { "Solo Trumpet II", "Brass, Horns, Trumpet, Solo, Expressive, Section Engine" },
        { "Muted Trumpet II", "Brass, Horns, Trumpet, Solo, Section Engine" }, { "Flugelhorn II", "Brass, Horns, Solo, Expressive, Section Engine" },
    };
    const auto it = tags.find (presetName);
    return it == tags.end() ? juce::StringArray() : PresetManager::parseTags (it->second);
}
