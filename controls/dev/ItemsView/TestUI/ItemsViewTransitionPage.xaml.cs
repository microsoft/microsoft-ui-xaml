// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Numerics;
using Windows.Foundation;
using Windows.Storage;
using Windows.System;
using ItemsViewTestHooks = Microsoft.UI.Private.Controls.ItemsViewTestHooks;

namespace MUXControlsTestApp
{
    public class RefreshableCollection<T> : ObservableCollection<T>
    {
        public void Refresh(IEnumerable<T> newItems)
        {
            Items.Clear();

            foreach (T item in newItems)
            {
                Items.Add(item);
            }

            OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
        }
    }

    public sealed partial class ItemsViewTransitionPage : TestPage
    {
        readonly RefreshableCollection<BitmapImage> items = new();

        public ItemsViewTransitionPage()
        {
            this.InitializeComponent();

            ItemCountComboBox.Items.Add(1);
            ItemCountComboBox.Items.Add(5);
            ItemCountComboBox.Items.Add(10);
            ItemCountComboBox.Items.Add(100);
            ItemCountComboBox.SelectedIndex = 0;
        }

        private void OnItemsViewKeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == VirtualKey.Delete)
            {
                DeleteSelectedItem();
            }
        }

        private void OnDeleteButtonClick(object sender, RoutedEventArgs e)
        {
            DeleteSelectedItem();
        }

        private void OnAddButtonClick(object sender, RoutedEventArgs e)
        {
            items.Insert(0, new BitmapImage(new Uri($"ms-appx:///Images/vette6.jpg")));
        }

        private void OnMoveButtonClick(object sender, RoutedEventArgs e)
        {
            var selectionModel = ItemsViewTestHooks.GetSelectionModel(itemsView);
            if (selectionModel.SelectedIndex?.GetSize() > 0)
            {
                int selectedIndex = selectionModel.SelectedIndex.GetAt(0);

                var item = items[selectedIndex];
                items.RemoveAt(selectedIndex);
                items.Insert(0, item);
            }
        }

        private void OnRefreshButtonClick(object sender, RoutedEventArgs e)
        {
            RefreshItems((int)ItemCountComboBox.SelectedItem);
        }

        private void OnResetItemTransitionProviderButtonClick(object sender, RoutedEventArgs e)
        {
            itemsView.ItemTransitionProvider = null;
        }

        private void OnLayoutSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (((ComboBox)sender).SelectedIndex)
            {
                case 0:
                    itemsView.Layout = null;
                    break;
                case 1:
                    itemsView.Layout = (LinedFlowLayout)Resources["LinedFlowLayout"];
                    itemsView.ItemTransitionProvider = (LinedFlowLayoutItemCollectionTransitionProvider)Resources["LinedFlowLayoutItemCollectionTransitionProvider"];
                    break;
                case 2:
                    itemsView.Layout = (StackLayout)Resources["StackLayout"];
                    break;
                case 3:
                    itemsView.Layout = (UniformGridLayout)Resources["UniformGridLayout"];
                    break;
            }
        }

        private void DeleteSelectedItem()
        {
            var selectionModel = ItemsViewTestHooks.GetSelectionModel(itemsView);
            if (selectionModel.SelectedIndex?.GetSize() > 0)
            {
                int selectedIndex = selectionModel.SelectedIndex.GetAt(0);
                items.RemoveAt(selectedIndex);
            }
        }

        private void RefreshItems(int itemCount)
        {
            List<BitmapImage> newItems = new();

            foreach (int i in Enumerable.Range(0, itemCount))
            {
                BitmapImage image = new(new Uri($"ms-appx:///Images/vette{i % 126 + 1}.jpg"));
                newItems.Add(image);
            }

            items.Refresh(newItems);
        }

        private void OnClearHistoryButtonClick(object sender, RoutedEventArgs e)
        {
            TransitionCompletedHistoryTextBox.Text = string.Empty;
        }

        private void OnTransitionProviderTransitionCompleted(ItemCollectionTransitionProvider sender, ItemCollectionTransitionCompletedEventArgs args)
        {
            static string BoundsToString(Rect bounds)
            {
                return $"({Math.Round(bounds.X)}, {Math.Round(bounds.Y)}, {Math.Round(bounds.Width)}, {Math.Round(bounds.Height)})";
            }

            TransitionCompletedHistoryTextBox.Text =
                $"Operation = {args.Transition.Operation}, " +
                $"Triggers = {args.Transition.Triggers}, " +
                $"OldBounds = {BoundsToString(args.Transition.OldBounds)}, " +
                $"NewBounds = {BoundsToString(args.Transition.NewBounds)}" +
                (TransitionCompletedHistoryTextBox.Text.Length > 0 ? Environment.NewLine : "") +
                TransitionCompletedHistoryTextBox.Text;
        }
    }
}
