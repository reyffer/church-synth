#include "ModSources.h"

#include <initializer_list>

namespace cs
{
namespace
{
struct Pt
{
    float x, y, c;
};

FunctionShape make (std::initializer_list<Pt> pts, int sustain = -1, int loopStart = 0)
{
    FunctionShape s;
    s.count = 0;
    for (const auto& p : pts)
    {
        if (s.count >= FunctionShape::kMaxPoints)
            break;
        s.x[s.count] = p.x;
        s.y[s.count] = p.y;
        s.curve[s.count] = p.c;
        ++s.count;
    }
    s.sustain = sustain;
    s.loopStart = loopStart;
    return s;
}

FunctionShape steps (std::initializer_list<float> levels)
{
    FunctionShape s;
    s.count = 0;
    const int n = (int) levels.size();
    int i = 0;
    for (float v : levels)
    {
        const float x0 = (float) i / (float) n, x1 = (float) (i + 1) / (float) n;
        s.x[s.count] = x0; s.y[s.count] = v; s.curve[s.count] = 0.0f; ++s.count;
        s.x[s.count] = x1 - 0.0005f; s.y[s.count] = v; s.curve[s.count] = 0.0f; ++s.count;
        ++i;
    }
    s.x[s.count - 1] = 1.0f;
    s.sustain = s.count - 2;
    return s;
}

struct Presets
{
    FunctionShape func[kNumFunctionPresets];
    FunctionShape remap[kNumRemapShapes];

    Presets()
    {
        func[0] = make ({ { 0, 0, 0 }, { 1, 1, 0 } }, 1);                                    // Ramp Up
        func[1] = make ({ { 0, 1, 0 }, { 1, 0, 0 } }, 0);                                    // Ramp Down
        func[2] = make ({ { 0, 0, 0 }, { 0.5f, 1, 0 }, { 1, 0, 0 } }, 1);                    // Triangle
        func[3] = make ({ { 0, 0.5f, 0.35f }, { 0.25f, 1, -0.35f }, { 0.5f, 0.5f, 0.35f },  // Sine-like
                          { 0.75f, 0, -0.35f }, { 1, 0.5f, 0 } }, 1);
        func[4] = make ({ { 0, 1, 0 }, { 0.4995f, 1, 0 }, { 0.5f, 0, 0 }, { 1, 0, 0 } }, 1); // Square
        func[5] = make ({ { 0, 1, 0 }, { 0.2495f, 1, 0 }, { 0.25f, 0, 0 }, { 1, 0, 0 } }, 1); // Pulse 25 %
        func[6] = make ({ { 0, 0, 0 }, { 0.02f, 1, -0.7f }, { 1, 0, 0 } }, 1);               // Decay
        func[7] = make ({ { 0, 0, 0.7f }, { 1, 1, 0 } }, 1);                                  // Swell
        func[8] = make ({ { 0, 0, 0.5f }, { 0.6f, 1, 0 }, { 0.85f, 1, -0.5f }, { 1, 0, 0 } }, 2, 1); // Rise & Fall
        func[9] = make ({ { 0, 1, 0.6f }, { 0.25f, 0, -0.6f }, { 0.45f, 0.55f, 0.6f },       // Bounce
                          { 0.62f, 0, -0.6f }, { 0.76f, 0.25f, 0.6f }, { 0.88f, 0, -0.6f },
                          { 0.95f, 0.08f, 0.6f }, { 1, 0, 0 } }, -1);
        func[10] = steps ({ 0.0f, 0.333f, 0.667f, 1.0f });                                    // Stairs Up
        func[11] = steps ({ 1.0f, 0.667f, 0.333f, 0.0f });                                    // Stairs Down
        func[12] = steps ({ 0.6f, 0.1f, 0.9f, 0.35f, 0.75f, 0.0f, 0.5f, 1.0f });              // Random Steps
        func[13] = steps ({ 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1 });                 // Trance Gate
        func[14] = make ({ { 0, 0, -0.5f }, { 0.2f, 1, 0.5f }, { 0.4f, 0.3f, -0.5f },        // Double Peak
                          { 0.6f, 0.8f, 0.5f }, { 1, 0, 0 } }, 3, 1);
        func[15] = make ({ { 0, 0, 0 }, { 0.03f, 0, -0.8f }, { 1, 1, 0 } }, 1);               // Duck

        remap[0] = make ({ { 0, 0, 0 }, { 1, 1, 0 } });                                       // Linear
        remap[1] = make ({ { 0, 0, 0.6f }, { 1, 1, 0 } });                                    // Exponential
        remap[2] = make ({ { 0, 0, -0.6f }, { 1, 1, 0 } });                                   // Logarithmic
        remap[3] = make ({ { 0, 0, 0.6f }, { 0.5f, 0.5f, -0.6f }, { 1, 1, 0 } });            // S-Curve
        remap[4] = steps ({ 0.0f, 0.333f, 0.667f, 1.0f });                                    // 4 Steps
        remap[5] = steps ({ 0, 0.143f, 0.286f, 0.429f, 0.571f, 0.714f, 0.857f, 1 });          // 8 Steps
        remap[6] = make ({ { 0, 0, 0 }, { 0.5f, 1, 0 }, { 1, 0, 0 } });                       // Fold
        remap[7] = make ({ { 0, 1, 0 }, { 1, 0, 0 } });                                       // Invert
        remap[8] = make ({ { 0, 1, 0 }, { 0.5f, 0, 0 }, { 1, 1, 0 } });                       // Rectify
        remap[9] = make ({ { 0, 0, 0 }, { 0.4995f, 0, 0 }, { 0.5f, 1, 0 }, { 1, 1, 0 } });    // Gate
    }
};

const Presets& presets()
{
    static const Presets p;
    return p;
}

const char* const kFuncNames[kNumFunctionPresets] = {
    "Ramp Up", "Ramp Down", "Triangle", "Sine", "Square", "Pulse 25%", "Decay", "Swell",
    "Rise & Fall", "Bounce", "Stairs Up", "Stairs Down", "Random Steps", "Trance Gate", "Double Peak", "Duck",
};
const char* const kRemapNames[kNumRemapShapes] = {
    "Linear", "Exponential", "Logarithmic", "S-Curve", "4 Steps", "8 Steps", "Fold", "Invert", "Rectify", "Gate",
};
} // namespace

const char* functionPresetName (int i) noexcept { return kFuncNames[std::clamp (i, 0, kNumFunctionPresets - 1)]; }
const FunctionShape& functionPreset (int i) noexcept { return presets().func[std::clamp (i, 0, kNumFunctionPresets - 1)]; }
const char* remapShapeName (int i) noexcept { return kRemapNames[std::clamp (i, 0, kNumRemapShapes - 1)]; }
const FunctionShape& remapShape (int i) noexcept { return presets().remap[std::clamp (i, 0, kNumRemapShapes - 1)]; }
} // namespace cs
