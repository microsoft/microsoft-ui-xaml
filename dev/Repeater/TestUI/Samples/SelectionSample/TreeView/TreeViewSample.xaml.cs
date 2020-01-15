// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections;
using System.Diagnostics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ItemsRepeaterElementIndexChangedEventArgs = Microsoft.UI.Xaml.Controls.ItemsRepeaterElementIndexChangedEventArgs;
using ItemsRepeaterElementPreparedEventArgs = Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs;
using RecyclingElementFactory = Microsoft.UI.Xaml.Controls.RecyclingElementFactory;
using SelectionModel = Microsoft.UI.Xaml.Controls.SelectionModel;
using SelectTemplateEventArgs = Microsoft.UI.Xaml.Controls.SelectTemplateEventArgs;

namespace MUXControlsTestApp.Samples.Selection
{
    public sealed partial class TreeViewSample : Page
    {
        static object _data = Data.CreateNested(3, 2, 4);
        public TreeViewSample()
        {
            this.InitializeComponent();
            rootRepeater.ItemsSource = _data;
            selectionModel.Source = _data;
            selectionModel.PropertyChanged += SelectionModel_PropertyChanged;
        }

        private void SelectionModel_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "SelectedItem")
            {
                var selected = (sender as SelectionModel).SelectedItem;
            }
        }

        private void ElementFactory_SelectTemplateKey(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            args.TemplateKey = args.DataContext is IEnumerable ? "group" : "item";
        }

        private void OnBackClicked(object sender, RoutedEventArgs e)
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
            Frame.GoBack();
        }

        private void OnMultipleSelectionClicked(object sender, RoutedEventArgs e)
        {
            selectionModel.SingleSelect = multipleSelection.IsChecked.Value ? false : true;
        }

        private void Repeater_ElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            if (args.Element is TreeViewItem)
            {
                (args.Element as TreeViewItem).RepeatedIndex = args.Index;
            }
        }

        private IList GetData()
        {
            var indices = indexPath.Text.Split('.');
            IList data = (IList)_data;
            foreach (var index in indices)
            {
                data = (IList)(data as IList)[int.Parse(index)];
            }

            return data;
        }

        private IList GetParentData()
        {
            var indices = indexPath.Text.Split('.');
            IList data = (IList)_data;
            for (int depth = 0; depth < indices.Length - 1; depth++)
            {
                var index = indices[depth];
                data = (IList)(data as IList)[int.Parse(index)];
            }

            return data;
        }

        private int GetLeafIndex()
        {
            var indices = indexPath.Text.Split('.');
            return int.Parse(indices[indices.Length - 1]);
        }

        private void insert_Click(object sender, RoutedEventArgs e)
        {
            var data = GetParentData();
            data.Insert(GetLeafIndex(), 100);
        }

        private void remove_Click(object sender, RoutedEventArgs e)
        {
            var data = GetParentData();
            data.RemoveAt(GetLeafIndex());
        }

        private void clear_Click(object sender, RoutedEventArgs e)
        {
            var data = GetData();
            data.Clear();
        }

        private void OnElementIndexChanged(ItemsRepeater sender, ItemsRepeaterElementIndexChangedEventArgs args)
        {
            if (args.Element is TreeViewItem)
            {
                (args.Element as TreeViewItem).RepeatedIndex = args.NewIndex;
            }
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            var indices = selectionModel.SelectedIndices;
            var items = selectionModel.SelectedItems;
            Debug.Assert(indices.Count == items.Count);
            for (int i = 0; i < items.Count; i++)
            {
                Debug.WriteLine(indices[i] + ":" + items[i]);
            }

            foreach (var index in selectionModel.SelectedIndices)
            {
                Debug.WriteLine(index);
            }

            foreach (var obj in selectionModel.SelectedItems)
            {
                Debug.WriteLine(obj);
            }
        }
    }
}
