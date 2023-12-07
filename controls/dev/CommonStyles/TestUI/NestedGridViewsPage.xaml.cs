// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;

namespace MUXControlsTestApp
{
    public sealed partial class NestedGridViewsPage : TestPage
    {
        public NestedGridViewsPage()
        {
            this.InitializeComponent();
        }

        private void BtnSwitchOuterListViewVisibility_Click(object sender, RoutedEventArgs e)
        {
            if (outerListView != null)
            {
                if (outerListView.Visibility == Visibility.Visible)
                {
                    outerListView.Visibility = Visibility.Collapsed;
                }
                else
                {
                    outerListView.Visibility = Visibility.Visible;
                }
            }
        }

        private void ChkOuterListViewSingleSelectionFollowsFocus_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (outerListView != null && chkOuterListViewSingleSelectionFollowsFocus != null)
            {
                outerListView.SingleSelectionFollowsFocus = (bool)chkOuterListViewSingleSelectionFollowsFocus.IsChecked;
            }
        }

        private void CmbOuterListViewSelectionMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (outerListView != null && cmbOuterListViewSelectionMode != null)
            {
                outerListView.SelectionMode = (ListViewSelectionMode)cmbOuterListViewSelectionMode.SelectedIndex;
            }
        }

        private void CmbOuterListViewTabNavigation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (outerListView != null && cmbOuterListViewTabNavigation != null)
            {
                outerListView.TabNavigation = (KeyboardNavigationMode)cmbOuterListViewTabNavigation.SelectedIndex;
            }
        }

        private void BtnGetItemsStackPanelCacheLength_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (outerListView != null && txtItemsStackPanelCacheLength != null)
                {
                    ItemsStackPanel itemsStackPanel = outerListView.ItemsPanelRoot as ItemsStackPanel;

                    if (itemsStackPanel != null)
                    {
                        txtItemsStackPanelCacheLength.Text = itemsStackPanel.CacheLength.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtItemsStackPanelCacheLength.Text = ex.ToString();
            }
        }

        private void BtnSetItemsStackPanelCacheLength_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (outerListView != null && txtItemsStackPanelCacheLength != null)
                {
                    ItemsStackPanel itemsStackPanel = outerListView.ItemsPanelRoot as ItemsStackPanel;

                    if (itemsStackPanel != null)
                    {
                        itemsStackPanel.CacheLength = double.Parse(txtItemsStackPanelCacheLength.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtItemsStackPanelCacheLength.Text = ex.ToString();
            }
        }
    }
}
