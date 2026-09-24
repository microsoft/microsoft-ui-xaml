// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BindTestbedModel
{
    public sealed class FakeSettings : INotifyPropertyChanged
    {
        public static FakeSettings Instance { get; } = new FakeSettings();
        private bool boolSetting;
        public bool BoolSetting
        {
            get { return boolSetting; }
            set
            {
                if (boolSetting != value)
                {
                    boolSetting = value;
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("BoolSetting"));
                }
            }
        }
        public event PropertyChangedEventHandler PropertyChanged;
    }
}
