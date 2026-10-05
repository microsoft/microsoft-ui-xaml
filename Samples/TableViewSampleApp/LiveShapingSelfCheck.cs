using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Media;

namespace TableViewSampleApp;

// Observable row for the live-shaping checks. Every property raises PropertyChanged, which is the
// only signal live shaping listens to.
public sealed class LiveRow : INotifyPropertyChanged
{
    private string _name = "";
    private int _score;
    private string _team = "";
    private int? _parentId;
    private LiveAddress _addr = new("");

    public int Id { get; init; }
    public string Name { get => _name; set => Set(ref _name, value); }
    public int Score { get => _score; set => Set(ref _score, value); }
    public string Team { get => _team; set => Set(ref _team, value); }
    public int? ParentId { get => _parentId; set => Set(ref _parentId, value); }
    public LiveAddress Addr { get => _addr; set => Set(ref _addr, value); }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void Set<T>(ref T field, T value, [CallerMemberName] string? name = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value))
        {
            return;
        }

        field = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    }
}

// Observable, but a change to City is raised on the ADDRESS, not on the row that owns it.
public sealed class LiveAddress : INotifyPropertyChanged
{
    private string _city;

    public LiveAddress(string city) => _city = city;

    public string City
    {
        get => _city;
        set
        {
            _city = value;
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(City)));
        }
    }

    public event PropertyChangedEventHandler? PropertyChanged;
}

// Same shape with no change notification at all.
public sealed class PlainRow
{
    public string Name { get; set; } = "";
    public int Score { get; set; }
}

public sealed record LiveCheckResult(string Name, bool Passed, string Expected, string Actual);

// Scripted checks for TableViewSource.IsLiveSorting / IsLiveFiltering / IsLiveGrouping, driven
// through the public API only. Each case builds its own source and TableView, mutates item
// properties, lets the dispatcher run the posted reshape, and compares the realized rows.
//
// Row text: flat rows print the name; hierarchical rows print name + Level (1-based) + "-" when
// expanded / "+" when collapsed; group headers print [key].
public sealed class LiveShapingSelfCheck
{
    private readonly Border _host;
    private readonly DispatcherQueue _queue;
    private readonly List<LiveCheckResult> _results = new();

    public LiveShapingSelfCheck(Border host, DispatcherQueue queue)
    {
        _host = host;
        _queue = queue;
    }

    // ---- fixtures ---------------------------------------------------------------------------

    private static TableViewKeySelector K(Func<LiveRow, object?> f) => new(o => f((LiveRow)o)!);
    private static readonly TableViewKeySelector ByScore = K(r => r.Score);
    private static readonly TableViewKeySelector ByTeam = K(r => r.Team);
    private static readonly TableViewKeySelector ById = K(r => r.Id);
    private static readonly TableViewKeySelector ByParent = K(r => r.ParentId);

    private static TableViewPredicate MinScore(int min) => new(o => ((LiveRow)o).Score >= min);

    // Flat: source order A B C D, scores ascending, teams alternating.
    private static ObservableCollection<LiveRow> Flat() => new(new[]
    {
        new LiveRow { Id = 1, Name = "A", Score = 10, Team = "X", Addr = new("Amsterdam") },
        new LiveRow { Id = 2, Name = "B", Score = 20, Team = "Y", Addr = new("Berlin") },
        new LiveRow { Id = 3, Name = "C", Score = 30, Team = "X", Addr = new("Cairo") },
        new LiveRow { Id = 4, Name = "D", Score = 40, Team = "Y", Addr = new("Dublin") },
    });

    // Hierarchy (ParentId): Ann > { Bob, Cat }, Dee > { Eli }. Children share their root's team.
    private static ObservableCollection<LiveRow> Tree() => new(new[]
    {
        new LiveRow { Id = 1, Name = "Ann", Score = 50, Team = "X" },
        new LiveRow { Id = 2, Name = "Bob", Score = 30, Team = "X", ParentId = 1 },
        new LiveRow { Id = 3, Name = "Cat", Score = 10, Team = "X", ParentId = 1 },
        new LiveRow { Id = 4, Name = "Dee", Score = 20, Team = "Y" },
        new LiveRow { Id = 5, Name = "Eli", Score = 5, Team = "Y", ParentId = 4 },
    });

