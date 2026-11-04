#include "Content.h"

#include <algorithm>
#include <cmath>

namespace cs
{
namespace content
{
// Iterative radix-2 FFT. Not used on the audio thread. No 1/N scaling either way.
void fft (std::vector<std::complex<double>>& a, bool inverse)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap (a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = 2.0 * 3.14159265358979323846 / (double) len * (inverse ? 1.0 : -1.0);
        const std::complex<double> wlen (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len)
        {
            std::complex<double> w (1.0, 0.0);
            for (size_t k = 0; k < len / 2; ++k)
            {
                const auto u = a[i + k];
                const auto v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}
} // namespace content

// ---------------------------------------------------------------------------
std::unique_ptr<Wavetable> Wavetable::fromSpectra (std::string name, const std::vector<Spectrum>& frames)
{
    auto wt = std::make_unique<Wavetable>();
    wt->name = std::move (name);
    wt->numFrames = std::clamp ((int) frames.size(), 1, kMaxFrames);

    for (int k = 0; k < kLevels; ++k)
    {
        const int h = kMaxHarmonic >> k;
        wt->levelSize[k] = std::clamp (4 * h, 256, kBaseSize);
        wt->data[k].assign ((size_t) wt->numFrames * (size_t) (wt->levelSize[k] + 1), 0.0f);
    }

    std::vector<std::complex<double>> buf;
    for (int f = 0; f < wt->numFrames; ++f)
    {
        const auto& spec = frames[(size_t) f];
        float norm = 1.0f;

        for (int k = 0; k < kLevels; ++k)
        {
            const int size = wt->levelSize[k];
            const int maxH = std::min ({ kMaxHarmonic >> k, size / 2 - 1, (int) spec.size() - 1 });
            buf.assign ((size_t) size, {});
            for (int hh = 1; hh <= maxH; ++hh)
            {
                const std::complex<double> c (spec[(size_t) hh].real(), spec[(size_t) hh].imag());
                buf[(size_t) hh] = c;
                buf[(size_t) (size - hh)] = std::conj (c);
            }
            content::fft (buf, true);

            float* out = wt->data[k].data() + (size_t) f * (size_t) (size + 1);
            if (k == 0)
            {
                float peak = 0.0f;
                for (int i = 0; i < size; ++i)
                    peak = std::max (peak, (float) std::abs (buf[(size_t) i].real()));
                norm = peak > 1.0e-9f ? 1.0f / peak : 0.0f;
            }
            for (int i = 0; i < size; ++i)
                out[i] = (float) buf[(size_t) i].real() * norm;
            out[size] = out[0];
        }
    }
    return wt;
}

std::unique_ptr<Wavetable> Wavetable::fromSamples (std::string name, const float* samples, size_t numSamples)
{
    const size_t frames = std::clamp<size_t> (numSamples / kBaseSize, 1, kMaxFrames);
    std::vector<Spectrum> spectra (frames);
    std::vector<std::complex<double>> buf;
    for (size_t f = 0; f < frames; ++f)
    {
        buf.assign (kBaseSize, {});
        for (size_t i = 0; i < kBaseSize; ++i)
        {
            const size_t idx = f * kBaseSize + i;
            buf[i] = idx < numSamples ? samples[idx] : 0.0f;
        }
        content::fft (buf, false);
        spectra[f].assign (kMaxHarmonic + 1, {});
        for (int h = 1; h < kMaxHarmonic; ++h)
            spectra[f][(size_t) h] = std::complex<float> ((float) (buf[(size_t) h].real() / kBaseSize),
                                                          (float) (buf[(size_t) h].imag() / kBaseSize));
    }
    return fromSpectra (std::move (name), spectra);
}

std::unique_ptr<NoiseSample> NoiseSample::fromSamples (std::string name, const float* l, const float* r,
                                                       size_t numSamples, float sampleRate)
{
    auto ns = std::make_unique<NoiseSample>();
    ns->name = std::move (name);
    ns->sampleRate = sampleRate;
    ns->left.assign (l, l + numSamples);
    ns->right.assign (r != nullptr ? r : l, (r != nullptr ? r : l) + numSamples);
    return ns;
}

// ---------------------------------------------------------------------------
ContentLibrary& ContentLibrary::get()
{
    static ContentLibrary lib;
    return lib;
}

ContentLibrary::ContentLibrary()
{
    factoryWt.resize ((size_t) content::kNumWavetables);
    factoryNoise_.resize ((size_t) content::kNumNoises);
}

int ContentLibrary::numFactoryWavetables() const noexcept { return content::kNumWavetables; }
int ContentLibrary::numFactoryNoises() const noexcept { return content::kNumNoises; }

ContentLibrary::Entry ContentLibrary::wavetableEntry (int index) const noexcept
{
    return content::kWavetableList[std::clamp (index, 0, content::kNumWavetables - 1)];
}

ContentLibrary::Entry ContentLibrary::noiseEntry (int index) const noexcept
{
    return content::kNoiseList[std::clamp (index, 0, content::kNumNoises - 1)];
}

const Wavetable* ContentLibrary::factoryWavetable (int index)
{
    index = std::clamp (index, 0, content::kNumWavetables - 1);
    std::lock_guard<std::mutex> lock (mutex);
    auto& slot = factoryWt[(size_t) index];
    if (slot == nullptr)
        slot = Wavetable::fromSpectra (content::kWavetableList[index].name, content::makeFactoryWavetable (index));
    return slot.get();
}

const NoiseSample* ContentLibrary::factoryNoise (int index)
{
    index = std::clamp (index, 0, content::kNumNoises - 1);
    std::lock_guard<std::mutex> lock (mutex);
    auto& slot = factoryNoise_[(size_t) index];
    if (slot == nullptr)
        slot = content::makeFactoryNoise (index);
    return slot.get();
}

const Wavetable* ContentLibrary::findUserWavetable (const std::string& key)
{
    std::lock_guard<std::mutex> lock (mutex);
    for (auto& [k, wt] : userWt)
        if (k == key)
            return wt.get();
    return nullptr;
}

const Wavetable* ContentLibrary::addUserWavetable (const std::string& key, std::unique_ptr<Wavetable> wt)
{
    std::lock_guard<std::mutex> lock (mutex);
    // A re-imported file with the same key gets a new entry; old pointers stay valid.
    userWt.emplace_back (key, std::move (wt));
    // Keep lookups returning the newest version.
    std::rotate (userWt.rbegin(), userWt.rbegin() + 1, userWt.rend());
    return userWt.front().second.get();
}

const NoiseSample* ContentLibrary::findUserNoise (const std::string& key)
{
    std::lock_guard<std::mutex> lock (mutex);
    for (auto& [k, ns] : userNoise)
        if (k == key)
            return ns.get();
    return nullptr;
}

const NoiseSample* ContentLibrary::addUserNoise (const std::string& key, std::unique_ptr<NoiseSample> ns)
{
    std::lock_guard<std::mutex> lock (mutex);
    userNoise.emplace_back (key, std::move (ns));
    std::rotate (userNoise.rbegin(), userNoise.rbegin() + 1, userNoise.rend());
    return userNoise.front().second.get();
}
} // namespace cs
