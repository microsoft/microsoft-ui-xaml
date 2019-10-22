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

    public class StringToNumberBoxSpinButtonPlacementModeConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is String)
            {
                switch (value)
                {
                    case "Hidden":
                        return NumberBoxSpinButtonPlacementMode.Hidden;
                    case "Inline":
                        return NumberBoxSpinButtonPlacementMode.Inline;
                }
            }
            return false;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            if (value is NumberBoxSpinButtonPlacementMode)
            {
                switch (value)
                {
                    case NumberBoxSpinButtonPlacementMode.Hidden:
                        return "Hidden";
                    case NumberBoxSpinButtonPlacementMode.Inline:
                        return "Inline";
                }
            }
            return false;
        }
    }

    public class DoubleToIntConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is double)
            {
                return System.Convert.ToInt32(value);
            }
            return false;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            if (value is int)
                return System.Convert.ToDouble(value);
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
                    case "IconMessage":
                        return NumberBoxBasicValidationMode.IconMessage;
                    case "TextBlockMessage":
                        return NumberBoxBasicValidationMode.TextBlockMessage;
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
                    case NumberBoxBasicValidationMode.IconMessage:
                        return "IconMessage";
                    case NumberBoxBasicValidationMode.TextBlockMessage:
                        return "TextBlockMessage";
                    case NumberBoxBasicValidationMode.InvalidInputOverwritten:
                        return "InvalidInputOverwritten";

                }
            }
            return false;
        }
    }
}