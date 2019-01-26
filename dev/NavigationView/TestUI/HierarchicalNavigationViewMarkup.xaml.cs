// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using mux = Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class HierarchicalNavigationViewMarkup : Page
    {

        public HierarchicalNavigationViewMarkup()
        {
            this.InitializeComponent();
        }

        private void ClickedItem(object sender, mux.NavigationViewItemInvokedEventArgs e)
        {
        }

        private void PrintSelectedItem(object sender, RoutedEventArgs e)
        {
            var selectedItem = navview.SelectedItem;
            if (selectedItem != null)
            {
                var label = (String)((mux.NavigationViewItem)selectedItem).Content;
                SelectedItemLabel.Text = label;
            }
        }

        private void CollapseSelectedItem(object sender, RoutedEventArgs e)
        {
            var selectedItem = navview.SelectedItem;
            if(selectedItem != null)
            {
                var container = (mux.NavigationViewItem)navview.ContainerFromMenuItem(selectedItem);
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
            foreach (mux.NavigationViewItem item in items)
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
