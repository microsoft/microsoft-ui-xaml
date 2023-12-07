// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Windows.ApplicationModel.DataTransfer;

namespace MUXControlsTestApp
{
    public sealed partial class MultipleTabViewPage : TestPage
    {
        public ObservableCollection<TabDataItem> FirstTabViewItems { get; } = new();
        public ObservableCollection<TabDataItem> SecondTabViewItems { get; } = new();
        public ObservableCollection<TabDataItem> CompactTabViewItems { get; } = new();
        public ObservableCollection<TabDataItem> SizeToContentTabViewItems { get; } = new();

        private int tabCount = 0;
        private SymbolIconSource symbolIconSource = new SymbolIconSource() { Symbol = Symbol.Placeholder };
        private const string tabDataItemIdentifier = "TabDataItem";
        private const string originatingTabViewIdentifier = "OriginatingTabView";

        private readonly Dictionary<TabDataItem, TabViewItem> dataItemToTabItemDictionary = new();

        public MultipleTabViewPage()
        {
            this.InitializeComponent();

            AddNewTab(FirstTabViewItems);
            AddNewTab(FirstTabViewItems);
            AddNewTab(FirstTabViewItems);
            AddNewTab(FirstTabViewItems);
            AddNewTab(SecondTabViewItems);
            AddNewTab(CompactTabViewItems);
            AddNewTab(SizeToContentTabViewItems);
        }

        public static string RemoveWhitespace(string s)
        {
            return s.Replace(" ", "");
        }

        private void AddNewTab(ObservableCollection<TabDataItem> itemCollection, int position = -1, TabDataItem item = null)
        {
            if (item == null)
            {
                item = new TabDataItem {
                    IconSource = symbolIconSource,
                    Header = $"Item {tabCount}",
                    Content = $"This is tab {tabCount}."
                };

                tabCount++;
            }

            if (position < 0)
            {
                itemCollection.Add(item);
            }
            else
            {
                itemCollection.Insert(position, item);
            }
        }

        private void TabView_AddButtonClick(TabView sender, object args)
        {
            AddNewTab((ObservableCollection<TabDataItem>)sender.TabItemsSource);
        }

        private void TabView_TabCloseRequested(TabView sender, TabViewTabCloseRequestedEventArgs args)
        {
            ((ObservableCollection<TabDataItem>)sender.TabItemsSource).Remove((TabDataItem)args.Item);
        }

        private void TabView_TabDragStarting(TabView sender, TabViewTabDragStartingEventArgs args)
        {
            args.Data.Properties.Add(tabDataItemIdentifier, args.Item);
            args.Data.Properties.Add(originatingTabViewIdentifier, sender);
            args.Data.RequestedOperation = DataPackageOperation.Move;
        }

        private void TabView_DragEnter(object sender, DragEventArgs args)
        {
            TabView target = (TabView)sender;

            // If we don't have the properties we expect in the data view properties, or if this is the same tab view
            // as what originated the drag, then we don't want to do anything - this either isn't a package we want to handle,
            // or it's a drag within the same tab view, in which case the tab view will handle everything internally.
            if (!args.DataView.Properties.TryGetValue(tabDataItemIdentifier, out _) ||
                !args.DataView.Properties.TryGetValue(originatingTabViewIdentifier, out object originatingTabView) ||
                originatingTabView == target)
            {
                args.Handled = false;
                return;
            }

            args.AcceptedOperation = DataPackageOperation.Move;
        }

        private void TabView_Drop(object sender, DragEventArgs args)
        {
            TabView target = (TabView)sender;

            if (!args.DataView.Properties.TryGetValue(tabDataItemIdentifier, out object value) ||
                !args.DataView.Properties.TryGetValue(originatingTabViewIdentifier, out object originatingTabViewObject) ||
                originatingTabViewObject == target)
            {
                args.Handled = false;
                return;
            }

            TabDataItem item = (TabDataItem)value;

            int index = -1;
            for (int i = 0; i < target.TabItems.Count; i++)
            {
                if (target.TabItems[i] is TabDataItem tabDataItem)
                {
                    var tabItem = dataItemToTabItemDictionary[tabDataItem];

                    // If the mouse cursor is within the bounds of this tab item,
                    // then this is the place where we're dropping the dragged item.
                    if (args.GetPosition(tabItem).X > 0 &&
                        args.GetPosition(tabItem).X < tabItem.ActualWidth)
                    {
                        index = i;
                        break;
                    }
                }
            }

            AddNewTab((ObservableCollection<TabDataItem>)target.TabItemsSource, index, item);

            // We've dragged a tab item to a new TabView, so we'll want to remove it
            // from its original TabView now.
            var originatingTabView = (TabView)originatingTabViewObject;
            var originatingItemsSource = (ObservableCollection<TabDataItem>)originatingTabView.TabItemsSource;

            int selectedIndex = originatingItemsSource.IndexOf(item);
            originatingItemsSource.Remove(item);
            originatingTabView.SelectedIndex = selectedIndex >= originatingItemsSource.Count ? originatingItemsSource.Count - 1 : selectedIndex;
        }

        private void TabViewItem_Loaded(object sender, RoutedEventArgs e)
        {
            TabViewItem item = (TabViewItem)sender;
            TabDataItem tabDataItem = (TabDataItem)item.Tag;

            dataItemToTabItemDictionary[tabDataItem] = item;
        }
    }
}
