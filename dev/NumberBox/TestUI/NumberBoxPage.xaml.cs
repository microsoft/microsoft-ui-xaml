// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls;


namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "NumberBox")]
    public sealed partial class NumberBoxPage : TestPage
    {
        public NumberBoxPage()
        {
            this.InitializeComponent();
        }

        private void HyperScroll_Checked(object sender, RoutedEventArgs e)
        {
            CheckBox check = (CheckBox) sender;
            if ( (bool) check.IsChecked)
            {
                TestNumberBox.HyperScrollEnabled = true;
            }
            else
            {
                TestNumberBox.HyperScrollEnabled = false;
            }
        }

        private void SpinMode_Changed(object sender, RoutedEventArgs e)
        {
            ComboBox spinmode = (ComboBox)sender;
            if (TestNumberBox == null)
            {
                return;
            }
            if ((string) spinmode.SelectedValue == "Inline")
            {
                TestNumberBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Inline;
            }
            else if ((string) spinmode.SelectedValue == "Hidden")
            {
                TestNumberBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Hidden;
            }
        }

        private void Wrap_CheckChanged(object sender, RoutedEventArgs e)
        {
            TestNumberBox.WrapEnabled = WrapCheckBox.IsEnabled;
        }
    }
}