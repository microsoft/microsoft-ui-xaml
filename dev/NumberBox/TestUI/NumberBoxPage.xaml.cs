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
using Windows.UI.Notifications;
using Microsoft.UI.Xaml.Controls;

#if !BUILD_WINDOWS
using NumberBox = Microsoft.UI.Xaml.Controls.NumberBox;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class NumberBoxPage : TestPage
    {
        NumberBox box;

        public NumberBoxPage()
        {
            this.InitializeComponent();

        }

        private void Hyperscroll_Checked(object sender, RoutedEventArgs e)
        {

            box = FindVisualChildByName(this, "numBox") as NumberBox;
            CheckBox check = (CheckBox) sender;
            if ( (bool) check.IsChecked)
            {
                box.HyperScrollEnabled = true;
            }
            else
            {
                box.HyperScrollEnabled = false;
            }
        }

        private void SpinMode_Changed(object sender, RoutedEventArgs e)
        {
            box = FindVisualChildByName(this, "numBox") as NumberBox;
            ComboBox spinmode = (ComboBox)sender;
            if (box == null)
            {
                return;
            }
            if ((string) spinmode.SelectedValue == "Inline")
            {
                box.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Inline;
            }
            else if ((string) spinmode.SelectedValue == "Hidden")
            {
                box.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Hidden;
            }



        }


    }
}
