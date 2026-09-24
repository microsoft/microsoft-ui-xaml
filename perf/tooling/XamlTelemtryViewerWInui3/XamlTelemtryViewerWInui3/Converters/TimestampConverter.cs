using Microsoft.UI.Xaml.Data;
using System;

namespace XamlTelemtryViewerWInui3.Converters
{
    public sealed class TimestampConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is DateTime dateTime)
            {
                // Format: HH:mm:ss:fff:uuu (hour:minute:second:millisecond:microsecond)
                // Microsecond sub-part = (ticks % 10000) / 10, gives 000-999
                long ticksPerMicrosecond = 10;
                long microsecondPart = (dateTime.Ticks % 10000) / ticksPerMicrosecond;
                
                return $"{dateTime:HH:mm:ss:fff}:{microsecondPart:D3}";
            }

            return string.Empty;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
