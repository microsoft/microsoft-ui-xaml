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
    // Category 5.3 of the TableView test plan, continued: recycling across grouping, templating and automation.
    // An open edit surviving a recycle is not tested: there is no programmatic BeginEdit, so it belongs in the interaction tests.
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

    // Category 12 crossed with recycling: automation peers must describe the item a recycled container now hosts.
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
