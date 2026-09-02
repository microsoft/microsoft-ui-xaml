// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

// Faithful C++/WinRT port of onecoreuap\...\inkcontrols\lib\ColorNames.{h,cpp}.
// UWP maps each color to a localized name via a ~15-bit-packed per-color OS string-resource table
// (IDS_INKTOOLBAR_COLOR_*). That table is NOT shipped by the lifted resource pipeline, so this port
// keeps the UWP GENERIC fallback path ("RGB r, g, b") and reports isGenericFormat=true. The localized
// per-color names are a lifted-platform resource gap (tracked separately), not a logic change.
struct ColorNames
{
    winrt::hstring GetColorName(winrt::Windows::UI::Color const& color, bool& isGenericFormat)
    {
        // UWP looks up a per-color resource first; absent in the lift, so always the generic format.
        isGenericFormat = true;
        wchar_t buffer[64]{};
        swprintf_s(buffer, L"RGB %u, %u, %u", color.R, color.G, color.B);
        return winrt::hstring{ buffer };
    }
};
