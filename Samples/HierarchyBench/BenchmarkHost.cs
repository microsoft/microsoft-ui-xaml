// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;

namespace HierarchyBench;

// One hierarchical row. Name/Id/Group/Value are the shaped fields; Cells pads the row to --cols.
public sealed class BenchNode
{
    public int Id { get; set; }
    public int? ParentId { get; set; }
    public string Name { get; set; } = string.Empty;
    public string Group { get; set; } = string.Empty;
    public int Value { get; set; }
    public object[] Cells { get; set; } = Array.Empty<object>();

    // Only used by the children-selector API: every node owns a collection (empty for a leaf).
    public ObservableCollection<BenchNode>? Children { get; set; }
}

// TableView hierarchy benchmark host, driven by the external perf harness (run-all.ps1 ->
// sampler.py), one process per matrix cell. Mirrors the protocol of the harness's TableView v1
// bench: CLI options, a ready-file on the first composition frame, an L2 metrics CSV, then exit.
//
// The tree is declared before first render (collapsed) for every scenario except "declare".
// Scenarios ending in "-x", and "collapseall", start with every row expanded (ExpandAllRows in
// Loaded, so the expansion is part of first render). Each timed operation is measured from the
// call to the fully rendered result: op + UpdateLayout + 2 composition frames (the harness rule).
public sealed class BenchmarkHost : Grid
{
#if HIERARCHY_API_CHILDREN
    public const string Api = "children";
#elif HIERARCHY_API_PARENTKEY2
    public const string Api = "combined";
#else
    public const string Api = "keyby";
#endif

    public static readonly string[] Scenarios =
    {
        "load", "load-x", "scroll-x", "resize-x", "select-x",
        "declare", "expandall", "collapseall", "expand1",
        "sort", "sort-x", "filter", "filter-x", "group", "group-x", "edit-x", "value-x",
    };

    private const int GroupCount = 8;
    private const int FixedColumns = 4;
    private const int RenderTimeoutMs = 5000;

    private readonly BenchmarkOptions _options;
    private readonly Stopwatch _swProcess = Stopwatch.StartNew();
    private readonly List<double> _frameIntervals = new();
    private readonly string _scenario;
    private readonly bool _startExpanded;
    private readonly bool _declareUpFront;

    private List<BenchNode> _nodes = new();
    private ObservableCollection<BenchNode> _items = new();
    private ObservableCollection<BenchNode> _roots = new();
    private TableViewSource _source = null!;
    // Baseline modes, picked by --control name: "tree" (default), "flat" (*Flat: TableView, no
    // hierarchy), "listview"/"itemsview" (ListViewFlat/ItemsViewFlat: the harness's flat list
    // controls with its row template, sort/filter by repopulating the collection).
    private readonly string _mode;
    private readonly TableView? _table;
    private readonly FrameworkElement _control;
    private ItemsRepeater? _repeater;

    private double _creationMs, _dataGenMs, _controlCreationMs, _firstRenderMs;
    private double _setupExpandMs = double.NaN, _opMs = double.NaN, _scenarioMs = double.NaN, _interactiveMs = double.NaN;
    private double _opSyncMs = double.NaN, _layoutMs = double.NaN, _renderMs = double.NaN;
    private int _rowsBefore = -1, _rowsAfter = -1;
    private string? _error;
    private DateTime _lastFrame;
    private bool _capturing, _firstRenderSeen;

