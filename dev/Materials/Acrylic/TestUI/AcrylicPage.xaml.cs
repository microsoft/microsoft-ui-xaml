// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;


namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "Acrylic", Icon = "AcrylicBrush.png")]
    public sealed partial class AcrylicPage : TestPage
    {
        public AcrylicPage()
        {
            this.InitializeComponent();

            navigateToBasicAcrylic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(AcrylicBrushPage), 0); };
            navigateToColorAcrylic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(AcrylicColorPage), 0); };
            navigateToMarkupAcrylic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(AcrylicMarkupPage), 0); };
            navigateToRenderingAcrylic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(AcrylicRenderingPage), 0); };
            navigateToLuminosityAcrylicTest.Click += delegate { Frame.NavigateWithoutAnimation(typeof(AcrylicBrushLuminosityTestPage), 0); };
        }
    }
}
