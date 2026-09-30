using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Media;

namespace TableViewSampleApp;

public sealed class Emp
{
    public int Id { get; set; }
    public int? ManagerId { get; set; }
    public int? MentorId { get; set; }
    public long? BigManagerId { get; set; }
    public string Code { get; set; } = "";
    public string ParentCode { get; set; } = "";
    public Emp? Manager { get; set; }
    public string Name { get; set; } = "";
    public string Dept { get; set; } = "";
}

public sealed record CheckResult(string Name, bool Passed, string Expected, string Actual);

// An app-defined enum. Not a WinRT type, so a boxed value crosses the ABI as an opaque object.
public enum EmpKey
{
    None = 0,
}

// Raises ONE Reset for a whole-content replacement (ObservableCollection.Clear raises Reset on an
// empty collection, which is a different scenario: the keys really leave the source).
public sealed class ResettableCollection<T> : ObservableCollection<T>
{
    public ResettableCollection(IEnumerable<T> items) : base(items) { }

    public void ReplaceAll(IEnumerable<T> items)
    {
        var copy = items.ToList();
        Items.Clear();
        foreach (var item in copy)
        {
            Items.Add(item);
        }

        OnPropertyChanged(new System.ComponentModel.PropertyChangedEventArgs("Count"));
        OnPropertyChanged(new System.ComponentModel.PropertyChangedEventArgs("Item[]"));
        OnCollectionChanged(new System.Collections.Specialized.NotifyCollectionChangedEventArgs(
            System.Collections.Specialized.NotifyCollectionChangedAction.Reset));
    }
}

// Row level convention: TableViewRow.Level, which is 1-based for a hierarchical source (roots are\n// level 1) and 0 for a flat one (documented in TableView.idl).
public sealed class HierarchySelfCheck
{
    private readonly Border _host;
    private readonly DispatcherQueue _queue;
    private readonly List<CheckResult> _results = new();

    public HierarchySelfCheck(Border host, DispatcherQueue queue)
    {
        _host = host;
        _queue = queue;
    }

    private static TableViewKeySelector K(Func<Emp, object?> f) => new(o => f((Emp)o)!);
    private static readonly TableViewKeySelector Id = K(e => e.Id);
    private static readonly TableViewKeySelector Manager = K(e => e.ManagerId);
    private static readonly TableViewKeySelector Mentor = K(e => e.MentorId);

    private static ObservableCollection<Emp> Fixture()
    {
        var ada = new Emp { Id = 1, Name = "Ada", Dept = "Eng", Code = "A" };
        var ben = new Emp { Id = 2, ManagerId = 1, Manager = ada, Name = "Ben", Dept = "Eng", Code = "B", ParentCode = "A" };
        var cy = new Emp { Id = 3, ManagerId = 1, Manager = ada, Name = "Cy", Dept = "Eng", Code = "C", ParentCode = "A" };
        var dan = new Emp { Id = 4, ManagerId = 2, Manager = ben, Name = "Dan", Dept = "Eng", Code = "D", ParentCode = "B" };
        var eve = new Emp { Id = 5, Name = "Eve", Dept = "Ops", Code = "E" };
        var fay = new Emp { Id = 6, ManagerId = 5, Manager = eve, Name = "Fay", Dept = "Ops", Code = "F", ParentCode = "E" };
        var gus = new Emp { Id = 7, ManagerId = 99, Name = "Gus", Dept = "Ops", Code = "G", ParentCode = "Z" };
        foreach (var e in new[] { ada, ben, cy, dan, eve, fay, gus })
        {
            e.MentorId = null;
            e.BigManagerId = e.ManagerId;
        }

        ben.MentorId = 5;
        fay.MentorId = 1;
        return new(new[] { ada, ben, cy, dan, eve, fay, gus });
    }

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

    private TableView NewTable(TableViewSource source)
    {
        var tv = new TableView { GridLinesVisibility = TableViewGridLinesVisibility.Horizontal };
        tv.Columns.Add(SampleColumns.Text("Name", nameof(Emp.Name), SampleColumns.Star()));
        tv.ItemsSource = source;
        return tv;
    }

