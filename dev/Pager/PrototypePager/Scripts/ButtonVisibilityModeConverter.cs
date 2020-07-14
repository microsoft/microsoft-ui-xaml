using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Data;

namespace MUXControlsTestApp.Scripts
{
    class ButtonVisibilityModeConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            switch (value)
            {
                case PrototypePager.ButtonVisibilityMode.HiddenOnLast:
                case PrototypePager.ButtonVisibilityMode.None:
                    return Visibility.Collapsed;
                case PrototypePager.ButtonVisibilityMode.Auto:
                case PrototypePager.ButtonVisibilityMode.AlwaysVisible:
                default:
                    return Visibility.Visible;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
