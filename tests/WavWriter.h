#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// Minimal 32-bit float stereo WAV writer for test renders.
inline bool writeWav (const std::string& path, const std::vector<float>& l, const std::vector<float>& r, int sampleRate)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (! f)
        return false;
    const uint32_t frames = (uint32_t) l.size();
    const uint32_t dataBytes = frames * 2 * 4;
    auto u32 = [f] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [f] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + dataBytes);
    std::fwrite ("WAVE", 1, 4, f);
    std::fwrite ("fmt ", 1, 4, f); u32 (16); u16 (3); u16 (2);
    u32 ((uint32_t) sampleRate); u32 ((uint32_t) sampleRate * 8); u16 (8); u16 (32);
    std::fwrite ("data", 1, 4, f); u32 (dataBytes);
    for (uint32_t i = 0; i < frames; ++i)
    {
        std::fwrite (&l[i], 4, 1, f);
        std::fwrite (&r[i], 4, 1, f);
    }
    std::fclose (f);
    return true;
}
