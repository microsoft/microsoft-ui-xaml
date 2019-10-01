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

using ProgressBar = Microsoft.UI.Xaml.Controls.ProgressBar;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ProgressBar")]
    public sealed partial class ProgressBarPage : TestPage
    {
        public ProgressBarPage()
        {
            this.InitializeComponent();
        }

        private static int _clicks = 0;

        public void Button_Click(object sender, RoutedEventArgs e)
        {
            _clicks += 1;
            TestProgressBar.Value = _clicks;

            double minimum = TestProgressBar.Minimum;

            if (_clicks >= TestProgressBar.Maximum) _clicks = (int)(minimum + 0.5);
        }
    }
}
