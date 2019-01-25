// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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

        private void ClickedItem(object sender, mux.NavigationViewItemInvokedEventArgs e)
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
        }

        private void AddMenuItem(object sender, RoutedEventArgs e)
        {
            categories.Add(new Category("Menu Item G", "Icon", null, true));
        }

        private void RemoveSecondMenuItem(object sender, RoutedEventArgs e)
        {
            categories.RemoveAt(1);
        }
    }
}