    public BenchmarkHost(BenchmarkOptions options)
    {
        _options = options;
        _scenario = options.Scenario.ToLowerInvariant();
        _mode = options.Control.StartsWith("ListView", StringComparison.OrdinalIgnoreCase) ? "listview"
            : options.Control.StartsWith("ItemsView", StringComparison.OrdinalIgnoreCase) ? "itemsview"
            : options.Control.EndsWith("Flat", StringComparison.OrdinalIgnoreCase) ? "flat"
            : "tree";
        _startExpanded = _mode == "tree" && (_scenario.EndsWith("-x", StringComparison.Ordinal) || _scenario == "collapseall");
        _declareUpFront = _mode == "tree" && _scenario != "declare";
        HorizontalAlignment = HorizontalAlignment.Stretch;
        VerticalAlignment = VerticalAlignment.Stretch;
        Loaded += OnLoaded;
        Log($"start api={Api} control={options.Control} scenario={options.Scenario} shape={options.Shape} rows={options.Rows} cols={options.Cols} settle={options.SettleMs}");

        var swData = Stopwatch.StartNew();
        Generate(options.Shape, options.Rows, options.Cols);
        swData.Stop();
        _dataGenMs = swData.Elapsed.TotalMilliseconds;

        var swCtl = Stopwatch.StartNew();
        if (_mode == "listview")
        {
            _control = BuildListView();
        }
        else if (_mode == "itemsview")
        {
            _control = BuildItemsView();
        }
        else
        {
            _table = BuildTableView();
            _control = _table;
        }

        Children.Add(_control);
        swCtl.Stop();
        _controlCreationMs = swCtl.Elapsed.TotalMilliseconds;
        _creationMs = _dataGenMs + _controlCreationMs;
    }

    // ---- Data ----

    // wide:     10 roots, every other row a direct child of one of them (one level, big sibling sets).
    // deep:     chains of 100 (each row the child of the previous one).
    // balanced: one root, fan-out 4.
    // shallow:  90% roots; every tenth row (index % 10 == 1) is the child of the row before it.
    private static int? ParentIndex(string shape, int i) => shape switch
    {
        "wide" => i < 10 ? null : i % 10,
        "deep" => i % 100 == 0 ? null : i - 1,
        "balanced" => i == 0 ? null : (i - 1) / 4,
        "shallow" => i % 10 == 1 ? i - 1 : null,
        _ => throw new ArgumentException("unknown shape " + shape),
    };

    private void Generate(string shape, int n, int cols)
    {
        string[] firsts = { "Ada", "Grace", "Alan", "Linus", "Margaret", "Donald", "Barbara", "Ken", "Edsger", "Katherine" };
        var rnd = new Random(12345);
        int extra = Math.Max(0, cols - FixedColumns);
        _nodes = new List<BenchNode>(n);
        for (int i = 0; i < n; i++)
        {
            var cells = new object[extra];
            for (int c = 0; c < extra; c++)
            {
                cells[c] = (c % 3) switch
                {
                    0 => firsts[rnd.Next(firsts.Length)] + "-" + i.ToString(CultureInfo.InvariantCulture),
                    1 => rnd.Next(0, 100000),
                    _ => Math.Round(rnd.NextDouble() * 10000, 2),
                };
            }

            _nodes.Add(new BenchNode
            {
                Id = i + 1,
                ParentId = ParentIndex(shape, i) + 1,
                // Scrambled but deterministic, so Sort really reorders siblings.
                Name = "n" + ((uint)(i * 2654435761u) % 1_000_000).ToString("D6", CultureInfo.InvariantCulture),
                Group = "g" + (i % GroupCount),
                Value = (int)((uint)(i * 40503u) % 100),
                Cells = cells,
            });
        }

#if HIERARCHY_API_CHILDREN
        foreach (var node in _nodes)
        {
            node.Children = new ObservableCollection<BenchNode>();
        }

        foreach (var node in _nodes)
        {
            if (node.ParentId is int parentId)
            {
                _nodes[parentId - 1].Children!.Add(node);
            }
        }

        _roots = new ObservableCollection<BenchNode>(_nodes.Where(x => x.ParentId is null));
        if (_mode != "tree")
        {
            _items = new ObservableCollection<BenchNode>(_nodes);
        }
#else
        _items = new ObservableCollection<BenchNode>(_nodes);
#endif
    }

