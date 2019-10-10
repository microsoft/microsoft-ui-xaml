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

        private void Hyperscroll_Checked(object sender, RoutedEventArgs e)
        {
            CheckBox check = (CheckBox) sender;
            if ( (bool) check.IsChecked)
            {
                numBox.HyperScrollEnabled = true;
            }
            else
            {
                numBox.HyperScrollEnabled = false;
            }
        }

        private void SpinMode_Changed(object sender, RoutedEventArgs e)
        {
            ComboBox spinmode = (ComboBox)sender;
            if (numBox == null)
            {
                return;
            }
            if ((string) spinmode.SelectedValue == "Inline")
            {
                numBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Inline;
            }
            else if ((string) spinmode.SelectedValue == "Hidden")
            {
                numBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Hidden;
            }
        }

        private void Bounds_Changed(object sender, RoutedEventArgs e)
        {
            if (numBox == null)
            {
                return;
            }
            ComboBox boundmode = (ComboBox)sender;
            numBox.MinMaxMode = (NumberBoxMinMaxMode) Enum.Parse(typeof(NumberBoxMinMaxMode), boundmode.SelectedValue as string);
        }
    }
}