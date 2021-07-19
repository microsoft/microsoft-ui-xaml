﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Shapes;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ProgressBar")]
    public sealed partial class ProgressBarPage : TestPage
    {
        public ProgressBarPage()
        {
            this.InitializeComponent();
            Loaded += ProgressBarPage_Loaded;

            NavigateToReTemplatePage.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ProgressBarReTemplatePage), 0); };
        }

        private void ProgressBarPage_Loaded(object sender, RoutedEventArgs e)
        {
            var layoutRoot = (Grid)VisualTreeHelper.GetChild(TestProgressBar, 0);

            VisualStateManager.GetVisualStateGroups(layoutRoot)[0].CurrentStateChanged += this.ProgressBarPage_CurrentStateChanged;
            VisualStateText.Text = VisualStateManager.GetVisualStateGroups(layoutRoot)[0].CurrentState.Name;

            var progressBarRoot = VisualTreeHelper.GetChild(layoutRoot, 0);
            var clip = VisualTreeHelper.GetChild(progressBarRoot, 0);
            var grid = VisualTreeHelper.GetChild(clip, 0);
            Rectangle indicator = null;
            int child = 0;

            do
            {
                indicator = VisualTreeHelper.GetChild(grid, child) as Rectangle;
                child++;
            }
            while ((indicator == null || indicator.Name != "DeterminateProgressBarIndicator") && child < VisualTreeHelper.GetChildrenCount(grid));

            if (indicator != null)
            {
                indicator.SizeChanged += this.Indicator_SizeChanged;
            }

            IndicatorWidthText.Text = indicator.ActualWidth.ToString();
            ROValueText.Text = TestProgressBar.Value.ToString();

            Loaded -= ProgressBarPage_Loaded;
        }

        private void Indicator_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            IndicatorWidthText.Text = ((Rectangle)sender).ActualWidth.ToString();
        }

        private void ProgressBarPage_CurrentStateChanged(object sender, VisualStateChangedEventArgs e)
        {
            VisualStateText.Text = e.NewState.Name;
        }

        public void UpdateMinMax_Click(object sender, RoutedEventArgs e)
        {
            TestProgressBar.Maximum = String.IsNullOrEmpty(MaximumInput.Text) ? Double.Parse(MaximumInput.PlaceholderText) : Double.Parse(MaximumInput.Text);
            TestWUXProgressBar.Maximum = String.IsNullOrEmpty(MaximumInput.Text) ? Double.Parse(MaximumInput.PlaceholderText) : Double.Parse(MaximumInput.Text);
            TestProgressBar.Minimum = String.IsNullOrEmpty(MinimumInput.Text) ? Double.Parse(MinimumInput.PlaceholderText) : Double.Parse(MinimumInput.Text);
            TestWUXProgressBar.Minimum = String.IsNullOrEmpty(MinimumInput.Text) ? Double.Parse(MinimumInput.PlaceholderText) : Double.Parse(MinimumInput.Text);
            ROValueText.Text = TestProgressBar.Value.ToString();
        }

        public void UpdateWidth_Click(object sender, RoutedEventArgs e)
        {
            TestProgressBar.Width = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
            TestWUXProgressBar.Width = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
        }

        public void UpdateValue_Click(object sender, RoutedEventArgs e)
        {
            TestProgressBar.Value = String.IsNullOrEmpty(ValueInput.Text) ? Double.Parse(ValueInput.PlaceholderText) : Double.Parse(ValueInput.Text);
            TestWUXProgressBar.Value = String.IsNullOrEmpty(ValueInput.Text) ? Double.Parse(ValueInput.PlaceholderText) : Double.Parse(ValueInput.Text);
            ROValueText.Text = TestProgressBar.Value.ToString();
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
            ROValueText.Text = TestProgressBar.Value.ToString();

            if (TestWUXProgressBar.Value + 1 > TestWUXProgressBar.Maximum)
            {
                TestWUXProgressBar.Value = (int)(TestWUXProgressBar.Minimum + 0.5);
            }
            else
            {
                TestWUXProgressBar.Value += 1;
            }
        }

        public void UpdatePadding_Click(object sender, RoutedEventArgs e)
        {
            double paddingLeft = String.IsNullOrEmpty(PaddingLeftInput.Text) ? Double.Parse(PaddingLeftInput.PlaceholderText) : Double.Parse(PaddingLeftInput.Text);
            double paddingRight = String.IsNullOrEmpty(PaddingRightInput.Text) ? Double.Parse(PaddingRightInput.PlaceholderText) : Double.Parse(PaddingRightInput.Text);

            TestProgressBar.Padding = new Thickness(paddingLeft, 0, paddingRight, 0);
            ROValueText.Text = TestProgressBar.Value.ToString();

            TestWUXProgressBar.Padding = new Thickness(paddingLeft, 0, paddingRight, 0);
        }
    }

    public class NullableBooleanToBooleanConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            return value is bool ? (bool)value : (object)false;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            return value is bool ? (bool)value : (object)false;
        }
    }
}
