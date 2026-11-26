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
        public static int StaticBindBackCalls { get; private set; }
        public static string StaticBindBackValue { get; private set; } = "";

        public static void ResetStaticBindBack()
        {
            StaticBindBackCalls = 0;
            StaticBindBackValue = "";
        }

        public static void SetFormattedSettingStatic(object value)
        {
            StaticBindBackCalls++;
            StaticBindBackValue = (string)value;
        }

        public static void SetFormattedTextStatic(string value)
        {
            SetFormattedSettingStatic(value);
        }

        public string FormatSetting(bool value)
        {
            return value ? "setting: true" : "setting: false";
        }

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
