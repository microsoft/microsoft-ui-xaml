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
    // Hierarchical rows: TableView.ItemKeyPath / ParentKeyPath, TableView.ExpandAllRows /
    // CollapseAllRows and TableViewRow.Level / IsExpandable / IsExpanded.
    //
    // Fixture (MakeTree, ItemKeyPath="Id", ParentKeyPath="ManagerId"):
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
        [TestProperty("Description", "Verifies the first projection of a table with ItemKeyPath and ParentKeyPath shows only roots, collapsed.")]
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
        [TestProperty("Description", "Verifies the key paths can be set before or after ItemsSource, and either path empty means flat rows.")]
        public void VerifyKeyPathsApplyInAnyOrder()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                tableView = CreateTreeTable(MakeTree());
                tableView.ItemKeyPath = IdPath;
                LoadContent(tableView);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, Flat, "ItemKeyPath alone");
                tableView.ParentKeyPath = ManagerPath;
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, Roots, "both paths set after ItemsSource");
                tableView.ItemsSource = MakeTree();
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "a new ItemsSource keeps the relation"));
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
        [TestProperty("Description", "Verifies clearing ParentKeyPath restores the flat projection in source order at Level 0.")]
        public void VerifyClearingParentKeyPathReturnsFlat()
        {
            var tableView = Place(MakeTree());

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => tableView.ParentKeyPath = "");
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, Flat, "after clearing ParentKeyPath");
                foreach (var row in GetProjectedElements(tableView).OfType<TableViewRow>())
                {
                    Verify.AreEqual(0, row.Level, $"Row {NameOf(row.DataContext)} is not in a hierarchy.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies clearing the relation drops every reference the hierarchy held to the items.")]
        public void VerifyClearingParentKeyPathReleasesRows()
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
        [TestProperty("Description", "Verifies clearing and re-declaring the relation resets expansion.")]
        public void VerifyRedeclareClearsIntent()
        {
            var tableView = Place(MakeTree());

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() =>
            {
                tableView.ParentKeyPath = "";
                tableView.ParentKeyPath = ManagerPath;
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "after re-declaring the relation"));
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

        #region 3.3 Error contract

        [TestMethod]
        [TestProperty("Description", "Verifies duplicate node keys throw from the setter and leave the projection unchanged.")]
        public void VerifyDuplicateKeyThrows()
        {
            VerifyThrowsKeepsProjection(_ => { }, t => t.ItemKeyPath = "Dept", "duplicate key");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a null node key throws and leaves the projection unchanged.")]
        public void VerifyNullKeyThrows()
        {
            VerifyThrowsKeepsProjection(_ => { }, t => t.ItemKeyPath = "MentorId", "null key");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an item cannot be its own parent.")]
        public void VerifySelfParentThrows()
        {
            VerifyThrowsKeepsProjection(l => l[0].BossId = 1, t => t.ParentKeyPath = "BossId", "self parent");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cycle throws and leaves the projection unchanged.")]
        public void VerifyCycleThrows()
        {
            // Ada <-> Dan via Ben.
            VerifyThrowsKeepsProjection(l => l[0].BossId = 4, t => t.ParentKeyPath = "BossId", "cycle");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an invalid relation present when ItemsSource is assigned shows the rows flat instead of failing the assignment.")]
        public void VerifyInvalidRelationAtAssignmentShowsFlatRows()
        {
            TableView tableView = null;
            string thrown = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var list = MakeTree();
                list[0].ManagerId = 4;
                tableView = CreateTreeTable(null, IdPath, ManagerPath);
                thrown = ThrownTypeName(() => tableView.ItemsSource = list);
                LoadContent(tableView);
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("no exception", thrown, "Assigning ItemsSource must not throw.");
                VerifyTree(tableView, Flat, "a cycle at assignment");
            });
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
        [TestProperty("Description", "Verifies a CollectionViewSource sort orders each sibling set, since siblings follow the view's order.")]
        public void VerifyCollectionViewSortOrdersSiblings()
        {
            object view = null;
            RunOnUIThread.Execute(() => view = MakeShapingView(MakeTree(), sortPath: "Name", direction: Microsoft.UI.Xaml.Data.ListSortDirection.Descending).View);
            var tableView = Place(view);

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "Gus1 Eve1+ Ada1- Cy2 Ben2+", "view sorted by Name descending"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a CollectionViewSource filter applies before the tree is built: a kept row whose parent is filtered out becomes a root.")]
        public void VerifyCollectionViewFilterPromotesOrphansToRoots()
        {
            object view = null;
            RunOnUIThread.Execute(() =>
            {
                var cvs = MakeShapingView(MakeTree());
                cvs.Filter = o => ((TreeEmployee)o).Name != "Ada";
                view = cvs.View;
            });
            var tableView = Place(view);

            RunOnUIThread.Execute(() => VerifyTree(tableView, "Ben1+ Cy1 Eve1+ Gus1", "Ada filtered out"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CollectionViewSource grouping buckets roots only and descendants follow their root.")]
        public void VerifyGroupingBucketsRootsOnly()
        {
            var tableView = PlaceGrouped(MakeTree());

            Toggle(tableView, "Ada", true);
            RunOnUIThread.Execute(() => VerifyTree(tableView, "[Eng] Ada1- Ben2+ Cy2 [Ops] Eve1+ Gus1", "grouped by Dept"));

            // Ben's own Dept is ignored: he stays under Ada, and header counts are root counts.
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                list[1].Dept = "Ops";
            });
            tableView = PlaceGrouped(list);
            Toggle(tableView, "Ada", true);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual("[Eng 1] Ada1- Cy2 Ben2+ [Ops 2] Eve1+ Gus1", HeaderLabels(tableView), "Ben in Ops, grouped by Dept"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a CollectionViewSource sort under grouping gives the same header order with or without a hierarchy.")]
        public void VerifyGroupedSortHeaderOrderIgnoresHierarchy()
        {
            Func<ObservableCollection<TreeEmployee>> pair = () => new ObservableCollection<TreeEmployee>(new[]
            {
                new TreeEmployee { Id = 1, Dept = "A", Name = "Zulu" },
                new TreeEmployee { Id = 2, Dept = "B", Name = "Alpha" },
            });

            var hier = PlacedNames(() => MakeShapingView(pair(), "Dept", "Name").View, IdPath, ManagerPath);
            var flat = PlacedNames(() => MakeShapingView(pair(), "Dept", "Name").View, null, null);

            Verify.AreEqual("Alpha Zulu", flat, "Groups follow their first item.");
            Verify.AreEqual(flat, hier, "The hierarchy must not change header order.");
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
        [TestProperty("Description", "Verifies clearing ParentKeyPath during an incremental add does not crash and ends flat.")]
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
                    tableView.ParentKeyPath = "";
                }, () => list.Add(new TreeEmployee { Id = 8, Name = "Hal", Dept = "Ops" })));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(fired, "The presented change fired (add).");
                Verify.IsNull(thrown, "The add must not throw.");
                VerifyTree(tableView, Flat + " Hal", "ParentKeyPath cleared during an add");
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
            // now host a leaf, and its gutter must follow.
            RunOnUIThread.Execute(() =>
            {
                for (var i = 0; i < 40; i++)
                {
                    list.Add(new LiveRow { Id = 100 + i, Name = $"R{i}", Score = 100 + i, Team = "Z" });
                    list.Add(new LiveRow { Id = 200 + i, Name = $"C{i}", Score = 1, Team = "Z", ParentId = 100 + i });
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
            for (var offset = 400.0; offset <= 2400.0 && reusedName == null; offset += 400.0)
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
        [TestProperty("Description", "Verifies a grouped root row is indented by the group header's expander gutter plus its spacing.")]
        public void VerifyGroupedRootIndentMatchesGroupHeaderGutterAndSpacing()
        {
            var tableView = PlaceGrouped(MakeTree());

            RunOnUIThread.Execute(() =>
            {
                var header = GetProjectedElements(tableView).OfType<TableViewGroupHeader>().First();
                var gutter = header.FindVisualChildByName("PART_ExpanderGutter") as FrameworkElement;
                var gutterGrid = gutter == null ? null : Microsoft.UI.Xaml.Media.VisualTreeHelper.GetParent(gutter) as Grid;
                Verify.IsNotNull(gutterGrid, "The group header should host PART_ExpanderGutter in a Grid.");
                double expected = gutter.ActualWidth + gutterGrid.ColumnSpacing;

                var row = FindTreeRow(tableView, "Ada");
                double rowX = row.TransformToVisual(tableView).TransformPoint(new Point(0, 0)).X;
                double chevronX = GetChevron(tableView, "Ada").TransformToVisual(tableView).TransformPoint(new Point(0, 0)).X;
                Log.Comment($"Root indent={chevronX - rowX}, gutter + TableViewGroupExpanderSpacing={expected}, header Padding.Left={header.Padding.Left}");
                Verify.AreEqual(24.0 + 8.0, expected, "Default gutter (TableViewGroupExpanderSize) plus TableViewGroupExpanderSpacing.");
                Verify.IsLessThan(Math.Abs(chevronX - rowX - expected), 0.5,
                    "A grouped root row's indent must equal the group header's gutter plus TableViewGroupExpanderSpacing.");
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

        // A tree over a CollectionViewSource view grouped by Dept.
        private TableView PlaceGrouped(ObservableCollection<TreeEmployee> list)
        {
            object view = null;
            RunOnUIThread.Execute(() => view = MakeShapingView(list, "Dept").View);
            return Place(view);
        }

        private static void Toggle(TableView tableView, string name, bool expand)
        {
            RunOnUIThread.Execute(() => ToggleTreeRow(tableView, name, expand));
            SettleLayout(tableView);
        }

        // Data-row names of a freshly placed table over the source, header rows skipped.
        private string PlacedNames(Func<object> makeSource, string itemKeyPath, string parentKeyPath)
        {
            object source = null;
            RunOnUIThread.Execute(() => source = makeSource());
            var tableView = Place(source, itemKeyPath, parentKeyPath);

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
            tableView.ParentKeyPath = "";
            list.Clear();
        }

        // On a table showing Roots: mutate the data, set a key path that makes the relation invalid,
        // and verify the setter throws ArgumentException and the projection is unchanged.
        private void VerifyThrowsKeepsProjection(
            Action<ObservableCollection<TreeEmployee>> mutate,
            Action<TableView> declare,
            string context)
        {
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() => list = MakeTree());
            var tableView = Place(list);

            string thrown = null;
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, Roots, $"{context}: before");
                mutate(list);
                thrown = ThrownTypeName(() => declare(tableView));
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("ArgumentException", thrown, $"{context}: the key path setter must throw E_INVALIDARG.");
                VerifyTree(tableView, Roots, $"{context}: unchanged after the throw");
            });
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
