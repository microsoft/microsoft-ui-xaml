using Microsoft.UI.Xaml.Data;
using System;
using XamlTelemtryViewerWInui3.Models;

namespace XamlTelemtryViewerWInui3.Converters
{
    public sealed class FilterFieldConverter : IValueConverter
    {
        public object? Convert(object? value, Type targetType, object? parameter, string language)
        {
            if (value is FilterField field)
            {
                return field.ToString();
            }
            return null;
        }

        public object? ConvertBack(object? value, Type targetType, object? parameter, string language)
        {
            if (value is string str && Enum.TryParse<FilterField>(str, out var field))
            {
                return field;
            }
            return FilterField.ProcessName;
        }
    }
}
