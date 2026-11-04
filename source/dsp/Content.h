#pragma once

// Sound content: band-limited wavetables and noise samples.
//
// Everything is generated in code (no third-party content). Factory items are
// generated lazily on first use and cached process-wide, so several plugin
// instances in one MainStage concert share the same memory.
//
// Thread safety: the library may be called from any NON-audio thread. Returned
// pointers stay valid for the lifetime of the process (nothing is ever freed),
// so the audio thread can safely hold raw pointers handed to it.
//
// FACTORY ORDER IS PERMANENT: host parameters store the index. Only append.

#include <complex>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace cs
{
// ---------------------------------------------------------------------------
class Wavetable
{
public:
    static constexpr int kMaxFrames = 256;
    static constexpr int kBaseSize = 2048;
    static constexpr int kMaxHarmonic = 1024;
    static constexpr int kLevels = 11; // level k holds harmonics up to 1024 >> k

    using Spectrum = std::vector<std::complex<float>>; // index = harmonic (0..1024), DC ignored

    // Builds the mip-mapped table from per-frame spectra. Each frame is
    // peak-normalised so sweeping position keeps a steady level.
    static std::unique_ptr<Wavetable> fromSpectra (std::string name, const std::vector<Spectrum>& frames);

    // Slices raw audio into consecutive 2048-sample frames (max 256), like the
    // usual wavetable WAV convention. Shorter files give fewer frames.
    static std::unique_ptr<Wavetable> fromSamples (std::string name, const float* samples, size_t numSamples);

    const std::string& getName() const noexcept { return name; }
    int getNumFrames() const noexcept { return numFrames; }
    int getLevelSize (int level) const noexcept { return levelSize[level]; }

    // Frame data for a mip level: getLevelSize(level) + 1 samples (last = guard copy of first).
    const float* frame (int level, int index) const noexcept
    {
        return data[level].data() + (size_t) index * (size_t) (levelSize[level] + 1);
    }

    // Highest mip level whose harmonics stay below Nyquist for this phase increment.
    static int levelFor (float inc) noexcept
    {
        int level = 0;
        while (level < kLevels - 1 && (float) (kMaxHarmonic >> level) * inc > 0.48f)
            ++level;
        return level;
    }

private:
    std::string name;
    int numFrames = 0;
    int levelSize[kLevels] {};
    std::vector<float> data[kLevels];
};

// ---------------------------------------------------------------------------
class NoiseSample
{
public:
    std::string name;
    float sampleRate = 48000.0f;
    bool oneShot = false;     // hint: transients are meant to play once
    std::vector<float> left;  // always filled
    std::vector<float> right; // same length as left (copy of left for mono sources)

    size_t length() const noexcept { return left.size(); }

    static std::unique_ptr<NoiseSample> fromSamples (std::string name, const float* l, const float* r,
                                                     size_t numSamples, float sampleRate);
};

// ---------------------------------------------------------------------------
class ContentLibrary
{
public:
    static ContentLibrary& get();

    struct Entry
    {
        const char* category;
        const char* name;
    };

    int numFactoryWavetables() const noexcept;
    Entry wavetableEntry (int index) const noexcept;
    const Wavetable* factoryWavetable (int index); // generates on first use

    int numFactoryNoises() const noexcept;
    Entry noiseEntry (int index) const noexcept;
    const NoiseSample* factoryNoise (int index); // generates on first use

    // User content, cached by key (e.g. file name). The library takes ownership.
    const Wavetable* findUserWavetable (const std::string& key);
    const Wavetable* addUserWavetable (const std::string& key, std::unique_ptr<Wavetable> wt);
    const NoiseSample* findUserNoise (const std::string& key);
    const NoiseSample* addUserNoise (const std::string& key, std::unique_ptr<NoiseSample> ns);

private:
    ContentLibrary();
    std::mutex mutex;
    std::vector<std::unique_ptr<Wavetable>> factoryWt;
    std::vector<std::unique_ptr<NoiseSample>> factoryNoise_;
    std::vector<std::pair<std::string, std::unique_ptr<Wavetable>>> userWt;
    std::vector<std::pair<std::string, std::unique_ptr<NoiseSample>>> userNoise;
};

// Internal generators (Content.cpp / ContentFactory.cpp).
namespace content
{
void fft (std::vector<std::complex<double>>& a, bool inverse);
std::vector<Wavetable::Spectrum> makeFactoryWavetable (int index);
std::unique_ptr<NoiseSample> makeFactoryNoise (int index);
extern const ContentLibrary::Entry kWavetableList[];
extern const int kNumWavetables;
extern const ContentLibrary::Entry kNoiseList[];
extern const int kNumNoises;
} // namespace content
} // namespace cs
