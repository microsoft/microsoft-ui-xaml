﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
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


            for (int i = 0; i < 3; i++)
            {
                menuItems.Add(
                    new NavigationViewItem() {
                        Content = "Item #" + i.ToString()
                    }
                );
            }


            for (int i = 0; i < 3; i++)
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

        private void ResetCollectionsButton_Click(object sender, RoutedEventArgs e)
        {
            for (int i = menuItems.Count - 1; i > 2; i--)
            {
                menuItems.RemoveAt(i);
            }

            for (int i = footerItems.Count - 1; i > 2; i--)
            {
                footerItems.RemoveAt(i);
            }
        }

        
        private void ClearCollectionsButton_Click(object sender, RoutedEventArgs e)
        {
            menuItems.Clear();
            footerItems.Clear();
        }

        private void GetLayoutHeightsButton_Click(object sender, RoutedEventArgs e)
        {
            var itemsScroll = VisualTreeUtils.FindVisualChildByName(RootNavigationView, "MenuItemsScrollViewer");
            var footerScroll = VisualTreeUtils.FindVisualChildByName(RootNavigationView, "FooterItemsScrollViewer");
            LayoutHeightsReport.Text = itemsScroll.ActualHeight + ";" + footerScroll.ActualHeight;
        }

        private void IsSettingsEnabledCheckbox_Checked(object sender, RoutedEventArgs e)
        {
            RootNavigationView.IsSettingsVisible = true;
        }

        private void IsSettingsEnabledCheckbox_UnChecked(object sender, RoutedEventArgs e)
        {
            RootNavigationView.IsSettingsVisible = false;
        }
    }
}
