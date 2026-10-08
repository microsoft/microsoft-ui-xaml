// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Microsoft.UI;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Data;

namespace TableViewSampleApp.Converters;

/// <summary>
/// Chip palette for the standings' Standing and Country template columns. Every brush is
/// created once into a static field and shared, so Convert never allocates. Under a Contrast
/// theme the tint drops to transparent and the dot to the theme text brush (the shared
/// <see cref="ChipBrushes"/> rule).
/// </summary>
internal static class LeagueChipPalette
{
    private const byte TintAlpha = 0x33;

    private static readonly Dictionary<string, SolidColorBrush> s_standingTints = BuildStanding(TintAlpha);
    private static readonly Dictionary<string, SolidColorBrush> s_standingDots = BuildStanding(0xFF);
    private static readonly Dictionary<string, SolidColorBrush> s_countryTints = BuildCountry(TintAlpha);
    private static readonly Dictionary<string, SolidColorBrush> s_countryDots = BuildCountry(0xFF);
    private static readonly SolidColorBrush s_fallbackTint = new(ColorHelper.FromArgb(TintAlpha, 0x64, 0x74, 0x8B));
    private static readonly SolidColorBrush s_fallbackDot = new(ColorHelper.FromArgb(0xFF, 0x64, 0x74, 0x8B));

    internal static Brush StandingTint(string band) => Tint(s_standingTints, band);

    internal static Brush StandingDot(string band) => Dot(s_standingDots, band);

    internal static Brush CountryTint(string? country) => Tint(s_countryTints, country);

    internal static Brush CountryDot(string? country) => Dot(s_countryDots, country);

    private static Brush Tint(Dictionary<string, SolidColorBrush> map, string? key) =>
        ChipBrushes.IsHighContrast ? ChipBrushes.Transparent
        : key is not null && map.TryGetValue(key, out var brush) ? brush : s_fallbackTint;

    private static Brush Dot(Dictionary<string, SolidColorBrush> map, string? key) =>
        ChipBrushes.IsHighContrast ? ChipBrushes.HighContrastForeground
        : key is not null && map.TryGetValue(key, out var brush) ? brush : s_fallbackDot;

    private static Dictionary<string, SolidColorBrush> BuildStanding(byte alpha) => new(StringComparer.Ordinal)
    {
        ["Qualified"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x16, 0xA3, 0x4A)),
        ["Playoff"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xF5, 0x9E, 0x0B)),
        ["Eliminated"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x64, 0x74, 0x8B)),
    };

    private static Dictionary<string, SolidColorBrush> BuildCountry(byte alpha) => new(StringComparer.Ordinal)
    {
        ["England"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xEF, 0x44, 0x44)),
        ["Spain"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xF5, 0x9E, 0x0B)),
        ["Italy"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x0E, 0xA5, 0xE9)),
        ["Germany"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x64, 0x74, 0x8B)),
        ["France"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x63, 0x66, 0xF1)),
        ["Portugal"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x22, 0xC5, 0x5E)),
        ["Netherlands"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xF9, 0x73, 0x16)),
        ["Belgium"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xA8, 0x55, 0xF7)),
    };
}

/// <summary>Chip background tint for the Standing template column. Returns a shared brush.</summary>
public sealed partial class SortStandingTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => LeagueChipPalette.StandingTint(LeagueData.Band(value));

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

/// <summary>Solid dot fill for the Standing chip. Returns a shared brush.</summary>
public sealed partial class SortStandingDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => LeagueChipPalette.StandingDot(LeagueData.Band(value));

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

/// <summary>Chip label: "Qualified", "Playoff" or "Eliminated".</summary>
public sealed partial class SortStandingTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => LeagueData.Band(value);

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

/// <summary>Chip background tint for the Country template column. Returns a shared brush.</summary>
public sealed partial class SortCountryTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => LeagueChipPalette.CountryTint(value as string);

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

/// <summary>Solid dot fill for the Country chip. Returns a shared brush.</summary>
public sealed partial class SortCountryDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => LeagueChipPalette.CountryDot(value as string);

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}
