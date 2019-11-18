// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using System;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace NugetPackageTestApp
{
    public sealed partial class RepeaterTestPage : TestPage
    {
        private ObservableCollection<string> items = new ObservableCollection<string>();

        public RepeaterTestPage()
        {
            InitializeComponent();

            Repeater.ItemsSource = items;
        }

        public void OnAddItemsClicked(Object sender, RoutedEventArgs e)
        {
            items.Add("Item1");
            items.Add("Item2");
            items.Add("Item3");
        }

        public void OnRemoveItemClicked(Object sender, RoutedEventArgs e)
        {
            items.RemoveAt(0);
        }

        public void OnUniformGridLayoutClicked(Object sender, RoutedEventArgs e)
        {
            Repeater.Layout = new UniformGridLayout();
        }
        public void OnClearOutputClicked(Object sender, RoutedEventArgs e)
        {
            OutputText.Text = string.Empty;
        }
        
        private void OnElementPrepared(ItemsRepeater sender,ItemsRepeaterElementPreparedEventArgs args)
        {
            OutputText.Text += args.Index + "Prepared" + Environment.NewLine;
        }

        private void OnElementClearing(ItemsRepeater sender, ItemsRepeaterElementClearingEventArgs args)
        {
            OutputText.Text += "Element Cleared" + Environment.NewLine;
        }
    }
}
