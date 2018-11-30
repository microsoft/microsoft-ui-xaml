// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;

namespace MUXControlsTestApp.Samples.Model
{
    public class Recipe : INotifyPropertyChanged
    {
        private double _resizeHeight = double.NaN;

        public Uri ImageUri { get; set; }
        public string Description { get; set; }
        public double ResizeHeight
        {
            get
            {
                return _resizeHeight;
            }

            set
            {
                _resizeHeight = value;
                OnPropertyChanged(new System.ComponentModel.PropertyChangedEventArgs("ResizeHeight"));
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyChanged(PropertyChangedEventArgs e)
        {
            if (PropertyChanged != null) { PropertyChanged(this, e); }
        }

        public override string ToString()
        {
            return Description;
        }
    }
}
