// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System.Collections.ObjectModel;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // TableViewSource2, Milestone 7: group and row expansion through ShapedRowMetadata. Collapsing removes
    // the shown descendants as one range, expanding inserts them as one range, and the bulk group verbs
    // Reset. MakeLiveFlat: A10 X, B20 Y, C30 X, D40 Y. MakeTree: Ada > { Ben > { Dan }, Cy }, Eve > { Fay }, Gus.
    // ExpandSubtree (the Multiply key) and Reveal have no input-free public entry, so they are not
    // covered here (drift #57).
    public partial class TableViewSource2Tests
    {
        private TableView PlaceGrouped(out TableViewSource2 source, out ObservableCollection<LiveRow> rows)
        {
            var tableView = PlaceFlat(out source, out rows);
            var grouped = source;
            OnUI(tableView, () => grouped.GroupBy(LiveByTeam, SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C [H] B D", "GroupBy Team ascending"));
            return tableView;
        }

        private static void SetGroupExpanded(TableView tableView, int headerIndex, bool expanded)
        {
            OnUI(tableView, () =>
            {
                var provider = GetExpandCollapseProvider(tableView, headerIndex);
                if (expanded)
                {
                    provider.Expand();
                }
                else
                {
                    provider.Collapse();
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies collapsing a group removes its members as one range, expanding inserts them as one range, and a repeated verb raises nothing.")]
        public void VerifyCollapseGroupHidesMembers()
        {
            var tableView = PlaceGrouped(out _, out _);
            var events = RecordViewEventsWithCounts(tableView);

            SetGroupExpanded(tableView, 0, false);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] [H] B D", "group X collapsed"));
            VerifyEvents(events, "Remove@1+2", "collapse removes both members");

            SetGroupExpanded(tableView, 0, false);
            VerifyEvents(events, "", "collapsing a collapsed group raises nothing");

            SetGroupExpanded(tableView, 1, false);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] [H]", "both groups collapsed"));
            VerifyEvents(events, "Remove@2+2", "the second header now sits at 1");

            SetGroupExpanded(tableView, 0, true);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C [H]", "group X expanded"));
            VerifyEvents(events, "Add@1+2", "expand inserts both members");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies expanding a tree row inserts its shown descendants after it as one range, and collapsing removes them, keeping the descendants' own expansion.")]
        public void VerifyExpandRowShowsChildren()
        {
            var source = FromOnUI(MakeTree());
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            VerifyTreeOnUI(tableView, "Ada1+ Eve1+ Gus1", "collapsed");
            var events = RecordViewEventsWithCounts(tableView);

            OnUI(tableView, () => ToggleTreeRow(tableView, "Ada", true));
            VerifyTreeOnUI(tableView, "Ada1- Ben2+ Cy2 Eve1+ Gus1", "Ada expanded");
            VerifyEvents(events, "Add@1+2", "Ada's children arrive as one range");

            OnUI(tableView, () => ToggleTreeRow(tableView, "Ben", true));
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1+ Gus1", "Ben expanded");
            VerifyEvents(events, "Add@2+1", "Ben's child arrives");

            OnUI(tableView, () => ToggleTreeRow(tableView, "Ada", false));
            VerifyTreeOnUI(tableView, "Ada1+ Eve1+ Gus1", "Ada collapsed");
            VerifyEvents(events, "Remove@1+3", "every shown descendant leaves as one range");

            OnUI(tableView, () => ToggleTreeRow(tableView, "Ada", true));
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1+ Gus1", "Ben stayed expanded under collapsed Ada");
            VerifyEvents(events, "Add@1+3", "the remembered subtree comes back as one range");
            RunOnUIThread.Execute(() => Verify.AreEqual(6, GetRowsRepeater(tableView).ItemsSourceView.Count, "Six rows are shown."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ExpandAllGroups and CollapseAllGroups each raise one Reset and set every group, and clear per-group exceptions.")]
        public void VerifyExpandAllAndCollapseAllGroups()
        {
            var tableView = PlaceGrouped(out _, out _);
            SetGroupExpanded(tableView, 0, false);
            var events = RecordViewEventsWithCounts(tableView);

            OnUI(tableView, () => tableView.CollapseAllGroups());
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] [H]", "CollapseAllGroups"));
            VerifyEvents(events, "Reset", "collapse all");

            SetGroupExpanded(tableView, 1, true);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] [H] B D", "group Y expanded alone"));
            VerifyEvents(events, "Add@2+2", "one group expands under collapse-all");

            OnUI(tableView, () => tableView.ExpandAllGroups());
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C [H] B D", "ExpandAllGroups"));
            VerifyEvents(events, "Reset", "expand all");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a group's expansion is kept by identity across a Sort and a Filter reapply.")]
        public void VerifyGroupExpansionSurvivesReapply()
        {
            var tableView = PlaceGrouped(out var source, out _);
            SetGroupExpanded(tableView, 0, false);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] [H] B D", "group X collapsed"));

            OnUI(tableView, () => source.Sort(LiveByScore, SortDirection.Descending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] [H] D B", "X stays collapsed after a Sort"));

            OnUI(tableView, () => source.Filter(MinScore(15)));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] [H] D B", "X (now just C) stays collapsed after a Filter"));

            SetGroupExpanded(tableView, 0, true);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] C [H] D B", "X expands to its filtered member"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies source changes under a collapsed group raise no view event but keep the counts, so a later expand inserts the right range.")]
        public void VerifyEditsUnderCollapsedGroupAreSilent()
        {
            var tableView = PlaceGrouped(out _, out var rows);
            SetGroupExpanded(tableView, 0, false);
            var events = RecordViewEventsWithCounts(tableView);

            OnUI(tableView, () => rows.Add(NewLive(5, "E", 50, "X")));
            OnUI(tableView, () => rows.RemoveAt(0));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] [H] B D", "hidden changes"));
            VerifyEvents(events, "", "changes under a collapsed group raise nothing");

            SetGroupExpanded(tableView, 0, true);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] C E [H] B D", "expanded after the hidden changes"));
            VerifyEvents(events, "Add@1+2", "expand inserts the current members");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies RowInfo reports PositionInSet and SizeOfSet among siblings, not projected rows, as the row peers expose them.")]
        public void VerifyRowInfoPositionInSet()
        {
            var source = FromOnUI(MakeTree());
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1", "expanded");

            RunOnUIThread.Execute(() => Verify.AreEqual("Ada 1/3 Ben 1/2 Dan 1/1 Cy 2/2 Eve 2/3 Fay 1/1 Gus 3/3", PositionLabels(tableView),
                "PositionInSet and SizeOfSet count siblings."));

            OnUI(tableView, () => ToggleTreeRow(tableView, "Ada", false));
            RunOnUIThread.Execute(() => Verify.AreEqual("Ada 1/3 Eve 2/3 Fay 1/1 Gus 3/3", PositionLabels(tableView),
                "Collapsing does not change a sibling set."));
        }
    }
}
