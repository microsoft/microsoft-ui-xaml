// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

enum PivotAnimationDirection
{
    PivotAnimationDirection_Center = 0,
    PivotAnimationDirection_Left,
    PivotAnimationDirection_Right,
    PivotAnimationDirection_Reset
};


// Mods negative numbers in a way that makes sense
// for generating an index position from an infinite
// sequence.
static UINT32 PositiveMod(_In_ INT32 x, _In_ INT32 n)
{
    return n == 0 ? 0 : (x%n + n) % n;
}

// This number MUST be even.
static unsigned GetPivotPanelMultiplier() { return 1000u; }

static BOOLEAN AreClose(_In_ DOUBLE a, _In_ DOUBLE b, _In_ DOUBLE threshold = 0.1) { return abs(a - b) < threshold; }

} } } } XAML_ABI_NAMESPACE_END
