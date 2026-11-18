// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    public sealed partial class HierarchicalNavigationViewFooterMenuItems : Page
    {
        public HierarchicalNavigationViewFooterMenuItems()
        {
            this.InitializeComponent();
        }

        private void PrintSelectedItem(object sender, RoutedEventArgs e)
        {
            var selectedItem = navview.SelectedItem;
            SelectedItemLabel.Text = selectedItem != null
                ? (string)((NavigationViewItem)selectedItem).Content
                : "No Item Selected";
        }

        // Children hosted in the flyout are reparented out of the item itself, so the presence of a
        // FlyoutPresenter above them is what tells the flyout and inline layouts apart.
        private void PrintFooterItem2Parent(object sender, RoutedEventArgs e)
        {
            if (FI2.FindVisualParentByType<FlyoutPresenter>() != null)
            {
                FooterItem2ParentLabel.Text = "Flyout";
            }
            else if (FI2.FindVisualParentByType<NavigationViewItem>() != null)
            {
                FooterItem2ParentLabel.Text = "Inline";
            }
            else
            {
                FooterItem2ParentLabel.Text = "Not realized";
            }
        }

        private void PaneDisplayModeCombobox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var tag = Convert.ToString(((sender as ComboBox).SelectedItem as ComboBoxItem).Tag);
            navview.PaneDisplayMode = (NavigationViewPaneDisplayMode)Enum.Parse(typeof(NavigationViewPaneDisplayMode), tag);
        }
    }
}
