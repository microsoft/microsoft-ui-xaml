// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using Microsoft.UI;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Converters;

namespace TableViewSampleApp.Pages;

// Vibrant cell tints for the Showcase page. Each tinted cell binds a Border background to a row
// value through one of these converters, so a live update on the bound Person recolors only that
// cell. They follow the shared converter rules: sealed partial, brushes built once and returned
// by reference, and a transparent background under a Windows Contrast theme so the theme's own
// text and background pair wins. ShowcasePage.Vibrant turns the tints off.

/// <summary>Department pill background: the shared department palette, or transparent.</summary>
public sealed partial class ShowcaseDepartmentTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        ShowcasePage.Vibrant ? ChipBrushes.DepartmentTint(value as string) : ChipBrushes.Transparent;

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>Department pill dot: the shared department palette, or transparent.</summary>
public sealed partial class ShowcaseDepartmentDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        ShowcasePage.Vibrant ? ChipBrushes.DepartmentDot(value as string) : ChipBrushes.Transparent;

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

/// <summary>
/// Stoplight tint for the Salary cell: green from 190,000, amber from 150,000, red below. The
/// thresholds suit the Person salary band (110,000 to 230,000).
/// </summary>
public sealed partial class ShowcaseSalaryTintConverter : IValueConverter
{
    private static readonly SolidColorBrush s_high = new(ColorHelper.FromArgb(0x4D, 0x16, 0xA3, 0x4A));
    private static readonly SolidColorBrush s_mid = new(ColorHelper.FromArgb(0x4D, 0xF5, 0x9E, 0x0B));
    private static readonly SolidColorBrush s_low = new(ColorHelper.FromArgb(0x4D, 0xDC, 0x26, 0x26));

    public object Convert(object value, Type targetType, object parameter, string language)
    {
        if (!ShowcasePage.Vibrant || ChipBrushes.IsHighContrast || value is not double salary)
        {
            return ChipBrushes.Transparent;
        }

        return salary >= 190_000 ? s_high : salary >= 150_000 ? s_mid : s_low;
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

public sealed partial class PersonAvatarBrushConverter : IValueConverter
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

public sealed partial class PersonAvatarColorNameConverter : IValueConverter
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
