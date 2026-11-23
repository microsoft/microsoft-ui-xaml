// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using System.Threading.Tasks;
using Windows.Foundation;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

using CollectionChange = global::Windows.Foundation.Collections.CollectionChange;
using SortDescription = Microsoft.UI.Xaml.Data.SortDescription;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // The CollectionViewSource shaping engine (VectorCollectionView) as TableView consumes it: incremental
    // updates against a full refresh, re-entrancy, incremental loading, lifetime, currency, key comparison
    // and live shaping, plus TableView's live and tree paths over it.
    [TestClass]
    public class TableViewViewEngineTests : TableViewApiTestBase
    {
        #region Incremental updates equal a full refresh

        [TestMethod]
        [TestProperty("Description", "Verifies random source changes, rule changes and refreshes leave a flat, grouped and two-level grouped view (groups ordered by name, or by ItemCount) equal to a fresh view over the same items.")]
        public void VerifyIncrementalMatchesFreshViewFuzz()
        {
            CollectGarbageAndWaitForIdle();
            foreach (var levels in new[] { 0, 1, 2, 3 })
            {
                RunOnUIThread.Execute(() =>
                {
                    var random = new Random(4711 + levels);
                    var fixture = new EngineFixture(random, levels, isLive: false);
                    for (var step = 0; step < 250; step++)
                    {
                        var op = fixture.ApplyRandomOperation(allowPropertyEdits: false);
                        fixture.VerifyMatchesFresh($"levels {levels}, step {step} ({op})");
                    }
                });
            }
            CollectGarbageAndWaitForIdle();
        }

        // Views and groups the GC finds unreachable are released on the UI thread; drained here, not inside a later wait.
        private static void CollectGarbageAndWaitForIdle()
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
            IdleSynchronizer.Wait();
        }

        private static string GcCounts() => $"gc {GC.CollectionCount(0)}/{GC.CollectionCount(1)}/{GC.CollectionCount(2)}";

        [TestMethod]
        [TestProperty("Description", "Verifies random property edits on a live sorted, filtered and grouped view, mixed with source changes, settle into the view a fresh view over the same items shows.")]
        public void VerifyLiveShapingMatchesFreshViewFuzz()
        {
            CollectGarbageAndWaitForIdle();
            foreach (var levels in new[] { 0, 1, 2, 3 })
            {
                EngineFixture fixture = null;
                RunOnUIThread.Execute(() => fixture = new EngineFixture(new Random(1337 + levels), levels, isLive: true));
                for (var round = 0; round < 40; round++)
                {
                    var ops = new List<string>();
                    RunOnUIThread.Execute(() =>
                    {
                        for (var i = 0; i < 6; i++)
                        {
                            ops.Add(fixture.ApplyRandomOperation(allowPropertyEdits: true));
                        }
                    });

                    // The live restore runs on a later dispatcher turn: wait for the view to settle into the expected
                    // state rather than for the whole app to be idle.
                    var gcBefore = GcCounts();
                    var stopwatch = global::System.Diagnostics.Stopwatch.StartNew();
                    var matches = false;
                    string expected = null;
                    string actual = null;
                    while (!matches && stopwatch.Elapsed < TimeSpan.FromSeconds(30))
                    {
                        RunOnUIThread.Execute(() => matches = fixture.Matches(out expected, out actual));
                        if (!matches)
                        {
                            Thread.Sleep(20);
                        }
                    }
                    Log.Comment($"live, levels {levels}, round {round}: {string.Join(", ", ops)} [{stopwatch.ElapsedMilliseconds} ms, {gcBefore} -> {GcCounts()}]");
                    if (!matches)
                    {
                        Verify.Fail($"live, levels {levels}, round {round}: the view differs from a fresh view.\n  expected {expected}\n  actual   {actual}");
                    }
                }
            }
        }

        #endregion

        #region Re-entrancy

        [TestMethod]
        [TestProperty("Description", "Verifies a source change made by a handler of the view's Reset is applied once the Reset unwinds.")]
        public void VerifySourceChangeInResetHandlerIsApplied()
        {
            RunOnUIThread.Execute(() =>
            {
                var rows = MakeEngineRows();
                var cvs = new CollectionViewSource { Source = rows };
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                var view = cvs.View;
                var added = false;
                view.VectorChanged += (sender, args) =>
                {
                    if (args.CollectionChange == CollectionChange.Reset && !added)
                    {
                        added = true;
                        rows.Add(new EngineRow(99, "Zed", -1, "X"));
                    }
                };

                ((ICollectionViewShaping)view).Refresh();

                Verify.IsTrue(added, "Precondition: the Reset handler ran.");
                Verify.AreEqual(rows.Count, view.Count, "The row added during the Reset is in the view.");
                Verify.AreEqual("Zed", ((EngineRow)view[0]).Name, "It is at its sorted place.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a SortDescriptions change and a Refresh made by a handler of the view's change are applied, not dropped.")]
        public void VerifyRuleChangeInChangeHandlerIsApplied()
        {
            RunOnUIThread.Execute(() =>
            {
                var rows = MakeEngineRows();
                var cvs = new CollectionViewSource { Source = rows };
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                var view = cvs.View;
                var shaping = (ICollectionViewShaping)view;
                var handled = 0;
                view.VectorChanged += (sender, args) =>
                {
                    if (handled++ == 0)
                    {
                        shaping.SortDescriptions[0] = new SortDescription("Score", CollectionViewSortDirection.Descending);
                    }
                };

                rows.Add(new EngineRow(50, "Mid", 25, "X"));

                Verify.IsGreaterThan(handled, 1, "The view changed again after the handler changed its sort.");
                Verify.AreEqual(string.Join(" ", rows.OrderByDescending(r => r.Score).ThenBy(r => rows.IndexOf(r)).Select(r => r.Name)), EngineNames(view),
                    "The rows follow the SortDescriptions the handler set.");

                var refreshed = false;
                NotifyOnce(view, () =>
                {
                    refreshed = true;
                    rows[0].Score = 1000;
                    shaping.Refresh();
                });
                rows.Add(new EngineRow(51, "Low", 0, "Y"));
                Verify.IsTrue(refreshed, "Precondition: the handler edited a row and refreshed.");
                Verify.AreEqual(rows[0].Name, ((EngineRow)view[0]).Name, "The Refresh the handler asked for read the edit.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a rule change made from inside a Filter callback is applied once the recompute that runs the filter finishes.")]
        public void VerifyRuleChangeInsideFilterIsApplied()
        {
            RunOnUIThread.Execute(() =>
            {
                var rows = MakeEngineRows();
                var cvs = new CollectionViewSource { Source = rows };
                var view = cvs.View;
                var shaping = (ICollectionViewShaping)view;
                var changed = false;
                shaping.Filter = new CollectionViewFilter(item =>
                {
                    if (!changed)
                    {
                        changed = true;
                        shaping.SortDescriptions.Add(new SortDescription("Name", CollectionViewSortDirection.Descending));
                    }
                    return true;
                });

                Verify.IsTrue(changed, "Precondition: the filter changed the sort.");
                Verify.AreEqual(string.Join(" ", rows.OrderByDescending(r => r.Name, StringComparer.CurrentCulture).Select(r => r.Name)), EngineNames(view),
                    "The sort set from the filter is applied.");
            });
        }

        #endregion

        #region Incremental loading, lifetime

        [TestMethod]
        [TestProperty("Description", "Verifies a shaped view forwards HasMoreItems and LoadMoreItemsAsync to an ISupportIncrementalLoading source, and the loaded items are placed by its rules.")]
        public void VerifyShapedViewForwardsIncrementalLoading()
        {
            RunOnUIThread.Execute(() =>
            {
                var rows = new IncrementalEngineRows(MakeEngineRows());
                var cvs = new CollectionViewSource { Source = rows };
                var view = cvs.View;
                Verify.IsTrue(view.HasMoreItems, "Precondition: the unshaped view forwards HasMoreItems.");

                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Descending));
                Verify.IsTrue(view.HasMoreItems, "The shaped view still forwards HasMoreItems.");

                view.LoadMoreItemsAsync(2);
                Verify.AreEqual(1, rows.LoadCount, "The shaped view forwarded LoadMoreItemsAsync.");
                Verify.AreEqual(rows.Count, view.Count, "The loaded items are in the view.");
                Verify.AreEqual(string.Join(" ", rows.OrderByDescending(r => r.Score).Select(r => r.Name)), EngineNames(view), "They are placed by the sort.");

                rows.CanLoad = false;
                Verify.IsFalse(view.HasMoreItems, "HasMoreItems follows the source.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the Filter event keeps filtering after a GC when the app keeps only the View, as WPF's view keeps the CollectionViewSource through the wrapped filter.")]
        public void VerifyFilterEventSurvivesGCWhenOnlyViewIsHeld()
        {
            ObservableCollection<EngineRow> rows = null;
            ICollectionView view = null;
            RunOnUIThread.Execute(() =>
            {
                rows = MakeEngineRows();
                view = CreateViewWithFilterEvent(rows);
                Verify.AreEqual(rows.Count(r => r.Score >= 20), view.Count, "Precondition: the event filters.");
            });

            TableViewLifetimeTestHelpers.CollectGarbage();

            RunOnUIThread.Execute(() =>
            {
                rows.Add(new EngineRow(70, "Low", 1, "X"));
                ((ICollectionViewShaping)view).Refresh();
                Verify.AreEqual(rows.Count(r => r.Score >= 20), view.Count, "The event still filters after a GC.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies live shaping does not keep items, their owner and the CollectionViewSource alive: its listeners are visible to the GC.")]
        public void VerifyLiveShapingDoesNotLeakItemOwner()
        {
            var tracked = new Dictionary<string, WeakReference>();
            RunOnUIThread.Execute(() => CreateLiveOwner(tracked));
            TableViewLifetimeTestHelpers.AssertCollected(tracked);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static ICollectionView CreateViewWithFilterEvent(ObservableCollection<EngineRow> rows)
        {
            var cvs = new CollectionViewSource { Source = rows };
            cvs.Filter += (sender, args) => args.Accepted = ((EngineRow)args.Item).Score >= 20;
            return cvs.View;
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void CreateLiveOwner(Dictionary<string, WeakReference> tracked)
        {
            var owner = new EngineOwner();
            owner.Rows = MakeEngineRows();
            foreach (var row in owner.Rows)
            {
                row.Owner = owner;
            }

            owner.Source = new CollectionViewSource { Source = owner.Rows, IsLiveSortingRequested = true };
            owner.Source.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
            owner.View = owner.Source.View;
            owner.Rows[0].Score = 500;
            tracked["Owner"] = new WeakReference(owner);
            tracked["Row"] = new WeakReference(owner.Rows[1]);
        }

        #endregion

        #region Groups, currency, keys

        [TestMethod]
        [TestProperty("Description", "Verifies a computed group raises PropertyChanged for ItemCount when an item joins or leaves it, as WPF's CollectionViewGroup does.")]
        public void VerifyGroupItemCountRaisesPropertyChanged()
        {
            RunOnUIThread.Execute(() =>
            {
                var rows = MakeEngineRows();
                var cvs = new CollectionViewSource { Source = rows };
                cvs.GroupDescriptions.Add(new PropertyGroupDescription("Team"));
                var group = ((ICollectionViewGroup)cvs.View.CollectionGroups[0]).Group;
                var raised = new List<string>();
                ((INotifyPropertyChanged)group).PropertyChanged += (sender, args) => raised.Add(args.PropertyName);
                var team = (string)GroupProperty(group, "Key");
                var before = (int)GroupProperty(group, "ItemCount");

                rows.Add(new EngineRow(60, "New", 5, team));
                Verify.AreEqual("ItemCount", string.Join(",", raised), "Adding an item raises ItemCount.");
                Verify.AreEqual(before + 1, (int)GroupProperty(group, "ItemCount"));

                raised.Clear();
                rows.Remove(rows.Last());
                Verify.AreEqual("ItemCount", string.Join(",", raised), "Removing an item raises ItemCount.");
                Verify.AreEqual(before, (int)GroupProperty(group, "ItemCount"));
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a refresh keeps currency before the first or after the last item, as WPF does, instead of moving it to the first item.")]
        public void VerifyRefreshKeepsCurrencyBeforeFirstAndAfterLast()
        {
            RunOnUIThread.Execute(() =>
            {
                var cvs = new CollectionViewSource { Source = MakeEngineRows() };
                var view = cvs.View;
                view.MoveCurrentToPosition(-1);
                Verify.IsTrue(view.IsCurrentBeforeFirst, "Precondition: before the first item.");

                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Descending));
                Verify.IsTrue(view.IsCurrentBeforeFirst, "A sort keeps it before the first item.");
                Verify.IsNull(view.CurrentItem);

                view.MoveCurrentToPosition(view.Count);
                Verify.IsTrue(view.IsCurrentAfterLast, "Precondition: after the last item.");
                ((ICollectionViewShaping)view).Refresh();
                Verify.IsTrue(view.IsCurrentAfterLast, "A refresh keeps it after the last item.");

                view.MoveCurrentToPosition(1);
                var current = view.CurrentItem;
                cvs.SortDescriptions[0] = new SortDescription("Score", CollectionViewSortDirection.Ascending);
                Verify.AreSame(current, view.CurrentItem, "A current item is followed.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a ListView synchronized with a CollectionViewSource view keeps no selection across a sort when nothing was selected, and a TableView header sort with nothing selected selects nothing.")]
        public void VerifySortWithNothingSelectedSelectsNothing()
        {
            ListView listView = null;
            CollectionViewSource cvs = null;
            RunOnUIThread.Execute(() =>
            {
                cvs = new CollectionViewSource { Source = MakeEngineRows() };
                listView = new ListView { ItemsSource = cvs.View, Width = 300, Height = 400 };
                LoadContent(listView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                listView.SelectedIndex = -1;
                Verify.IsTrue(cvs.View.IsCurrentBeforeFirst, "Precondition: deselecting moves currency before the first item.");
                cvs.SortDescriptions.Add(new SortDescription("Name", CollectionViewSortDirection.Ascending));
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(-1, listView.SelectedIndex, "The sort selected nothing.");
                Content = null;
            });

            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                tableView = CreateTreeTable(MakeLiveFlat());
                LoadContent(tableView);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Precondition: the column sorts."));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(tableView.SelectedItem, "A header sort with nothing selected selects nothing.");
                Verify.AreEqual(-1, tableView.SelectedIndex);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Int64 and UInt64 sort keys compare exactly, beyond the 53 bits a double holds.")]
        public void VerifyInt64SortKeysCompareExactly()
        {
            RunOnUIThread.Execute(() =>
            {
                const long baseValue = 1L << 53;
                var rows = new ObservableCollection<EngineRow>(new[] { 1, 0, 3, 2 }.Select(i => new EngineRow(i, $"r{i}", 0, "X")
                {
                    Big = baseValue + i,
                    UBig = ulong.MaxValue - (ulong)(3 - i),
                }));
                rows.Add(new EngineRow(9, "neg", 0, "X") { Big = long.MinValue, UBig = 0 });

                var cvs = new CollectionViewSource { Source = rows };
                cvs.SortDescriptions.Add(new SortDescription("Big", CollectionViewSortDirection.Ascending));
                Verify.AreEqual("neg r0 r1 r2 r3", EngineNames(cvs.View), "Int64 keys in exact order.");

                cvs.SortDescriptions[0] = new SortDescription("UBig", CollectionViewSortDirection.Descending);
                Verify.AreEqual("r3 r2 r1 r0 neg", EngineNames(cvs.View), "UInt64 keys in exact order.");

                rows.Add(new EngineRow(10, "r1b", 0, "X") { Big = baseValue + 1, UBig = ulong.MaxValue - 2 });
                Verify.AreEqual("r3 r2 r1 r1b r0 neg", EngineNames(cvs.View), "An incremental insert places it exactly, after its equal.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies hashed group lookup matches exactly the names NamesMatch matches: each StringComparison for strings, value equality for scalars (-0 and 0 are one group, NaNs are not), and a value's type.")]
        public void VerifyGroupNamesMatchUnderEachComparison()
        {
            RunOnUIThread.Execute(() =>
            {
                string Groups(CollectionViewStringComparison comparison, params object[] names)
                {
                    var rows = new ObservableCollection<EngineRow>(names.Select((name, i) => new EngineRow(i, $"r{i}", i, "X") { Owner = name }));
                    var cvs = new CollectionViewSource { Source = rows };
                    cvs.GroupDescriptions.Add(new PropertyGroupDescription("Owner", null, comparison));
                    var before = string.Join(" ", cvs.View.CollectionGroups.Select(g => GroupProperty(((ICollectionViewGroup)g).Group, "ItemCount")));
                    rows.Add(new EngineRow(99, "late", 99, "X") { Owner = names[0] });
                    return before + " | " + string.Join(" ", cvs.View.CollectionGroups.Select(g => GroupProperty(((ICollectionViewGroup)g).Group, "ItemCount")));
                }

                var composed = "\u00e1";
                var decomposed = "a\u0301";
                Verify.AreEqual("1 1 1 1 | 2 1 1 1", Groups(CollectionViewStringComparison.Ordinal, "x", "X", composed, decomposed));
                Verify.AreEqual("2 1 1 | 3 1 1", Groups(CollectionViewStringComparison.OrdinalIgnoreCase, "x", "X", composed, decomposed));
                Verify.AreEqual("1 1 2 | 2 1 2", Groups(CollectionViewStringComparison.InvariantCulture, "x", "X", composed, decomposed));
                Verify.AreEqual("2 2 | 3 2", Groups(CollectionViewStringComparison.CurrentCultureIgnoreCase, "x", "X", composed, decomposed));
                Verify.AreEqual("2 1 1 1 1 | 3 1 1 1 1", Groups(CollectionViewStringComparison.Ordinal, 0.0, -0.0, double.NaN, double.NaN, 1, 1L),
                    "-0 joins 0; each NaN is its own group; Int32 1 and Int64 1 differ.");
                Verify.AreEqual("2 1 | 3 1", Groups(CollectionViewStringComparison.Ordinal, null, null, "1"), "Null names share a group.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies keys of different types and NaN sort in one total order (null, Boolean, numbers with NaN first, strings, then values that cannot be ordered), so a full sort and incremental inserts agree.")]
        public void VerifyMixedTypeAndNaNSortKeysHaveATotalOrder()
        {
            RunOnUIThread.Execute(() =>
            {
                var values = new object[] { "b", 3, double.NaN, null, true, "a", 1.5, new object(), 2L, false };
                var rows = new ObservableCollection<EngineRow>(values.Select((value, i) => new EngineRow(i, $"r{i}", 0, "X") { Owner = value }));
                var cvs = new CollectionViewSource { Source = rows };
                cvs.SortDescriptions.Add(new SortDescription("Owner", CollectionViewSortDirection.Ascending));
                Verify.AreEqual("r3 r9 r4 r2 r6 r8 r1 r5 r0 r7", EngineNames(cvs.View),
                    "null, false, true, NaN, 1.5, 2, 3, \"a\", \"b\", then the object.");

                rows.Insert(0, new EngineRow(10, "r10", 0, "X") { Owner = double.NaN });
                rows.Add(new EngineRow(11, "r11", 0, "X") { Owner = "aa" });
                rows.Add(new EngineRow(12, "r12", 0, "X") { Owner = 0 });
                var fresh = new CollectionViewSource { Source = rows.ToList() };
                fresh.SortDescriptions.Add(new SortDescription("Owner", CollectionViewSortDirection.Ascending));
                Verify.AreEqual(EngineNames(fresh.View), EngineNames(cvs.View), "Incremental inserts land where a full sort puts them.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a C# GroupDescription subclass groups through its overrides, with and without NamesMatch, and that adds, removes and replaces leave the view equal to a fresh view.")]
        public void VerifyCSharpGroupDescriptionMatchesFreshView()
        {
            foreach (var ignoreCase in new[] { false, true })
            {
                RunOnUIThread.Execute(() =>
                {
                    Func<GroupDescription> create = () =>
                    {
                        var description = ignoreCase ? new IgnoreCaseTeamGroupDescription() : (GroupDescription)new TeamGroupDescription();
                        description.SortDescriptions.Add(new SortDescription("Name", CollectionViewSortDirection.Ascending));
                        return description;
                    };
                    var random = new Random(77);
                    var teams = new[] { "x", "X", "y", "Y", "z" };
                    var rows = new ObservableCollection<EngineRow>(Enumerable.Range(0, 20).Select(i => new EngineRow(i, $"r{i}", random.Next(10), teams[random.Next(teams.Length)])));
                    var cvs = new CollectionViewSource { Source = rows };
                    cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                    cvs.GroupDescriptions.Add(create());
                    Verify.IsGreaterThan(cvs.View.CollectionGroups.Count, 1, "Precondition: grouped by the subclass.");

                    for (var step = 0; step < 60; step++)
                    {
                        var row = new EngineRow(100 + step, $"n{step}", random.Next(10), teams[random.Next(teams.Length)]);
                        switch (random.Next(3))
                        {
                            case 0: rows.Insert(random.Next(rows.Count + 1), row); break;
                            case 1: rows.RemoveAt(random.Next(rows.Count)); break;
                            default: rows[random.Next(rows.Count)] = row; break;
                        }

                        var fresh = new CollectionViewSource { Source = rows.ToList() };
                        fresh.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                        fresh.GroupDescriptions.Add(create());
                        // An ignore-case group keeps the name its first item gave it; a fresh view may name it by another.
                        Verify.AreEqual(Describe(fresh.View).ToUpperInvariant(), Describe(cvs.View).ToUpperInvariant(), $"ignoreCase {ignoreCase}, step {step}");
                    }
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies GroupDescription.GroupNameFromItem and NamesMatch are public and call the overridables, so a view an app writes can group by any description, as in WPF.")]
        public void VerifyGroupDescriptionIsUsableByAppViews()
        {
            RunOnUIThread.Execute(() =>
            {
                var row = new EngineRow(1, "a", 3, "X");
                var property = new PropertyGroupDescription("Team");
                Verify.AreEqual("X", property.GroupNameFromItem(row, 0, string.Empty));
                Verify.IsTrue(property.NamesMatch("X", "X"));
                Verify.IsFalse(property.NamesMatch("X", "x"));
                Verify.IsTrue(new PropertyGroupDescription("Team", null, CollectionViewStringComparison.OrdinalIgnoreCase).NamesMatch("X", "x"));

                GroupDescription custom = new IgnoreCaseTeamGroupDescription();
                Verify.AreEqual("X", custom.GroupNameFromItem(row, 0, string.Empty), "The public method calls the subclass's GroupNameFromItemCore.");
                Verify.IsTrue(custom.NamesMatch("x", "X"), "The public method calls the subclass's NamesMatchCore.");
                Verify.IsTrue(new TeamGroupDescription().NamesMatch("X", "X"), "Without an override, NamesMatch is Equals.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a tick of live filtering only (no live sorting) leaves the items in place under a CustomSort whatever the tick's size, while the filter's changes apply.")]
        public void VerifyLiveFilterTickKeepsOrderWithoutLiveSorting()
        {
            foreach (var changed in new[] { 5, 1500 })
            {
                ObservableCollection<EngineRow> rows = null;
                ICollectionView view = null;
                string before = null;
                RunOnUIThread.Execute(() =>
                {
                    rows = new ObservableCollection<EngineRow>(Enumerable.Range(0, 3000).Select(i => new EngineRow(i, $"r{i}", i, "X")));
                    var cvs = new CollectionViewSource { Source = rows, IsLiveFilteringRequested = true };
                    cvs.LiveFilteringProperties.Add("Score");
                    view = cvs.View;
                    var shaping = (ICollectionViewShaping)view;
                    shaping.CustomSort = (left, right) => ((EngineRow)left).Score.CompareTo(((EngineRow)right).Score);
                    shaping.Filter = item => ((EngineRow)item).Score % 7 != 3;
                    before = EngineNames(view);
                    for (var i = 0; i < changed; i++)
                    {
                        rows[i * 2].Score = 10000 - rows[i * 2].Score * 7;
                    }
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var beforeOrder = before.Split(' ');
                    var now = new HashSet<string>(EngineNames(view).Split(' '));
                    var wasIn = new HashSet<string>(beforeOrder);
                    var kept = string.Join(" ", EngineNames(view).Split(' ').Where(wasIn.Contains));
                    Verify.AreEqual(string.Join(" ", beforeOrder.Where(now.Contains)), kept, $"{changed} changed: the items that stay keep their order.");
                    Verify.AreEqual(rows.Count(r => r.Score % 7 != 3), view.Count, $"{changed} changed: the filter applied.");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies integer and floating-point sort keys compare exactly (2^53 + 1 sorts after 2^53 as a double), so incremental and full sorts agree.")]
        public void VerifyIntegerAndDoubleKeysCompareExactly()
        {
            RunOnUIThread.Execute(() =>
            {
                var rows = new ObservableCollection<EngineRow>
                {
                    new EngineRow(1, "big", 0, "X") { Owner = 9007199254740993L },
                    new EngineRow(2, "double", 0, "X") { Owner = 9007199254740992.0 },
                    new EngineRow(3, "small", 0, "X") { Owner = 9007199254740991L },
                };
                var cvs = new CollectionViewSource { Source = rows };
                cvs.SortDescriptions.Add(new SortDescription("Owner", CollectionViewSortDirection.Ascending));
                Verify.AreEqual("small double big", EngineNames(cvs.View));
                rows.Insert(0, new EngineRow(5, "huge", 0, "X") { Owner = ulong.MaxValue });
                rows.Insert(0, new EngineRow(6, "neg", 0, "X") { Owner = -9007199254740993L });
                rows.Insert(0, new EngineRow(7, "negd", 0, "X") { Owner = -9007199254740992.0 });
                var fresh = new CollectionViewSource { Source = rows.ToList() };
                fresh.SortDescriptions.Add(new SortDescription("Owner", CollectionViewSortDirection.Ascending));
                Verify.AreEqual(EngineNames(fresh.View), EngineNames(cvs.View), "Incremental inserts equal a fresh sort.");
                Verify.AreEqual("neg negd small double big huge", EngineNames(cvs.View));
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a shaped view whose filter rejects every loaded item stops reporting HasMoreItems after a few loads instead of draining the source, and reports it again once the view grows.")]
        public void VerifyFilteredIncrementalLoadingStops()
        {
            RunOnUIThread.Execute(() =>
            {
                var rows = new IncrementalEngineRows(MakeEngineRows());
                var cvs = new CollectionViewSource { Source = rows };
                var view = cvs.View;
                var shaping = (ICollectionViewShaping)view;
                shaping.Filter = item => ((EngineRow)item).Score < 100;
                var visible = view.Count;

                var loads = 0;
                while (view.HasMoreItems && loads < 50)
                {
                    view.LoadMoreItemsAsync(5);
                    loads++;
                }

                Log.Comment($"Loads until HasMoreItems turned false: {loads}");
                Verify.IsLessThanOrEqual(loads, 11, "The loads stop.");
                Verify.AreEqual(visible, view.Count, "Precondition: the filter rejected every loaded item.");
                Verify.IsTrue(rows.HasMoreItems, "The source still has more.");

                shaping.Filter = item => ((EngineRow)item).Score < 99;
                Verify.IsTrue(view.HasMoreItems, "A new rule recomputes the view and loading starts over.");

                shaping.Filter = null;
                Verify.IsGreaterThan(view.Count, visible);
                Verify.IsTrue(view.HasMoreItems, "Once the view grows, HasMoreItems follows the source again.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies sort and group descriptions, GroupNames and StringComparison declared in compiled XAML (XBF) load and shape the view.")]
        public void VerifyShapingDeclaredInCompiledXaml()
        {
            RunOnUIThread.Execute(() =>
            {
                var page = new MUXControlsTestApp.CollectionViewShapingMarkupPage();
                var cvs = (CollectionViewSource)page.Resources["People"];
                Verify.AreEqual(1, cvs.SortDescriptions.Count);
                Verify.AreEqual("Name", cvs.SortDescriptions[0].PropertyName);
                Verify.AreEqual(CollectionViewSortDirection.Descending, cvs.SortDescriptions[0].Direction);
                var group = (PropertyGroupDescription)cvs.GroupDescriptions[0];
                Verify.AreEqual(CollectionViewStringComparison.OrdinalIgnoreCase, group.StringComparison);
                Verify.AreEqual("Z X", string.Join(" ", group.GroupNames));

                cvs.Source = MakeEngineRows();
                Verify.AreEqual("Z X Y", string.Join(" ", cvs.View.CollectionGroups.Select(g => (string)GroupProperty(((ICollectionViewGroup)g).Group, "Key"))));
                Verify.AreEqual("Bo Lin Kai Eve Ada", EngineNames(cvs.View));
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a filter that rejects the loaded items stops incremental loading through a shaped view also when the source reports each load as one Reset (an AddRange-style collection).")]
        public void VerifyFilteredResetStyleIncrementalLoadingStops()
        {
            RunOnUIThread.Execute(() =>
            {
                var rows = new ResetIncrementalEngineRows(MakeEngineRows());
                var cvs = new CollectionViewSource { Source = rows };
                var view = cvs.View;
                ((ICollectionViewShaping)view).Filter = item => ((EngineRow)item).Score < 100;
                var visible = view.Count;

                var loads = 0;
                while (view.HasMoreItems && loads < 50)
                {
                    view.LoadMoreItemsAsync(5);
                    loads++;
                }

                Log.Comment($"Loads until HasMoreItems turned false: {loads}");
                Verify.IsLessThanOrEqual(loads, 11, "The loads stop.");
                Verify.AreEqual(visible, view.Count);

                ((ICollectionViewShaping)view).Refresh();
                Verify.IsTrue(view.HasMoreItems, "An app Refresh starts loading over.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies index writes through a view over a source that raises no changes (a List) fail without changing the source once the view was shaped, even with its rules removed: the view could not follow the write.")]
        public void VerifyWritesThroughViewOverNonObservableSourceFailSafely()
        {
            RunOnUIThread.Execute(() =>
            {
                var rows = MakeEngineRows().ToList();
                var cvs = new CollectionViewSource { Source = rows };
                var view = cvs.View;
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                cvs.SortDescriptions.Clear();
                var before = string.Join(" ", rows.Select(row => row.Name));

                var threw = false;
                try
                {
                    view.RemoveAt(0);
                }
                catch (Exception)
                {
                    threw = true;
                }
                Verify.IsTrue(threw, "The write fails.");
                Verify.AreEqual(before, string.Join(" ", rows.Select(row => row.Name)), "The source is unchanged.");
                Verify.AreEqual(rows.Count, view.Count);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a sparse filter (one match every few loads) keeps loading through a shaped view: only loads in a row that bring the view nothing stop it.")]
        public void VerifySparseFilteredIncrementalLoadingContinues()
        {
            RunOnUIThread.Execute(() =>
            {
                var rows = new IncrementalEngineRows(MakeEngineRows());
                var cvs = new CollectionViewSource { Source = rows };
                var view = cvs.View;
                var shaping = (ICollectionViewShaping)view;
                shaping.Filter = item => ((EngineRow)item).Score < 100 || ((EngineRow)item).Id % 35 == 0;
                var visible = view.Count;

                for (var loads = 0; loads < 40; loads++)
                {
                    Verify.IsTrue(view.HasMoreItems, $"Still loading after {loads} loads.");
                    view.LoadMoreItemsAsync(5);
                }
                Verify.IsGreaterThan(view.Count, visible, "The sparse matches were loaded.");
            });
        }

        #endregion

        #region Lifetime of app objects, exceptions in app code

        [TestMethod]
        [TestProperty("Description", "Verifies a CustomSort, a group CustomSort, a group Converter and a GroupDescription subclass that capture their owner (a page) do not keep it alive: the view holds them through references the GC can see.")]
        public void VerifyShapingDelegatesDoNotLeakTheirOwner()
        {
            var tracked = new Dictionary<string, WeakReference>();
            RunOnUIThread.Execute(() => CreateShapingDelegateOwner(tracked));
            TableViewLifetimeTestHelpers.AssertCollected(tracked);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void CreateShapingDelegateOwner(Dictionary<string, WeakReference> tracked)
        {
            var owner = new EngineOwner();
            owner.Rows = MakeEngineRows();
            owner.Source = new CollectionViewSource { Source = owner.Rows };
            ((ICollectionViewShaping)owner.Source.View).CustomSort = (left, right) => owner.CompareScores(left, right);
            var team = new PropertyGroupDescription("Team", new OwnerConverter { Owner = owner });
            team.CustomSort = (left, right) => owner.CompareNames(left, right);
            owner.Source.GroupDescriptions.Add(team);
            owner.Source.GroupDescriptions.Add(new OwnerGroupDescription { Owner = owner });
            owner.View = owner.Source.View;
            Verify.AreEqual(3, owner.View.CollectionGroups.Count, "Precondition: grouped by the converted Team.");
            owner.Rows.Add(new EngineRow(60, "New", 25, "Y"));
            tracked["Owner"] = new WeakReference(owner);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies app code that throws while the view applies a source change (Filter, CustomSort, a group Converter, GroupDescription overrides, the Filter event, a handler of a change the view or a group raises) does not reach the source's other listeners, is reported to Application.UnhandledException with its message, and leaves the view equal to a fresh view.")]
        public void VerifyThrowingAppCodeLeavesViewConsistent()
        {
            foreach (var callback in new[] { "Filter", "CustomSort", "Converter", "GroupNameFromItem", "NamesMatch", "FilterEvent", "ViewChanged", "GroupItemCount" })
            {
                foreach (var operation in new[] { "insert", "replace", "remove", "move" })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        var context = $"{callback} on {operation}";
                        var thrower = new ThrowOnce();
                        var rows = new ObservableCollection<EngineRow>(Enumerable.Range(0, 12).Select(i => new EngineRow(i, $"r{i}", (i * 7) % 10, (i % 3 == 0) ? "X" : "Y")));
                        var cvs = new CollectionViewSource { Source = rows };
                        ApplyThrowingRules(cvs, callback, thrower);
                        var view = cvs.View;
                        var heard = 0;
                        rows.CollectionChanged += (sender, args) => heard++;

                        string thrown;
                        List<string> reported;
                        using (var unhandled = new UnhandledErrors())
                        {
                            thrower.IsArmed = true;
                            thrown = ThrownTypeName(() =>
                            {
                                switch (operation)
                                {
                                    case "insert": rows.Insert(2, new EngineRow(50, "New", 5, "X")); break;
                                    case "replace": rows[1] = new EngineRow(51, "Swap", 4, "X"); break;
                                    case "remove": rows.RemoveAt(4); break;
                                    default: rows.Move(0, 7); break;
                                }
                            });
                            reported = unhandled.Messages;
                        }
                        var threw = thrower.Count > 0;
                        thrower.IsArmed = false;

                        Log.Comment($"{context}: the callback {(threw ? "threw" : "did not run")}; reported: {string.Join(" | ", reported)}");
                        Verify.AreEqual("no exception", thrown, $"{context}: the failure does not reach the code that changed the source.");
                        Verify.AreEqual(1, heard, $"{context}: the source's other listeners heard the change.");
                        Verify.AreEqual(thrower.Count, reported.Count, $"{context}: each failure is reported to Application.UnhandledException.");
                        if (reported.Count > 0)
                        {
                            Verify.IsTrue(reported[0].Contains("Thrown by the test."), $"{context}: the report carries the app's message.");
                        }
                        var fresh = new CollectionViewSource { Source = rows.ToList() };
                        ApplyThrowingRules(fresh, callback, new ThrowOnce());
                        Verify.AreEqual(Describe(fresh.View), Describe(view), $"{context}: the view equals a fresh view.");
                        if (operation == "insert" || operation == "replace")
                        {
                            Verify.IsTrue(threw, $"{context}: precondition: the callback threw.");
                        }
                    });
                }
            }
        }

        // Application.UnhandledException while it is attached (handled, so the app keeps running).
        private sealed class UnhandledErrors : IDisposable
        {
            public UnhandledErrors()
            {
                Microsoft.UI.Xaml.Application.Current.UnhandledException += OnUnhandledException;
            }

            public List<string> Messages { get; } = new List<string>();

            public void Dispose()
            {
                Microsoft.UI.Xaml.Application.Current.UnhandledException -= OnUnhandledException;
            }

            private void OnUnhandledException(object sender, Microsoft.UI.Xaml.UnhandledExceptionEventArgs e)
            {
                Messages.Add(e.Message ?? string.Empty);
                e.Handled = true;
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a CustomSort that throws during a live restore leaves the view equal to a fresh view, and that a Refresh whose Filter throws throws to its caller and keeps the previous items.")]
        public void VerifyThrowingAppCodeInLiveRestoreAndRefresh()
        {
            var thrower = new ThrowOnce();
            ObservableCollection<EngineRow> rows = null;
            CollectionViewSource cvs = null;
            UnhandledErrors unhandled = null;
            RunOnUIThread.Execute(() =>
            {
                rows = MakeEngineRows();
                cvs = new CollectionViewSource { Source = rows, IsLiveSortingRequested = true };
                cvs.LiveSortingProperties.Add("Score");
                ApplyThrowingRules(cvs, "CustomSort", thrower);
                Verify.IsNotNull(cvs.View);
                unhandled = new UnhandledErrors();
                thrower.IsArmed = true;
                rows[0].Score = 1;
                rows[3].Score = 2;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                unhandled.Dispose();
                Verify.AreEqual(1, thrower.Count, "Precondition: the comparer threw in the restore.");
                Verify.AreEqual(1, unhandled.Messages.Count, "The failed restore is reported to Application.UnhandledException.");
                var fresh = new CollectionViewSource { Source = rows.ToList() };
                ApplyThrowingRules(fresh, "CustomSort", new ThrowOnce());
                Verify.AreEqual(Describe(fresh.View), Describe(cvs.View), "The view equals a fresh view after the failed restore.");

                var shaping = (ICollectionViewShaping)cvs.View;
                var before = Describe(cvs.View);
                shaping.Filter = item =>
                {
                    thrower.Check();
                    return true;
                };
                thrower.IsArmed = true;
                Verify.AreNotEqual("no exception", ThrownTypeName(() => shaping.Refresh()), "Refresh throws the filter's exception to its caller.");
                Verify.AreEqual(before, Describe(cvs.View), "The view keeps its items.");
                rows.Add(new EngineRow(70, "Late", 0, "X"));
                fresh = new CollectionViewSource { Source = rows.ToList() };
                ApplyThrowingRules(fresh, "CustomSort", new ThrowOnce());
                Verify.AreEqual(Describe(fresh.View), Describe(cvs.View), "The next change recomputes the view.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a handler that refreshes the view on each of its changes ends after a bounded number of recomputes, without an exception reaching the source.")]
        public void VerifyEndlessRefreshFromHandlerIsBounded()
        {
            RunOnUIThread.Execute(() =>
            {
                var rows = MakeEngineRows();
                var cvs = new CollectionViewSource { Source = rows };
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                var view = cvs.View;
                var resets = 0;
                view.VectorChanged += (sender, args) =>
                {
                    if (args.CollectionChange == CollectionChange.Reset)
                    {
                        resets++;
                    }
                    ((ICollectionViewShaping)view).Refresh();
                };

                using (var unhandled = new UnhandledErrors())
                {
                    Verify.AreEqual("no exception", ThrownTypeName(() => rows.Add(new EngineRow(60, "New", 25, "X"))));
                    Log.Comment($"Resets: {resets}; reported: {string.Join(" | ", unhandled.Messages)}");
                    Verify.AreEqual(1, unhandled.Messages.Count, "The endless recompute is reported to Application.UnhandledException.");
                }
                Verify.IsLessThanOrEqual(resets, 10, "The recomputes stop.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a view's Filter stays with that view when the CollectionViewSource replaces it, as in WPF, so a later IsSourceGrouped does not throw.")]
        public void VerifyViewFilterStaysWithItsView()
        {
            RunOnUIThread.Execute(() =>
            {
                var cvs = new CollectionViewSource { Source = MakeEngineRows() };
                var first = (ICollectionViewShaping)cvs.View;
                first.Filter = item => ((EngineRow)item).Score >= 20;
                cvs.Source = MakeEngineRows();
                Verify.IsNotNull(first.Filter, "The replaced view keeps its Filter.");
                Verify.IsNull(((ICollectionViewShaping)cvs.View).Filter, "The new view does not take it.");
                Verify.AreEqual(5, cvs.View.Count);

                var grouped = new CollectionViewSource { Source = new List<List<object>> { new List<object> { "a", "b" } } };
                ((ICollectionViewShaping)grouped.View).Filter = item => true;
                Verify.AreEqual("no exception", ThrownTypeName(() => grouped.IsSourceGrouped = true), "IsSourceGrouped after a view Filter does not throw.");
                Verify.AreEqual(1, grouped.View.CollectionGroups.Count);
            });
        }

        private static void ApplyThrowingRules(CollectionViewSource cvs, string callback, ThrowOnce thrower)
        {
            var shaping = (ICollectionViewShaping)cvs.View;
            switch (callback)
            {
                case "Filter":
                    shaping.Filter = item =>
                    {
                        thrower.Check();
                        return ((EngineRow)item).Score != 9;
                    };
                    cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                    break;
                case "CustomSort":
                    shaping.CustomSort = (left, right) =>
                    {
                        thrower.Check();
                        return ((EngineRow)left).Score.CompareTo(((EngineRow)right).Score);
                    };
                    break;
                case "Converter":
                    cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                    cvs.GroupDescriptions.Add(new PropertyGroupDescription("Team", new ThrowingConverter { Thrower = thrower }));
                    break;
                case "GroupNameFromItem":
                case "NamesMatch":
                    cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                    cvs.GroupDescriptions.Add(new ThrowingGroupDescription { Thrower = thrower, ThrowsInNamesMatch = callback == "NamesMatch" });
                    break;
                case "FilterEvent":
                    cvs.Filter += (sender, args) =>
                    {
                        thrower.Check();
                        args.Accepted = ((EngineRow)args.Item).Score != 9;
                    };
                    cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                    break;
                case "ViewChanged":
                    cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                    cvs.View.VectorChanged += (sender, args) => thrower.Check();
                    break;
                case "GroupItemCount":
                    cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                    cvs.GroupDescriptions.Add(new PropertyGroupDescription("Team"));
                    foreach (ICollectionViewGroup group in cvs.View.CollectionGroups)
                    {
                        ((INotifyPropertyChanged)group.Group).PropertyChanged += (sender, args) => thrower.Check();
                    }
                    break;
            }
        }

        #endregion

        #region Live restore cost

        [TestMethod]
        [TestProperty("Description", "Verifies a live tick publishes moves while they are cheap and keep the realized rows: nine of ten rows changing move, a large tick whose keys changed but kept their order raises nothing, a few moves among many changed keys raise only those moves, and a large shuffle raises one Reset.")]
        public void VerifyMassLiveSortChangeIsOneReset()
        {
            ObservableCollection<EngineRow> rows = null;
            ICollectionView view = null;
            var changes = new List<CollectionChange>();
            string Summary() => $"Reset={changes.Count(c => c == CollectionChange.Reset)} Removed={changes.Count(c => c == CollectionChange.ItemRemoved)} Inserted={changes.Count(c => c == CollectionChange.ItemInserted)}";
            void Place(int count)
            {
                RunOnUIThread.Execute(() =>
                {
                    rows = new ObservableCollection<EngineRow>(Enumerable.Range(0, count).Select(i => new EngineRow(i, $"r{i}", i, "X")));
                    var cvs = new CollectionViewSource { Source = rows, IsLiveSortingRequested = true };
                    cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                    view = cvs.View;
                    changes.Clear();
                    view.VectorChanged += (sender, args) => changes.Add(args.CollectionChange);
                });
            }
            void Tick(Action edit, string expected, string context)
            {
                RunOnUIThread.Execute(() =>
                {
                    changes.Clear();
                    edit();
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(expected, Summary(), context);
                    Verify.AreEqual(string.Join(" ", rows.Select((r, i) => (r, i)).OrderBy(p => p.r.Score).ThenBy(p => p.i).Select(p => p.r.Name)), EngineNames(view), $"{context}: sorted.");
                });
            }

            Place(10);
            Tick(() =>
            {
                foreach (var row in rows.Take(9))
                {
                    row.Score = 100 - row.Score;
                }
            }, "Reset=0 Removed=9 Inserted=9", "Nine of ten small rows changed: moves, no Reset.");

            Place(3000);
            Tick(() =>
            {
                foreach (var row in rows)
                {
                    row.Score += 1000;
                }
            }, "Reset=0 Removed=0 Inserted=0", "Every key changed but the order did not: nothing is raised.");
            Tick(() =>
            {
                foreach (var row in rows)
                {
                    row.Score += 1000;
                }
                foreach (var index in new[] { 3, 500, 1200, 2000, 2999 })
                {
                    rows[index].Score = -index;
                }
            }, "Reset=0 Removed=5 Inserted=5", "Five rows moved among 3000 changed keys: five moves.");
            var random = new Random(5);
            Tick(() =>
            {
                foreach (var row in rows)
                {
                    row.Score = random.Next(1_000_000);
                }
            }, "Reset=1 Removed=0 Inserted=0", "A shuffle of 3000 rows: one Reset.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a live tick on a large TableView that changes every key but moves few rows reaches the rows as moves, not a Reset, and keeps the selection.")]
        public void VerifyLargeLiveTickKeepsTableViewRows()
        {
            var list = new ObservableCollection<LiveRow>(Enumerable.Range(1, 2000).Select(i => new LiveRow { Id = i, Name = $"R{i:0000}", Score = i, Team = "X" }));
            CollectionViewSource cvs = null;
            var tableView = PlaceLive(list, s => s.IsLiveSortingRequested = true, out cvs);
            RunOnUIThread.Execute(() => tableView.Select(2));
            SettleLayout(tableView);

            var actions = new List<string>();
            RunOnUIThread.Execute(() =>
            {
                GetRowsRepeater(tableView).ItemsSourceView.CollectionChanged += (sender, args) => actions.Add(args.Action.ToString());
                foreach (var row in list)
                {
                    row.Score += 10_000;
                }
                list[2].Score = 50_000;
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(actions.Contains("Reset"), $"No Reset ({string.Join(",", actions)}).");
                Verify.AreEqual("R0003", (tableView.SelectedItem as LiveRow)?.Name ?? "(none)", "Selection follows the moved row.");
                Verify.AreEqual(list.Count - 1, tableView.SelectedIndex, "It moved to the end.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies keyboard focus stays on a row's item when live sorting moves the row (Remove + Insert) and when a large tick re-sorts the rows with one Reset.")]
        public void VerifyLiveMoveKeepsKeyboardFocusOnTheItem()
        {
            foreach (var count in new[] { 30, 3000 })
            {
                var list = new ObservableCollection<LiveRow>(Enumerable.Range(1, count).Select(i => new LiveRow { Id = i, Name = $"R{i:0000}", Score = i, Team = "X" }));
                var tableView = PlaceLive(list, s => s.IsLiveSortingRequested = true, out _);
                LiveRow focusedItem = null;
                RunOnUIThread.Execute(() =>
                {
                    focusedItem = list[2];
                    Verify.IsTrue(GetRowForItem(tableView, focusedItem).Focus(FocusState.Keyboard), "Precondition: the row takes focus.");
                });
                SettleLayout(tableView);

                var actions = new List<string>();
                RunOnUIThread.Execute(() =>
                {
                    GetRowsRepeater(tableView).ItemsSourceView.CollectionChanged += (sender, args) => actions.Add(args.Action.ToString());
                    if (count == 30)
                    {
                        focusedItem.Score = 25;
                    }
                    else
                    {
                        foreach (var row in list)
                        {
                            row.Score = (row.Id * 7919) % count;
                        }
                        focusedItem.Score = 5;
                    }
                });
                SettleLayout(tableView);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"{count} rows: {string.Join(",", actions.Distinct())}");
                    Verify.AreEqual(count == 3000, actions.Contains("Reset"), "Precondition: a move for a small tick, one Reset for a large one.");
                    DependencyObject node = Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(tableView.XamlRoot) as DependencyObject;
                    while (node != null && !(node is TableViewRow))
                    {
                        node = Microsoft.UI.Xaml.Media.VisualTreeHelper.GetParent(node);
                    }
                    Verify.IsNotNull(node, $"{count} rows: focus is on a row.");
                    Verify.AreSame(focusedItem, ((TableViewRow)node).DataContext, $"{count} rows: focus followed the item to its new row.");
                });
            }
        }

        private static object FocusedRowItem(TableView tableView)
        {
            var node = Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(tableView.XamlRoot) as DependencyObject;
            while (node != null && !(node is TableViewRow))
            {
                node = Microsoft.UI.Xaml.Media.VisualTreeHelper.GetParent(node);
            }
            return (node as TableViewRow)?.DataContext;
        }

        [TestMethod]
        [TestProperty("Description", "Verifies focus follows its item when rows moved above it first shifted its row: the focused row is found where it is, not where focus last landed.")]
        public void VerifyFocusFollowsItemAfterRowsMovedAboveIt()
        {
            var list = new ObservableCollection<LiveRow>(Enumerable.Range(1, 30).Select(i => new LiveRow { Id = i, Name = $"R{i:0000}", Score = i, Team = "X" }));
            var tableView = PlaceLive(list, s => s.IsLiveSortingRequested = true, out _);
            var focusedItem = list[5];
            RunOnUIThread.Execute(() => Verify.IsTrue(GetRowForItem(tableView, focusedItem).Focus(FocusState.Keyboard), "Precondition: the row takes focus."));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => list[20].Score = 0);
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreSame(focusedItem, FocusedRowItem(tableView), "Precondition: a move above the row keeps focus on it."));

            RunOnUIThread.Execute(() => focusedItem.Score = 25);
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreSame(focusedItem, FocusedRowItem(tableView), "Focus followed the item to its new row."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing another row at the index the focused row had before an insert above it neither moves focus nor scrolls back to it.")]
        public void VerifyUnrelatedRemoveDoesNotScrollBackToFocus()
        {
            var list = new ObservableCollection<LiveRow>(Enumerable.Range(1, 300).Select(i => new LiveRow { Id = i, Name = $"R{i:0000}", Score = i, Team = "X" }));
            var tableView = PlaceLive(list, s => { }, out _);
            var focusedItem = list[10];
            RunOnUIThread.Execute(() => Verify.IsTrue(GetRowForItem(tableView, focusedItem).Focus(FocusState.Keyboard), "Precondition: the row takes focus."));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => list.Insert(0, new LiveRow { Id = 1000, Name = "R1000", Score = 0, Team = "X" }));
            SettleLayout(tableView);
            ScrollBodyTo(tableView, ScrollAxis.Vertical, 3000.0);

            double offset = 0;
            RunOnUIThread.Execute(() =>
            {
                offset = GetBodyScroller(tableView).VerticalOffset;
                list.Remove(list.Single(row => row.Score == 10));
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                // A row removed above the viewport shifts the content by its height; a scroll back to the focused row would
                // move the viewport thousands of pixels.
                var after = GetBodyScroller(tableView).VerticalOffset;
                Verify.IsTrue(Math.Abs(offset - after) <= 100.0, $"The viewport stays where the user scrolled it ({offset} -> {after}).");
                Verify.AreSame(focusedItem, FocusedRowItem(tableView), "Focus stays on the original item.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an app that moves focus to another row right after the view raises a Reset keeps that focus: the focused item is not followed over it.")]
        public void VerifyAppFocusAfterResetIsKept()
        {
            var list = new ObservableCollection<LiveRow>(Enumerable.Range(1, 30).Select(i => new LiveRow { Id = i, Name = $"R{i:0000}", Score = i, Team = "X" }));
            var tableView = PlaceLive(list, s => { }, out var cvs);
            RunOnUIThread.Execute(() => Verify.IsTrue(GetRowForItem(tableView, list[3]).Focus(FocusState.Keyboard), "Precondition: the row takes focus."));
            SettleLayout(tableView);

            LiveRow first = null;
            RunOnUIThread.Execute(() =>
            {
                cvs.SortDescriptions.Clear();
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Descending));
                tableView.UpdateLayout();
                first = (LiveRow)cvs.View[0];
                var row = GetRowForItem(tableView, first) ?? (TableViewRow)GetRowsRepeater(tableView).GetOrCreateElement(0);
                tableView.UpdateLayout();
                Verify.IsTrue(GetRowCell(row, 0).Focus(FocusState.Keyboard) || row.Focus(FocusState.Keyboard), "Precondition: the app focuses row 0.");
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => Verify.AreSame(first, FocusedRowItem(tableView), "The app's focus stays."));
        }

        [TestMethod]
        [TestProperty("Description", "Records where keyboard focus goes in a ListView when live sorting moves the focused item: ListView keeps focus inside the list (on the container that takes the removed one's place), it does not follow the item.")]
        public void VerifyListViewLiveMoveKeepsFocusInList()
        {
            ListView listView = null;
            ObservableCollection<LiveRow> list = null;
            RunOnUIThread.Execute(() =>
            {
                list = new ObservableCollection<LiveRow>(Enumerable.Range(1, 30).Select(i => new LiveRow { Id = i, Name = $"R{i:0000}", Score = i }));
                var cvs = new CollectionViewSource { Source = list, IsLiveSortingRequested = true };
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                listView = new ListView { ItemsSource = cvs.View, DisplayMemberPath = "Name", Width = 300, Height = 400 };
                LoadContent(listView);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.IsTrue(((Control)listView.ContainerFromIndex(2)).Focus(FocusState.Keyboard), "Precondition: the item takes focus."));
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => list[2].Score = 25);
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var focused = Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(listView.XamlRoot) as DependencyObject;
                var container = focused as ListViewItem;
                Log.Comment($"Focus after the move: {(container == null ? focused?.GetType().Name ?? "(none)" : ((LiveRow)listView.ItemFromContainer(container)).Name)}; the moved item is at {listView.Items.IndexOf(list[2])}.");
                Verify.IsNotNull(focused, "Focus stays in the window.");
                Content = null;
            });
        }

        #endregion

        #region Live shaping on/off, stale views, unsupported rules, threads

        [TestMethod]
        [TestProperty("Description", "Verifies turning live sorting on catches up an edit made while it was off and raises PropertyChanged(IsLiveSorting), and raises no Reset when nothing was stale.")]
        public void VerifyTurningLiveOnCatchesUpAndNotifies()
        {
            RunOnUIThread.Execute(() =>
            {
                var rows = MakeEngineRows();
                var cvs = new CollectionViewSource { Source = rows };
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                var view = cvs.View;
                var changes = new List<CollectionChange>();
                var properties = new List<string>();
                view.VectorChanged += (sender, args) => changes.Add(args.CollectionChange);
                ((INotifyPropertyChanged)view).PropertyChanged += (sender, args) => properties.Add(args.PropertyName);

                cvs.IsLiveSortingRequested = true;
                Verify.AreEqual(0, changes.Count, "Nothing was stale, so turning live sorting on changes nothing.");
                Verify.IsTrue(properties.Contains("IsLiveSorting"), "PropertyChanged(IsLiveSorting) is raised.");

                cvs.IsLiveSortingRequested = false;
                var stale = EngineNames(view);
                rows.First().Score = 1000;
                Verify.AreEqual(stale, EngineNames(view), "Precondition: with live sorting off the edit does not move the row.");

                properties.Clear();
                ((ICollectionViewLiveShaping)view).IsLiveSorting = true;
                Verify.AreNotEqual(stale, EngineNames(view), "Turning live sorting on re-sorts the edit made while it was off.");
                Verify.AreEqual(string.Join(" ", rows.OrderBy(r => r.Score).Select(r => r.Name)), EngineNames(view));
                Verify.AreEqual("IsLiveSorting", string.Join(",", properties.Where(p => p.StartsWith("IsLive"))));
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a View the CollectionViewSource replaced keeps its own rules and its source: changing them does not reach the new View.")]
        public void VerifyReplacedViewDoesNotForwardToNewView()
        {
            RunOnUIThread.Execute(() =>
            {
                var first = MakeEngineRows();
                var cvs = new CollectionViewSource { Source = first };
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Descending));
                var oldView = cvs.View;

                cvs.Source = MakeEngineRows();
                var newView = cvs.View;
                Verify.AreNotSame(oldView, newView, "Precondition: a new Source gives a new View.");

                var oldShaping = (ICollectionViewShaping)oldView;
                Verify.AreEqual(1, oldShaping.SortDescriptions.Count, "The old View keeps its rules.");
                oldShaping.SortDescriptions.Clear();
                Verify.AreEqual(1, cvs.SortDescriptions.Count, "Clearing them does not reach the CollectionViewSource.");
                Verify.AreEqual(1, ((ICollectionViewShaping)newView).SortDescriptions.Count, "Nor the new View.");
                Verify.AreEqual(string.Join(" ", first.Select(r => r.Name)), EngineNames(oldView), "The old View shows its source unsorted.");

                first.Add(new EngineRow(80, "Late", 3, "Y"));
                Verify.AreEqual(first.Count, oldView.Count, "The old View still follows its source.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a rule on a CollectionViewSource whose View cannot shape (IsSourceGrouped) throws, as WPF's CannotSortView does, instead of being ignored.")]
        public void VerifyRuleOnSourceGroupedViewThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                var cvs = new CollectionViewSource
                {
                    IsSourceGrouped = true,
                    Source = new List<List<object>> { new List<object> { "a", "b" } },
                };

                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => cvs.SortDescriptions.Add(new SortDescription("Length", CollectionViewSortDirection.Ascending))),
                    "A sort the view cannot apply throws.");
                cvs.SortDescriptions.Clear();
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => cvs.Filter += (sender, args) => { }),
                    "A filter the view cannot apply throws.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a live property change raised off the UI thread is an error, as it is for {Binding}, not a change the view silently misses.")]
        public void VerifyOffThreadLiveChangeThrows()
        {
            ObservableCollection<EngineRow> rows = null;
            ICollectionView view = null;
            RunOnUIThread.Execute(() =>
            {
                rows = MakeEngineRows();
                var cvs = new CollectionViewSource { Source = rows, IsLiveSortingRequested = true };
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                view = cvs.View;
            });

            Exception thrown = null;
            Task.Run(() =>
            {
                try
                {
                    rows[0].Score = 5000;
                }
                catch (Exception ex)
                {
                    thrown = ex;
                }
            }).Wait();

            Verify.IsNotNull(thrown, "A PropertyChanged off the view's thread throws.");
            Log.Comment($"Off-thread change: {thrown?.GetType().Name} 0x{thrown?.HResult:X8}");

            RunOnUIThread.Execute(() => rows[1].Score = 6000);
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.AreEqual(rows[1].Name, ((EngineRow)view[view.Count - 1]).Name, "Live sorting still works on the UI thread."));
        }

        #endregion

        [TestMethod]
        [TestProperty("Description", "Verifies a source insert made while a live sort edit waits for its restore is applied as an insert, not a Reset, and the restore then settles the order a fresh view shows.")]
        public void VerifySourceChangeWhileLiveSortPendingIsIncremental()
        {
            ObservableCollection<EngineRow> rows = null;
            ICollectionView view = null;
            var changes = new List<CollectionChange>();
            RunOnUIThread.Execute(() =>
            {
                rows = MakeEngineRows();
                var cvs = new CollectionViewSource { Source = rows, IsLiveSortingRequested = true };
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                view = cvs.View;
                view.VectorChanged += (sender, args) => changes.Add(args.CollectionChange);

                rows[0].Score = 5;
                rows.Insert(2, new EngineRow(50, "New", 25, "X"));
                Verify.AreEqual("ItemInserted", string.Join(",", changes), "The insert is applied at once, as an insert.");
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(changes.Contains(CollectionChange.Reset), "No Reset.");
                Verify.AreEqual(string.Join(" ", rows.Select((r, i) => (r, i)).OrderBy(p => p.r.Score).ThenBy(p => p.i).Select(p => p.r.Name)), EngineNames(view),
                    "The restore placed the edited row.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a row inserted into an expanded tree reaches the rows as an insert, not a Reset, and the tree shows it under its parent.")]
        public void VerifyTreeInsertIsSplicedIntoRows()
        {
            var list = MakeLiveTree();
            CollectionViewSource cvs = null;
            var tableView = PlaceLive(list, s => { }, out cvs, IdPath, ParentIdPath);
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);
            Verify.AreEqual("Dee1- Eli2 Ann1- Cat2 Bob2", TreeLabelsOf(tableView), "Precondition: siblings by Score.");

            var actions = new List<string>();
            RunOnUIThread.Execute(() =>
            {
                GetRowsRepeater(tableView).ItemsSourceView.CollectionChanged += (sender, args) => actions.Add(args.Action.ToString());
                list.Add(new LiveRow { Id = 6, Name = "Fay", Score = 20, Team = "X", ParentId = 1 });
            });
            SettleLayout(tableView);

            Verify.AreEqual("Add", string.Join(",", actions), "One row inserted, no Reset.");
            Verify.AreEqual("Dee1- Eli2 Ann1- Cat2 Fay2 Bob2", TreeLabelsOf(tableView), "Fay is among Ann's children by Score.");

            actions.Clear();
            RunOnUIThread.Execute(() => list.Remove(Named(list, "Eli")));
            SettleLayout(tableView);
            Verify.AreEqual("Remove", string.Join(",", actions), "One row removed, no Reset.");
            Verify.AreEqual("Dee1 Ann1- Cat2 Fay2 Bob2", TreeLabelsOf(tableView), "Dee lost its only child.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies random inserts, removes, replacements, re-parentings, moves and expand/collapse on a TableView tree over a sorted view leave the rows (labels, depth, expansion, position in set) equal to a full rebuild of the same tree and expansion.")]
        public void VerifyIncrementalTreeMatchesRebuildFuzz()
        {
            var random = new Random(2026);
            var nextId = 1;
            LiveRow NewRow()
            {
                var id = nextId++;
                int? parent = null;
                var roll = random.Next(10);
                if (roll < 6 && id > 1)
                {
                    parent = random.Next(1, id);
                }
                else if (roll == 6)
                {
                    parent = 1000 + random.Next(5);
                }
                return new LiveRow { Id = id, Name = $"N{id}", Score = random.Next(20), Team = "X", ParentId = parent };
            }

            var list = new ObservableCollection<LiveRow>();
            for (var i = 0; i < 14; i++)
            {
                list.Add(NewRow());
            }
            CollectionViewSource cvs = null;
            var tableView = PlaceLive(list, s => { }, out cvs, IdPath, ParentIdPath);
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);

            string Rows()
            {
                string labels = null;
                RunOnUIThread.Execute(() => labels = TreeLabels(tableView) + " | " + PositionLabels(tableView));
                return labels;
            }

            for (var step = 0; step < 160; step++)
            {
                var op = "";
                RunOnUIThread.Execute(() =>
                {
                    var roll = random.Next(100);
                    if (list.Count < 6 || (roll < 30 && list.Count < 24))
                    {
                        var row = NewRow();
                        var at = random.Next(list.Count + 1);
                        list.Insert(at, row);
                        op = $"insert {row.Name}->{row.ParentId}@{at}";
                    }
                    else if (roll < 55)
                    {
                        var at = random.Next(list.Count);
                        op = $"remove {list[at].Name}";
                        list.RemoveAt(at);
                    }
                    else if (roll < 75)
                    {
                        // Same key, new parent (no cycle: a parent's Id is always smaller) or a new score.
                        var at = random.Next(list.Count);
                        var old = list[at];
                        var parent = (old.Id > 1 && random.Next(3) > 0) ? random.Next(1, old.Id) : (int?)null;
                        list[at] = new LiveRow { Id = old.Id, Name = old.Name, Score = random.Next(20), Team = "X", ParentId = parent };
                        op = $"replace {old.Name}->{parent}";
                    }
                    else if (roll < 85)
                    {
                        var from = random.Next(list.Count);
                        var to = random.Next(list.Count);
                        list.Move(from, to);
                        op = $"move {from}->{to}";
                    }
                    else
                    {
                        var rows = GetProjectedElements(tableView).OfType<TableViewRow>().Where(r => r.IsExpandable).ToList();
                        if (rows.Count > 0)
                        {
                            var row = rows[random.Next(rows.Count)];
                            var expand = !row.IsExpanded;
                            ToggleTreeRow(tableView, NameOf(row.DataContext), expand);
                            op = $"{(expand ? "expand" : "collapse")} {NameOf(row.DataContext)}";
                        }
                    }
                });
                SettleLayout(tableView);
                var incremental = Rows();

                RunOnUIThread.Execute(() => ((ICollectionViewShaping)cvs.View).Refresh());
                SettleLayout(tableView);
                var rebuilt = Rows();
                if (rebuilt != incremental)
                {
                    Verify.Fail($"step {step} ({op}): the rows differ from a rebuild.\n  rebuilt     {rebuilt}\n  incremental {incremental}");
                }
            }
        }

        #region TableView over a live view

        [TestMethod]
        [TestProperty("Description", "Verifies a sort key edit on a live CollectionViewSource view moves the TableView row, several edits in one turn settle into one order, and an unrelated edit moves nothing.")]
        public void VerifyLiveSortMovesTableViewRows()
        {
            var list = MakeLiveFlat();
            CollectionViewSource cvs = null;
            var tableView = PlaceLive(list, s => s.IsLiveSortingRequested = true, out cvs);
            Verify.AreEqual("A B C D", TreeLabelsOf(tableView), "Precondition: sorted by Score.");

            Verify.AreEqual("B C D A", After(tableView, () => Named(list, "A").Score = 50), "A sort key edit moves the row.");
            Verify.AreEqual("C D B A", After(tableView, () =>
            {
                Named(list, "B").Score = 45;
                Named(list, "C").Score = 1;
            }), "Edits in one turn settle into one order.");
            Verify.AreEqual("C D2 B A", After(tableView, () => Named(list, "D").Name = "D2"), "Renaming does not reorder a Score sort.");
            Verify.AreEqual("E C D2 B A", After(tableView, () =>
            {
                Named(list, "A").Score = 60;
                list.Add(new LiveRow { Id = 5, Name = "E", Score = 0, Team = "X" });
            }), "An edit and an Add in one turn settle into the sorted order.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies selection stays on the TableView row a live sort moved.")]
        public void VerifyLiveSortKeepsTableViewSelection()
        {
            var list = MakeLiveFlat();
            CollectionViewSource cvs = null;
            var tableView = PlaceLive(list, s => s.IsLiveSortingRequested = true, out cvs);
            RunOnUIThread.Execute(() => tableView.Select(0));
            SettleLayout(tableView);

            After(tableView, () => Named(list, "A").Score = 50);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("A", (tableView.SelectedItem as LiveRow)?.Name ?? "(none)", "Selection follows the moved item.");
                Verify.AreEqual(3, tableView.SelectedIndex, "SelectedIndex reports its new position.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies turning live sorting off stops tracking edits, and turning it on again catches up the edit made meanwhile.")]
        public void VerifyLiveSortOffThenOnInTableView()
        {
            var list = MakeLiveFlat();
            CollectionViewSource cvs = null;
            var tableView = PlaceLive(list, s => s.IsLiveSortingRequested = true, out cvs);

            RunOnUIThread.Execute(() => cvs.IsLiveSortingRequested = false);
            Verify.AreEqual("A B C D", After(tableView, () => Named(list, "A").Score = 50), "Edits made with live sorting off do not move rows.");
            Verify.AreEqual("B C D A", After(tableView, () => cvs.IsLiveSortingRequested = true), "Turning it on catches up the edit.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies live filtering removes and restores TableView rows as the filtered property changes, and an added item is tracked while a removed one is not.")]
        public void VerifyLiveFilterInTableView()
        {
            var list = MakeLiveFlat();
            CollectionViewSource cvs = null;
            var tableView = PlaceLive(list, s =>
            {
                ((ICollectionViewShaping)s.View).Filter = item => ((LiveRow)item).Score < 35;
                s.IsLiveFilteringRequested = true;
                s.LiveFilteringProperties.Add("Score");
            }, out cvs);
            Verify.AreEqual("A B C", TreeLabelsOf(tableView), "Precondition: D is filtered out.");

            Verify.AreEqual("A C", After(tableView, () => Named(list, "B").Score = 36), "B leaves the filter.");
            Verify.AreEqual("A B C", After(tableView, () => Named(list, "B").Score = 20), "B joins it again.");

            var added = new LiveRow { Id = 5, Name = "E", Score = 99, Team = "X" };
            Verify.AreEqual("A B C", After(tableView, () => list.Add(added)), "Precondition: E is filtered out.");
            Verify.AreEqual("A B C E", After(tableView, () => added.Score = 33), "An added item is tracked.");

            var removed = Named(list, "A");
            Verify.AreEqual("B C E", After(tableView, () => list.Remove(removed)), "Precondition: A is removed.");
            Verify.AreEqual("B C E", After(tableView, () => removed.Score = 1), "A removed item is no longer tracked.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a live group key edit moves the TableView row to its group, creating it, and removes the group it emptied.")]
        public void VerifyLiveGroupingInTableView()
        {
            var list = MakeLiveFlat();
            CollectionViewSource cvs = null;
            var tableView = PlaceLive(list, s =>
            {
                s.GroupDescriptions.Add(new PropertyGroupDescription("Team"));
                s.IsLiveGroupingRequested = true;
            }, out cvs);
            Verify.AreEqual("[X] A C [Y] B D", TreeLabelsOf(tableView), "Precondition: grouped by Team.");

            Verify.AreEqual("[X] A C [Y] B [Z] D", After(tableView, () => Named(list, "D").Team = "Z"), "D moves to a new group.");
            Verify.AreEqual("[X] A B C [Z] D", After(tableView, () => Named(list, "B").Team = "X"), "B joins X and Y goes.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a live sort on a tree moves a root with its expanded subtree within its siblings.")]
        public void VerifyLiveSortMovesTreeSubtree()
        {
            var list = MakeLiveTree();
            CollectionViewSource cvs = null;
            var tableView = PlaceLive(list, s => s.IsLiveSortingRequested = true, out cvs, IdPath, ParentIdPath);
            RunOnUIThread.Execute(() => tableView.ExpandAllRows());
            SettleLayout(tableView);
            Verify.AreEqual("Dee1- Eli2 Ann1- Cat2 Bob2", TreeLabelsOf(tableView), "Precondition: siblings by Score.");

            Verify.AreEqual("Ann1- Cat2 Bob2 Dee1- Eli2", After(tableView, () => Named(list, "Dee").Score = 70), "Dee moves after Ann with its child.");
            Verify.AreEqual("Ann1- Bob2 Cat2 Dee1- Eli2", After(tableView, () => Named(list, "Cat").Score = 40), "Cat moves within Ann's children.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a live tick that moves many rows of a tree rebuilds it at most once per dispatcher turn, not twice per moved row.")]
        public void VerifyTreeRebuildsOncePerLiveTick()
        {
            var list = new ObservableCollection<LiveRow>(Enumerable.Range(1, 20).Select(i => new LiveRow { Id = i, Name = $"R{i:00}", Score = i, Team = "X" }));
            CollectionViewSource cvs = null;
            var tableView = PlaceLive(list, s => s.IsLiveSortingRequested = true, out cvs, IdPath, ParentIdPath);

            LiveRow.IdReads = 0;
            var labels = After(tableView, () =>
            {
                foreach (var row in list.Take(8))
                {
                    row.Score += 100;
                }
            });

            Verify.AreEqual(string.Join(" ", list.OrderBy(r => r.Score).Select(r => r.Name + "1")), labels, "The tree shows the new order.");
            Log.Comment($"Id reads for one tick moving 8 of {list.Count} rows: {LiveRow.IdReads}");
            Verify.IsLessThanOrEqual(LiveRow.IdReads, 4 * list.Count, "The tree is rebuilt at most a couple of times for the tick, not twice per moved row.");
        }

        private TableView PlaceLive(ObservableCollection<LiveRow> list, Action<CollectionViewSource> configure, out CollectionViewSource source,
            string itemKeyPath = null, string parentKeyPath = null)
        {
            TableView tableView = null;
            CollectionViewSource cvs = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                cvs = new CollectionViewSource { Source = list };
                cvs.SortDescriptions.Add(new SortDescription("Score", CollectionViewSortDirection.Ascending));
                configure(cvs);
                tableView = CreateTreeTable(cvs.View, itemKeyPath, parentKeyPath);
                tableView.Tag = cvs;
                LoadContent(tableView);
            });
            SettleLayout(tableView);
            source = cvs;
            return tableView;
        }

        private static string TreeLabelsOf(TableView tableView)
        {
            string labels = null;
            RunOnUIThread.Execute(() => labels = TreeLabels(tableView));
            return labels;
        }

        private static string After(TableView tableView, Action action)
        {
            RunOnUIThread.Execute(action);
            SettleLayout(tableView);
            return TreeLabelsOf(tableView);
        }

        #endregion

        #region Fixtures

        internal static ObservableCollection<EngineRow> MakeEngineRows() => new ObservableCollection<EngineRow>(new[]
        {
            new EngineRow(1, "Kai", 30, "X"),
            new EngineRow(2, "Ada", 10, "Y"),
            new EngineRow(3, "Lin", 20, "X"),
            new EngineRow(4, "Bo", 40, "Z"),
            new EngineRow(5, "Eve", 20, "Y"),
        });

        internal static string EngineNames(IEnumerable<object> items) => string.Join(" ", items.Select(item => ((EngineRow)item).Name));

        private static object GroupProperty(object group, string name)
            => ((ICustomPropertyProvider)group).GetCustomProperty(name).GetValue(group);

        // Runs action from the next change of view, once.
        private static void NotifyOnce(ICollectionView view, Action action)
        {
            var done = false;
            view.VectorChanged += (sender, args) =>
            {
                if (!done)
                {
                    done = true;
                    action();
                }
            };
        }

        // A view under random changes, and the same rules applied by a fresh view over a copy of its items.
        private sealed class EngineFixture
        {
            private static readonly string[] s_names = { "Ada", "Bo", "Cy", "Dee", "Eve" };
            private static readonly string[] s_teams = { "X", "Y", "Z", "W" };
            private readonly Random m_random;
            private readonly int m_levels;
            private readonly bool m_isLive;
            private readonly ObservableCollection<EngineRow> m_rows = new ObservableCollection<EngineRow>();
            private readonly CollectionViewSource m_cvs;
            private int m_nextId = 1;
            private int m_threshold = 3;
            private bool m_descending;
            private bool m_secondKey = true;

            public EngineFixture(Random random, int levels, bool isLive)
            {
                m_random = random;
                m_levels = levels;
                m_isLive = isLive;
                for (var i = 0; i < 30; i++)
                {
                    m_rows.Add(NewRow());
                }

                m_cvs = new CollectionViewSource { Source = m_rows };
                ApplyRules(m_cvs);
                if (isLive)
                {
                    m_cvs.IsLiveSortingRequested = true;
                    m_cvs.IsLiveFilteringRequested = true;
                    m_cvs.IsLiveGroupingRequested = true;
                    m_cvs.LiveFilteringProperties.Add("Score");
                }

                Verify.IsNotNull(m_cvs.View);
            }

            public string ApplyRandomOperation(bool allowPropertyEdits)
            {
                var roll = m_random.Next(100);
                if (allowPropertyEdits && roll < 45 && m_rows.Count > 0)
                {
                    var row = m_rows[m_random.Next(m_rows.Count)];
                    switch (m_random.Next(4))
                    {
                        case 0: row.Score = m_random.Next(10); return $"edit {row.Id}.Score={row.Score}";
                        case 1: row.Team = s_teams[m_random.Next(s_teams.Length)]; return $"edit {row.Id}.Team={row.Team}";
                        case 2: row.Bucket = m_random.Next(3); return $"edit {row.Id}.Bucket={row.Bucket}";
                        default: row.Name = s_names[m_random.Next(s_names.Length)]; return $"edit {row.Id}.Name={row.Name}";
                    }
                }

                roll = m_random.Next(100);
                if (roll < 30 || m_rows.Count == 0)
                {
                    var index = m_random.Next(m_rows.Count + 1);
                    var row = NewRow();
                    m_rows.Insert(index, row);
                    return $"insert {row.Id}@{index}";
                }
                if (roll < 50)
                {
                    var index = m_random.Next(m_rows.Count);
                    m_rows.RemoveAt(index);
                    return $"remove @{index}";
                }
                if (roll < 65)
                {
                    var index = m_random.Next(m_rows.Count);
                    var row = NewRow();
                    m_rows[index] = row;
                    return $"replace @{index} by {row.Id}";
                }
                if (roll < 75)
                {
                    var from = m_random.Next(m_rows.Count);
                    var to = m_random.Next(m_rows.Count);
                    m_rows.Move(from, to);
                    return $"move {from}->{to}";
                }
                if (roll < 77)
                {
                    m_rows.Clear();
                    for (var i = 0; i < 10; i++)
                    {
                        m_rows.Add(NewRow());
                    }
                    return "clear";
                }
                if (roll < 85)
                {
                    m_descending = !m_descending;
                    m_cvs.SortDescriptions[0] = new SortDescription("Score", m_descending ? CollectionViewSortDirection.Descending : CollectionViewSortDirection.Ascending);
                    return "flip sort";
                }
                if (roll < 90)
                {
                    m_threshold = m_random.Next(6);
                    ((ICollectionViewShaping)m_cvs.View).Filter = MakeFilter(m_threshold);
                    return $"filter >= {m_threshold}";
                }
                if (roll < 94)
                {
                    m_secondKey = !m_secondKey;
                    using (m_cvs.DeferRefresh())
                    {
                        m_cvs.SortDescriptions.Clear();
                        AddSortDescriptions(m_cvs);
                    }
                    return $"second key {m_secondKey}";
                }
                ((ICollectionViewShaping)m_cvs.View).Refresh();
                return "refresh";
            }

            // The oracle: one view per rule set over a copy of the items, recomputed from scratch by a Reset each time.
            private readonly ResettableEngineRows m_freshRows = new ResettableEngineRows();
            private CollectionViewSource m_fresh;
            private string m_freshRules;

            public bool Matches(out string expected, out string actual)
            {
                var rules = $"{m_descending}|{m_secondKey}|{m_threshold}";
                if (m_fresh == null || m_freshRules != rules)
                {
                    m_fresh = new CollectionViewSource { Source = m_freshRows };
                    ApplyRules(m_fresh);
                    m_freshRules = rules;
                }
                m_freshRows.ReplaceAll(m_rows);
                expected = Describe(m_fresh.View);
                actual = Describe(m_cvs.View);
                return expected == actual;
            }

            public void VerifyMatchesFresh(string context)
            {
                if (!Matches(out var expected, out var actual))
                {
                    Verify.Fail($"{context}: the view differs from a fresh view.\n  expected {expected}\n  actual   {actual}");
                }
            }

            private void ApplyRules(CollectionViewSource cvs)
            {
                AddSortDescriptions(cvs);
                ((ICollectionViewShaping)cvs.View).Filter = MakeFilter(m_threshold);
                if (m_levels >= 1)
                {
                    // Levels 3: two levels, the teams ordered by their item count (which changes with the items).
                    var team = new PropertyGroupDescription("Team");
                    if (m_levels == 3)
                    {
                        team.SortDescriptions.Add(new SortDescription("ItemCount", CollectionViewSortDirection.Descending));
                    }
                    team.SortDescriptions.Add(new SortDescription("Name", CollectionViewSortDirection.Ascending));
                    cvs.GroupDescriptions.Add(team);
                }
                if (m_levels >= 2)
                {
                    var bucket = new PropertyGroupDescription("Bucket");
                    bucket.SortDescriptions.Add(new SortDescription("Name", CollectionViewSortDirection.Descending));
                    cvs.GroupDescriptions.Add(bucket);
                }
            }

            private void AddSortDescriptions(CollectionViewSource cvs)
            {
                cvs.SortDescriptions.Add(new SortDescription("Score", m_descending ? CollectionViewSortDirection.Descending : CollectionViewSortDirection.Ascending));
                if (m_secondKey)
                {
                    cvs.SortDescriptions.Add(new SortDescription("Name", CollectionViewSortDirection.Ascending));
                }
            }

            private static CollectionViewFilter MakeFilter(int threshold) => item => ((EngineRow)item).Score >= threshold;

            private EngineRow NewRow()
            {
                var id = m_nextId++;
                return new EngineRow(id, s_names[m_random.Next(s_names.Length)], m_random.Next(10), s_teams[m_random.Next(s_teams.Length)])
                {
                    Bucket = m_random.Next(3),
                };
            }
        }

        // The items and the group tree of a view (keys, counts, items by Id).
        private static string Describe(ICollectionView view)
        {
            var items = string.Join(",", view.Select(item => ((EngineRow)item).Id));
            if (view.CollectionGroups == null)
            {
                return items;
            }

            return items + " | " + string.Join(" ", view.CollectionGroups.Select(entry => DescribeGroup(((ICollectionViewGroup)entry).Group)));
        }

        private static string DescribeGroup(object group)
        {
            var viewGroup = (ICollectionViewGroup)group;
            var key = GroupProperty(group, "Key");
            var count = (int)GroupProperty(group, "ItemCount");
            var children = viewGroup.GroupItems;
            if (children.Count > 0 && children[0] is ICollectionViewGroup)
            {
                return $"{key}#{count}{{{string.Join(" ", children.Select(DescribeGroup))}}}";
            }
            return $"{key}#{count}[{string.Join(",", children.Select(item => ((EngineRow)item).Id))}]";
        }

        #endregion
    }

    internal sealed class EngineRow : INotifyPropertyChanged
    {
        private string _name;
        private int _score;
        private string _team;
        private int _bucket;

        public EngineRow(int id, string name, int score, string team)
        {
            Id = id;
            _name = name;
            _score = score;
            _team = team;
        }

        public int Id { get; }
        public string Name { get => _name; set => Set(ref _name, value); }
        public int Score { get => _score; set => Set(ref _score, value); }
        public string Team { get => _team; set => Set(ref _team, value); }
        public int Bucket { get => _bucket; set => Set(ref _bucket, value); }
        public long Big { get; set; }
        public ulong UBig { get; set; }
        public object Owner { get; set; }

        public event PropertyChangedEventHandler PropertyChanged;

        private void Set<T>(ref T field, T value, [CallerMemberName] string name = null)
        {
            if (EqualityComparer<T>.Default.Equals(field, value))
            {
                return;
            }

            field = value;
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    // Holds its rows and the CollectionViewSource over them, as a view model would.
    internal sealed class EngineOwner
    {
        public ObservableCollection<EngineRow> Rows { get; set; }
        public CollectionViewSource Source { get; set; }
        public ICollectionView View { get; set; }

        public int CompareScores(object left, object right) => ((EngineRow)left).Score.CompareTo(((EngineRow)right).Score);

        public int CompareNames(object left, object right) => string.CompareOrdinal((string)left, (string)right);
    }

    internal sealed class OwnerConverter : IValueConverter
    {
        public EngineOwner Owner { get; set; }

        public object Convert(object value, Type targetType, object parameter, string language) => value;

        public object ConvertBack(object value, Type targetType, object parameter, string language) => throw new NotSupportedException();
    }

    internal sealed class OwnerGroupDescription : GroupDescription
    {
        public EngineOwner Owner { get; set; }

        protected override object GroupNameFromItemCore(object item, int level, string culture) => ((EngineRow)item).Bucket;
    }

    internal sealed class TeamGroupDescription : GroupDescription
    {
        protected override object GroupNameFromItemCore(object item, int level, string culture) => ((EngineRow)item).Team;
    }

    internal sealed class IgnoreCaseTeamGroupDescription : GroupDescription
    {
        protected override object GroupNameFromItemCore(object item, int level, string culture) => ((EngineRow)item).Team;

        protected override bool NamesMatchCore(object groupName, object itemName) => string.Equals((string)groupName, (string)itemName, StringComparison.OrdinalIgnoreCase);
    }

    // Throws once, when armed.
    internal sealed class ThrowOnce
    {
        public bool IsArmed { get; set; }
        public int Count { get; private set; }

        public void Check()
        {
            if (IsArmed)
            {
                IsArmed = false;
                Count++;
                throw new InvalidOperationException("Thrown by the test.");
            }
        }
    }

    internal sealed class ThrowingConverter : IValueConverter
    {
        public ThrowOnce Thrower { get; set; }

        public object Convert(object value, Type targetType, object parameter, string language)
        {
            Thrower.Check();
            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language) => throw new NotSupportedException();
    }

    internal sealed class ThrowingGroupDescription : GroupDescription
    {
        public ThrowOnce Thrower { get; set; }
        public bool ThrowsInNamesMatch { get; set; }

        protected override object GroupNameFromItemCore(object item, int level, string culture)
        {
            if (!ThrowsInNamesMatch)
            {
                Thrower.Check();
            }
            return ((EngineRow)item).Team;
        }

        protected override bool NamesMatchCore(object groupName, object itemName)
        {
            if (ThrowsInNamesMatch)
            {
                Thrower.Check();
            }
            return string.Equals(groupName as string, itemName as string, StringComparison.Ordinal);
        }
    }

    internal sealed class ResettableEngineRows : ObservableCollection<EngineRow>
    {
        public void ReplaceAll(IEnumerable<EngineRow> rows)
        {
            Items.Clear();
            foreach (var row in rows)
            {
                Items.Add(row);
            }
            OnCollectionChanged(new global::System.Collections.Specialized.NotifyCollectionChangedEventArgs(global::System.Collections.Specialized.NotifyCollectionChangedAction.Reset));
        }
    }

    // Loads arrive as one Reset, as from an AddRange-style collection.
    internal sealed partial class ResetIncrementalEngineRows : ObservableCollection<EngineRow>, ISupportIncrementalLoading
    {
        private int m_nextId = 1000;

        public ResetIncrementalEngineRows(IEnumerable<EngineRow> rows) : base(rows)
        {
        }

        public bool HasMoreItems => true;

        public IAsyncOperation<LoadMoreItemsResult> LoadMoreItemsAsync(uint count)
        {
            for (uint i = 0; i < count; i++)
            {
                var id = m_nextId++;
                Items.Add(new EngineRow(id, $"L{id}", 100 + (int)i, "X"));
            }
            OnCollectionChanged(new global::System.Collections.Specialized.NotifyCollectionChangedEventArgs(global::System.Collections.Specialized.NotifyCollectionChangedAction.Reset));
            return AsyncInfo.Run(_ => Task.FromResult(new LoadMoreItemsResult { Count = count }));
        }
    }

    internal sealed partial class IncrementalEngineRows : ObservableCollection<EngineRow>, ISupportIncrementalLoading
    {
        private int m_nextId = 1000;

        public IncrementalEngineRows(IEnumerable<EngineRow> rows) : base(rows)
        {
        }

        public bool CanLoad { get; set; } = true;
        public int LoadCount { get; private set; }

        public bool HasMoreItems => CanLoad;

        public IAsyncOperation<LoadMoreItemsResult> LoadMoreItemsAsync(uint count)
        {
            LoadCount++;
            for (uint i = 0; i < count; i++)
            {
                var id = m_nextId++;
                Add(new EngineRow(id, $"L{id}", 100 + (int)i, "X"));
            }

            return AsyncInfo.Run(_ => Task.FromResult(new LoadMoreItemsResult { Count = count }));
        }
    }
}
