// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Data;

namespace BindTestbedModel
{
    public sealed class BoolToVisibilityConverter : IValueConverter 
    {
        object IValueConverter.Convert(object value, Type targetType, object parameter, string language)
        {
            if (!value.GetType().Equals(typeof(bool)))
            {
                throw new ArgumentException("Only Boolean is supported");
            }
            if (targetType.Equals(typeof(Microsoft.UI.Xaml.Visibility)))
            {
                return ((bool)value) ? Microsoft.UI.Xaml.Visibility.Visible : Microsoft.UI.Xaml.Visibility.Collapsed;
            }
            else
            {
                throw new ArgumentException("Unsuported type {0}", targetType.FullName);
            }
        }

        object IValueConverter.ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
