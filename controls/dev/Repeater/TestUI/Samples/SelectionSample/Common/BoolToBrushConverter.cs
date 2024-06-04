// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;

namespace MUXControlsTestApp.Samples.Selection
{
    public class BoolToBrushConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            var val = value as bool?;
            if (!val.HasValue)
            {
                return new SolidColorBrush(Microsoft.UI.Colors.Yellow);
            }
            else
            {
                if (val.Value)
                {
                    return new SolidColorBrush(Microsoft.UI.Colors.Green);
                }
                else
                {
                    return new SolidColorBrush(Microsoft.UI.Colors.Transparent);
                }
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
