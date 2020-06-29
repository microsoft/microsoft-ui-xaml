// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

using RecyclingElementFactory = Microsoft.UI.Xaml.Controls.RecyclingElementFactory;
using SelectTemplateEventArgs = Microsoft.UI.Xaml.Controls.SelectTemplateEventArgs;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class CollectionChangeDemo : Page
    {
        MyDataSource _dataSource = new MyDataSource(Enumerable.Range(0, 10).Select(i => i.ToString()).ToList());
        public List<object> ResettingListItems { get; set; } = new List<object> { "item0", "item1", "item2", "item3", "item4", "item5", "item6", "item7", "item8", "item9" };
        public CollectionChangeDemo()
        {
            this.InitializeComponent();
            goBackButton.Click += delegate { Frame.GoBack(); };
            insertButton.Click += delegate { _dataSource.Insert(int.Parse(newStartIndex.Text), int.Parse(newCount.Text), resetMode.IsChecked ?? false); };
            removeButton.Click += delegate { _dataSource.Remove(int.Parse(oldStartIndex.Text), int.Parse(oldCount.Text), resetMode.IsChecked ?? false); };
            replaceButton.Click += delegate { _dataSource.Replace(int.Parse(oldStartIndex.Text), int.Parse(oldCount.Text), int.Parse(newCount.Text), resetMode.IsChecked ?? false); };
            moveButton.Click += delegate { _dataSource.Move(int.Parse(oldStartIndex.Text), int.Parse(newStartIndex.Text), resetMode.IsChecked ?? false); };
            resetButton.Click += delegate { _dataSource.Reset(); };

            repeater.ItemTemplate = elementFactory;
            repeater.ItemsSource = _dataSource;
        }

        private void OnSelectTemplateKey(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            args.TemplateKey = (int.Parse(args.DataContext.ToString()) % 2 == 0) ? "even" : "odd";
        }

        private void ResettingCollectionRemoveItemButton_ItemClick(object sender, RoutedEventArgs e)
        {
            ResettingListItems.Remove((sender as Button).Content);
            ResettingListItems = new List<object>(ResettingListItems);
            ResettingCollectionRepeater.ItemsSource = ResettingListItems;
        }
        private void OnItemClicked(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            int index = repeater.GetElementIndex(sender as UIElement);
            _dataSource.Remove(index, 1 /* count */ , true /* reset */);
        }

        private class MyDataSource : CustomItemsSourceViewWithUniqueIdMapping
        {
            List<string> _inner;

            public MyDataSource(List<string> source)
            {
                Inner = source;
            }

            protected override int GetSizeCore()
            {
                return Inner.Count;
            }

            public List<string> Inner
            {
                get
                {
                    return _inner;
                }

                set
                {
                    _inner = value;
                }
            }

            protected override object GetAtCore(int index)
            {
                return Inner[index];
            }

            protected override string KeyFromIndexCore(int index)
            {
                // data is the same as its unique id
                return Inner[index].ToString();
            }

            public void Insert(int index, int count, bool reset)
            {
                for (int i = 0; i < count; i++)
                {
                    Inner.Insert(index + i, (1000 + i).ToString());
                }

                if (reset)
                {
                    OnItemsSourceChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
                }
                else
                {
                    OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(
                        NotifyCollectionChangedAction.Add,
                        oldStartingIndex: -1,
                        oldItemsCount: 0,
                        newStartingIndex: index,
                        newItemsCount: count));
                }
            }

            public void Remove(int index, int count, bool reset)
            {
                for (int i = 0; i < count; i++)
                {
                    Inner.RemoveAt(index);
                }

                if (reset)
                {
                    OnItemsSourceChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
                }
                else
                {
                    OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(
                        NotifyCollectionChangedAction.Remove,
                        oldStartingIndex: index,
                        oldItemsCount: count,
                        newStartingIndex: -1,
                        newItemsCount: 0));
                }
            }

            public void Replace(int index, int oldCount, int newCount, bool reset)
            {
                for (int i = 0; i < oldCount; i++)
                {
                    Inner.RemoveAt(index);
                }

                for (int i = 0; i < newCount; i++)
                {
                    Inner.Insert(index, (10000 + i).ToString());
                }

                if (reset)
                {
                    OnItemsSourceChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
                }
                else
                {
                    OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(
                        NotifyCollectionChangedAction.Replace,
                        oldStartingIndex: index,
                        oldItemsCount: oldCount,
                        newStartingIndex: index,
                        newItemsCount: newCount));
                }
            }

            public void Move(int oldIndex, int newIndex, bool reset)
            {
                var item = Inner[oldIndex];
                Inner.RemoveAt(oldIndex);
                Inner.Insert(newIndex, item);

                if (reset)
                {
                    OnItemsSourceChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
                }
                else
                {
                    OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(
                        NotifyCollectionChangedAction.Move,
                        oldStartingIndex: oldIndex,
                        oldItemsCount: 1,
                        newStartingIndex: newIndex,
                        newItemsCount: 1));
                }
            }

            public void Reset()
            {
                Random rand = new Random(123);
                for (int i = 0; i < 10; i++)
                {
                    int from = rand.Next(0, Inner.Count - 1);
                    var value = Inner[from];
                    Inner.RemoveAt(from);
                    int to = rand.Next(0, Inner.Count - 1);
                    Inner.Insert(to, value);
                }

                // something changed, but i dont want to tell you the 
                // exact changes 
                OnItemsSourceChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
            }
        }
    }
}
