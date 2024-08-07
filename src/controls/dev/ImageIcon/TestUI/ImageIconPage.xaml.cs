﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;

using ImageIcon = Microsoft.UI.Xaml.Controls.ImageIcon;
using Microsoft.UI.Xaml.Media.Imaging;

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
            this.ImageIcon.Source = new SvgImageSource(new Uri("ms-appx:///Assets/libre-camera-panorama.svg"));
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
