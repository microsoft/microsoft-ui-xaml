// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Globalization;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Windows.UI.ViewManagement;

namespace TableViewSampleApp.Converters;

/// <summary>
/// Shared brush cache for every chip in the gallery (Department chip, Active status chip).
/// <list type="number">
///   <item><description>Brushes are built once into <c>static readonly</c> dictionaries and
///     handed back by reference. A converter that returns <c>new SolidColorBrush(...)</c>
///     allocates per realized cell per call, which under virtualization is continuous churn.</description></item>
///   <item><description>Under a Windows Contrast theme the brand tints are dropped: the chip
///     falls back to transparent plus the theme's own stroke and text brushes, so the user's
///     guaranteed contrast pair wins. <see cref="IsHighContrast"/> is read live, so a theme
///     switch mid-session is picked up the next time a cell converts.</description></item>
/// </list>
/// </summary>
public static class ChipBrushes
{
    private const byte TintAlpha = 0x33;

    private static AccessibilitySettings? s_accessibilitySettings;
    private static Brush? s_highContrastForeground;

    private static readonly SolidColorBrush s_transparent = new(Colors.Transparent);

    private static readonly Dictionary<string, SolidColorBrush> s_departmentTints = BuildDepartmentBrushes(TintAlpha);
    private static readonly Dictionary<string, SolidColorBrush> s_departmentDots = BuildDepartmentBrushes(0xFF);

    private static readonly SolidColorBrush s_fallbackTint = new(ColorHelper.FromArgb(TintAlpha, 0x64, 0x74, 0x8B));
    private static readonly SolidColorBrush s_fallbackDot = new(ColorHelper.FromArgb(0xFF, 0x64, 0x74, 0x8B));

    private static readonly SolidColorBrush s_activeTint = new(ColorHelper.FromArgb(TintAlpha, 0x16, 0xA3, 0x4A));
    private static readonly SolidColorBrush s_activeDot = new(ColorHelper.FromArgb(0xFF, 0x16, 0xA3, 0x4A));
    private static readonly SolidColorBrush s_inactiveTint = new(ColorHelper.FromArgb(TintAlpha, 0x64, 0x74, 0x8B));
    private static readonly SolidColorBrush s_inactiveDot = new(ColorHelper.FromArgb(0xFF, 0x64, 0x74, 0x8B));

    /// <summary>True while Windows runs a Contrast theme. The property is live.</summary>
    public static bool IsHighContrast
    {
        get
        {
            s_accessibilitySettings ??= new AccessibilitySettings();
            return s_accessibilitySettings.HighContrast;
        }
    }

    /// <summary>Cached transparent brush; the HighContrast tint for every chip.</summary>
    public static Brush Transparent => s_transparent;

    /// <summary>Theme-owned foreground used for chip dots under a Contrast theme.</summary>
    public static Brush HighContrastForeground =>
        s_highContrastForeground ??=
            Application.Current.Resources["TextFillColorPrimaryBrush"] as Brush ?? s_transparent;

    public static Brush DepartmentTint(string? department)
    {
        if (IsHighContrast)
        {
            return s_transparent;
        }

        return department is not null && s_departmentTints.TryGetValue(department, out var brush)
            ? brush
            : s_fallbackTint;
    }

    public static Brush DepartmentDot(string? department)
    {
        if (IsHighContrast)
        {
            return HighContrastForeground;
        }

        return department is not null && s_departmentDots.TryGetValue(department, out var brush)
            ? brush
            : s_fallbackDot;
    }

    public static Brush ActiveTint(bool isActive) =>
        IsHighContrast ? s_transparent : (isActive ? s_activeTint : s_inactiveTint);

    public static Brush ActiveDot(bool isActive) =>
        IsHighContrast ? HighContrastForeground : (isActive ? s_activeDot : s_inactiveDot);