    private async Task<TableView> Place(TableViewSource source)
    {
        var tv = NewTable(source);
        _host.Child = tv;
        await Settle();
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

    private static List<(int Index, UIElement Element)> Realized(TableView tv)
    {
        var repeater = FindDescendant<ItemsRepeater>(tv, "PART_RowsRepeater")
            ?? throw new InvalidOperationException("PART_RowsRepeater not found");
        var list = new List<(int, UIElement)>();
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

        return list.OrderBy(p => p.Item1).ToList();
    }

    private static string RowText(TableViewRow r)
    {
        var name = (r.DataContext as Emp)?.Name ?? "?";
        var mark = r.IsExpandable ? (r.IsExpanded ? "-" : "+") : "";
        return $"{name}{r.Level}{mark}";
    }

    private static string Snapshot(TableView tv) =>
        string.Join(" ", Realized(tv).Select(p => p.Element is TableViewRow r ? RowText(r) : "[H]"));

    private static TableViewRow FindRow(TableView tv, string name) =>
        Realized(tv).Select(p => p.Element).OfType<TableViewRow>()
            .FirstOrDefault(r => (r.DataContext as Emp)?.Name == name)
        ?? throw new InvalidOperationException($"row {name} not realized; snapshot: {Snapshot(tv)}");

    private async Task Toggle(TableView tv, string name, bool expand)
    {
        var peer = FrameworkElementAutomationPeer.CreatePeerForElement(FindRow(tv, name));
        var p = (IExpandCollapseProvider)peer.GetPattern(PatternInterface.ExpandCollapse);
        if (expand)
        {
            p.Expand();
        }
        else
        {
            p.Collapse();
        }

        await Settle();
    }

    private static string Position(TableView tv) =>
        string.Join(" ", Realized(tv).Select(p => p.Element).OfType<TableViewRow>().Select(r =>
        {
            var peer = FrameworkElementAutomationPeer.CreatePeerForElement(r);
            return $"{((Emp)r.DataContext).Name} {peer.GetPositionInSet()}/{peer.GetSizeOfSet()}";
        }));

    private static TableViewSource Src(ObservableCollection<Emp> list, TableViewKeySelector key, TableViewKeySelector parent)
        => TableViewSource.From(list).WithParent(key, parent);

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

        _results.Add(new CheckResult(name, actual == expected, expected, actual));
    }

    private static string Thrown(Action a)
    {
        try
        {
            a();
            return "no exception";
        }
        catch (Exception ex)
        {
            return ex.GetType().Name;
        }
    }

    private static string ThrownMessage(Action a)
    {
        try
        {
            a();
            return "no exception";
        }
        catch (Exception ex)
        {
            return ex.Message;
        }
    }

