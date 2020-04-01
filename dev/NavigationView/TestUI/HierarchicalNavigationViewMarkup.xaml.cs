// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

using NavigationViewItemInvokedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewItemInvokedEventArgs;
using NavigationViewItem = Microsoft.UI.Xaml.Controls.NavigationViewItem;
using NavigationViewItemExpandingEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewExpandingEventArgs;
using NavigationViewCollapsedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewCollapsedEventArgs;
using NavigationViewPaneDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewPaneDisplayMode;

namespace MUXControlsTestApp
{
    public sealed partial class HierarchicalNavigationViewMarkup : Page
    {

        public HierarchicalNavigationViewMarkup()
        {
            this.InitializeComponent();
        }

        private void ClickedItem(object sender, NavigationViewItemInvokedEventArgs e)
        {
        }

        private void PrintSelectedItem(object sender, RoutedEventArgs e)
        {
            var selectedItem = navview.SelectedItem;
            if (selectedItem != null)
            {
                var label = (String)((NavigationViewItem)selectedItem).Content;
                SelectedItemLabel.Text = label;
            }
            else
            {
                SelectedItemLabel.Text = "No Item Selected";
            }
        }

        private void CollapseSelectedItem(object sender, RoutedEventArgs e)
        {
            var selectedItem = navview.SelectedItem;
            if(selectedItem != null)
            {
                var container = (NavigationViewItem)navview.ContainerFromMenuItem(selectedItem);
                container.IsExpanded = false;
            }
        }

        private void RemoveSecondMenuItem(object sender, RoutedEventArgs e)
        {
            navview.MenuItems.RemoveAt(2);
        }

        private void PrintTopLevelIsChildSelectedItems(object sender, RoutedEventArgs e)
        {
            string itemstring = "";
            itemstring = BuildIsChildSelectedString(navview.MenuItems, itemstring);
            if(itemstring == "")
            {
                itemstring = "None";
            }
            IsChildSelectedLabel.Text = itemstring;
        }

        private string BuildIsChildSelectedString(IList<object> items, string itemstring)
        {
            foreach (NavigationViewItem item in items)
            {
                if (item.IsChildSelected == true)
                {
                    itemstring += item.Name + " ";
                }
            }
            return itemstring;
        }

        private void SelectSecondItem(object sender, RoutedEventArgs e)
        {
            IList<Object> menuItems = navview.MenuItems;
            navview.SelectedItem = menuItems[1];

        }

        private void ExpandingItem(object sender, NavigationViewItemExpandingEventArgs e)
        {
            var nvib = e.ExpandingItemContainer;
            if (nvib != null)
            {
                var name = "Last Expanding: " + nvib.Content;
                ExpandingItemLabel.Text = name;
            }
            else
            {
                ExpandingItemLabel.Text = "Last Expanding: ERROR - No container returned!";
            }
        }

        private void CollapsedItem(object sender, NavigationViewCollapsedEventArgs e)
        {
            var nvib = e.CollapsedItemContainer;
            if (nvib != null)
            {
                var name = "Last Collapsed: " + nvib.Content;
                CollapsedItemLabel.Text = name;
            }
            else
            {
                CollapsedItemLabel.Text = "Last Collapsed: ERROR - No container returned!";
            }
        }

        private void PaneDisplayModeCombobox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var tag = Convert.ToString(((sender as ComboBox).SelectedItem as ComboBoxItem).Tag);
            var mode = (NavigationViewPaneDisplayMode)Enum.Parse(typeof(NavigationViewPaneDisplayMode), tag);
            navview.PaneDisplayMode = mode;
        }
    }
}
