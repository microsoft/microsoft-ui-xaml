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


    public class StringToNumberBoxMinMaxModeConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is String)
            {
                switch (value)
                {
                    case "None":
                        return NumberBoxMinMaxMode.None;
                    case "MinEnabled":
                        return NumberBoxMinMaxMode.MinEnabled;
                    case "MaxEnabled":
                        return NumberBoxMinMaxMode.MaxEnabled;
                    case "MinAndMaxEnabled":
                        return NumberBoxMinMaxMode.MinAndMaxEnabled;
                    case "WrapEnabled":
                        return NumberBoxMinMaxMode.WrapEnabled;
                }
            }
            return false;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            if (value is NumberBoxMinMaxMode)
            {
                switch (value)
                {
                    case NumberBoxMinMaxMode.None:
                        return "None";
                    case NumberBoxMinMaxMode.MinEnabled:
                        return "MinEnabled";
                    case NumberBoxMinMaxMode.MaxEnabled:
                        return "MaxEnabled";
                    case NumberBoxMinMaxMode.MinAndMaxEnabled:
                        return "MinAndMaxEnabled";
                    case NumberBoxMinMaxMode.WrapEnabled:
                        return "WrapEnabled";
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

    public class StringToRoundingAlgConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is String)
            {
                switch (value)
                {
                    case "None":
                        return RoundingAlgorithm.None;
                    case "RoundAwayFromZero":
                        return RoundingAlgorithm.RoundAwayFromZero;
                    case "RoundDown":
                        return RoundingAlgorithm.RoundDown;
                    case "RoundHalfAwayFromZero":
                        return RoundingAlgorithm.RoundHalfAwayFromZero;
                    case "RoundHalfDown":
                        return RoundingAlgorithm.RoundHalfDown;
                    case "RoundHalfToEven":
                        return RoundingAlgorithm.RoundHalfToEven;
                    case "RoundHalfToOdd":
                        return RoundingAlgorithm.RoundHalfToOdd;
                    case "RoundHalfTowardsZero":
                        return RoundingAlgorithm.RoundHalfTowardsZero;
                    case "RoundHalfUp":
                        return RoundingAlgorithm.RoundHalfUp;
                    case "RoundTowardsZero":
                        return RoundingAlgorithm.RoundTowardsZero;
                    case "RoundUp":
                        return RoundingAlgorithm.RoundUp;
                }
            }
            return false;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            if (value is RoundingAlgorithm)
            {
                switch (value)
                {
                    case RoundingAlgorithm.None:
                        return "None";
                    case RoundingAlgorithm.RoundAwayFromZero:
                        return "RoundAwayFromZero";
                    case RoundingAlgorithm.RoundDown:
                        return "RoundDown";
                    case RoundingAlgorithm.RoundHalfAwayFromZero:
                        return "RoundHalfAwayFromZero";
                    case RoundingAlgorithm.RoundHalfDown:
                        return "RoundHalfDown";
                    case RoundingAlgorithm.RoundHalfToEven:
                        return "RoundHalfToEven";
                    case RoundingAlgorithm.RoundHalfToOdd:
                        return "RoundHalfToOdd";
                    case RoundingAlgorithm.RoundHalfTowardsZero:
                        return "RoundHalfTowardsZero";
                    case RoundingAlgorithm.RoundHalfUp:
                        return "RoundHalfUp";
                    case RoundingAlgorithm.RoundTowardsZero:
                        return "RoundTowardsZero";
                    case RoundingAlgorithm.RoundUp:
                        return "RoundUp";
                }
            }
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
            if (value is NumberBoxMinMaxMode)
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


    public class StringToRounderConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is String)
            {
                switch (value)
                {
                    case "IncrementNumberRounder":
                        return NumberBoxNumberRounder.IncrementNumberRounder;
                    case "SignificantDigitsNumberRounder":
                        return NumberBoxNumberRounder.SignificantDigitsNumberRounder;
                    case "None":
                        return NumberBoxNumberRounder.None;

                }
            }
            return false;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            if (value is NumberBoxMinMaxMode)
            {
                switch (value)
                {
                    case NumberBoxNumberRounder.IncrementNumberRounder:
                        return "IncrementNumberRounder";
                    case NumberBoxNumberRounder.SignificantDigitsNumberRounder:
                        return "SignificantDigitsNumberRounder";
                    case NumberBoxNumberRounder.None:
                        return "NoRounding";
                }
            }
            return false;
        }
    }




    public class DoubleToUIntConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is double)
            {
                return System.Convert.ToUInt32(value);
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
}