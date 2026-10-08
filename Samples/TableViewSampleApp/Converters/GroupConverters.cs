// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using Microsoft.UI.Xaml.Data;

namespace TableViewSampleApp.Converters;

/// <summary>Expansion chip text for the custom group header.</summary>
public sealed partial class GroupExpansionTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => value is bool isExpanded && isExpanded ? "Expanded" : "Collapsed";

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotImplementedException();
}

/// <summary>
/// Formats TableViewGroupInfo.ItemCount for the custom group header. Bound to ItemCount (Int32)
/// rather than the ItemCountText projection so the header always shows a count.
/// </summary>
public sealed partial class GroupCountTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        var count = value is int number ? number : 0;
        return string.Format(CultureInfo.CurrentCulture, count == 1 ? "{0} item" : "{0} items", count);
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotImplementedException();
}
