// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;

namespace SelectionModelSampleApp.Common
{
    /// <summary>
    /// Maps the tri-state selection value reported by SelectionModel.IsSelected* onto a brush:
    /// true = accent, null = partial (muted accent), false = transparent.
    /// </summary>
    public partial class SelectionStateToBrushConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            var isSelected = value as bool?;
            if (isSelected == true)
            {
                return Application.Current.Resources["AccentFillColorDefaultBrush"];
            }

            if (isSelected == null)
            {
                return new SolidColorBrush(Colors.Goldenrod);
            }

            return Application.Current.Resources["ControlFillColorDefaultBrush"];
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
            => throw new NotImplementedException();
    }

    public partial class SelectionStateToGlyphConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            var isSelected = value as bool?;
            return isSelected switch
            {
                true => "\uE73E",   // checkmark
                null => "\uE73C",   // partial
                _ => string.Empty,
            };
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
            => throw new NotImplementedException();
    }
}
