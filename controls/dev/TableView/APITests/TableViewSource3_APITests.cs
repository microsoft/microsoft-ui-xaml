// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewShapingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // TableViewSource3: the key-identified source. Rows are identified by the string key the app's
    // keySelector returns, and one pre-order row array serves the flat, grouped, ParentBy(identity)
    // and grouped+hierarchical shapes. See docs\superpowers\specs\2026-10-27-tableviewsource2-design.md.
    //
    // Flat fixtures reuse MakeShapedPeople (keyed by Name); tree fixtures reuse MakeTree, keyed by Id
    // with ParentBy(identity) returning ManagerId, so the tree is the same one TableViewHierarchyTests
    // asserts: Ada > { Ben > { Dan }, Cy }, Eve > { Fay }, Gus (ManagerId 99 matches nobody).
    [TestClass]
    public class TableViewSource3Tests : TableViewApiTestBase
    {
        private const string Roots = "Ada1+ Eve1+ Gus1";
        private const string FullTree = "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1";

        #region Construction and keys

        [TestMethod]
        [TestProperty("Description", "Verifies the keyless From throws: TableViewSource3 identifies rows only by app key.")]
        public void VerifyKeylessFromThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                Verify.Throws<ArgumentException>(() => TableViewSource3.From(MakeShapedPeople()),
                    "From(items) without a key selector must throw E_INVALIDARG.");
                Verify.Throws<ArgumentException>(() => TableViewSource3.From(MakeShapedPeople(), null),
                    "A null key selector must throw E_INVALIDARG.");
                Verify.Throws<ArgumentException>(() => TableViewSource3.From(null, PersonKey),
                    "Null items must throw E_INVALIDARG.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies duplicate, null and empty keys are rejected at construction.")]
        public void VerifyInvalidKeysThrowAtConstruction()
        {
            RunOnUIThread.Execute(() =>
            {
                var duplicate = MakeShapedPeople();
                duplicate.Add(new ShapedPerson("Asha", "Engineer", "Platform"));
                Verify.Throws<ArgumentException>(() => TableViewSource3.From(duplicate, PersonKey),
                    "Two rows with the key 'Asha' must throw.");

                Verify.Throws<ArgumentException>(() => TableViewSource3.From(MakeShapedPeople(), o => null),
                    "A null key must throw.");
                Verify.Throws<ArgumentException>(() => TableViewSource3.From(MakeShapedPeople(), o => ""),
                    "An empty key must throw.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the registry creates the v3 variant and lists it.")]
        public void VerifyRegistryCreatesV3()
        {
            TableView tableView = null;
            List<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(TableViewSourceRegistry.Variants.Contains("v3"), "Variants must list v3.");
                items = MakeShapedPeople();
                var source = TableViewSourceRegistry.Create("v3", items, PersonKey);
                Verify.IsTrue(source is TableViewSource3, "Create(\"v3\") must return a TableViewSource3.");
                tableView = CreateShapedTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyProjection(tableView, items.Select(p => p.Name).ToList(), "registry v3"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the hierarchy verbs v3 does not implement throw E_NOTIMPL, and v1 rejects ParentByIdentity.")]
        public void VerifyUnsupportedVerbsThrowNotImplemented()
        {
            RunOnUIThread.Execute(() =>
            {
                var v3 = TableViewSource3.From(MakeTree(), EmpId);
                Verify.Throws<NotImplementedException>(() => v3.ParentBy(ById, ByManager),
                    "The two-selector ParentBy is not supported by v3.");
                Verify.Throws<NotImplementedException>(() => v3.ChildrenWith(o => null),
                    "ChildrenWith is not supported by v3.");
                Verify.Throws<NotImplementedException>(() => v3.ClearChildrenWith(),
                    "ClearChildrenWith is not supported by v3.");

                var v1 = TableViewSource.From(MakeTree());
                Verify.Throws<NotImplementedException>(() => v1.ParentBy(EmpManagerId),
                    "v1 does not support ParentBy(identity).");
            });
        }

        #endregion

        #region Flat shaping

        [TestMethod]
        [TestProperty("Description", "Verifies an unshaped v3 source mirrors the source order with no headers.")]
        public void VerifyFreshSourceMirrorsSource()
        {
            TableView tableView = null;
            List<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeShapedPeople();
                tableView = CreateShapedTable(TableViewSource3.From(items, PersonKey));
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, items.Select(p => p.Name).ToList(), "fresh v3 source");
                Verify.AreEqual(0, GroupHeaderCount(tableView), "No GroupBy means no header rows.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Filter and Sort apply and ClearFilter / ClearSort restore.")]
        public void VerifyFilterAndSort()
        {
            TableView tableView = null;
            ITableViewShapingSource source = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource3.From(MakeShapedPeople(), PersonKey)
                    .Filter(o => Person(o).Role == "Engineer")
                    .Sort("Name", SortDirection.Descending);
                tableView = CreateShapedTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new List<string> { "Rafa", "Owen", "Diego" }, "filter + sort descending");
                source.ClearFilter();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new List<string> { "Rafa", "Owen", "Mei", "Ines", "Diego", "Asha" }, "filter cleared");
                source.ClearSort();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
                VerifyProjection(tableView, MakeShapedPeople().Select(p => p.Name).ToList(), "sort cleared"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies insert, remove and replace in an observable source update the projection.")]
        public void VerifySourceChangesTrack()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateShapedTable(TableViewSource3.From(items, PersonKey));
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => items.Insert(2, new ShapedPerson("Nadia", "Engineer", "Platform")));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new List<string> { "Asha", "Diego", "Nadia", "Mei", "Rafa", "Ines", "Owen" }, "after insert");
                items.RemoveAt(0);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new List<string> { "Diego", "Nadia", "Mei", "Rafa", "Ines", "Owen" }, "after remove");
                items[2] = new ShapedPerson("Zoe", "Architect", "Research");
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
                VerifyProjection(tableView, new List<string> { "Diego", "Nadia", "Zoe", "Rafa", "Ines", "Owen" }, "after replace"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies inserting a row whose key already exists fails the mutation.")]
        public void VerifyDuplicateInsertThrows()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateShapedTable(TableViewSource3.From(items, PersonKey));
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                var thrown = ThrownTypeName(() => items.Add(new ShapedPerson("Mei", "Engineer", "Platform")));
                Verify.AreNotEqual("no exception", thrown, "Adding a second 'Mei' must fail the app's mutation.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies flat GroupBy: headers in first-appearance order, members in source order, counts.")]
        public void VerifyFlatGroupBy()
        {
            TableView tableView = null;
            ITableViewShapingSource source = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource3.From(MakeShapedPeople(), PersonKey).GroupBy(o => Person(o).Role);
                tableView = CreateShapedTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(9, GetProjectedCount(tableView), "Three headers plus six items.");
                VerifyProjection(
                    tableView,
                    new List<string>
                    {
                        "#Designer(2)", "Asha", "Ines",
                        "#Engineer(3)", "Diego", "Rafa", "Owen",
                        "#Architect(1)", "Mei",
                    },
                    "grouped by Role");
                source.Filter(o => Person(o).Role != "Architect");
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(
                    tableView,
                    new List<string> { "#Designer(2)", "Asha", "Ines", "#Engineer(3)", "Diego", "Rafa", "Owen" },
                    "a group with no kept item disappears");
                source.ClearGroupBy();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreEqual(0, GroupHeaderCount(tableView), "ClearGroupBy removes every header."));
        }

        #endregion

        #region Hierarchy

        [TestMethod]
        [TestProperty("Description", "Verifies ParentBy(identity) projects roots collapsed, and an unmatched parent key is a root.")]
        public void VerifyParentByIdentityProjectsRoots()
        {
            ITableViewShapingSource source = null;
            RunOnUIThread.Execute(() => source = KeyedTree(MakeTree()));
            var tableView = Place(source);

            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, Roots, "first projection");
                tableView.ExpandAllRows();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, FullTree, "expanded");
                var dan = FindTreeRow(tableView, "Dan");
                Verify.AreEqual(3, dan.Level, "Dan is a grandchild.");
                Verify.IsFalse(dan.IsExpandable, "Dan is a leaf.");
                tableView.CollapseAllRows();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "collapsed again"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a row expanded and collapsed through its peer shows and hides only its subtree.")]
        public void VerifyToggleExpandsOneSubtree()
        {
            ITableViewShapingSource source = null;
            RunOnUIThread.Execute(() => source = KeyedTree(MakeTree()));
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2+ Cy2 Eve1+ Gus1", "Ada expanded"));
            Toggle(tableView, "Ben", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1+ Gus1", "Ben expanded"));
            Toggle(tableView, "Ada", false);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "Ada collapsed"));
            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1+ Gus1", "Ben's state survives Ada's collapse"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a self-parent or a parent cycle is rejected.")]
        public void VerifyCyclesThrow()
        {
            RunOnUIThread.Execute(() =>
            {
                var self = MakeTree();
                self[0].ManagerId = self[0].Id;
                Verify.Throws<ArgumentException>(() => KeyedTree(self), "A row that is its own parent must throw.");

                var cycle = MakeTree();
                cycle[0].ManagerId = 4; // Ada > Ben > Dan > Ada.
                Verify.Throws<ArgumentException>(() => KeyedTree(cycle), "A parent cycle must throw.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a filter keeps the ancestors of matching rows as context.")]
        public void VerifyFilterKeepsAncestors()
        {
            ITableViewShapingSource source = null;
            RunOnUIThread.Execute(() => source = KeyedTree(MakeTree()));
            var tableView = Place(source);

            RunOnUIThread.Execute(() => source.Filter(o => ((TreeEmployee)o).Name == "Dan"));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2- Dan3", "Dan's ancestors are context rows"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Sort orders each sibling run independently.")]
        public void VerifySortOrdersSiblings()
        {
            ITableViewShapingSource source = null;
            RunOnUIThread.Execute(() => source = KeyedTree(MakeTree()).Sort("Score", SortDirection.Descending));
            var tableView = Place(source);

            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);

            // Roots by score: Gus 70, Eve 60, Ada 50. Ada's children: Ben 40, Cy 30.
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Gus1 Eve1- Fay2 Ada1- Ben2- Dan3 Cy2", "sorted by score descending"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies inserting a child under an expanded parent places it in that parent's run.")]
        public void VerifyInsertChildIntoTree()
        {
            ObservableCollection<TreeEmployee> list = null;
            ITableViewShapingSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = KeyedTree(list);
            });
            var tableView = Place(source);

            Toggle(tableView, "Eve", true);
            RunOnUIThread.Execute(() => list.Add(new TreeEmployee { Id = 8, ManagerId = 5, Name = "Hal", Dept = "Ops" }));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada1+ Eve1- Fay2 Hal2 Gus1", "Hal added under Eve");
                list.Remove(Named(list, "Fay"));
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1+ Eve1- Hal2 Gus1", "Fay removed"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies GroupBy with ParentBy buckets roots only; descendants follow their root.")]
        public void VerifyGroupedHierarchyBucketsRootsOnly()
        {
            ITableViewShapingSource source = null;
            RunOnUIThread.Execute(() => source = KeyedTree(MakeTree()).GroupBy(EmpKey(e => e.Dept)));
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "[Eng] Ada1- Ben2+ Cy2 [Ops] Eve1+ Gus1", "grouped by Dept"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearParentBy returns to a flat projection of every row.")]
        public void VerifyClearParentByFlattens()
        {
            ITableViewShapingSource source = null;
            RunOnUIThread.Execute(() => source = KeyedTree(MakeTree()));
            var tableView = Place(source);

            RunOnUIThread.Execute(() => source.ClearParentBy());
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada Ben Cy Dan Eve Fay Gus", "flat after ClearParentBy"));
        }

        #endregion

        #region Live shaping

        [TestMethod]
        [TestProperty("Description", "Verifies live shaping re-sorts a row whose sort key changes.")]
        public void VerifyLiveSortMovesRow()
        {
            ObservableCollection<LiveRow> list = null;
            ITableViewShapingSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeLiveFlat();
                source = TableViewSource3.From(list, o => ((LiveRow)o).Id.ToString()).Sort(LiveByScore, SortDirection.Ascending);
                source.IsLiveShaping = true;
            });
            var tableView = Place(source);

            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "A B C D", "baseline by score");
                Named(list, "A").Score = 35;
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "B C A D", "A moved after its score changed"));
        }

        #endregion

        #region Incremental updates

        [TestMethod]
        [TestProperty("Description", "Verifies insert, remove and replace into a sorted, filtered source land where a rebuild puts them.")]
        public void VerifyIncrementalEditsInSortedFilteredSource()
        {
            ObservableCollection<LiveRow> list = null;
            ITableViewShapingSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeLiveFlat();
                source = TableViewSource3.From(list, LiveId).Filter(MinScore(15)).Sort(LiveByScore, SortDirection.Ascending);
            });
            var tableView = Place(source);

            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "B C D", "baseline");
                list.Add(new LiveRow { Id = 5, Name = "E", Score = 25, Team = "X" });
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "B E C D", "E slotted by score");
                list.Insert(0, new LiveRow { Id = 6, Name = "F", Score = 5, Team = "Y" });
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "B E C D", "a filtered-out insert shows nothing");
                list.Remove(Named(list, "C"));
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "B E D", "C removed");
                list[list.IndexOf(Named(list, "B"))] = new LiveRow { Id = 2, Name = "B", Score = 50, Team = "Y" };
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "E D B", "replacing B with a higher score moves it");
                list[list.IndexOf(Named(list, "F"))] = new LiveRow { Id = 6, Name = "F", Score = 35, Team = "Y" };
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "E F D B", "a replacement that now matches appears"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies grouped inserts, a live move between groups, and an emptied group stay consistent.")]
        public void VerifyIncrementalGroupedEdits()
        {
            ObservableCollection<LiveRow> list = null;
            ITableViewShapingSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeLiveFlat();
                source = TableViewSource3.From(list, LiveId).GroupBy(LiveByTeam);
                source.IsLiveShaping = true;
            });
            var tableView = Place(source);

            RunOnUIThread.Execute(() =>
            {
                VerifyFlat(tableView, "[X] A C [Y] B D", "baseline");
                list.Add(new LiveRow { Id = 5, Name = "E", Score = 50, Team = "X" });
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyFlat(tableView, "[X] A C E [Y] B D", "E joins an existing group");
                list.Add(new LiveRow { Id = 6, Name = "F", Score = 60, Team = "Z" });
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyFlat(tableView, "[X] A C E [Y] B D [Z] F", "F opens a new group");
                Named(list, "C").Team = "Y";
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyFlat(tableView, "[X] A E [Y] B C D [Z] F", "C moved between groups");
                Named(list, "A").Team = "Y";
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyFlat(tableView, "[Y] A B C D [X] E [Z] F", "Y now appears first, so the groups reorder");
                list.Remove(Named(list, "F"));
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyFlat(tableView, "[Y] A B C D [X] E", "an emptied group disappears"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies live sort changes and a live reparent move whole rows within the tree.")]
        public void VerifyIncrementalLiveTreeMoves()
        {
            ObservableCollection<LiveRow> list = null;
            ITableViewShapingSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeLiveTree();
                source = TableViewSource3.From(list, LiveId).ParentBy(LiveParentId).Sort(LiveByScore, SortDirection.Ascending);
                source.IsLiveShaping = true;
            });
            var tableView = Place(source);

            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Dee1- Eli2 Ann1- Cat2 Bob2", "baseline by score");
                Named(list, "Bob").ParentId = 4;
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Dee1- Eli2 Bob2 Ann1- Cat2", "Bob reparented under Dee");
                Named(list, "Eli").Score = 40;
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Dee1- Bob2 Eli2 Ann1- Cat2", "Eli moved within Dee");
                Named(list, "Ann").Score = 1;
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ann1- Cat2 Dee1- Bob2 Eli2", "Ann's subtree moved ahead of Dee's");
                Named(list, "Cat").ParentId = 4;
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ann1 Dee1- Cat2 Bob2 Eli2", "Ann lost its last child"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies insert and remove inside a collapsed parent update only its expander.")]
        public void VerifyIncrementalEditsUnderCollapsedParent()
        {
            ObservableCollection<TreeEmployee> list = null;
            ITableViewShapingSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = KeyedTree(list);
            });
            var tableView = Place(source);

            RunOnUIThread.Execute(() => list.Add(new TreeEmployee { Id = 8, ManagerId = 7, Name = "Hal", Dept = "Ops" }));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada1+ Eve1+ Gus1+", "Gus gained a hidden child");
                list.Remove(Named(list, "Fay"));
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1+ Eve1 Gus1+", "Eve lost its only child"));
            Toggle(tableView, "Gus", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1+ Eve1 Gus1- Hal2", "Gus expands to Hal"));
        }

        #endregion

        #region Helpers

        private static readonly TableViewIdentitySelector LiveId = o => ((LiveRow)o).Id.ToString();
        private static readonly TableViewIdentitySelector LiveParentId = o => ((LiveRow)o).ParentId?.ToString();

        // Flat grouped rows: compare names and headers, not the levels the rows carry.
        private static void VerifyFlat(TableView tableView, string expected, string context)
            => Verify.AreEqual(expected, global::System.Text.RegularExpressions.Regex.Replace(TreeLabels(tableView), @"(?<=[A-Za-z])\d+[+-]?", ""), $"Projection ({context}).");

        private static readonly TableViewIdentitySelector PersonKey = o => Person(o).Name;
        private static readonly TableViewIdentitySelector EmpId = o => ((TreeEmployee)o).Id.ToString();
        private static readonly TableViewIdentitySelector EmpManagerId = o => ((TreeEmployee)o).ManagerId?.ToString();

        private static ITableViewShapingSource KeyedTree(ObservableCollection<TreeEmployee> list)
            => TableViewSource3.From(list, EmpId).ParentBy(EmpManagerId);

        private TableView Place(object source)
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                tableView = CreateTreeTable(source);
                LoadContent(tableView);
            });
            SettleLayout(tableView);
            return tableView;
        }

        private static void Toggle(TableView tableView, string name, bool expand)
        {
            RunOnUIThread.Execute(() => ToggleTreeRow(tableView, name, expand));
            SettleLayout(tableView);
        }

        #endregion
    }
}
