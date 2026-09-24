// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI;
using Microsoft.UI.Xaml.Media;

namespace BindTestbedModel
{
    public sealed class NestedLoadModel : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        private Windows.UI.Color _Option1Background = Colors.Red;
        public Windows.UI.Color Option1Background
        {
            get
            {
                return _Option1Background;
            }
            set
            {
                if (value != _Option1Background)
                {
                    _Option1Background = value;
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Option1Background"));
                }
            }
        }

        private Windows.UI.Color _Option2Background = Colors.Red;
        public Windows.UI.Color Option2Background
        {
            get
            {
                return _Option2Background;
            }
            set
            {
                if (value != _Option2Background)
                {
                    _Option2Background = value;
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Option2Background"));
                }
            }
        }

        private Windows.UI.Color _Option3Background = Colors.Blue;
        public Windows.UI.Color Option3Background
        {
            get
            {
                return _Option3Background;
            }
            set
            {
                if (value != _Option3Background)
                {
                    _Option3Background = value;
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Option3Background"));
                }
            }
        }

        public void Option1Click()
        {
            if (Option1Background == Colors.Red)
            {
                Option1Background = Colors.Green;
            }
            else
            {
                Option1Background = Colors.Red;
            }
        }

        public void Option2Click()
        {
            if (Option2Background == Colors.Red)
            {
                Option2Background = Colors.Green;
            }
            else
            {
                Option2Background = Colors.Red;
            }
        }

        public void Option3Click()
        {
            if (Option3Background == Colors.Blue)
            {
                Option3Background = Colors.Green;
            }
            else
            {
                Option3Background = Colors.Blue;
            }
        }

        public static Brush ColorToBrush(Windows.UI.Color color)
        {
            return new SolidColorBrush(color);
        }

        public static bool IsGreen(Windows.UI.Color color)
        {
            return color == Colors.Green;
        }
    }
}
