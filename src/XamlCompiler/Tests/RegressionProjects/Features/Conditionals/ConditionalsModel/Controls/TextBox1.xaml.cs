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
    public sealed partial class TextBox1 : UserControl
    {
        public TextBox1()
        {
            Test.EnsureVersion<V1Type>();
            this.InitializeComponent();
        }


        //public static DependencyProperty TextProperty { get { return _TextProperty; } }
        //private static readonly DependencyProperty _TextProperty = DependencyProperty.Register("DPOnPage", typeof(string), typeof(TextBlock2), new PropertyMetadata(0));


        public string Text
        {
            set
            {
                Test.EnsureVersion<V1Type>();
                textBlock.Text = value;
            }
            get
            {
                Test.EnsureVersion<V1Type>();
                return textBlock.Text;
            }
        }
    }
}
