// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System.Collections.Generic;
using System.Linq;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewRowTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewRecyclingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // An open edit surviving a recycle needs BeginEdit, which has no API, so the interaction tests cover it.
    [TestClass]
    public class TableViewGroupHeaderRecyclingTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies a group header that was pooled before the template changed shows the current template when it is realized again.")]
        public void VerifyPooledGroupHeaderShowsTheCurrentTemplateAfterRecycling()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateScrollableGroupedTable();
                tableView.GroupHeaderTemplate = CreateRecyclingMarkerTemplate(OldMarker);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var markers = ProjectedGroupHeaderTexts(tableView);
                Verify.IsGreaterThan(markers.Count, 0, "Precondition: group headers must realize before scrolling.");
                Verify.IsTrue(markers.All(text => text.StartsWith(OldMarker)),
                    $"Precondition: every header must start on the first template. Saw: {Describe(markers)}.");
            });

            ScrollToEnd(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(ProjectedGroupHeaderTexts(tableView).Contains(OldMarker + FirstGroupKey),
                    "Precondition: the first group's header must have left realization, so its container is pooled.");

                // Swapped while the first header is pooled; the contract is that it is re-prepared on its way back in.
                tableView.GroupHeaderTemplate = CreateRecyclingMarkerTemplate(NewMarker);
            });

            SettleLayout(tableView);

            ScrollToStart(tableView);

            RunOnUIThread.Execute(() =>
            {
                var markers = ProjectedGroupHeaderTexts(tableView);
                Verify.IsGreaterThan(markers.Count, 0, "Headers must realize again after scrolling back.");

                Verify.IsFalse(markers.Any(text => text.StartsWith(OldMarker)),
                    $"No header may resurface with the replaced template. Saw: {Describe(markers)}.");

                Verify.IsTrue(markers.All(text => text.StartsWith(NewMarker)),
                    $"Every realized header must carry the current template. Saw: {Describe(markers)}.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies collapsed group state is rebuilt from the group rather than from the container when headers are recycled.")]
        public void VerifyCollapsedGroupStateSurvivesHeaderRecycling()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateScrollableGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsGreaterThan(ProjectedRowCount(tableView), 0, "Precondition: data rows must be projected before collapsing.");

                tableView.CollapseAllGroups();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual(0, ProjectedRowCount(tableView), "Precondition: collapsing every group must leave only headers projected."));

            ScrollToEnd(tableView);
            ScrollToStart(tableView);

            RunOnUIThread.Execute(() =>
            {
                var headers = ProjectedGroupHeaders(tableView);
                Verify.IsGreaterThan(headers.Count, 0, "Headers must realize again after scrolling back.");

                foreach (var header in headers)
                {
                    Verify.IsFalse(header.IsExpanded,
                        "A recycled group header must report the collapsed state of the group it now represents.");
                }

                Verify.AreEqual(0, ProjectedRowCount(tableView),
                    "No data row may reappear after recycling a collapsed group's header.");
            });
        }
    }

    // Automation peers must describe the item a recycled container now hosts.
    [TestClass]
    public class TableViewAutomationRecyclingTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies a recycled row's automation peer names the item the row now represents.")]
        public void VerifyRecycledRowPeerNamesTheNewItem()
        {
            TableView tableView = null;
            List<Person> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeManyItems(RecyclingSourceCount);
                tableView = CreateScrollableTable(items);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.IsTrue(RealizedProjectedRows(tableView).Any(row => ReferenceEquals(row.DataContext, items[0])),
                    "Precondition: the first item must be realized before scrolling."));

            ScrollToEnd(tableView);

            RunOnUIThread.Execute(() =>
            {
                var rows = RealizedProjectedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 0, "Rows must still be realized after scrolling.");

                Verify.IsFalse(rows.Any(row => ReferenceEquals(row.DataContext, items[0])),
                    "Precondition: the first item must have left realization, so its container was recycled.");

                foreach (var row in rows)
                {
                    var item = row.DataContext as Person;
                    Verify.IsNotNull(item, "Every realized row must carry a data item.");

                    var name = GetRowPeer(row).GetName();
                    Log.Comment($"Row hosting '{item.Name}' reports name '{name}'.");

                    Verify.IsTrue(name != null && name.Contains(item.Name),
                        $"A recycled row must be named for the item it now hosts; '{name}' does not mention '{item.Name}'.");

                    Verify.IsFalse(name.Contains(items[0].Name),
                        $"A recycled row must not still name the item it previously hosted; saw '{name}'.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a recycled cell's automation peer reports the value of the item its row now represents.")]
        public void VerifyRecycledCellPeerReportsTheNewItemsValue()
        {
            TableView tableView = null;
            List<Person> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeManyItems(RecyclingSourceCount);
                tableView = CreateScrollableTable(items);
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            ScrollToEnd(tableView);

            RunOnUIThread.Execute(() =>
            {
                var rows = RealizedProjectedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 0, "Rows must still be realized after scrolling.");

                Verify.IsFalse(rows.Any(row => ReferenceEquals(row.DataContext, items[0])),
                    "Precondition: the first item must have left realization.");

                foreach (var row in rows)
                {
                    var item = row.DataContext as Person;
                    Verify.IsNotNull(item, "Every realized row must carry a data item.");

                    var cellPeer = GetCellPeer(GetRowPeer(row), 0);
                    Verify.IsNotNull(cellPeer, "Every realized row must expose a peer for its first visible cell.");

                    var name = cellPeer.GetName();
                    Verify.IsTrue(name != null && name.Contains(item.Name),
                        $"A recycled cell must report the current item's value; '{name}' does not mention '{item.Name}'.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a recycled row reports the set position of the index it now occupies rather than the one it was prepared for.")]
        public void VerifyRecycledRowPeerReportsTheNewPositionInSet()
        {
            TableView tableView = null;
            List<Person> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeManyItems(RecyclingSourceCount);
                tableView = CreateScrollableTable(items);
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            ScrollToEnd(tableView);

            RunOnUIThread.Execute(() =>
            {
                var projected = GetProjectedElements(tableView, requireAllRealized: false);
                var rows = RealizedProjectedRows(tableView);

                Verify.IsGreaterThan(rows.Count, 0, "Rows must still be realized after scrolling.");

                var checkedAny = false;

                foreach (var row in rows)
                {
                    var index = projected.IndexOf(row);

                    if (index < 0)
                    {
                        continue;
                    }

                    checkedAny = true;

                    var position = GetRowPeer(row).GetPositionInSet();
                    Log.Comment($"Row at projected index {index} reports PositionInSet {position}.");

                    Verify.AreEqual(index + 1, position,
                        $"A recycled row at projected index {index} must report PositionInSet {index + 1}.");
                }

                Verify.IsTrue(checkedAny, "Precondition: at least one realized row must be locatable in the projection.");
            });
        }
    }

    // Grouping and sorting must not defeat virtualization.
    [TestClass]
    public class TableViewVirtualizationAcrossFeaturesTests : TableViewApiTestBase
    {
        private const int ManyGroupsItemCount = 2000;
        private const int ItemsPerGroup = 5;
        private const int ManyGroupsCount = ManyGroupsItemCount / ItemsPerGroup;

        [TestMethod]
        [TestProperty("Description", "Verifies a grouped source with hundreds of groups realizes only a viewport-bounded set of headers and rows at every scroll offset.")]
        public void VerifyGroupedProjectionRealizesBoundedRowsAcrossManyGroups()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateManyGroupsTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            var bound = 0;
            var scrollable = 0.0;

            RunOnUIThread.Execute(() =>
            {
                bound = MixedRealizationBound(tableView);
                scrollable = GetBodyScroller(tableView)?.ScrollableHeight ?? 0.0;

                Verify.AreEqual(ManyGroupsItemCount + ManyGroupsCount, GetProjectedCount(tableView),
                    "Precondition: the projection must hold one header per group plus every data row.");
                Verify.IsLessThan(bound, GetProjectedCount(tableView),
                    "Precondition: the projection must be far larger than the bound, or the bound proves nothing.");

                VerifyGroupedRealizationBounded(tableView, bound, "at the start", expectRows: true);
            });

            foreach (var fraction in new[] { 1.0 / 3.0, 2.0 / 3.0 })
            {
                ScrollBodyTo(tableView, ScrollAxis.Vertical, scrollable * fraction);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNull(GetProjectedElements(tableView, requireAllRealized: false)[0],
                        $"Precondition: the first group header must have left realization at {fraction:P0}.");

                    VerifyGroupedRealizationBounded(tableView, bound, $"at {fraction:P0}", expectRows: true);
                });
            }

            ScrollToEnd(tableView);

            RunOnUIThread.Execute(() => VerifyGroupedRealizationBounded(tableView, bound, "at the end", expectRows: true));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies collapsing every group of a large grouped source realizes headers only, at every scroll offset.")]
        public void VerifyCollapsedGroupsRealizeNoDataRows()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateManyGroupsTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsGreaterThan(ProjectedRowCount(tableView), 0, "Precondition: data rows must be realized before collapsing.");

                tableView.CollapseAllGroups();
            });

            SettleLayout(tableView);

            var bound = 0;
            var scrollable = 0.0;

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(ManyGroupsCount, GetProjectedCount(tableView),
                    "A full collapse must project exactly one header per group.");

                bound = MixedRealizationBound(tableView);
                scrollable = GetBodyScroller(tableView)?.ScrollableHeight ?? 0.0;

                Verify.IsLessThan(bound, ManyGroupsCount,
                    "Precondition: the collapsed projection must still be far larger than the bound, so headers themselves virtualize.");

                VerifyGroupedRealizationBounded(tableView, bound, "collapsed, at the start", expectRows: false);
            });

            ScrollBodyTo(tableView, ScrollAxis.Vertical, scrollable / 2.0);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(GetProjectedElements(tableView, requireAllRealized: false)[0],
                    "Precondition: the first group header must have left realization.");

                VerifyGroupedRealizationBounded(tableView, bound, "collapsed, halfway", expectRows: false);
            });

            ScrollToEnd(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyGroupedRealizationBounded(tableView, bound, "collapsed, at the end", expectRows: false);

                foreach (var header in ProjectedGroupHeaders(tableView))
                {
                    Verify.IsFalse(header.IsExpanded, "Every header realized after scrolling must report its group collapsed.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a long hop-scroll over a sorted large source keeps realization and the container pool bounded and every realized row in sorted position.")]
        public void VerifyScrollingUnderActiveSortKeepsRealizationBounded()
        {
            TableView tableView = null;
            List<Person> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeManyItems(TableViewVirtualizationTestHelpers.LargeSourceCount);

                var indexOf = new Dictionary<Person, int>();
                for (var i = 0; i < items.Count; i++)
                {
                    indexOf[items[i]] = i;
                }

                var source = TableViewSource.From(items).Sort(o => indexOf[(Person)o], SortDirection.Descending);
                tableView = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 240);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            var bound = 0;
            var scrollable = 0.0;
            var leftStart = false;

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(ReferenceEquals(items[items.Count - 1], GetProjectedItem(tableView, 0)),
                    "Precondition: the descending sort must be active, so the last source item projects first.");

                bound = TableViewVirtualizationTestHelpers.RealizationBound(tableView);
                scrollable = GetBodyScroller(tableView)?.ScrollableHeight ?? 0.0;
                Verify.IsLessThan(bound, items.Count, "Precondition: the source must be far larger than the bound.");

                VerifySortedRealization(tableView, items, bound, "at the start");
            });

            for (var hop = 1; hop <= 10; hop++)
            {
                ScrollBodyTo(tableView, ScrollAxis.Vertical, scrollable * hop / 11.0);

                RunOnUIThread.Execute(() =>
                {
                    leftStart |= GetProjectedElements(tableView, requireAllRealized: false)[0] == null;
                    VerifySortedRealization(tableView, items, bound, $"at hop {hop}");
                });
            }

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(leftStart, "Precondition: the first sorted row must have left realization, so containers were recycled.");

                // A visual-tree count on purpose: it measures the container pool, which grows if recycled rows leak.
                var containers = GetRealizedRows(tableView).Count;
                Log.Comment($"Containers parented after ten hops: {containers}, bound {bound}.");

                Verify.IsLessThanOrEqual(containers, bound,
                    $"The container pool must not grow with the distance scrolled under a sort; {containers} containers, bound {bound}.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cell peer is reference-stable while its row stays realized, and that re-acquiring it after a scroll away and back describes the same item and coordinates.")]
        public void VerifyCellPeerIdentityIsStableAcrossScrollAwayAndBack()
        {
            TableView tableView = null;
            List<Person> items = null;
            Person target = null;
            TableViewCellAutomationPeer before = null;
            string nameBefore = null;
            const int targetIndex = 1;

            RunOnUIThread.Execute(() =>
            {
                items = MakeManyItems(RecyclingSourceCount);
                target = items[targetIndex];
                tableView = CreateScrollableTable(items);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var row = RequireRealizedRowFor(tableView, target);
                if (row == null)
                {
                    return;
                }

                before = GetCellPeer(row, 0);

                // TableViewRowAutomationPeer::GetOrCreateCellPeer is the single source of cell-peer identity while realized.
                Verify.IsTrue(ReferenceEquals(before, GetCellPeer(row, 0)),
                    "Re-acquiring a realized cell's peer must return the same provider; UIA compares providers by identity.");

                nameBefore = before.GetName();
                Verify.IsTrue(nameBefore != null && nameBefore.Contains(target.Name),
                    $"Precondition: the cell peer must name the target item; saw '{nameBefore}'.");
                VerifyCellCoordinates(before, targetIndex, 0, "before scrolling");
            });

            ScrollToEnd(tableView);

            RunOnUIThread.Execute(() =>
                Verify.IsFalse(RealizedProjectedRows(tableView).Any(row => ReferenceEquals(row.DataContext, target)),
                    "Precondition: the target item must have left realization, so its container was recycled."));

            ScrollToStart(tableView);

            RunOnUIThread.Execute(() =>
            {
                var row = RequireRealizedRowFor(tableView, target);
                if (row == null)
                {
                    return;
                }

                var after = GetCellPeer(row, 0);

                // Peers are per container: rehosting another item drops the cell-peer cache, so only value equality holds.
                Log.Comment($"Re-acquired peer is {(ReferenceEquals(before, after) ? "the same" : "a different")} instance.");

                Verify.AreEqual(nameBefore, after.GetName(),
                    "The re-acquired cell peer must report the same name for the same item after a scroll away and back.");
                VerifyCellCoordinates(after, targetIndex, 0, "after scrolling away and back");

                Verify.IsTrue(ReferenceEquals(after, GetCellPeer(row, 0)),
                    "Once realized again, the cell peer must again be reference-stable across acquisitions.");
            });
        }

        private static TableView CreateManyGroupsTable()
        {
            var items = Enumerable
                .Range(0, ManyGroupsItemCount)
                .Select(i => new Person { Name = $"Person {i}", Role = $"Group {i / ItemsPerGroup}" })
                .ToList();

            var source = TableViewSource.From(items);
            source.GroupBy(o => ((Person)o).Role);

            return CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 240);
        }

        // RealizationBound's viewport-plus-cache formula, sized by the shortest realized element so shorter group headers cannot exceed it.
        private static int MixedRealizationBound(TableView tableView)
        {
            var heights = GetProjectedElements(tableView, requireAllRealized: false)
                .OfType<FrameworkElement>()
                .Select(element => element.ActualHeight)
                .Where(height => height > 0.0)
                .ToList();

            var scroller = GetBodyScroller(tableView);

            if (heights.Count == 0 || scroller == null || scroller.ViewportHeight <= 0.0)
            {
                Verify.Fail("The fixture must realize sized elements inside a laid-out viewport.");
                return 0;
            }

            var viewportElements = (int)global::System.Math.Ceiling(scroller.ViewportHeight / heights.Min());
            Log.Comment($"Shortest element {heights.Min()}, viewport {scroller.ViewportHeight}, viewport elements {viewportElements}.");

            return (viewportElements * 5) + 4;
        }

        private static void VerifyGroupedRealizationBounded(TableView tableView, int bound, string context, bool expectRows)
        {
            var realized = TableViewVirtualizationTestHelpers.RealizedElementCount(tableView);
            var headers = ProjectedGroupHeaders(tableView).Count;
            var rows = ProjectedRowCount(tableView);
            Log.Comment($"{context}: realized {realized} ({headers} headers, {rows} rows), bound {bound}.");

            Verify.IsGreaterThan(headers, 0, $"Group headers must be realized ({context}).");
            Verify.IsLessThanOrEqual(realized, bound,
                $"Realization, headers included, must stay inside the viewport-plus-cache bound ({context}); realized {realized}, bound {bound}.");

            if (expectRows)
            {
                Verify.IsGreaterThan(rows, 0, $"Data rows must be realized alongside headers ({context}).");
            }
            else
            {
                Verify.AreEqual(0, rows, $"No data row may be realized while every group is collapsed ({context}).");
            }
        }

        private static void VerifySortedRealization(TableView tableView, List<Person> items, int bound, string context)
        {
            var elements = GetProjectedElements(tableView, requireAllRealized: false);
            var realized = elements.Count(element => element != null);
            Log.Comment($"{context}: realized {realized}, bound {bound}.");

            Verify.IsGreaterThan(realized, 0, $"Rows must be realized ({context}).");
            Verify.IsLessThanOrEqual(realized, bound,
                $"Realization must stay bounded under a sort ({context}); realized {realized}, bound {bound}.");

            for (var i = 0; i < elements.Count; i++)
            {
                if (elements[i] is TableViewRow row)
                {
                    Verify.IsTrue(ReferenceEquals(items[items.Count - 1 - i], row.DataContext),
                        $"The row realized at sorted index {i} must host the item sorted there ({context}).");
                }
            }
        }

        private static TableViewRow RequireRealizedRowFor(TableView tableView, Person item)
        {
            var row = RealizedProjectedRows(tableView).FirstOrDefault(candidate => ReferenceEquals(candidate.DataContext, item));
            Verify.IsNotNull(row, $"'{item.Name}' must be realized.");
            return row;
        }

        private static void VerifyCellCoordinates(TableViewCellAutomationPeer cellPeer, int row, int column, string context)
        {
            var gridItem = cellPeer.GetPattern(Microsoft.UI.Xaml.Automation.Peers.PatternInterface.GridItem)
                as Microsoft.UI.Xaml.Automation.Provider.IGridItemProvider;

            if (gridItem == null)
            {
                Verify.Fail($"The cell peer must advertise GridItem ({context}).");
                return;
            }

            Verify.AreEqual(row, gridItem.Row, $"The cell peer must report the row of the item it describes ({context}).");
            Verify.AreEqual(column, gridItem.Column, $"The cell peer must report its visible-column index ({context}).");
        }
    }

    internal static class TableViewRecyclingTestHelpers
    {
        internal const int RecyclingSourceCount = 120;

        internal const string OldMarker = "A:";
        internal const string NewMarker = "B:";

        internal static string FirstGroupKey => "Role 0";

        internal static TableView CreateScrollableTable(IList<Person> items)
            => CreateTableView(items, headers: new[] { "Name", "Role" }, width: 500, height: 240);

        internal static TableView CreateScrollableGroupedTable()
        {
            var source = TableViewSource.From(MakeManyItems(RecyclingSourceCount));
            source.GroupBy(o => ((Person)o).Role);

            return CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 240);
        }

        internal static DataTemplate CreateRecyclingMarkerTemplate(string marker) => (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
            $@"<DataTemplate xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation"">
                   <StackPanel Orientation=""Horizontal"">
                       <TextBlock Text=""{marker}"" />
                       <TextBlock Text=""{{Binding KeyText}}"" />
                   </StackPanel>
               </DataTemplate>");

        internal static void ScrollToEnd(TableView tableView)
        {
            var extent = 0.0;
            RunOnUIThread.Execute(() => extent = GetBodyScroller(tableView)?.ScrollableHeight ?? 0.0);

            Verify.IsGreaterThan(extent, 1.0, "The fixture must overflow its viewport, or nothing is recycled.");

            // One pixel short: ScrollBodyTo requires the extent to be strictly greater than the offset.
            ScrollBodyTo(tableView, ScrollAxis.Vertical, extent - 1.0);
        }

        internal static void ScrollToStart(TableView tableView) => ScrollBodyTo(tableView, ScrollAxis.Vertical, 0.0);

        // Not GetRealizedRows: it walks the visual tree and also returns pooled containers with a null DataContext.
        internal static List<TableViewRow> RealizedProjectedRows(TableView tableView)
            => GetProjectedElements(tableView, requireAllRealized: false)
                .OfType<TableViewRow>()
                .ToList();

        // requireAllRealized:false throughout (and no GetGroupHeader/GetGroupHeaderPeer): a virtualizing fixture never has every element realized.
        internal static List<TableViewGroupHeader> ProjectedGroupHeaders(TableView tableView)
            => GetProjectedElements(tableView, requireAllRealized: false)
                .OfType<TableViewGroupHeader>()
                .ToList();

        internal static int ProjectedRowCount(TableView tableView)
            => GetProjectedElements(tableView, requireAllRealized: false)
                .OfType<TableViewRow>()
                .Count();

        // Scoped to the ContentPresenter so the expander glyph stays out of the text.
        internal static List<string> ProjectedGroupHeaderTexts(TableView tableView)
        {
            var texts = new List<string>();

            foreach (var header in ProjectedGroupHeaders(tableView))
            {
                var presenter = FindVisualChildrenByType<ContentPresenter>(header).FirstOrDefault();

                if (presenter == null)
                {
                    continue;
                }

                var text = string.Concat(FindVisualChildrenByType<TextBlock>(presenter).Select(block => block.Text));

                if (!string.IsNullOrEmpty(text))
                {
                    texts.Add(text);
                }
            }

            return texts;
        }

        internal static string Describe(IEnumerable<string> texts) => string.Join(" | ", texts);
    }
}
