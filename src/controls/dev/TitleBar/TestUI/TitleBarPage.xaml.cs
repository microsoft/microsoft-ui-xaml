// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;

using Windows.UI.ViewManagement;
using Windows.Foundation;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Windowing;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "TitleBar", Icon = "DefaultIcon.png")]
    public sealed partial class TitleBarPage : TestPage
    {
        public TitleBarPage()
        {
            this.InitializeComponent();
        }

        private void TitleBarWindowingButton_Click(object sender, RoutedEventArgs e)
        {
            var newWindow = new TitleBarPageWindow();
            newWindow.Activate();
        }

    }
}
