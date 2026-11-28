// Help bar texts: what a control does, in plain words, shown in the footer while
// the mouse is over it. Written for players learning sound design by ear.

#include "Help.h"

#include "dsp/fx/Effects.h"

#include <regex>
#include <utility>
#include <vector>

namespace ui
{
namespace
{
// (pattern on the parameter ID, explanation). First match wins.
const std::vector<std::pair<std::regex, const char*>>& rules()
{
    static const std::vector<std::pair<std::regex, const char*>> r = [] {
        std::vector<std::pair<const char*, const char*>> t {
            // --- master / play
            { "master_vol", "the output level of this instance" },
            { "master_width", "stereo width of the whole output; 0 % is mono. Check pads at 0 % if your PA is mono" },
            { "macro[1-4]", "a macro: one knob that moves several settings at once (right-click to MIDI-learn it)" },
            { "voicing", "Poly plays chords; Mono plays one note; Legato only retriggers when you lift between notes" },
            { "max_voices", "how many notes can sound at once; fewer saves CPU" },
            { "glide", "time to slide from one note's pitch to the next" },
            { "glide_legato", "glide only when notes overlap (legato playing)" },
            { "bend_(up|down)", "pitch-bend range in semitones" },
            { "hold", "notes keep sounding after you let go, until you play again" },
            { "transpose|fine_tune", "shifts the pitch of the whole instrument" },
            { "voice_pan", "spreads voices left and right" },
            { "amp_velo", "how much the source (normally velocity) changes the volume: 0 = every note equally loud" },
            { "amp_mod_src", "what controls the voice volume besides the envelope (velocity, an LFO for tremolo...)" },
            // --- engines
            { "e[12]_on", "turns the engine on or off (off = no CPU used)" },
            { "e[12]_type", "Analog: classic oscillators. Wavetable: a table of waveforms you can scan through. Organ: a tonewheel organ with drawbars. Section: strings or brass played by several players at once" },
            // --- section engine
            { "e[12]_sec_family", "Strings or Brass: how the players sound and attack. Pick a matching Body below (a violin body for strings, a bell for brass) or mix them for something new" },
            { "e[12]_sec_players", "how many players each note has: 1 = a soloist, 3 = chamber, 6 = a section, 8 = full. Each player is tuned, timed and seated a little differently" },
            { "e[12]_sec_into", "intonation: how far the players are out of tune with each other (up to 15 cents) plus a slow drift. 0 = perfectly together" },
            { "e[12]_sec_loose", "looseness: players don't start at exactly the same moment (up to 40 ms). 0 = machine-tight entries" },
            { "e[12]_sec_width", "how wide the players are seated across the stereo field (mono-safe)" },
            { "e[12]_sec_pos", "where the section sits: left (e.g. violins) to right (e.g. cellos)" },
            { "e[12]_sec_dyn", "dynamics: soft to loud. Moves level, brightness and vibrato together, like playing harder. Put it on the mod wheel (the presets do)" },
            { "e[12]_sec_bright", "extra brightness on top of what Dynamics gives (0.5 = neutral)" },
            { "e[12]_sec_noise", "bow noise (strings) or breath (brass), around each note; it follows Dynamics" },
            { "e[12]_sec_vib_depth", "vibrato depth. Each player has their own vibrato speed and depth around this" },
            { "e[12]_sec_vib_rate", "vibrato speed, about 4 to 7 Hz (each player a little different)" },
            { "e[12]_sec_vib_delay", "how long after the note starts the vibrato fades in (0 = at once)" },
            { "e[12]_sec_attack", "strings: soft bow to marcato. Brass: soft to sforzando (at the top it overshoots, then drops back)" },
            { "e[12]_sec_bite", "strings: Bite, the bow digging in (a burst of brightness and noise at the start). Brass: Blat, a short bright burst on loud attacks" },
            { "e[12]_sec_scoop", "brass only: the pitch starts up to 3 semitones below and rises into the note (30 to 80 ms)" },
            { "e[12]_sec_body_mix", "how much of the sound passes through the body / bell resonances (0 = just the players)" },
            { "e[12]_sec_body", "the instrument body (Violin, Viola, Cello, Bass, Ensemble) or bell (Trumpet, Trombone, Horn, Muted). Its fixed resonances make vibrato shimmer, like real instruments" },
            // --- organ engine
            { "e[12]_org_db1", "16' drawbar: one octave below the key (sub). Brown bars are the sub-harmonics" },
            { "e[12]_org_db2", "5 1/3' drawbar: a fifth above the key, an octave down (adds weight and hollowness)" },
            { "e[12]_org_db3", "8' drawbar: the note you play (the fundamental). White bars are octaves of the key" },
            { "e[12]_org_db4", "4' drawbar: one octave up" },
            { "e[12]_org_db5", "2 2/3' drawbar: an octave and a fifth up (a twelfth). Black bars are the non-octave harmonics" },
            { "e[12]_org_db6", "2' drawbar: two octaves up" },
            { "e[12]_org_db7", "1 3/5' drawbar: two octaves and a major third up (the 5th harmonic), a reedy colour" },
            { "e[12]_org_db8", "1 1/3' drawbar: two octaves and a fifth up (the 6th harmonic)" },
            { "e[12]_org_db9", "1' drawbar: three octaves up: sparkle and edge. Pulling bars out adds each harmonic; 8 = full, 0 = off" },
            { "e[12]_org_char", "how old the tonewheels sound: 0 = clean sine waves, higher = a little grit in each wheel and slightly uneven levels" },
            { "e[12]_org_tuning", "Tonewheel: the gear ratios of a real organ, very slightly off equal temperament (part of its sound). Equal: perfectly tuned" },
            { "e[12]_org_bass", "on: the 16' goes all the way down. Off: the 16' folds back up an octave in the lowest octave, like smaller organs" },
            { "e[12]_org_leak", "leakage: faint sound of the neighbouring tonewheels in the generator, the 'dirt' of a real organ (0 = perfectly clean)" },
            { "e[12]_org_click_tone", "brightness of the key click" },
            { "e[12]_org_click", "key click: each drawbar's contact closes at its own moment with a small click. 0 = smooth, soft keys" },
            { "e[12]_org_perc_harm", "percussion harmonic: Second (an octave up) or Third (an octave and a fifth up, the classic jazz sound)" },
            { "e[12]_org_perc_decay", "how quickly the percussion dies away: Fast (a tick) or Slow (a ping)" },
            { "e[12]_org_perc_vol", "percussion level. Normal also lowers the drawbars a little, as on the original" },
            { "e[12]_org_perc_cut", "with percussion on, the 1' drawbar is silent (the original used its contact for the percussion)" },
            { "e[12]_org_perc", "percussion: a decaying 'ping' on the attack. It only plays when you start from all keys up, so legato playing doesn't retrigger it" },
            { "e[12]_coarse", "pitch of the whole engine in semitones (12 = an octave)" },
            { "e[12]_fine", "fine tuning; a few cents against the other engine makes a thick, beating sound" },
            { "e[12]_drift", "slow random pitch wander, like an old analog synth warming up" },
            { "e[12]_kbd", "on: pitch follows the keys. Off: every key plays the same pitch (for drones and effects)" },
            { "e[12]_vol", "level of this engine in the mix" },
            { "e[12]_fmix", "sends this engine to Filter 1, Filter 2, or a blend" },
            { "e[12]_o[123]_wave", "waveform. Sine: pure. Triangle: soft. Saw: bright, all harmonics. Square: hollow, odd harmonics" },
            { "e[12]_o[123]_width", "pulse width of the square (and triangle shape): narrower is thinner and nasal" },
            { "e[12]_o[123]_vol", "level of this oscillator" },
            { "e[12]_o[123]_coarse", "oscillator pitch in semitones: 12 = octave, 7 = fifth, 19 = octave + fifth" },
            { "e[12]_o[123]_fine", "detunes this oscillator slightly: small amounts beat against the others" },
            { "e[12]_o[123]_kbd", "on: follows the keys. Off: fixed pitch (useful for FM)" },
            { "e[12]_o1_fm", "frequency modulation of Osc 1 by Osc 3 (or noise): adds metallic, bell or bark tones" },
            { "e[12]_o[23]_fm", "frequency modulation of this oscillator: adds metallic, bell or bark tones" },
            { "e[12]_sync", "hard sync: Osc 2 restarts with Osc 1. Changing Osc 2's pitch then changes the tone, not the note" },
            { "e[12]_mod_src", "what modulates Osc 1's FM: Osc 3 or noise" },
            { "e[12]_mod_eng", "uses the other engine's sound as the FM modulator (cross-mod)" },
            { "e[12]_noise_(vol|color)", "noise inside the engine: adds air or grit" },
            { "e[12]_uni_on", "unison: several copies of each note, detuned, for a thick sound" },
            { "e[12]_uni_mode", "Classic: copies of the whole engine. Super: the famous supersaw. Chord: copies at chord intervals" },
            { "e[12]_uni_voices", "how many unison copies (more = thicker, more CPU)" },
            { "e[12]_uni_detune", "how far apart the unison copies are tuned: small = chorus-like, large = huge and wide" },
            { "e[12]_uni_stereo", "spreads the unison copies across the stereo field" },
            { "e[12]_uni_chord", "the chord the unison copies play (root + fifth, octave...)" },
            { "e[12]_uni_mix", "Super unison: level of the detuned copies against the centre one" },
            { "e[12]_wt_table", "the wavetable: a series of waveforms; Position picks one (or a blend)" },
            { "e[12]_wt_pos", "position in the wavetable: scans through its waveforms (modulate it for movement)" },
            { "e[12]_wt_morph", "how positions blend: smoothly, or in steps" },
            { "e[12]_wt_vol", "level of the wavetable oscillator" },
            { "e[12]_wt_fm_type|e[12]_wt_fm", "frequency modulation or ring modulation by the mod oscillator: bells, metal, growl" },
            { "e[12]_wt_pm", "phase modulation by the mod oscillator: the classic DX electric-piano and bell sound" },
            { "e[12]_wt_retrig|e[12]_wt_phase", "where each note starts in the waveform: the same place for a consistent attack, or free" },
            { "e[12]_wt_pd", "phase transform: bends the waveform in time, for brighter or more nasal tones" },
            { "e[12]_wt_fold", "wavefolding: folds loud parts of the wave back, adding bright harmonics" },
            { "e[12]_wt_uni_phase", "unison copies start at random points (wide) or together (punchy)" },
            { "e[12]_wt_mod_(ratio|coarse|hz|fine|tune_mode)", "pitch of the mod oscillator. Whole-number ratios sound musical; odd ratios sound like bells" },
            { "e[12]_wt_mod_vol", "lets you hear the mod oscillator itself as well" },
            { "e[12]_wt_mod_wave", "waveform of the mod oscillator" },
            { "e[12]_q_mask|u_osc_q_mask", "when the engine pitch is modulated, it snaps to these notes" },
            // --- utility
            { "u_on", "turns the Utility engine on or off (noises and the sub oscillator)" },
            { "u_n[12]_type", "the noise sample: breath, clicks, vinyl, rain... adds realism and air" },
            { "u_n[12]_vol", "level of this noise" },
            { "u_n[12]_tune|u_n[12]_kbd", "pitch of the noise sample, and whether it follows the keys" },
            { "u_n[12]_filter", "tilts the noise darker (left) or thinner (right)" },
            { "u_n[12]_loop|u_n[12]_len", "loop the noise (beds) or play it once (clicks); Length cuts a one-shot short" },
            { "u_n[12]_phase", "start the sample at the beginning, or at a random point for variety" },
            { "u_n[12]_fmix|u_osc_fmix", "sends this source to Filter 1, Filter 2, or a blend" },
            { "u_osc_wave|u_osc_width", "waveform of the utility oscillator (usually a sub)" },
            { "u_osc_coarse|u_osc_fine|u_osc_kbd", "pitch of the utility oscillator: -12 for a sub octave" },
            { "u_osc_vol", "level of the utility oscillator" },
            { "u_osc_route", "the utility oscillator can skip the filters and go straight out" },
            { "u_(n[12]|osc)_on", "turns this source on or off" },
            // --- filters
            { "f[12]_on", "turns the filter on or off" },
            { "f[12]_type", "the filter model; each has its own character (hover the menu items for a description)" },
            { "f[12]_mode|f[12]_submode", "lowpass keeps lows (darker), highpass keeps highs (thinner), bandpass keeps the middle" },
            { "f[12]_cutoff", "where the filter starts to cut: lower = darker. The most important tone control" },
            { "f[12]_reso", "boosts the frequencies at the cutoff: a vocal, whistling peak. High values can ring" },
            { "f[12]_drive", "saturates before and after the filter: more grit and weight, slightly louder" },
            { "f[12]_analog", "analog-style resonance that thickens and softens at high levels" },
            { "f[12]_kbd", "key tracking: higher notes open the filter more, so high notes aren't dull" },
            { "f[12]_vol", "level after the filter (up to +12 dB, to make up what the filter takes away)" },
            { "f[12]_pan", "places this filter's output left or right" },
            { "f[12]_[abc]", "a model-specific control (its name shows on the panel)" },
            { "f[12]_fm|f[12]_fm_src", "audio-rate modulation of the cutoff: aggressive, metallic or growling tones" },
            { "f_routing", "Series: Filter 1 then Filter 2. Parallel: side by side" },
            { "f_output", "how the two filters reach the output (summed or split)" },
            // --- envelopes
            { "env[123]_a", "attack: time to reach full level. 0 = instant (sharp); longer = swells in" },
            { "env[123]_d", "decay: time to fall from full to the sustain level" },
            { "env[123]_s", "sustain: the level held while the key is down" },
            { "env[123]_r", "release: how long the sound takes to fade after you let go" },
            { "env[123]_acurve|env[123]_dcurve", "curve of the segment: bowed shapes sound more natural than straight lines" },
            { "env[123]_adr", "ADR: the envelope ignores the key-up and runs attack-decay-release on its own" },
            { "env[123]_link", "release uses the decay time" },
            { "env[123]_retrig", "when the envelope restarts: every key, only after a gap (legato), or never" },
            // --- LFOs / functions / random / combinators
            { "lfo[123]_wave", "the shape of the movement: sine is smooth, square jumps, random wanders" },
            { "lfo[123]_rate|lfo[123]_div|lfo[123]_sync", "speed of the LFO, in Hz or synced to the tempo" },
            { "lfo[123]_fade", "the LFO fades in after the note starts, like a singer's delayed vibrato" },
            { "lfo[123]_", "an LFO is a slow repeating movement: route it to pitch (vibrato), volume (tremolo) or cutoff (wah)" },
            { "func[123]_shape", "the drawn shape this function plays (draw your own on the Functions tab)" },
            { "func[123]_mode", "loop the shape, or play it once (for risers and one-off sweeps)" },
            { "func[123]_", "a Function plays a drawn shape over time: rhythmic gates, swells, custom envelopes" },
            { "rand[123]_", "a Random source: unpredictable movement. Smooth it for slow drifting, or step it for sample-and-hold" },
            { "comb[123]_", "a Combinator mixes or reshapes two modulation sources (or follows the audio level)" },
            { "curve_(velo|at|kbd)", "reshapes velocity / aftertouch / keyboard response before it modulates anything" },
            // --- sequencer
            { "seq_on", "turns the sequencer / arpeggiator on" },
            { "seq_mode", "Arp: plays held notes one at a time. Seq: plays the held notes in a programmed pattern" },
            { "seq_rate", "step length (1/16 = sixteenth notes)" },
            { "seq_swing", "delays every second step: 50 % straight, about 58 % groove, 66 % triplet feel" },
            { "seq_arp_order|seq_arp_octaves", "arpeggio direction and how many octaves it climbs" },
            { "seq_scale|seq_snap", "random pitches stay in this scale" },
            { "seq_", "sequencer setting" },
            // --- FX
            { "fx_on", "turns the whole FX section on or off (off = the dry synth)" },
            { "fx_routing", "how the A and B buses connect (in series or in parallel)" },
            { "fx_aux_send", "how much goes to the Aux bus (usually the reverb)" },
            { "fx_[abx][0-9]_type", "the effect in this slot" },
            { "fx_", "effect setting" },
        };
        std::vector<std::pair<std::regex, const char*>> out;
        for (const auto& [pat, text] : t)
            out.push_back ({ std::regex (std::string ("^(") + pat + ")"), text });
        return out;
    }();
    return r;
}

// Effect controls, by effect and display name ("Effect/Control"), then by name.
const char* fxControlHelp (const juce::String& effect, const juce::String& name)
{
    static const std::vector<std::pair<const char*, const char*>> specific {
        { "Vintage Chorus/Brightness", "opens the dark, classic bucket-brigade sound (0) up to a full-range chorus with a slight top lift (1)" },
        { "Vintage Chorus/Wide", "stretches the short classic chorus (0) to the longer, deeper and wider chorus of a big poly synth (1)" },
        { "Vintage Chorus/Width", "stereo width of the chorus (0 = mono)" },
        { "Reverb/Brightness", "lifts the highs of the tail: a brighter, more open reverb" },
        { "Reverb/Mod Depth", "how much the tail's pitch drifts: smooths metallic ringing, gives a lush, chorused tail" },
        { "Reverb/Mod Rate", "speed of the tail's drift" },
        { "Rotary/Speed", "Slow (chorale) or Fast (tremolo). The organ presets connect the mod wheel here: past halfway it switches to Fast, "
                          "back below halfway to Slow, and this switch shows the live speed (outlined in the wheel's colour). "
                          "The lamp on the Organ panel shows it too and switches it with a click. For a button or footswitch: right-click > MIDI learn" },
        { "Rotary/Brake", "stops both rotors (slowing down over the Ramp time); turn it down to start them again" },
        { "Organ Scanner/Mode", "V1-V3: vibrato, light to deep (pitch wobble only). C1-C3: chorus, the vibrato mixed with the dry organ for the classic swirl. C3 is the usual choice" },
        { "Organ Scanner/Rate", "speed of the scanner (about 6.9 Hz on the original, which ran at a fixed speed)" },
        { "Organ Scanner/Depth", "scales the mode's depth (50 % = as the mode is meant to be)" },
        { "Organ Scanner/Tone", "how bright the scanned signal is (the original's delay line dulled the highs a little)" },
        { "Organ Scanner/Stereo", "0 = mono like the original; more offsets the right side's sweep for width" },
        { "Super Unison/Width", "0 = mono, 1 = left and right have their own voices (very wide); the bass stays centred" },
        { "Super Unison/Detune", "how far apart the copies are tuned (about +/-20 cents by default)" },
    };
    for (const auto& [n, text] : specific)
        if ((effect + "/" + name).equalsIgnoreCase (n))
            return text;
    static const std::vector<std::pair<const char*, const char*>> t {
        { "Dry/Wet", "balance between the original and the effect" },
        { "Feedback", "how much is fed back: more repeats (delay) or a stronger sweep (flanger, phaser)" },
        { "Time", "delay time" }, { "Rate", "speed of the effect's movement" }, { "Depth", "how strong the movement is" },
        { "Decay", "how long the tail lasts" }, { "Size", "size of the space" }, { "Pre-Delay", "a short gap before the reverb starts: keeps the voice clear" },
        { "Damping", "how quickly the highs die away in the tail (higher = darker)" },
        { "Drive", "how hard the effect is pushed: more saturation" }, { "Type", "the character of the effect" },
        { "Ping-Pong", "echoes bounce between left and right" }, { "Width", "stereo width of the effect" },
        { "Speed", "rotary speed: Slow (chorale) or Fast (tremolo). MIDI-learn it to a switch or route the mod wheel" },
        { "Brake", "stops the rotary speaker" }, { "Horn/Drum", "balance of the treble horn and the bass drum" },
        { "Ramp", "how long the rotors take to change speed" },
        { "Mode", "Vintage Chorus: I is slow, II is faster with the same depth, I + II is a fast, mono shimmer" },
        { "Low Cut", "removes lows from the effect" }, { "High Cut", "removes highs from the effect" },
        { "Threshold", "level where the compressor starts working" }, { "Ratio", "how strongly it compresses" },
        { "Attack", "how fast it reacts" }, { "Release", "how fast it lets go" }, { "Makeup", "level after compression" },
        { "Cutoff", "filter frequency" }, { "Resonance", "boost at the cutoff" },
        { "Shimmer", "amount of the octave-up layer in the tail" },
    };
    for (const auto& [n, text] : t)
        if (name.equalsIgnoreCase (n))
            return text;
    return nullptr;
}
} // namespace

juce::String paramHelp (UiContext& ctx, const juce::String& paramId)
{
    const auto* d = ctx.def (paramId);
    if (d == nullptr)
        return {};
    juce::String name = d->name;
    juce::String text;

    // FX slot controls: name and help come from the effect in the slot.
    static const std::regex slotParam ("^fx_([abx][0-9])_p([0-9]+)$");
    std::smatch m;
    const auto idStd = paramId.toStdString();
    if (std::regex_match (idStd, m, slotParam))
    {
        const auto slot = juce::String (m[1].str());
        const int k = std::stoi (m[2].str()) - 1;
        const int type = ctx.intValue ("fx_" + slot + "_type");
        const auto& info = cs::fx::fxInfo (type);
        if (k >= 0 && k < cs::fx::kFxParams && info.params[(size_t) k] != nullptr)
        {
            name = juce::String (info.name) + " " + info.params[(size_t) k];
            if (const char* h = fxControlHelp (info.name, info.params[(size_t) k]))
                text = h;
        }
    }
    if (text.isEmpty())
        for (const auto& [re, h] : rules())
            if (std::regex_search (idStd, re))
            {
                text = h;
                break;
            }
    return text.isEmpty() ? name : name + ": " + text;
}
} // namespace ui
