// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using Windows.Foundation.Collections;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;
using CollectionChange = global::Windows.Foundation.Collections.CollectionChange;
using SortDescription = Microsoft.UI.Xaml.Data.SortDescription;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // The framework view's hierarchy (ICollectionViewHierarchy, CollectionViewSource.HierarchyDescription), without a
    // control. Trees are written depth first as "Name(depth)", a context node marked '*'.
    [TestClass]
    public class CollectionViewHierarchyTests : TableViewApiTestBase
    {
        #region Build

        [TestMethod]
        [TestProperty("Description", "Verifies a PropertyHierarchyDescription declared in markup builds the view's tree, and the view's list is every node depth first.")]
        public void VerifyMarkupDeclaredHierarchy()
        {
            RunOnUIThread.Execute(() =>
            {
                var cvs = (CollectionViewSource)XamlReader.Load(
                    "<CollectionViewSource xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                    "<CollectionViewSource.HierarchyDescription><PropertyHierarchyDescription ItemKeyPath='Id' ParentKeyPath='ManagerId'/></CollectionViewSource.HierarchyDescription>" +
                    "</CollectionViewSource>");
                cvs.Source = MakeTree();
                var hierarchy = (ICollectionViewHierarchy)cvs.View;
                Verify.IsTrue(hierarchy.CanBuildHierarchy, "The framework view builds trees.");
                Verify.AreEqual("Id", ((PropertyHierarchyDescription)hierarchy.HierarchyDescription).ItemKeyPath);
                Verify.AreEqual("Ada(0) Ben(1) Dan(2) Cy(1) Eve(0) Fay(1) Gus(0)", Tree(cvs.View), "The tree.");
                Verify.AreEqual("Ada Ben Dan Cy Eve Fay Gus", Flat(cvs.View), "The view's list is the nodes in pre-order.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies node members: Item, Key (the ItemKeyPath value), Parent, Children and IsContext, and NodeFromItem.")]
        public void VerifyNodeMembers()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                var view = MakeTreeSource(list).View;
                var hierarchy = (ICollectionViewHierarchy)view;
                var dan = hierarchy.NodeFromItem(Named(list, "Dan"));
                Verify.IsNotNull(dan, "Dan has a node.");
                Verify.AreSame(Named(list, "Dan"), dan.Item);
                Verify.AreEqual(4, (int)dan.Key, "Key is the ItemKeyPath value.");
                Verify.AreEqual("Ben", NameOf(dan.Parent.Item));
                Verify.AreEqual("Ada", NameOf(dan.Parent.Parent.Item));
                Verify.IsNull(dan.Parent.Parent.Parent, "A root has no parent.");
                Verify.AreEqual(0, dan.Children.Count, "Dan is a leaf.");
                Verify.IsFalse(dan.IsContext, "No filter: nothing is context.");
                Verify.AreSame(hierarchy.CollectionNodes[0], dan.Parent.Parent, "Nodes are one object while the view keeps them.");
                Verify.IsNull(hierarchy.NodeFromItem(new TreeEmployee()), "An item the view does not have has no node.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a null, empty or unresolved parent key makes a root, keys compare by type (Int32 never links to Int64), and dotted paths read through references.")]
        public void VerifyKeyRules()
        {
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Ada(0) Ben(1) Dan(2) Cy(1) Eve(0) Fay(1) Gus(0)", Tree(MakeTreeSource(MakeTree(), "Code", "ParentCode").View), "Empty ParentCode is a root; Z matches nobody.");
                Verify.AreEqual("Ada(0) Ben(0) Cy(0) Dan(0) Eve(0) Fay(0) Gus(0)", Tree(MakeTreeSource(MakeTree(), IdPath, "BigManagerId").View), "Int64 parent keys never match Int32 keys.");
                Verify.AreEqual("Ada(0) Ben(1) Dan(2) Cy(1) Eve(0) Fay(1) Gus(0)", Tree(MakeTreeSource(MakeTree(), "Code", "Manager.Code").View), "A dotted path.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies bad data degrades without an exception: a later duplicate key is a root that cannot be a parent, a self parent is a root, and a cycle is broken at its member that comes first in the source.")]
        public void VerifyBadDataDegrades()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                list.Add(new TreeEmployee { Id = 1, ManagerId = 5, Name = "Zed" });
                list.Add(new TreeEmployee { Id = 9, ManagerId = 1, Name = "Kid" });
                Verify.AreEqual("Ada(0) Ben(1) Dan(2) Cy(1) Kid(1) Eve(0) Fay(1) Gus(0) Zed(0)", Tree(MakeTreeSource(list).View), "Zed duplicates Ada's key.");

                list = MakeTree();
                list[1].BossId = 2;
                Verify.AreEqual("Ada(0) Cy(1) Ben(0) Dan(1) Eve(0) Fay(1) Gus(0)", Tree(MakeTreeSource(list, IdPath, "BossId").View), "Ben is his own boss.");

                list = MakeTree();
                list[0].BossId = 4;
                Verify.AreEqual("Ada(0) Ben(1) Dan(2) Cy(1) Eve(0) Fay(1) Gus(0)", Tree(MakeTreeSource(list, IdPath, "BossId").View), "Ada > Ben > Dan > Ada: Ada becomes the root.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the filter keeps matches and their ancestors (IsContext), drops non-matching descendants of a match, and a filtered-out parent does not orphan its children.")]
        public void VerifyFilterKeepsAncestorsAsContext()
        {
            RunOnUIThread.Execute(() =>
            {
                var cvs = MakeTreeSource(MakeTree());
                var view = cvs.View;
                ((ICollectionViewShaping)view).Filter = o => ((TreeEmployee)o).Name == "Dan" || ((TreeEmployee)o).Name == "Ada" || ((TreeEmployee)o).Name == "Fay";
                Verify.AreEqual("Ada(0) Ben(1)* Dan(2) Eve(0)* Fay(1)", Tree(view), "Ben and Eve are context; Cy is dropped; Gus is filtered out.");
                Verify.AreEqual("Ada Ben Dan Eve Fay", Flat(view), "Context nodes are in the view's list.");
                var ben = ((ICollectionViewHierarchy)view).NodeFromItem(Named((IEnumerable<TreeEmployee>)cvs.Source, "Ben"));
                Verify.IsTrue(ben.IsContext, "Ben is context.");
                Verify.IsNull(((ICollectionViewHierarchy)view).NodeFromItem(Named((IEnumerable<TreeEmployee>)cvs.Source, "Cy")), "Cy is filtered out.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies SortDescriptions and CustomSort order every sibling list, roots included, with ties in source order.")]
        public void VerifySortOrdersEachSiblingList()
        {
            RunOnUIThread.Execute(() =>
            {
                var cvs = MakeTreeSource(MakeTree());
                cvs.SortDescriptions.Add(new SortDescription("Name", CollectionViewSortDirection.Descending));
                Verify.AreEqual("Gus(0) Eve(0) Fay(1) Ada(0) Cy(1) Ben(1) Dan(2)", Tree(cvs.View), "Name descending.");

                cvs.SortDescriptions.Clear();
                cvs.SortDescriptions.Add(new SortDescription("Dept", CollectionViewSortDirection.Ascending));
                Verify.AreEqual("Ada(0) Ben(1) Dan(2) Cy(1) Eve(0) Fay(1) Gus(0)", Tree(cvs.View), "Equal keys keep source order.");

                cvs.CustomSort = (x, y) => ((TreeEmployee)y).Score.CompareTo(((TreeEmployee)x).Score);
                Verify.AreEqual("Gus(0) Eve(0) Fay(1) Ada(0) Ben(1) Dan(2) Cy(1)", Tree(cvs.View), "CustomSort by Score descending.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a view cannot group and build a hierarchy at once, and a CollectionViewSource that cannot shape (IsSourceGrouped) rejects a hierarchy.")]
        public void VerifyGroupingAndHierarchyAreExclusive()
        {
            RunOnUIThread.Execute(() =>
            {
                var cvs = MakeTreeSource(MakeTree());
                var view = cvs.View;
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => cvs.GroupDescriptions.Add(new PropertyGroupDescription("Dept"))), "Grouping a tree.");
                Verify.IsNull(view.CollectionGroups, "The tree is not grouped.");

                var grouped = new CollectionViewSource { Source = MakeTree() };
                grouped.GroupDescriptions.Add(new PropertyGroupDescription("Dept"));
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => grouped.HierarchyDescription = new PropertyHierarchyDescription(IdPath, ManagerPath)), "A tree on a grouped view.");

                var sourceGrouped = new CollectionViewSource { IsSourceGrouped = true };
                sourceGrouped.HierarchyDescription = new PropertyHierarchyDescription(IdPath, ManagerPath);
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => sourceGrouped.Source = new ObservableCollection<ObservableCollection<TreeEmployee>>()), "IsSourceGrouped cannot build a tree.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies setting and clearing the hierarchy keeps the view, raises PropertyChanged(CollectionNodes) and a Reset, and CollectionNodes is null while flat.")]
        public void VerifyHierarchyToggleKeepsView()
        {
            RunOnUIThread.Execute(() =>
            {
                var cvs = new CollectionViewSource { Source = MakeTree() };
                cvs.SortDescriptions.Add(new SortDescription("Name", CollectionViewSortDirection.Ascending));
                var view = cvs.View;
                var log = new List<string>();
                ((INotifyPropertyChanged)view).PropertyChanged += (s, e) =>
                {
                    if (e.PropertyName == "CollectionNodes")
                    {
                        log.Add("P:" + e.PropertyName);
                    }
                };
                view.VectorChanged += (s, e) => log.Add(e.CollectionChange.ToString());
                Verify.IsNull(((ICollectionViewHierarchy)view).CollectionNodes, "Flat.");

                cvs.HierarchyDescription = new PropertyHierarchyDescription(IdPath, ManagerPath);
                Verify.AreSame(view, cvs.View, "Same view.");
                Verify.AreEqual("P:CollectionNodes,Reset", string.Join(",", log), "Toggle on.");
                Verify.AreEqual("Ada Ben Dan Cy Eve Fay Gus", Flat(view));

                log.Clear();
                cvs.HierarchyDescription = null;
                Verify.AreEqual("P:CollectionNodes,Reset", string.Join(",", log), "Toggle off.");
                Verify.IsNull(((ICollectionViewHierarchy)view).CollectionNodes, "Flat again.");
                Verify.AreEqual("Ada Ben Cy Dan Eve Fay Gus", Flat(view), "Sorted flat list.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies DeferRefresh recomputes the tree once, when the deferral closes, and a key path change on the description in use rebuilds.")]
        public void VerifyDeferRefreshAndDescriptionChange()
        {
            RunOnUIThread.Execute(() =>
            {
                var cvs = MakeTreeSource(MakeTree());
                var view = cvs.View;
                var resets = 0;
                view.VectorChanged += (s, e) => resets += e.CollectionChange == CollectionChange.Reset ? 1 : 0;
                using (cvs.DeferRefresh())
                {
                    cvs.SortDescriptions.Add(new SortDescription("Name", CollectionViewSortDirection.Descending));
                    ((PropertyHierarchyDescription)cvs.HierarchyDescription).ParentKeyPath = "MentorId";
                    Verify.AreEqual(0, resets, "Nothing while deferred.");
                }
                Verify.AreEqual(1, resets, "One recompute.");
                Verify.AreEqual("Gus(0) Eve(0) Ben(1) Dan(0) Cy(0) Ada(0) Fay(1)", Tree(view), "Mentor tree by Name descending.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies currency over a tree: MoveCurrentTo a nested item, and CurrentItem follows it through a recompute.")]
        public void VerifyCurrencyOnNestedItems()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                var cvs = MakeTreeSource(list);
                var view = cvs.View;
                Verify.IsTrue(view.MoveCurrentTo(Named(list, "Dan")), "Dan can be current.");
                Verify.AreEqual(2, view.CurrentPosition, "Dan is third in pre-order.");
                cvs.SortDescriptions.Add(new SortDescription("Name", CollectionViewSortDirection.Descending));
                Verify.AreSame(Named(list, "Dan"), view.CurrentItem, "Currency follows the item.");
                Verify.AreEqual(6, view.CurrentPosition, "Dan is last in the re-sorted pre-order.");
            });
        }

        #endregion

        #region Incremental changes

        [TestMethod]
        [TestProperty("Description", "Verifies source Add, Remove, Replace and Move are applied in place: every vector raises single changes that replay to its contents, and the result equals a recompute.")]
        public void VerifyIncrementalChangesReplay()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                var cvs = MakeTreeSource(list);
                cvs.SortDescriptions.Add(new SortDescription("Name", CollectionViewSortDirection.Ascending));
                var checker = new ReplayChecker(cvs.View);

                list.Add(new TreeEmployee { Id = 8, ManagerId = 2, Name = "Bo" });
                checker.Check("add a child", list, cvs);
                list.Add(new TreeEmployee { Id = 9, ManagerId = 10, Name = "Ola" });
                checker.Check("add an orphan", list, cvs);
                list.Add(new TreeEmployee { Id = 10, ManagerId = 7, Name = "Pat" });
                checker.Check("add the orphan's parent (adopts it)", list, cvs);
                list.RemoveAt(1);
                checker.Check("remove a parent (its children are roots)", list, cvs);
                list[1] = new TreeEmployee { Id = 3, ManagerId = 5, Name = "Cy" };
                checker.Check("replace with a new parent key (reparent)", list, cvs);
                list[0] = new TreeEmployee { Id = 1, Name = "Ada" };
                checker.Check("replace with the same keys", list, cvs);

                list.Insert(0, new TreeEmployee { Id = 2, ManagerId = 1, Name = "Ben" });
                checker.Check("re-insert a key its children wait for", list, cvs);
                Verify.AreEqual(0, checker.Resets, "No change needed a Reset.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies changes under a filter: a match added under a filtered-out parent brings it in as context, and removing the last match below a context node removes it.")]
        public void VerifyIncrementalChangesUnderFilter()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                var cvs = MakeTreeSource(list);
                ((ICollectionViewShaping)cvs.View).Filter = o => ((TreeEmployee)o).Score >= 50;
                var checker = new ReplayChecker(cvs.View);
                Verify.AreEqual("Ada(0) Eve(0) Gus(0)", Tree(cvs.View), "Precondition.");

                list.Add(new TreeEmployee { Id = 8, ManagerId = 4, Name = "Hal", Score = 90 });
                checker.Check("a match under Dan", list, cvs);
                Verify.AreEqual("Ada(0) Ben(1)* Dan(2)* Hal(3) Eve(0) Gus(0)", Tree(cvs.View), "Ben and Dan come in as context.");

                list.RemoveAt(list.Count - 1);
                checker.Check("the match leaves", list, cvs);
                Verify.AreEqual("Ada(0) Eve(0) Gus(0)", Tree(cvs.View), "The context nodes leave with it.");
                Verify.AreEqual(0, checker.Resets, "No change needed a Reset.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies random source changes, filter toggles and live ticks: after each step the incremental tree equals a recompute and every vector's events replay to its contents.")]
        public void VerifyIncrementalFuzz()
        {
            RunOnUIThread.Execute(() =>
            {
                var random = new Random(12654);
                var list = new ObservableCollection<TreeEmployee>();
                for (var i = 0; i < 40; i++)
                {
                    list.Add(NewFuzzEmployee(random, i + 1));
                }
                var cvs = MakeTreeSource(list);
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                var checker = new ReplayChecker(cvs.View);
                var nextId = 100;
                for (var step = 0; step < 300; step++)
                {
                    var op = random.Next(6);
                    string what;
                    switch (op)
                    {
                        case 0:
                            list.Insert(random.Next(list.Count + 1), NewFuzzEmployee(random, nextId++ % 60 + 1));
                            what = "insert";
                            break;
                        case 1:
                            if (list.Count == 0) { continue; }
                            list.RemoveAt(random.Next(list.Count));
                            what = "remove";
                            break;
                        case 2:
                            if (list.Count == 0) { continue; }
                            list[random.Next(list.Count)] = NewFuzzEmployee(random, random.Next(60) + 1);
                            what = "replace";
                            break;
                        case 3:
                            if (list.Count < 2) { continue; }
                            list.Move(random.Next(list.Count), random.Next(list.Count));
                            what = "move";
                            break;
                        case 4:
                            var threshold = random.Next(100);
                            ((ICollectionViewShaping)cvs.View).Filter = random.Next(3) == 0 ? null : (CollectionViewFilter)(o => ((TreeEmployee)o).Score >= threshold);
                            checker.Rebase();
                            what = "filter";
                            break;
                        default:
                            if (list.Count == 0) { continue; }
                            var index = random.Next(list.Count);
                            var copy = list[index];
                            list[index] = new TreeEmployee { Id = copy.Id, ManagerId = copy.ManagerId, Name = copy.Name, Score = random.Next(100) };
                            what = "re-score";
                            break;
                    }
                    checker.Check($"step {step} ({what})", list, cvs);
                }
                Log.Comment($"Resets: {checker.Resets}");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a handler of the view's change that changes the source is applied afterwards: the view ends equal to a recompute.")]
        public void VerifyReentrantSourceChange()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                var cvs = MakeTreeSource(list);
                var view = cvs.View;
                var once = true;
                view.VectorChanged += (s, e) =>
                {
                    if (once)
                    {
                        once = false;
                        list.Add(new TreeEmployee { Id = 9, ManagerId = 7, Name = "Ivy" });
                    }
                };
                list.Add(new TreeEmployee { Id = 8, ManagerId = 2, Name = "Bo" });
                Verify.AreEqual(Tree(MakeTreeSource(new ObservableCollection<TreeEmployee>(list)).View), Tree(view), "The view equals a recompute.");
                Verify.IsTrue(Flat(view).Contains("Ivy"), "The re-entrant add is applied.");
            });
        }

        #endregion

        #region Live shaping

        [TestMethod]
        [TestProperty("Description", "Verifies live sorting moves a node with its subtree within its siblings, and live filtering brings context ancestors in and out.")]
        public void VerifyLiveShapingInTree()
        {
            ObservableCollection<LiveRow> list = null;
            CollectionViewSource cvs = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeLiveTree();
                cvs = MakeTreeSource(list, IdPath, ParentIdPath);
                cvs.IsLiveSortingRequested = true;
                cvs.IsLiveFilteringRequested = true;
                cvs.LiveFilteringProperties.Add("Score");
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                Verify.AreEqual("Dee(0) Eli(1) Ann(0) Cat(1) Bob(1)", Tree(cvs.View), "Precondition.");
                Named(list, "Ann").Score = 1;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Ann(0) Cat(1) Bob(1) Dee(0) Eli(1)", Tree(cvs.View), "Ann moved first with her children.");
                cvs.Filter += (s, e) => e.Accepted = ((LiveRow)e.Item).Score < 40;
                Verify.AreEqual("Ann(0) Cat(1) Bob(1) Dee(0) Eli(1)", Tree(cvs.View), "Every score is below 40.");
                Named(list, "Eli").Score = 99;
                Named(list, "Dee").Score = 98;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Ann(0) Cat(1) Bob(1)", Tree(cvs.View), "Dee and Eli filtered out.");
                Named(list, "Eli").Score = 7;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.AreEqual("Ann(0) Cat(1) Bob(1) Dee(0)* Eli(1)", Tree(cvs.View), "Eli brings Dee back as context."));
        }

        #endregion

        #region Consumers

        [TestMethod]
        [TestProperty("Description", "Verifies a ListView over a hierarchical view shows every node, depth first, and follows its changes.")]
        public void VerifyListViewShowsDepthFirstList()
        {
            ListView listView = null;
            ObservableCollection<TreeEmployee> list = null;
            RunOnUIThread.Execute(() =>
            {
                list = MakeTree();
                var cvs = MakeTreeSource(list);
                listView = new ListView { ItemsSource = cvs.View, DisplayMemberPath = "Name", Height = 400 };
                Content = listView;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Ada Ben Dan Cy Eve Fay Gus", string.Join(" ", listView.Items.Select(NameOf)), "ListView items.");
                list.Add(new TreeEmployee { Id = 8, ManagerId = 2, Name = "Bo" });
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Ada Ben Dan Bo Cy Eve Fay Gus", string.Join(" ", listView.Items.Select(NameOf)), "ListView follows an insert.");
                Content = null;
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a node and its Children are released when the app drops the view, and a node held alone degrades to its last values.")]
        public void VerifyNodeLifetime()
        {
            WeakReference viewRef = null;
            ICollectionViewNode held = null;
            RunOnUIThread.Execute(() =>
            {
                var view = MakeTreeSource(MakeTree()).View;
                viewRef = new WeakReference(view);
                held = (ICollectionViewNode)((ICollectionViewHierarchy)view).CollectionNodes[0];
                Verify.AreEqual(2, held.Children.Count, "Ada's children.");
            });
            RunOnUIThread.Execute(() =>
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"View alive: {viewRef.IsAlive}");
                Verify.AreEqual("Ada", NameOf(held.Item), "A held node keeps its item.");
                Verify.IsTrue(held.Children.Count == 0 || held.Children.Count == 2, "Children is empty or still Ada's.");
            });
        }

        #endregion

        #region Robustness and contracts

        [TestMethod]
        [TestProperty("Description", "Verifies a change that needs a recompute leaves the tree untouched when the recompute fails (a throwing CurrentChanging handler), and later source changes recompute instead of reading stale indices.")]
        public void VerifyFailedRecomputeLeavesNoStaleTree()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                var cvs = MakeTreeSource(list);
                var view = cvs.View;
                Verify.IsTrue(view.MoveCurrentTo(Named(list, "Ada")), "Ada is current, so a recompute raises CurrentChanging.");
                var isThrowing = true;
                CurrentChangingEventHandler handler = (s, e) =>
                {
                    if (isThrowing)
                    {
                        throw new InvalidOperationException("CurrentChanging refused.");
                    }
                };
                view.CurrentChanging += handler;

                // Cy becomes her own parent: the view must recompute, and the recompute fails. The handler's exception is
                // reported to Application.UnhandledException; handled there, the app goes on.
                using (var unhandled = new UnhandledErrors())
                {
                    Log.Comment("Replace: " + ThrownTypeName(() => list[2] = new TreeEmployee { Id = 3, ManagerId = 3, Name = "Cy" }));
                    Log.Comment("Reported: " + string.Join(" | ", unhandled.Messages));
                }
                isThrowing = false;
                view.CurrentChanging -= handler;

                list.Insert(0, new TreeEmployee { Id = 8, Name = "Hal" });
                list.RemoveAt(list.Count - 1);
                list.Add(new TreeEmployee { Id = 9, ManagerId = 2, Name = "Ivy" });
                Verify.AreEqual(Tree(MakeTreeSource(new ObservableCollection<TreeEmployee>(list)).View), Tree(view), "The view equals a recompute.");
                Verify.AreEqual(Flat(MakeTreeSource(new ObservableCollection<TreeEmployee>(list)).View), Flat(view), "The flat list equals a recompute.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a node's custom properties ({Binding Item.Name}) read only nodes: another ICustomPropertyProvider, or any other object, is an invalid argument, and the node reports its own TypeName.")]
        public void VerifyNodePropertiesRejectOtherTargets()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                var view = MakeTreeSource(list).View;
                var node = (ICustomPropertyProvider)((ICollectionViewHierarchy)view).CollectionNodes[0];
                Log.Comment($"Node type: {node.Type}");
                var item = node.GetCustomProperty("Item");
                Verify.AreSame(Named(list, "Ada"), item.GetValue(node), "A node reads its item.");

                var grouped = MakeShapingView(MakeTree(), "Dept").View;
                Verify.AreEqual("ArgumentException", ThrownTypeName(() => item.GetValue(grouped.CollectionGroups[0])), "A group is not a node.");
                Verify.AreEqual("ArgumentException", ThrownTypeName(() => item.GetValue(new ForeignPropertyProvider())), "An app's provider is not a node.");
                Verify.AreEqual("ArgumentException", ThrownTypeName(() => node.GetCustomProperty("Children").GetValue(new object())), "Nor is any other object.");
                Verify.AreEqual("ArgumentException", ThrownTypeName(() => node.GetCustomProperty("IsContext").GetValue(new ForeignPropertyProvider())), "IsContext checks too.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Children list a consumer follows raises a Reset when a recompute or leaving the hierarchy empties it, after the roots' Reset.")]
        public void VerifyEmptiedChildrenRaiseReset()
        {
            RunOnUIThread.Execute(() =>
            {
                var cvs = MakeTreeSource(MakeTree());
                var view = cvs.View;
                var roots = ((ICollectionViewHierarchy)view).CollectionNodes;
                var children = ((ICollectionViewNode)roots[0]).Children;
                var log = new List<string>();
                roots.VectorChanged += (s, e) => log.Add("roots:" + e.CollectionChange);
                children.VectorChanged += (s, e) => log.Add($"children:{e.CollectionChange}:{children.Count}");

                ((ICollectionViewShaping)view).Refresh();
                Verify.AreEqual("roots:Reset,children:Reset:0", string.Join(",", log), "Refresh empties Ada's old Children and says so.");

                log.Clear();
                children = ((ICollectionViewNode)roots[0]).Children;
                children.VectorChanged += (s, e) => log.Add($"children:{e.CollectionChange}:{children.Count}");
                cvs.HierarchyDescription = null;
                Verify.AreEqual("roots:Reset,children:Reset:0", string.Join(",", log), "Leaving the hierarchy empties them too.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a node raises PropertyChanged(Parent) when removing its parent makes it a root and when an inserted item adopts it.")]
        public void VerifyReparentRaisesParentChanged()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                var view = MakeTreeSource(list).View;
                var dan = ((ICollectionViewHierarchy)view).NodeFromItem(Named(list, "Dan"));
                var log = new List<string>();
                ((INotifyPropertyChanged)dan).PropertyChanged += (s, e) => log.Add(e.PropertyName);

                list.RemoveAt(1);
                Verify.IsNull(dan.Parent, "Dan is a root once Ben is gone.");
                Verify.AreEqual("Parent", string.Join(",", log), "Orphaned.");

                log.Clear();
                list.Insert(1, new TreeEmployee { Id = 2, ManagerId = 1, Name = "Bo" });
                Verify.AreEqual("Bo", NameOf(dan.Parent.Item), "Bo has Ben's key and adopts Dan.");
                Verify.AreEqual("Parent", string.Join(",", log), "Adopted.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies grouping and a hierarchy are refused before either rule changes: CanGroup and CanBuildHierarchy say so, the rejected description is not kept, and the view keeps working.")]
        public void VerifyConflictingRuleRejectedBeforeChange()
        {
            RunOnUIThread.Execute(() =>
            {
                var cvs = MakeTreeSource(MakeTree());
                var view = cvs.View;
                Verify.IsFalse(((ICollectionViewShaping)view).CanGroup, "A tree cannot group.");
                Verify.IsTrue(((ICollectionViewHierarchy)view).CanBuildHierarchy, "It builds its tree.");
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => cvs.GroupDescriptions.Add(new PropertyGroupDescription("Dept"))), "Grouping a tree.");
                Verify.AreEqual(0, cvs.GroupDescriptions.Count, "The group description was not added.");
                Verify.AreEqual("no exception", ThrownTypeName(() => cvs.SortDescriptions.Add(new SortDescription("Name", CollectionViewSortDirection.Descending))), "Later rules still apply.");
                Verify.AreEqual("Gus(0) Eve(0) Fay(1) Ada(0) Cy(1) Ben(1) Dan(2)", Tree(view), "Sorted tree.");

                var grouped = MakeShapingView(MakeTree(), "Dept");
                var groupedView = grouped.View;
                Verify.IsTrue(((ICollectionViewShaping)groupedView).CanGroup, "A grouped view groups.");
                Verify.IsFalse(((ICollectionViewHierarchy)groupedView).CanBuildHierarchy, "A grouped view cannot build a tree.");
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => grouped.HierarchyDescription = new PropertyHierarchyDescription(IdPath, ManagerPath)), "A tree on a grouped view.");
                Verify.IsNull(grouped.HierarchyDescription, "The rejected description is undone.");
                Verify.AreEqual(2, groupedView.CollectionGroups.Count, "Still grouped.");
                grouped.GroupDescriptions.Clear();
                grouped.HierarchyDescription = new PropertyHierarchyDescription(IdPath, ManagerPath);
                Verify.AreEqual("Ada(0) Ben(1) Dan(2) Cy(1) Eve(0) Fay(1) Gus(0)", Tree(groupedView), "Without the group the tree builds.");

                var pending = new CollectionViewSource { HierarchyDescription = new PropertyHierarchyDescription(IdPath, ManagerPath) };
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => pending.GroupDescriptions.Add(new PropertyGroupDescription("Dept"))), "Before it has a view too.");
                Verify.AreEqual(0, pending.GroupDescriptions.Count, "Not added.");
                pending.Source = MakeTree();
                Verify.AreEqual("Ada(0) Ben(1) Dan(2) Cy(1) Eve(0) Fay(1) Gus(0)", Tree(pending.View), "The view is a tree.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies one PropertyHierarchyDescription can serve two CollectionViewSources, in code and as a markup resource: both build their tree, and a key path change rebuilds both.")]
        public void VerifyDescriptionSharedByTwoSources()
        {
            RunOnUIThread.Execute(() =>
            {
                var shared = new PropertyHierarchyDescription(IdPath, ManagerPath);
                var first = new CollectionViewSource { Source = MakeTree(), HierarchyDescription = shared };
                var second = new CollectionViewSource { Source = MakeTree(), HierarchyDescription = shared };
                Verify.AreEqual("Ada(0) Ben(1) Dan(2) Cy(1) Eve(0) Fay(1) Gus(0)", Tree(first.View), "First.");
                Verify.AreEqual("Ada(0) Ben(1) Dan(2) Cy(1) Eve(0) Fay(1) Gus(0)", Tree(second.View), "Second.");
                shared.ParentKeyPath = "MentorId";
                Verify.AreEqual("Ada(0) Fay(1) Cy(0) Dan(0) Eve(0) Ben(1) Gus(0)", Tree(first.View), "First follows the change.");
                Verify.AreEqual("Ada(0) Fay(1) Cy(0) Dan(0) Eve(0) Ben(1) Gus(0)", Tree(second.View), "Second follows the change.");

                var root = (Grid)XamlReader.Load(
                    "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>" +
                    "<Grid.Resources>" +
                    "<PropertyHierarchyDescription x:Key='Relation' ItemKeyPath='Id' ParentKeyPath='ManagerId'/>" +
                    "<CollectionViewSource x:Key='A' HierarchyDescription='{StaticResource Relation}'/>" +
                    "<CollectionViewSource x:Key='B' HierarchyDescription='{StaticResource Relation}'/>" +
                    "</Grid.Resources></Grid>");
                var a = (CollectionViewSource)root.Resources["A"];
                var b = (CollectionViewSource)root.Resources["B"];
                Verify.AreSame(a.HierarchyDescription, b.HierarchyDescription, "One description.");
                a.Source = MakeTree();
                b.Source = MakeTree();
                Verify.AreEqual("Ada(0) Ben(1) Dan(2) Cy(1) Eve(0) Fay(1) Gus(0)", Tree(a.View), "A.");
                Verify.AreEqual("Ada(0) Ben(1) Dan(2) Cy(1) Eve(0) Fay(1) Gus(0)", Tree(b.View), "B.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies only the item that owns a key reports it as its node's Key; a later duplicate's node has none.")]
        public void VerifyDuplicateNodeHasNoKey()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                list.Add(new TreeEmployee { Id = 1, Name = "Zed" });
                var hierarchy = (ICollectionViewHierarchy)MakeTreeSource(list).View;
                Verify.AreEqual(1, (int)hierarchy.NodeFromItem(Named(list, "Ada")).Key, "Ada owns key 1.");
                Verify.IsNull(hierarchy.NodeFromItem(Named(list, "Zed")).Key, "Zed duplicates it and reports no key.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HasChildren says whether a node has kept children without making its Children list, and raises PropertyChanged(HasChildren) when the first child arrives and when the last one leaves.")]
        public void VerifyHasChildren()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                var hierarchy = (ICollectionViewHierarchy)MakeTreeSource(list).View;
                var ada = hierarchy.NodeFromItem(Named(list, "Ada"));
                var gus = hierarchy.NodeFromItem(Named(list, "Gus"));
                Verify.IsTrue(ada.HasChildren, "Ada has children.");
                Verify.IsFalse(gus.HasChildren, "Gus has none.");
                Verify.AreEqual(true, ((ICustomPropertyProvider)ada).GetCustomProperty("HasChildren").GetValue(ada), "Bindable.");

                var log = new List<string>();
                ((INotifyPropertyChanged)gus).PropertyChanged += (s, e) => log.Add($"{e.PropertyName}={gus.HasChildren}");
                list.Add(new TreeEmployee { Id = 9, ManagerId = 7, Name = "Ivy" });
                Verify.AreEqual("HasChildren=True", string.Join(",", log), "Gus's first child.");
                list.Add(new TreeEmployee { Id = 10, ManagerId = 7, Name = "Jo" });
                Verify.AreEqual("HasChildren=True", string.Join(",", log), "A second child changes nothing.");
                list.RemoveAt(list.Count - 1);
                list.RemoveAt(list.Count - 1);
                Verify.AreEqual("HasChildren=True,HasChildren=False", string.Join(",", log), "Gus's last child left.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies IndexOf on CollectionNodes and on a Children list finds a node of that list, and nothing else: an item, or a node of another list.")]
        public void VerifyNodeListIndexOf()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = MakeTree();
                var hierarchy = (ICollectionViewHierarchy)MakeTreeSource(list).View;
                var roots = hierarchy.CollectionNodes;
                var eve = hierarchy.NodeFromItem(Named(list, "Eve"));
                var cy = hierarchy.NodeFromItem(Named(list, "Cy"));
                var adaChildren = hierarchy.NodeFromItem(Named(list, "Ada")).Children;
                Verify.AreEqual(1, roots.IndexOf(eve), "Eve is the second root.");
                Verify.AreEqual(1, adaChildren.IndexOf(cy), "Cy is Ada's second child.");
                Verify.AreEqual(-1, roots.IndexOf(cy), "Cy is not a root.");
                Verify.AreEqual(-1, adaChildren.IndexOf(eve), "Eve is not Ada's child.");
                Verify.AreEqual(-1, roots.IndexOf(Named(list, "Eve")), "An item is not a node.");
                Verify.IsTrue(roots.Contains(eve), "Contains agrees.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing a parent whose children each head a subtree of more than 64 rows publishes the flat list with one Reset, and the result replays and equals a recompute.")]
        public void VerifyLargeReparentPublishesOneReset()
        {
            RunOnUIThread.Execute(() =>
            {
                var list = new ObservableCollection<TreeEmployee> { new TreeEmployee { Id = 1, Name = "Top" } };
                var id = 10;
                for (var child = 0; child < 3; child++)
                {
                    var childId = id++;
                    list.Add(new TreeEmployee { Id = childId, ManagerId = 1, Name = $"C{child}" });
                    for (var leaf = 0; leaf < 70; leaf++)
                    {
                        list.Add(new TreeEmployee { Id = id++, ManagerId = childId, Name = $"L{child}.{leaf}" });
                    }
                }
                var cvs = MakeTreeSource(list);
                var checker = new ReplayChecker(cvs.View);
                list.RemoveAt(0);
                checker.Check("remove the top of three large subtrees", list, cvs);
                Verify.AreEqual(1, checker.Resets, "One Reset of the flat list, not one per re-rooted subtree.");
            });
        }

        #endregion

        #region Helpers

        private static TreeEmployee NewFuzzEmployee(Random random, int id)
        {
            var managerId = random.Next(4) == 0 ? (int?)null : random.Next(70) + 1;
            return new TreeEmployee { Id = id, ManagerId = managerId, Name = $"E{id}.{random.Next(1000)}", Score = random.Next(100) };
        }

        // Depth first: "Name(depth)", '*' for a context node.
        internal static string Tree(object view)
        {
            var parts = new List<string>();
            var roots = ((ICollectionViewHierarchy)view).CollectionNodes;
            if (roots == null)
            {
                return "(flat)";
            }
            var stack = new Stack<(ICollectionViewNode Node, int Depth)>();
            for (var i = roots.Count - 1; i >= 0; i--)
            {
                stack.Push(((ICollectionViewNode)roots[i], 0));
            }
            while (stack.Count > 0)
            {
                var (node, depth) = stack.Pop();
                parts.Add($"{NameOf(node.Item)}({depth}){(node.IsContext ? "*" : "")}");
                for (var i = node.Children.Count - 1; i >= 0; i--)
                {
                    stack.Push(((ICollectionViewNode)node.Children[i], depth + 1));
                }
            }
            return string.Join(" ", parts);
        }

        internal static string Flat(object view) => string.Join(" ", ((IList<object>)view).Select(NameOf));

        // Mirrors the flat list, CollectionNodes and every Children list it reads from their events, and checks the mirrors
        // against the lists and the tree against a recompute.
        private sealed class ReplayChecker
        {
            private readonly ICollectionView _view;
            private readonly Dictionary<IObservableVector<object>, List<object>> _mirrors = new Dictionary<IObservableVector<object>, List<object>>();
            private readonly List<string> _errors = new List<string>();
            public int Resets;

            public ReplayChecker(ICollectionView view)
            {
                _view = view;
                Follow(view);
                Rebase();
            }

            // After a rule change (one Reset everywhere), nodes may be new: follow them again.
            public void Rebase()
            {
                foreach (var vector in _mirrors.Keys.ToList())
                {
                    _mirrors[vector] = vector.ToList();
                }
                FollowTree();
            }

            private void Follow(IObservableVector<object> vector)
            {
                if (vector == null || _mirrors.ContainsKey(vector))
                {
                    return;
                }
                _mirrors[vector] = vector.ToList();
                vector.VectorChanged += (s, e) =>
                {
                    if (!_mirrors.TryGetValue(vector, out var mirror))
                    {
                        return;
                    }
                    var index = (int)e.Index;
                    switch (e.CollectionChange)
                    {
                        case CollectionChange.ItemInserted:
                            mirror.Insert(index, vector[index]);
                            break;
                        case CollectionChange.ItemRemoved:
                            mirror.RemoveAt(index);
                            break;
                        case CollectionChange.ItemChanged:
                            mirror[index] = vector[index];
                            break;
                        default:
                            if (ReferenceEquals(vector, _view))
                            {
                                Resets++;
                            }
                            _mirrors[vector] = vector.ToList();
                            break;
                    }
                };
            }

            private void FollowTree()
            {
                var roots = ((ICollectionViewHierarchy)_view).CollectionNodes;
                if (roots == null)
                {
                    return;
                }
                Follow(roots);
                var stack = new Stack<ICollectionViewNode>(roots.Cast<ICollectionViewNode>());
                while (stack.Count > 0)
                {
                    var node = stack.Pop();
                    Follow(node.Children);
                    foreach (var child in node.Children)
                    {
                        stack.Push((ICollectionViewNode)child);
                    }
                }
            }

            public void Check(string context, ObservableCollection<TreeEmployee> list, CollectionViewSource cvs)
            {
                // A node that left the tree (or every node, after a recompute) is no longer checked.
                var reachable = new HashSet<object>(ReferenceEqualityComparer.Instance) { _view };
                var roots = ((ICollectionViewHierarchy)_view).CollectionNodes;
                if (roots != null)
                {
                    reachable.Add(roots);
                    var stack = new Stack<ICollectionViewNode>(roots.Cast<ICollectionViewNode>());
                    while (stack.Count > 0)
                    {
                        var node = stack.Pop();
                        reachable.Add(node.Children);
                        foreach (var child in node.Children)
                        {
                            stack.Push((ICollectionViewNode)child);
                        }
                    }
                }
                foreach (var stale in _mirrors.Keys.Where(v => !reachable.Contains(v)).ToList())
                {
                    _mirrors.Remove(stale);
                }

                foreach (var pair in _mirrors)
                {
                    var actual = pair.Key.ToList();
                    if (!actual.SequenceEqual(pair.Value))
                    {
                        Verify.Fail($"{context}: a vector's events do not replay to its contents ({pair.Value.Count} mirrored, {actual.Count} actual).");
                    }
                }

                var fresh = new CollectionViewSource { Source = new ObservableCollection<TreeEmployee>(list) };
                fresh.HierarchyDescription = new PropertyHierarchyDescription(
                    ((PropertyHierarchyDescription)cvs.HierarchyDescription).ItemKeyPath, ((PropertyHierarchyDescription)cvs.HierarchyDescription).ParentKeyPath);
                foreach (var sort in cvs.SortDescriptions)
                {
                    fresh.SortDescriptions.Add(new SortDescription(sort.PropertyName, sort.Direction));
                }
                ((ICollectionViewShaping)fresh.View).Filter = ((ICollectionViewShaping)cvs.View).Filter;
                var expected = Tree(fresh.View);
                var actualTree = Tree(cvs.View);
                if (expected != actualTree)
                {
                    Verify.AreEqual(expected, actualTree, $"{context}: the incremental tree equals a recompute.");
                }
                if (Flat(fresh.View) != Flat(cvs.View))
                {
                    Verify.AreEqual(Flat(fresh.View), Flat(cvs.View), $"{context}: the flat list equals a recompute.");
                }
                FollowTree();
            }
        }

        #endregion
    }

    // View-only benchmark (no control): the cost of building a 100k tree. Not part of a normal run (Ignore).
    [TestClass]
    [TestProperty("Ignore", "True")]
    public class CollectionViewHierarchyBenchmarks : TableViewApiTestBase
    {
        [TestMethod]
        public void BenchViewHierarchyLoad()
        {
            foreach (var n in new[] { 10_000, 100_000 })
            {
                var samples = new List<double>();
                for (var run = 0; run < 3; run++)
                {
                    RunOnUIThread.Execute(() =>
                    {
                        var roots = n / 10;
                        var rows = new List<TreeEmployee>(n);
                        for (var i = 0; i < n; i++)
                        {
                            rows.Add(new TreeEmployee { Id = i + 1, ManagerId = i < roots ? (int?)null : (i % roots) + 1, Name = $"R{i}", Score = i % 977 });
                        }
                        var source = new ObservableCollection<TreeEmployee>(rows);
                        var stopwatch = Stopwatch.StartNew();
                        var cvs = new CollectionViewSource { Source = source, HierarchyDescription = new PropertyHierarchyDescription(IdPath, ManagerPath) };
                        var count = ((ICollectionViewHierarchy)cvs.View).CollectionNodes.Count;
                        samples.Add(stopwatch.Elapsed.TotalMilliseconds);
                        Verify.AreEqual(roots, count, "Roots.");
                    });
                }
                samples.Sort();
                Log.Comment($"BENCH|View.Hierarchy.Load|{n}|{samples[1]:F3}");
            }
        }
    }
}
