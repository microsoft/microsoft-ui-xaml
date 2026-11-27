// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Runtime.CompilerServices;
using Windows.Foundation;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Hierarchical rows: a TableView over a view that builds a tree (CollectionViewSource.HierarchyDescription),
    // TableView.ExpandRow / CollapseRow / ExpandAllRows / CollapseAllRows and TableViewRow.Level / IsExpandable /
    // IsExpanded.
    //
    // Fixture (MakeTree, PropertyHierarchyDescription("Id", "ManagerId")):
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
        [TestProperty("Description", "Verifies the first projection of a table over a hierarchical view shows only roots, collapsed.")]
        public void VerifyKeyPathsProjectRootsCollapsed()
        {
            var tableView = Place(MakeTree());

            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "first projection"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a null parent key, or one that matches no item, makes the row a root and no row is dropped.")]
        public void VerifyRootsAndOrphansAreRoots()
        {
            var tableView = Place(MakeTree());

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
            var tableView = Place(MakeTree(), "Code", "ParentCode");

            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "ParentCode \"\" on Ada and Eve"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies setting and clearing CollectionViewSource.HierarchyDescription on a loaded table switches its rows between flat and tree over the same view.")]
        public void VerifyHierarchyDescriptionTogglesOnTheSameView()
        {
            CollectionViewSource cvs = null;
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                cvs = new CollectionViewSource { Source = MakeTree() };
                tableView = CreateTreeTable(cvs.View);
                LoadContent(tableView);
            });
            SettleLayout(tableView);
            object view = null;
            RunOnUIThread.Execute(() =>
            {
                view = cvs.View;
                VerifyTree(tableView, Flat, "no description");
                Verify.IsNull(((ICollectionViewHierarchy)view).CollectionNodes, "A flat view has no nodes.");
                cvs.HierarchyDescription = new PropertyHierarchyDescription(IdPath, ManagerPath);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(view, cvs.View, "The view stays.");
                VerifyTree(tableView, Roots, "description set after ItemsSource");
                cvs.HierarchyDescription = null;
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Flat, "description cleared"));
        }
        [TestMethod]
        [TestProperty("Description", "Verifies TableViewRow Level, IsExpandable and IsExpanded reflect node state after ExpandAllRows.")]
        public void VerifyLevelAndExpandableDPs()
        {
            var tableView = Place(MakeTree());

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
            var tableView = Place(MakeTree());

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
            var tableView = Place(MakeTree());

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
        [TestProperty("Description", "Verifies clearing the view's HierarchyDescription restores the flat projection in source order at Level 0.")]
        public void VerifyClearingHierarchyDescriptionReturnsFlat()
        {
            var tableView = Place(MakeTree());

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => TreeSourceOf(tableView).HierarchyDescription = null);
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, Flat, "after clearing the HierarchyDescription");
                foreach (var row in GetProjectedElements(tableView).OfType<TableViewRow>())
                {
                    Verify.AreEqual(0, row.Level, $"Row {NameOf(row.DataContext)} is not in a hierarchy.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies clearing the relation drops every reference the hierarchy held to the items.")]
        public void VerifyClearingHierarchyDescriptionReleasesRows()
        {
            TableView tableView = null;
            ObservableCollection<TreeEmployee> list = null;
            List<WeakReference> weak = null;
            RunOnUIThread.Execute(() => RetractAndEmpty(out tableView, out list, out weak));

            // The table applies source changes on a later dispatcher turn.
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();

                var alive = string.Join(" ", weak.Where(w => w.IsAlive).Select(w => ((TreeEmployee)w.Target)?.Name));
                Verify.AreEqual(0, weak.Count(w => w.IsAlive), $"No item may stay alive after clearing the relation and the list. Alive: [{alive}]");
                GC.KeepAlive(tableView);
                GC.KeepAlive(list);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies two TableViews over one collection keep independent trees.")]
        public void VerifyTwoRelationsOneSource()
        {
            TableView managerTable = null;
            TableView mentorTable = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var list = MakeTree();
                managerTable = CreateTreeTable(list, IdPath, ManagerPath);
                mentorTable = CreateTreeTable(list, IdPath, "MentorId");
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
        [TestProperty("Description", "Verifies a new description with the same relation, or a key path change on the description in use, rebuilds the tree and keeps expansion by key.")]
        public void VerifyNewDescriptionKeepsIntentByKey()
        {
            var tableView = Place(MakeTree());

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => TreeSourceOf(tableView).HierarchyDescription = new PropertyHierarchyDescription(IdPath, ManagerPath));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada1- Ben2+ Cy2 Eve1+ Gus1", "a new description of the same relation");
                ((PropertyHierarchyDescription)TreeSourceOf(tableView).HierarchyDescription).ParentKeyPath = "MentorId";
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Fay2 Cy1 Dan1 Eve1+ Gus1", "ParentKeyPath changed to MentorId"));
        }
        #endregion

        #region 3.2 Key semantics

        [TestMethod]
        [TestProperty("Description", "Verifies keys compare by type and value: Int32 ids never link to Int64 parent keys.")]
        public void VerifyInt32VsInt64KeysDoNotLink()
        {
            var tableView = Place(MakeTree(), IdPath, "BigManagerId");

            RunOnUIThread.Execute(() => VerifyTree(tableView, AllRoots, "Int32 keys, Int64 parent keys"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a dotted ParentKeyPath reads the parent key through a reference, and a null link is a root.")]
        public void VerifyDottedParentKeyPath()
        {
            var tableView = Place(MakeTree(), "Code", "Manager.Code");
            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "dotted parent path"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies expansion intent is keyed by node key, not object identity.")]
        public void VerifyKeySurvivesRecreate()
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => list[0] = new TreeEmployee { Id = 1, Name = "Ada", Dept = "Eng" });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2+ Cy2 Eve1+ Gus1", "Ada replaced by a same-key object"));
        }

        #endregion

        #region 3.3 Bad data degrades

        [TestMethod]
        [TestProperty("Description", "Verifies a key an earlier item has makes the later item a root with no children, without an exception.")]
        public void VerifyDuplicateKeyIsRoot()
        {
            ObservableCollection<TreeEmployee> list = null;
            string thrown = null;
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);
            RunOnUIThread.Execute(() => thrown = ThrownTypeName(() => list.Add(new TreeEmployee { Id = 1, ManagerId = 5, Name = "Zed", Dept = "Ops" })));
            SettleLayout(tableView);
            Toggle(tableView, "Ada", true);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("no exception", thrown, "A duplicate key does not throw.");
                VerifyTree(tableView, "Ada1- Ben2+ Cy2 Eve1+ Gus1 Zed1", "Zed duplicates Ada's key");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an item with no key (empty string) cannot be a parent: the items that name it are roots.")]
        public void VerifyEmptyKeyCannotBeParent()
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                list[0].Code = "";
            });
            var tableView = Place(list, "Code", "ParentCode");

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1 Ben1+ Cy1 Eve1+ Gus1", "Ada has no Code"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an item that is its own parent is a root.")]
        public void VerifySelfParentIsRoot()
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                list[0].BossId = 1;
            });
            var tableView = Place(list, IdPath, "BossId");

            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "Ada is her own boss"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cycle is broken at the member that comes first in the source, which becomes a root.")]
        public void VerifyCycleBreaksAtFirstSourceItem()
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                // Ben's boss is Dan, Dan's is Ben.
                list[1].BossId = 4;
            });
            var tableView = Place(list, IdPath, "BossId");
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Cy2 Ben1- Dan2 Eve1- Fay2 Gus1", "Ben and Dan form a cycle"));
        }

        #endregion
        #region 3.4 Composition with sort, filter and group

        [TestMethod]
        [TestProperty("Description", "Verifies a header sort orders each sibling set and children stay under their parent.")]
        public void VerifySortWithinSiblings()
        {
            var tableView = Place(MakeTree());

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Gus1 Eve1+ Ada1- Cy2 Ben2+", "Name descending"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header sort keeps expansion intent.")]
        public void VerifySortKeepsExpansion()
        {
            var tableView = Place(MakeTree());

            Toggle(tableView, "Ada", true);
            Toggle(tableView, "Ben", true);
            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, AdaBenExpanded, "after sort"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header sort on a tree over a CollectionViewSource of a plain list orders each sibling set, leaves the list's order alone, and places a later child by its key.")]
        public void VerifyHeaderSortKeepsListOrder()
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending), "Name descending sorts."));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Gus1 Eve1+ Ada1- Cy2 Ben2+", "Name descending");
                Verify.AreEqual("Ada Ben Cy Dan Eve Fay Gus", string.Join(" ", list.Select(e => e.Name)), "The list keeps its order.");
                list.Add(new TreeEmployee { Id = 8, ManagerId = 1, Name = "Bo", Dept = "Eng" });
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Gus1 Eve1+ Ada1- Cy2 Bo2 Ben2+", "a new child lands at its sorted place"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header sort on a tree over a CollectionViewSource View writes the view's SortDescriptions and orders each sibling set.")]
        public void VerifyHeaderSortOnCollectionViewTreeSortsTheView()
        {
            Microsoft.UI.Xaml.Data.CollectionViewSource cvs = null;
            RunOnUIThread.Execute(() => cvs = MakeShapingView(MakeTree()));
            object view = null;
            RunOnUIThread.Execute(() => view = cvs.View);
            var tableView = Place(view);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending), "Name descending sorts."));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, cvs.SortDescriptions.Count, "The sort is the view's.");
                Verify.AreEqual("Name", cvs.SortDescriptions[0].PropertyName);
                VerifyTree(tableView, "Gus1 Eve1+ Ada1- Cy2 Ben2+", "Name descending");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a CollectionViewSource sort orders each sibling set, since siblings follow the view's order.")]
        public void VerifyCollectionViewSortOrdersSiblings()
        {
            object view = null;
            RunOnUIThread.Execute(() => view = MakeShapingView(MakeTree(), sortPath: "Name", direction: CollectionViewSortDirection.Descending).View);
            var tableView = Place(view);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Gus1 Eve1+ Ada1- Cy2 Ben2+", "view sorted by Name descending"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a filter keeps the matches and their ancestors (as context rows, expanded so the matches show), and a filtered-out parent does not orphan its children.")]
        public void VerifyFilterKeepsAncestorsAsContext()
        {
            object view = null;
            RunOnUIThread.Execute(() =>
            {
                var cvs = MakeShapingView(MakeTree());
                ((ICollectionViewShaping)cvs.View).Filter = o => ((TreeEmployee)o).Name == "Dan" || ((TreeEmployee)o).Name == "Gus";
                view = cvs.View;
            });
            var tableView = Place(view);

            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada1- Ben2- Dan3 Gus1", "Dan and Gus match; Ada and Ben are context");
                Verify.AreEqual("Ada* Ben* Dan Gus", NodeLabels(view), "The view's nodes.");
                Verify.AreEqual(4, ((IList<object>)view).Count, "The view's list holds the context rows.");
            });

            Toggle(tableView, "Ben", false);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada1- Ben2+ Gus1", "a context row collapses");
                ((ICollectionViewShaping)view).Filter = null;
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "without a filter, collapsed baseline"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a view cannot group and build a hierarchy at once: adding the second rule throws, whichever comes first.")]
        public void VerifyGroupingWithHierarchyThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                var cvs = MakeTreeSource(MakeTree());
                var view = cvs.View;
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => cvs.GroupDescriptions.Add(new PropertyGroupDescription("Dept"))),
                    "Grouping a hierarchical view throws.");

                var grouped = MakeShapingView(MakeTree(), "Dept");
                var groupedView = grouped.View;
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => ((ICollectionViewHierarchy)groupedView).HierarchyDescription = new PropertyHierarchyDescription(IdPath, ManagerPath)),
                    "A hierarchy on a grouping view throws.");
                Verify.IsNull(((ICollectionViewHierarchy)groupedView).CollectionNodes, "The grouped view stays flat.");
            });
        }
        #endregion

        #region 3.5 Source collection changes

        [TestMethod]
        [TestProperty("Description", "Verifies an incremental add of a root lands at its source position, collapsed.")]
        public void VerifyAddRoot()
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);

            RunOnUIThread.Execute(() => list.Add(new TreeEmployee { Id = 8, Name = "Hal", Dept = "Ops" }));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots + " Hal1", "after adding root Hal"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies adding a child under a leaf makes the leaf expandable.")]
        public void VerifyAddChildUnderLeaf()
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);

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
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);

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
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => list[2] = new TreeEmployee { Id = 3, ManagerId = 5, Name = "Cy", Dept = "Eng" });
            SettleLayout(tableView);
            Toggle(tableView, "Eve", true);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2+ Eve1- Cy2 Fay2 Gus1", "Cy moved under Eve"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Replace with the same object (a refresh) updates its row in place, for a leaf and for an expanded parent, without rebuilding the tree.")]
        public void VerifySameObjectReplaceRefreshesRowInPlace()
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);

            Toggle(tableView, "Ada", true);
            var actions = new List<string>();
            RunOnUIThread.Execute(() =>
            {
                GetRowsRepeater(tableView).ItemsSourceView.CollectionChanged += (s, e) => actions.Add(e.Action.ToString());
                list[0] = list[0];
                list[2] = list[2];
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Replace,Replace", string.Join(",", actions), "Each row is refreshed in place.");
                VerifyTree(tableView, "Ada1- Ben2+ Cy2 Eve1+ Gus1", "after refreshing Ada and Cy");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a same-object Replace in the turn the tree read that row's keys reads its new parent key: the row read last at load (Gus) is reparented and moves under its new parent.")]
        public void VerifySameObjectReplaceInLoadTurnReadsNewParentKey()
        {
            ObservableCollection<TreeEmployee> list = null;
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                list = MakeTree();
                tableView = CreateTreeTable(list, IdPath, ManagerPath);
                list[6].ManagerId = 5;
                list[6] = list[6];
                LoadContent(tableView);
            });
            SettleLayout(tableView);
            Toggle(tableView, "Eve", true);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1+ Eve1- Fay2 Gus2", "Gus moved under Eve"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies unsorted siblings follow source order after a Move.")]
        public void VerifyMoveKeepsSiblingOrderBySource()
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);

            RunOnUIThread.Execute(() => list.Move(6, 0));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Gus1 Ada1+ Eve1+", "after moving Gus first"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a collection Reset keeps expansion intent for surviving keys.")]
        public void VerifyResetKeepsIntent()
        {
            ResettableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() => list = new ResettableCollection<TreeEmployee>(MakeTree()));
            var tableView = Place(list);

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
            var tableView = Place(MakeTree());

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
            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));
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
        [TestProperty("Description", "Verifies clearing the HierarchyDescription from a handler of an incremental add does not crash and ends flat.")]
        public void VerifyReentrantClearParentDuringAdd()
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);

            var fired = false;
            string thrown = null;
            RunOnUIThread.Execute(() =>
                thrown = OnFirstPresentedChange(tableView, () =>
                {
                    fired = true;
                    TreeSourceOf(tableView).HierarchyDescription = null;
                }, () => list.Add(new TreeEmployee { Id = 8, Name = "Hal", Dept = "Ops" })));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(fired, "The presented change fired (add).");
                Verify.IsNull(thrown, "The add must not throw.");
                VerifyTree(tableView, Flat + " Hal", "HierarchyDescription cleared during an add");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a rows repeater handler (ElementClearing, ElementIndexChanged) that clears and declares the relation again while a collapse removes a node's visible descendants (three rows) ends the collapse safely: nothing is thrown, and the rows are the new relation's collapsed roots.")]
        public void VerifyRelationRedeclaredFromRepeaterHandlerDuringCollapse()
        {
            var tableView = Place(MakeTree());
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);

            var armed = false;
            var fired = 0;
            UnhandledErrors unhandled = null;
            try
            {
                RunOnUIThread.Execute(() =>
                {
                    VerifyTree(tableView, FullTree, "precondition: every row expanded");
                    unhandled = new UnhandledErrors();
                    Action redeclare = () =>
                    {
                        if (!armed)
                        {
                            return;
                        }
                        armed = false;
                        fired++;
                        TreeSourceOf(tableView).HierarchyDescription = null;
                        TreeSourceOf(tableView).HierarchyDescription = new PropertyHierarchyDescription(IdPath, ManagerPath);
                    };
                    var repeater = GetRowsRepeater(tableView);
                    repeater.ElementClearing += (s, e) => redeclare();
                    repeater.ElementIndexChanged += (s, e) => redeclare();
                    armed = true;
                    // Ada's visible descendants (Ben, Dan, Cy) leave one row at a time; the first notification re-declares.
                    ToggleTreeRow(tableView, "Ada", false);
                });
                SettleLayout(tableView);

                RunOnUIThread.Execute(() =>
                {
                    unhandled.Dispose();
                    Verify.AreEqual(1, fired, "A repeater handler re-declared the relation during the collapse.");
                    Verify.AreEqual(0, unhandled.Messages.Count, $"Nothing is thrown: {string.Join(" | ", unhandled.Messages)}");
                    VerifyTree(tableView, "Ada1+ Eve1- Fay2 Gus1", "a relation declared again keeps expansion by key");
                    Verify.AreEqual(4, GetProjectedCount(tableView), "Each row is projected once.");
                });
            }
            finally
            {
                // Detached even when a Verify above failed, so the handler cannot hide a later test's exceptions.
                RunOnUIThread.Execute(() => unhandled?.Dispose());
            }
            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1", "the new tree expands"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a rows repeater handler that clears the relation while a Replace removes the old row ends the Replace before it inserts the new row into the released tree: nothing is thrown, and the rows end flat with the new row.")]
        public void VerifyRelationClearedFromRepeaterHandlerDuringReplace()
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);

            var armed = false;
            var fired = 0;
            string thrown = null;
            UnhandledErrors unhandled = null;
            try
            {
                RunOnUIThread.Execute(() =>
                {
                    VerifyTree(tableView, FullTree, "precondition: every row expanded");
                    unhandled = new UnhandledErrors();
                    Action clear = () =>
                    {
                        if (!armed)
                        {
                            return;
                        }
                        armed = false;
                        fired++;
                        TreeSourceOf(tableView).HierarchyDescription = null;
                    };
                    var repeater = GetRowsRepeater(tableView);
                    repeater.ElementClearing += (s, e) => clear();
                    repeater.ElementIndexChanged += (s, e) => clear();
                    armed = true;
                    // Dan is a visible leaf: the Replace removes its row, then inserts Dov's.
                    thrown = ThrownTypeName(() => list[3] = new TreeEmployee { Id = 8, ManagerId = 2, Name = "Dov", Dept = "Eng" });
                });
                SettleLayout(tableView);

                RunOnUIThread.Execute(() =>
                {
                    unhandled.Dispose();
                    Verify.AreEqual(1, fired, "A repeater handler cleared the relation during the Replace.");
                    Verify.AreEqual("no exception", thrown, "The Replace does not throw.");
                    Verify.AreEqual(0, unhandled.Messages.Count, $"Nothing is thrown later: {string.Join(" | ", unhandled.Messages)}");
                    VerifyTree(tableView, "Ada Ben Cy Dov Eve Fay Gus", "the relation cleared during the Replace");
                });
            }
            finally
            {
                // Detached even when a Verify above failed, so the handler cannot hide a later test's exceptions.
                RunOnUIThread.Execute(() => unhandled?.Dispose());
            }
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
                Verify.IsTrue(FindTreeRow(tableView, "Dan").IsExpandable, "Precondition: Dan is expandable.");
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
                Verify.AreEqual("ok", CheckChevronClip(tableView, "Hal"), "Widened: chevron clip for Hal.");
                Verify.IsFalse(GetChevron(tableView, "Hal").IsHitTestVisible, "Widened: leaf Hal's gutter stays non-hit-testable.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a right-click on a leaf row's chevron gutter opens the lead cell menu, while an expandable row's gutter keeps the row menu, across leaf/expandable changes and recycling.")]
        public void VerifyLeafGutterRequestOpensCellMenu()
        {
            var list = MakeLiveTree();
            var recorder = new MenuRecorder();
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                tableView = CreateTreeTable(list, IdPath, ParentIdPath);
                tableView.Columns[0].CellContextFlyout = recorder.Create("CellMenu");
                tableView.Resources[typeof(TableViewRow)] = new Style(typeof(TableViewRow))
                {
                    Setters = { new Setter(UIElement.ContextFlyoutProperty, recorder.Create("RowMenu")) },
                };
                LoadContent(tableView);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ann1- Bob2 Cat2 Dee1- Eli2", "expanded"));

            VerifyGutterRequest(tableView, recorder, "Ann", expandable: true, "expandable root");
            VerifyGutterRequest(tableView, recorder, "Bob", expandable: false, "leaf child");

            // Bob gains a child: leaf -> expandable on the same container.
            var fin = new LiveRow { Id = 6, Name = "Fin", Score = 1, Team = "X", ParentId = 2 };
            RunOnUIThread.Execute(() => list.Add(fin));
            SettleLayout(tableView);
            VerifyGutterRequest(tableView, recorder, "Bob", expandable: true, "Bob after gaining Fin");
            VerifyGutterRequest(tableView, recorder, "Fin", expandable: false, "new leaf grandchild");

            // And back: expandable -> leaf.
            RunOnUIThread.Execute(() => list.Remove(fin));
            SettleLayout(tableView);
            VerifyGutterRequest(tableView, recorder, "Bob", expandable: false, "Bob after losing Fin");

            // Scroll a taller tree so containers are recycled; one that hosted an expandable row must
            // now host a leaf, and its gutter must follow. Roots have zero to two children at random:
            // with exactly one child each, rows alternate expandable/leaf, and when the realized row
            // count is even a container that leaves the top is reused for a row of the same kind.
            RunOnUIThread.Execute(() =>
            {
                var random = new Random(12654);
                for (var i = 0; i < 40; i++)
                {
                    list.Add(new LiveRow { Id = 100 + i, Name = $"R{i}", Score = 100 + i, Team = "Z" });
                    for (var j = random.Next(3); j > 0; j--)
                    {
                        list.Add(new LiveRow { Id = 1000 + i * 10 + j, Name = $"C{i}.{j}", Score = j, Team = "Z", ParentId = 100 + i });
                    }
                }
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);

            var wasExpandable = new Dictionary<TableViewRow, object>();
            RunOnUIThread.Execute(() =>
            {
                foreach (var row in GetRealizedRows(tableView).Where(r => r.IsExpandable))
                {
                    wasExpandable[row] = row.DataContext;
                }
            });

            string reusedName = null;
            TableViewRow reused = null;
            double scrollable = 0;
            RunOnUIThread.Execute(() => scrollable = GetBodyScroller(tableView).ScrollableHeight);
            for (var offset = 400.0; offset < scrollable && reusedName == null; offset += 400.0)
            {
                ScrollBodyTo(tableView, ScrollAxis.Vertical, offset);
                RunOnUIThread.Execute(() =>
                {
                    var viewport = GetBodyScroller(tableView);
                    var bounds = viewport.TransformToVisual(null).TransformBounds(new Rect(0, 0, viewport.ActualWidth, viewport.ActualHeight));
                    foreach (var pair in wasExpandable)
                    {
                        var row = pair.Key;
                        var item = row.DataContext;
                        if (item == null || ReferenceEquals(item, pair.Value) || row.IsExpandable ||
                            !ReferenceEquals(GetRowForItem(tableView, item), row))
                        {
                            continue;
                        }

                        var rowBounds = row.TransformToVisual(null).TransformBounds(new Rect(0, 0, row.ActualWidth, row.ActualHeight));
                        if (rowBounds.Top >= bounds.Top && rowBounds.Bottom <= bounds.Bottom)
                        {
                            reusedName = NameOf(item);
                            reused = row;
                            break;
                        }
                    }
                });
            }

            Verify.IsNotNull(reusedName, "Precondition: a container that hosted an expandable row must be reused for a visible leaf.");
            VerifyGutterRequest(tableView, recorder, reusedName, expandable: false, "container recycled from expandable to leaf", reused);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies section-only alignment leaves ParentBy first-cell and chevron indentation unchanged at all three levels, with or without sections and in LTR/RTL.")]
        public void VerifySectionAlignmentDoesNotChangeParentByIndentation()
        {
            CollectionViewSource source = null;
            ICollectionView view = null;
            RunOnUIThread.Execute(() =>
            {
                source = MakeShapingView(MakeTree());
                view = source.View;
            });
            var table = Place(view);
            foreach (var grouped in new[] { false })
            foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
            foreach (var density in new[] { TableViewDensity.Comfortable, TableViewDensity.Standard, TableViewDensity.Compact })
            {
                RunOnUIThread.Execute(() =>
                {
                    table.FlowDirection = flow;
                    table.Density = density;
                    source.GroupDescriptions.Clear();
                    if (grouped) { source.GroupDescriptions.Add(new PropertyGroupDescription("Dept")); }
                    table.ExpandAllRows();
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    foreach (var name in new[] { "Ada", "Ben", "Dan" })
                    {
                        var row = FindTreeRow(table, name);
                        var indent = (grouped ? 16.0 : 0.0) + (row.Level - 1) * 16.0;
                        Verify.AreEqual(indent + 24.0, GetRowCell(row, 0).Padding.Left,
                            $"ParentBy cell reservation stays unchanged: {name}, {grouped}, {flow}, {density}.");
                        Verify.AreEqual(indent, GetChevron(table, name).Margin.Left,
                            "ParentBy chevrons retain their existing indentation.");
                    }
                });
            }
        }

        #endregion

        #region 3.9 ExpandRow and shared views

        [TestMethod]
        [TestProperty("Description", "Verifies ExpandRow expands the item's row and its ancestors, CollapseRow collapses only that row, and an item the view does not show is ignored.")]
        public void VerifyExpandRowExpandsAncestors()
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);

            RunOnUIThread.Execute(() => tableView.ExpandRow(Named(list, "Ben")));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, AdaBenExpanded, "ExpandRow(Ben) expands Ada too");
                tableView.CollapseRow(Named(list, "Ada"));
                tableView.ExpandRow(new TreeEmployee { Id = 42, Name = "Nobody" });
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "CollapseRow(Ada); an unknown item is ignored"));
            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, AdaBenExpanded, "Ben's expansion survived Ada's collapse");
                Verify.AreEqual(2, tableView.IndexOf(Named(list, "Dan")), "IndexOf is the displayed row.");
                tableView.CollapseRow(Named(list, "Ben"));
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreEqual(-1, tableView.IndexOf(Named(list, "Dan")), "A row inside a collapsed node has no index."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies two TableViews over one hierarchical view keep their own expansion, and a ListView over it shows every node depth first.")]
        public void VerifyOneViewManyControls()
        {
            TableView first = null;
            TableView second = null;
            ListView listView = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var view = MakeTreeSource(MakeTree()).View;
                first = CreateTreeTable(view);
                second = CreateTreeTable(view);
                listView = new ListView { ItemsSource = view, DisplayMemberPath = "Name", Width = 150 };
                first.Width = 200;
                second.Width = 200;
                var panel = new StackPanel { Orientation = Orientation.Horizontal };
                panel.Children.Add(first);
                panel.Children.Add(second);
                panel.Children.Add(listView);
                LoadContent(panel);
            });
            SettleLayout(first);
            SettleLayout(second);

            Toggle(first, "Ada", true);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(first, "Ada1- Ben2+ Cy2 Eve1+ Gus1", "first table");
                VerifyTree(second, Roots, "second table keeps its own expansion");
                Verify.AreEqual("Ada Ben Dan Cy Eve Fay Gus", string.Join(" ", listView.Items.Select(NameOf)), "ListView shows the depth-first list.");
            });
        }

        #endregion

        #region Helpers

        // Loads a tree table over items as the test content and settles it. Call off the UI thread.
        private TableView Place(object items, string itemKeyPath = IdPath, string parentKeyPath = ManagerPath)
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                tableView = CreateTreeTable(items, itemKeyPath, parentKeyPath);
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

        private static string SelectedName(TableView tableView)
            => tableView.SelectedItem == null ? "null" : NameOf(tableView.SelectedItem);

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
        private static void RetractAndEmpty(out TableView tableView, out ObservableCollection<TreeEmployee> list, out List<WeakReference> weak)
        {
            list = MakeTree();
            weak = list.Select(e => new WeakReference(e)).ToList();
            tableView = CreateTreeTable(list, IdPath, ManagerPath);
            TreeSourceOf(tableView).HierarchyDescription = null;
            list.Clear();
        }

        // A 30px Name lead column plus a star Dept column, fully expanded. Hal under Dan makes Dan an
        // expandable row whose chevron is clipped away.
        private TableView PlaceNarrowLeadColumn()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var list = MakeTree();
                list.Add(new TreeEmployee { Id = 8, ManagerId = 4, Name = "Hal", Dept = "Eng", Code = "H", ParentCode = "D", Score = 5 });
                tableView = CreateTreeTable(list, IdPath, ManagerPath);
                tableView.Columns[0].Width = new GridLength(30);
                tableView.Columns.Add(MakeTextTemplateColumn("Dept", "Dept", new GridLength(1, GridUnitType.Star)));
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
        // matches a non-empty clip on an expandable row.
        private static string CheckChevronClip(TableView tableView, string name)
        {
            var gutter = GetChevron(tableView, name);
            var expected = Math.Max(0.0, tableView.Columns[0].ActualWidth - gutter.Margin.Left);
            var clip = gutter.Clip;
            var hitTestable = expected > 0 && FindTreeRow(tableView, name).IsExpandable;
            var ok = clip != null && Math.Abs(clip.Rect.Width - expected) < 0.5 && gutter.IsHitTestVisible == hitTestable;
            return ok
                ? "ok"
                : $"{name} clip={(clip == null ? "null" : clip.Rect.Width.ToString())} expected={expected} hit={gutter.IsHitTestVisible}";
        }

        // Hit-tests the centre of the row's chevron gutter and raises a context request from the topmost
        // element there, as a right-click would. Leaf gutters must yield the lead cell menu.
        private static void VerifyGutterRequest(TableView tableView, MenuRecorder recorder, string name, bool expandable, string context, TableViewRow container = null)
        {
            RunOnUIThread.Execute(() =>
            {
                var row = container ?? FindTreeRow(tableView, name);
                var gutter = row.FindVisualChildByName("PART_RowExpanderGutter") as Border;
                Verify.IsNotNull(gutter, $"Row {name} should have PART_RowExpanderGutter.");
                Verify.AreEqual(expandable, row.IsExpandable, $"Precondition ({context}): {name}.IsExpandable.");
                Verify.AreEqual(expandable, gutter.IsHitTestVisible, $"{context}: {name}'s gutter hit-testability.");

                var centre = gutter.TransformToVisual(null).TransformPoint(new Point(gutter.ActualWidth / 2.0, gutter.ActualHeight / 2.0));
                var hit = Microsoft.UI.Xaml.Media.VisualTreeHelper.FindElementsInHostCoordinates(centre, row).OfType<FrameworkElement>().FirstOrDefault();
                Verify.IsNotNull(hit, $"{context}: something under {name}'s gutter must be hit-testable.");

                var peer = Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer.CreatePeerForElement(hit)
                    ?? new Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer(hit);
                Log.Comment($"{context}: {name} hit {hit.GetType().Name} '{hit.Name}'.");
                peer.ShowContextMenu();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(expandable ? "RowMenu" : "CellMenu", recorder.Describe(), $"{context}: menu opened from {name}'s gutter.");
                recorder.HideAll();
                recorder.Clear();
            });
            IdleSynchronizer.Wait();
        }

        #endregion
    }
}
