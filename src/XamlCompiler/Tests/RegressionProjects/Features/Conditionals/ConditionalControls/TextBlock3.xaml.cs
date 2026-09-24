// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

namespace ConditionalControls
{
    public sealed partial class TextBlock3 : UserControl
    {
        private static Type ver = typeof(V3Type);

        public TextBlock3()
        {
            Test.EnsureVersion(ver);
            this.InitializeComponent();
        }

        public string Text
        {
            set
            {
                Test.EnsureVersion(ver);
                textBlock.Text = value;
            }
            get
            {
                Test.EnsureVersion(ver);
                return textBlock.Text;
            }
        }
    }
}
