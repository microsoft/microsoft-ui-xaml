using Microsoft.UI.Xaml.Controls;
using System;
using Windows.Globalization.NumberFormatting;
using Windows.UI.Xaml.Data;

namespace MUXControlsTestApp
{
    public class NullableBooleanToBooleanConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is bool?)
            {
                return (bool)value;
            }
            return false;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            if (value is bool)
                return (bool)value;
            return false;
        }
    }

    public class StringToValidationModeConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is String)
            {
                switch (value)
                {
                    case "Disabled":
                        return NumberBoxBasicValidationMode.Disabled;
                    case "InvalidInputOverwritten":
                        return NumberBoxBasicValidationMode.InvalidInputOverwritten;

                }
            }
            return false;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            if (value is NumberBoxBasicValidationMode)
            {
                switch (value)
                {
                    case NumberBoxBasicValidationMode.Disabled:
                        return "Disabled";
                    case NumberBoxBasicValidationMode.InvalidInputOverwritten:
                        return "InvalidInputOverwritten";

                }
            }
            return false;
        }
    }
}