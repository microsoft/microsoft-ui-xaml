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
/// Shared palette for every chip and tinted cell in the gallery (Department, Active, Standing,
/// Country, Priority, Salary, Task Manager heat).
/// <list type="number">
///   <item><description>Each brush is created once (<see cref="CreateTint"/> / <see cref="CreateDot"/>)
///     and handed back by reference. A converter that returns <c>new SolidColorBrush(...)</c>
///     allocates per realized cell per call, which under virtualization is continuous churn.</description></item>
///   <item><description>Under a Windows Contrast theme the brand colours are dropped: tints become
///     transparent and dots take the system window-text colour, so the user's guaranteed contrast
///     pair wins. The brushes are RECOLOURED IN PLACE by <see cref="Refresh"/> (MainWindow calls it
///     when the system colours change), so cells that are already realized follow a switch at once;
///     a converter alone would only re-run when the bound value changes. Light and Dark need no
///     change: the tints are translucent and the dots are mid-tone.</description></item>
/// </list>
/// </summary>
public static class ChipBrushes
{
    private const byte TintAlpha = 0x33;

    // Every palette brush with its brand colour; declared first so the initializers below can register.
    private static readonly List<(SolidColorBrush Brush, Windows.UI.Color Color, bool IsDot)> s_palette = new();
    private static readonly AccessibilitySettings s_accessibilitySettings = new();
    private static readonly UISettings s_uiSettings = new();

    private static readonly SolidColorBrush s_transparent = new(Colors.Transparent);

    private static readonly Dictionary<string, SolidColorBrush> s_departmentTints = BuildDepartmentBrushes(dot: false);
    private static readonly Dictionary<string, SolidColorBrush> s_departmentDots = BuildDepartmentBrushes(dot: true);

    private static readonly SolidColorBrush s_fallbackTint = CreateTint(0x64, 0x74, 0x8B);
    private static readonly SolidColorBrush s_fallbackDot = CreateDot(0x64, 0x74, 0x8B);

    private static readonly SolidColorBrush s_activeTint = CreateTint(0x16, 0xA3, 0x4A);
    private static readonly SolidColorBrush s_activeDot = CreateDot(0x16, 0xA3, 0x4A);
    private static readonly SolidColorBrush s_inactiveTint = CreateTint(0x64, 0x74, 0x8B);
    private static readonly SolidColorBrush s_inactiveDot = CreateDot(0x64, 0x74, 0x8B);

    /// <summary>True while Windows runs a Contrast theme. The property is live.</summary>
    public static bool IsHighContrast => s_accessibilitySettings.HighContrast;

    /// <summary>Shared transparent brush: "no tint" in every theme.</summary>
    public static Brush Transparent => s_transparent;

    /// <summary>The fallback (slate) dot, for a value with no colour of its own.</summary>
    public static Brush FallbackDot => s_fallbackDot;

    /// <summary>A translucent chip background (alpha <paramref name="alpha"/>); transparent under a Contrast theme.</summary>
    public static SolidColorBrush CreateTint(byte r, byte g, byte b, byte alpha = TintAlpha) =>
        Register(ColorHelper.FromArgb(alpha, r, g, b), isDot: false);

    /// <summary>A solid chip dot; the system window-text colour under a Contrast theme.</summary>
    public static SolidColorBrush CreateDot(byte r, byte g, byte b) =>
        Register(ColorHelper.FromArgb(0xFF, r, g, b), isDot: true);

    /// <summary>
    /// Re-reads the Contrast state and recolours every palette brush in place. Call on the UI
    /// thread when the system colours change (UISettings.ColorValuesChanged).
    /// </summary>
    public static void Refresh()
    {
        var highContrast = IsHighContrast;
        var windowText = s_uiSettings.UIElementColor(UIElementType.WindowText);
        foreach (var (brush, color, isDot) in s_palette)
        {
            brush.Color = ColorFor(color, isDot, highContrast, windowText);
        }
    }

    public static Brush DepartmentTint(string? department) =>
        department is not null && s_departmentTints.TryGetValue(department, out var brush) ? brush : s_fallbackTint;

    public static Brush DepartmentDot(string? department) =>
        department is not null && s_departmentDots.TryGetValue(department, out var brush) ? brush : s_fallbackDot;

    public static Brush ActiveTint(bool isActive) => isActive ? s_activeTint : s_inactiveTint;

    public static Brush ActiveDot(bool isActive) => isActive ? s_activeDot : s_inactiveDot;

    private static SolidColorBrush Register(Windows.UI.Color color, bool isDot)
    {
        var highContrast = IsHighContrast;
        var brush = new SolidColorBrush(ColorFor(color, isDot, highContrast, highContrast ? s_uiSettings.UIElementColor(UIElementType.WindowText) : default));
        s_palette.Add((brush, color, isDot));
        return brush;
    }

    private static Windows.UI.Color ColorFor(Windows.UI.Color color, bool isDot, bool highContrast, Windows.UI.Color windowText) =>
        !highContrast ? color : isDot ? windowText : Colors.Transparent;

    private static Dictionary<string, SolidColorBrush> BuildDepartmentBrushes(bool dot)
    {
        SolidColorBrush Create(byte r, byte g, byte b) => dot ? CreateDot(r, g, b) : CreateTint(r, g, b);
        return new(StringComparer.Ordinal)
        {
            ["Marketing"] = Create(0xA8, 0x55, 0xF7),   // purple
            ["Product"] = Create(0x0E, 0xA5, 0xE9),     // blue
            ["Finance"] = Create(0x22, 0xC5, 0x5E),     // green
            ["Design"] = Create(0xEC, 0x48, 0x99),      // pink
            ["Sales"] = Create(0x14, 0xB8, 0xA6),       // teal
            ["HR"] = Create(0xF5, 0x9E, 0x0B),          // amber
            ["Engineering"] = Create(0x63, 0x66, 0xF1), // indigo
            ["Operations"] = Create(0xEF, 0x44, 0x44),  // red
        };
    }
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