    private BenchNode NewNode(int id, BenchNode? parent) => new()
    {
        Id = id,
        ParentId = parent?.Id,
        Name = "n" + ((uint)(id * 2654435761u) % 1_000_000).ToString("D6", CultureInfo.InvariantCulture),
        Group = "g" + (id % GroupCount),
        Value = id % 100,
        Cells = new object[Math.Max(0, _options.Cols - FixedColumns)].Select((_, c) => (object)("x" + c)).ToArray(),
        Children = Api == "children" ? new ObservableCollection<BenchNode>() : null,
    };

    // ---- Control ----

    private TableView BuildTableView()
    {
        var table = new TableView
        {
            HorizontalAlignment = HorizontalAlignment.Stretch,
            VerticalAlignment = VerticalAlignment.Stretch,
        };

        table.Columns.Add(TextColumn("Name", nameof(BenchNode.Name), 200));
        table.Columns.Add(TextColumn("Id", nameof(BenchNode.Id), 80));
        table.Columns.Add(TextColumn("Group", nameof(BenchNode.Group), 80));
        table.Columns.Add(TextColumn("Value", nameof(BenchNode.Value), 70));
        for (int c = 0; c < _options.Cols - FixedColumns; c++)
        {
            table.Columns.Add(TextColumn("C" + (c + FixedColumns + 1), string.Create(CultureInfo.InvariantCulture, $"Cells[{c}]"), c % 3 == 0 ? 160 : 100));
        }

#if HIERARCHY_API_CHILDREN
        _source = TableViewSource.From(_mode == "tree" ? _roots : _items);
#else
        _source = TableViewSource.From(_items);
#endif
        if (_declareUpFront)
        {
            Declare();
        }

        table.ItemsSource = _source;
        return table;
    }

    private ListView BuildListView() => new()
    {
        ItemsSource = _items,
        SelectionMode = ListViewSelectionMode.Single,
        HorizontalAlignment = HorizontalAlignment.Stretch,
        VerticalAlignment = VerticalAlignment.Stretch,
        ItemTemplate = BuildRowTemplate(wrapInItemContainer: false),
        ItemsPanel = (ItemsPanelTemplate)XamlReader.Load(
            "<ItemsPanelTemplate xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"><ItemsStackPanel Orientation=\"Vertical\"/></ItemsPanelTemplate>"),
    };

    private ItemsView BuildItemsView() => new()
    {
        ItemsSource = _items,
        HorizontalAlignment = HorizontalAlignment.Stretch,
        VerticalAlignment = VerticalAlignment.Stretch,
        Layout = new StackLayout { Orientation = Orientation.Vertical },
        ItemTemplate = BuildRowTemplate(wrapInItemContainer: true),
    };

    // The harness's list-control row: one TextBlock per column in a horizontal StackPanel, same
    // columns and widths as the TableView.
    private DataTemplate BuildRowTemplate(bool wrapInItemContainer)
    {
        var row = new StringBuilder("<DataTemplate xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\">");
        row.Append(wrapInItemContainer ? "<ItemContainer>" : string.Empty).Append("<StackPanel Orientation=\"Horizontal\">");
        void Cell(string path, int width) => row.Append(CultureInfo.InvariantCulture, $"<TextBlock Text=\"{{Binding {path}}}\" Width=\"{width}\" Margin=\"4,2\"/>");
        Cell(nameof(BenchNode.Name), 200);
        Cell(nameof(BenchNode.Id), 80);
        Cell(nameof(BenchNode.Group), 80);
        Cell(nameof(BenchNode.Value), 70);
        for (int c = 0; c < _options.Cols - FixedColumns; c++)
        {
            Cell(string.Create(CultureInfo.InvariantCulture, $"Cells[{c}]"), c % 3 == 0 ? 160 : 100);
        }

        row.Append("</StackPanel>").Append(wrapInItemContainer ? "</ItemContainer>" : string.Empty).Append("</DataTemplate>");
        return (DataTemplate)XamlReader.Load(row.ToString());
    }

