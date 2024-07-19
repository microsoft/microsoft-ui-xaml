﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp.Samples.Selection
{
    public sealed partial class GroupedSample : Page
    {
        private Random _rnd = new Random(12345);
        private StackLayout _layout = new StackLayout();
        private ObservableCollection<object> _groupedData = new ObservableCollection<object>(Enumerable.Range(0, 10).Select(i => Enumerable.Range(0, 20).Select(j => j.ToString()).ToList()));

        public GroupedSample()
        {
            this.InitializeComponent();
            repeater.ItemsSource = _groupedData;
            selectionModel.Source = _groupedData;
        }

        private void OnMultipleSelectionClicked(object sender, RoutedEventArgs e)
        {
            selectionModel.SingleSelect = multipleSelection.IsChecked.Value ? false : true;
        }

        private void OnSelectTemplateKey(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            args.TemplateKey = args.DataContext is string ? "RepeaterItemTemplate" : "RepeaterGroupTemplate";
        }

        private void groupRepeater_ElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            if (args.Element is GroupedRepeaterItem)
            {
                (args.Element as GroupedRepeaterItem).RepeatedIndex = args.Index;
            }
        }

        private void OnBackClicked(object sender, RoutedEventArgs e)
        {
            Frame.GoBack();
        }

        private void OnTreeViewClicked(object sender, RoutedEventArgs e)
        {
            Frame.NavigateWithoutAnimation(typeof(TreeViewSample));
        }

        private void groupRepeater_ElementIndexChanged(ItemsRepeater sender, ItemsRepeaterElementIndexChangedEventArgs args)
        {
            if (args.Element is GroupedRepeaterItem)
            {
                (args.Element as GroupedRepeaterItem).RepeatedIndex = args.NewIndex;
            }
        }
    }
}
