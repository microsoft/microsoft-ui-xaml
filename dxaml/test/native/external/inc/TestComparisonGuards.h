// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Small default per-channel pixel tolerance (out of 255) for the handful of tests
// that can't share an exact master across OS versions -- mainly JPEG decode rounding
// on the 2 tests we kept on JPEG, plus minor shadow / emoji-font differences. A few
// text tests need a larger local tolerance and pass their own value instead (e.g.
// HyperlinkInText uses 15 for glyph antialiasing). Measured at 3/255.
constexpr int RENDER_COMPARE_TOLERANCE_SMALL = 3;

// RAII guard that sets per-channel pixel tolerance for DComp surface (PNG)
// comparisons and resets it to 0 when the guard goes out of scope. This
// ensures tolerance doesn't leak between tests even if the test throws.
//
// Usage:
//   ImageCompareToleranceGuard tolerance(RENDER_COMPARE_TOLERANCE_SMALL);
//
class ImageCompareToleranceGuard
{
public:
    explicit ImageCompareToleranceGuard(int tolerance)
    {
        Microsoft::UI::Xaml::Tests::Common::TestServices::Utilities->SetImageCompareTolerance(tolerance);
    }

    ~ImageCompareToleranceGuard()
    {
        Microsoft::UI::Xaml::Tests::Common::TestServices::Utilities->SetImageCompareTolerance(0);
    }

    ImageCompareToleranceGuard(const ImageCompareToleranceGuard&) = delete;
    ImageCompareToleranceGuard& operator=(const ImageCompareToleranceGuard&) = delete;
};
