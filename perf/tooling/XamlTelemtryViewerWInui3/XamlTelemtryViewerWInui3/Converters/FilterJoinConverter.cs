using Microsoft.UI.Xaml.Data;
using System;
using XamlTelemtryViewerWInui3.Models;

namespace XamlTelemtryViewerWInui3.Converters
{
    public sealed class FilterJoinConverter : IValueConverter
    {
        public object? Convert(object? value, Type targetType, object? parameter, string language)
        {
            if (value is FilterJoin join)
            {
                return join.ToString();
            }
            return null;
        }

        public object? ConvertBack(object? value, Type targetType, object? parameter, string language)
        {
            if (value is string str && Enum.TryParse<FilterJoin>(str, out var join))
            {
                return join;
            }
            return FilterJoin.And;
        }
    }
}
