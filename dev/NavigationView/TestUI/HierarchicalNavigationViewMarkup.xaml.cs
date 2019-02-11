// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

#if !BUILD_WINDOWS
using NavigationViewItemInvokedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewItemInvokedEventArgs;
using NavigationViewItem = Microsoft.UI.Xaml.Controls.NavigationViewItem;
#endif

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

        private void PrintAllIsChildSelectedItems(object sender, RoutedEventArgs e)
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
                itemstring = BuildIsChildSelectedString(item.MenuItems, itemstring);
            }
            return itemstring;
        }
    }
}
