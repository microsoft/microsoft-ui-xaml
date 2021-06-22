using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI.Xaml.Data;

namespace MUXControlsTestApp
{
    class DoubleToStringConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            double parsedValue = 0.0;
            return Double.TryParse((string)value, out parsedValue) ? parsedValue * 100 : 0.0;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            return (((double)value)/100).ToString();
        }
    }
}