    private static LiveRow Row(IEnumerable<LiveRow> list, string name) => list.First(r => r.Name == name);

    // ---- harness ----------------------------------------------------------------------------

    // Low priority so every Normal-priority posted live-shaping restore runs first.
    private async Task Settle()
    {
        for (int i = 0; i < 6; i++)
        {
            var tcs = new TaskCompletionSource();
            _queue.TryEnqueue(DispatcherQueuePriority.Low, () =>
            {
                _host.UpdateLayout();
                tcs.SetResult();
            });
            await tcs.Task;
            await Task.Delay(20);
        }
    }

    private async Task<TableView> Place(TableViewSource source, bool expandRows = false)
    {
        var tv = new TableView { GridLinesVisibility = TableViewGridLinesVisibility.Horizontal };
        tv.Columns.Add(SampleColumns.Text("Name", nameof(LiveRow.Name), SampleColumns.Star()));
        tv.ItemsSource = source;
        _host.Child = tv;
        await Settle();
        if (expandRows)
        {
            tv.ExpandAllRows();
            await Settle();
        }

        return tv;
    }

    private static T? FindDescendant<T>(DependencyObject root, string? name = null) where T : FrameworkElement
    {
        int n = VisualTreeHelper.GetChildrenCount(root);
        for (int i = 0; i < n; i++)
        {
            var c = VisualTreeHelper.GetChild(root, i);
            if (c is T t && (name is null || t.Name == name))
            {
                return t;
            }

            var d = FindDescendant<T>(c, name);
            if (d is not null)
            {
                return d;
            }
        }

        return null;
    }

    private static List<UIElement> Realized(TableView tv)
    {
        var repeater = FindDescendant<ItemsRepeater>(tv, "PART_RowsRepeater")
            ?? throw new InvalidOperationException("PART_RowsRepeater not found");
        var list = new List<(int Index, UIElement Element)>();
        int n = VisualTreeHelper.GetChildrenCount(repeater);
        for (int i = 0; i < n; i++)
        {
            if (VisualTreeHelper.GetChild(repeater, i) is UIElement child)
            {
                int idx = repeater.GetElementIndex(child);
                if (idx >= 0)
                {
                    list.Add((idx, child));
                }
            }
        }

        return list.OrderBy(p => p.Index).Select(p => p.Element).ToList();
    }

    private static string HeaderText(UIElement element)
    {
        if (element is FrameworkElement fe)
        {
            var info = fe.DataContext as TableViewGroupInfo
                ?? (fe as ContentControl)?.Content as TableViewGroupInfo
                ?? FindDescendant<TableViewGroupHeader>(fe)?.DataContext as TableViewGroupInfo;
            if (info is not null)
            {
                return $"[{info.Key}]";
            }
        }

        return "[H]";
    }

    private static string RowText(TableViewRow r)
    {
        var name = r.DataContext switch
        {
            LiveRow l => l.Name,
            PlainRow p => p.Name,
            _ => "?",
        };
        if (r.Level == 0)
        {
            return name;
        }

        var mark = r.IsExpandable ? (r.IsExpanded ? "-" : "+") : "";
        return $"{name}{r.Level}{mark}";
    }

    private static string Snapshot(TableView tv) =>
        string.Join(" ", Realized(tv).Select(e => e is TableViewRow r ? RowText(r) : HeaderText(e)));

    private async Task Case(string name, string expected, Func<Task<string>> run)
    {
        string actual;
        try
        {
            actual = await run();
        }
        catch (Exception ex)
        {
            actual = "EXCEPTION " + ex.GetType().Name + ": " + ex.Message;
        }

        _results.Add(new LiveCheckResult(name, actual == expected, expected, actual));
    }