    private static Dictionary<string, SolidColorBrush> BuildDepartmentBrushes(byte alpha) => new(StringComparer.Ordinal)
    {
        ["Marketing"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xA8, 0x55, 0xF7)),   // purple
        ["Product"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x0E, 0xA5, 0xE9)),     // blue
        ["Finance"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x22, 0xC5, 0x5E)),     // green
        ["Design"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xEC, 0x48, 0x99)),      // pink
        ["Sales"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x14, 0xB8, 0xA6)),       // teal
        ["HR"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xF5, 0x9E, 0x0B)),          // amber
        ["Engineering"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x63, 0x66, 0xF1)), // indigo
        ["Operations"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xEF, 0x44, 0x44)),  // red
    };
}

/// <summary>Translucent chip background for the Department chip.</summary>
public sealed partial class DepartmentChipTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        ChipBrushes.DepartmentTint(value as string);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>Solid accent dot for the Department chip.</summary>
public sealed partial class DepartmentChipDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        ChipBrushes.DepartmentDot(value as string);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>Translucent chip background for the Active / Inactive status chip.</summary>
public sealed partial class ActiveChipTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        ChipBrushes.ActiveTint(value is bool isActive && isActive);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>Solid accent dot for the Active / Inactive status chip.</summary>
public sealed partial class ActiveChipDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        ChipBrushes.ActiveDot(value is bool isActive && isActive);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>
/// Text label for the status chip. The chip never conveys meaning by colour alone; this label
/// is both the visible and the accessible value.
/// </summary>
public sealed partial class ActiveChipTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        value is bool isActive && isActive ? "Active" : "Inactive";

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>
/// Builds a per-row accessible name from a format in ConverterParameter, e.g.
/// <c>ConverterParameter='Active for {0}'</c>. Formats with the current culture.
/// </summary>
public sealed partial class RowAutomationNameConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        var rowName = value?.ToString();
        if (string.IsNullOrWhiteSpace(rowName))
        {
            rowName = "row";
        }

        return parameter is string format && !string.IsNullOrWhiteSpace(format)
            ? string.Format(CultureInfo.CurrentCulture, format, rowName)
            : rowName;
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>Formats a salary as whole currency units (C0) in the current culture.</summary>
public sealed partial class SalaryTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        Format(value);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();

    /// <summary>The same formatting for code-behind readouts.</summary>
    public static string Format(object? value) => value switch
    {
        double amount => amount.ToString("C0", CultureInfo.CurrentCulture),
        decimal amount => amount.ToString("C0", CultureInfo.CurrentCulture),
        int amount => amount.ToString("C0", CultureInfo.CurrentCulture),
        _ => value?.ToString() ?? string.Empty,
    };
}

/// <summary>
/// Turns an avatar path (an <c>ms-appx:///</c> URI string) into an <see cref="ImageSource"/>
/// for <c>PersonPicture.ProfilePicture</c>. Null, blank or malformed input returns null, so
/// PersonPicture falls back to initials from its DisplayName and the cell is never blank.
/// Never bind a raw string to an ImageSource: an empty string throws inside the generated
/// bindings and blanks the whole template. One BitmapImage is cached per distinct path.
/// </summary>
public sealed partial class AvatarImageConverter : IValueConverter
{
    private static readonly Dictionary<string, BitmapImage> s_cache = new(StringComparer.Ordinal);

    public object? Convert(object value, Type targetType, object parameter, string language) =>
        FromPath(value as string);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();

    public static ImageSource? FromPath(string? path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            return null;
        }

        if (s_cache.TryGetValue(path, out var cached))
        {
            return cached;
        }

        if (!Uri.TryCreate(path, UriKind.Absolute, out var uri))
        {
            return null;
        }

        try
        {
            var image = new BitmapImage
            {
                DecodePixelWidth = 56,
                DecodePixelType = DecodePixelType.Logical,
                UriSource = uri,
            };
            s_cache[path] = image;
            return image;
        }
        catch (Exception)
        {
            return null;
        }
    }
}
