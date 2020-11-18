// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;

using ImageIcon = Microsoft.UI.Xaml.Controls.ImageIcon;
using Windows.UI.Xaml.Media.Imaging;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ImageIcon")]
    public sealed partial class ImageIconPage : TestPage
    {
        public ImageIconPage()
        {
            this.InitializeComponent();
        }

        private void ToggleButton_Checked(object sender, RoutedEventArgs e)
        {
            this.ImageIcon.Source = new SvgImageSource(new Uri("ms-appx:///Assets/Nuclear_symbol.svg"));
        }
        private void ToggleButton_Unchecked(object sender, RoutedEventArgs e)
        {
            BitmapImage bitmapImage = new BitmapImage();
            Uri uri = new Uri("ms-appx:///Assets/ingredient2.png");
            bitmapImage.UriSource = uri;

            this.ImageIcon.Source = bitmapImage;
        }
    }
}