    // Mutates, settles, snapshots. The common shape of most cases.
    private async Task<string> After(TableView tv, Action mutate)
    {
        mutate();
        await Settle();
        return Snapshot(tv);
    }

    // ---- cases ------------------------------------------------------------------------------

    public async Task<List<LiveCheckResult>> RunAllAsync()
    {
        _results.Clear();

        // -- contract basics --

        await Case("Flat.DefaultsAreOff", "False False False", () =>
        {
            var src = TableViewSource.From(Flat());
            return Task.FromResult($"{src.IsLiveSorting} {src.IsLiveFiltering} {src.IsLiveGrouping}");
        });

        await Case("Flat.FlagsRoundTrip", "True True True|False False False", () =>
        {
            var src = TableViewSource.From(Flat());
            src.IsLiveSorting = src.IsLiveFiltering = src.IsLiveGrouping = true;
            var on = $"{src.IsLiveSorting} {src.IsLiveFiltering} {src.IsLiveGrouping}";
            src.IsLiveSorting = src.IsLiveFiltering = src.IsLiveGrouping = false;
            return Task.FromResult($"{on}|{src.IsLiveSorting} {src.IsLiveFiltering} {src.IsLiveGrouping}");
        });

        // -- sorting --

        await Case("Sort.LiveOff_StaysStale", "A B C D", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Sort(ByScore, SortDirection.Ascending);
            var tv = await Place(src);
            return await After(tv, () => Row(list, "A").Score = 50);
        });

        await Case("Sort.LiveOn_RowMoves", "B C D A", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Sort(ByScore, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            return await After(tv, () => Row(list, "A").Score = 50);
        });

        await Case("Sort.LiveOn_PathAxis", "D A B C", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Sort(nameof(LiveRow.Score), SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            return await After(tv, () => Row(list, "D").Score = 5);
        });

        await Case("Sort.ReshapeIsPosted_NotInline", "inline=A B C D; posted=B C D A", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Sort(ByScore, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            Row(list, "A").Score = 50;
            tv.UpdateLayout();
            var inline = Snapshot(tv);
            await Settle();
            return $"inline={inline}; posted={Snapshot(tv)}";
        });

        await Case("Sort.BulkMutation_OneTurn", "C D B A", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Sort(ByScore, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            return await After(tv, () =>
            {
                Row(list, "A").Score = 50;
                Row(list, "B").Score = 45;
                Row(list, "C").Score = 1;
            });
        });

        await Case("Sort.UnrelatedProperty_KeepsOrder", "A B2 C D", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Sort(ByScore, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            return await After(tv, () => Row(list, "B").Name = "B2");
        });

        await Case("Sort.SelectionFollowsItem", "A 3", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Sort(ByScore, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            tv.Select(0);
            await Settle();
            Row(list, "A").Score = 50;
            await Settle();
            return $"{(tv.SelectedItem as LiveRow)?.Name ?? "(none)"} {tv.SelectedIndex}";
        });

