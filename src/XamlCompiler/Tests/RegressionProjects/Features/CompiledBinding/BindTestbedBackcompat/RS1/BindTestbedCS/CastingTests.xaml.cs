// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;

namespace BindTestbed
{
    internal sealed class CastingTestsVM : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        private string _username = string.Empty;
        public string Username
        {
            get { return _username; }
            set
            {
                _username = value;
                RaisePropertyChanged();
            }
        }

        private bool _isVisible = false;
        public bool IsVisible
        {
            get { return _isVisible; }
            set
            {
                _isVisible = value;
                RaisePropertyChanged();
            }
        }

        public bool? IsVisibleNullable
        {
            get { return _isVisible; }
            set
            {
                _isVisible = value ?? false;
                RaisePropertyChanged();
            }
        }

        private Visibility _visibilityValue = Visibility.Collapsed;
        public Visibility VisibilityValue
        {
            get { return _visibilityValue; }
            set
            {
                _visibilityValue = value;
                RaisePropertyChanged();
            }
        }

        private bool _isChecked = true;
        public bool IsChecked
        {
            get { return _isChecked; }
            set
            {
                _isChecked = value;
                RaisePropertyChanged();
            }
        }

        private double _doubleVal = 15.0;
        public double DoubleVal
        {
            get { return _doubleVal; }
            set
            {
                _doubleVal = value;
                RaisePropertyChanged();
            }
        }

        private int _intVal = 20;
        public int IntVal
        {
            get { return _intVal; }
            set
            {
                _intVal = value;
                RaisePropertyChanged();
            }
        }

        public string Prefix { get { return "Converting double to int."; } }
        public double Postfix { get { return 15.4; } }
        public string CombineStringWithInt(string str, int number)
        {
            return string.Format("{0} {1}", str, number.ToString());
        }

        private void RaisePropertyChanged([CallerMemberName]string propertyName = "")
        {
            if (null != PropertyChanged)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }
    }

    internal sealed partial class CastingTests
    {
        public CastingTests()
        {
            InitializeComponent();
        }

        public static explicit operator Thickness(CastingTests inst)
        {
            return new Thickness(5.0);
        }
    }
}