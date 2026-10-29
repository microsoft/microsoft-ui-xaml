// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Counts the handlers attached to CollectionChanged, native subscribers included.
    internal sealed class CountingCollection<T> : ObservableCollection<T>
    {
        private NotifyCollectionChangedEventHandler _handlers;

        public CountingCollection(IEnumerable<T> items) : base(items) { }

        public int HandlerCount => _handlers?.GetInvocationList().Length ?? 0;

        public override event NotifyCollectionChangedEventHandler CollectionChanged
        {
            add { _handlers += value; }
            remove { _handlers -= value; }
        }

        protected override void OnCollectionChanged(NotifyCollectionChangedEventArgs e)
        {
            _handlers?.Invoke(this, e);
        }
    }

    // TableViewSource2, Milestone 3: ParentBy / ChildrenBy build a source tree; the bound TableView
    // shows it through TableViewRow.Level / IsExpandable / IsExpanded (see RowLabel).
    public partial class TableViewSource2Tests
    {
        // Roots Ann, Dee. ChildrenBy: Ann > { Bob > { Gil }, Cat }; Dee has no children (null).
        private sealed class ChildrenFixture
        {
            public ObservableCollection<LiveRow> Roots;
            public Dictionary<LiveRow, CountingCollection<LiveRow>> Children = new Dictionary<LiveRow, CountingCollection<LiveRow>>();
            public LiveRow Ann, Bob, Cat, Dee, Gil;

            public TableViewChildrenSelector2 Selector =>
                new TableViewChildrenSelector2(o => Children.TryGetValue((LiveRow)o, out var c) ? c : null);
        }

        private static ChildrenFixture MakeChildrenFixture()
        {
            var f = new ChildrenFixture
            {
                Ann = new LiveRow { Id = 1, Name = "Ann" },
                Bob = new LiveRow { Id = 2, Name = "Bob" },
                Cat = new LiveRow { Id = 3, Name = "Cat" },
                Dee = new LiveRow { Id = 4, Name = "Dee" },
                Gil = new LiveRow { Id = 5, Name = "Gil" },
            };
            f.Roots = new ObservableCollection<LiveRow>(new[] { f.Ann, f.Dee });
            f.Children[f.Ann] = new CountingCollection<LiveRow>(new[] { f.Bob, f.Cat });
            f.Children[f.Bob] = new CountingCollection<LiveRow>(new[] { f.Gil });
            return f;
        }

        // The source binds to the creating thread, so it is created on the UI thread.
        private static TableViewSource2 FromOnUI(object items)
        {
            TableViewSource2 source = null;
            RunOnUIThread.Execute(() => source = TableViewSource2.From(items));
            return source;
        }

        // Binds source to a loaded single-column tree table and applies hierarchy (may be null).
        private TableView PlaceTreeSource(TableViewSource2 source, Action<TableViewSource2> hierarchy)
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                hierarchy?.Invoke(source);
                tableView = CreateTreeTable(source);
                LoadContent(tableView);
            });
            SettleLayout(tableView);
            return tableView;
        }

        private static void ExpandAll(TableView tableView)
        {
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);
        }

        private static void VerifyTreeOnUI(TableView tableView, string expected, string context)
            => RunOnUIThread.Execute(() => VerifyTree(tableView, expected, context));

        private static void OnUI(TableView tableView, Action action)
        {
            RunOnUIThread.Execute(action);
            SettleLayout(tableView);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ParentBy builds the key tree: roots collapsed, ExpandAllRows shows every level in source order (Task 1, 4).")]
        public void VerifyParentByBuildsTree()
        {
            var source = FromOnUI(MakeTree());
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));

            VerifyTreeOnUI(tableView, "Ada1+ Eve1+ Gus1", "ParentBy, collapsed");
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1", "ParentBy, expanded");
            RunOnUIThread.Execute(() => Verify.AreEqual(7, ProjectedCount(tableView), "Every item must project when expanded."));

            OnUI(tableView, () => tableView.CollapseAllRows());
            VerifyTreeOnUI(tableView, "Ada1+ Eve1+ Gus1", "CollapseAllRows");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ChildrenBy builds the tree from child collections; a null collection is a leaf (Task 2, 4).")]
        public void VerifyChildrenByBuildsTree()
        {
            var f = MakeChildrenFixture();
            var source = FromOnUI(f.Roots);
            var tableView = PlaceTreeSource(source, s => s.ChildrenBy(f.Selector));

            VerifyTreeOnUI(tableView, "Ann1+ Dee1", "ChildrenBy, collapsed");
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Ann1- Bob2- Gil3 Cat2 Dee1", "ChildrenBy, expanded");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an item whose parent key matches no item is a top-level row (Task 1).")]
        public void VerifyParentByOrphanBecomesTopLevel()
        {
            var people = MakeTree();
            Named(people, "Fay").ManagerId = 42;
            var source = FromOnUI(people);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));

            // Gus (ManagerId 99) and Fay (42) are orphans; Eve loses her only child.
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1 Fay1 Gus1", "Orphans at the top level, source order");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a ParentBy whose keys form a cycle throws and leaves the flat projection; a live edit that closes a cycle moves the edited row to the top level with its subtree and throws, and the row returns to its parent once the key is valid (Task 1, 3; rulings 4, 6).")]
        public void VerifyParentByCycleIsBroken()
        {
            var rows = MakeLiveTree();
            var ann = Named(rows, "Ann");
            ann.ParentId = 2;
            var source = FromOnUI(rows);
            var tableView = PlaceTreeSource(source, null);

            RunOnUIThread.Execute(() =>
            {
                VerifyArgumentException(() => source.ParentBy(LiveById, LiveByParent), "cycle", "ParentBy over a cycle");
                VerifyTree(tableView, "Ann Bob Cat Dee Eli", "A rejected ParentBy keeps the flat projection");
            });

            OnUI(tableView, () =>
            {
                ann.ParentId = null;
                source.IsLiveShaping = true;
                source.ParentBy(LiveById, LiveByParent);
            });
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Ann1- Bob2 Cat2 Dee1- Eli2", "Valid ParentBy after the rejection");

            // Dee > Eli; Dee.ParentId = 5 closes Dee -> Eli -> Dee. Dee was edited: it stays at the top level with Eli.
            OnUI(tableView, () => VerifyArgumentException(() => Named(rows, "Dee").ParentId = 5, "cycle", "Live edit closing a cycle"));
            VerifyTreeOnUI(tableView, "Ann1- Bob2 Cat2 Dee1- Eli2", "Live cycle: the edited row stays top level with its subtree");
            RunOnUIThread.Execute(() => Verify.AreEqual(5, ProjectedCount(tableView), "No row is lost to the cycle."));

            // Eli moves under Ann, so Dee's parent key (5) is valid again: Dee moves under Eli.
            OnUI(tableView, () => Named(rows, "Eli").ParentId = 1);
            VerifyTreeOnUI(tableView, "Ann1- Bob2 Cat2 Eli2- Dee3", "Dee returns to its parent once the cycle is gone");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ParentBy rejects duplicate keys, self-parents and null selectors with ArgumentException and the projection intact (Task 1; rulings 4, 20).")]
        public void VerifyParentByRejectsInvalidKeys()
        {
            var rows = MakeLiveTree();
            var source = FromOnUI(rows);
            var tableView = PlaceTreeSource(source, null);

            RunOnUIThread.Execute(() =>
            {
                VerifyArgumentException(() => source.ParentBy(null, LiveByParent), "required", "Null key selector");
                VerifyArgumentException(() => source.ParentBy(LiveById, null), "required", "Null parent key selector");
                VerifyArgumentException(() => source.ChildrenBy(null), "required", "Null children selector");
                VerifyArgumentException(() => source.ParentBy(LiveKey(r => r.Team), LiveByParent), "same key", "Duplicate keys");

                Named(rows, "Dee").ParentId = 4;
                VerifyArgumentException(() => source.ParentBy(LiveById, LiveByParent), "own parent", "Self-parent");
                VerifyTree(tableView, "Ann Bob Cat Dee Eli", "Rejected verbs keep the flat projection");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an Add to a child collection inserts a child row, and a Remove removes it (Task 2).")]
        public void VerifyChildFeedAddInsertsChild()
        {
            var f = MakeChildrenFixture();
            var source = FromOnUI(f.Roots);
            var tableView = PlaceTreeSource(source, s => s.ChildrenBy(f.Selector));
            ExpandAll(tableView);

            var hal = new LiveRow { Id = 6, Name = "Hal" };
            OnUI(tableView, () => f.Children[f.Ann].Add(hal));
            VerifyTreeOnUI(tableView, "Ann1- Bob2- Gil3 Cat2 Hal2 Dee1", "Child Add");

            OnUI(tableView, () => f.Children[f.Ann].Insert(0, new LiveRow { Id = 7, Name = "Ivy" }));
            VerifyTreeOnUI(tableView, "Ann1- Ivy2 Bob2- Gil3 Cat2 Hal2 Dee1", "Child Insert at 0");

            OnUI(tableView, () => f.Children[f.Ann].Remove(f.Cat));
            VerifyTreeOnUI(tableView, "Ann1- Ivy2 Bob2- Gil3 Hal2 Dee1", "Child Remove");

            // Removing Bob removes his subtree.
            OnUI(tableView, () => f.Children[f.Ann].Remove(f.Bob));
            VerifyTreeOnUI(tableView, "Ann1- Ivy2 Hal2 Dee1", "Child Remove with a subtree");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ChildrenBy rejects an item that appears twice in the tree and keeps the flat projection (Task 2).")]
        public void VerifyChildrenByDuplicateChildRejected()
        {
            var f = MakeChildrenFixture();
            f.Children[f.Dee] = new CountingCollection<LiveRow>(new[] { f.Cat });
            var source = FromOnUI(f.Roots);
            var tableView = PlaceTreeSource(source, null);

            RunOnUIThread.Execute(() =>
            {
                VerifyArgumentException(() => source.ChildrenBy(f.Selector), "more than once", "Cat under Ann and Dee");
                VerifyTree(tableView, "Ann Dee", "A rejected ChildrenBy keeps the flat projection");
                Verify.AreEqual(0, f.Children[f.Ann].HandlerCount, "A rejected ChildrenBy subscribes to nothing.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearHierarchy restores the flat source-order projection (Task 1, 4).")]
        public void VerifyClearHierarchyRestoresFlat()
        {
            var source = FromOnUI(MakeTree());
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            ExpandAll(tableView);

            OnUI(tableView, () => source.ClearHierarchy());
            VerifyTreeOnUI(tableView, "Ada Ben Cy Dan Eve Fay Gus", "ClearHierarchy");

            // ClearHierarchy without a hierarchy is a no-op; the verbs chain.
            OnUI(tableView, () => Verify.AreSame(source, source.ClearHierarchy(), "ClearHierarchy returns the source."));
            VerifyTreeOnUI(tableView, "Ada Ben Cy Dan Eve Fay Gus", "Second ClearHierarchy");

            OnUI(tableView, () => source.ParentBy(ById, ByManager));
            // Expanded flags live on the records, so they survive the flat round trip.
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1", "ParentBy again after ClearHierarchy");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a throwing hierarchy selector propagates out of the verb and leaves the projection intact (Task 1, 2; Review Focus 1).")]
        public void VerifyHierarchySelectorThrowsPropagates()
        {
            var people = MakeTree();
            var source = FromOnUI(people);
            var tableView = PlaceTreeSource(source, null);
            var throwingKey = EmpKey(e => e.Name == "Dan" ? throw new InvalidOperationException("key selector failed") : (object)e.Id);

            RunOnUIThread.Execute(() =>
            {
                VerifyThrows<InvalidOperationException>(() => source.ParentBy(throwingKey, ByManager), "ParentBy must propagate the key selector's exception");
                VerifyTree(tableView, "Ada Ben Cy Dan Eve Fay Gus", "Flat after a throwing ParentBy");
            });

            // The previous hierarchy also survives a throwing replacement.
            OnUI(tableView, () => source.ParentBy(ById, ByManager));
            RunOnUIThread.Execute(() =>
            {
                VerifyThrows<InvalidOperationException>(() => source.ParentBy(ById, EmpKey(e => e.Name == "Dan" ? throw new InvalidOperationException("key selector failed") : (object)e.ManagerId)), "ParentBy must propagate the parent key selector's exception");
                VerifyThrows<InvalidOperationException>(
                    () => source.ChildrenBy(new TableViewChildrenSelector2(o => throw new InvalidOperationException("children selector failed"))),
                    "ChildrenBy must propagate the children selector's exception");
                VerifyTree(tableView, "Ada1+ Eve1+ Gus1", "The ParentBy tree survives throwing replacements");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a root Add under ParentBy places the item under its parent, and adopts existing orphans whose parent key it matches (Task 3; Review Focus 4).")]
        public void VerifyParentByRootAddReparents()
        {
            var people = MakeTree();
            var source = FromOnUI(people);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            ExpandAll(tableView);

            OnUI(tableView, () => people.Add(new TreeEmployee { Id = 8, ManagerId = 3, Name = "Hal" }));
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2- Hal3 Eve1- Fay2 Gus1", "Add under Cy (Cy keeps its expanded flag)");

            // Zed (Id 99) adopts the orphan Gus; Zed is new, so collapsed.
            OnUI(tableView, () => people.Insert(0, new TreeEmployee { Id = 99, Name = "Zed" }));
            VerifyTreeOnUI(tableView, "Zed1+ Ada1- Ben2- Dan3 Cy2- Hal3 Eve1- Fay2", "Insert at 0 adopts Gus");

            OnUI(tableView, () => tableView.ExpandAllRows());
            VerifyTreeOnUI(tableView, "Zed1- Gus2 Ada1- Ben2- Dan3 Cy2- Hal3 Eve1- Fay2", "Gus under Zed");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a root Remove under ParentBy orphans the removed item's children to the top level, in source order (Task 3; Review Focus 4).")]
        public void VerifyParentByRootRemoveOrphansChildren()
        {
            var people = MakeTree();
            var source = FromOnUI(people);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            ExpandAll(tableView);

            OnUI(tableView, () => people.Remove(Named(people, "Ben")));
            VerifyTreeOnUI(tableView, "Ada1- Cy2 Dan1 Eve1- Fay2 Gus1", "Remove Ben: Dan becomes top level");

            OnUI(tableView, () => people.Remove(Named(people, "Eve")));
            VerifyTreeOnUI(tableView, "Ada1- Cy2 Dan1 Fay1 Gus1", "Remove Eve: Fay becomes top level");

            OnUI(tableView, () => people.Move(0, people.Count - 1));
            VerifyTreeOnUI(tableView, "Dan1 Fay1 Gus1 Ada1- Cy2", "Move Ada to the end");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a parent-key PropertyChanged under live shaping re-parents the row (Task 3; rulings 5, 12).")]
        public void VerifyParentByLiveParentKeyChange()
        {
            var rows = MakeLiveTree();
            var source = FromOnUI(rows);
            var tableView = PlaceTreeSource(source, s =>
            {
                s.IsLiveShaping = true;
                s.ParentBy(LiveById, LiveByParent);
            });
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Ann1- Bob2 Cat2 Dee1- Eli2", "Live tree");

            OnUI(tableView, () => Named(rows, "Cat").ParentId = 4);
            VerifyTreeOnUI(tableView, "Ann1- Bob2 Dee1- Cat2 Eli2", "Cat moves under Dee");

            OnUI(tableView, () => Named(rows, "Bob").ParentId = null);
            VerifyTreeOnUI(tableView, "Ann1 Bob1 Dee1- Cat2 Eli2", "Bob becomes top level; Ann is a leaf");

            // A non-key property change does not relink.
            OnUI(tableView, () => Named(rows, "Eli").Score = 99);
            VerifyTreeOnUI(tableView, "Ann1 Bob1 Dee1- Cat2 Eli2", "Non-key change");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearHierarchy unsubscribes every child collection, so later child changes reach nothing (Task 2; Review Focus 3).")]
        public void VerifyClearHierarchyUnsubscribesChildFeeds()
        {
            var f = MakeChildrenFixture();
            var source = FromOnUI(f.Roots);
            var tableView = PlaceTreeSource(source, s => s.ChildrenBy(f.Selector));
            ExpandAll(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, f.Children[f.Ann].HandlerCount, "ChildrenBy subscribes once to Ann's children.");
                Verify.AreEqual(1, f.Children[f.Bob].HandlerCount, "ChildrenBy subscribes once to Bob's children.");
            });

            OnUI(tableView, () => source.ClearHierarchy());
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, f.Children[f.Ann].HandlerCount, "ClearHierarchy unsubscribes Ann's children.");
                Verify.AreEqual(0, f.Children[f.Bob].HandlerCount, "ClearHierarchy unsubscribes Bob's children.");
                VerifyTree(tableView, "Ann Dee", "Flat after ClearHierarchy");
            });

            OnUI(tableView, () =>
            {
                f.Children[f.Ann].Add(new LiveRow { Id = 9, Name = "Hal" });
                f.Children[f.Bob].Clear();
            });
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ann Dee", "Child changes after ClearHierarchy reach nothing");
                Verify.AreEqual(2, ProjectedCount(tableView), "Two root rows.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Refresh under ParentBy reuses records, so expansion survives, and picks up a changed parent key (Task 1; Review Focus 5).")]
        public void VerifyRefreshUnderHierarchyKeepsExpansion()
        {
            var people = MakeTree();
            var source = FromOnUI(people);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            ExpandAll(tableView);

            // TreeEmployee does not notify: the change is seen only by Refresh.
            Named(people, "Cy").ManagerId = 5;
            OnUI(tableView, () => source.Refresh());
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Eve1- Cy2 Fay2 Gus1", "Refresh re-links Cy and keeps every expanded row");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Refresh under ChildrenBy reuses root and child records, so expansion survives, and picks up child collection changes (Task 2; Review Focus 5).")]
        public void VerifyChildrenByRefreshKeepsExpansion()
        {
            var f = MakeChildrenFixture();
            var source = FromOnUI(f.Roots);
            var tableView = PlaceTreeSource(source, s => s.ChildrenBy(f.Selector));
            ExpandAll(tableView);

            // Dee gains a child collection the selector did not return before: only Refresh sees it.
            var jo = new LiveRow { Id = 8, Name = "Jo" };
            f.Children[f.Dee] = new CountingCollection<LiveRow>(new[] { jo });
            OnUI(tableView, () => source.Refresh());

            // Ann, Bob and Dee keep Expanded (Dee was expanded as a leaf by ExpandAllRows).
            VerifyTreeOnUI(tableView, "Ann1- Bob2- Gil3 Cat2 Dee1- Jo2", "Refresh keeps expansion and adds Jo");
            RunOnUIThread.Execute(() => Verify.AreEqual(1, f.Children[f.Dee].HandlerCount, "Refresh subscribes once to the new feed."));
        }

        // Records every CollectionChanged the bound view raises from now on, as "Add@i", "Remove@i",
        // "Move@old>new", "Replace@i" or "Reset".
        private static List<string> RecordViewEvents(TableView tableView, Action<NotifyCollectionChangedEventArgs> onEvent = null)
        {
            var events = new List<string>();
            RunOnUIThread.Execute(() => GetRowsRepeater(tableView).ItemsSourceView.CollectionChanged += (s, e) =>
            {
                switch (e.Action)
                {
                    case NotifyCollectionChangedAction.Add: events.Add($"Add@{e.NewStartingIndex}"); break;
                    case NotifyCollectionChangedAction.Remove: events.Add($"Remove@{e.OldStartingIndex}"); break;
                    case NotifyCollectionChangedAction.Move: events.Add($"Move@{e.OldStartingIndex}>{e.NewStartingIndex}"); break;
                    case NotifyCollectionChangedAction.Replace: events.Add($"Replace@{e.NewStartingIndex}"); break;
                    default: events.Add("Reset"); break;
                }

                onEvent?.Invoke(e);
            });
            return events;
        }

        private static string Describe(List<string> events)
            => string.Join(",", events);

        private static void VerifyEvents(List<string> events, string expected, string context)
        {
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(expected, Describe(events), $"View events ({context}).");
                events.Clear();
            });
        }

        private static Exception Thrown(Action action)
        {
            try
            {
                action();
                return null;
            }
            catch (Exception ex)
            {
                return ex;
            }
        }

        private static void VerifyThrows<T>(Action action, string context) where T : Exception
        {
            var ex = Thrown(action);
            Verify.IsNotNull(ex, $"{context}: expected an exception.");
            Verify.AreEqual(typeof(T), ex.GetType(), $"{context}: exception type (was {ex.GetType().Name}: {ex.Message}).");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an insert that adopts one orphan is surgical: the view sees targeted changes and no Reset (Task 3, 4; ruling 5; Review Focus 6).")]
        public void VerifyParentBySingleAdoptionIsSurgical()
        {
            var people = MakeTree();
            var source = FromOnUI(people);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            ExpandAll(tableView);
            var events = RecordViewEvents(tableView);

            // Zed is added collapsed at the top; adopting Gus is one Move, and Gus is hidden under collapsed Zed.
            OnUI(tableView, () => people.Insert(0, new TreeEmployee { Id = 99, Name = "Zed" }));
            VerifyTreeOnUI(tableView, "Zed1+ Ada1- Ben2- Dan3 Cy2 Eve1- Fay2", "Zed adopts Gus");
            VerifyEvents(events, "Add@0,Remove@7", "single adoption");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an insert that adopts more than one orphan is bulk: the view sees one Reset (Task 1, 4; ruling 5; Review Focus 6).")]
        public void VerifyParentByBulkAdoptionResets()
        {
            var people = MakeTree();
            Named(people, "Fay").ManagerId = 99;
            var source = FromOnUI(people);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1 Fay1 Gus1", "Fay and Gus are orphans");
            var events = RecordViewEvents(tableView);

            OnUI(tableView, () => people.Insert(0, new TreeEmployee { Id = 99, Name = "Zed" }));
            VerifyTreeOnUI(tableView, "Zed1+ Ada1- Ben2- Dan3 Cy2 Eve1", "Zed adopts Fay and Gus; expansion survives the Reset");
            VerifyEvents(events, "Add@0,Reset", "adopting two orphans: Zed's Add, then one Reset");

            OnUI(tableView, () => tableView.ExpandAllRows());
            VerifyTreeOnUI(tableView, "Zed1- Fay2 Gus2 Ada1- Ben2- Dan3 Cy2 Eve1", "Fay and Gus under Zed, in source order");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a parent-key edit and the removal of a parent are surgical: targeted changes, no Reset, and the removed parent's children go to the top level (Task 3, 4; rulings 5, 9; Review Focus 6).")]
        public void VerifyParentBySurgicalEditsRaiseNoReset()
        {
            var rows = MakeLiveTree();
            var source = FromOnUI(rows);
            var tableView = PlaceTreeSource(source, s =>
            {
                s.IsLiveShaping = true;
                s.ParentBy(LiveById, LiveByParent);
            });
            ExpandAll(tableView);
            var events = RecordViewEvents(tableView);

            // Flat: Ann0 Bob1 Cat2 Dee3 Eli4. Cat becomes Dee's first child: out at 2, in at 3.
            OnUI(tableView, () => Named(rows, "Cat").ParentId = 4);
            VerifyTreeOnUI(tableView, "Ann1- Bob2 Dee1- Cat2 Eli2", "Cat moves under Dee");
            VerifyEvents(events, "Remove@2,Add@3", "parent-key edit");

            // Flat: Ann0 Bob1 Dee2 Cat3 Eli4. Cat and Eli are orphaned one Move each, in source order (each lands
            // before the leaving Dee, so Cat is out at 3 and in at 2, Eli out at 4 and in at 3), then Dee leaves at 4.
            OnUI(tableView, () => rows.Remove(Named(rows, "Dee")));
            VerifyTreeOnUI(tableView, "Ann1- Bob2 Cat1 Eli1", "Dee removed: Cat and Eli go to the top level in source order");
            VerifyEvents(events, "Remove@3,Add@2,Remove@4,Add@3,Remove@4", "parent removal");
            RunOnUIThread.Execute(() => Verify.AreEqual(4, ProjectedCount(tableView), "Four rows remain."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a realized row's expander follows its first child coming and going, collapsed or expanded, with no Reset (Task 4 Step 9; ruling 24; drift #21; Review Focus 6).")]
        public void VerifyParentByExpanderFollowsFirstChild()
        {
            var people = MakeTree();
            var source = FromOnUI(people);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            VerifyTreeOnUI(tableView, "Ada1+ Eve1+ Gus1", "collapsed; Gus is a leaf");
            var events = RecordViewEvents(tableView);

            // Collapsed parent: the child is linked hidden, so no row event is raised at all.
            var hal = new TreeEmployee { Id = 8, ManagerId = 7, Name = "Hal" };
            OnUI(tableView, () => people.Add(hal));
            VerifyTreeOnUI(tableView, "Ada1+ Eve1+ Gus1+", "Gus gains an expander for his first child");
            VerifyEvents(events, "", "child under a collapsed parent");
            RunOnUIThread.Execute(() => Verify.AreEqual(3, ProjectedCount(tableView), "Hal stays hidden under collapsed Gus."));

            OnUI(tableView, () => people.Remove(hal));
            VerifyTreeOnUI(tableView, "Ada1+ Eve1+ Gus1", "Gus loses the expander with his last child");
            VerifyEvents(events, "", "hidden child removed");

            // Expanded parent: ExpandAllRows also marks the leaf Cy expanded, so his first child shows.
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1", "expanded; Cy is a leaf");
            RunOnUIThread.Execute(() => events.Clear());

            // Flat: Ada0 Ben1 Dan2 Cy3 Eve4 Fay5 Gus6. Ivy shows right after Cy.
            var ivy = new TreeEmployee { Id = 9, ManagerId = 3, Name = "Ivy" };
            OnUI(tableView, () => people.Add(ivy));
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2- Ivy3 Eve1- Fay2 Gus1", "Cy gains an expander and shows Ivy");
            VerifyEvents(events, "Add@4", "child under an expanded parent");

            OnUI(tableView, () => people.Remove(ivy));
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1", "Cy loses the expander with Ivy");
            VerifyEvents(events, "Remove@4", "shown child removed");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that after the verb a duplicate key or a null key excludes the new row (it behaves as filtered out), the exception still propagates from Add, and Refresh re-evaluates the excluded row (Task 1, 3; ruling 6; Review Focus 7).")]
        public void VerifyParentByExcludesBadKeys()
        {
            var people = MakeTree();
            var source = FromOnUI(people);
            var byIdOrNull = EmpKey(e => e.Name == "Nul" ? null : (object)e.Id);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(byIdOrNull, ByManager));
            ExpandAll(tableView);
            const string expanded = "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1";

            // Cy already holds key 3: Cy keeps it and Dup is excluded.
            RunOnUIThread.Execute(() =>
                VerifyArgumentException(() => people.Add(new TreeEmployee { Id = 3, ManagerId = 1, Name = "Dup" }), "same key", "Duplicate key after the verb"));
            SettleLayout(tableView);
            VerifyTreeOnUI(tableView, expanded, "The duplicate is excluded; the holder keeps its key");
            RunOnUIThread.Execute(() => Verify.AreEqual(7, ProjectedCount(tableView), "An excluded row does not project."));

            RunOnUIThread.Execute(() =>
                VerifyArgumentException(() => people.Add(new TreeEmployee { Id = 50, ManagerId = 1, Name = "Nul" }), "empty key", "Null key after the verb"));
            SettleLayout(tableView);
            VerifyTreeOnUI(tableView, expanded, "The null-keyed row is excluded");

            // Cy leaves, so key 3 is free; Refresh re-evaluates Dup (now valid, under Ada) and Nul (still excluded).
            OnUI(tableView, () => people.Remove(Named(people, "Cy")));
            OnUI(tableView, () => source.Refresh());
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Dup2 Eve1- Fay2 Gus1", "Refresh includes Dup; Nul stays excluded");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that after the verb a throwing parent-key selector places the row at the top level and a throwing children selector makes the row a leaf; both exceptions propagate (Task 1, 2; rulings 6, 14; Review Focus 7).")]
        public void VerifyHierarchySelectorThrowsPlacesRow()
        {
            var people = MakeTree();
            var source = FromOnUI(people);
            var throwingParent = EmpKey(e => e.Name == "Hal" ? throw new InvalidOperationException("parent key selector failed") : (object)e.ManagerId);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, throwingParent));
            ExpandAll(tableView);

            RunOnUIThread.Execute(() =>
                VerifyThrows<InvalidOperationException>(() => people.Add(new TreeEmployee { Id = 8, ManagerId = 3, Name = "Hal" }), "Add must propagate the parent key selector's exception"));
            SettleLayout(tableView);
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1 Hal1", "Hal is at the top level, like an orphan");

            var f = MakeChildrenFixture();
            var boom = new LiveRow { Id = 9, Name = "Boom" };
            var childrenSource = FromOnUI(f.Roots);
            var throwingChildren = new TableViewChildrenSelector2(o =>
                o == boom ? throw new InvalidOperationException("children selector failed") : (f.Children.TryGetValue((LiveRow)o, out var c) ? c : null));
            var childTable = PlaceTreeSource(childrenSource, s => s.ChildrenBy(throwingChildren));
            ExpandAll(childTable);

            RunOnUIThread.Execute(() =>
                VerifyThrows<InvalidOperationException>(() => f.Children[f.Ann].Add(boom), "A child Add must propagate the children selector's exception"));
            SettleLayout(childTable);
            VerifyTreeOnUI(childTable, "Ann1- Bob2- Gil3 Cat2 Boom2 Dee1", "Boom is shown as a leaf");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Level is 1-based for hierarchical rows and 0 for a flat projection (Task 4; user ruling).")]
        public void VerifyHierarchicalLevelIsOneBased()
        {
            var source = FromOnUI(MakeTree());
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            ExpandAll(tableView);

            string Levels() => string.Join(" ", GetProjectedElements(tableView).OfType<TableViewRow>().Select(r => r.Level));

            RunOnUIThread.Execute(() => Verify.AreEqual("1 2 3 2 1 2 1", Levels(), "Roots are Level 1, their children 2, grandchildren 3."));

            OnUI(tableView, () => source.ClearHierarchy());
            RunOnUIThread.Execute(() => Verify.AreEqual("0 0 0 0 0 0 0", Levels(), "A flat projection is Level 0."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing a record whose children re-key in the same turn reports the exact flat indices: an ancestor-bound Move lands before the old parent (Task 4 I1 regression).")]
        public void VerifyParentByRekeyToAncestorAfterRemoveIndices()
        {
            // Source order X G A A1 P Q. Tree: G > { A > { A1 }, P > { X }, Q }.
            var g = new LiveRow { Id = 1, Name = "G" };
            var a = new LiveRow { Id = 2, Name = "A", ParentId = 1 };
            var a1 = new LiveRow { Id = 3, Name = "A1", ParentId = 2 };
            var p = new LiveRow { Id = 4, Name = "P", ParentId = 1 };
            var q = new LiveRow { Id = 5, Name = "Q", ParentId = 1 };
            var x = new LiveRow { Id = 6, Name = "X", ParentId = 4 };
            var rows = new ObservableCollection<LiveRow>(new[] { x, g, a, a1, p, q });
            var source = FromOnUI(rows);
            var tableView = PlaceTreeSource(source, s =>
            {
                s.IsLiveShaping = true;
                s.ParentBy(LiveById, LiveByParent);
            });
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "G1- A2- A13 P2- X3 Q2", "G > { A > A1, P > X, Q }");
            var events = RecordViewEvents(tableView);

            OnUI(tableView, () => rows.Remove(q));
            VerifyTreeOnUI(tableView, "G1- A2- A13 P2- X3", "Q removed");
            VerifyEvents(events, "Remove@5", "Q removed");

            // X (flat 4) becomes G's first child (source order X < A): out at 4, back in at 1.
            OnUI(tableView, () => x.ParentId = 1);
            VerifyTreeOnUI(tableView, "G1- X2 A2- A13 P2", "X re-keyed to G");
            VerifyEvents(events, "Remove@4,Add@1", "X re-keyed to an ancestor");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a source Move within one parent's children reorders the siblings, both directions, with exact view events (Task 3; ruling 7).")]
        public void VerifyParentBySiblingReorder()
        {
            var people = MakeTree();
            var source = FromOnUI(people);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1", "start");
            var events = RecordViewEvents(tableView);

            // Source order Ada Ben Cy Dan ...: Cy (index 2) moves before Ben (index 1).
            OnUI(tableView, () => people.Move(2, 1));
            VerifyTreeOnUI(tableView, "Ada1- Cy2 Ben2- Dan3 Eve1- Fay2 Gus1", "Cy moved before Ben");
            VerifyEvents(events, "Remove@3,Add@1", "backward sibling reorder");

            // And forward again: Cy (index 1) moves after Ben (index 2).
            OnUI(tableView, () => people.Move(1, 2));
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1", "Cy moved after Ben");
            VerifyEvents(events, "Remove@1,Add@3", "forward sibling reorder");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an app handler on the projection that throws during a ParentBy removal with children leaves a consistent tree: the exact removal events still arrive, a Refresh re-reads the same tree and a later Add works (Task 3 zombie path, as far as the public API reaches).")]
        public void VerifyParentByThrowingViewHandlerDuringRemovalRecovers()
        {
            var people = MakeTree();
            var source = FromOnUI(people);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            ExpandAll(tableView);

            // XAML's ItemsSourceView absorbs the app handler's exception before the engine's listener
            // sees it, so the engine's listenerFault path itself is not reachable from the public API.
            var thrown = 0;
            var events = RecordViewEvents(tableView, e =>
            {
                if (thrown++ == 0)
                {
                    throw new InvalidOperationException("listener failed");
                }
            });

            // Flat: Ada0 Ben1 Dan2 Cy3 Eve4 Fay5 Gus6. Dan (Ben's child) is orphaned to the top level first,
            // after Ada's subtree, then Ben's row leaves.
            OnUI(tableView, () => people.Remove(Named(people, "Ben")));
            RunOnUIThread.Execute(() => Verify.IsTrue(thrown >= 1, "The throwing handler must have run."));
            VerifyEvents(events, "Remove@2,Add@3,Remove@1", "removal with a throwing handler");
            VerifyTreeOnUI(tableView, "Ada1- Cy2 Dan1 Eve1- Fay2 Gus1", "Consistent after the throw");

            OnUI(tableView, () => source.Refresh());
            VerifyTreeOnUI(tableView, "Ada1- Cy2 Dan1 Eve1- Fay2 Gus1", "Refresh re-reads the same tree");

            OnUI(tableView, () => people.Add(new TreeEmployee { Id = 8, ManagerId = 3, Name = "Hal" }));
            VerifyTreeOnUI(tableView, "Ada1- Cy2- Hal3 Dan1 Eve1- Fay2 Gus1", "A later Add under Cy");
        }
        [TestMethod]
        [TestProperty("Description", "Verifies a ChildrenBy root Replace whose replacement shares the leaving root's child collection is accepted, not rejected as a duplicate (Task 3 I3).")]
        public void VerifyChildrenByReplaceSharingChildrenAccepted()
        {
            var f = MakeChildrenFixture();
            var source = FromOnUI(f.Roots);
            var tableView = PlaceTreeSource(source, s => s.ChildrenBy(f.Selector));
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Ann1- Bob2- Gil3 Cat2 Dee1", "start");

            var clone = new LiveRow { Id = 10, Name = "Anna" };
            f.Children[clone] = f.Children[f.Ann];
            OnUI(tableView, () => f.Roots[0] = clone);

            // The clone is a new record, so collapsed; its feed replaces the old one.
            VerifyTreeOnUI(tableView, "Anna1+ Dee1", "Replacement accepted");
            RunOnUIThread.Execute(() => Verify.AreEqual(1, f.Children[clone].HandlerCount, "The shared collection is subscribed exactly once."));
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Anna1- Bob2- Gil3 Cat2 Dee1", "The shared children show under the clone");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an Add to a child collection that duplicates an item elsewhere in the tree is rejected with ArgumentException and the source stays consistent (Task 2).")]
        public void VerifyChildFeedDuplicateAddRejected()
        {
            var f = MakeChildrenFixture();
            f.Children[f.Dee] = new CountingCollection<LiveRow>(new LiveRow[0]);
            var source = FromOnUI(f.Roots);
            var tableView = PlaceTreeSource(source, s => s.ChildrenBy(f.Selector));
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Ann1- Bob2- Gil3 Cat2 Dee1", "start (Dee has an empty feed)");
            RunOnUIThread.Execute(() =>
                VerifyArgumentException(() => f.Children[f.Dee].Add(f.Cat), "more than once", "Cat is already under Ann"));
            SettleLayout(tableView);
            VerifyTreeOnUI(tableView, "Ann1- Bob2- Gil3 Cat2 Dee1", "Cat still appears once");
        }
    }
}