        await Case("Sort.ThrowingSelector_NoCrash", "rows=4", async () =>
        {
            var list = Flat();
            var throwing = K(r => r.Score == 99 ? throw new InvalidOperationException("boom") : r.Score);
            var src = TableViewSource.From(list).Sort(throwing, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            await After(tv, () => Row(list, "A").Score = 99);
            return $"rows={Realized(tv).Count}";
        });

        await Case("Sort.DisableStopsTracking", "A B C D", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Sort(ByScore, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            src.IsLiveSorting = false;
            return await After(tv, () => Row(list, "A").Score = 50);
        });

        // -- source membership changes keep subscriptions in step --

        await Case("Source.AddedItemIsTracked", "add=A B E C D; mutate=A B C D E", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Sort(ByScore, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            var e = new LiveRow { Id = 5, Name = "E", Score = 25, Team = "X" };
            var added = await After(tv, () => list.Add(e));
            var mutated = await After(tv, () => e.Score = 100);
            return $"add={added}; mutate={mutated}";
        });

        await Case("Source.RemovedItemIsUntracked", "B C D", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Sort(ByScore, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            var a = Row(list, "A");
            await After(tv, () => list.Remove(a));
            return await After(tv, () => a.Score = 100);
        });

        await Case("Source.ReplacedItem_OldOutNewIn", "old=A2 B C D; new=B C D A2", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Sort(ByScore, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            var oldA = list[0];
            var newA = new LiveRow { Id = 1, Name = "A2", Score = 10, Team = "X" };
            await After(tv, () => list[0] = newA);
            var afterOld = await After(tv, () => oldA.Score = 100);
            var afterNew = await After(tv, () => newA.Score = 100);
            return $"old={afterOld}; new={afterNew}";
        });

        await Case("Source.ResetRetracks", "B C D A", async () =>
        {
            var list = Flat();
            var items = list.ToList();
            var src = TableViewSource.From(list).Sort(ByScore, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            list.Clear();
            foreach (var item in items)
            {
                list.Add(item);
            }

            await Settle();
            return await After(tv, () => Row(list, "A").Score = 50);
        });

        // -- filtering --

        await Case("Filter.LiveOff_StaysStale", "B C D", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Filter(MinScore(20));
            var tv = await Place(src);
            return await After(tv, () =>
            {
                Row(list, "B").Score = 5;   // should leave
                Row(list, "A").Score = 25;  // should join
            });
        });

        await Case("Filter.LiveOn_LeaveAndJoin", "leave=C D; join=A C D", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Filter(MinScore(20));
            src.IsLiveFiltering = true;
            var tv = await Place(src);
            var leave = await After(tv, () => Row(list, "B").Score = 5);
            var join = await After(tv, () => Row(list, "A").Score = 25);
            return $"leave={leave}; join={join}";
        });

        // The flags are TRIGGERS, not scopes: a reshape triggered by a live sort key re-runs the
        // whole pipeline, so the (non-live) filter is re-evaluated too. WPF only repositions.
        await Case("Filter.TriggeredBySortReshape", "C D", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Filter(MinScore(20)).Sort(ByScore, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            return await After(tv, () => Row(list, "B").Score = 5);
        });

        // -- grouping --