    // List controls have no sort/filter model: rebuild the bound collection, as the harness does.
    private void Repopulate(IEnumerable<BenchNode> rows)
    {
        var list = rows.ToList();
        _items.Clear();
        foreach (var row in list)
        {
            _items.Add(row);
        }
    }

    private static TableViewTextColumn TextColumn(string header, string path, double width) => new()
    {
        Header = header,
        Binding = new Binding { Path = new PropertyPath(path) },
        Width = new GridLength(width),
    };

    private void Declare()
    {
#if HIERARCHY_API_CHILDREN
        _source.WithChildren(new TableViewChildrenSelector(o => ((BenchNode)o).Children!));
#elif HIERARCHY_API_PARENTKEY2
        _source.ParentBy(new TableViewKeySelector(o => ((BenchNode)o).Id), new TableViewKeySelector(o => ((BenchNode)o).ParentId!));
#else
        _source.KeyBy(new TableViewKeySelector(o => ((BenchNode)o).Id));
        _source.ParentBy(new TableViewKeySelector(o => ((BenchNode)o).ParentId!));
#endif
    }

    // ---- Lifecycle ----

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        if (_startExpanded)
        {
            var sw = Stopwatch.StartNew();
            _table!.ExpandAllRows();
            _setupExpandMs = sw.Elapsed.TotalMilliseconds;
        }

