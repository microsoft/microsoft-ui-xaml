// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    public class BindableItem
    {
        public string Content { get; set; }
    }
    
    public sealed partial class FooterItemsSelectionTestPage : TestPage
    {
        public FooterItemsSelectionTestPage()
        {
            this.InitializeComponent();

            this.Loaded += Page_Loaded;
        }
        private void Page_Loaded(object sender, RoutedEventArgs e)
        {
            var bindableItems = new ObservableCollection<BindableItem>();
            for (int i = 1; i < 5; i++)
            {
                bindableItems.Add(new BindableItem() { Content = "Header " + i.ToString() });
            }
            NavView.MenuItemsSource = bindableItems;
        }

        private void NavView_SelectionChanged(NavigationView sender, NavigationViewSelectionChangedEventArgs args)
        {
            if(args.SelectedItem is NavigationViewItem item)
            {
                SelectedItem.Text = item.Content.ToString();
            }else if(args.SelectedItem is BindableItem bindableItem)
            {
                SelectedItem.Text = bindableItem.Content;
            }
        }

    }
}
