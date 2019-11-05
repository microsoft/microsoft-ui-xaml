// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls;
using Windows.Globalization.NumberFormatting;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "NumberBox")]
    public sealed partial class NumberBoxPage : TestPage
    {
        public NumberBoxPage()
        {
            this.InitializeComponent();
        }

        private void SpinMode_Changed(object sender, RoutedEventArgs e)
        {
            if (TestNumberBox != null)
            {
                if (SpinModeComboBox.SelectedIndex == 0)
                {
                    TestNumberBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Hidden;
                }
                else if (SpinModeComboBox.SelectedIndex == 1)
                {
                    TestNumberBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Inline;
                }
            }
        }


        private void Validation_Changed(object sender, RoutedEventArgs e)
        {
            if (TestNumberBox != null)
            {
                if (ValidationComboBox.SelectedIndex == 0)
                {
                    TestNumberBox.BasicValidationMode = NumberBoxBasicValidationMode.InvalidInputOverwritten;
                }
                else if (ValidationComboBox.SelectedIndex == 1)
                {
                    TestNumberBox.BasicValidationMode = NumberBoxBasicValidationMode.Disabled;
                }
            }
        }

        private void MinCheckBox_CheckChanged(object sender, RoutedEventArgs e)
        {
            MinNumberBox.IsEnabled = (bool)MinCheckBox.IsChecked;
            MinValueChanged(null, null);
        }

        private void MaxCheckBox_CheckChanged(object sender, RoutedEventArgs e)
        {
            MaxNumberBox.IsEnabled = (bool)MaxCheckBox.IsChecked;
            MaxValueChanged(null, null);
        }

        private void MinValueChanged(object sender, object e)
        {
            if (TestNumberBox != null)
            {
                TestNumberBox.Minimum = (bool)MinCheckBox.IsChecked ? MinNumberBox.Value : double.MinValue;
            }
        }

        private void MaxValueChanged(object sender, object e)
        {
            if (TestNumberBox != null)
            {
                TestNumberBox.Maximum = (bool)MaxCheckBox.IsChecked ? MaxNumberBox.Value : double.MaxValue;
            }
        }

        private void NumberBoxValueChanged(object sender, NumberBoxValueChangedEventArgs e)
        {
            if (TestNumberBox != null)
            {
                CurrentValueTextBox.Text = e.NewValue.ToString();
                OldValueTextBox.Text = e.OldValue.ToString();
            }
        }
        private void CustomFormatterButton_Click(object sender, RoutedEventArgs e)
        {
            DecimalFormatter formatter = new DecimalFormatter();
            formatter.IntegerDigits = 1;
            formatter.FractionDigits = 2;
            TestNumberBox.NumberFormatter = formatter;
        }
    }
}