    // Declares a relation, retracts it, then empties the source; counts the items still alive.
    [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)]
    private static (TableViewSource Source, ObservableCollection<Emp> List, List<WeakReference> Weak) RetractAndEmpty()
    {
        var list = Fixture();
        var weak = list.Select(e => new WeakReference(e)).ToList();
        var src = Src(list, Id, Manager);
        src.ClearParent();
        list.Clear();
        return (src, list, weak);
    }

    // Declares a throwing relation on a table that already shows a valid one; the snapshot must not change.
    private async Task<string> ThrowsKeepsSnapshot(Action<ObservableCollection<Emp>> mutate, Func<TableViewSource, TableViewSource> declare)
    {
        var list = Fixture();
        var src = Src(list, Id, Manager);
        var tv = await Place(src);
        var before = Snapshot(tv);
        if (before != "Ada1+ Eve1+ Gus1")
        {
            return $"unexpected pre-state: '{before}'";
        }

        mutate(list);
        var thrown = Thrown(() => declare(src));
        await Settle();
        var after = Snapshot(tv);
        return $"{thrown}; unchanged={before == after}";
    }

    public async Task<List<CheckResult>> RunAllAsync()
    {
        const string Roots = "Ada1+ Eve1+ Gus1";
        const string AllExpanded = "Ada1- Ben2- Dan3 Cy2 Eve1+ Gus1";

        await Case("RootsAndOrphans", Roots, async () =>
            Snapshot(await Place(Src(Fixture(), Id, Manager))));

        await Case("NestedReexpandKeepsState", AllExpanded, async () =>
        {
            var tv = await Place(Src(Fixture(), Id, Manager));
            await Toggle(tv, "Ada", true);
            await Toggle(tv, "Ben", true);
            await Toggle(tv, "Ada", false);
            await Toggle(tv, "Ada", true);
            return Snapshot(tv);
        });

        await Case("SortWithinSiblings", "Gus1 Eve1+ Ada1- Cy2 Ben2+", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            await Toggle(tv, "Ada", true);
            src.Sort(K(e => e.Name), SortDirection.Descending);
            await Settle();
            return Snapshot(tv);
        });

        await Case("SortKeepsExpansion", AllExpanded, async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            await Toggle(tv, "Ada", true);
            await Toggle(tv, "Ben", true);
            src.Sort(K(e => e.Name), SortDirection.Ascending);
            await Settle();
            return Snapshot(tv);
        });

        await Case("FilterKeepsAncestors", "Ada1- Ben2- Dan3", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            src.Filter(new TableViewPredicate(o => ((Emp)o).Name == "Dan"));
            await Settle();
            return Snapshot(tv);
        });

        await Case("FilterClearRestores", "Ada1+ Eve1- Fay2 Gus1", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            await Toggle(tv, "Eve", true);
            src.Filter(new TableViewPredicate(o => ((Emp)o).Name == "Dan"));
            await Settle();
            src.ClearFilter();
            await Settle();
            return Snapshot(tv);
        });

        await Case("CollapseContextRow", "Ada1+ | Ada1- Ben2- Dan3", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            src.Filter(new TableViewPredicate(o => ((Emp)o).Name == "Dan"));
            await Settle();
            await Toggle(tv, "Ada", false);
            var a = Snapshot(tv);
            await Toggle(tv, "Ada", true);
            return a + " | " + Snapshot(tv);
        });

        await Case("ContextCollapseRestoresIntent", "Ada1+ | Ada1- Ben2+ Cy2 Eve1+ Gus1", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            await Toggle(tv, "Ada", true);
            src.Filter(new TableViewPredicate(o => ((Emp)o).Name == "Dan"));
            await Settle();
            await Toggle(tv, "Ada", false);
            var a = Snapshot(tv);
            src.ClearFilter();
            await Settle();
            return a + " | " + Snapshot(tv);
        });

        await Case("RemoveParentOrphansChildren", "Ben1+ Cy1 Eve1+ Gus1", async () =>
        {
            var list = Fixture();
            var tv = await Place(Src(list, Id, Manager));
            await Toggle(tv, "Ada", true);
            list.RemoveAt(0);
            await Settle();
            return Snapshot(tv);
        });

        await Case("MoveKeepsSiblingOrderBySource", "Gus1 Ada1+ Eve1+", async () =>
        {
            var list = Fixture();
            var tv = await Place(Src(list, Id, Manager));
            list.Move(6, 0);
            await Settle();
            return Snapshot(tv);
        });

        await Case("ResetSourceKeepsIntent", "Ada1- Ben2+ Cy2 Eve1+ Gus1", async () =>
        {
            var list = new ResettableCollection<Emp>(Fixture());
            var tv = await Place(Src(list, Id, Manager));
            await Toggle(tv, "Ada", true);
            list.ReplaceAll(list.ToList());
            await Settle();
            return Snapshot(tv);
        });

        await Case("AddRoot", Roots + " Hal1", async () =>
        {
            var list = Fixture();
            var tv = await Place(Src(list, Id, Manager));
            list.Add(new Emp { Id = 8, Name = "Hal", Dept = "Ops" });
            await Settle();
            return Snapshot(tv);
        });

        await Case("AddChildUnderLeaf", "Ada1+ Eve1+ Gus1+ | Ada1+ Eve1+ Gus1- Ivy2", async () =>
        {
            var list = Fixture();
            var tv = await Place(Src(list, Id, Manager));
            list.Add(new Emp { Id = 9, ManagerId = 7, Name = "Ivy", Dept = "Ops" });
            await Settle();
            var a = Snapshot(tv);
            await Toggle(tv, "Gus", true);
            return a + " | " + Snapshot(tv);
        });

        await Case("ReplaceReparents", "Ada1- Ben2+ Eve1- Cy2 Fay2 Gus1", async () =>
        {
            var list = Fixture();
            var tv = await Place(Src(list, Id, Manager));
            await Toggle(tv, "Ada", true);
            list[2] = new Emp { Id = 3, ManagerId = 5, Name = "Cy", Dept = "Eng" };
            await Settle();
            await Toggle(tv, "Eve", true);
            return Snapshot(tv);
        });

        await Case("KeySurvivesRecreate", "Ada1- Ben2+ Cy2 Eve1+ Gus1", async () =>
        {
            var list = Fixture();
            var tv = await Place(Src(list, Id, Manager));
            await Toggle(tv, "Ada", true);
            list[0] = new Emp { Id = 1, Name = "Ada", Dept = "Eng" };
            await Settle();
            return Snapshot(tv);
        });

        await Case("RedeclareClearsIntent", Roots, async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            await Toggle(tv, "Ada", true);
            src.WithParent(Id, Manager);
            await Settle();
            return Snapshot(tv);
        });

        await Case("TwoRelationsOneSource",
            "Ada1+ Cy1 Dan1 Eve1+ Gus1 | Ada1- Fay2 Cy1 Dan1 Eve1- Ben2 Gus1 | " + Roots, async () =>
        {
            var list = Fixture();
            var tv1 = NewTable(Src(list, Id, Manager));
            var tv2 = NewTable(Src(list, Id, Mentor));
            // Side by side: the page scrolls, and a table below the fold would not realize its rows.
            var panel = new Grid { ColumnDefinitions = { new ColumnDefinition(), new ColumnDefinition() } };
            Grid.SetColumn(tv2, 1);
            panel.Children.Add(tv1);
            panel.Children.Add(tv2);
            _host.Child = panel;
            await Settle();
            var a = Snapshot(tv2);
            await Toggle(tv2, "Ada", true);
            await Toggle(tv2, "Eve", true);
            return a + " | " + Snapshot(tv2) + " | " + Snapshot(tv1);
        });

        await Case("ObjectKeys", Roots, async () =>
            Snapshot(await Place(Src(Fixture(), K(e => e), K(e => e.Manager)))));

        await Case("EmptyStringParentIsRoot", Roots, async () =>
            Snapshot(await Place(Src(Fixture(), K(e => e.Code), K(e => e.ParentCode)))));

        await Case("Int32VsInt64", "Ada1 Ben1 Cy1 Dan1 Eve1 Fay1 Gus1", async () =>
            Snapshot(await Place(Src(Fixture(), Id, K(e => e.BigManagerId)))));

        const string FullTree = "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1";

        // A WinRT enum boxes as a value, afresh on every call, and must still compare by value.
        await Case("EnumKeys", FullTree, async () =>
        {
            var tv = await Place(Src(Fixture(),
                K(e => (Windows.System.VirtualKey)e.Id),
                K(e => e.ManagerId is int m ? (Windows.System.VirtualKey)m : null)));
            tv.ExpandAllRows();
            await Settle();
            return Snapshot(tv);
        });

        // An app-defined .NET enum is not a WinRT type, so it crosses the ABI as an opaque object
        // and is compared by reference identity: a fresh box per call links nothing, and never
        // aliases into a spurious duplicate.
        await Case("AppEnumKeysAreIdentity", "Ada1 Ben1 Cy1 Dan1 Eve1 Fay1 Gus1", async () =>
            Snapshot(await Place(Src(Fixture(), K(e => (EmpKey)e.Id), K(e => e.ManagerId is int m ? (EmpKey)m : null)))));

        await Case("PointKeys", FullTree, async () =>
        {
            var tv = await Place(Src(Fixture(),
                K(e => new Windows.Foundation.Point(e.Id, 0)),
                K(e => e.ManagerId is int m ? new Windows.Foundation.Point(m, 0) : null)));
            tv.ExpandAllRows();
            await Settle();
            return Snapshot(tv);
        });

        // A fresh object per call is compared by reference identity: never a duplicate, never a match.
        await Case("FreshObjectKeysAreIdentity", "Ada1 Ben1 Cy1 Dan1 Eve1 Fay1 Gus1", async () =>
            Snapshot(await Place(Src(Fixture(), K(e => new object()), K(e => e.ManagerId is null ? null : new object())))));

        await Case("DuplicateEnumKeyNamesValue", "named", () =>
        {
            var msg = ThrownMessage(() => Src(Fixture(), K(e => (Windows.System.VirtualKey)(e.Id == 7 ? 3 : e.Id)), Manager));
            return Task.FromResult(msg.Contains("duplicate key '3'") ? "named" : msg);
        });

        // Ada (and her whole subtree) hangs below the Eve <-> Fay cycle and precedes it in source order.
        await Case("CycleNamesCycleMember", "named", () =>
        {
            var list = Fixture();
            list[0].ManagerId = 5;
            list[4].ManagerId = 6;
            var msg = ThrownMessage(() => Src(list, Id, Manager));
            return Task.FromResult(msg.Contains("involving key '5'") || msg.Contains("involving key '6'") ? "named" : msg);
        });

        await Case("ClearParentReleasesRows", "alive=0", () =>
        {
            var (src, list, weak) = RetractAndEmpty();
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
            var alive = weak.Count(w => w.IsAlive);
            GC.KeepAlive(src);
            GC.KeepAlive(list);
            return Task.FromResult($"alive={alive}");
        });

        await Case("DuplicateKeyThrows", "ArgumentException; unchanged=True", () =>
            ThrowsKeepsSnapshot(_ => { }, s => s.WithParent(K(e => 1), Manager)));

        await Case("CycleThrows", "ArgumentException; unchanged=True", () =>
            ThrowsKeepsSnapshot(l => l[0].ManagerId = 4, s => s.WithParent(Id, Manager)));

        await Case("SelfParentThrows", "ArgumentException; unchanged=True", () =>
            ThrowsKeepsSnapshot(l => l[0].ManagerId = 1, s => s.WithParent(Id, Manager)));

        await Case("NullKeyThrows", "ArgumentException; unchanged=True", () =>
            ThrowsKeepsSnapshot(_ => { }, s => s.WithParent(K(e => e.Id == 7 ? null : e.Id), Manager)));

        await Case("GroupByDept", "[H] Ada1- Ben2+ Cy2 [H] Eve1+ Gus1", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            src.GroupBy(K(e => e.Dept));
            var tv = await Place(src);
            await Toggle(tv, "Ada", true);
            return Snapshot(tv);
        });

        await Case("PositionInSetUngrouped", "Ada 1/3 Ben 1/2 Cy 2/2 Eve 2/3 Gus 3/3", async () =>
        {
            var tv = await Place(Src(Fixture(), Id, Manager));
            await Toggle(tv, "Ada", true);
            return Position(tv);
        });

        await Case("PositionInSetGrouped", "Ada 1/1 Eve 1/2 Gus 2/2", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            src.GroupBy(K(e => e.Dept));
            return Position(await Place(src));
        });

        await Case("ExpandCollapseAll", "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1 | " + Roots, async () =>
        {
            var tv = await Place(Src(Fixture(), Id, Manager));
            tv.ExpandAllRows();
            await Settle();
            var a = Snapshot(tv);
            tv.CollapseAllRows();
            await Settle();
            return a + " | " + Snapshot(tv);
        });

        await Case("ClearParentFlat", "Ada0 Ben0 Cy0 Dan0 Eve0 Fay0 Gus0", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            await Toggle(tv, "Ada", true);
            src.ClearParent();
            await Settle();
            return Snapshot(tv);
        });

        return _results;
    }
}
