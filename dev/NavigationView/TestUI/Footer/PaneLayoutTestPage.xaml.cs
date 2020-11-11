// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using System.Collections;
using System.Collections.Generic;
using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class PaneLayoutTestPage : TestPage
    {
        IList<NavigationViewItem> menuItems = new System.Collections.ObjectModel.ObservableCollection<NavigationViewItem>();
        IList<NavigationViewItem> footerItems = new System.Collections.ObjectModel.ObservableCollection<NavigationViewItem>();

        public PaneLayoutTestPage()
        {
            this.InitializeComponent();


            for (int i = 0; i < 4; i++)
            {
                menuItems.Add(
                    new NavigationViewItem() {
                        Content = "Item #" + i.ToString()
                    }
                );
            }


            for (int i = 0; i < 4; i++)
            {
                footerItems.Add(
                    new NavigationViewItem() {
                        Content = "Footer #" + i.ToString()
                    }
                );
            }
        }


        private void TestCaseSelectionChanged(object sender, Windows.UI.Xaml.Controls.SelectionChangedEventArgs e)
        {
            if (sender is RadioButtons rb)
            {
                var caseTag = rb.SelectedIndex;

                switch (caseTag)
                {
                    case 0:
                        RootNavigationView.MenuItemsSource = null;
                        RootNavigationView.FooterMenuItemsSource = footerItems;
                        break;
                    case 1:
                        RootNavigationView.MenuItemsSource = menuItems;
                        RootNavigationView.FooterMenuItemsSource = null;
                        break;
                    case 2:
                        RootNavigationView.MenuItemsSource = menuItems;
                        RootNavigationView.FooterMenuItemsSource = footerItems;
                        break;
                }
            }
        }

        private void AddMenuItemButton_Click(object sender, RoutedEventArgs e)
        {
            menuItems.Add(new NavigationViewItem() { Content = "Text" });
        }
        private void AddFooterItemButton_Click(object sender, RoutedEventArgs e)
        {
            footerItems.Add(new NavigationViewItem() { Content = "Text" });
        }

    }
}
