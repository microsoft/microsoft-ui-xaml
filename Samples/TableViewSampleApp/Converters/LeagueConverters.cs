// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Data;

namespace TableViewSampleApp.Converters;

internal static class LeagueChipPalette
{
    private static readonly Dictionary<string, SolidColorBrush> s_standingTints = BuildStanding(dot: false);
    private static readonly Dictionary<string, SolidColorBrush> s_standingDots = BuildStanding(dot: true);
    private static readonly Dictionary<string, SolidColorBrush> s_countryTints = BuildCountry(dot: false);
    private static readonly Dictionary<string, SolidColorBrush> s_countryDots = BuildCountry(dot: true);
    private static readonly SolidColorBrush s_fallbackTint = ChipBrushes.CreateTint(0x64, 0x74, 0x8B);
    private static readonly SolidColorBrush s_fallbackDot = ChipBrushes.CreateDot(0x64, 0x74, 0x8B);

    internal static Brush StandingTint(string band) => Tint(s_standingTints, band);

    internal static Brush StandingDot(string band) => Dot(s_standingDots, band);

    internal static Brush CountryTint(string? country) => Tint(s_countryTints, country);

    internal static Brush CountryDot(string? country) => Dot(s_countryDots, country);

    private static Brush Tint(Dictionary<string, SolidColorBrush> map, string? key) =>
        key is not null && map.TryGetValue(key, out var brush) ? brush : s_fallbackTint;

    private static Brush Dot(Dictionary<string, SolidColorBrush> map, string? key) =>
        key is not null && map.TryGetValue(key, out var brush) ? brush : s_fallbackDot;

    private static SolidColorBrush Create(bool dot, byte r, byte g, byte b) =>
        dot ? ChipBrushes.CreateDot(r, g, b) : ChipBrushes.CreateTint(r, g, b);

    private static Dictionary<string, SolidColorBrush> BuildStanding(bool dot) => new(StringComparer.Ordinal)
    {
        ["Qualified"] = Create(dot, 0x16, 0xA3, 0x4A),
        ["Playoff"] = Create(dot, 0xF5, 0x9E, 0x0B),
        ["Eliminated"] = Create(dot, 0x64, 0x74, 0x8B),
    };

    private static Dictionary<string, SolidColorBrush> BuildCountry(bool dot) => new(StringComparer.Ordinal)
    {
        ["England"] = Create(dot, 0xEF, 0x44, 0x44),
        ["Spain"] = Create(dot, 0xF5, 0x9E, 0x0B),
        ["Italy"] = Create(dot, 0x0E, 0xA5, 0xE9),
        ["Germany"] = Create(dot, 0x64, 0x74, 0x8B),
        ["France"] = Create(dot, 0x63, 0x66, 0xF1),
        ["Portugal"] = Create(dot, 0x22, 0xC5, 0x5E),
        ["Netherlands"] = Create(dot, 0xF9, 0x73, 0x16),
        ["Belgium"] = Create(dot, 0xA8, 0x55, 0xF7),
    };
}

public sealed partial class SortStandingTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => LeagueChipPalette.StandingTint(LeagueData.Band(value));

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

public sealed partial class SortStandingDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => LeagueChipPalette.StandingDot(LeagueData.Band(value));

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

public sealed partial class SortStandingTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => LeagueData.Band(value);

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

public sealed partial class SortCountryTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => LeagueChipPalette.CountryTint(value as string);

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

public sealed partial class SortCountryDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => LeagueChipPalette.CountryDot(value as string);

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}
