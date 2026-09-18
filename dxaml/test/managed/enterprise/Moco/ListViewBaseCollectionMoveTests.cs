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
using System.Runtime.InteropServices;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Settings;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Enterprise.Moco.Data;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Foundation.Collections;
using ListControl = Microsoft.UI.Xaml.Controls.ListViewBase;

namespace Microsoft.UI.Xaml.Tests.Controls.ListViewBase
{
    public partial class ListViewBaseTests
    {
        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:false}")]
        public void CollectionMoveNotificationsCanBeDisabled()
        {
            WithCollectionMoveChange(false, () =>
            {
                UIExecutor.Execute(() =>
                {
                    foreach (bool useGridView in new[] { false, true })
                    {
                        VerifyMoveNotificationCase(useGridView, MoveTestStrings(), 0, 5, 1, false, false);
                        VerifyMoveNotificationCase(useGridView, MoveTestStrings(), 5, 0, 1, false, false);
                        VerifyMoveNotificationCase(useGridView, MoveTestStrings(), 2, 2, 1, false, false);
                        VerifyMoveNotificationCase(useGridView, MoveTestStrings(), 0, 4, 2, true, false);
                        VerifyMoveNotificationCase(useGridView, MoveTestStrings(), 0, 0, 6, true, false);
                    }
                });
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void CollectionMoveSingleItemNotificationsAreCoherent()
        {
            WithCollectionMoveChange(true, () =>
            {
                UIExecutor.Execute(() =>
                {
                    int[,] moves = { { 0, 5 }, { 5, 0 }, { 1, 4 }, { 4, 1 }, { 1, 2 }, { 2, 1 }, { 0, 0 }, { 2, 2 }, { 5, 5 } };
                    foreach (bool useGridView in new[] { false, true })
                    {
                        foreach (string[] values in new[] { MoveTestStrings(), MoveTestStringsWithDuplicates() })
                        {
                            for (int i = 0; i < moves.GetLength(0); i++)
                            {
                                VerifyMoveNotificationCase(useGridView, values, moves[i, 0], moves[i, 1], 1, false, true);
                            }
                        }
                    }
                });
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:true}")]
        public void CollectionMoveRangeNotificationsAreCoherent()
        {
            WithCollectionMoveChange(true, () =>
            {
                UIExecutor.Execute(() =>
                {
                    int[,] moves =
                    {
                        { 0, 4, 2 }, { 4, 0, 2 }, { 1, 4, 2 }, { 3, 0, 2 },
                        { 1, 2, 2 }, { 2, 1, 2 }, { 0, 1, 3 }, { 1, 0, 3 },
                        { 0, 1, 5 }, { 1, 0, 5 }, { 0, 0, 2 }, { 2, 2, 2 }, { 0, 0, 6 }
                    };
                    foreach (bool useGridView in new[] { false, true })
                    {
                        foreach (string[] values in new[] { MoveTestStrings(), MoveTestStringsWithDuplicates() })
                        {
                            for (int i = 0; i < moves.GetLength(0); i++)
                            {
                                VerifyMoveNotificationCase(useGridView, values, moves[i, 0], moves[i, 1], moves[i, 2], true, true);
                            }
                        }
                    }
                });
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:true}")]
        public void CollectionMoveDoesNotChangeOtherNotifications()
        {
            VerifyCollectionMoveOtherNotifications(true);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:false}")]
        public void CollectionMoveDoesNotChangeOtherNotificationsWhenDisabled()
        {
            VerifyCollectionMoveOtherNotifications(false);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:true}")]
        public void CollectionMoveDoesNotEnumerateVirtualizedSource()
        {
            WithCollectionMoveChange(true, () =>
            {
                UIExecutor.Execute(() =>
                {
                    const int count = 4096;
                    int[,] moves = { { 0, 4095, 1 }, { 4095, 0, 1 }, { 0, 4093, 3 }, { 4093, 0, 3 } };
                    for (int i = 0; i < moves.GetLength(0); i++)
                    {
                        var source = new MoveReadTrackingSource(count);
                        var viewSource = new CollectionViewSource { Source = source };
                        var view = viewSource.View;
                        object[] finalItems;
                        var initialItems = Enumerable.Range(0, count).Select(index => (object)("Item " + index)).ToArray();
                        var expected = CreateMoveNotifications(initialItems, moves[i, 0], moves[i, 1], moves[i, 2], out finalItems);
                        int notifications = 0;
                        VectorChangedEventHandler<object> handler = (sender, args) =>
                        {
                            Verify.IsTrue(notifications < expected.Length);
                            var notification = expected[notifications++];
                            Verify.AreEqual(notification.Change, args.CollectionChange);
                            Verify.AreEqual(notification.Index, args.Index);
                            Verify.AreEqual(notification.Items.Length, view.Count);
                            Verify.AreEqual(notification.Items[0], view[0]);
                            Verify.AreEqual(notification.Items.Last(), view[view.Count - 1]);
                        };
                        view.VectorChanged += handler;
                        try
                        {
                            source.ResetReadCounts();
                            source.MoveRange(moves[i, 0], moves[i, 1], moves[i, 2]);

                            Verify.AreEqual(expected.Length, notifications);
                            Verify.AreEqual(0, source.EnumeratorCount, "Preparing a Move must not enumerate a virtualized source.");
                            Verify.AreEqual(0, source.CopyCount, "Preparing a Move must not copy a virtualized source.");
                            Verify.IsTrue(source.IndexedReadCount < 64, "Moving at most three items must not read all 4096 source items.");
                            Verify.AreEqual(count, view.Count);
                            Verify.AreEqual(finalItems[0], view[0]);
                            Verify.AreEqual(finalItems[count - 1], view[count - 1]);
                        }
                        finally
                        {
                            view.VectorChanged -= handler;
                        }
                    }
                });
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:true}")]
        public void CollectionMoveIndexOfRejectsMutationDuringRead()
        {
            WithCollectionMoveChange(true, () =>
            {
                UIExecutor.Execute(() =>
                {
                    const int changedState = unchecked((int)0x8000000C);
                    var source = new MoveReadTrackingSource(3);
                    var viewSource = new CollectionViewSource { Source = source };
                    var view = viewSource.View;
                    view.MoveCurrentToPosition(-1);
                    Verify.AreEqual(-1, view.CurrentPosition);

                    var notifications = new List<CollectionChange>();
                    bool mutatedDuringRead = false;
                    VectorChangedEventHandler<object> handler = (sender, args) =>
                    {
                        notifications.Add(args.CollectionChange);
                        if (args.CollectionChange == CollectionChange.ItemRemoved)
                        {
                            source.OnIndexedRead = () =>
                            {
                                source.OnIndexedRead = null;
                                mutatedDuringRead = true;
                                source.MoveRange(0, 1, 1);
                            };
                            VerifyMoveFailure(() => view.IndexOf("Item 1"), changedState);
                        }
                    };
                    view.VectorChanged += handler;
                    try
                    {
                        source.MoveRange(0, 2, 1);
                        Verify.IsTrue(mutatedDuringRead);
                        Verify.IsTrue(notifications.SequenceEqual(new[] { CollectionChange.ItemRemoved, CollectionChange.Reset }));
                        var finalItems = new object[] { "Item 2", "Item 1", "Item 0" };
                        VerifyMoveItems(view, finalItems, finalItems);
                        VerifyMoveSource(source, finalItems);
                    }
                    finally
                    {
                        source.OnIndexedRead = null;
                        view.VectorChanged -= handler;
                    }
                });
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:true}")]
        public void CollectionMoveRejectsReentrantViewMutation()
        {
            WithCollectionMoveChange(true, () =>
            {
                UIExecutor.Execute(() =>
                {
                    var source = new ObservableCollection<string>(new[] { "A", "B", "C" });
                    var viewSource = new CollectionViewSource { Source = source };
                    var view = viewSource.View;
                    object[] finalItems;
                    var expected = CreateMoveNotifications(source.Cast<object>().ToArray(), 0, 2, 1, out finalItems);
                    int rejectedCallbacks = 0;
                    VectorChangedEventHandler<object> handler = (sender, args) =>
                    {
                        if (args.CollectionChange == CollectionChange.ItemRemoved)
                        {
                            const int illegalMethodCall = unchecked((int)0x8000000E);
                            VerifyMoveFailure(() => view.Add("unexpected"), illegalMethodCall);
                            VerifyMoveFailure(() => view.RemoveAt(0), illegalMethodCall);
                            VerifyMoveFailure(() => view[0] = "unexpected", illegalMethodCall);
                            VerifyMoveFailure(view.Clear, illegalMethodCall);
                            VerifyMoveItems(view, new object[] { "B", "C" }, new object[] { "A", "B", "C", "unexpected" });
                            rejectedCallbacks++;
                        }
                    };
                    using (var observer = new MoveNotificationObserver(view, () => viewSource.View, source, expected, finalItems, true))
                    {
                        view.VectorChanged += handler;
                        try
                        {
                            source.Move(0, 2);
                            Verify.AreEqual(1, rejectedCallbacks);
                            observer.VerifyComplete();
                        }
                        finally
                        {
                            view.VectorChanged -= handler;
                        }
                    }
                });
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:true}")]
        public void CollectionMoveReconcilesReentrantSourceMutation()
        {
            WithCollectionMoveChange(true, () =>
            {
                UIExecutor.Execute(() =>
                {
                    const int changedState = unchecked((int)0x8000000C);
                    var source = new ObservableCollection<string>(new[] { "A", "B", "C" });
                    var viewSource = new CollectionViewSource { Source = source };
                    var view = viewSource.View;
                    Verify.IsTrue(view.MoveCurrentToFirst());
                    var finalItems = new object[] { "B", "C", "A", "nested" };
                    int notifications = 0;
                    VectorChangedEventHandler<object> handler = (sender, args) =>
                    {
                        int notification = notifications++;
                        Verify.AreEqual(0u, args.Index);
                        if (notification == 0)
                        {
                            Verify.AreEqual(CollectionChange.ItemRemoved, args.CollectionChange);
                            source.Add("nested");
                            VerifyMoveFailure(() => { _ = view.Count; }, changedState);
                            VerifyMoveFailure(() => { _ = view[0]; }, changedState);
                            VerifyMoveFailure(() => view.IndexOf("B"), changedState);
                            VerifyMoveFailure(() =>
                            {
                                using (var enumerator = view.GetEnumerator())
                                {
                                    enumerator.MoveNext();
                                }
                            }, changedState);
                        }
                        else
                        {
                            Verify.AreEqual(1, notification, "Remaining synthetic notifications must be abandoned.");
                            Verify.AreEqual(CollectionChange.Reset, args.CollectionChange);
                            VerifyMoveItems(view, finalItems, finalItems);
                            VerifyMoveSource(source, finalItems);
                        }
                    };
                    view.VectorChanged += handler;
                    try
                    {
                        // CollectionView also reads the invalidated view when updating its removed current item.
                        VerifyMoveFailure(() => source.Move(0, 2), changedState);
                        Verify.AreEqual(2, notifications, "A reentrant source mutation must be reconciled by one Reset.");
                        VerifyMoveItems(view, finalItems, finalItems);
                    }
                    finally
                    {
                        view.VectorChanged -= handler;
                    }

                    finalItems = new object[] { "B", "C", "A", "nested", "after" };
                    var expected = new[] { new MoveNotification(CollectionChange.ItemInserted, 4, finalItems) };
                    using (var observer = new MoveNotificationObserver(view, () => viewSource.View, source, expected, finalItems, false))
                    {
                        source.Add("after");
                        observer.VerifyComplete();
                    }
                });
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:true}")]
        public void CollectionMovePreservesCallbackFailureAndRecovers()
        {
            WithCollectionMoveChange(true, () =>
            {
                UIExecutor.Execute(() =>
                {
                    const int callbackHResult = unchecked((int)0x8004A503);
                    const string callbackMessage = "CollectionMove original managed callback failure";
                    var source = new ObservableCollection<string>(new[] { "A", "B", "C", "D" });
                    var viewSource = new CollectionViewSource { Source = source };
                    var view = viewSource.View;
                    object[] finalItems;
                    var moveNotifications = CreateMoveNotifications(source.Cast<object>().ToArray(), 0, 3, 1, out finalItems);
                    var recoveryNotifications = new[]
                    {
                        moveNotifications[0],
                        new MoveNotification(CollectionChange.Reset, 0, finalItems)
                    };
                    int failingCallbacks = 0;
                    VectorChangedEventHandler<object> handler = (sender, args) =>
                    {
                        if (args.CollectionChange == CollectionChange.ItemRemoved)
                        {
                            failingCallbacks++;
                            throw new COMException(callbackMessage, callbackHResult);
                        }
                    };

                    using (var observer = new MoveNotificationObserver(view, () => viewSource.View, source, recoveryNotifications, finalItems, false))
                    {
                        view.VectorChanged += handler;
                        try
                        {
                            Exception failure = VerifyMoveFailure(() => source.Move(0, 3), callbackHResult);
                            Verify.IsTrue(failure.Message.Contains(callbackMessage),
                                "The original managed callback message must survive recovery Reset. Actual: " + failure.Message);
                            Verify.AreEqual(1, failingCallbacks);
                            observer.VerifyComplete();
                        }
                        finally
                        {
                            view.VectorChanged -= handler;
                        }
                    }

                    object[] restoredItems;
                    var subsequentNotifications = CreateMoveNotifications(finalItems, 3, 0, 1, out restoredItems);
                    using (var observer = new MoveNotificationObserver(view, () => viewSource.View, source, subsequentNotifications, restoredItems, true))
                    {
                        source.Move(3, 0);
                        observer.VerifyComplete();
                    }
                });
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:true}")]
        public void ListViewMovePreservesUnmovedItemState()
        {
            VerifyMovePreservesUnmovedItemState(false);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:true}")]
        public void GridViewMovePreservesUnmovedItemState()
        {
            VerifyMovePreservesUnmovedItemState(true);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:true}")]
        public void ListViewMoveIntoAndOutOfViewportPreservesUnmovedContainers()
        {
            VerifyMoveAcrossViewport(false);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Data:XamlOptionalChanges", "{CollectionMoveNotifications:true}")]
        public void GridViewMoveIntoAndOutOfViewportPreservesUnmovedContainers()
        {
            VerifyMoveAcrossViewport(true);
        }

        private static string[] MoveTestStrings()
        {
            return new[] { "A", "B", "C", "D", "E", "F" };
        }

        private static string[] MoveTestStringsWithDuplicates()
        {
            return new[] { "repeated", null, new string("repeated".ToCharArray()), "other", null, "repeated" };
        }

        private static ListControl CreateMoveControl(bool useGridView)
        {
            return useGridView ? (ListControl)new GridView() : new ListView();
        }

        private static void WithCollectionMoveChange(bool enabled, Action test)
        {
            var previouslyEnabled = new List<XamlChangeId>();
            bool wasLocked = false;
            UIExecutor.Execute(() =>
            {
                wasLocked = XamlOptionalChanges.IsLocked();
                foreach (XamlChangeId change in Enum.GetValues(typeof(XamlChangeId)))
                {
                    if (XamlOptionalChanges.IsChangeEnabled(change))
                    {
                        previouslyEnabled.Add(change);
                    }
                }
            });

            try
            {
                // Class setup runs before TAEF exposes TestData. Initialize on the test thread
                // so WindowHelper applies Data:XamlOptionalChanges before creating test controls.
                TestServices.WindowHelper.InitializeXaml();
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(enabled, XamlOptionalChanges.IsChangeEnabled(XamlChangeId.CollectionMoveNotifications),
                        "Test defaults and Data:XamlOptionalChanges must configure the Move behavior for this test.");
                    XamlOptionalChanges.Lock();
                    Verify.IsTrue(XamlOptionalChanges.IsLocked());
                });
                test();
            }
            finally
            {
                try
                {
                    TestServices.WindowHelper.ResetWindowContentAndWaitForIdle();
                }
                finally
                {
                    UIExecutor.Execute(() =>
                    {
                        TestServices.Utilities.ResetOptionalChanges();
                        foreach (XamlChangeId change in previouslyEnabled)
                        {
                            XamlOptionalChanges.EnableChange(change);
                        }
                        if (wasLocked)
                        {
                            XamlOptionalChanges.Lock();
                        }
                    });
                }
            }
        }

        private static void VerifyMoveNotificationCase(
            bool useGridView, string[] values, int oldIndex, int newIndex, int count, bool useRange, bool enabled)
        {
            Log.Comment("{0}: Move({1}, {2}, {3}), range={4}, enabled={5}",
                useGridView ? "GridView" : "ListView", oldIndex, newIndex, count, useRange, enabled);
            var source = new MoveRangeCollection<string>(values);
            var control = CreateMoveControl(useGridView);
            control.ItemsSource = source;
            var viewSource = new CollectionViewSource { Source = source };
            object[] finalItems;
            var expected = CreateMoveNotifications(source.Cast<object>().ToArray(), oldIndex, newIndex, count, out finalItems);
            if (!enabled)
            {
                expected = new[] { new MoveNotification(CollectionChange.Reset, 0, finalItems) };
            }

            int sourceNotifications = 0;
            source.CollectionChanged += (sender, args) =>
            {
                sourceNotifications++;
                Verify.AreEqual(NotifyCollectionChangedAction.Move, args.Action, "The adapter must not mutate the original source.");
                Verify.AreEqual(oldIndex, args.OldStartingIndex);
                Verify.AreEqual(newIndex, args.NewStartingIndex);
                Verify.AreEqual(count, args.OldItems.Count);
                Verify.AreEqual(count, args.NewItems.Count);
                VerifyMoveSource(source, finalItems);
            };

            using (var itemObserver = new MoveNotificationObserver(control.Items, () => control.Items, source, expected, finalItems, enabled))
            using (var viewObserver = new MoveNotificationObserver(viewSource.View, () => viewSource.View, source, expected, finalItems, enabled))
            {
                if (useRange)
                {
                    source.MoveRange(oldIndex, newIndex, count);
                }
                else
                {
                    source.Move(oldIndex, newIndex);
                }
                Verify.AreEqual(1, sourceNotifications, "One managed Move must not become additional source mutations.");
                itemObserver.VerifyComplete();
                viewObserver.VerifyComplete();
            }
        }

        private static MoveNotification[] CreateMoveNotifications(
            object[] initialItems, int oldIndex, int newIndex, int count, out object[] finalItems)
        {
            var items = initialItems.ToList();
            var movedItems = items.GetRange(oldIndex, count);
            var notifications = new List<MoveNotification>();
            if (oldIndex != newIndex)
            {
                for (int i = 0; i < count; i++)
                {
                    items.RemoveAt(oldIndex);
                    notifications.Add(new MoveNotification(CollectionChange.ItemRemoved, (uint)oldIndex, items.ToArray()));
                }
                for (int i = 0; i < count; i++)
                {
                    items.Insert(newIndex + i, movedItems[i]);
                    notifications.Add(new MoveNotification(CollectionChange.ItemInserted, (uint)(newIndex + i), items.ToArray()));
                }
            }
            finalItems = items.ToArray();
            return notifications.ToArray();
        }

        private static void VerifyOrdinaryNotification(
            ListControl control, CollectionViewSource viewSource, ObservableCollection<string> source,
            Action mutation, CollectionChange change, uint index, object[] finalItems)
        {
            var expected = new[] { new MoveNotification(change, index, finalItems) };
            using (var itemObserver = new MoveNotificationObserver(control.Items, () => control.Items, source, expected, finalItems, false))
            using (var viewObserver = new MoveNotificationObserver(viewSource.View, () => viewSource.View, source, expected, finalItems, false))
            {
                mutation();
                itemObserver.VerifyComplete();
                viewObserver.VerifyComplete();
            }
        }

        private static void VerifyCollectionMoveOtherNotifications(bool enabled)
        {
            WithCollectionMoveChange(enabled, () =>
            {
                UIExecutor.Execute(() =>
                {
                    foreach (bool useGridView in new[] { false, true })
                    {
                        var source = new ObservableCollection<string>(new[] { "A", "B", "C" });
                        var control = CreateMoveControl(useGridView);
                        control.ItemsSource = source;
                        var viewSource = new CollectionViewSource { Source = source };

                        VerifyOrdinaryNotification(control, viewSource, source, () => source.Insert(1, "inserted"),
                            CollectionChange.ItemInserted, 1, new object[] { "A", "inserted", "B", "C" });
                        VerifyOrdinaryNotification(control, viewSource, source, () => source.RemoveAt(2),
                            CollectionChange.ItemRemoved, 2, new object[] { "A", "inserted", "C" });
                        VerifyOrdinaryNotification(control, viewSource, source, () => source[1] = "replacement",
                            CollectionChange.ItemChanged, 1, new object[] { "A", "replacement", "C" });
                        VerifyOrdinaryNotification(control, viewSource, source, source.Clear,
                            CollectionChange.Reset, 0, new object[0]);
                        VerifyOrdinaryNotification(control, viewSource, source, () => source.Add(null),
                            CollectionChange.ItemInserted, 0, new object[] { null });
                    }
                });
            });
        }

        private static void VerifyMovePreservesUnmovedItemState(bool useGridView)
        {
            WithCollectionMoveChange(true, () =>
            {
                foreach (ListViewSelectionMode mode in new[]
                {
                    ListViewSelectionMode.None, ListViewSelectionMode.Single,
                    ListViewSelectionMode.Multiple, ListViewSelectionMode.Extended
                })
                {
                    Log.Comment("{0}, selection mode {1}", useGridView ? "GridView" : "ListView", mode);
                    var source = new MoveRangeCollection<Person>(
                        Enumerable.Range(0, 6).Select(i => new Person("First " + i, "Last " + i)));
                    Person editedItem = source[2];
                    var selectedItems = mode == ListViewSelectionMode.None ? new Person[0] :
                        mode == ListViewSelectionMode.Single ? new[] { editedItem } : new[] { editedItem, source[3] };
                    ListControl control = null;
                    MoveItemState editedState = null;
                    var reboundItems = new HashSet<Person>();
                    UIExecutor.Execute(() =>
                    {
                        control = CreateMoveControl(useGridView);
                        control.Width = 480;
                        control.Height = 480;
                        control.SelectionMode = mode;
                        control.ShowsScrollingPlaceholders = false;
                        control.ItemTemplate = (DataTemplate)XamlReader.Load(@"
                            <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                          xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                <StackPanel Width='180' Height='64'>
                                    <TextBlock x:Name='MoveLabel' Text='{Binding FirstName, Mode=OneWay}' />
                                    <TextBox x:Name='MoveEditor' Text='{Binding LastName, Mode=TwoWay, UpdateSourceTrigger=Explicit}' />
                                </StackPanel>
                            </DataTemplate>");
                        control.ItemsSource = source;
                        TestServices.WindowHelper.WindowContent = control;
                    });
                    TestServices.WindowHelper.WaitForIdle();
                    TestServicesExtensions.EnsureForegroundWindow();

                    UIExecutor.Execute(() =>
                    {
                        editedState = new MoveItemState(control, editedItem);
                        Verify.AreEqual(editedItem.FirstName, editedState.Label.Text);
                        Verify.AreEqual(editedItem.LastName, editedState.Editor.Text);
                        Verify.IsTrue(editedState.Editor.Focus(FocusState.Keyboard));
                        if (mode == ListViewSelectionMode.Single)
                        {
                            control.SelectedItem = editedItem;
                        }
                        else if (mode != ListViewSelectionMode.None)
                        {
                            control.SelectedItems.Clear();
                            foreach (Person item in selectedItems)
                            {
                                control.SelectedItems.Add(item);
                            }
                        }
                        editedState.Editor.Text = "Uncommitted edit";
                        editedState.Editor.Select(2, 5);
                        Verify.AreEqual("Last 2", editedItem.LastName, "The active edit has not been committed to the source.");
                        Verify.IsNotNull(editedState.Editor.GetBindingExpression(TextBox.TextProperty));
                        control.ContainerContentChanging += (sender, args) =>
                        {
                            if (!args.InRecycleQueue && args.Item is Person item)
                            {
                                reboundItems.Add(item);
                            }
                        };
                    });
                    TestServices.WindowHelper.WaitForIdle();

                    int[,] moves = { { 0, 5, 1 }, { 5, 0, 1 }, { 0, 4, 2 }, { 4, 0, 2 }, { 0, 0, 6 } };
                    for (int i = 0; i < moves.GetLength(0); i++)
                    {
                        int oldIndex = moves[i, 0];
                        int newIndex = moves[i, 1];
                        int count = moves[i, 2];
                        List<MoveItemState> unmovedStates = null;
                        UIExecutor.Execute(() =>
                        {
                            Verify.IsTrue(ReferenceEquals(editedState.Editor, FocusManager.GetFocusedElement(control.XamlRoot)),
                                "The editor must have focus before the collection change.");
                            var movedItems = oldIndex == newIndex ? new Person[0] : source.Skip(oldIndex).Take(count).ToArray();
                            unmovedStates = source.Except(movedItems).Select(item => new MoveItemState(control, item)).ToList();
                            reboundItems.Clear();
                            object[] finalItems;
                            var expected = CreateMoveNotifications(source.Cast<object>().ToArray(), oldIndex, newIndex, count, out finalItems);
                            using (var observer = new MoveNotificationObserver(control.Items, () => control.Items, source, expected, finalItems, true))
                            {
                                if (count == 1)
                                {
                                    source.Move(oldIndex, newIndex);
                                }
                                else
                                {
                                    source.MoveRange(oldIndex, newIndex, count);
                                }
                                observer.VerifyComplete();
                            }
                            control.UpdateLayout();
                        });
                        TestServices.WindowHelper.WaitForIdle();
                        UIExecutor.Execute(() =>
                        {
                            foreach (MoveItemState state in unmovedStates)
                            {
                                state.VerifyUnchanged(control);
                                Verify.IsFalse(reboundItems.Contains(state.Item), "An unmoved item must not be rebound.");
                            }
                            Verify.AreEqual(selectedItems.Length, control.SelectedItems.Count);
                            foreach (Person item in selectedItems)
                            {
                                Verify.IsTrue(control.SelectedItems.Contains(item), "An unmoved item lost its selection.");
                            }
                            Verify.AreEqual(selectedItems.Length == 0 ? -1 : source.IndexOf(selectedItems[0]), control.SelectedIndex);
                            Verify.IsTrue(ReferenceEquals(editedState.Editor, FocusManager.GetFocusedElement(control.XamlRoot)),
                                "Adjusting an unchanged selected item's index must not transfer focus away from its editor.");
                            Verify.AreEqual("Uncommitted edit", editedState.Editor.Text);
                            Verify.AreEqual(2, editedState.Editor.SelectionStart);
                            Verify.AreEqual(5, editedState.Editor.SelectionLength);
                            Verify.AreEqual("Last 2", editedItem.LastName);
                        });
                    }

                    UIExecutor.Execute(() =>
                    {
                        editedItem.FirstName = "Updated after Move";
                        editedState.Editor.GetBindingExpression(TextBox.TextProperty).UpdateSource();
                        Verify.AreEqual("Uncommitted edit", editedItem.LastName);
                    });
                    TestServices.WindowHelper.WaitForIdle();
                    UIExecutor.Execute(() =>
                    {
                        Verify.AreEqual("Updated after Move", editedState.Label.Text);
                        if (mode == ListViewSelectionMode.Single)
                        {
                            Person newSelection = source[4];
                            control.SelectedItem = newSelection;
                            Verify.IsTrue(ReferenceEquals(control.ContainerFromItem(newSelection), FocusManager.GetFocusedElement(control.XamlRoot)),
                                "Selecting a different item must still move focus to its container.");
                        }
                    });
                    TestServices.WindowHelper.ResetWindowContentAndWaitForIdle();
                }
            });
        }

        private static void VerifyMoveAcrossViewport(bool useGridView)
        {
            WithCollectionMoveChange(true, () =>
            {
                var source = new ObservableCollection<string>(Enumerable.Range(0, 200).Select(i => "Item " + i));
                string movedItem = source[0];
                string anchorItem = source[1];
                ListControl control = null;
                ContentControl anchorContainer = null;
                object anchorDataContext = null;
                UIExecutor.Execute(() =>
                {
                    control = CreateMoveControl(useGridView);
                    control.Width = 240;
                    control.Height = 200;
                    control.ShowsScrollingPlaceholders = false;
                    control.ItemTemplate = (DataTemplate)XamlReader.Load(@"
                        <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <TextBlock Width='80' Height='32' Text='{Binding}' />
                        </DataTemplate>");
                    control.ItemsSource = source;
                    TestServices.WindowHelper.WindowContent = control;
                });
                TestServices.WindowHelper.WaitForIdle();
                UIExecutor.Execute(() =>
                {
                    if (useGridView)
                    {
                        var panel = control.ItemsPanelRoot as ItemsWrapGrid;
                        Verify.IsNotNull(panel);
                        panel.CacheLength = 0;
                    }
                    else
                    {
                        var panel = control.ItemsPanelRoot as ItemsStackPanel;
                        Verify.IsNotNull(panel);
                        panel.CacheLength = 0;
                    }
                    control.ScrollIntoView(movedItem, ScrollIntoViewAlignment.Leading);
                    control.UpdateLayout();
                    control.SelectedItem = anchorItem;
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServicesExtensions.EnsureForegroundWindow();
                UIExecutor.Execute(() =>
                {
                    anchorContainer = (ContentControl)control.ContainerFromItem(anchorItem);
                    Verify.IsNotNull(anchorContainer);
                    Verify.AreEqual(anchorItem, anchorContainer.Content);
                    anchorDataContext = anchorContainer.DataContext;
                    Verify.IsNotNull(control.ContainerFromItem(movedItem));
                    Verify.IsNull(control.ContainerFromIndex(source.Count - 1), "The endpoint must start outside the realized viewport.");
                    Verify.IsTrue(anchorContainer.Focus(FocusState.Keyboard));
                });
                TestServices.WindowHelper.WaitForIdle();

                int[,] moves = { { 0, 199 }, { 199, 2 } };
                for (int i = 0; i < moves.GetLength(0); i++)
                {
                    int oldIndex = moves[i, 0];
                    int newIndex = moves[i, 1];
                    UIExecutor.Execute(() =>
                    {
                        object[] finalItems;
                        var expected = CreateMoveNotifications(source.Cast<object>().ToArray(), oldIndex, newIndex, 1, out finalItems);
                        using (var observer = new MoveNotificationObserver(control.Items, () => control.Items, source, expected, finalItems, true))
                        {
                            source.Move(oldIndex, newIndex);
                            observer.VerifyComplete();
                        }
                        control.UpdateLayout();
                    });
                    TestServices.WindowHelper.WaitForIdle();
                    UIExecutor.Execute(() =>
                    {
                        Verify.IsTrue(ReferenceEquals(anchorContainer, control.ContainerFromItem(anchorItem)));
                        Verify.IsTrue(ReferenceEquals(anchorContainer, control.ContainerFromIndex(source.IndexOf(anchorItem))));
                        Verify.AreEqual(anchorItem, anchorContainer.Content);
                        Verify.AreEqual(anchorDataContext, anchorContainer.DataContext, "An unmoved container's inherited DataContext changed.");
                        Verify.AreEqual(anchorItem, control.SelectedItem);
                        Verify.IsTrue(ReferenceEquals(anchorContainer, FocusManager.GetFocusedElement(control.XamlRoot)));
                        if (newIndex == source.Count - 1)
                        {
                            Verify.IsNull(control.ContainerFromItem(movedItem), "The moved item left the realized viewport.");
                        }
                        else
                        {
                            var movedContainer = (ContentControl)control.ContainerFromItem(movedItem);
                            Verify.IsNotNull(movedContainer, "The moved item entered the realized viewport.");
                            Verify.AreEqual(movedItem, movedContainer.Content);
                        }
                    });
                }
            });
        }

        private static void VerifyMoveSource(IList source, object[] expected)
        {
            Verify.AreEqual(expected.Length, source.Count, "The source must already be in its final state during every notification.");
            for (int i = 0; i < expected.Length; i++)
            {
                Verify.AreEqual(expected[i], source[i], "Unexpected source item at " + i);
            }
        }

        private static Exception VerifyMoveFailure(Action action, int expectedHResult)
        {
            Exception failure = null;
            try
            {
                action();
            }
            catch (Exception exception)
            {
                failure = exception;
            }
            Verify.IsNotNull(failure, "The operation must fail while a Move notification is being processed.");
            Verify.AreEqual(expectedHResult, failure.HResult);
            return failure;
        }

        private static void VerifyMoveItems(IList<object> items, object[] expected, object[] probes)
        {
            Verify.AreEqual(expected.Length, items.Count, "The projected Count must describe the current notification.");
            for (int i = 0; i < expected.Length; i++)
            {
                Verify.AreEqual(expected[i], items[i], "Unexpected projected item at " + i);
            }
            foreach (object probe in probes)
            {
                Verify.AreEqual(Array.IndexOf(expected, probe), items.IndexOf(probe), "IndexOf must use the same intermediate sequence.");
            }
            using (var enumerator = items.GetEnumerator())
            {
                VerifyMoveEnumerator(enumerator, expected);
            }
        }

        private static void VerifyMoveEnumerator(IEnumerator<object> enumerator, object[] expected)
        {
            for (int i = 0; i < expected.Length; i++)
            {
                Verify.IsTrue(enumerator.MoveNext(), "Enumeration ended before index " + i);
                Verify.AreEqual(expected[i], enumerator.Current, "Unexpected enumerated item at " + i);
            }
            Verify.IsFalse(enumerator.MoveNext(), "Enumeration must not expose items beyond the projected Count.");
        }

        private sealed class MoveNotification
        {
            public readonly CollectionChange Change;
            public readonly uint Index;
            public readonly object[] Items;

            public MoveNotification(CollectionChange change, uint index, object[] items)
            {
                Change = change;
                Index = index;
                Items = items;
            }
        }

        private sealed class MoveNotificationObserver : IDisposable
        {
            private readonly IObservableVector<object> vector;
            private readonly Func<IList<object>> getCurrentView;
            private readonly IList source;
            private readonly MoveNotification[] expected;
            private readonly object[] finalItems;
            private readonly object[] probes;
            private readonly List<IEnumerator<object>> priorEnumerators = new List<IEnumerator<object>>();
            private IEnumerator<object> notificationEnumerator;
            private int notificationCount;

            public MoveNotificationObserver(
                IObservableVector<object> vector, Func<IList<object>> getCurrentView, IList source,
                MoveNotification[] expected, object[] finalItems, bool checkPriorEnumerators)
            {
                this.vector = vector;
                this.getCurrentView = getCurrentView;
                this.source = source;
                this.expected = expected;
                this.finalItems = finalItems;
                probes = source.Cast<object>().Concat(finalItems).Concat(new object[] { null, "not in the source" })
                    .Distinct().Select(value => value is string text ? new string(text.ToCharArray()) : value).ToArray();
                if (checkPriorEnumerators)
                {
                    // Keep unadvanced iterators for every callback and for completion. These must
                    // read the adapted sequence, not a stale iterator over the managed source.
                    for (int i = 0; i <= expected.Length; i++)
                    {
                        priorEnumerators.Add(((IList<object>)vector).GetEnumerator());
                    }
                }
                vector.VectorChanged += OnVectorChanged;
            }

            private void OnVectorChanged(IObservableVector<object> sender, IVectorChangedEventArgs args)
            {
                Verify.IsTrue(notificationCount < expected.Length, "Unexpected extra vector notification (including a Reset for a no-op).");
                var notification = expected[notificationCount];
                Verify.AreEqual(notification.Change, args.CollectionChange, "An opted-in Move must not produce Reset.");
                Verify.AreEqual(notification.Index, args.Index);
                Verify.AreEqual(vector, sender);
                VerifyMoveSource(source, finalItems);
                VerifyMoveItems((IList<object>)vector, notification.Items, probes);
                VerifyMoveItems(getCurrentView(), notification.Items, probes);
                if (priorEnumerators.Count != 0)
                {
                    VerifyMoveEnumerator(priorEnumerators[notificationCount], notification.Items);
                    if (notificationEnumerator != null)
                    {
                        VerifyMoveEnumerator(notificationEnumerator, notification.Items);
                        notificationEnumerator.Dispose();
                    }
                    notificationEnumerator = ((IList<object>)vector).GetEnumerator();
                }
                notificationCount++;
            }

            public void VerifyComplete()
            {
                Verify.AreEqual(expected.Length, notificationCount, "Missing vector notifications.");
                VerifyMoveItems((IList<object>)vector, finalItems, probes);
                VerifyMoveItems(getCurrentView(), finalItems, probes);
                VerifyMoveSource(source, finalItems);
                if (priorEnumerators.Count != 0)
                {
                    VerifyMoveEnumerator(priorEnumerators[expected.Length], finalItems);
                }
                if (notificationEnumerator != null)
                {
                    VerifyMoveEnumerator(notificationEnumerator, finalItems);
                    notificationEnumerator.Dispose();
                    notificationEnumerator = null;
                }
            }

            public void Dispose()
            {
                vector.VectorChanged -= OnVectorChanged;
                foreach (var enumerator in priorEnumerators)
                {
                    enumerator.Dispose();
                }
                notificationEnumerator?.Dispose();
            }
        }

        private sealed class MoveItemState
        {
            public readonly Person Item;
            public readonly ContentControl Container;
            public readonly TextBlock Label;
            public readonly TextBox Editor;
            private readonly BindingExpression labelBinding;
            private readonly BindingExpression editorBinding;
            private readonly object containerDataContext;
            private readonly string labelText;
            private readonly string editorText;
            private readonly int selectionStart;
            private readonly int selectionLength;

            public MoveItemState(ListControl control, Person item)
            {
                Item = item;
                Container = (ContentControl)control.ContainerFromItem(item);
                Verify.IsNotNull(Container, "All six test items must be realized.");
                var root = (FrameworkElement)Container.ContentTemplateRoot;
                Verify.IsNotNull(root);
                Label = (TextBlock)root.FindName("MoveLabel");
                Editor = (TextBox)root.FindName("MoveEditor");
                Verify.IsNotNull(Label);
                Verify.IsNotNull(Editor);
                labelBinding = Label.GetBindingExpression(TextBlock.TextProperty);
                editorBinding = Editor.GetBindingExpression(TextBox.TextProperty);
                Verify.IsNotNull(labelBinding);
                Verify.IsNotNull(editorBinding);
                labelText = Label.Text;
                editorText = Editor.Text;
                selectionStart = Editor.SelectionStart;
                selectionLength = Editor.SelectionLength;
                containerDataContext = Container.DataContext;
                // Generated containers hold the item in Content; template children bind to that item.
                Verify.IsTrue(ReferenceEquals(item, Container.Content));
                Verify.IsTrue(ReferenceEquals(item, Label.DataContext));
                Verify.IsTrue(ReferenceEquals(item, Editor.DataContext));
            }

            public void VerifyUnchanged(ListControl control)
            {
                var current = new MoveItemState(control, Item);
                Verify.IsTrue(ReferenceEquals(Container, current.Container), "An unmoved item's container was replaced.");
                Verify.IsTrue(ReferenceEquals(Label, current.Label));
                Verify.IsTrue(ReferenceEquals(Editor, current.Editor));
                Verify.AreEqual(containerDataContext, current.containerDataContext, "An unmoved container's inherited DataContext changed.");
                Verify.AreEqual(labelBinding, current.labelBinding, "The unmoved item's label binding was replaced.");
                Verify.AreEqual(editorBinding, current.editorBinding, "The unmoved item's editor binding was replaced.");
                Verify.AreEqual(labelText, current.labelText);
                Verify.AreEqual(editorText, current.editorText);
                Verify.AreEqual(selectionStart, current.selectionStart);
                Verify.AreEqual(selectionLength, current.selectionLength);
            }
        }

        private sealed class MoveRangeCollection<T> : ObservableCollection<T>
        {
            public MoveRangeCollection(IEnumerable<T> items) : base(items)
            {
            }

            public void MoveRange(int oldIndex, int newIndex, int count)
            {
                if (count <= 0 || oldIndex < 0 || oldIndex > Count - count || newIndex < 0 || newIndex > Count - count)
                {
                    throw new ArgumentOutOfRangeException();
                }
                CheckReentrancy();
                var movedItems = new List<T>();
                for (int i = 0; i < count; i++)
                {
                    movedItems.Add(Items[oldIndex]);
                    Items.RemoveAt(oldIndex);
                }
                for (int i = 0; i < count; i++)
                {
                    Items.Insert(newIndex + i, movedItems[i]);
                }
                OnPropertyChanged(new PropertyChangedEventArgs("Item[]"));
                OnCollectionChanged(new NotifyCollectionChangedEventArgs(
                    NotifyCollectionChangedAction.Move, (IList)movedItems, newIndex, oldIndex));
            }
        }

        private sealed class MoveReadTrackingSource : IList, INotifyCollectionChanged
        {
            private readonly List<string> items;
            public int IndexedReadCount { get; private set; }
            public int EnumeratorCount { get; private set; }
            public int CopyCount { get; private set; }
            public Action OnIndexedRead { get; set; }
            public event NotifyCollectionChangedEventHandler CollectionChanged;

            public MoveReadTrackingSource(int count)
            {
                items = Enumerable.Range(0, count).Select(i => "Item " + i).ToList();
            }

            public void ResetReadCounts()
            {
                IndexedReadCount = 0;
                EnumeratorCount = 0;
                CopyCount = 0;
            }

            public void MoveRange(int oldIndex, int newIndex, int count)
            {
                var movedItems = items.GetRange(oldIndex, count);
                items.RemoveRange(oldIndex, count);
                items.InsertRange(newIndex, movedItems);
                CollectionChanged?.Invoke(this, new NotifyCollectionChangedEventArgs(
                    NotifyCollectionChangedAction.Move, (IList)movedItems, newIndex, oldIndex));
            }

            public object this[int index]
            {
                get
                {
                    IndexedReadCount++;
                    var item = items[index];
                    OnIndexedRead?.Invoke();
                    return item;
                }
                set { throw new NotSupportedException(); }
            }

            public int Count => items.Count;
            public bool IsReadOnly => true;
            public bool IsFixedSize => true;
            public bool IsSynchronized => false;
            public object SyncRoot => this;
            public int IndexOf(object value) => ((IList)items).IndexOf(value);
            public bool Contains(object value) => ((IList)items).Contains(value);
            public int Add(object value) => throw new NotSupportedException();
            public void Clear() => throw new NotSupportedException();
            public void Insert(int index, object value) => throw new NotSupportedException();
            public void Remove(object value) => throw new NotSupportedException();
            public void RemoveAt(int index) => throw new NotSupportedException();

            public void CopyTo(Array array, int index)
            {
                CopyCount++;
                ((IList)items).CopyTo(array, index);
            }

            public IEnumerator GetEnumerator()
            {
                EnumeratorCount++;
                return items.GetEnumerator();
            }
        }
    }
}
