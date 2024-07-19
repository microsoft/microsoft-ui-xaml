// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
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

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "PTR", Icon = "PullToRefresh.png")]
    public sealed partial class PTRPage : TestPage
    {
        public PTRPage()
        {
            this.InitializeComponent();

            RefreshContainerButton.Click += delegate { Frame.Navigate(typeof(RefreshContainerPage), 0); };
            RefreshContainerOnImageButton.Click += delegate { Frame.Navigate(typeof(RefreshContainerOnImagePage), 0); };
            RefreshVisualizerButton.Click += delegate { Frame.Navigate(typeof(RefreshVisualizerPage), 0); };
        }
    }
}
