// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace ConditionalControls
{
    public sealed partial class Button1 : UserControl, IVersionedProperties
    {
        public Button1()
        {
            Test.EnsureVersion<V1Type>();
            this.InitializeComponent();
        }

        public string Text
        {
            get
            {
                Test.EnsureVersion<V1Type>();
                return textBlock.Text;
            }
            set
            {
                Test.EnsureVersion<V1Type>();
                textBlock.Text = value;
            }

        }

        public event RoutedEventHandler Click;

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            Test.EnsureVersion<V1Type>();
            Click?.Invoke(this, e);
        }

        public string Caption
        {
            get
            {
                Test.EnsureVersion<V1Type>();
                return textBlock.Text;
            }
            set
            {
                Test.EnsureVersion<V1Type>();
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
                Test.EnsureVersion<V1Type>();
                return _V1Property;
            }
            set
            {
                Test.EnsureVersion<V1Type>();
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
                Test.EnsureVersion<V2Type>();
                return _V2Property;
            }
            set
            {
                Test.EnsureVersion<V2Type>();
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
                Test.EnsureVersion<V3Type>();
                return _V3Property;
            }
            set
            {
                Test.EnsureVersion<V3Type>();
                if (_V3Property != value)
                {
                    _V3Property = value;
                }
            }
        }
    }
}