        CompositionTarget.Rendering += OnFirstRendering;
    }

    private void OnFirstRendering(object? sender, object e)
    {
        if (_firstRenderSeen)
        {
            return;
        }

        _firstRenderSeen = true;
        CompositionTarget.Rendering -= OnFirstRendering;
        _firstRenderMs = _swProcess.Elapsed.TotalMilliseconds;
        if (!string.IsNullOrEmpty(_options.ReadyFile))
        {
            try { File.WriteAllText(_options.ReadyFile, _firstRenderMs.ToString("F1", CultureInfo.InvariantCulture)); } catch { }
        }

        Log($"first_render={_firstRenderMs:F1}ms creation={_creationMs:F1}ms setup_expand={F(_setupExpandMs)}ms");
        // Time-to-interactive: the UI thread drained to Low priority after first render.
        _ = DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, () =>
        {
            _interactiveMs = _swProcess.Elapsed.TotalMilliseconds;
            _ = DispatcherQueue.TryEnqueue(async () => await RunScenario());
        });
    }

    private async Task RunScenario()
    {
        var sw = Stopwatch.StartNew();
        try
        {
            await Task.Delay(150);
            _control.UpdateLayout();
            bool list = _table is null;
            if (_mode != "tree" && _scenario is "declare" or "expandall" or "collapseall" or "expand1")
            {
                throw new NotSupportedException(_scenario + " needs a hierarchy");
            }

            if (list && _scenario is "group" or "group-x")
            {
                throw new NotSupportedException(_scenario + " is not supported by " + _mode);
            }

            switch (_scenario)
            {
                case "load":
                case "load-x":
                    await Settle();
                    break;
                case "scroll-x": await ScrollSweep(); break;
                case "resize-x": await ResizeSweep(); break;
                case "select-x" when _control is ListView lv: await Timed(() => lv.SelectedIndex = 10); break;
                case "select-x" when _control is ItemsView iv: await Timed(() => iv.Select(10)); break;
                case "select-x": await Timed(() => _table!.Select(Math.Min(10, Math.Max(0, VisibleRows() - 1)))); break;
                case "declare": await Timed(Declare); break;
                case "expandall": await Timed(_table!.ExpandAllRows); break;
                case "collapseall": await Timed(_table!.CollapseAllRows); break;
                case "expand1": await Timed(() => ToggleRow(FirstRoot(), expand: true)); break;
                case "sort" or "sort-x" when list:
                    await Timed(() => Repopulate(_nodes.OrderBy(n => n.Name, StringComparer.Ordinal)));
                    break;
                case "filter" or "filter-x" when list:
                    await Timed(() => Repopulate(_nodes.Where(n => n.Value < 10)));
                    break;
                case "sort":
                case "sort-x":
                    await Timed(() => _source.Sort(new TableViewKeySelector(o => ((BenchNode)o).Name), SortDirection.Ascending));
                    break;
                case "filter":
                case "filter-x":
                    await Timed(() => _source.Filter(new TableViewPredicate(o => ((BenchNode)o).Value < 10)));
                    break;
                case "group":
                case "group-x":
                    await Timed(() => _source.GroupBy(new TableViewKeySelector(o => ((BenchNode)o).Group)));
                    break;
                case "edit-x": await Timed(InsertLeaf); break;
                case "value-x": await Timed(() => _nodes[_nodes.Count / 2].Value++); break;
                default: throw new ArgumentException("unknown scenario " + _options.Scenario);
            }

            if (_scenario is not ("load" or "load-x" or "scroll-x" or "resize-x"))
            {
                await Settle();
            }
        }
        catch (Exception ex)
        {
            _error = ex.GetType().Name + ": " + ex.Message;
            Log("scenario error " + ex);
        }

        sw.Stop();
        _scenarioMs = sw.Elapsed.TotalMilliseconds;
        EmitL2();
        Log("done");
        // Metrics are flushed; skip XAML teardown, which crashes (0xC0000409) and adds ~20s of WER time per run.
        TerminateProcess(GetCurrentProcess(), 0);
    }

    [System.Runtime.InteropServices.DllImport("kernel32.dll")]
    private static extern IntPtr GetCurrentProcess();

    [System.Runtime.InteropServices.DllImport("kernel32.dll")]
    private static extern bool TerminateProcess(IntPtr hProcess, uint exitCode);

    private Task Settle() => Task.Delay(_options.SettleMs);

    // Action -> fully rendered UI: op + UpdateLayout + 2 composition frames.
    private async Task Timed(Action op)
    {
        _rowsBefore = VisibleRows();
        var sw = Stopwatch.StartNew();
        var phase = Stopwatch.StartNew();
        op();
        _opSyncMs = phase.Elapsed.TotalMilliseconds;
        phase.Restart();
        _control.UpdateLayout();
        _layoutMs = phase.Elapsed.TotalMilliseconds;
        phase.Restart();
        bool rendered = await WaitForRenderedAsync();
        _renderMs = phase.Elapsed.TotalMilliseconds;
        sw.Stop();
        _opMs = sw.Elapsed.TotalMilliseconds;
        _rowsAfter = VisibleRows();
        if (!rendered)
        {
            _error = "no frame within " + RenderTimeoutMs + " ms";
        }

        Log($"op={_scenario} op_ms={_opMs:F2} rows {_rowsBefore} -> {_rowsAfter}");
    }

    // Same logical edit for every API: a new leaf under the middle row.
    private void InsertLeaf()
    {
        var host = _nodes[_nodes.Count / 2];
        var node = NewNode(_nodes.Count + 1, host);
#if HIERARCHY_API_CHILDREN
        if (_mode == "tree")
        {
            host.Children!.Insert(0, node);
            return;
        }
#endif
        _items.Insert(_nodes.Count / 2, node);
    }

    private BenchNode FirstRoot() => _nodes.First(n => n.ParentId is null);

    private async Task ScrollSweep()
    {
        var scrollViewer = FindDescendant<ScrollViewer>(_control);
        var scrollView = scrollViewer is null ? FindDescendant<ScrollView>(_control) : null;
        if (scrollViewer is null && scrollView is null)
        {
            Log("scroll scroller not found");
            await Settle();
            return;
        }

        double max = scrollViewer?.ScrollableHeight ?? scrollView?.ScrollableHeight ?? 0;
        StartFrames();
        var sw = Stopwatch.StartNew();
        while (sw.ElapsedMilliseconds < _options.ScrollMs)
        {
            double t = sw.ElapsedMilliseconds / (double)_options.ScrollMs;
            double pos = t < 0.5 ? (t * 2.0) : (2.0 - t * 2.0);
            if (scrollViewer is not null)
            {
                scrollViewer.ChangeView(null, max * pos, null, disableAnimation: true);
            }
            else
            {
                scrollView!.ScrollTo(scrollView.HorizontalOffset, max * pos, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled));
            }

            await Task.Delay(12);
        }

        StopFrames();
    }

    private async Task ResizeSweep()
    {
        // Column-less list controls have nothing to resize; emit empty frame stats (as the harness does).
        if (_table is null)
        {
            await Settle();
            return;
        }

        var column = _table.Columns[0];
        double baseWidth = column.Width.IsAbsolute ? column.Width.Value : 200;
        StartFrames();
        var sw = Stopwatch.StartNew();
        while (sw.ElapsedMilliseconds < _options.ScrollMs)
        {
            double delta = 60.0 * Math.Sin(sw.ElapsedMilliseconds / 250.0);
            column.Width = new GridLength(Math.Max(40, baseWidth + delta));
            _table.UpdateLayout();
            await Task.Delay(12);
        }

        StopFrames();
        column.Width = new GridLength(baseWidth);
    }

    // ---- Tree helpers ----

    private ItemsRepeater? Repeater() => _table is null ? null : _repeater ??= FindDescendant<ItemsRepeater>(_table, "PART_RowsRepeater");

    private int VisibleRows() => _table is null ? _items.Count : Repeater()?.ItemsSourceView?.Count ?? -1;

    // Expands or collapses one realized row through its ExpandCollapse automation pattern (the
    // chevron and keyboard path).
    private void ToggleRow(BenchNode node, bool expand)
    {
        var repeater = Repeater() ?? throw new InvalidOperationException("rows repeater not found");
        TableViewRow? row = null;
        int count = VisualTreeHelper.GetChildrenCount(repeater);
        for (int i = 0; i < count && row is null; i++)
        {
            if (VisualTreeHelper.GetChild(repeater, i) is TableViewRow r && repeater.GetElementIndex(r) >= 0 && ReferenceEquals(r.DataContext, node))
            {
                row = r;
            }
        }

        if (row is null)
        {
            throw new InvalidOperationException($"row {node.Id} is not realized");
        }

        var peer = FrameworkElementAutomationPeer.CreatePeerForElement(row);
        var pattern = (IExpandCollapseProvider)peer.GetPattern(PatternInterface.ExpandCollapse)
            ?? throw new InvalidOperationException($"row {node.Id} has no ExpandCollapse pattern");
        if (expand)
        {
            pattern.Expand();
        }
        else
        {
            pattern.Collapse();
        }
    }

    private static T? FindDescendant<T>(DependencyObject root, string? name = null) where T : DependencyObject
    {
        int c = VisualTreeHelper.GetChildrenCount(root);
        for (int i = 0; i < c; i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            if (child is T t && (name is null || (child is FrameworkElement fe && fe.Name == name)))
            {
                return t;
            }

            if (FindDescendant<T>(child, name) is { } found)
            {
                return found;
            }
        }

        return null;
    }

    private int CountRealized<T>() where T : DependencyObject
    {
        int n = 0;
        void Walk(DependencyObject d)
        {
            int c = VisualTreeHelper.GetChildrenCount(d);
            for (int i = 0; i < c; i++)
            {
                var child = VisualTreeHelper.GetChild(d, i);
                if (child is T)
                {
                    n++;
                }

                Walk(child);
            }
        }

        Walk(_control);
        return n;
    }

    // ---- Frames ----

    private Task<bool> WaitForRenderedAsync(int frames = 2)
    {
        var tcs = new TaskCompletionSource<bool>();
        int n = 0;
        EventHandler<object>? handler = null;
        handler = (_, _) =>
        {
            if (++n >= frames)
            {
                CompositionTarget.Rendering -= handler;
                tcs.TrySetResult(true);
            }
        };
        CompositionTarget.Rendering += handler;
        return Task.WhenAny(tcs.Task, Task.Delay(RenderTimeoutMs)).ContinueWith(t =>
        {
            if (t.Result != tcs.Task)
            {
                CompositionTarget.Rendering -= handler;
                return false;
            }

            return true;
        }, TaskScheduler.FromCurrentSynchronizationContext());
    }

    private void StartFrames()
    {
        _frameIntervals.Clear();
        _lastFrame = DateTime.UtcNow;
        _capturing = true;
        CompositionTarget.Rendering += OnRender;
    }

    private void StopFrames()
    {
        _capturing = false;
        CompositionTarget.Rendering -= OnRender;
    }

    private void OnRender(object? sender, object e)
    {
        var now = DateTime.UtcNow;
        var dt = (now - _lastFrame).TotalMilliseconds;
        _lastFrame = now;
        if (_capturing && dt > 0 && dt < 2000)
        {
            _frameIntervals.Add(dt);
        }
    }

    private (double fpsAvg, double p95, double p99, double frameMax, double jankPct, int drops) FrameStats()
    {
        if (_frameIntervals.Count == 0)
        {
            return (double.NaN, double.NaN, double.NaN, double.NaN, double.NaN, 0);
        }

        var sorted = _frameIntervals.OrderBy(static x => x).ToList();
        double mean = _frameIntervals.Average();
        double p95 = sorted[(int)Math.Min(sorted.Count - 1, Math.Floor(sorted.Count * 0.95))];
        double p99 = sorted[(int)Math.Min(sorted.Count - 1, Math.Floor(sorted.Count * 0.99))];
        double jank = 100.0 * _frameIntervals.Count(static x => x > 16.7) / _frameIntervals.Count;
        return (mean > 0 ? 1000.0 / mean : double.NaN, p95, p99, sorted[^1], jank, _frameIntervals.Count(static x => x > 33.3));
    }

    // ---- Output ----

    private void EmitL2()
    {
        var (fps, p95, p99, frameMax, jank, drops) = FrameStats();
        int realized = _mode switch
        {
            "listview" => CountRealized<ListViewItem>(),
            "itemsview" => CountRealized<ItemContainer>(),
            _ => CountRealized<TableViewRow>(),
        };
        int cells = CountRealized<TextBlock>();
        int realizedCols = realized > 0 ? (int)Math.Round(cells / (double)realized) : cells;
        int gc0 = GC.CollectionCount(0), gc1 = GC.CollectionCount(1), gc2 = GC.CollectionCount(2);
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        double heapMb = GC.GetTotalMemory(true) / (1024.0 * 1024.0);
        string I(int v) => v.ToString(CultureInfo.InvariantCulture);

        // Harness-standard columns first (sort_ms/filter_ms/select_ms only for the flat-named
        // scenarios, so the report's sort/filter/select sections stay meaningful), then hierarchy extras.
        var row = new List<(string, string)>
        {
            ("control", _options.Control), ("scenario", _options.Scenario), ("rows", I(_options.Rows)), ("cols", I(_options.Cols)),
            ("creation_ms", F(_creationMs)), ("first_render_ms", F(_firstRenderMs)),
            ("fps_avg", F(fps)), ("frame_p99_ms", F(p99)), ("jank_pct", F(jank)), ("frame_drops", I(drops)),
            ("realized_rows", I(realized)), ("total_rows", I(_options.Rows)), ("managed_heap_mb", F(heapMb)),
            ("gc_gen0", I(gc0)), ("gc_gen1", I(gc1)), ("gc_gen2", I(gc2)),
            ("sort_ms", _scenario == "sort" ? F(_opMs) : ""), ("filter_ms", _scenario == "filter" ? F(_opMs) : ""),
            ("select_ms", _scenario == "select-x" ? F(_opMs) : ""), ("scenario_ms", F(_scenarioMs)),
            ("realized_cols", I(realizedCols)), ("realized_cells", I(cells)), ("frame_p95_ms", F(p95)), ("frame_max_ms", F(frameMax)),
            ("time_to_interactive_ms", F(_interactiveMs)), ("data_gen_ms", F(_dataGenMs)), ("control_creation_ms", F(_controlCreationMs)),
            ("api", _mode == "tree" ? Api : _mode), ("shape", _options.Shape), ("op_ms", F(_opMs)), ("setup_expand_ms", F(_setupExpandMs)),
            ("op_sync_ms", F(_opSyncMs)), ("layout_ms", F(_layoutMs)), ("render_ms", F(_renderMs)),
            ("visible_rows_before", I(_rowsBefore)), ("visible_rows_after", I(_rowsAfter)), ("error", (_error ?? "").Replace(',', ';')),
        };

        try
        {
            File.WriteAllText(_options.EmitFile,
                string.Join(",", row.Select(r => r.Item1)) + Environment.NewLine + string.Join(",", row.Select(r => r.Item2)) + Environment.NewLine);
            Log($"emitted L2 -> {_options.EmitFile} (op_ms={F(_opMs)} realized={realized} fps={F(fps)} heap={F(heapMb)}MB)");
        }
        catch (Exception ex)
        {
            Log("emit error " + ex);
        }
    }

    private static string F(double d) => double.IsNaN(d) ? "" : d.ToString("F2", CultureInfo.InvariantCulture);

    private void Log(string message)
    {
        try { File.AppendAllText(_options.LogFile, $"[{DateTime.Now:HH:mm:ss.fff}] {message}{Environment.NewLine}"); } catch { }
    }
}

