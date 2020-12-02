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
            try
            {
                return Double.Parse((string)value) * 100;
            }
            catch
            {
                return 0.0;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            return (((double)value)/100).ToString();
        }
    }
}
