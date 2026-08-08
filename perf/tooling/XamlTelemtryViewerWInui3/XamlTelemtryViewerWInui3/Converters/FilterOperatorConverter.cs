using Microsoft.UI.Xaml.Data;
using System;
using XamlTelemtryViewerWInui3.Models;

namespace XamlTelemtryViewerWInui3.Converters
{
    public sealed class FilterOperatorConverter : IValueConverter
    {
        public object? Convert(object? value, Type targetType, object? parameter, string language)
        {
            if (value is FilterOperator op)
            {
                return op.ToString();
            }
            return null;
        }

        public object? ConvertBack(object? value, Type targetType, object? parameter, string language)
        {
            if (value is string str && Enum.TryParse<FilterOperator>(str, out var op))
            {
                return op;
            }
            return FilterOperator.Contains;
        }
    }
}
