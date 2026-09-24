// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace ConditionalControls
{
    public sealed partial class Button1 : UserControl, IVersionedProperties
    {
        private static Type ver = typeof(V1Type);

        public Button1()
        {
            Test.EnsureVersion(ver);
            this.InitializeComponent();
        }

        public string Text
        {
            get
            {
                Test.EnsureVersion(ver);
                return textBlock.Text;
            }
            set
            {
                Test.EnsureVersion(ver);
                textBlock.Text = value;
            }

        }

        public event RoutedEventHandler Click;

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            Test.EnsureVersion(ver);
            Click?.Invoke(this, e);
        }

        public string Caption
        {
            get
            {
                Test.EnsureVersion(ver);
                return textBlock.Text;
            }
            set
            {
                Test.EnsureVersion(ver);
                if (textBlock.Text != value)
                {
                    textBlock.Text = value;
                }
            }
        }

        private string _V1Property = "V1";
        public string V1Property
        {
            get 
            {
                Test.EnsureVersion(typeof(V1Type));
                return _V1Property;
            }
            set
            {
                Test.EnsureVersion(typeof(V1Type));
                if ( _V1Property != value)
                {
                    _V1Property = value;
                }
            }
        }

        private string _V2Property = "V2";
        public string V2Property
        {
            get
            {
                Test.EnsureVersion(typeof(V2Type));
                return _V2Property;
            }
            set
            {
                Test.EnsureVersion(typeof(V2Type));
                if (_V2Property != value)
                {
                    _V2Property = value;
                }
            }
        }

        private string _V3Property = "V3";
        public string V3Property
        {
            get
            {
                Test.EnsureVersion(typeof(V3Type));
                return _V3Property;
            }
            set
            {
                Test.EnsureVersion(typeof(V3Type));
                if (_V3Property != value)
                {
                    _V3Property = value;
                }
            }
        }
    }
}
