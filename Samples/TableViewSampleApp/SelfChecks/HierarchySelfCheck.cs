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

using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.SelfChecks;

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

// Raises ONE Reset for whole-content replacement (unlike Clear on an empty collection).
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

// Row level: TableViewRow.Level, 1-based for hierarchical sources (roots = 1), 0 for flat.
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
        => TableViewSource.From(list).ParentBy(key, parent);

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
        src.ClearParentBy();
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
            src.ParentBy(Id, Manager);
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

        // App .NET enum crosses the ABI opaque, compared by reference: fresh boxes never link or
        // alias into a duplicate.
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
            ThrowsKeepsSnapshot(_ => { }, s => s.ParentBy(K(e => 1), Manager)));

        await Case("CycleThrows", "ArgumentException; unchanged=True", () =>
            ThrowsKeepsSnapshot(l => l[0].ManagerId = 4, s => s.ParentBy(Id, Manager)));

        await Case("SelfParentThrows", "ArgumentException; unchanged=True", () =>
            ThrowsKeepsSnapshot(l => l[0].ManagerId = 1, s => s.ParentBy(Id, Manager)));

        await Case("NullKeyThrows", "ArgumentException; unchanged=True", () =>
            ThrowsKeepsSnapshot(_ => { }, s => s.ParentBy(K(e => e.Id == 7 ? null : e.Id), Manager)));

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
            src.ClearParentBy();
            await Settle();
            return Snapshot(tv);
        });

        // Retracting the relation inside ExpandAllRows' Reset stops the in-flight publication safely.
        await Case("ReentrantClearParentDuringExpandAll", "fired; Ada0 Ben0 Cy0 Dan0 Eve0 Fay0 Gus0", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            var fired = false;
            var thrown = OnFirstPresentedChange(tv, () =>
            {
                fired = true;
                src.ClearParentBy();
            }, () => tv.ExpandAllRows());
            await Settle();
            return (fired ? "fired" : "not fired") + (thrown is null ? "" : " " + thrown) + "; " + Snapshot(tv);
        });

        // Adding a root in a NEW group during a grouped node toggle waits for the re-slice, then shows it.
        await Case("ReentrantAddNewGroupRootDuringExpand", "fired; [H] Ada1- Ben2+ Cy2 [H] Eve1+ Gus1 [H] Hal1", async () =>
        {
            var list = Fixture();
            var src = Src(list, Id, Manager);
            src.GroupBy(K(e => e.Dept));
            var tv = await Place(src);
            var fired = false;
            string? thrown = null;
            var repeater = RowsRepeater(tv);
            var view = repeater.ItemsSourceView;
            void Handler(object? sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs args)
            {
                if (fired)
                {
                    return;
                }

                fired = true;
                list.Add(new Emp { Id = 8, Name = "Hal", Dept = "Qa" });
            }

            view.CollectionChanged += Handler;
            try
            {
                await Toggle(tv, "Ada", true);
            }
            catch (Exception ex)
            {
                thrown = ex.GetType().Name;
            }
            finally
            {
                view.CollectionChanged -= Handler;
            }

            await Settle();
            return (fired ? "fired" : "not fired") + (thrown is null ? "" : " " + thrown) + "; " + Snapshot(tv);
        });

        // Key selector re-declares the relation on first call: obsolete build discarded, latest projected.
        await Case("SelectorRedeclaresWithParent", "no exception; " + FullTree, async () =>
        {
            var src = TableViewSource.From(Fixture());
            var once = false;
            var redeclaring = new TableViewKeySelector(o =>
            {
                if (!once)
                {
                    once = true;
                    src.ParentBy(Id, Manager);
                }

                return 1;
            });
            var thrown = Thrown(() => src.ParentBy(redeclaring, Manager));
            var tv = await Place(src);
            tv.ExpandAllRows();
            await Settle();
            return thrown + "; " + Snapshot(tv);
        });

        // GroupBy + ParentBy header order matches plain GroupBy for either verb order.
        await Case("GroupedPostSortKeepsHeaderOrder",
            "Zulu Alpha = Zulu Alpha | Alpha Zulu = Alpha Zulu", async () =>
        {
            static ObservableCollection<Emp> Pair() => new(new[]
            {
                new Emp { Id = 1, Dept = "A", Name = "Zulu" },
                new Emp { Id = 2, Dept = "B", Name = "Alpha" },
            });

            var byName = K(e => e.Name);
            var byDept = K(e => e.Dept);

            var hierPost = await Names(TableViewSource.From(Pair()).ParentBy(Id, Manager).GroupBy(byDept).Sort(byName, SortDirection.Ascending));
            var flatPost = await Names(TableViewSource.From(Pair()).GroupBy(byDept).Sort(byName, SortDirection.Ascending));
            var hierPre = await Names(TableViewSource.From(Pair()).ParentBy(Id, Manager).Sort(byName, SortDirection.Ascending).GroupBy(byDept));
            var flatPre = await Names(TableViewSource.From(Pair()).Sort(byName, SortDirection.Ascending).GroupBy(byDept));
            return $"{hierPost} = {flatPost} | {hierPre} = {flatPre}";
        });

        // CollapseAllRows under a filter persists: clearing the filter shows the tree collapsed.
        await Case("FilteredCollapseAllThenClear", Roots, async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            await Toggle(tv, "Ada", true);
            src.Filter(new TableViewPredicate(o => ((Emp)o).Name == "Dan"));
            await Settle();
            tv.CollapseAllRows();
            await Settle();
            src.ClearFilter();
            await Settle();
            return Snapshot(tv);
        });

        // ExpandCollapsePattern collapse restamps the row (IsExpanded, peer state) and removes children.
        await Case("ChevronCollapseRestamps", "Ada1- Ben2+ Cy2 Eve1+ Gus1 | Ada1+ Eve1+ Gus1; dp=False; peer=Collapsed", async () =>
        {
            var tv = await Place(Src(Fixture(), Id, Manager));
            await Toggle(tv, "Ada", true);
            var a = Snapshot(tv);
            await Toggle(tv, "Ada", false);
            var row = FindRow(tv, "Ada");
            return $"{a} | {Snapshot(tv)}; dp={row.IsExpanded}; peer={ExpandState(row)}";
        });

        await Case("SelectionSurvivesSameKeyReplace", "0 True True", async () =>
        {
            var list = Fixture();
            var tv = await Place(Src(list, Id, Manager));
            tv.Select(0);
            await Settle();
            var fresh = new Emp { Id = 1, Name = "Ada", Dept = "Eng" };
            list[0] = fresh;
            await Settle();
            return $"{tv.SelectedIndex} {ReferenceEquals(tv.SelectedItem, fresh)} {IsRowSelected(FindRow(tv, "Ada"))}";
        });

        await Case("SelectionSurvivesSameKeyReplaceGrouped", "1 True True", async () =>
        {
            var list = Fixture();
            var src = Src(list, Id, Manager);
            src.GroupBy(K(e => e.Dept));
            var tv = await Place(src);
            tv.Select(1);
            await Settle();
            var fresh = new Emp { Id = 1, Name = "Ada", Dept = "Eng" };
            list[0] = fresh;
            await Settle();
            return $"{tv.SelectedIndex} {ReferenceEquals(tv.SelectedItem, fresh)} {IsRowSelected(FindRow(tv, "Ada"))}";
        });

        // Small collapse clears selection; a later Reset must not restore it.
        await Case("CollapseDropsSelectionAnchor", "sel=Dan 1; collapsed=null 2; after=null 2", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            await Toggle(tv, "Ada", true);
            await Toggle(tv, "Ben", true);
            var changes = 0;
            tv.SelectionChanged += (s, e) => changes++;
            static string Name(object? item) => (item as Emp)?.Name ?? "null";

            tv.Select(Realized(tv).First(p => (p.Element as TableViewRow)?.DataContext is Emp { Name: "Dan" }).Index);
            await Settle();
            var sel = $"{Name(tv.SelectedItem)} {changes}";

            await Toggle(tv, "Ada", false);
            var collapsed = $"{Name(tv.SelectedItem)} {changes}";

            tv.ExpandAllRows();
            await Settle();
            src.Sort(K(e => e.Name), SortDirection.Ascending);
            await Settle();
            return $"sel={sel}; collapsed={collapsed}; after={Name(tv.SelectedItem)} {changes}";
        });

        // Matched parent with no matching children is a leaf, even after ExpandAllRows.
        await Case("FilterParentMatchExcludesUnmatchedChildren", "Ada1", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            src.Filter(new TableViewPredicate(o => ((Emp)o).Name == "Ada"));
            await Settle();
            tv.ExpandAllRows();
            await Settle();
            return Snapshot(tv);
        });

        await Case("FilterMatchesNothing", "", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            src.Filter(new TableViewPredicate(o => false));
            await Settle();
            return Snapshot(tv);
        });

        // Context-row collapse is discarded on filter change; new context ancestors come back expanded.
        await Case("FilterReplaceAfterContextCollapse", "Ada1+ | Ada1- Cy2", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            await Toggle(tv, "Ada", true);
            src.Filter(new TableViewPredicate(o => ((Emp)o).Name == "Dan"));
            await Settle();
            await Toggle(tv, "Ada", false);
            var a = Snapshot(tv);
            src.Filter(new TableViewPredicate(o => ((Emp)o).Name == "Cy"));
            await Settle();
            return a + " | " + Snapshot(tv);
        });

        // GroupBy buckets roots only (Ben stays under Ada's Eng group); header counts are root counts.
        await Case("GroupedRootsOnlyMembership", "[Eng 1] Ada1- Ben2+ Cy2 [Ops 2] Eve1+ Gus1", async () =>
        {
            var list = Fixture();
            list[1].Dept = "Ops";
            var src = Src(list, Id, Manager);
            src.GroupBy(K(e => e.Dept));
            var tv = await Place(src);
            await Toggle(tv, "Ada", true);
            return HeaderSnapshot(tv);
        });

        await Case("RegroupExpandedTree", "[Eng 1] Ada1- Ben2- Dan3 Cy2 [Ops 2] Eve1+ Gus1 | Ada1- Ben2- Dan3 Cy2 Eve1+ Gus1", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            await Toggle(tv, "Ada", true);
            await Toggle(tv, "Ben", true);
            src.GroupBy(K(e => e.Dept));
            await Settle();
            var a = HeaderSnapshot(tv);
            src.ClearGroupBy();
            await Settle();
            return a + " | " + Snapshot(tv);
        });

        await Case("UiaLevelAndLeaf", "Ada:1:Expanded Ben:2:Collapsed Cy:2:LeafNode Gus:1:LeafNode | Ada:none", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            await Toggle(tv, "Ada", true);
            string P(string name)
            {
                var row = FindRow(tv, name);
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(row);
                return $"{name}:{peer.GetLevel()}:{ExpandState(row)}";
            }

            var a = string.Join(" ", new[] { "Ada", "Ben", "Cy", "Gus" }.Select(P));
            src.ClearParentBy();
            await Settle();
            return a + " | Ada:" + ExpandState(FindRow(tv, "Ada"));
        });

        // A no-op must not end an open edit (kept open via a one-shot CellEditEnding veto).
        await Case("BulkOnFlatDoesNotEndEdit", "editing=True kept=True endings=0", async () =>
        {
            var src = TableViewSource.From(Fixture()).Sort(K(e => e.Name), SortDirection.Ascending);
            var tv = new TableView { IsReadOnly = false };
            tv.Columns.Add(new TableViewTextColumn
            {
                Header = "Name",
                Binding = new Microsoft.UI.Xaml.Data.Binding { Path = new PropertyPath(nameof(Emp.Name)), Mode = Microsoft.UI.Xaml.Data.BindingMode.TwoWay },
                Width = SampleColumns.Star(),
            });
            tv.ItemsSource = src;
            var veto = true;
            var endings = 0;
            tv.CellEditEnding += (s, e) =>
            {
                endings++;
                if (veto)
                {
                    e.Cancel = true;
                }
            };
            _host.Child = tv;
            await Settle();

            var rowPeer = FrameworkElementAutomationPeer.CreatePeerForElement(FindRow(tv, "Ada"));
            var cellPeer = rowPeer.GetChildren().FirstOrDefault(p => p.GetPattern(PatternInterface.Value) is not null);
            if (cellPeer is null)
            {
                return "no editable cell peer";
            }

            try
            {
                ((IValueProvider)cellPeer.GetPattern(PatternInterface.Value)).SetValue("Zed");
            }
            catch (Exception)
            {
                // Expected: the vetoed commit reports failure and leaves the editor open.
            }

            veto = false;
            var editing = tv.IsEditing;
            endings = 0;
            tv.ExpandAllRows();
            tv.CollapseAllRows();
            tv.ExpandAllGroups();
            tv.CollapseAllGroups();
            var kept = tv.IsEditing;
            var observed = endings;
            tv.CancelEdit();
            await Settle();
            return $"editing={editing} kept={kept} endings={observed}";
        });

        // Deep row, narrow lead column: chevron clipped to the lead cell, and not hit-testable past it.
        await Case("ChevronClippedToLeadCell", "Ada ok; Ben ok; Dan ok hit=False; Cy ok", async () =>
        {
            var tv = NewTable(Src(Fixture(), Id, Manager));
            tv.Columns[0].Width = SampleColumns.Pixels(30);
            tv.Columns.Add(SampleColumns.Text("Dept", nameof(Emp.Dept), SampleColumns.Star()));
            _host.Child = tv;
            await Settle();
            tv.ExpandAllRows();
            await Settle();

            var parts = new List<string>();
            foreach (var name in new[] { "Ada", "Ben", "Dan", "Cy" })
            {
                var row = FindRow(tv, name);
                var gutter = FindDescendant<Border>(row, "PART_RowExpanderGutter");
                if (gutter is null)
                {
                    parts.Add($"{name} no gutter");
                    continue;
                }

                var expected = Math.Max(0.0, tv.Columns[0].ActualWidth - gutter.Margin.Left);
                var clip = gutter.Clip?.Rect;
                var ok = clip is { } r && Math.Abs(r.Width - expected) < 0.5 && gutter.IsHitTestVisible == expected > 0;
                parts.Add(name + (ok ? " ok" : $" clip={clip?.Width.ToString() ?? "null"} expected={expected}") +
                    (name == "Dan" ? $" hit={gutter.IsHitTestVisible}" : ""));
            }

            return string.Join("; ", parts);
        });

        // ClearParentBy inside an engine-refresh Reset: outer rebuild publishes coherently, then flat.
        await Case("ReentrantClearParentDuringRefreshAdd", "fired; Ada0 Ben0 Cy0 Dan0 Eve0 Fay0 Gus0 Hal0", async () =>
        {
            var list = Fixture();
            var src = Src(list, Id, Manager);
            var tv = await Place(src);
            var fired = false;
            var thrown = OnFirstPresentedChange(tv, () =>
            {
                fired = true;
                src.ClearParentBy();
            }, () => list.Add(new Emp { Id = 8, Name = "Hal", Dept = "Ops" }));
            await Settle();
            return (fired ? "fired" : "not fired") + (thrown is null ? "" : " " + thrown) + "; " + Snapshot(tv);
        });

        await Case("ReentrantClearParentDuringRefreshFilter", "fired; Ada0 Ben0 Dan0 Eve0 Fay0 Gus0", async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            var fired = false;
            var thrown = OnFirstPresentedChange(tv, () =>
            {
                fired = true;
                src.ClearParentBy();
            }, () => src.Filter(new TableViewPredicate(o => ((Emp)o).Name != "Cy")));
            await Settle();
            return (fired ? "fired" : "not fired") + (thrown is null ? "" : " " + thrown) + "; " + Snapshot(tv);
        });

        // Selector fixes a duplicate key mid-pass: that pass still fails, the queued refresh projects
        // the fixed data.
        await Case("FixDuringFailingPassIsReplayed", "ArgumentException; " + Roots, async () =>
        {
            var list = Fixture();
            var dup = new Emp { Id = 1, Name = "Dup", Dept = "Eng" };
            list.Add(dup);
            var src = TableViewSource.From(list);
            var fixedOnce = false;
            var key = new TableViewKeySelector(o =>
            {
                if (!fixedOnce)
                {
                    fixedOnce = true;
                    list.Remove(dup);
                }

                return ((Emp)o).Id;
            });
            var thrown = Thrown(() => src.ParentBy(key, Manager));
            var tv = await Place(src);
            return thrown + "; " + Snapshot(tv);
        });

        // ClearParentBy + duplicate Add inside a Filter Reset: flat refresh fails, yet rows end flat
        // and inert; removing the duplicate re-shapes flat.
        await Case("ClearParentTeardownSurvivesFailingRefresh",
            "fired ArgumentException; Ada0 Eve0 Gus0 | uia: none none none | again: Ada0 Eve0 Gus0 | fixed: Ada0 Ben0 Cy0 Dan0 Eve0 Fay0 Gus0",
            async () =>
        {
            var list = Fixture();
            var src = Src(list, Id, Manager);
            var tv = await Place(src);
            var fired = false;
            var thrown = OnFirstPresentedChange(tv, () =>
            {
                fired = true;
                src.ClearParentBy();
                list.Add(list[0]);
            }, () => src.Filter(new TableViewPredicate(_ => true)));
            await Settle();
            var after = Snapshot(tv);
            var uia = string.Join(" ", Realized(tv).Select(p => p.Element).OfType<TableViewRow>().Select(ExpandState));

            src.ClearParentBy();
            tv.ExpandAllRows();
            await Settle();
            var again = Snapshot(tv);

            list.RemoveAt(list.Count - 1);
            await Settle();
            return (fired ? "fired" : "not fired") + (thrown is null ? "" : " " + thrown) + "; " + after
                + " | uia: " + uia + " | again: " + again + " | fixed: " + Snapshot(tv);
        });

        // Grouped: group keys valid only on roots break the flat rebuild after ClearParentBy; rows
        // still end flat and inert, and fixing the keys groups them flat.
        await Case("ClearParentTeardownSurvivesFailingGroupedRefresh",
            "fired ArgumentException; [H] Ada0 [H] Eve0 Gus0 | uia: none none none | again: [H] Ada0 [H] Eve0 Gus0 | fixed: [H] Ada0 Ben0 Cy0 Dan0 [H] Eve0 Fay0 Gus0",
            async () =>
        {
            var list = Fixture();
            var src = Src(list, Id, Manager);
            var badDescendantKeys = true;
            src.GroupBy(K(e => badDescendantKeys && e.Name is "Ben" or "Cy" or "Dan" or "Fay" ? new object() : e.Dept));
            var tv = await Place(src);
            var fired = false;
            var thrown = OnFirstPresentedChange(tv, () =>
            {
                fired = true;
                src.ClearParentBy();
            }, () => src.Filter(new TableViewPredicate(_ => true)));
            await Settle();
            var after = Snapshot(tv);
            var uia = string.Join(" ", Realized(tv).Select(p => p.Element).OfType<TableViewRow>().Select(ExpandState));

            src.ClearParentBy();
            tv.ExpandAllRows();
            await Settle();
            var again = Snapshot(tv);

            badDescendantKeys = false;
            src.Filter(new TableViewPredicate(_ => true));
            await Settle();
            return (fired ? "fired" : "not fired") + (thrown is null ? "" : " " + thrown) + "; " + after
                + " | uia: " + uia + " | again: " + again + " | fixed: " + Snapshot(tv);
        });

        // Flat publication throws (via a SelectionChanged handler): teardown isn't done until a flat
        // publication completes; rows end flat and inert.
        await Case("ClearParentTeardownSurvivesFailingPublication",
            "before: Ada1- Ben2- Dan3; fired InvalidOperationException; Dan0 | uia: none | again: no exception Dan0 | fixed: Ada0 Ben0 Cy0 Dan0 Eve0 Fay0 Gus0",
            async () =>
        {
            var src = Src(Fixture(), Id, Manager);
            var tv = await Place(src);
            src.Filter(new TableViewPredicate(o => ((Emp)o).Name == "Dan"));
            await Settle();
            var before = Snapshot(tv);
            tv.Select(0);
            await Settle();

            var fired = false;
            void OnSelectionChanged(object sender, SelectionChangedEventArgs e)
            {
                tv.SelectionChanged -= OnSelectionChanged;
                fired = true;
                throw new InvalidOperationException("publication handler");
            }

            tv.SelectionChanged += OnSelectionChanged;
            var thrown = Thrown(() => src.ClearParentBy());
            tv.SelectionChanged -= OnSelectionChanged;
            await Settle();
            var after = Snapshot(tv);
            var uia = string.Join(" ", Realized(tv).Select(p => p.Element).OfType<TableViewRow>().Select(ExpandState));

            var again = Thrown(() => src.ClearParentBy());
            tv.ExpandAllRows();
            await Settle();
            again += " " + Snapshot(tv);

            src.ClearFilter();
            await Settle();
            return "before: " + before + "; " + (fired ? "fired " : "not fired ") + thrown + "; " + after
                + " | uia: " + uia + " | again: " + again + " | fixed: " + Snapshot(tv);
        });

        // Re-entrant ClearParentBy from each teardown publication is deferred (no recursion); rows end
        // flat and inert.
        await Case("ClearParentReentrantDuringCompletion",
            "fired ArgumentException; Ada0 Eve0 Gus0 | uia: none none none | calls: 1 | again: Ada0 Eve0 Gus0 calls: 1 | fixed: Ada0 Ben0 Cy0 Dan0 Eve0 Fay0 Gus0",
            async () =>
        {
            var list = Fixture();
            var src = Src(list, Id, Manager);
            var tv = await Place(src);
            var repeater = RowsRepeater(tv);
            var calls = 0;
            var token = repeater.RegisterPropertyChangedCallback(ItemsRepeater.ItemsSourceProperty, (_, _) =>
            {
                if (++calls > 50)
                {
                    return;
                }

                src.ClearParentBy();
            });

            string? thrown;
            try
            {
                var fired = false;
                thrown = OnFirstPresentedChange(tv, () =>
                {
                    fired = true;
                    src.ClearParentBy();
                    list.Add(list[0]);
                }, () => src.Filter(new TableViewPredicate(_ => true)));
                thrown = (fired ? "fired" : "not fired") + (thrown is null ? "" : " " + thrown);
                await Settle();
                var after = Snapshot(tv);
                var uia = string.Join(" ", Realized(tv).Select(p => p.Element).OfType<TableViewRow>().Select(ExpandState));
                var afterCalls = calls;

                src.ClearParentBy();
                tv.ExpandAllRows();
                await Settle();
                var again = Snapshot(tv) + " calls: " + calls;

                list.RemoveAt(list.Count - 1);
                await Settle();
                return thrown + "; " + after + " | uia: " + uia + " | calls: " + afterCalls
                    + " | again: " + again + " | fixed: " + Snapshot(tv);
            }
            finally
            {
                repeater.UnregisterPropertyChangedCallback(ItemsRepeater.ItemsSourceProperty, token);
            }
        });

        // Clear + re-declare during a failed teardown's fallback publication: the deferred refresh is
        // posted, not stranded, so the hierarchy returns after a tick.
        await Case("DeferredRequestSurvivesFailingIncrementalChange",
            "callback: 1; count: 8; Ada1+ Eve1+ Gus1 Hal1",
            async () =>
        {
            var list = Fixture();
            var src = Src(list, Id, Manager);
            // A verb in force, or the unshaped flat mirror would tolerate the duplicate.
            src.Filter(new TableViewPredicate(_ => true));
            var tv = await Place(src);
            var repeater = RowsRepeater(tv);
            var callbacks = 0;
            var armed = false;
            var token = repeater.RegisterPropertyChangedCallback(ItemsRepeater.ItemsSourceProperty, (_, _) =>
            {
                if (!armed)
                {
                    return;
                }

                armed = false;
                ++callbacks;
                list.RemoveAt(list.Count - 1);
                src.ParentBy(Id, Manager);
            });

            try
            {
                _ = OnFirstPresentedChange(tv, () =>
                {
                    armed = true;
                    src.ClearParentBy();
                    list.Add(list[0]);
                }, () => list.Add(new Emp { Id = 8, Name = "Hal", Dept = "Ops" }));
                await Settle();
                // Engine error doesn't surface through list.Add; assert outcome only.
                return "callback: " + callbacks + "; count: " + list.Count + "; " + Snapshot(tv);
            }
            finally
            {
                repeater.UnregisterPropertyChangedCallback(ItemsRepeater.ItemsSourceProperty, token);
            }
        });

        // All columns hidden: chevron clipped, not hit-testable. Showing restores; widening grows clip.
        await Case("ChevronClipTracksColumns", "hidden: w=0 hit=False | shown: ok | widened: ok", async () =>
        {
            var tv = NewTable(Src(Fixture(), Id, Manager));
            tv.Columns[0].Width = SampleColumns.Pixels(30);
            tv.Columns.Add(SampleColumns.Text("Dept", nameof(Emp.Dept), SampleColumns.Star()));
            _host.Child = tv;
            await Settle();
            tv.ExpandAllRows();
            await Settle();

            string Check(string name)
            {
                var g = Chevron(tv, name);
                var expected = Math.Max(0.0, tv.Columns[0].ActualWidth - g.Margin.Left);
                var clip = g.Clip?.Rect;
                return clip is { } r && Math.Abs(r.Width - expected) < 0.5 && g.IsHitTestVisible == expected > 0
                    ? "ok"
                    : $"{name} clip={clip?.Width.ToString() ?? "null"} expected={expected} hit={g.IsHitTestVisible}";
            }

            foreach (var c in tv.Columns)
            {
                c.Visibility = Visibility.Collapsed;
            }

            await Settle();
            var ada = Chevron(tv, "Ada");
            var hidden = $"w={ada.Clip?.Rect.Width.ToString() ?? "null"} hit={ada.IsHitTestVisible}";

            tv.Columns[0].Visibility = Visibility.Visible;
            await Settle();
            var shown = Check("Ada");

            tv.Columns[0].Width = SampleColumns.Pixels(200);
            await Settle();
            var widened = Check("Dan") == "ok" && Check("Ben") == "ok" && Chevron(tv, "Dan").IsHitTestVisible ? "ok" : Check("Dan");
            return $"hidden: {hidden} | shown: {shown} | widened: {widened}";
        });

        // The real chevron gesture: an injected left press on PART_RowExpanderGutter.
        await Case("ChevronPointerCollapseRestamps", "Ada1- Ben2+ Cy2 Eve1+ Gus1 | Ada1+ Eve1+ Gus1; dp=False; peer=Collapsed", async () =>
        {
            var tv = await Place(Src(Fixture(), Id, Manager));
            var failed = await PressAsync(Chevron(tv, "Ada"), 1);
            if (failed is not null)
            {
                return failed;
            }

            var a = Snapshot(tv);

            // Past the double-click window: this case is about the toggle, not a double press.
            await Task.Delay(TimeSpan.FromMilliseconds(GetDoubleClickTime() + 100));
            failed = await PressAsync(Chevron(tv, "Ada"), 1);
            if (failed is not null)
            {
                return failed;
            }

            var row = FindRow(tv, "Ada");
            return $"{a} | {Snapshot(tv)}; dp={row.IsExpanded}; peer={ExpandState(row)}";
        });

        // Two quick chevron presses are two toggles, not a double-click; a plain-cell control proves
        // injected presses can begin an edit.
        await Case("ChevronDoublePressNoEdit", "chevron: beginning=0 editing=False Ada1+ | cell: beginning=1", async () =>
        {
            var tv = new TableView { IsReadOnly = false };
            foreach (var (header, path) in new[] { ("Name", nameof(Emp.Name)), ("Dept", nameof(Emp.Dept)) })
            {
                tv.Columns.Add(new TableViewTextColumn
                {
                    Header = header,
                    Binding = new Microsoft.UI.Xaml.Data.Binding { Path = new PropertyPath(path), Mode = Microsoft.UI.Xaml.Data.BindingMode.TwoWay },
                    Width = SampleColumns.Star(),
                });
            }

            tv.ItemsSource = Src(Fixture(), Id, Manager);
            var beginning = 0;
            tv.BeginningEdit += (s, e) => beginning++;
            _host.Child = tv;
            await Settle();

            var failed = await PressAsync(Chevron(tv, "Ada"), 2);
            if (failed is not null)
            {
                return failed;
            }

            var chevron = $"beginning={beginning} editing={tv.IsEditing} {RowText(FindRow(tv, "Ada"))}";
            beginning = 0;

            // Past the double-click window, so the control's first press is not paired with the above.
            await Task.Delay(TimeSpan.FromMilliseconds(GetDoubleClickTime() + 100));
            var eve = FindRow(tv, "Eve");
            failed = await PressAsync(eve, 2, new Windows.Foundation.Point(eve.ActualWidth * 0.75, eve.ActualHeight / 2));
            if (failed is not null)
            {
                return failed;
            }

            var control = $"beginning={beginning}";
            tv.CancelEdit();
            await Settle();
            return $"chevron: {chevron} | cell: {control}";
        });

        // ClearParentBy inside a sort key selector: pass dropped as obsolete and replayed flat.
        await Case("ClearParentFromSortSelector", "no exception | Ada0 Ben0 Cy0 Dan0 Eve0 Fay0 Gus0", async () =>
        {
            TableViewSource? src = null;
            var armed = false;
            src = Src(Fixture(), Id, Manager).Sort(K(e =>
            {
                if (armed)
                {
                    armed = false;
                    src!.ClearParentBy();
                }

                return e.Name;
            }), SortDirection.Ascending);
            var tv = await Place(src);
            armed = true;
            var thrown = Thrown(() => src.Filter(new TableViewPredicate(_ => true)));
            await Settle();
            return $"{thrown} | {Snapshot(tv)}";
        });

        // Tree keys need no modifiers: Shift+Right drills to cell, Ctrl+Left doesn't collapse, Right
        // expands. (Shift probe on Eve leaves focus in her cell.)
        await Case("TreeKeysIgnoreModifiers", "Ada1+ Eve1+ Gus1 | Ada1- Ben2+ Cy2 Eve1+ Gus1 | Ada1- Ben2+ Cy2 Eve1+ Gus1", async () =>
        {
            var tv = await Place(Src(Fixture(), Id, Manager));
            var failed = await KeysAsync(FindRow(tv, "Eve"), Windows.System.VirtualKey.Right, Windows.System.VirtualKey.Shift);
            if (failed is not null)
            {
                return failed;
            }

            var shifted = Snapshot(tv);
            failed = await KeysAsync(FindRow(tv, "Ada"), Windows.System.VirtualKey.Right);
            if (failed is not null)
            {
                return failed;
            }

            var plain = Snapshot(tv);
            failed = await KeysAsync(FindRow(tv, "Ada"), Windows.System.VirtualKey.Left, Windows.System.VirtualKey.Control);
            if (failed is not null)
            {
                return failed;
            }

            return $"{shifted} | {plain} | {Snapshot(tv)}";
        });

        // Left on a leaf child selects the parent; Right on an expanded parent drills into its first cell.
        await Case("TreeMovesSelect", "Ada 0 | none 0", async () =>
        {
            var tv = await Place(Src(Fixture(), Id, Manager));
            await Toggle(tv, "Ada", true);
            var failed = await KeysAsync(FindRow(tv, "Ben"), Windows.System.VirtualKey.Left);
            if (failed is not null)
            {
                return failed;
            }

            var toParent = $"{FocusedRowName(tv)} {tv.SelectedIndex}";
            failed = await KeysAsync(FindRow(tv, "Ada"), Windows.System.VirtualKey.Right);
            if (failed is not null)
            {
                return failed;
            }

            return $"{toParent} | {FocusedRowName(tv)} {tv.SelectedIndex}";
        });

        return _results;
    }

    private static string FocusedRowName(TableView tv) =>
        (Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(tv.XamlRoot) as TableViewRow)?.DataContext is Emp e
            ? e.Name
            : "none";

    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern IntPtr GetForegroundWindow();

    // Keyboard-focuses `row`, then injects `key` (with `modifier` held, if any). Refuses (returns a
    // reason) unless this app's window is in the foreground, so keys never land in another app.
    private async Task<string?> KeysAsync(TableViewRow row, Windows.System.VirtualKey key, Windows.System.VirtualKey? modifier = null)
    {
        var hwnd = Microsoft.UI.Win32Interop.GetWindowFromWindowId(row.XamlRoot.ContentIslandEnvironment.AppWindowId);
        SetForegroundWindow(hwnd);
        await Settle();
        if (GetForegroundWindow() != hwnd)
        {
            return "SKIPPED window not foreground";
        }

        if (!row.Focus(FocusState.Keyboard))
        {
            return "SKIPPED row not focusable";
        }

        await Settle();
        if (Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(row.XamlRoot) != row)
        {
            return "focus not on row: " + Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(row.XamlRoot)?.GetType().Name;
        }

        var injector = Windows.UI.Input.Preview.Injection.InputInjector.TryCreate();
        if (injector is null)
        {
            return "SKIPPED no InputInjector";
        }

        const Windows.UI.Input.Preview.Injection.InjectedInputKeyOptions up =
            Windows.UI.Input.Preview.Injection.InjectedInputKeyOptions.KeyUp;
        const Windows.UI.Input.Preview.Injection.InjectedInputKeyOptions extended =
            Windows.UI.Input.Preview.Injection.InjectedInputKeyOptions.ExtendedKey;
        var infos = new List<Windows.UI.Input.Preview.Injection.InjectedInputKeyboardInfo>();
        if (modifier is { } m)
        {
            infos.Add(new() { VirtualKey = (ushort)m });
        }

        infos.Add(new() { VirtualKey = (ushort)key, KeyOptions = extended });
        infos.Add(new() { VirtualKey = (ushort)key, KeyOptions = extended | up });
        if (modifier is { } m2)
        {
            infos.Add(new() { VirtualKey = (ushort)m2, KeyOptions = up });
        }

        injector.InjectKeyboardInput(infos);
        await Settle();
        return null;
    }

    private static Border Chevron(TableView tv, string name) =>
        FindDescendant<Border>(FindRow(tv, name), "PART_RowExpanderGutter")
            ?? throw new InvalidOperationException("no gutter");

    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    private struct NativePoint
    {
        public int X;
        public int Y;
    }

    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern bool ClientToScreen(IntPtr hwnd, ref NativePoint point);

    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern bool SetCursorPos(int x, int y);

    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern IntPtr WindowFromPoint(NativePoint point);

    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern IntPtr GetAncestor(IntPtr hwnd, uint flags);

    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern bool SetForegroundWindow(IntPtr hwnd);

    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern uint GetDoubleClickTime();

    // Injects left presses at `local` (default: centre). Refuses unless this window is under the
    // point. Null on success, else a reason.
    private async Task<string?> PressAsync(FrameworkElement target, int count, Windows.Foundation.Point? local = null)
    {
        var root = target.XamlRoot;
        var hwnd = Microsoft.UI.Win32Interop.GetWindowFromWindowId(root.ContentIslandEnvironment.AppWindowId);
        var p = target.TransformToVisual(null).TransformPoint(
            local ?? new Windows.Foundation.Point(target.ActualWidth / 2, target.ActualHeight / 2));
        var scale = root.RasterizationScale;
        var pt = new NativePoint { X = (int)Math.Round(p.X * scale), Y = (int)Math.Round(p.Y * scale) };
        ClientToScreen(hwnd, ref pt);

        SetForegroundWindow(hwnd);
        await Settle();
        const uint GA_ROOT = 2;
        if (GetAncestor(WindowFromPoint(pt), GA_ROOT) != hwnd)
        {
            return "SKIPPED window not under point";
        }

        var injector = Windows.UI.Input.Preview.Injection.InputInjector.TryCreate();
        if (injector is null)
        {
            return "SKIPPED no InputInjector";
        }

        SetCursorPos(pt.X, pt.Y);
        await Task.Delay(50);
        var infos = new List<Windows.UI.Input.Preview.Injection.InjectedInputMouseInfo>();
        for (int i = 0; i < count; i++)
        {
            infos.Add(new() { MouseOptions = Windows.UI.Input.Preview.Injection.InjectedInputMouseOptions.LeftDown });
            infos.Add(new() { MouseOptions = Windows.UI.Input.Preview.Injection.InjectedInputMouseOptions.LeftUp });
        }

        injector.InjectMouseInput(infos);
        await Settle();
        return null;
    }

    private static string ExpandState(TableViewRow row)
    {
        var peer = FrameworkElementAutomationPeer.CreatePeerForElement(row);
        return peer.GetPattern(PatternInterface.ExpandCollapse) is IExpandCollapseProvider p
            ? p.ExpandCollapseState.ToString()
            : "none";
    }

    private static bool IsRowSelected(TableViewRow row)
    {
        var peer = FrameworkElementAutomationPeer.CreatePeerForElement(row);
        return peer.GetPattern(PatternInterface.SelectionItem) is ISelectionItemProvider p && p.IsSelected;
    }

    // Snapshot with headers rendered as [Key ItemCount].
    private static string HeaderSnapshot(TableView tv) =>
        string.Join(" ", Realized(tv).Select(p => p.Element switch
        {
            TableViewRow r => RowText(r),
            TableViewGroupHeader h when h.Content is TableViewGroupInfo g => $"[{g.Key} {g.ItemCount}]",
            _ => "[H]",
        }));

    private static ItemsRepeater RowsRepeater(TableView tv) =>
        FindDescendant<ItemsRepeater>(tv, "PART_RowsRepeater")
            ?? throw new InvalidOperationException("PART_RowsRepeater not found");

    // Runs `act` with a one-shot handler on the presented row view's CollectionChanged; returns
    // the exception type name if `act` threw.
    private static string? OnFirstPresentedChange(TableView tv, Action onFirst, Action act)
    {
        var view = RowsRepeater(tv).ItemsSourceView;
        var fired = false;
        void Handler(object? sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs args)
        {
            if (fired)
            {
                return;
            }

            fired = true;
            onFirst();
        }

        view.CollectionChanged += Handler;
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
            view.CollectionChanged -= Handler;
        }
    }

    // Data-row names in presented order, headers skipped.
    private async Task<string> Names(TableViewSource source)
    {
        var tv = await Place(source);
        return string.Join(" ", Realized(tv).Select(p => p.Element).OfType<TableViewRow>()
            .Select(r => (r.DataContext as Emp)?.Name ?? "?"));
    }
}
