// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

#if !BUILD_WINDOWS
using ColorPicker = Microsoft.UI.Xaml.Controls.ColorPicker;
using ColorChangedEventArgs = Microsoft.UI.Xaml.Controls.ColorChangedEventArgs;
#endif

namespace MUXControlsTestApp
{
    public class ColorItem
    {
        private string title;
        private SolidColorBrush colorBrush = new SolidColorBrush(Colors.Red);

        public string Title { get { return title; } set { title = value; } }
        public SolidColorBrush ColorBrush { get { return colorBrush; } set { colorBrush = value; } }
    }

    public sealed partial class RevealScenarioColors : TestPage
    {
        public RevealScenarioColors()
        {
            this.InitializeComponent();
        }

        private Color darkHoverPlateColor;
        private Color darkBorderColor;
        private Color lightHoverPlateColor;
        private Color lightBorderColor;

        public Color DarkHoverPlateColor { get { return darkHoverPlateColor; } set { darkHoverPlateColor = value; } }
        public Color DarkBorderColor { get { return darkBorderColor; } set { darkBorderColor = value; } }
        public Color LightHoverPlateColor { get { return lightHoverPlateColor; } set { lightHoverPlateColor = value; } }
        public Color LightBorderColor { get { return lightBorderColor; } set { lightBorderColor = value; } }

        private void ColorList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var colorItem = ColorList.SelectedItem as ColorItem;
            if (colorItem != null)
            {
                ColorPicker.Color = colorItem.ColorBrush.Color;
                ColorPicker.Visibility = Visibility.Visible;
            }
            else
            {
                ColorPicker.Visibility = Visibility.Collapsed;
            }
        }

        private void ColorPicker_ColorChanged(ColorPicker sender, ColorChangedEventArgs args)
        {
            var colorItem = ColorList.SelectedItem as ColorItem;
            if (colorItem != null)
            {
                colorItem.ColorBrush.Color = args.NewColor;
            }
        }
    }
}
