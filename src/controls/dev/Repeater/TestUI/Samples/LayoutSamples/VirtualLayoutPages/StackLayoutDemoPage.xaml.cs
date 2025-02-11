// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class StackLayoutDemoPage : Page
    {
        private List<ItemsRepeater> ItemsRepeaters
        { 
            get; set;
        }

        private List<string> ItemsSource
        {
            get; set;
        }

        public StackLayoutDemoPage()
        {
            this.InitializeComponent();

            Loaded += StackLayoutDemoPage_Loaded;
        }

        private void StackLayoutDemoPage_Loaded(object sender, RoutedEventArgs e)
        {
            if (itemsView1 != null && itemsView1.ScrollView != null)
            {
                itemsView1.ScrollView.ContentOrientation = ScrollingContentOrientation.Both;
            }

            ItemsRepeaters = new List<ItemsRepeater>();
            ItemsRepeaters.Add(itemsRepeater1);
            ItemsRepeaters.Add(itemsRepeater2);
            ItemsRepeaters.Add(itemsRepeater3);
            ItemsRepeaters.Add(itemsView1.ScrollView.Content as ItemsRepeater);

            UpdateItemsRepeatersHorizontalAlignment();
            UpdateItemsRepeatersVerticalCacheLength();
            UpdateItemsRepeatersItemsSourceCount();

            SetItemsRepeatersItemsSourceCount();
            SetItemsRepeatersItemsSource();
            SetItemsRepeatersVerticalCacheLength();
            SetItemsRepeatersHorizontalAlignment();
        }

        private void SetItemsRepeatersItemsSourceCount()
        {
            if (txtItemsRepeatersItemsSourceCount != null)
            {
                ItemsSource = new List<string>();
                string item = string.Empty;
                int itemsRepeatersItemsSourceCount = int.Parse(txtItemsRepeatersItemsSourceCount.Text);

                for (int i = 0; i < itemsRepeatersItemsSourceCount; i++)
                {
                    item += "A";
                    ItemsSource.Add(item);
                }
            }
        }

        private void SetItemsRepeatersItemsSource()
        {
            foreach (ItemsRepeater itemsRepeater in ItemsRepeaters)
            {
                if (itemsRepeater != null)
                {
                    if (itemsRepeater == itemsView1.ScrollView.Content)
                    {
                        itemsView1.ItemsSource = ItemsSource;
                    }
                    else
                    {
                        itemsRepeater.ItemsSource = ItemsSource;
                    }
                }
            }
        }

        private void SetItemsRepeatersVerticalCacheLength()
        {
            if (txtItemsRepeatersVerticalCacheLength != null)
            {
                foreach (ItemsRepeater itemsRepeater in ItemsRepeaters)
                {
                    if (itemsRepeater != null)
                    {
                        itemsRepeater.VerticalCacheLength = Convert.ToDouble(txtItemsRepeatersVerticalCacheLength.Text);
                    }
                }
            }
        }

        private void SetItemsRepeatersHorizontalAlignment()
        {
            if (cmbItemsRepeatersHorizontalAlignment != null)
            {
                foreach (ItemsRepeater itemsRepeater in ItemsRepeaters)
                {
                    if (itemsRepeater != null)
                    {
                        itemsRepeater.HorizontalAlignment = (HorizontalAlignment)cmbItemsRepeatersHorizontalAlignment.SelectedIndex;
                    }
                }
            }
        }

        private void UpdateItemsRepeatersHorizontalAlignment()
        {
            if (cmbItemsRepeatersHorizontalAlignment != null)
            {
                foreach (ItemsRepeater itemsRepeater in ItemsRepeaters)
                {
                    if (itemsRepeater != null)
                    {
                        cmbItemsRepeatersHorizontalAlignment.SelectedIndex = (int)itemsRepeater.HorizontalAlignment;
                        break;
                    }
                }
            }
        }

        private void UpdateItemsRepeatersVerticalCacheLength()
        {
            if (txtItemsRepeatersVerticalCacheLength != null)
            {
                foreach (ItemsRepeater itemsRepeater in ItemsRepeaters)
                {
                    if (itemsRepeater != null)
                    {
                        txtItemsRepeatersVerticalCacheLength.Text = itemsRepeater.VerticalCacheLength.ToString();
                        break;
                    }
                }
            }
        }

        private void UpdateItemsRepeatersItemsSourceCount()
        {
            if (txtItemsRepeatersItemsSourceCount != null && ItemsSource != null)
            {
                txtItemsRepeatersItemsSourceCount.Text = ItemsSource.Count.ToString();
            }
        }

        private void BtnGetItemsRepeatersVerticalCacheLength_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsRepeatersVerticalCacheLength();
        }

        private void BtnSetItemsRepeatersVerticalCacheLength_Click(object sender, RoutedEventArgs e)
        {
            SetItemsRepeatersVerticalCacheLength();
        }

        private void BtnGetItemsRepeatersItemsSourceCount_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsRepeatersItemsSourceCount();
        }

        private void BtnSetItemsRepeatersItemsSourceCount_Click(object sender, RoutedEventArgs e)
        {
            SetItemsRepeatersItemsSourceCount();
            SetItemsRepeatersItemsSource();
        }

        private void CmbItemsRepeatersHorizontalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            SetItemsRepeatersHorizontalAlignment();
        }
    }
}
