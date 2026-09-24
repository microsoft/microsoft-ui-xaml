using System;
using Microsoft.UI.Xaml.Data;

namespace XamlTelemtryViewerWInui3.Converters;

public sealed class GuidToLowercaseConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        if (value is Guid guid)
        {
            return guid.ToString().ToLower();
        }
        return value?.ToString() ?? string.Empty;
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language)
    {
        if (value is string str && Guid.TryParse(str, out var guid))
        {
            return guid;
        }
        return Guid.Empty;
    }
}
