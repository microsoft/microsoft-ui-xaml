// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewShapingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewSortingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewEditingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // TableViewSource2, Milestone 9: the owner hooks a TableView drives for a TableViewSource2 - sort
    // chevrons from the applied spec, the grouped row kinds, the edit hold over live shaping, and the
    // TiP-gated control sort.
    public partial class TableViewSource2Tests
    {
        private TableView LoadOwnedTable(TableViewSource2 source, bool writable = false)
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 700);
                tableView.IsReadOnly = !writable;
                LoadContent(tableView);
            });
            SettleLayout(tableView);
            return tableView;
        }

        private static string ColumnSorts(TableView tableView)
            => string.Join(",", tableView.Columns.Select(column => column.SortDirection.ToString()));

        [TestMethod]
        [TestProperty("Description", "Verifies the column chevrons follow the primary level of the TableViewSource2 applied sort.")]
        public void VerifySortIndicatorFollowsAppliedSpec()
        {
            TableViewSource2 source = null;
            RunOnUIThread.Execute(() => source = TableViewSource2.From(MakeShapedPeople()));
            var tableView = LoadOwnedTable(source);

            RunOnUIThread.Execute(() => source.Sort("Name", SortDirection.Ascending));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreEqual("Ascending,None", ColumnSorts(tableView), "A Name path primary lights the Name column."));

            // A lower-priority level does not change the primary.
            RunOnUIThread.Execute(() => source.Sort("Role", SortDirection.Descending));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreEqual("Ascending,None", ColumnSorts(tableView), "A secondary level lights nothing."));

            RunOnUIThread.Execute(() => source.Sort("Name", SortDirection.Descending));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreEqual("Descending,None", ColumnSorts(tableView), "Re-sorting the primary path flips its chevron."));

            RunOnUIThread.Execute(() => source.ClearSort());
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreEqual("None,None", ColumnSorts(tableView), "ClearSort darkens every chevron."));

            RunOnUIThread.Execute(() => source.Sort("Role", SortDirection.Ascending));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreEqual("None,Ascending", ColumnSorts(tableView), "A Role path primary lights the Role column."));

            // A key selector names no property, so no column can claim it.
            RunOnUIThread.Execute(() => source.ClearSort().Sort(o => Person(o).Name.Length, SortDirection.Ascending));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreEqual("None,None", ColumnSorts(tableView), "A key-selector primary lights nothing."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies grouped row metadata follows GroupBy and ClearGroupBy on a TableViewSource2.")]
        public void VerifyIsGroupedFollowsGroupBy()
        {
            TableViewSource2 source = null;
            List<ShapedPerson> people = null;
            RunOnUIThread.Execute(() =>
            {
                people = MakeShapedPeople();
                source = TableViewSource2.From(people);
            });
            var tableView = LoadOwnedTable(source);
            var ines = people.First(p => p.Name == "Ines");

            RunOnUIThread.Execute(() => source.GroupBy(o => Person(o).Role, SortDirection.None));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(3, GroupHeaderCount(tableView), "Three roles, three headers.");
                Verify.IsTrue(GetRowsRepeater(tableView).TryGetElement(0) is TableViewGroupHeader, "Row 0 is a group header.");

                // Grouped: set metadata is relative to the row's group (Designer: Asha, Ines).
                var peer = GetRowPeer(GetRowForItem(tableView, ines));
                Verify.AreEqual(2, peer.GetPositionInSet(), "Ines is second in her group.");
                Verify.AreEqual(2, peer.GetSizeOfSet(), "The Designer group has two rows.");
            });

            RunOnUIThread.Execute(() => source.ClearGroupBy());
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, GroupHeaderCount(tableView), "ClearGroupBy removes every header.");
                var peer = GetRowPeer(GetRowForItem(tableView, ines));
                Verify.AreEqual(5, peer.GetPositionInSet(), "Flat: Ines is fifth in source order.");
                Verify.AreEqual(6, peer.GetSizeOfSet(), "Flat: the set is every row.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an open edit holds live shaping, so the edited row keeps its place until the edit closes.")]
        public void VerifyEditKeepsRowDuringLiveReshape()
        {
            TableViewSource2 source = null;
            List<EditablePerson> items = null;
            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems("Asha", "Diego", "Mei");
                source = TableViewSource2.From(items).Sort("Name", SortDirection.Ascending);
                source.IsLiveShaping = true;
            });
            var tableView = LoadOwnedTable(source, writable: true);
            var asha = items[0];
            var diego = items[1];

            Func<string> names = () => string.Join(",", GetProjectedItems(tableView).Select(o => ((EditablePerson)o).Name));

            RunOnUIThread.Execute(() =>
            {
                // A vetoed commit leaves the edit open without writing the pending value.
                bool veto = true;
                tableView.CellEditEnding += (s, e) => e.Cancel = veto;
                CaptureSetValue(GetValueProvider(tableView, asha, columnIndex: 0), "Pending");
                veto = false;
                Verify.IsTrue(tableView.IsEditing, "Precondition: the edit is open.");

                // A live sort-key change to the edited row would move it to the end.
                asha.Name = "Zoe";
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.IsEditing, "The live reshape must not close the edit.");
                Verify.AreEqual("Zoe,Diego,Mei", names(), "The edited row keeps its place while the edit is open.");
                Verify.IsTrue(tableView.CancelEdit(), "The edit closes.");
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual("Diego,Mei,Zoe", names(), "Closing the edit releases the held live change."));

            // The hold is per edit: with no edit open, a live change applies on the next turn.
            RunOnUIThread.Execute(() => diego.Name = "Zed");
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
                Verify.AreEqual("Mei,Zed,Zoe", names(), "A live change after the edit is not held."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a control-initiated column sort applies to a TableViewSource2 and replaces its levels.")]
        public void VerifyTipAttachesToSource2()
        {
            TableViewSource2 source = null;
            RunOnUIThread.Execute(() => source = TableViewSource2.From(MakeShapedPeople()).Sort("Role", SortDirection.Ascending));
            var tableView = LoadOwnedTable(source);
            int sorted = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView.Sorted += (s, e) => sorted++;
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "The column sort is accepted.");
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                // The control sort replaces the app's Role level rather than ranking under it.
                VerifySequence(NameAscendingOrder, RowValues(tableView, p => p.Name), "sorted by Name only");
                Verify.AreEqual("Ascending,None", ColumnSorts(tableView), "The sorted column is lit.");
                Verify.IsTrue(sorted > 0, "Sorted is raised.");
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[1], SortDirection.Descending), "A second column sort is accepted.");
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Diego", "Rafa", "Owen", "Asha", "Ines", "Mei" }, RowValues(tableView, p => p.Name),
                    "sorted by Role descending, stable in source order");
                Verify.AreEqual("None,Descending", ColumnSorts(tableView), "Only the new column is lit.");

                // A comparer column sorts through a TableViewComparer2 and keeps its own chevron.
                tableView.Columns[0].CustomSortComparer = new LengthThenOrdinalComparer();
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending), "The comparer sort is accepted.");
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "Diego", "Rafa", "Owen", "Ines", "Asha", "Mei" }, RowValues(tableView, p => p.Name),
                    "sorted by the comparer, descending");
                Verify.AreEqual("Descending,None", ColumnSorts(tableView), "The comparer column keeps its chevron.");
            });
        }
    }
}
