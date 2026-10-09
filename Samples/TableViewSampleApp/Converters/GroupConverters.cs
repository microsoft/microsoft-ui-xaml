// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using Microsoft.UI.Xaml.Data;

namespace TableViewSampleApp.Converters;

public sealed partial class GroupExpansionTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => value is bool isExpanded && isExpanded ? "Expanded" : "Collapsed";

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotImplementedException();
}

// Bound to ItemCount rather than ItemCountText so the header always shows a count.
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
