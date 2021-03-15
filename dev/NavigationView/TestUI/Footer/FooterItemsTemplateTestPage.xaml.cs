// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    public sealed partial class FooterItemsTemplateTestPage : TestPage
    {
        public FooterItemsTemplateTestPage()
        {
            this.InitializeComponent();

            this.Loaded += Page_Loaded;
        }
        private void Page_Loaded(object sender, RoutedEventArgs e)
        {
            var bindableItems = new ObservableCollection<BindableItem>();
            for (int i = 1; i < 5; i++)
            {
                bindableItems.Add(new BindableItem() { Content = "Header MenuItems" + i.ToString() });
            }
            var bindableFooterItems = new ObservableCollection<BindableItem>();
            for (int i = 1; i < 5; i++)
            {
                bindableFooterItems.Add(new BindableItem() { Content = "Header FooterItems" + i.ToString() });
            }
            NavView.MenuItemsSource = bindableItems;
            NavView.FooterMenuItemsSource = bindableFooterItems;
        }

        private void NavView_SelectionChanged(NavigationView sender, NavigationViewSelectionChangedEventArgs args)
        {
            if (args.SelectedItem is NavigationViewItem item)
            {
                SelectedItem.Text = item.Content.ToString();
            }
            else if (args.SelectedItem is BindableItem bindableItem)
            {
                SelectedItem.Text = bindableItem.Content;
            }
        }

    }
}