        await Case("Group.LiveOff_StaysStale", "[X] A C [Y] B D", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).GroupBy(ByTeam);
            var tv = await Place(src);
            return await After(tv, () => Row(list, "A").Team = "Y");
        });

        // The reshape is a full rebuild, so buckets are re-ordered by first appearance in the source:
        // A (first item) now opens [Y], which moves ahead of [X]. WPF keeps existing groups in place
        // and only moves the item.
        await Case("Group.LiveOn_RowChangesBucket", "[Y] A B D [X] C", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).GroupBy(ByTeam);
            src.IsLiveGrouping = true;
            var tv = await Place(src);
            return await After(tv, () => Row(list, "A").Team = "Y");
        });

        // -- documented constraints (vs. WPF's per-property binding observation) --

        await Case("Constraint.NoINPC_NotTracked", "A B C D", async () =>
        {
            var list = new ObservableCollection<PlainRow>(new[]
            {
                new PlainRow { Name = "A", Score = 10 }, new PlainRow { Name = "B", Score = 20 },
                new PlainRow { Name = "C", Score = 30 }, new PlainRow { Name = "D", Score = 40 },
            });
            var src = TableViewSource.From(list).Sort(nameof(PlainRow.Score), SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            return await After(tv, () => list[0].Score = 50);
        });

        await Case("Constraint.NestedPathHop_NotTracked", "nested=A B C D; replaced=B C D A", async () =>
        {
            var list = Flat();
            var src = TableViewSource.From(list).Sort("Addr.City", SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src);
            var nested = await After(tv, () => Row(list, "A").Addr.City = "Zagreb");
            var replaced = await After(tv, () => Row(list, "A").Addr = new LiveAddress("Zagreb"));
            return $"nested={nested}; replaced={replaced}";
        });

        // -- hierarchy (ParentBy) --

        await Case("Tree.LiveSort_WithinSiblings", "before=Dee1- Eli2 Ann1- Cat2 Bob2; after=Dee1- Eli2 Ann1- Bob2 Cat2", async () =>
        {
            var list = Tree();
            var src = TableViewSource.From(list).ParentBy(ById, ByParent).Sort(ByScore, SortDirection.Ascending);
            src.IsLiveSorting = true;
            var tv = await Place(src, expandRows: true);
            var before = Snapshot(tv);
            var after = await After(tv, () => Row(list, "Cat").Score = 40);
            return $"before={before}; after={after}";
        });

        await Case("Tree.LiveOff_ReparentStaysStale", "Ann1- Bob2 Cat2 Dee1- Eli2", async () =>
        {
            var list = Tree();
            var src = TableViewSource.From(list).ParentBy(ById, ByParent);
            var tv = await Place(src, expandRows: true);
            return await After(tv, () => Row(list, "Eli").ParentId = 1);
        });

        await Case("Tree.LiveOn_ReparentByProperty", "Ann1- Bob2 Cat2 Eli2 Dee1", async () =>
        {
            var list = Tree();
            var src = TableViewSource.From(list).ParentBy(ById, ByParent);
            src.IsLiveSorting = true;
            var tv = await Place(src, expandRows: true);
            return await After(tv, () => Row(list, "Eli").ParentId = 1);
        });

        await Case("Tree.CycleKeepsProjection_ThenRecovers",
            "cycle=Ann1- Bob2 Cat2 Dee1- Eli2; recovered=Ann1- Bob2 Cat2 Eli2 Dee1", async () =>
        {
            var list = Tree();
            var src = TableViewSource.From(list).ParentBy(ById, ByParent);
            src.IsLiveSorting = true;
            var tv = await Place(src, expandRows: true);
            var cycle = await After(tv, () => Row(list, "Ann").ParentId = 2);
            await After(tv, () => Row(list, "Ann").ParentId = null);
            var recovered = await After(tv, () => Row(list, "Eli").ParentId = 1);
            return $"cycle={cycle}; recovered={recovered}";
        });

        await Case("Tree.LiveFilter_MatchBringsAncestor", "before=Ann1; after=Ann1 Dee1- Eli2", async () =>
        {
            var list = Tree();
            var src = TableViewSource.From(list).ParentBy(ById, ByParent).Filter(MinScore(35));
            src.IsLiveFiltering = true;
            var tv = await Place(src, expandRows: true);
            var before = Snapshot(tv);
            var after = await After(tv, () => Row(list, "Eli").Score = 40);
            return $"before={before}; after={after}";
        });

        await Case("Tree.Grouped_RootMovesWithSubtree", "[Y] Ann1- Bob2 Cat2 Dee1- Eli2", async () =>
        {
            var list = Tree();
            var src = TableViewSource.From(list).ParentBy(ById, ByParent).GroupBy(ByTeam);
            src.IsLiveGrouping = true;
            var tv = await Place(src, expandRows: true);
            return await After(tv, () => Row(list, "Ann").Team = "Y");
        });

        await Case("Tree.Grouped_DescendantKeyIgnored", "[X] Ann1- Bob2 Cat2 [Y] Dee1- Eli2", async () =>
        {
            var list = Tree();
            var src = TableViewSource.From(list).ParentBy(ById, ByParent).GroupBy(ByTeam);
            src.IsLiveGrouping = true;
            var tv = await Place(src, expandRows: true);
            return await After(tv, () => Row(list, "Bob").Team = "Z");
        });

        _host.Child = null;
        return _results.ToList();
    }
}
