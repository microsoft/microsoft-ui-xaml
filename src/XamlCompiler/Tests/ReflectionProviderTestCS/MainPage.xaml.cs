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

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace ReflectionProviderTestCS
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            if (EnumTextBox.SByteP != SByte.NegativeOne)
            {
                throw new Exception("SByte invalid");
            }

            if (EnumTextBox.ByteP != Byte.One)
            {
                throw new Exception("Byte invalid");
            }

            if (EnumTextBox.ShortP != Short.One)
            {
                throw new Exception("Short invalid");
            }

            if (EnumTextBox.UShortP != UShort.One)
            {
                throw new Exception("UShort invalid");
            }

            if (EnumTextBox.IntP != Int.One)
            {
                throw new Exception("Int invalid");
            }

            if (EnumTextBox.UIntP != UInt.One)
            {
                throw new Exception("UInt invalid");
            }

            if (EnumTextBox.LongP != Long.NegativeOne)
            {
                throw new Exception("Long invalid");
            }

            // ULongP is set to "One,Two" in markup which should correspond to ULong.Three if the provider bitwise-ors the enum values correctly
            if (EnumTextBox.ULongP != ULong.Three)
            {
                throw new Exception("ULong invalid");
            }
        }
    }
}
