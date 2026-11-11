// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Controls.ListViewBase
{
    // Tests for the experimental RowContext attached properties that ListView maintains on ListViewItem.
    public partial class ListViewBaseTests
    {
        private ListView CreateAndLoadRowContextList(Action<ListView> configure)
        {
            ListView list = null;
            var loaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = new ListView { Height = 300 };
                configure(list);
                list.Loaded += (s, e) => loaded.Set();
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(loaded.WaitOne(TimeSpan.FromSeconds(10)), "List failed to raise Loaded event.");
            TestServices.WindowHelper.WaitForIdle();
            return list;
        }

        private static void VerifyRowContext(
            ListView list,
            int index,
            NeighborRelation previousRelation,
            NeighborRelation nextRelation,
            bool isPreviousSelected,
            bool isNextSelected)
        {
            var container = (ListViewItem)list.ContainerFromIndex(index);
            Verify.IsNotNull(container, "Container " + index + " should be realized.");

            string description = "item " + index;
            Verify.AreEqual(previousRelation, RowContext.GetPreviousRelation(container), "PreviousRelation of " + description);
            Verify.AreEqual(nextRelation, RowContext.GetNextRelation(container), "NextRelation of " + description);
            Verify.AreEqual(isPreviousSelected, RowContext.GetIsPreviousSelected(container), "IsPreviousSelected of " + description);
            Verify.AreEqual(isNextSelected, RowContext.GetIsNextSelected(container), "IsNextSelected of " + description);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void RowContextReportsNeighborsInFlatList()
        {
            var list = CreateAndLoadRowContextList(l =>
            {
                l.SelectionMode = ListViewSelectionMode.Multiple;
                l.ItemsSource = Enumerable.Range(0, 5).Select(i => "Item " + i).ToList();
            });

            UIExecutor.Execute(() =>
            {
                // Nothing selected: neighbors exist but none is selected. Ends have no neighbor.
                VerifyRowContext(list, 0, NeighborRelation.None, NeighborRelation.SameGroup, false, false);
                VerifyRowContext(list, 2, NeighborRelation.SameGroup, NeighborRelation.SameGroup, false, false);
                VerifyRowContext(list, 4, NeighborRelation.SameGroup, NeighborRelation.None, false, false);

                list.SelectedItems.Add(list.Items[1]);
                list.SelectedItems.Add(list.Items[2]);
                list.SelectedItems.Add(list.Items[4]);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                VerifyRowContext(list, 0, NeighborRelation.None, NeighborRelation.SameGroup, false, true);
                VerifyRowContext(list, 1, NeighborRelation.SameGroup, NeighborRelation.SameGroup, false, true);
                VerifyRowContext(list, 2, NeighborRelation.SameGroup, NeighborRelation.SameGroup, true, false);
                VerifyRowContext(list, 3, NeighborRelation.SameGroup, NeighborRelation.SameGroup, true, true);
                VerifyRowContext(list, 4, NeighborRelation.SameGroup, NeighborRelation.None, false, false);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void RowContextIsCurrentWhenSelectionChangedIsRaised()
        {
            var list = CreateAndLoadRowContextList(l =>
            {
                l.SelectionMode = ListViewSelectionMode.Multiple;
                l.ItemsSource = Enumerable.Range(0, 4).Select(i => "Item " + i).ToList();
            });

            bool handlerRan = false;
            bool verifyInHandler = false;

            UIExecutor.Execute(() =>
            {
                list.SelectionChanged += (s, e) =>
                {
                    if (!verifyInHandler)
                    {
                        return;
                    }

                    handlerRan = true;
                    VerifyRowContext(list, 0, NeighborRelation.None, NeighborRelation.SameGroup, false, true);
                    VerifyRowContext(list, 2, NeighborRelation.SameGroup, NeighborRelation.SameGroup, true, false);
                };

                verifyInHandler = true;
                list.SelectedItems.Add(list.Items[1]);
                verifyInHandler = false;
            });

            Verify.IsTrue(handlerRan, "SelectionChanged should have been raised.");

            UIExecutor.Execute(() =>
            {
                list.SelectedItems.Clear();
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                VerifyRowContext(list, 0, NeighborRelation.None, NeighborRelation.SameGroup, false, false);
                VerifyRowContext(list, 2, NeighborRelation.SameGroup, NeighborRelation.SameGroup, false, false);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void RowContextUsesSelectionOfUnrealizedNeighbor()
        {
            var list = CreateAndLoadRowContextList(l =>
            {
                l.SelectionMode = ListViewSelectionMode.Multiple;
                l.ItemsSource = Enumerable.Range(0, 1000).Select(i => "Item " + i).ToList();
            });

            int lastRealizedIndex = -1;

            UIExecutor.Execute(() =>
            {
                for (int i = 0; i < 1000 && list.ContainerFromIndex(i) != null; i++)
                {
                    lastRealizedIndex = i;
                }

                Verify.IsGreaterThan(lastRealizedIndex, 0, "Some containers should be realized.");
                Verify.IsLessThan(lastRealizedIndex, 999, "Virtualization should leave containers unrealized.");

                // Selecting by range does not realize or scroll to the item.
                list.SelectRange(new ItemIndexRange(lastRealizedIndex + 1, 1));
                Verify.IsNull(list.ContainerFromIndex(lastRealizedIndex + 1), "The selected neighbor must stay unrealized.");

                VerifyRowContext(list, lastRealizedIndex, NeighborRelation.SameGroup, NeighborRelation.SameGroup, false, true);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void RowContextUpdatesWhenItemsAreInsertedAndRemoved()
        {
            var source = new ObservableCollection<string>(Enumerable.Range(0, 4).Select(i => "Item " + i));
            var list = CreateAndLoadRowContextList(l =>
            {
                l.SelectionMode = ListViewSelectionMode.Multiple;
                l.ItemsSource = source;
            });

            UIExecutor.Execute(() =>
            {
                list.SelectedItems.Add(source[0]);
                list.SelectedItems.Add(source[1]);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                VerifyRowContext(list, 0, NeighborRelation.None, NeighborRelation.SameGroup, false, true);

                // An unselected item between the selected ones separates them.
                source.Insert(1, "Gap");
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                VerifyRowContext(list, 0, NeighborRelation.None, NeighborRelation.SameGroup, false, false);
                VerifyRowContext(list, 1, NeighborRelation.SameGroup, NeighborRelation.SameGroup, true, true);
                VerifyRowContext(list, 2, NeighborRelation.SameGroup, NeighborRelation.SameGroup, false, false);

                source.RemoveAt(1);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                VerifyRowContext(list, 0, NeighborRelation.None, NeighborRelation.SameGroup, false, true);
                VerifyRowContext(list, 1, NeighborRelation.SameGroup, NeighborRelation.SameGroup, true, false);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void RowContextReportsGroupRelations()
        {
            var groups = GetGroupedData(numGroups: 2, itemsPerGroup: 3);

            var list = CreateAndLoadRowContextList(l =>
            {
                l.SelectionMode = ListViewSelectionMode.Multiple;
                l.GroupStyle.Add((GroupStyle)XamlReader.Load(
                    "<GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                    "<GroupStyle.HeaderTemplate><DataTemplate><TextBlock Text='{Binding}' /></DataTemplate></GroupStyle.HeaderTemplate>" +
                    "</GroupStyle>"));
                l.ItemsSource = new CollectionViewSource { IsSourceGrouped = true, Source = groups }.View;
            });

            UIExecutor.Execute(() =>
            {
                // Select the last item of the first group and the first item of the second group.
                list.SelectRange(new ItemIndexRange(2, 2));
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                VerifyRowContext(list, 0, NeighborRelation.None, NeighborRelation.SameGroup, false, false);
                VerifyRowContext(list, 2, NeighborRelation.SameGroup, NeighborRelation.OtherGroup, false, true);
                VerifyRowContext(list, 3, NeighborRelation.OtherGroup, NeighborRelation.SameGroup, true, false);
                VerifyRowContext(list, 5, NeighborRelation.SameGroup, NeighborRelation.None, false, false);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void RowContextKeepsDefaultsInUnsupportedLayout()
        {
            var list = CreateAndLoadRowContextList(l =>
            {
                l.SelectionMode = ListViewSelectionMode.Multiple;
                l.ItemsPanel = (ItemsPanelTemplate)XamlReader.Load(
                    "<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                    "<ItemsStackPanel Orientation='Horizontal' /></ItemsPanelTemplate>");
                l.ItemsSource = Enumerable.Range(0, 4).Select(i => "Item " + i).ToList();
            });

            UIExecutor.Execute(() =>
            {
                list.SelectedItems.Add(list.Items[1]);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                VerifyRowContext(list, 0, NeighborRelation.None, NeighborRelation.None, false, false);
                VerifyRowContext(list, 2, NeighborRelation.None, NeighborRelation.None, false, false);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void RowContextCannotBeSetByApps()
        {
            var list = CreateAndLoadRowContextList(l =>
            {
                l.ItemsSource = Enumerable.Range(0, 3).Select(i => "Item " + i).ToList();
            });

            UIExecutor.Execute(() =>
            {
                var container = (ListViewItem)list.ContainerFromIndex(1);
                Verify.Throws<Exception>(() => container.SetValue(RowContext.PreviousRelationProperty, NeighborRelation.None),
                    "RowContext properties are read-only.");
                Verify.Throws<Exception>(() => container.SetValue(RowContext.IsNextSelectedProperty, true),
                    "RowContext properties are read-only.");
            });
        }
    }
}
