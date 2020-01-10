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

        public void UpdateMinMax_Click(object sender, RoutedEventArgs e)
        {
            TestProgressRing.Maximum = String.IsNullOrEmpty(MaximumInput.Text) ? Double.Parse(MaximumInput.PlaceholderText) : Double.Parse(MaximumInput.Text);
            TestProgressRing.Minimum = String.IsNullOrEmpty(MinimumInput.Text) ? Double.Parse(MinimumInput.PlaceholderText) : Double.Parse(MinimumInput.Text);
        }

        public void UpdateWidth_Click(object sender, RoutedEventArgs e)
        {
            TestProgressRing.Width = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
            TestProgressRing.Height = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
        }

        public void UpdateStroke_Click(object sender, RoutedEventArgs e)
        {
            TestProgressRing.StrokeThickness = String.IsNullOrEmpty(StrokeInput.Text) ? Double.Parse(StrokeInput.PlaceholderText) : Double.Parse(StrokeInput.Text);
        }

        public void UpdateValue_Click(object sender, RoutedEventArgs e)
        {
            TestProgressRing.Value = String.IsNullOrEmpty(ValueInput.Text) ? Double.Parse(ValueInput.PlaceholderText) : Double.Parse(ValueInput.Text);
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
