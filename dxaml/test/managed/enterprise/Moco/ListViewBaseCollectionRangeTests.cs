// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Foundation.Collections;
using ListControl = Microsoft.UI.Xaml.Controls.ListViewBase;

namespace Microsoft.UI.Xaml.Tests.Controls.ListViewBase
{
    // Regression tests for multi-item INotifyCollectionChanged notifications
    // (GitHub issues #9607, #10235 and #11254).
    public partial class ListViewBaseTests
    {
        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void CollectionRangeNotificationsAreRaisedPerItem()
        {
            UIExecutor.Execute(() =>
            {
                foreach (bool useGridView in new[] { false, true })
                {
                    VerifyRangeNotifications(useGridView, "InsertRange",
                        new[] { "A", "B", "C" },
                        source => source.InsertRange(1, "X", "Y", "Z"),
                        new RangeNotification(CollectionChange.ItemInserted, 1, "A", "X", "B", "C"),
                        new RangeNotification(CollectionChange.ItemInserted, 2, "A", "X", "Y", "B", "C"),
                        new RangeNotification(CollectionChange.ItemInserted, 3, "A", "X", "Y", "Z", "B", "C"));

                    VerifyRangeNotifications(useGridView, "RemoveRange",
                        new[] { "A", "B", "C", "D", "E" },
                        source => source.RemoveRange(1, 3),
                        new RangeNotification(CollectionChange.ItemRemoved, 1, "A", "C", "D", "E"),
                        new RangeNotification(CollectionChange.ItemRemoved, 1, "A", "D", "E"),
                        new RangeNotification(CollectionChange.ItemRemoved, 1, "A", "E"));

                    VerifyRangeNotifications(useGridView, "Replace with equal counts",
                        new[] { "A", "B", "C", "D" },
                        source => source.ReplaceRange(1, 2, "X", "Y"),
                        new RangeNotification(CollectionChange.ItemChanged, 1, "A", "X", "C", "D"),
                        new RangeNotification(CollectionChange.ItemChanged, 2, "A", "X", "Y", "D"));

                    VerifyRangeNotifications(useGridView, "Replace that grows the collection",
                        new[] { "A", "B", "C" },
                        source => source.ReplaceRange(1, 1, "X", "Y"),
                        new RangeNotification(CollectionChange.ItemChanged, 1, "A", "X", "C"),
                        new RangeNotification(CollectionChange.ItemInserted, 2, "A", "X", "Y", "C"));

                    VerifyRangeNotifications(useGridView, "Replace that shrinks the collection",
                        new[] { "A", "B", "C", "D" },
                        source => source.ReplaceRange(1, 2, "X"),
                        new RangeNotification(CollectionChange.ItemChanged, 1, "A", "X", "C", "D"),
                        new RangeNotification(CollectionChange.ItemRemoved, 2, "A", "X", "D"));
                }
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void CollectionRangeNotificationsKeepContainersInSync()
        {
            foreach (bool useStackPanel in new[] { true, false })
            {
                Log.Comment(useStackPanel ? "ItemsPanel: StackPanel" : "ItemsPanel: default");
                var source = new RangeCollection<string>(new[] { "A", "B", "C", "D", "E" });
                ListView list = null;
                UIExecutor.Execute(() =>
                {
                    list = new ListView { Height = 800 };
                    if (useStackPanel)
                    {
                        list.ItemsPanel = (ItemsPanelTemplate)XamlReader.Load(
                            "<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><StackPanel /></ItemsPanelTemplate>");
                    }
                    list.ItemsSource = source;
                    TestServices.WindowHelper.WindowContent = list;
                });
                TestServices.WindowHelper.WaitForIdle();

                VerifyRangeChangeKeepsContainersInSync(list, source, "InsertRange", () => source.InsertRange(1, "X", "Y", "Z"));
                // #11254: a removal following a range Add used to fail with E_INVALIDARG.
                VerifyRangeChangeKeepsContainersInSync(list, source, "RemoveAt after InsertRange", () => source.RemoveAt(1));
                // #10235
                VerifyRangeChangeKeepsContainersInSync(list, source, "RemoveRange", () => source.RemoveRange(0, 3));
                // #9607
                VerifyRangeChangeKeepsContainersInSync(list, source, "Replace that grows", () => source.ReplaceRange(0, 1, "P", "Q", "R"));
                VerifyRangeChangeKeepsContainersInSync(list, source, "Replace that shrinks", () => source.ReplaceRange(1, 3, "S"));
                VerifyRangeChangeKeepsContainersInSync(list, source, "Add after range changes", () => source.Add("T"));

                TestServices.WindowHelper.ResetWindowContentAndWaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void CollectionRangeNotificationsPreserveSelection()
        {
            var source = new RangeCollection<string>(new[] { "A", "B", "C", "D", "E" });
            ListView list = null;
            UIExecutor.Execute(() =>
            {
                list = new ListView { Height = 800, SelectionMode = ListViewSelectionMode.Single };
                list.ItemsSource = source;
                TestServices.WindowHelper.WindowContent = list;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                list.SelectedItem = "C";
                Verify.AreEqual(2, list.SelectedIndex);

                source.InsertRange(0, "X", "Y");
                Verify.AreEqual("C", list.SelectedItem, "InsertRange before the selection must keep the selected item.");
                Verify.AreEqual(4, list.SelectedIndex, "InsertRange before the selection must shift the selected index by the range size.");

                source.RemoveRange(0, 2);
                Verify.AreEqual("C", list.SelectedItem, "RemoveRange before the selection must keep the selected item.");
                Verify.AreEqual(2, list.SelectedIndex, "RemoveRange before the selection must shift the selected index by the range size.");
            });

            TestServices.WindowHelper.ResetWindowContentAndWaitForIdle();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void CollectionRangeNotificationsPreserveMultipleSelection()
        {
            foreach (ListViewSelectionMode mode in new[] { ListViewSelectionMode.Multiple, ListViewSelectionMode.Extended })
            {
                foreach (bool useGridView in new[] { false, true })
                {
                    Log.Comment("{0}, selection mode {1}", useGridView ? "GridView" : "ListView", mode);
                    var source = new RangeCollection<string>(new[] { "A", "B", "C", "D", "E", "F", "G", "H" });
                    var removedItems = new List<object>();
                    var addedItems = new List<object>();
                    ListControl control = null;
                    UIExecutor.Execute(() =>
                    {
                        control = CreateRangeControl(useGridView);
                        control.Width = 400;
                        control.Height = 800;
                        control.SelectionMode = mode;
                        control.ItemsSource = source;
                        TestServices.WindowHelper.WindowContent = control;
                    });
                    TestServices.WindowHelper.WaitForIdle();

                    UIExecutor.Execute(() =>
                    {
                        foreach (string item in new[] { "B", "D", "F" })
                        {
                            control.SelectedItems.Add(item);
                        }
                        control.SelectionChanged += (sender, args) =>
                        {
                            removedItems.AddRange(args.RemovedItems);
                            addedItems.AddRange(args.AddedItems);
                        };
                    });
                    TestServices.WindowHelper.WaitForIdle();

                    // A B C [X Y] D E F G H
                    VerifyRangeChangePreservesSelection(control, source, "InsertRange between selected items",
                        () => source.InsertRange(3, "X", "Y"),
                        new[] { "B", "D", "F" }, new string[0], removedItems, addedItems);
                    // A B [C X Y D] E F G H: removes the selected D along with unselected items.
                    VerifyRangeChangePreservesSelection(control, source, "RemoveRange of selected and unselected items",
                        () => source.RemoveRange(2, 4),
                        new[] { "B", "F" }, new[] { "D" }, removedItems, addedItems);
                    // A [B E] F G H -> A P Q R F G H: replaces the selected B and the unselected E.
                    VerifyRangeChangePreservesSelection(control, source, "Replace of selected and unselected items",
                        () => source.ReplaceRange(1, 2, "P", "Q", "R"),
                        new[] { "F" }, new[] { "B" }, removedItems, addedItems);
                    // Clearing unselects by stored index, so a stale index would leave a container selected.
                    VerifyRangeChangePreservesSelection(control, source, "Clear the selection after the range changes",
                        () => control.SelectedItems.Clear(),
                        new string[0], new[] { "F" }, removedItems, addedItems);

                    TestServices.WindowHelper.ResetWindowContentAndWaitForIdle();
                }
            }
        }

        private static void VerifyRangeChangePreservesSelection(
            ListControl control, RangeCollection<string> source, string description, Action change,
            string[] expectedSelection, string[] expectedUnselected, List<object> removedItems, List<object> addedItems)
        {
            UIExecutor.Execute(() =>
            {
                Log.Comment(description);
                removedItems.Clear();
                addedItems.Clear();
                change();
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(
                    string.Join(",", expectedSelection),
                    string.Join(",", control.SelectedItems.Cast<string>().OrderBy(item => source.IndexOf(item))),
                    "SelectedItems must keep exactly the selected items that are still in the source.");
                // SelectedRanges is not checked: it is only refreshed when the selection itself
                // changes, so it is stale after any shifting insert or removal, even a single-item one.
                Verify.AreEqual(source.IndexOf((string)control.SelectedItem), control.SelectedIndex,
                    "SelectedIndex must match SelectedItem.");
                Verify.AreEqual(string.Join(",", expectedUnselected), string.Join(",", removedItems.Cast<string>()),
                    "Only removed or replaced selected items may be unselected.");
                Verify.AreEqual(0, addedItems.Count, "A collection change must not select items.");

                for (int i = 0; i < source.Count; i++)
                {
                    var container = control.ContainerFromIndex(i) as SelectorItem;
                    Verify.IsNotNull(container, "Every item must be realized at " + i);
                    Verify.AreEqual(source[i], container.Content, "Unexpected container content at " + i);
                    Verify.AreEqual(expectedSelection.Contains(source[i]), container.IsSelected,
                        "Container selection state must match the selection model at " + i);
                }
            });
        }

        private static void VerifyRangeNotifications(
            bool useGridView, string description, string[] initialItems,
            Action<RangeCollection<string>> change, params RangeNotification[] expected)
        {
            Log.Comment("{0}: {1}", useGridView ? "GridView" : "ListView", description);
            var source = new RangeCollection<string>(initialItems);
            var control = CreateRangeControl(useGridView);
            control.ItemsSource = source;
            var viewSource = new CollectionViewSource { Source = source };

            using (var itemObserver = new RangeNotificationObserver(control.Items, expected))
            using (var viewObserver = new RangeNotificationObserver(viewSource.View, expected))
            {
                change(source);
                itemObserver.VerifyComplete(source);
                viewObserver.VerifyComplete(source);
            }
        }

        private static void VerifyRangeChangeKeepsContainersInSync(
            ListView list, RangeCollection<string> source, string description, Action change)
        {
            UIExecutor.Execute(() =>
            {
                Log.Comment(description);
                change();
            });
            // Replaced containers are rebound by ListView's deferred tree building, not by UpdateLayout.
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(source.Count, list.Items.Count, "Items.Count must match the source.");
                for (int i = 0; i < source.Count; i++)
                {
                    Verify.AreEqual(source[i], list.Items[i], "Unexpected item at " + i);
                    var container = list.ContainerFromIndex(i) as ListViewItem;
                    Verify.IsNotNull(container, "Every item must be realized at " + i);
                    Verify.AreEqual(source[i], container.Content, "Unexpected container content at " + i);
                    Verify.AreEqual(i, list.IndexFromContainer(container), "Container index mapping must match the source at " + i);
                }
                Verify.IsNull(list.ContainerFromIndex(source.Count), "No container may exist beyond the source.");
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        private static ListControl CreateRangeControl(bool useGridView)
        {
            return useGridView ? (ListControl)new GridView() : new ListView();
        }

        private sealed class RangeNotification
        {
            public readonly CollectionChange Change;
            public readonly uint Index;
            public readonly string[] Items;

            public RangeNotification(CollectionChange change, uint index, params string[] items)
            {
                Change = change;
                Index = index;
                Items = items;
            }
        }

        private sealed class RangeNotificationObserver : IDisposable
        {
            private readonly IObservableVector<object> vector;
            private readonly RangeNotification[] expected;
            private int notificationCount;

            public RangeNotificationObserver(IObservableVector<object> vector, RangeNotification[] expected)
            {
                this.vector = vector;
                this.expected = expected;
                vector.VectorChanged += OnVectorChanged;
            }

            private void OnVectorChanged(IObservableVector<object> sender, IVectorChangedEventArgs args)
            {
                Verify.IsTrue(notificationCount < expected.Length,
                    string.Format("Unexpected extra notification {0} at {1}.", args.CollectionChange, args.Index));
                var notification = expected[notificationCount];
                Verify.AreEqual(notification.Change, args.CollectionChange, "Unexpected change for notification " + notificationCount);
                Verify.AreEqual(notification.Index, args.Index, "Unexpected index for notification " + notificationCount);
                VerifyItems(notification.Items, "notification " + notificationCount);
                notificationCount++;
            }

            private void VerifyItems(string[] items, string context)
            {
                // Each notification must describe a collection that is consistent with every
                // notification raised so far, not the source's final state.
                Verify.AreEqual(items.Length, vector.Count, "Unexpected Count during " + context);
                for (int i = 0; i < items.Length; i++)
                {
                    Verify.AreEqual(items[i], vector[i], "Unexpected item at " + i + " during " + context);
                    Verify.AreEqual(i, vector.IndexOf(items[i]), "Unexpected IndexOf(" + items[i] + ") during " + context);
                }
            }

            public void VerifyComplete(IList<string> source)
            {
                Verify.AreEqual(expected.Length, notificationCount, "Missing vector notifications.");
                VerifyItems(source.ToArray(), "completion");
            }

            public void Dispose()
            {
                vector.VectorChanged -= OnVectorChanged;
            }
        }

        // Raises a single multi-item notification per range operation, like popular
        // range-capable collections do.
        private sealed class RangeCollection<T> : ObservableCollection<T>
        {
            public RangeCollection(IEnumerable<T> items) : base(items)
            {
            }

            public void InsertRange(int index, params T[] items)
            {
                CheckReentrancy();
                for (int i = 0; i < items.Length; i++)
                {
                    Items.Insert(index + i, items[i]);
                }
                RaiseRangeChange(new NotifyCollectionChangedEventArgs(
                    NotifyCollectionChangedAction.Add, (IList)items.ToList(), index));
            }

            public void RemoveRange(int index, int count)
            {
                CheckReentrancy();
                var removed = Items.Skip(index).Take(count).ToList();
                for (int i = 0; i < count; i++)
                {
                    Items.RemoveAt(index);
                }
                RaiseRangeChange(new NotifyCollectionChangedEventArgs(
                    NotifyCollectionChangedAction.Remove, (IList)removed, index));
            }

            public void ReplaceRange(int index, int oldCount, params T[] newItems)
            {
                CheckReentrancy();
                var oldItems = Items.Skip(index).Take(oldCount).ToList();
                for (int i = 0; i < oldCount; i++)
                {
                    Items.RemoveAt(index);
                }
                for (int i = 0; i < newItems.Length; i++)
                {
                    Items.Insert(index + i, newItems[i]);
                }
                RaiseRangeChange(new NotifyCollectionChangedEventArgs(
                    NotifyCollectionChangedAction.Replace, (IList)newItems.ToList(), (IList)oldItems, index));
            }

            private void RaiseRangeChange(NotifyCollectionChangedEventArgs args)
            {
                OnPropertyChanged(new PropertyChangedEventArgs("Count"));
                OnPropertyChanged(new PropertyChangedEventArgs("Item[]"));
                OnCollectionChanged(args);
            }
        }
    }
}
