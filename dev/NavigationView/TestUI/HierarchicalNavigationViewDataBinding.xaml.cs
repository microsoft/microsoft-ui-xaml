// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

using NavigationViewItemInvokedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewItemInvokedEventArgs;
using NavigationViewItemExpandingEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewExpandingEventArgs;
using NavigationViewCollapsedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewCollapsedEventArgs;
using NavigationViewPaneDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewPaneDisplayMode;

namespace MUXControlsTestApp
{
    public class Category
    {
        public String Name { get; set; }
        public String Icon { get; set; }
        public ObservableCollection<Category> Children { get; set; }
        public bool IsLeaf { get; set; }

        public Category(String name, String icon, ObservableCollection<Category> children, bool isLeaf)
        {
            this.Name = name;
            this.Icon = icon;
            this.Children = children;
            this.IsLeaf = isLeaf;
        }
    }

    public sealed partial class HierarchicalNavigationViewDataBinding : Page
    {

        ObservableCollection<Category> categories = new ObservableCollection<Category>();

        public HierarchicalNavigationViewDataBinding()
        {
            this.InitializeComponent();

            var categories3 = new ObservableCollection<Category>();
            categories3.Add(new Category("Menu Item 3", "Icon", null, true));
            categories3.Add(new Category("Menu Item 4", "Icon", null, true));

            var categories2 = new ObservableCollection<Category>();
            categories2.Add(new Category("Menu Item 2", "Icon", categories3, false));

            
            var categories5 = new ObservableCollection<Category>();
            categories5.Add(new Category("Menu Item 7", "Icon", null, true));
            categories5.Add(new Category("Menu Item 8", "Icon", null, true));

            var categories4 = new ObservableCollection<Category>();
            categories4.Add(new Category("Menu Item 6", "Icon", categories5, false));

            categories.Add(new Category("Menu Item 1", "Icon", categories2, false));
            categories.Add(new Category("Menu Item 5", "Icon", categories4, true));
            categories.Add(new Category("Menu Item 9", "Icon", null, true));

        }

        private void ClickedItem(object sender, NavigationViewItemInvokedEventArgs e)
        {
            var clickedItem = e.InvokedItem;
            var clickedItemContainer = e.InvokedItemContainer;
        }

        private void PrintSelectedItem(object sender, RoutedEventArgs e)
        {
            var selectedItem = navview.SelectedItem;
            if(selectedItem != null)
            {
                var label = ((Category)selectedItem).Name;
                SelectedItemLabel.Text = label;
            }
            else
            {
                SelectedItemLabel.Text = "No Item Selected";
            }
        }

        private void AddMenuItem(object sender, RoutedEventArgs e)
        {
            categories.Add(new Category("Menu Item G", "Icon", null, true));
        }

        private void RemoveSecondMenuItem(object sender, RoutedEventArgs e)
        {
            categories.RemoveAt(1);
        }
        private void SelectSecondItem(object sender, RoutedEventArgs e)
        {
            navview.SelectedItem = categories[1];
        }        
        private void SelectItemUsingAPI(object sender, RoutedEventArgs e)
        {
            navview.SelectedItem = categories[0].Children[0].Children[1];
        }
        private void ExpandingItem(object sender, NavigationViewItemExpandingEventArgs e)
        {
            var nvib = e.ExpandingItemContainer;
            if(nvib != null)
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
