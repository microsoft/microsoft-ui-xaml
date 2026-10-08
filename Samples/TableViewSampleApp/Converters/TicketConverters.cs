// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Microsoft.UI;
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
        ["Sev 1"] = (new SolidColorBrush(ColorHelper.FromArgb(0x33, 0xDC, 0x26, 0x26)), new SolidColorBrush(ColorHelper.FromArgb(0xFF, 0xDC, 0x26, 0x26))),
        ["Sev 2"] = (new SolidColorBrush(ColorHelper.FromArgb(0x33, 0xEA, 0x58, 0x0C)), new SolidColorBrush(ColorHelper.FromArgb(0xFF, 0xEA, 0x58, 0x0C))),
        ["Sev 3"] = (new SolidColorBrush(ColorHelper.FromArgb(0x33, 0xCA, 0x8A, 0x04)), new SolidColorBrush(ColorHelper.FromArgb(0xFF, 0xCA, 0x8A, 0x04))),
        ["Sev 4"] = (new SolidColorBrush(ColorHelper.FromArgb(0x33, 0x64, 0x74, 0x8B)), new SolidColorBrush(ColorHelper.FromArgb(0xFF, 0x64, 0x74, 0x8B))),
    };

    public static Brush PriorityTint(string priority) =>
        ChipBrushes.IsHighContrast ? ChipBrushes.Transparent
        : s_priorityBrushes.TryGetValue(priority ?? string.Empty, out var brushes) ? brushes.Tint : ChipBrushes.Transparent;

    public static Brush PriorityDot(string priority) =>
        ChipBrushes.IsHighContrast ? ChipBrushes.HighContrastForeground
        : s_priorityBrushes.TryGetValue(priority ?? string.Empty, out var brushes) ? brushes.Dot : ChipBrushes.HighContrastForeground;
}
