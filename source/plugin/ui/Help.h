#pragma once

// Plain-language help for the footer help bar.

#include "Widgets.h"

namespace ui
{
// "<Name>: what it does", or just the name when there's no text for it.
juce::String paramHelp (UiContext& ctx, const juce::String& paramId);
} // namespace ui
