// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Pages;

namespace TableViewSampleApp.Converters;

public sealed partial class ShowcaseDepartmentTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        ShowcasePage.Vibrant ? ChipBrushes.DepartmentTint(value as string) : ChipBrushes.Transparent;

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

public sealed partial class ShowcaseDepartmentDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        ShowcasePage.Vibrant ? ChipBrushes.DepartmentDot(value as string) : ChipBrushes.Transparent;

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotImplementedException();
}

public sealed partial class ShowcaseSalaryTintConverter : IValueConverter
{
    private static readonly SolidColorBrush s_high = ChipBrushes.CreateTint(0x16, 0xA3, 0x4A, alpha: 0x4D);
    private static readonly SolidColorBrush s_mid = ChipBrushes.CreateTint(0xF5, 0x9E, 0x0B, alpha: 0x4D);
    private static readonly SolidColorBrush s_low = ChipBrushes.CreateTint(0xDC, 0x26, 0x26, alpha: 0x4D);

    public object Convert(object value, Type targetType, object parameter, string language)
    {
        if (!ShowcasePage.Vibrant || value is not double salary)
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
