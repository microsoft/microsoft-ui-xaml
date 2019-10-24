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
//using Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;
using Windows.UI.Xaml.Data;
using System.Numerics;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ProgressBar")]
    public sealed partial class ProgressBarPage : TestPage
    {
        public ProgressBarPage()
        {
            this.InitializeComponent();
        }

        public void UpdateMinMax_Click(object sender, RoutedEventArgs e)
        {
            if (!String.IsNullOrEmpty(MaximumInput.Text))
            {
                TestProgressBar.Maximum = Double.Parse(MaximumInput.Text);
            } else
            {
                TestProgressBar.Maximum = Double.Parse(MaximumInput.PlaceholderText);
            }

            if (!String.IsNullOrEmpty(MinimumInput.Text))
            {
                TestProgressBar.Minimum = Double.Parse(MinimumInput.Text);
            }
            else
            {
                TestProgressBar.Minimum = Double.Parse(MinimumInput.PlaceholderText);
            }
        }
        public void ChangeValue_Click(object sender, RoutedEventArgs e)
        {
            if (TestProgressBar.Value + 1 > TestProgressBar.Maximum)
            {
                TestProgressBar.Value = (int)(TestProgressBar.Minimum + 0.5);
            }
            else
            {
                TestProgressBar.Value += 1;
            }           
        }
    }

    public class NullableBooleanToBooleanConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is bool?)
            {
                return (bool)value;
            }
            return false;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            if (value is bool)
                return (bool)value;
            return false;
        }
    }
}