public sealed class BenchmarkOptions
{
    private readonly Dictionary<string, string> _args;

    private BenchmarkOptions(Dictionary<string, string> args, string baseDirectory)
    {
        _args = args;
        Control = Arg("control", "TableView-" + BenchmarkHost.Api);
        Scenario = Arg("scenario", "load");
        Shape = Arg("shape", "balanced").ToLowerInvariant();
        Rows = ArgInt("rows", 10000);
        Cols = Math.Max(4, ArgInt("cols", 10));
        SettleMs = ArgInt("settle-ms", 1500);
        ScrollMs = ArgInt("scroll-ms", 5000);
        EmitFile = Arg("emit", Path.Combine(baseDirectory, "l2.csv"));
        ReadyFile = Arg("ready-file", "");
        LogFile = Arg("log", Path.ChangeExtension(EmitFile, ".log"));
    }

    public string Control { get; }
    public string Scenario { get; }
    public string Shape { get; }
    public int Rows { get; }
    public int Cols { get; }
    public int SettleMs { get; }
    public int ScrollMs { get; }
    public string EmitFile { get; }
    public string ReadyFile { get; }
    public string LogFile { get; }

    // "--name value" pairs; a bare "--flag" maps to "true".
    public static BenchmarkOptions Parse(string[] argv, string baseDirectory)
    {
        var map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        for (int i = 0; i < argv.Length; i++)
        {
            if (!argv[i].StartsWith("--", StringComparison.Ordinal))
            {
                continue;
            }

            string name = argv[i][2..];
            if (i + 1 < argv.Length && !argv[i + 1].StartsWith("--", StringComparison.Ordinal))
            {
                map[name] = argv[++i];
            }
            else
            {
                map[name] = "true";
            }
        }

        return new BenchmarkOptions(map, baseDirectory);
    }

    private string Arg(string name, string fallback) => _args.TryGetValue(name, out var v) && v.Length > 0 ? v : fallback;

    private int ArgInt(string name, int fallback) => int.TryParse(Arg(name, ""), NumberStyles.Integer, CultureInfo.InvariantCulture, out var v) ? v : fallback;
}
