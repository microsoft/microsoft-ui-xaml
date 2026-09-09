// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.UI.Xaml;
using System;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace ReflectionProviderTestCS
{
    /// <summary>
    /// An empty window that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
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
