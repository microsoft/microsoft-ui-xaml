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
using ListSortDirection = Microsoft.UI.Xaml.Data.ListSortDirection;

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
        [TestProperty("Description", "Verifies random source changes, rule changes and refreshes leave a flat, grouped and two-level grouped view equal to a fresh view over the same items.")]
        public void VerifyIncrementalMatchesFreshViewFuzz()
        {
            foreach (var levels in new[] { 0, 1, 2 })
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
        }

        [TestMethod]
        [TestProperty("Description", "Verifies random property edits on a live sorted, filtered and grouped view, mixed with source changes, settle into the view a fresh view over the same items shows.")]
        public void VerifyLiveShapingMatchesFreshViewFuzz()
        {
            foreach (var levels in new[] { 0, 1, 2 })
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
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() => fixture.VerifyMatchesFresh($"live, levels {levels}, round {round} ({string.Join(", ", ops)})"));
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
                cvs.SortDescriptions.Add(new SortDescription("Score", ListSortDirection.Ascending));
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
                cvs.SortDescriptions.Add(new SortDescription("Score", ListSortDirection.Ascending));
                var view = cvs.View;
                var shaping = (ICollectionViewShaping)view;
                var handled = 0;
                view.VectorChanged += (sender, args) =>
                {
                    if (handled++ == 0)
                    {
                        shaping.SortDescriptions[0] = new SortDescription("Score", ListSortDirection.Descending);
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
                        shaping.SortDescriptions.Add(new SortDescription("Name", ListSortDirection.Descending));
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

                cvs.SortDescriptions.Add(new SortDescription("Score", ListSortDirection.Descending));
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
        [TestProperty("Description", "Verifies FilterRequested keeps filtering after a GC when the app keeps only the View, as WPF's view keeps the CollectionViewSource through the wrapped filter.")]
        public void VerifyFilterRequestedSurvivesGCWhenOnlyViewIsHeld()
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
            cvs.FilterRequested += (sender, args) => args.Accepted = ((EngineRow)args.Item).Score >= 20;
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
            owner.Source.SortDescriptions.Add(new SortDescription("Score", ListSortDirection.Ascending));
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

                cvs.SortDescriptions.Add(new SortDescription("Score", ListSortDirection.Descending));
                Verify.IsTrue(view.IsCurrentBeforeFirst, "A sort keeps it before the first item.");
                Verify.IsNull(view.CurrentItem);

                view.MoveCurrentToPosition(view.Count);
                Verify.IsTrue(view.IsCurrentAfterLast, "Precondition: after the last item.");
                ((ICollectionViewShaping)view).Refresh();
                Verify.IsTrue(view.IsCurrentAfterLast, "A refresh keeps it after the last item.");

                view.MoveCurrentToPosition(1);
                var current = view.CurrentItem;
                cvs.SortDescriptions[0] = new SortDescription("Score", ListSortDirection.Ascending);
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
                cvs.SortDescriptions.Add(new SortDescription("Name", ListSortDirection.Ascending));
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
                cvs.SortDescriptions.Add(new SortDescription("Big", ListSortDirection.Ascending));
                Verify.AreEqual("neg r0 r1 r2 r3", EngineNames(cvs.View), "Int64 keys in exact order.");

                cvs.SortDescriptions[0] = new SortDescription("UBig", ListSortDirection.Descending);
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
                cvs.SortDescriptions.Add(new SortDescription("Score", ListSortDirection.Ascending));
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
                cvs.SortDescriptions.Add(new SortDescription("Score", ListSortDirection.Descending));
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

                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => cvs.SortDescriptions.Add(new SortDescription("Length", ListSortDirection.Ascending))),
                    "A sort the view cannot apply throws.");
                cvs.SortDescriptions.Clear();
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => cvs.FilterRequested += (sender, args) => { }),
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
                cvs.SortDescriptions.Add(new SortDescription("Score", ListSortDirection.Ascending));
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
                cvs.SortDescriptions.Add(new SortDescription("Score", ListSortDirection.Ascending));
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
        [TestProperty("Description", "Verifies a live tick in which most rows changed their sort key is restored by one Reset, as WPF re-sorts the whole list then, and a tick with few changes by moves.")]
        public void VerifyMassLiveSortChangeIsOneReset()
        {
            ObservableCollection<EngineRow> rows = null;
            ICollectionView view = null;
            var changes = new List<CollectionChange>();
            RunOnUIThread.Execute(() =>
            {
                rows = new ObservableCollection<EngineRow>(Enumerable.Range(0, 10).Select(i => new EngineRow(i, $"r{i}", i, "X")));
                var cvs = new CollectionViewSource { Source = rows, IsLiveSortingRequested = true };
                cvs.SortDescriptions.Add(new SortDescription("Score", ListSortDirection.Ascending));
                view = cvs.View;
                view.VectorChanged += (sender, args) => changes.Add(args.CollectionChange);
                foreach (var row in rows.Take(9))
                {
                    row.Score = 100 - row.Score;
                }
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Reset", string.Join(",", changes), "Nine of ten rows changed: one Reset.");
                Verify.AreEqual(string.Join(" ", rows.OrderBy(r => r.Score).Select(r => r.Name)), EngineNames(view));
                changes.Clear();
                rows[0].Score = -1;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("ItemRemoved,ItemInserted", string.Join(",", changes), "One row changed: one move.");
                Verify.AreEqual("r0", ((EngineRow)view[0]).Name);
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
                s.Filter = item => ((LiveRow)item).Score < 35;
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
                cvs.SortDescriptions.Add(new SortDescription("Score", ListSortDirection.Ascending));
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
                    m_cvs.SortDescriptions[0] = new SortDescription("Score", m_descending ? ListSortDirection.Descending : ListSortDirection.Ascending);
                    return "flip sort";
                }
                if (roll < 90)
                {
                    m_threshold = m_random.Next(6);
                    m_cvs.Filter = MakeFilter(m_threshold);
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

            public void VerifyMatchesFresh(string context)
            {
                var fresh = new CollectionViewSource { Source = m_rows.ToList() };
                ApplyRules(fresh);
                var expected = Describe(fresh.View);
                var actual = Describe(m_cvs.View);
                if (expected != actual)
                {
                    Verify.Fail($"{context}: the view differs from a fresh view.\n  expected {expected}\n  actual   {actual}");
                }
            }

            private void ApplyRules(CollectionViewSource cvs)
            {
                AddSortDescriptions(cvs);
                cvs.Filter = MakeFilter(m_threshold);
                if (m_levels >= 1)
                {
                    var team = new PropertyGroupDescription("Team");
                    team.SortDescriptions.Add(new SortDescription("Name", ListSortDirection.Ascending));
                    cvs.GroupDescriptions.Add(team);
                }
                if (m_levels >= 2)
                {
                    var bucket = new PropertyGroupDescription("Bucket");
                    bucket.SortDescriptions.Add(new SortDescription("Name", ListSortDirection.Descending));
                    cvs.GroupDescriptions.Add(bucket);
                }
            }

            private void AddSortDescriptions(CollectionViewSource cvs)
            {
                cvs.SortDescriptions.Add(new SortDescription("Score", m_descending ? ListSortDirection.Descending : ListSortDirection.Ascending));
                if (m_secondKey)
                {
                    cvs.SortDescriptions.Add(new SortDescription("Name", ListSortDirection.Ascending));
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
