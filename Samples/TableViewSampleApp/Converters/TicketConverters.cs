// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Media;

namespace TableViewSampleApp.Converters;

/// <summary>
/// x:Bind functions for the support tickets' Priority chip (HeadersVisibilityPage). Every brush is
/// created once and shared. Under a Contrast theme the tint drops to transparent and the dot to the
/// theme text brush (the shared <see cref="ChipBrushes"/> rule), so the chip never relies on
/// colour alone.
/// </summary>
public static class TicketChipPalette
{
    private static readonly Dictionary<string, (SolidColorBrush Tint, SolidColorBrush Dot)> s_priorityBrushes = new(StringComparer.Ordinal)
    {
        ["Sev 1"] = Pair(0xDC, 0x26, 0x26),
        ["Sev 2"] = Pair(0xEA, 0x58, 0x0C),
        ["Sev 3"] = Pair(0xCA, 0x8A, 0x04),
        ["Sev 4"] = Pair(0x64, 0x74, 0x8B),
    };

    public static Brush PriorityTint(string priority) =>
        s_priorityBrushes.TryGetValue(priority ?? string.Empty, out var brushes) ? brushes.Tint : ChipBrushes.Transparent;

    public static Brush PriorityDot(string priority) =>
        s_priorityBrushes.TryGetValue(priority ?? string.Empty, out var brushes) ? brushes.Dot : ChipBrushes.FallbackDot;

    private static (SolidColorBrush Tint, SolidColorBrush Dot) Pair(byte r, byte g, byte b) =>
        (ChipBrushes.CreateTint(r, g, b), ChipBrushes.CreateDot(r, g, b));
}
