// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;

namespace WinUICsDesktopClassLibrary
{
    public sealed partial class MuxcIxmpTestUserControl : UserControl
    {
        public MuxcIxmpTestUserControl()
        {
            this.InitializeComponent();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            muxcIxmpTextBlock.Text = $"ColorPicker.Color={colorPicker.Color}, ColorPicker.PreviousColor={colorPicker.PreviousColor}, AcrylicBrush.TintLuminosityOpacity={acrylicBrush.TintLuminosityOpacity}";
        }
    }
}
