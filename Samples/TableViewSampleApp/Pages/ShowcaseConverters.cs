// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using Microsoft.UI;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;

namespace TableViewSampleApp.Pages;

// Data-bound conditional cell tints for the Showcase page. Each tinted cell
// binds a Border background (or text) to a row value through one of these value
// converters, so a live mutation on the bound Person recolors just that one cell
// with no per-column rebuild. All tint converters honor the shared
// ShowcasePage.Vibrant flag so the "Vibrant cells" toggle can turn them off.
//
// These live in a page-private file (not a shared converter library) so the
// Showcase page owns them outright; they read ShowcasePage.Vibrant directly
// rather than holding a page back-reference.

/// <summary>
/// Tints the Department cell with a per-category pill hue, gated by
/// <see cref="ShowcasePage.Vibrant"/>. Returns a theme-independent,
/// semi-transparent brush (reads correctly over both Light and Dark row
/// backgrounds), or transparent when vibrant cells are turned off.
/// </summary>
public sealed class ShowcaseDepartmentTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        if (!ShowcasePage.Vibrant || value is not string department)
        {
            return new SolidColorBrush(Colors.Transparent);
        }

        return new SolidColorBrush(DepartmentColor(department, 0x4D));
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();

    internal static Windows.UI.Color DepartmentColor(string department, byte alpha) => department switch
    {
        "Engineering" => ColorHelper.FromArgb(alpha, 0x00, 0x78, 0xD4),
        "Sales" => ColorHelper.FromArgb(alpha, 0x14, 0xB8, 0xA6),
        "Marketing" => ColorHelper.FromArgb(alpha, 0xA8, 0x55, 0xF7),
        "HR" => ColorHelper.FromArgb(alpha, 0xF5, 0x9E, 0x0B),
        "Operations" => ColorHelper.FromArgb(alpha, 0xEF, 0x44, 0x44),
        "Design" => ColorHelper.FromArgb(alpha, 0xEC, 0x48, 0x99),
        "Product" => ColorHelper.FromArgb(alpha, 0x0E, 0xA5, 0xE9),
        "Finance" => ColorHelper.FromArgb(alpha, 0x22, 0xC5, 0x5E),
        _ => ColorHelper.FromArgb(alpha, 0x64, 0x74, 0x8B),
    };
}

/// <summary>
/// Solid (full-opacity) per-category dot drawn at the leading edge of the
/// Department pill, gated by <see cref="ShowcasePage.Vibrant"/>. Mirrors the
/// pill's hue but fully saturated so it reads as a crisp status dot.
/// </summary>
public sealed class ShowcaseDepartmentDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        if (!ShowcasePage.Vibrant || value is not string department)
        {
            return new SolidColorBrush(Colors.Transparent);
        }

        return new SolidColorBrush(ShowcaseDepartmentTintConverter.DepartmentColor(department, 0xFF));
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

public sealed class PersonAvatarBrushConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        var department = value as string ?? string.Empty;
        return new SolidColorBrush(PersonAvatarColor(department));
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();

    internal static Windows.UI.Color PersonAvatarColor(string department) => department switch
    {
        "Engineering" or "Product" or "Sales" => ColorHelper.FromArgb(0xFF, 0x00, 0x78, 0xD4),
        "Finance" or "Operations" => ColorHelper.FromArgb(0xFF, 0x10, 0x7C, 0x10),
        "Design" or "HR" or "Marketing" => ColorHelper.FromArgb(0xFF, 0xA8, 0x36, 0x00),
        _ => ColorHelper.FromArgb(0xFF, 0x00, 0x78, 0xD4),
    };
}

public sealed class PersonAvatarColorNameConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        var department = value as string ?? string.Empty;
        return department switch
        {
            "Engineering" or "Product" or "Sales" => "blue avatar",
            "Finance" or "Operations" => "green avatar",
            "Design" or "HR" or "Marketing" => "rust avatar",
            _ => "blue avatar",
        };
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>
/// Stoplight tint for the Salary cell — green when high, amber for mid, red for
/// low — gated by <see cref="ShowcasePage.Vibrant"/>. Thresholds are tuned for
/// the Person dataset's salary band (~115k–220k). Transparent when vibrant
/// cells are off.
/// </summary>
public sealed class ShowcaseSalaryTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        if (!ShowcasePage.Vibrant || value is not double salary)
        {
            return new SolidColorBrush(Colors.Transparent);
        }

        var color = salary >= 190_000 ? ColorHelper.FromArgb(0x4D, 0x16, 0xA3, 0x4A)   // green
                  : salary >= 150_000 ? ColorHelper.FromArgb(0x4D, 0xF5, 0x9E, 0x0B)    // amber
                  : ColorHelper.FromArgb(0x4D, 0xDC, 0x26, 0x26);                        // red
        return new SolidColorBrush(color);
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>
/// Formats the Salary cell as whole-dollar currency in the current culture.
/// </summary>
public sealed class ShowcaseSalaryTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        value is double salary ? salary.ToString("C0", CultureInfo.CurrentCulture) : string.Empty;

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>
/// Status-chip background — green when Active, slate when Inactive — gated by
/// <see cref="ShowcasePage.Vibrant"/>. Transparent when vibrant cells are off.
/// </summary>
public sealed class ShowcaseActiveChipConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        if (!ShowcasePage.Vibrant || value is not bool isActive)
        {
            return new SolidColorBrush(Colors.Transparent);
        }

        var color = isActive
            ? ColorHelper.FromArgb(0x59, 0x16, 0xA3, 0x4A)   // green
            : ColorHelper.FromArgb(0x59, 0x64, 0x74, 0x8B);  // slate
        return new SolidColorBrush(color);
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>
/// Status-chip label — "Active" / "Inactive".
/// </summary>
public sealed class ShowcaseActiveTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        value is bool isActive ? (isActive ? "Active" : "Inactive") : string.Empty;

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}
