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
    // Category 5.3 of the TableView test plan, continued: recycling ACROSS features.
    //
    // WHY A SECOND FILE. TableViewRowRecyclingTests in TableView_Rows_APITests.cs owns recycling of a
    // row's own state - cell values, banding, and the selection visual. Everything here is recycling
    // crossed with a feature another category owns: grouping (§9), automation (§12) and templating.
    // The per-category split means no existing file owns those crossings, and they are where the bugs
    // actually were.
    //
    // WHY THIS CROSSING IS WORTH TESTING. PR #11820 states the root cause of a dozen separate defects
    // in one line: state keyed only by ItemsRepeater container identity goes stale when the container
    // is rebound to another item. PR #12183 was a visual state silently not running. PR #12190 fixed
    // pooled group headers resurfacing with a template that had since been replaced. Each was a
    // container that outlived the assumption attached to it.
    //
    // ALREADY COVERED, DELIBERATELY NOT REPEATED:
    //   Cell values after recycle        - VerifyRecycledCellsShowNewItemValues (5.3).
    //   Banding after recycle            - VerifyRecycledRowRebandsForItsNewIndex (5.3).
    //   Selection visual after recycle   - VerifyRecycledRowDoesNotInheritPreviousSelectionVisual (5.3).
    //   Custom column element on recycle - VerifyCustomColumnElementUpdatesOnRecycle (3.6).
    //   Tooltip on a recycled row        - VerifyRecycledRowShowsTheNewItemsToolTip (14).
    //   Live GroupHeaderTemplate swap    - VerifyGroupHeaderTemplateChangeAfterLoadUpdatesLive (9),
    //                                      which covers REALIZED headers. The pooled half is below,
    //                                      and is the half #12190 actually fixed.
    //
    // NOT IMPLEMENTED HERE: an open edit surviving a recycle. There is no programmatic BeginEdit in
    // this release (TableView.idl), and the Value provider's SetValue opens and closes the edit inside
    // one call, so an API test cannot hold an edit open across a scroll. It is gesture-bound and
    // belongs in section 11, for the same reason section 10.1 lists the gesture-driven editor tests
    // there rather than in the API suite.
    //
    // FIXTURES. Every test here uses a viewport deliberately shorter than its source so that scrolling
    // genuinely recycles. Each test asserts a precondition that recycling actually happened; without
    // it these would all pass on a table that never scrolled.
    //
    // REALIZATION. GetProjectedElements is called with requireAllRealized:false throughout. The
    // all-realized default - and GetGroupHeader / GetGroupHeaderPeer, which depend on it - would fail
    // the moment a fixture is tall enough to virtualize, which is the entire point of these tests.

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

            // Send the headers realized above into the container pool.
            ScrollToEnd(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(ProjectedGroupHeaderTexts(tableView).Any(text => text.StartsWith(OldMarker) && text.EndsWith(FirstGroupKey)),
                    "Precondition: the first group's header must have left realization, so its container is pooled.");

                // #12190 skips pooled containers when re-preparing headers, precisely so a pooled
                // container cannot be written to and then resurface. The contract is that it is
                // re-prepared on its way back in, not that it is updated while pooled.
                tableView.GroupHeaderTemplate = CreateRecyclingMarkerTemplate(NewMarker);
            });

            SettleLayout(tableView);

            ScrollToStart(tableView);

            RunOnUIThread.Execute(() =>
            {
                var markers = ProjectedGroupHeaderTexts(tableView);
                Verify.IsGreaterThan(markers.Count, 0, "Headers must realize again after scrolling back.");

                // The regression this guards is a container returning from the pool still carrying the
                // template it was prepared with, so the user sees two different header layouts in one
                // table depending on which headers happen to have been recycled.
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

                // CollapseAllGroups is the only public single-call route; the per-group peer provider
                // needs every header realized, which a virtualizing fixture by definition does not have.
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

                // Expansion belongs to the group, not to the container that happens to display it. A
                // container returning from the pool with a stale IsExpanded makes groups appear to
                // re-open as the user scrolls - and #12190 fixed a related case where queued per-group
                // toggles wrote an exception back after a bulk collapse.
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

    // Category 12 crossed with recycling: what the automation tree reports once containers have been
    // rebound. TableView_AutomationPeer_APITests.cs asserts every one of these values on a table at
    // rest; none of its tests scrolls. #11820 fixed virtualized rows reporting the wrong PositionInSet
    // and cells whose peers were cached against a recycled container, so the at-rest assertions are
    // exactly the ones that did not catch it.
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
                Verify.IsTrue(GetRealizedRows(tableView).Any(row => ReferenceEquals(row.DataContext, items[0])),
                    "Precondition: the first item must be realized before scrolling."));

            ScrollToEnd(tableView);

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 0, "Rows must still be realized after scrolling.");

                Verify.IsFalse(rows.Any(row => ReferenceEquals(row.DataContext, items[0])),
                    "Precondition: the first item must have left realization, so its container was recycled.");

                foreach (var row in rows)
                {
                    var item = row.DataContext as Person;
                    Verify.IsNotNull(item, "Every realized row must carry a data item.");

                    var name = GetRowPeer(row).GetName();
                    Log.Comment($"Row hosting '{item.Name}' reports name '{name}'.");

                    // Asserted by containment rather than equality: the row name is composed from the
                    // cell values through a localized separator, which §17 owns. What matters here is
                    // that it describes the CURRENT item.
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
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 0, "Rows must still be realized after scrolling.");

                Verify.IsFalse(rows.Any(row => ReferenceEquals(row.DataContext, items[0])),
                    "Precondition: the first item must have left realization.");

                foreach (var row in rows)
                {
                    var item = row.DataContext as Person;
                    Verify.IsNotNull(item, "Every realized row must carry a data item.");

                    // The cell peer is reached through the row peer, which is how a UIA client walks
                    // the tree. #11820's defect was a classification cached against the container, so
                    // a cell described the item that used to live there.
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
                var rows = GetRealizedRows(tableView);

                Verify.IsGreaterThan(rows.Count, 0, "Rows must still be realized after scrolling.");

                var checkedAny = false;

                foreach (var row in rows)
                {
                    var index = projected.IndexOf(row);

                    // A row realized outside the projection window this call observed is not something
                    // to assert against; the ones inside it are enough.
                    if (index < 0)
                    {
                        continue;
                    }

                    checkedAny = true;

                    var position = GetRowPeer(row).GetPositionInSet();
                    Log.Comment($"Row at projected index {index} reports PositionInSet {position}.");

                    // One-based, matching the column-header assertions in §12. A container recycled
                    // from the top of the source to the bottom keeps its old position if the value was
                    // captured at prepare time rather than read from the projection.
                    Verify.AreEqual(index + 1, position,
                        $"A recycled row at projected index {index} must report PositionInSet {index + 1}.");
                }

                Verify.IsTrue(checkedAny, "Precondition: at least one realized row must be locatable in the projection.");
            });
        }
    }

    internal static class TableViewRecyclingTestHelpers
    {
        // Each person gets a distinct Role, so grouping this source yields one group per item. That is
        // what makes a grouped fixture tall enough to recycle HEADERS rather than only rows.
        internal const int RecyclingSourceCount = 120;

        internal const string OldMarker = "A:";
        internal const string NewMarker = "B:";

        internal static string FirstGroupKey => "Role 0";

        // Short enough that most of the source stays virtualized. The row tests in 5.3 use the same
        // shape with a fixed scroll offset; these scroll to the extremes instead, so the fixture does
        // not have to agree with a hard-coded pixel value.
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

            Verify.IsGreaterThan(extent, 0.0, "The fixture must overflow its viewport, or nothing is recycled.");

            ScrollBodyTo(tableView, ScrollAxis.Vertical, extent);
        }

        internal static void ScrollToStart(TableView tableView) => ScrollBodyTo(tableView, ScrollAxis.Vertical, 0.0);

        // requireAllRealized:false throughout: a virtualizing fixture never has every element realized,
        // and the default would turn that into a failure.
        internal static List<TableViewGroupHeader> ProjectedGroupHeaders(TableView tableView)
            => GetProjectedElements(tableView, requireAllRealized: false)
                .OfType<TableViewGroupHeader>()
                .ToList();

        internal static int ProjectedRowCount(TableView tableView)
            => GetProjectedElements(tableView, requireAllRealized: false)
                .OfType<TableViewRow>()
                .Count();

        // Scoped to the header's ContentPresenter so the expander glyph - a private-use character in an
        // icon TextBlock outside the presenter - stays out of the comparison, matching HeaderText in §9.
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
