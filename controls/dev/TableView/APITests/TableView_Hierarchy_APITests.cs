// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Runtime.CompilerServices;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Hierarchical rows: TableViewSource.ParentBy / ClearParentBy, TableView.ExpandAllRows /
    // CollapseAllRows and TableViewRow.Level / IsExpandable / IsExpanded.
    //
    // Fixture (MakeTree, ParentBy(Id, ManagerId)):
    //   Ada > { Ben > { Dan }, Cy }, Eve > { Fay }, Gus (ManagerId 99 matches nobody, so a root).
    // Projections are asserted as TreeLabels strings; see TableView_Hierarchy_APITests_Common.cs.
    [TestClass]
    public class TableViewHierarchyTests : TableViewApiTestBase
    {
        // PENDING DECISION (not implemented): L01 SelectionSurvivesSameKeyReplace.
        // Flat sources do not re-anchor selection to a re-created item (VerifySelectionIsNotReanchoredAcrossItemRecreation),
        // but hierarchical sources re-anchor by node key. Add this test once the contract is documented or aligned.

        private const string Roots = "Ada1+ Eve1+ Gus1";
        private const string AdaBenExpanded = "Ada1- Ben2- Dan3 Cy2 Eve1+ Gus1";
        private const string FullTree = "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1";
        private const string AllRoots = "Ada1 Ben1 Cy1 Dan1 Eve1 Fay1 Gus1";
        private const string Flat = "Ada Ben Cy Dan Eve Fay Gus";

        #region 3.1 Projection basics

        [TestMethod]
        [TestProperty("Description", "Verifies the first projection of a ParentBy source shows only roots, collapsed.")]
        public void VerifyParentByProjectsRootsCollapsed()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "first projection"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a null parent key, or one that matches no item, makes the row a root and no row is dropped.")]
        public void VerifyRootsAndOrphansAreRoots()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            // Ada and Eve have a null ManagerId; Gus's ManagerId (99) matches nobody.
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, Roots, "null and orphan parent keys");
                tableView.ExpandAllRows();
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, FullTree, "expanded: every row is projected");
                Verify.AreEqual(7, GetProjectedCount(tableView), "No row may be dropped.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an empty-string parent key means no parent, not a key lookup.")]
        public void VerifyEmptyStringParentIsRoot()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
                source = TableViewSource.From(MakeTree()).ParentBy(EmpKey(e => e.Code), EmpKey(e => e.ParentCode)));
            var tableView = Place(source);

            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "ParentCode \"\" on Ada and Eve"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies TableViewRow Level, IsExpandable and IsExpanded reflect node state after ExpandAllRows.")]
        public void VerifyLevelAndExpandableDPs()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var ada = FindTreeRow(tableView, "Ada");
                Verify.AreEqual(1, ada.Level, "Ada is a root.");
                Verify.IsTrue(ada.IsExpandable, "Ada has children.");
                Verify.IsTrue(ada.IsExpanded, "Ada is expanded after ExpandAllRows.");

                var dan = FindTreeRow(tableView, "Dan");
                Verify.AreEqual(3, dan.Level, "Dan is a grandchild.");
                Verify.IsFalse(dan.IsExpandable, "Dan is a leaf.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ExpandAllRows and CollapseAllRows touch every node.")]
        public void VerifyExpandAllCollapseAll()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, FullTree, "after ExpandAllRows");
                tableView.CollapseAllRows();
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "after CollapseAllRows"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies expansion intent is per node: collapsing and re-expanding a parent keeps its descendants' state.")]
        public void VerifyNestedReexpandKeepsState()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            Toggle(tableView, "Ben", true);
            Toggle(tableView, "Ada", false);
            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, AdaBenExpanded, "Ben stays expanded under a re-expanded Ada"));

            Toggle(tableView, "Ben", false);
            Toggle(tableView, "Ada", false);
            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2+ Cy2 Eve1+ Gus1", "Ben stays collapsed under a re-expanded Ada"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearParentBy restores the flat projection in source order at Level 0.")]
        public void VerifyClearParentByReturnsFlat()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => source.ClearParentBy());
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, Flat, "after ClearParentBy");
                foreach (var row in GetProjectedElements(tableView).OfType<TableViewRow>())
                {
                    Verify.AreEqual(0, row.Level, $"Row {NameOf(row.DataContext)} is not in a hierarchy.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearParentBy drops every reference the hierarchy held to the items.")]
        public void VerifyClearParentByReleasesRows()
        {
            RunOnUIThread.Execute(() =>
            {
                TableViewSource source;
                ObservableCollection<TreeEmployee> list;
                List<WeakReference> weak;
                RetractAndEmpty(out source, out list, out weak);

                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();

                Verify.AreEqual(0, weak.Count(w => w.IsAlive), "No item may stay alive after ClearParentBy and Clear.");
                GC.KeepAlive(source);
                GC.KeepAlive(list);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies two TableViewSources over one collection keep independent trees.")]
        public void VerifyTwoRelationsOneSource()
        {
            TableView managerTable = null;
            TableView mentorTable = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var list = MakeTree();
                managerTable = CreateTreeTable(TableViewSource.From(list).ParentBy(ById, ByManager));
                mentorTable = CreateTreeTable(TableViewSource.From(list).ParentBy(ById, ByMentor));
                managerTable.Width = 200;
                mentorTable.Width = 200;

                // Side by side, so both tables are in the viewport and realize their rows.
                var panel = new Grid();
                panel.ColumnDefinitions.Add(new ColumnDefinition());
                panel.ColumnDefinitions.Add(new ColumnDefinition());
                Grid.SetColumn(mentorTable, 1);
                panel.Children.Add(managerTable);
                panel.Children.Add(mentorTable);
                LoadContent(panel);
            });
            SettleLayout(managerTable);
            SettleLayout(mentorTable);

            RunOnUIThread.Execute(() => VerifyTree(mentorTable, "Ada1+ Cy1 Dan1 Eve1+ Gus1", "mentor tree"));

            Toggle(mentorTable, "Ada", true);
            Toggle(mentorTable, "Eve", true);

            RunOnUIThread.Execute(() =>
            {
                VerifyTree(mentorTable, "Ada1- Fay2 Cy1 Dan1 Eve1- Ben2 Gus1", "mentor tree expanded");
                VerifyTree(managerTable, Roots, "manager tree is unaffected by the mentor tree");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a second ParentBy replaces the relation and resets expansion.")]
        public void VerifyRedeclareClearsIntent()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => source.ParentBy(ById, ByManager));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "after re-declaring ParentBy"));
        }

        #endregion

        #region 3.2 Key semantics

        [TestMethod]
        [TestProperty("Description", "Verifies keys compare by type and value: Int32 ids never link to Int64 parent keys.")]
        public void VerifyInt32VsInt64KeysDoNotLink()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
                source = TableViewSource.From(MakeTree()).ParentBy(ById, EmpKey(e => e.BigManagerId)));
            var tableView = Place(source);

            RunOnUIThread.Execute(() => VerifyTree(tableView, AllRoots, "Int32 keys, Int64 parent keys"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies WinRT enum keys, boxed afresh on every call, link by value.")]
        public void VerifyEnumKeys()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
                source = TableViewSource.From(MakeTree()).ParentBy(
                    EmpKey(e => (global::Windows.System.VirtualKey)e.Id),
                    EmpKey(e => e.ManagerId.HasValue ? (object)(global::Windows.System.VirtualKey)e.ManagerId.Value : null)));
            var tableView = Place(source);

            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, FullTree, "VirtualKey keys"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies reference-type keys link by identity, and fresh objects per call never link or collide.")]
        public void VerifyObjectKeysLinkByIdentity()
        {
            TableViewSource shared = null;
            RunOnUIThread.Execute(() =>
                shared = TableViewSource.From(MakeTree()).ParentBy(EmpKey(e => e), EmpKey(e => e.Manager)));
            var tableView = Place(shared);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "shared instances link"));

            TableViewSource fresh = null;
            RunOnUIThread.Execute(() =>
                fresh = TableViewSource.From(MakeTree()).ParentBy(
                    EmpKey(e => new object()),
                    EmpKey(e => e.ManagerId == null ? null : new object())));
            tableView = Place(fresh);
            RunOnUIThread.Execute(() => VerifyTree(tableView, AllRoots, "fresh instances never link"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies expansion intent is keyed by node key, not object identity.")]
        public void VerifyKeySurvivesRecreate()
        {
            ObservableCollection<TreeEmployee> list = null;
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = Tree(list);
            });
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => list[0] = new TreeEmployee { Id = 1, Name = "Ada", Dept = "Eng" });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2+ Cy2 Eve1+ Gus1", "Ada replaced by a same-key object"));
        }

        #endregion

        #region 3.3 Error contract

        [TestMethod]
        [TestProperty("Description", "Verifies ParentBy rejects a null key or parent key selector.")]
        public void VerifyNullSelectorThrows()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            RunOnUIThread.Execute(() =>
            {
                Verify.Throws<ArgumentException>(() => source.ParentBy(null, ByManager), "ParentBy(null, parent) must throw.");
                Verify.Throws<ArgumentException>(() => source.ParentBy(ById, null), "ParentBy(key, null) must throw.");
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "after the rejected calls"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies duplicate node keys throw, name the duplicate value, and leave the projection unchanged.")]
        public void VerifyDuplicateKeyThrows()
        {
            VerifyThrowsKeepsProjection(_ => { }, s => s.ParentBy(EmpKey(e => 1), ByManager), "duplicate key");

            RunOnUIThread.Execute(() =>
            {
                var message = ThrownMessage(() => TableViewSource.From(MakeTree()).ParentBy(
                    EmpKey(e => (global::Windows.System.VirtualKey)(e.Id == 7 ? 3 : e.Id)), ByManager));
                Verify.IsTrue(message.Contains("duplicate key '3'"), $"The message should name the duplicate value: {message}");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a null node key throws and leaves the projection unchanged.")]
        public void VerifyNullKeyThrows()
        {
            VerifyThrowsKeepsProjection(_ => { }, s => s.ParentBy(EmpKey(e => e.Id == 7 ? null : (object)e.Id), ByManager), "null key");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an item cannot be its own parent.")]
        public void VerifySelfParentThrows()
        {
            VerifyThrowsKeepsProjection(l => l[0].ManagerId = 1, s => s.ParentBy(ById, ByManager), "self parent");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cycle throws, names a cycle member even when a descendant is visited first, and leaves the projection unchanged.")]
        public void VerifyCycleThrows()
        {
            // Ada <-> Dan via Ben.
            VerifyThrowsKeepsProjection(l => l[0].ManagerId = 4, s => s.ParentBy(ById, ByManager), "cycle");

            RunOnUIThread.Execute(() =>
            {
                // Ada's subtree hangs below the Eve <-> Fay cycle and precedes it in source order.
                var list = MakeTree();
                list[0].ManagerId = 5;
                list[4].ManagerId = 6;
                var message = ThrownMessage(() => TableViewSource.From(list).ParentBy(ById, ByManager));
                Verify.IsTrue(message.Contains("involving key '5'") || message.Contains("involving key '6'"),
                    $"The message should name a member of the cycle: {message}");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a failed pass is not poisoned: data fixed during the failing pass is projected by the replayed refresh.")]
        public void VerifyRecoverAfterFix()
        {
            TableViewSource source = null;
            string thrown = null;

            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                var dup = new TreeEmployee { Id = 1, Name = "Dup", Dept = "Eng" };
                list.Add(dup);
                source = TableViewSource.From(list);
                var fixedOnce = false;
                var key = new TableViewKeySelector(o =>
                {
                    if (!fixedOnce)
                    {
                        fixedOnce = true;
                        list.Remove(dup);
                    }

                    return ((TreeEmployee)o).Id;
                });
                thrown = ThrownTypeName(() => source.ParentBy(key, ByManager));
            });
            var tableView = Place(source);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("ArgumentException", thrown, "The pass that saw the duplicate fails.");
                VerifyTree(tableView, Roots, "the fixed data projects");
            });
        }

        #endregion

        #region 3.4 Composition with sort, filter and group

        [TestMethod]
        [TestProperty("Description", "Verifies sort orders each sibling set and children stay under their parent.")]
        public void VerifySortWithinSiblings()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => source.Sort(ByName, SortDirection.Descending));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Gus1 Eve1+ Ada1- Cy2 Ben2+", "Name descending"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a sort reshape keeps expansion intent.")]
        public void VerifySortKeepsExpansion()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            Toggle(tableView, "Ben", true);
            RunOnUIThread.Execute(() => source.Sort(ByName, SortDirection.Ascending));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, AdaBenExpanded, "after sort"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a deep filter match brings its ancestor chain, auto-expanded.")]
        public void VerifyFilterKeepsAncestors()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            RunOnUIThread.Execute(() => source.Filter(NameIs("Dan")));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2- Dan3", "filter Dan"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies matches pull in ancestors, not descendants, and an empty result is valid.")]
        public void VerifyFilterParentMatchExcludesUnmatchedChildren()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            RunOnUIThread.Execute(() => source.Filter(NameIs("Ada")));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada1", "filter Ada, then ExpandAllRows");
                source.Filter(o => false);
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, GetProjectedCount(tableView), "A filter matching nothing projects zero rows.");
                VerifyTree(tableView, "", "filter matching nothing");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the filter auto-expand overlay is temporary: clearing the filter restores prior intent.")]
        public void VerifyFilterClearRestoresIntent()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            Toggle(tableView, "Eve", true);
            RunOnUIThread.Execute(() => source.Filter(NameIs("Dan")));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => source.ClearFilter());
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1+ Eve1- Fay2 Gus1", "after ClearFilter"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies collapsing a context row while filtered is overlay-only, while CollapseAllRows under a filter persists.")]
        public void VerifyFilteredTogglesAreOverlayOnly()
        {
            // Context collapse and re-expand within the filter.
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            RunOnUIThread.Execute(() => source.Filter(NameIs("Dan")));
            SettleLayout(tableView);
            Toggle(tableView, "Ada", false);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1+", "context Ada collapsed"));
            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2- Dan3", "context Ada re-expanded"));

            // Context collapse does not write through to intent.
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            tableView = Place(source);
            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => source.Filter(NameIs("Dan")));
            SettleLayout(tableView);
            Toggle(tableView, "Ada", false);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada1+", "context Ada collapsed");
                source.ClearFilter();
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2+ Cy2 Eve1+ Gus1", "original intent after ClearFilter"));

            // CollapseAllRows under a filter persists: clearing the filter shows the tree collapsed.
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            tableView = Place(source);
            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => source.Filter(NameIs("Dan")));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => tableView.CollapseAllRows());
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => source.ClearFilter());
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "CollapseAllRows under a filter, then ClearFilter"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies changing the filter discards context-row collapses and rebuilds the overlay.")]
        public void VerifyFilterReplaceAfterContextCollapse()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => source.Filter(NameIs("Dan")));
            SettleLayout(tableView);
            Toggle(tableView, "Ada", false);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada1+", "context Ada collapsed");
                source.Filter(NameIs("Cy"));
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Cy2", "new filter"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies grouping buckets roots only and descendants follow their root.")]
        public void VerifyGroupByBucketsRootsOnly()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()).GroupBy(ByDept));
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "[Eng] Ada1- Ben2+ Cy2 [Ops] Eve1+ Gus1", "grouped by Dept"));

            // Ben's own Dept is ignored: he stays under Ada, and header counts are root counts.
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                list[1].Dept = "Ops";
                source = Tree(list).GroupBy(ByDept);
            });
            tableView = Place(source);
            Toggle(tableView, "Ada", true);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual("[Eng 1] Ada1- Ben2+ Cy2 [Ops 2] Eve1+ Gus1", HeaderLabels(tableView), "Ben in Ops, grouped by Dept"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a sort under grouping keeps the header order of plain GroupBy, for either verb order.")]
        public void VerifyGroupedPostSortKeepsHeaderOrder()
        {
            Func<ObservableCollection<TreeEmployee>> pair = () => new ObservableCollection<TreeEmployee>(new[]
            {
                new TreeEmployee { Id = 1, Dept = "A", Name = "Zulu" },
                new TreeEmployee { Id = 2, Dept = "B", Name = "Alpha" },
            });

            var hierPost = PlacedNames(() => TableViewSource.From(pair()).ParentBy(ById, ByManager).GroupBy(ByDept).Sort(ByName, SortDirection.Ascending));
            var flatPost = PlacedNames(() => TableViewSource.From(pair()).GroupBy(ByDept).Sort(ByName, SortDirection.Ascending));
            var hierPre = PlacedNames(() => TableViewSource.From(pair()).ParentBy(ById, ByManager).Sort(ByName, SortDirection.Ascending).GroupBy(ByDept));
            var flatPre = PlacedNames(() => TableViewSource.From(pair()).Sort(ByName, SortDirection.Ascending).GroupBy(ByDept));

            Verify.AreEqual("Zulu Alpha", flatPost, "Plain GroupBy then Sort.");
            Verify.AreEqual(flatPost, hierPost, "ParentBy must not change header order when GroupBy precedes Sort.");
            Verify.AreEqual("Alpha Zulu", flatPre, "Plain Sort then GroupBy.");
            Verify.AreEqual(flatPre, hierPre, "ParentBy must not change header order when Sort precedes GroupBy.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies grouping and ungrouping an expanded tree keeps expansion.")]
        public void VerifyRegroupExpandedTree()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            Toggle(tableView, "Ben", true);
            RunOnUIThread.Execute(() => source.GroupBy(ByDept));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("[Eng 1] Ada1- Ben2- Dan3 Cy2 [Ops 2] Eve1+ Gus1", HeaderLabels(tableView), "Grouped.");
                source.ClearGroupBy();
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, AdaBenExpanded, "ungrouped"));
        }

        #endregion

        #region 3.5 Source collection changes

        [TestMethod]
        [TestProperty("Description", "Verifies an incremental add of a root lands at its source position, collapsed.")]
        public void VerifyAddRoot()
        {
            ObservableCollection<TreeEmployee> list = null;
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = Tree(list);
            });
            var tableView = Place(source);

            RunOnUIThread.Execute(() => list.Add(new TreeEmployee { Id = 8, Name = "Hal", Dept = "Ops" }));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots + " Hal1", "after adding root Hal"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies adding a child under a leaf makes the leaf expandable.")]
        public void VerifyAddChildUnderLeaf()
        {
            ObservableCollection<TreeEmployee> list = null;
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = Tree(list);
            });
            var tableView = Place(source);

            RunOnUIThread.Execute(() => list.Add(new TreeEmployee { Id = 9, ManagerId = 7, Name = "Ivy", Dept = "Ops" }));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1+ Eve1+ Gus1+", "Gus gained a child"));

            Toggle(tableView, "Gus", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1+ Eve1+ Gus1- Ivy2", "Gus expanded"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing a parent promotes its children to roots.")]
        public void VerifyRemoveParentOrphansChildren()
        {
            ObservableCollection<TreeEmployee> list = null;
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = Tree(list);
            });
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => list.RemoveAt(0));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ben1+ Cy1 Eve1+ Gus1", "after removing Ada"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Replace with a changed parent key moves the subtree.")]
        public void VerifyReplaceReparents()
        {
            ObservableCollection<TreeEmployee> list = null;
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = Tree(list);
            });
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => list[2] = new TreeEmployee { Id = 3, ManagerId = 5, Name = "Cy", Dept = "Eng" });
            SettleLayout(tableView);
            Toggle(tableView, "Eve", true);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2+ Eve1- Cy2 Fay2 Gus1", "Cy moved under Eve"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies unsorted siblings follow source order after a Move.")]
        public void VerifyMoveKeepsSiblingOrderBySource()
        {
            ObservableCollection<TreeEmployee> list = null;
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = Tree(list);
            });
            var tableView = Place(source);

            RunOnUIThread.Execute(() => list.Move(6, 0));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Gus1 Ada1+ Eve1+", "after moving Gus first"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a collection Reset keeps expansion intent for surviving keys.")]
        public void VerifyResetKeepsIntent()
        {
            ResettableCollection<TreeEmployee> list = null;
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = new ResettableCollection<TreeEmployee>(MakeTree());
                source = TableViewSource.From(list).ParentBy(ById, ByManager);
            });
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => list.ReplaceAll(list.ToList()));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2+ Cy2 Eve1+ Gus1", "after Reset"));
        }

        #endregion

        #region 3.6 Selection and focus across reshapes

        [TestMethod]
        [TestProperty("Description", "Verifies collapsing the parent of the selected row clears selection, and re-expanding does not restore it.")]
        public void VerifyCollapseDropsSelectionOfHiddenRow()
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            Toggle(tableView, "Ada", true);
            Toggle(tableView, "Ben", true);

            var changes = 0;
            RunOnUIThread.Execute(() =>
            {
                tableView.SelectionChanged += (s, e) => changes++;
                var elements = GetProjectedElements(tableView);
                var danIndex = elements.FindIndex(el => el is TableViewRow r && NameOf(r.DataContext) == "Dan");
                tableView.Select(danIndex);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Dan", SelectedName(tableView), "Dan is selected.");
                Verify.AreEqual(1, changes, "One SelectionChanged for the select.");
            });

            Toggle(tableView, "Ada", false);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("null", SelectedName(tableView), "Collapsing Ada hides Dan and clears selection.");
                Verify.AreEqual(2, changes, "One SelectionChanged for the clear.");
                tableView.ExpandAllRows();
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => source.Sort(ByName, SortDirection.Ascending));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("null", SelectedName(tableView), "Re-expanding and reshaping must not restore the selection.");
                Verify.AreEqual(2, changes, "No further SelectionChanged.");
            });
        }

        #endregion

        #region 3.7 Re-entrancy and robustness

        [TestMethod]
        [TestProperty("Description", "Verifies app code may call ClearParentBy or ParentBy from inside a rebuild (ExpandAllRows, key or sort selector) without a crash.")]
        public void VerifyReentrantClearParentFromSelector()
        {
            // ClearParentBy inside ExpandAllRows' presented change.
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            var tableView = Place(source);

            var fired = false;
            string thrown = null;
            RunOnUIThread.Execute(() =>
                thrown = OnFirstPresentedChange(tableView, () =>
                {
                    fired = true;
                    source.ClearParentBy();
                }, () => tableView.ExpandAllRows()));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(fired, "The presented change fired.");
                Verify.IsNull(thrown, "ExpandAllRows must not throw.");
                VerifyTree(tableView, Flat, "ClearParentBy during ExpandAllRows");
            });

            // A key selector re-declares the relation on its first call.
            string redeclareThrown = null;
            RunOnUIThread.Execute(() =>
            {
                TableViewSource redeclared = null;
                redeclared = TableViewSource.From(MakeTree());
                var once = false;
                var redeclaring = new TableViewKeySelector(o =>
                {
                    if (!once)
                    {
                        once = true;
                        redeclared.ParentBy(ById, ByManager);
                    }

                    return 1;
                });
                redeclareThrown = ThrownTypeName(() => redeclared.ParentBy(redeclaring, ByManager));
                source = redeclared;
            });
            tableView = Place(source);
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("no exception", redeclareThrown, "The obsolete build is discarded.");
                VerifyTree(tableView, FullTree, "the latest relation is projected");
            });

            // ClearParentBy inside a sort key selector.
            var armed = false;
            RunOnUIThread.Execute(() =>
            {
                TableViewSource sorted = null;
                sorted = Tree(MakeTree()).Sort(EmpKey(e =>
                {
                    if (armed)
                    {
                        armed = false;
                        sorted.ClearParentBy();
                    }

                    return e.Name;
                }), SortDirection.Ascending);
                source = sorted;
            });
            tableView = Place(source);
            string sortThrown = null;
            RunOnUIThread.Execute(() =>
            {
                armed = true;
                sortThrown = ThrownTypeName(() => source.Filter(o => true));
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("no exception", sortThrown, "The pass is dropped as obsolete.");
                VerifyTree(tableView, Flat, "ClearParentBy from a sort selector");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearParentBy during an incremental add or filter refresh does not crash and ends flat.")]
        public void VerifyReentrantClearParentDuringRefresh()
        {
            // During an incremental add.
            ObservableCollection<TreeEmployee> list = null;
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = Tree(list);
            });
            var tableView = Place(source);

            var fired = false;
            string thrown = null;
            RunOnUIThread.Execute(() =>
                thrown = OnFirstPresentedChange(tableView, () =>
                {
                    fired = true;
                    source.ClearParentBy();
                }, () => list.Add(new TreeEmployee { Id = 8, Name = "Hal", Dept = "Ops" })));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(fired, "The presented change fired (add).");
                Verify.IsNull(thrown, "The add must not throw.");
                VerifyTree(tableView, Flat + " Hal", "ClearParentBy during an add");
            });

            // During a filter refresh.
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            tableView = Place(source);
            fired = false;
            thrown = null;
            RunOnUIThread.Execute(() =>
                thrown = OnFirstPresentedChange(tableView, () =>
                {
                    fired = true;
                    source.ClearParentBy();
                }, () => source.Filter(o => ((TreeEmployee)o).Name != "Cy")));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(fired, "The presented change fired (filter).");
                Verify.IsNull(thrown, "The filter must not throw.");
                VerifyTree(tableView, "Ada Ben Dan Eve Fay Gus", "ClearParentBy during a filter");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies adding a root in a new group during a grouped expand shows the new group without a crash.")]
        public void VerifyReentrantAddNewGroupRootDuringExpand()
        {
            ObservableCollection<TreeEmployee> list = null;
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = Tree(list).GroupBy(ByDept);
            });
            var tableView = Place(source);

            var fired = false;
            string thrown = null;
            ItemsSourceView view = null;
            NotifyCollectionChangedEventHandler handler = (sender, args) =>
            {
                if (fired)
                {
                    return;
                }

                fired = true;
                list.Add(new TreeEmployee { Id = 8, Name = "Hal", Dept = "Qa" });
            };

            RunOnUIThread.Execute(() =>
            {
                view = GetRowsRepeater(tableView).ItemsSourceView;
                view.CollectionChanged += handler;
                thrown = ThrownTypeName(() => ToggleTreeRow(tableView, "Ada", true));
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => view.CollectionChanged -= handler);
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(fired, "The presented change fired.");
                Verify.AreEqual("no exception", thrown, "The expand must not throw.");
                VerifyTree(tableView, "[Eng] Ada1- Ben2+ Cy2 [Ops] Eve1+ Gus1 [Qa] Hal1", "Hal added in a new group");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a throwing reshape during ClearParentBy (flat, grouped, publication) surfaces once, leaves rows flat and inert, and the next call succeeds.")]
        public void VerifyClearParentTeardownSurvivesFailingReshape()
        {
            // Flat: a duplicate added during the teardown fails the flat refresh.
            ObservableCollection<TreeEmployee> list = null;
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = Tree(list);
            });
            var tableView = Place(source);

            var fired = false;
            string thrown = null;
            RunOnUIThread.Execute(() =>
                thrown = OnFirstPresentedChange(tableView, () =>
                {
                    fired = true;
                    source.ClearParentBy();
                    list.Add(list[0]);
                }, () => source.Filter(o => true)));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(fired, "Flat: the presented change fired.");
                Verify.AreEqual("ArgumentException", thrown, "Flat: the failure surfaces once.");
                VerifyTree(tableView, "Ada Eve Gus", "flat: after the failed refresh");
                Verify.AreEqual("none none none", ExpandStates(tableView), "Flat: rows are inert.");
                source.ClearParentBy();
                tableView.ExpandAllRows();
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada Eve Gus", "flat: again");
                list.RemoveAt(list.Count - 1);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Flat, "flat: duplicate removed"));

            // Grouped: group keys valid only on roots break the flat rebuild after ClearParentBy.
            var badDescendantKeys = true;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = Tree(list).GroupBy(EmpKey(e =>
                    badDescendantKeys && (e.Name == "Ben" || e.Name == "Cy" || e.Name == "Dan" || e.Name == "Fay")
                        ? new object()
                        : (object)e.Dept));
            });
            tableView = Place(source);

            fired = false;
            thrown = null;
            RunOnUIThread.Execute(() =>
                thrown = OnFirstPresentedChange(tableView, () =>
                {
                    fired = true;
                    source.ClearParentBy();
                }, () => source.Filter(o => true)));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(fired, "Grouped: the presented change fired.");
                Verify.AreEqual("ArgumentException", thrown, "Grouped: the failure surfaces once.");
                VerifyTree(tableView, "[Eng] Ada [Ops] Eve Gus", "grouped: after the failed refresh");
                Verify.AreEqual("none none none", ExpandStates(tableView), "Grouped: rows are inert.");
                source.ClearParentBy();
                tableView.ExpandAllRows();
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "[Eng] Ada [Ops] Eve Gus", "grouped: again");
                badDescendantKeys = false;
                source.Filter(o => true);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "[Eng] Ada Ben Cy Dan [Ops] Eve Fay Gus", "grouped: keys fixed"));

            // Publication: a SelectionChanged handler throws during the flat publication.
            RunOnUIThread.Execute(() => source = Tree(MakeTree()));
            tableView = Place(source);
            RunOnUIThread.Execute(() => source.Filter(NameIs("Dan")));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada1- Ben2- Dan3", "publication: before");
                tableView.Select(0);
            });
            SettleLayout(tableView);

            fired = false;
            thrown = null;
            RunOnUIThread.Execute(() =>
            {
                var table = tableView;
                global::Windows.Foundation.TypedEventHandler<TableView, SelectionChangedEventArgs> onSelectionChanged = null;
                onSelectionChanged = (s, e) =>
                {
                    table.SelectionChanged -= onSelectionChanged;
                    fired = true;
                    throw new InvalidOperationException("publication handler");
                };
                table.SelectionChanged += onSelectionChanged;
                thrown = ThrownTypeName(() => source.ClearParentBy());
                table.SelectionChanged -= onSelectionChanged;
            });
            SettleLayout(tableView);

            string again = null;
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(fired, "Publication: the handler ran.");
                Verify.AreEqual("InvalidOperationException", thrown, "Publication: the failure surfaces once.");
                VerifyTree(tableView, "Dan", "publication: after the failed publication");
                Verify.AreEqual("none", ExpandStates(tableView), "Publication: rows are inert.");
                again = ThrownTypeName(() => source.ClearParentBy());
                tableView.ExpandAllRows();
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("no exception", again, "Publication: the next ClearParentBy succeeds.");
                VerifyTree(tableView, "Dan", "publication: again");
                source.ClearFilter();
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Flat, "publication: filter cleared"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a request queued during a failed incremental change is posted, runs once, and the hierarchy returns.")]
        public void VerifyDeferredRequestSurvivesFailingIncrementalChange()
        {
            ObservableCollection<TreeEmployee> list = null;
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();

                // A verb in force, or the unshaped flat mirror would tolerate the duplicate.
                source = Tree(list).Filter(o => true);
            });
            var tableView = Place(source);

            var callbacks = 0;
            var armed = false;
            ItemsRepeater repeater = null;
            long token = 0;
            RunOnUIThread.Execute(() =>
            {
                repeater = GetRowsRepeater(tableView);
                token = repeater.RegisterPropertyChangedCallback(ItemsRepeater.ItemsSourceProperty, (s, dp) =>
                {
                    if (!armed)
                    {
                        return;
                    }

                    armed = false;
                    ++callbacks;
                    list.RemoveAt(list.Count - 1);
                    source.ParentBy(ById, ByManager);
                });

                // The engine error does not surface through list.Add; only the outcome is asserted.
                OnFirstPresentedChange(tableView, () =>
                {
                    armed = true;
                    source.ClearParentBy();
                    list.Add(list[0]);
                }, () => list.Add(new TreeEmployee { Id = 8, Name = "Hal", Dept = "Ops" }));
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                repeater.UnregisterPropertyChangedCallback(ItemsRepeater.ItemsSourceProperty, token);
                Verify.AreEqual(1, callbacks, "The deferred request runs exactly once.");
                Verify.AreEqual(8, list.Count, "The duplicate is gone and Hal stays.");
                VerifyTree(tableView, Roots + " Hal1", "the hierarchy returns");
            });
        }

        #endregion

        #region 3.8 Chevron and template

        [TestMethod]
        [TestProperty("Description", "Verifies the chevron gutter is clipped to the lead cell at every level and is not hit-testable past it.")]
        public void VerifyChevronClippedToLeadCell()
        {
            var tableView = PlaceNarrowLeadColumn();

            RunOnUIThread.Execute(() =>
            {
                foreach (var name in new[] { "Ada", "Ben", "Dan", "Cy" })
                {
                    Verify.AreEqual("ok", CheckChevronClip(tableView, name), $"Chevron clip for {name}.");
                }

                // Dan's indent exceeds the 30px lead column, so his chevron is fully clipped.
                Verify.IsFalse(GetChevron(tableView, "Dan").IsHitTestVisible, "Dan's chevron is past the lead cell.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the chevron clip tracks the lead column being hidden, shown and widened.")]
        public void VerifyChevronClipTracksColumns()
        {
            var tableView = PlaceNarrowLeadColumn();

            RunOnUIThread.Execute(() =>
            {
                foreach (var column in tableView.Columns)
                {
                    column.Visibility = Visibility.Collapsed;
                }
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                var ada = GetChevron(tableView, "Ada");
                Verify.IsNotNull(ada.Clip, "Hidden: the chevron is clipped.");
                Verify.AreEqual(0.0, ada.Clip.Rect.Width, "Hidden: the clip has zero width.");
                Verify.IsFalse(ada.IsHitTestVisible, "Hidden: the chevron is not hit-testable.");
                tableView.Columns[0].Visibility = Visibility.Visible;
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("ok", CheckChevronClip(tableView, "Ada"), "Shown: chevron clip for Ada.");
                tableView.Columns[0].Width = new GridLength(200);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("ok", CheckChevronClip(tableView, "Dan"), "Widened: chevron clip for Dan.");
                Verify.AreEqual("ok", CheckChevronClip(tableView, "Ben"), "Widened: chevron clip for Ben.");
                Verify.IsTrue(GetChevron(tableView, "Dan").IsHitTestVisible, "Widened: Dan's chevron is hit-testable.");
            });
        }

        #endregion

        #region Helpers

        private static readonly TableViewKeySelector ByName = EmpKey(e => e.Name);
        private static readonly TableViewKeySelector ByDept = EmpKey(e => e.Dept);

        private static TableViewSource Tree(ObservableCollection<TreeEmployee> list)
            => TableViewSource.From(list).ParentBy(ById, ByManager);

        private static TableViewPredicate NameIs(string name)
            => new TableViewPredicate(o => ((TreeEmployee)o).Name == name);

        // Loads a tree table over source as the test content and settles it. Call off the UI thread.
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

        // Data-row names of a freshly placed table over the source, header rows skipped.
        private string PlacedNames(Func<TableViewSource> makeSource)
        {
            TableViewSource source = null;
            RunOnUIThread.Execute(() => source = makeSource());
            var tableView = Place(source);

            string names = null;
            RunOnUIThread.Execute(() => names = string.Join(" ",
                GetProjectedElements(tableView).OfType<TableViewRow>().Select(r => NameOf(r.DataContext))));
            return names;
        }

        // TreeLabels with group headers written as [Key ItemCount].
        private static string HeaderLabels(TableView tableView)
            => string.Join(" ", GetProjectedElements(tableView).Select(element =>
            {
                if (element is TableViewRow row)
                {
                    return RowLabel(row);
                }

                var info = (element as TableViewGroupHeader)?.Content as TableViewGroupInfo;
                return info == null ? "[H]" : $"[{info.Key} {info.ItemCount}]";
            }));

        private static string SelectedName(TableView tableView)
            => tableView.SelectedItem == null ? "null" : NameOf(tableView.SelectedItem);

        // ExpandCollapse state of each projected row's peer, or "none" when the pattern is absent.
        private static string ExpandStates(TableView tableView)
            => string.Join(" ", GetProjectedElements(tableView).OfType<TableViewRow>().Select(row =>
            {
                IExpandCollapseProvider provider = GetRowExpandCollapse(row);
                return provider == null ? "none" : provider.ExpandCollapseState.ToString();
            }));

        private static string ThrownMessage(Action action)
        {
            try
            {
                action();
                return "no exception";
            }
            catch (Exception ex)
            {
                return ex.Message;
            }
        }

        // Runs act with a one-shot handler on the presented row view's CollectionChanged. Returns the
        // exception type name if act threw, else null.
        private static string OnFirstPresentedChange(TableView tableView, Action onFirst, Action act)
        {
            var view = GetRowsRepeater(tableView).ItemsSourceView;
            var fired = false;
            NotifyCollectionChangedEventHandler handler = (sender, args) =>
            {
                if (fired)
                {
                    return;
                }

                fired = true;
                onFirst();
            };

            view.CollectionChanged += handler;
            try
            {
                act();
                return null;
            }
            catch (Exception ex)
            {
                return ex.GetType().Name;
            }
            finally
            {
                view.CollectionChanged -= handler;
            }
        }

        // Declares a relation, retracts it, then empties the source. Not inlined, so no local here
        // keeps an item alive for the caller's GC.
        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void RetractAndEmpty(out TableViewSource source, out ObservableCollection<TreeEmployee> list, out List<WeakReference> weak)
        {
            list = MakeTree();
            weak = list.Select(e => new WeakReference(e)).ToList();
            source = Tree(list);
            source.ClearParentBy();
            list.Clear();
        }

        // On a table showing Roots: mutate the data, declare a throwing relation, and verify the throw
        // is ArgumentException and the projection is unchanged.
        private void VerifyThrowsKeepsProjection(
            Action<ObservableCollection<TreeEmployee>> mutate,
            Func<TableViewSource, TableViewSource> declare,
            string context)
        {
            ObservableCollection<TreeEmployee> list = null;
            TableViewSource source = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                source = Tree(list);
            });
            var tableView = Place(source);

            string thrown = null;
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, Roots, $"{context}: before");
                mutate(list);
                thrown = ThrownTypeName(() => declare(source));
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("ArgumentException", thrown, $"{context}: ParentBy must throw E_INVALIDARG.");
                VerifyTree(tableView, Roots, $"{context}: unchanged after the throw");
            });
        }

        // A 30px Name lead column plus a star Dept column, fully expanded.
        private TableView PlaceNarrowLeadColumn()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                tableView = CreateTreeTable(Tree(MakeTree()));
                tableView.Columns[0].Width = new GridLength(30);
                tableView.Columns.Add(MakeTextColumn("Dept", "Dept", new GridLength(1, GridUnitType.Star)));
                LoadContent(tableView);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);
            return tableView;
        }

        private static Border GetChevron(TableView tableView, string name)
        {
            var gutter = FindTreeRow(tableView, name).FindVisualChildByName("PART_RowExpanderGutter") as Border;
            Verify.IsNotNull(gutter, $"Row {name} should have PART_RowExpanderGutter.");
            return gutter;
        }

        // "ok" when the gutter clip is the lead column width past the gutter's indent and hit-testing
        // matches a non-empty clip.
        private static string CheckChevronClip(TableView tableView, string name)
        {
            var gutter = GetChevron(tableView, name);
            var expected = Math.Max(0.0, tableView.Columns[0].ActualWidth - gutter.Margin.Left);
            var clip = gutter.Clip;
            var ok = clip != null && Math.Abs(clip.Rect.Width - expected) < 0.5 && gutter.IsHitTestVisible == expected > 0;
            return ok
                ? "ok"
                : $"{name} clip={(clip == null ? "null" : clip.Rect.Width.ToString())} expected={expected} hit={gutter.IsHitTestVisible}";
        }

        #endregion
    }
}
