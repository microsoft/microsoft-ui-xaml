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

using ProgressRing = Microsoft.UI.Xaml.Controls.ProgressRing;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ProgressRing")]
    public sealed partial class ProgressRingPage : TestPage
    {
        public ProgressRingPage()
        {
            this.InitializeComponent();
        }

        public void ChangeValue_Click(object sender, RoutedEventArgs e)
        {
            if (TestProgressRing.Value + 1 > TestProgressRing.Maximum)
            {
                TestProgressRing.Value = (int)(TestProgressRing.Minimum + 0.5);
            }
            else
            {
                TestProgressRing.Value += 1;
            }
        }
    }
}
