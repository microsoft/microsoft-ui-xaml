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
using System.Diagnostics;
using System.Linq;
using System.Runtime.CompilerServices;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

using SortDescription = Microsoft.UI.Xaml.Data.SortDescription;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Timings of the CollectionViewSource shaping engine and TableView's tree over it. Not part of a normal run:
    // te.exe ... /select:"@Name='*CollectionViewPerfBenchmarks*'" /runIgnoredTests. Each result is logged as
    // "BENCH|<scenario>|<items>|<milliseconds>".
    [TestClass]
    [TestProperty("Ignore", "True")]
    public class CollectionViewPerfBenchmarks : TableViewApiTestBase
    {
        private static readonly int[] s_sizes = { 10_000, 100_000 };

        [TestMethod]
        [TestProperty("Description", "Times the first sort and a Refresh of a filtered, sorted view.")]
        public void BenchSortAndRefresh()
        {
            foreach (var n in s_sizes)
            {
                RunOnUIThread.Execute(() =>
                {
                    var rows = MakeRows(n, seed: 1);
                    var cvs = new CollectionViewSource { Source = rows };
                    cvs.SortDescriptions.Add(new SortDescription(nameof(BenchRow.Name), CollectionViewSortDirection.Ascending));
                    var view = cvs.View;
                    Verify.AreEqual(n, view.Count);
                    Report("SortString.Refresh", n, Median(() => ((ICollectionViewShaping)view).Refresh()));

                    var intSorted = new CollectionViewSource { Source = rows };
                    intSorted.SortDescriptions.Add(new SortDescription(nameof(BenchRow.Score), CollectionViewSortDirection.Ascending));
                    intSorted.SortDescriptions.Add(new SortDescription(nameof(BenchRow.Id), CollectionViewSortDirection.Descending));
                    var intView = intSorted.View;
                    Report("SortInt2.Refresh", n, Median(() => ((ICollectionViewShaping)intView).Refresh()));

                    var filtered = new CollectionViewSource { Source = rows };
                    ((ICollectionViewShaping)filtered.View).Filter = item => ((BenchRow)item).Score % 2 == 0;
                    filtered.SortDescriptions.Add(new SortDescription(nameof(BenchRow.Name), CollectionViewSortDirection.Ascending));
                    var filteredView = filtered.View;
                    Report("FilterSortString.Refresh", n, Median(() => ((ICollectionViewShaping)filteredView).Refresh()));
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Times single source inserts and removes on a sorted flat view and a sorted view grouped into 100 groups.")]
        public void BenchIncrementalChanges()
        {
            const int changes = 200;
            foreach (var n in s_sizes)
            {
                foreach (var grouped in new[] { false, true })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        var rows = MakeRows(n, seed: 2);
                        var cvs = new CollectionViewSource { Source = rows };
                        cvs.SortDescriptions.Add(new SortDescription(nameof(BenchRow.Name), CollectionViewSortDirection.Ascending));
                        if (grouped)
                        {
                            cvs.GroupDescriptions.Add(new PropertyGroupDescription(nameof(BenchRow.Team100)));
                        }
                        Verify.AreEqual(n, cvs.View.Count);

                        var random = new Random(3);
                        var name = grouped ? "Grouped100" : "Flat";
                        var insertMs = Time(() =>
                        {
                            for (var i = 0; i < changes; i++)
                            {
                                rows.Insert(random.Next(rows.Count + 1), NewRow(n + i, random));
                            }
                        });
                        Report($"{name}.InsertEach", n, insertMs / changes);

                        var removeMs = Time(() =>
                        {
                            for (var i = 0; i < changes; i++)
                            {
                                rows.RemoveAt(random.Next(rows.Count));
                            }
                        });
                        Report($"{name}.RemoveEach", n, removeMs / changes);

                        var replaceMs = Time(() =>
                        {
                            for (var i = 0; i < changes; i++)
                            {
                                rows[random.Next(rows.Count)] = NewRow(2 * n + i, random);
                            }
                        });
                        Report($"{name}.ReplaceEach", n, replaceMs / changes);
                        Verify.AreEqual(n, cvs.View.Count);
                    });
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Times grouping a sorted view into 1000 groups, and into three levels.")]
        public void BenchGrouping()
        {
            foreach (var n in s_sizes)
            {
                RunOnUIThread.Execute(() =>
                {
                    var rows = MakeRows(n, seed: 4);
                    var cvs = new CollectionViewSource { Source = rows };
                    cvs.SortDescriptions.Add(new SortDescription(nameof(BenchRow.Name), CollectionViewSortDirection.Ascending));
                    var view = cvs.View;
                    Verify.AreEqual(n, view.Count);

                    Report("Group1k.Regroup", n, Time(() => cvs.GroupDescriptions.Add(new PropertyGroupDescription(nameof(BenchRow.Team1k)))));
                    Verify.AreEqual(1000, view.CollectionGroups.Count);
                    Report("Group1k.Refresh", n, Time(() => ((ICollectionViewShaping)view).Refresh()));

                    var ignoreCase = new PropertyGroupDescription(nameof(BenchRow.Team1k), null, CollectionViewStringComparison.CurrentCultureIgnoreCase);
                    Report("Group1kCultureIgnoreCase.Regroup", n, Time(() => cvs.GroupDescriptions[0] = ignoreCase));

                    cvs.GroupDescriptions.Clear();
                    Report("Group3Levels.Regroup", n, Time(() =>
                    {
                        using (cvs.DeferRefresh())
                        {
                            cvs.GroupDescriptions.Add(new PropertyGroupDescription(nameof(BenchRow.Team10)));
                            cvs.GroupDescriptions.Add(new PropertyGroupDescription(nameof(BenchRow.Bucket)));
                            cvs.GroupDescriptions.Add(new PropertyGroupDescription(nameof(BenchRow.Shard)));
                        }
                    }));
                    Verify.AreEqual(10, view.CollectionGroups.Count);
                    Verify.AreEqual(n, view.Count);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Times grouping into 1000 groups by an app GroupDescription subclass, with and without a NamesMatch override.")]
        public void BenchCustomGroupDescription()
        {
            foreach (var n in s_sizes)
            {
                RunOnUIThread.Execute(() =>
                {
                    var rows = MakeRows(n, seed: 8);
                    var cvs = new CollectionViewSource { Source = rows };
                    cvs.SortDescriptions.Add(new SortDescription(nameof(BenchRow.Name), CollectionViewSortDirection.Ascending));
                    var view = cvs.View;
                    Verify.AreEqual(n, view.Count);

                    Report("CustomGroup1k.Regroup", n, Time(() => cvs.GroupDescriptions.Add(new BenchTeamGroupDescription())));
                    Verify.AreEqual(1000, view.CollectionGroups.Count);
                    var replaceMs = Time(() =>
                    {
                        var random = new Random(9);
                        for (var i = 0; i < 200; i++)
                        {
                            rows[random.Next(rows.Count)] = NewRow(3 * n + i, random);
                        }
                    });
                    Report("CustomGroup1k.ReplaceEach", n, replaceMs / 200);

                    cvs.GroupDescriptions.Clear();
                    Report("CustomGroup1kNamesMatch.Regroup", n, Time(() => cvs.GroupDescriptions.Add(new BenchTeamNamesMatchGroupDescription())));
                    Verify.AreEqual(1000, view.CollectionGroups.Count);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Times a live sorting tick in which 1000 rows, or 90% of the rows, change their sort key.")]
        public void BenchLiveSortingTick()
        {
            foreach (var n in s_sizes)
            {
                foreach (var grouped in new[] { false, true })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        var rows = MakeRows(n, seed: 5);
                        var cvs = new CollectionViewSource { Source = rows, IsLiveSortingRequested = true };
                        cvs.SortDescriptions.Add(new SortDescription(nameof(BenchRow.Score), CollectionViewSortDirection.Ascending));
                        if (grouped)
                        {
                            cvs.GroupDescriptions.Add(new PropertyGroupDescription(nameof(BenchRow.Team100)));
                        }
                        ICollectionView view = null;
                        var name = grouped ? "LiveGrouped100" : "LiveFlat";
                        // The first sort and one observer per row (P-4).
                        Report($"{name}.Create", n, Time(() => view = cvs.View));
                        var shaping = (ICollectionViewShaping)view;
                        Verify.AreEqual(n, view.Count);

                        Report($"{name}.Setup", n, Time(() => shaping.Refresh()));

                        var random = new Random(6);
                        var resets = 0;
                        var moves = 0;
                        view.VectorChanged += (sender, args) =>
                        {
                            if (args.CollectionChange == global::Windows.Foundation.Collections.CollectionChange.Reset)
                            {
                                resets++;
                            }
                            else if (args.CollectionChange == global::Windows.Foundation.Collections.CollectionChange.ItemInserted)
                            {
                                moves++;
                            }
                        };
                        foreach (var changed in new[] { 50, 255, 256, 1000, n * 9 / 10 })
                        {
                            resets = 0;
                            moves = 0;
                            var ms = Time(() =>
                            {
                                using (shaping.DeferRefresh())
                                {
                                    for (var i = 0; i < changed; i++)
                                    {
                                        rows[random.Next(rows.Count)].Score = random.Next(1_000_000);
                                    }
                                }
                            });
                            var tick = changed == n * 9 / 10 ? "Tick90pct" : (changed == 1000 ? "Tick1k" : $"Tick{changed}");
                            Report($"{name}.{tick}", n, ms);
                            Log.Comment($"BENCHCHANGES|{name}.{tick}|{n}|resets={resets}|moves={moves}");
                        }

                        var previous = int.MinValue;
                        if (!grouped)
                        {
                            foreach (BenchRow row in view)
                            {
                                Verify.IsTrue(row.Score >= previous, "The view is sorted after the ticks.");
                                previous = row.Score;
                            }
                        }
                    });
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Times TableView's tree over a sorted view: expanding every row and inserting one row.")]
        public void BenchTree()
        {
            foreach (var n in s_sizes)
            {
                ObservableCollection<BenchRow> rows = null;
                TableView tableView = null;
                RunOnUIThread.Execute(() =>
                {
                    rows = MakeTreeRows(n);
                    var cvs = new CollectionViewSource { Source = rows };
                    cvs.SortDescriptions.Add(new SortDescription(nameof(BenchRow.Score), CollectionViewSortDirection.Ascending));
                    var ms = Time(() =>
                    {
                        tableView = CreateTreeTable(cvs.View, nameof(BenchRow.Id), nameof(BenchRow.ParentId));
                        tableView.Tag = cvs;
                        LoadContent(tableView);
                    });
                    Report("Tree.Load", n, ms);
                });
                SettleLayout(tableView);

                RunOnUIThread.Execute(() => Report("Tree.ExpandAll", n, Time(() => tableView.ExpandAllRows())));
                SettleLayout(tableView);

                var total = 0.0;
                const int inserts = 5;
                for (var i = 0; i < inserts; i++)
                {
                    RunOnUIThread.Execute(() =>
                    {
                        var row = new BenchRow(n + 1 + i, "Inserted", i, parentId: 1 + i);
                        total += Time(() => rows.Insert(rows.Count / 2, row));
                    });
                    SettleLayout(tableView);
                }
                Report("Tree.InsertEach", n, total / inserts);

                total = 0.0;
                for (var i = 0; i < inserts; i++)
                {
                    RunOnUIThread.Execute(() => total += Time(() => rows.RemoveAt(rows.Count - 1 - i)));
                    SettleLayout(tableView);
                }
                Report("Tree.RemoveLeafEach", n, total / inserts);

                // Through the end of the dispatcher turn, which holds any work the tree defers.
                Report("Tree.TurnBaseline", n, Enumerable.Range(0, inserts).Select(_ => TimeThroughTurn(() => { })).Average());
                total = 0.0;
                for (var i = 0; i < inserts; i++)
                {
                    var row = new BenchRow(n + 101 + i, "Inserted", i, parentId: 1 + i);
                    total += TimeThroughTurn(() => rows.Insert(rows.Count / 2, row));
                    SettleLayout(tableView);
                }
                Report("Tree.InsertTurnEach", n, total / inserts);

                total = 0.0;
                for (var i = 0; i < inserts; i++)
                {
                    total += TimeThroughTurn(() => rows.RemoveAt(rows.Count - 1 - i));
                    SettleLayout(tableView);
                }
                Report("Tree.RemoveLeafTurnEach", n, total / inserts);

                total = 0.0;
                for (var i = 0; i < inserts; i++)
                {
                    var at = rows.Count - 1 - 2 * inserts - i;
                    var old = rows[at];
                    var moved = new BenchRow(old.Id, old.Name, old.Score, parentId: 1 + (old.ParentId.Value + 7 * (i + 1)) % (n / 10));
                    total += TimeThroughTurn(() => rows[at] = moved);
                    SettleLayout(tableView);
                }
                Report("Tree.ReparentTurnEach", n, total / inserts);

                RunOnUIThread.Execute(() => Report("Tree.CollapseAll", n, Time(() => tableView.CollapseAllRows())));
                SettleLayout(tableView);
                RunOnUIThread.Execute(() => Content = null);
                IdleSynchronizer.Wait();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Times how long the UI thread takes to release views and groups the GC found unreachable: many small grouped views (over copies, or live over one shared source), and one view with many groups.")]
        public void BenchTeardown()
        {
            var baseline = DrainAfterCollect();
            Report("Teardown.Baseline", 0, baseline);
            foreach (var live in new[] { false, true })
            {
                // Created in one UI turn, so nothing created is released before the turn ends: the creation time grows
                // faster than the count, the drain does not.
                foreach (var views in live ? new[] { 25, 50, 100 } : new[] { 100, 400, 1600 })
                {
                    var created = 0.0;
                    RunOnUIThread.Execute(() => created = Time(() => CreateGroupedViews(views, 60, live)));
                    var drained = DrainAfterCollect();
                    var name = live ? "Teardown.LiveViewsOverOneSource" : "Teardown.ViewsOverCopies";
                    Report($"{name}.Create", views, created);
                    Report($"{name}.CreatePerView", views, created / views);
                    Report($"{name}.Drain", views, drained);
                    Report($"{name}.DrainPerView", views, (drained - baseline) / views);
                }
            }
            foreach (var views in new[] { 25, 100 })
            {
                var created = 0.0;
                for (var i = 0; i < views; i++)
                {
                    RunOnUIThread.Execute(() => created += Time(() => CreateGroupedViews(1, 60, live: true)));
                }
                Report("Teardown.LiveViewsOneTurnEach.CreatePerView", views, created / views);
                Report("Teardown.LiveViewsOneTurnEach.Drain", views, DrainAfterCollect());
            }
            foreach (var n in s_sizes)
            {
                RunOnUIThread.Execute(() => CreateGroupedViews(1, n, live: false, team: nameof(BenchRow.Team1k)));
                Report("Teardown.OneView1kGroups.Drain", n, DrainAfterCollect());
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void CreateGroupedViews(int views, int n, bool live, string team = nameof(BenchRow.Team10))
        {
            var rows = MakeRows(n, seed: 9);
            for (var i = 0; i < views; i++)
            {
                var cvs = new CollectionViewSource { Source = live ? (object)rows : rows.ToList() };
                cvs.SortDescriptions.Add(new SortDescription(nameof(BenchRow.Score), CollectionViewSortDirection.Ascending));
                cvs.GroupDescriptions.Add(new PropertyGroupDescription(team));
                cvs.GroupDescriptions.Add(new PropertyGroupDescription(nameof(BenchRow.Bucket)));
                var view = cvs.View;
                if (live)
                {
                    var liveShaping = (ICollectionViewLiveShaping)view;
                    liveShaping.IsLiveSorting = true;
                    liveShaping.IsLiveGrouping = true;
                }
                Verify.IsGreaterThan(view.CollectionGroups.Count, 0);
            }
        }

        // From the first GC until the UI thread is idle, retrying IdleSynchronizer's 10 s wait.
        private static double DrainAfterCollect()
        {
            var stopwatch = Stopwatch.StartNew();
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
            for (var attempt = 1; attempt <= 12; attempt++)
            {
                string error;
                try
                {
                    error = IdleSynchronizer.TryWait();
                }
                catch (global::System.Runtime.InteropServices.COMException ex)
                {
                    error = ex.Message;
                }
                if (string.IsNullOrEmpty(error))
                {
                    break;
                }
                Log.Comment($"idle wait {attempt} failed after {stopwatch.ElapsedMilliseconds} ms: {error}");
            }
            return stopwatch.Elapsed.TotalMilliseconds;
        }

        private static ObservableCollection<BenchRow> MakeRows(int n, int seed)
        {
            var random = new Random(seed);
            var rows = new List<BenchRow>(n);
            for (var i = 0; i < n; i++)
            {
                rows.Add(NewRow(i, random));
            }
            return new ObservableCollection<BenchRow>(rows);
        }

        private static BenchRow NewRow(int id, Random random)
        {
            var chars = new char[8];
            for (var c = 0; c < chars.Length; c++)
            {
                var letter = (char)('a' + random.Next(26));
                chars[c] = random.Next(4) == 0 ? char.ToUpperInvariant(letter) : letter;
            }
            return new BenchRow(id, new string(chars), random.Next(1_000_000), parentId: null);
        }

        // n/10 roots, each with nine children.
        private static ObservableCollection<BenchRow> MakeTreeRows(int n)
        {
            var roots = n / 10;
            var random = new Random(7);
            var rows = new List<BenchRow>(n);
            for (var i = 0; i < n; i++)
            {
                rows.Add(new BenchRow(i + 1, $"R{i}", random.Next(1_000_000), i < roots ? (int?)null : (i % roots) + 1));
            }
            return new ObservableCollection<BenchRow>(rows);
        }

        private static double Time(Action action)
        {
            var stopwatch = Stopwatch.StartNew();
            action();
            return stopwatch.Elapsed.TotalMilliseconds;
        }

        private static double TimeThroughTurn(Action action)
        {
            var elapsed = 0.0;
            using (var done = new global::System.Threading.ManualResetEventSlim(false))
            {
                RunOnUIThread.Execute(() =>
                {
                    var queue = Microsoft.UI.Dispatching.DispatcherQueue.GetForCurrentThread();
                    var stopwatch = Stopwatch.StartNew();
                    action();
                    queue.TryEnqueue(() =>
                    {
                        elapsed = stopwatch.Elapsed.TotalMilliseconds;
                        done.Set();
                    });
                });
                Verify.IsTrue(done.Wait(TimeSpan.FromSeconds(60)), "The turn ended.");
            }
            return elapsed;
        }
        private static double Median(Action action)
        {
            var samples = Enumerable.Range(0, 3).Select(_ => Time(action)).OrderBy(ms => ms).ToList();
            return samples[1];
        }

        private static void Report(string scenario, int n, double ms)
        {
            Log.Comment($"BENCH|{scenario}|{n}|{ms:F3}");
        }
    }

    internal sealed class BenchTeamGroupDescription : GroupDescription
    {
        protected override object GroupNameFromItemCore(object item, int level, string culture) => ((BenchRow)item).Team1k;
    }

    internal sealed class BenchTeamNamesMatchGroupDescription : GroupDescription
    {
        protected override object GroupNameFromItemCore(object item, int level, string culture) => ((BenchRow)item).Team1k;

        protected override bool NamesMatchCore(object groupName, object itemName) => string.Equals((string)groupName, (string)itemName, StringComparison.Ordinal);
    }

    internal sealed class BenchRow : INotifyPropertyChanged
    {
        private int _score;

        public BenchRow(int id, string name, int score, int? parentId)
        {
            Id = id;
            Name = name;
            _score = score;
            ParentId = parentId;
            Team100 = $"T{id % 100:D2}";
            Team1k = $"K{id % 1000:D3}";
            Team10 = $"G{id % 10}";
            Bucket = (id / 10) % 10;
            Shard = $"S{(id / 100) % 10}";
        }

        public int Id { get; }
        public string Name { get; }
        public int? ParentId { get; }
        public string Team100 { get; }
        public string Team1k { get; }
        public string Team10 { get; }
        public int Bucket { get; }
        public string Shard { get; }
        public int Score { get => _score; set => Set(ref _score, value); }

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
}
