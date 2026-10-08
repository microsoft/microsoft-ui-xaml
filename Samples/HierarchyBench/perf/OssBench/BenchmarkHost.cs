using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using OssTableView = WinUI.TableView.TableView;
using OssTextColumn = WinUI.TableView.TableViewTextColumn;
using OssRow = WinUI.TableView.TableViewRow;
using OssSortDescription = WinUI.TableView.SortDescription;
using OssFilterDescription = WinUI.TableView.FilterDescription;
using OssSortDirection = WinUI.TableView.SortDirection;

namespace OssBench;

// Same row shape as HierarchyBench.BenchNode (flat).
public sealed class BenchNode
{
    public int Id { get; set; }
    public int? ParentId { get; set; }
    public string Name { get; set; } = string.Empty;
    public string Group { get; set; } = string.Empty;
    public int Value { get; set; }
    public object[] Cells { get; set; } = Array.Empty<object>();
}

// Open-source WinUI.TableView (w-ahmad) flat baseline for the hierarchy runs. Mirrors
// HierarchyBench's flat modes: same data generator, columns, sort key (Name), filter (Value < 10),
// edit (insert one row mid-collection), timing (op + UpdateLayout + 2 frames) and L2 columns.
// Sort/filter use the control's own SortDescriptions/FilterDescriptions, as the harness's bench does.
public sealed class BenchmarkHost : Grid
{
    private const int FixedColumns = 4;
    private const int GroupCount = 8;
    private const int RenderTimeoutMs = 5000;

    private readonly BenchmarkOptions _options;
    private readonly Stopwatch _swProcess = Stopwatch.StartNew();
    private readonly List<double> _frameIntervals = new();
    private readonly string _scenario;
    private List<BenchNode> _nodes = new();
    private ObservableCollection<BenchNode> _items = new();
    private readonly OssTableView _table;

    private double _creationMs, _dataGenMs, _controlCreationMs, _firstRenderMs;
    private double _opMs = double.NaN, _scenarioMs = double.NaN, _interactiveMs = double.NaN;
    private double _opSyncMs = double.NaN, _layoutMs = double.NaN, _renderMs = double.NaN;
    private int _rowsBefore = -1, _rowsAfter = -1;
    private string? _error;
    private DateTime _lastFrame;
    private bool _capturing, _firstRenderSeen;

    public BenchmarkHost(BenchmarkOptions options)
    {
        _options = options;
        _scenario = options.Scenario.ToLowerInvariant();
        HorizontalAlignment = HorizontalAlignment.Stretch;
        VerticalAlignment = VerticalAlignment.Stretch;
        Loaded += (_, _) => CompositionTarget.Rendering += OnFirstRendering;
        Log($"start control={options.Control} scenario={options.Scenario} rows={options.Rows} cols={options.Cols}");

        var swData = Stopwatch.StartNew();
        Generate(options.Rows, options.Cols);
        _dataGenMs = swData.Elapsed.TotalMilliseconds;

        var swCtl = Stopwatch.StartNew();
        _table = BuildTableView();
        Children.Add(_table);
        _controlCreationMs = swCtl.Elapsed.TotalMilliseconds;
        _creationMs = _dataGenMs + _controlCreationMs;
    }

    private void Generate(int n, int cols)
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

            _nodes.Add(NewNode(i + 1, cells));
        }

        _items = new ObservableCollection<BenchNode>(_nodes);
    }

    private static BenchNode NewNode(int id, object[] cells) => new()
    {
        Id = id,
        Name = "n" + ((uint)((id - 1) * 2654435761u) % 1_000_000).ToString("D6", CultureInfo.InvariantCulture),
        Group = "g" + ((id - 1) % GroupCount),
        Value = (int)((uint)((id - 1) * 40503u) % 100),
        Cells = cells,
    };

    private OssTableView BuildTableView()
    {
        var table = new OssTableView
        {
            ItemsSource = _items,
            SelectionMode = ListViewSelectionMode.Single,
            AutoGenerateColumns = false,
            CanSortColumns = true,
            CanFilterColumns = true,
            AllowLiveShaping = false,
            HorizontalAlignment = HorizontalAlignment.Stretch,
            VerticalAlignment = VerticalAlignment.Stretch,
        };

        table.Columns.Add(Column("Name", nameof(BenchNode.Name), 200));
        table.Columns.Add(Column("Id", nameof(BenchNode.Id), 80));
        table.Columns.Add(Column("Group", nameof(BenchNode.Group), 80));
        table.Columns.Add(Column("Value", nameof(BenchNode.Value), 70));
        for (int c = 0; c < _options.Cols - FixedColumns; c++)
        {
            table.Columns.Add(Column("C" + (c + FixedColumns + 1), string.Create(CultureInfo.InvariantCulture, $"Cells[{c}]"), c % 3 == 0 ? 160 : 100));
        }

        return table;
    }

    private static OssTextColumn Column(string header, string path, double width) => new()
    {
        Header = header,
        Width = new GridLength(width),
        Binding = new Binding { Path = new PropertyPath(path), Mode = BindingMode.OneWay },
    };

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
            _table.UpdateLayout();
            switch (_scenario)
            {
                case "load":
                case "load-x":
                    await Settle();
                    break;
                case "scroll-x": await ScrollSweep(); break;
                case "resize-x": await ResizeSweep(); break;
                case "select-x": await Timed(() => _table.SelectedIndex = 10); break;
                case "sort":
                case "sort-x":
                    await Timed(() =>
                    {
                        _table.ClearAllSorting();
                        _table.Columns[0].SortDirection = OssSortDirection.Ascending;
                        _table.SortDescriptions.Add(new OssSortDescription(
                            propertyName: null,
                            direction: OssSortDirection.Ascending,
                            comparer: Comparer<object>.Create((x, y) => string.CompareOrdinal((string?)x, (string?)y)),
                            valueDelegate: o => ((BenchNode)o).Name));
                        _table.RefreshSorting();
                    });
                    break;
                case "filter":
                case "filter-x":
                    await Timed(() =>
                    {
                        _table.ClearAllFilters();
                        _table.FilterDescriptions.Add(new OssFilterDescription(propertyName: null, predicate: o => o is BenchNode n && n.Value < 10));
                        _table.RefreshFilter();
                    });
                    break;
                case "edit-x":
                    await Timed(() => _items.Insert(_nodes.Count / 2, NewNode(_nodes.Count + 1, new object[Math.Max(0, _options.Cols - FixedColumns)].Select((_, c) => (object)("x" + c)).ToArray())));
                    break;
                case "value-x": await Timed(() => _nodes[_nodes.Count / 2].Value++); break;
                default: throw new NotSupportedException(_scenario + " is not supported by the flat OSS TableView");
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

        _scenarioMs = sw.Elapsed.TotalMilliseconds;
        EmitL2();
        // Skip XAML teardown, as in HierarchyBench.
        TerminateProcess(GetCurrentProcess(), 0);
    }

    [System.Runtime.InteropServices.DllImport("kernel32.dll")]
    private static extern IntPtr GetCurrentProcess();

    [System.Runtime.InteropServices.DllImport("kernel32.dll")]
    private static extern bool TerminateProcess(IntPtr hProcess, uint exitCode);

    private Task Settle() => Task.Delay(_options.SettleMs);

    private int VisibleRows() => _table.Items.Count;

    private async Task Timed(Action op)
    {
        _rowsBefore = VisibleRows();
        var sw = Stopwatch.StartNew();
        var phase = Stopwatch.StartNew();
        op();
        _opSyncMs = phase.Elapsed.TotalMilliseconds;
        phase.Restart();
        _table.UpdateLayout();
        _layoutMs = phase.Elapsed.TotalMilliseconds;
        phase.Restart();
        bool rendered = await WaitForRenderedAsync();
        _renderMs = phase.Elapsed.TotalMilliseconds;
        _opMs = sw.Elapsed.TotalMilliseconds;
        _rowsAfter = VisibleRows();
        if (!rendered)
        {
            _error = "no frame within " + RenderTimeoutMs + " ms";
        }
    }

    private async Task ScrollSweep()
    {
        var sv = FindDescendant<ScrollViewer>(_table);
        if (sv is null)
        {
            Log("scrollviewer not found");
            await Settle();
            return;
        }

        double max = sv.ScrollableHeight;
        StartFrames();
        var sw = Stopwatch.StartNew();
        while (sw.ElapsedMilliseconds < _options.ScrollMs)
        {
            double t = sw.ElapsedMilliseconds / (double)_options.ScrollMs;
            double pos = t < 0.5 ? (t * 2.0) : (2.0 - t * 2.0);
            sv.ChangeView(null, max * pos, null, disableAnimation: true);
            await Task.Delay(12);
        }

        StopFrames();
    }

    private async Task ResizeSweep()
    {
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

    private static T? FindDescendant<T>(DependencyObject root) where T : DependencyObject
    {
        int c = VisualTreeHelper.GetChildrenCount(root);
        for (int i = 0; i < c; i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            if (child is T t)
            {
                return t;
            }

            if (FindDescendant<T>(child) is { } found)
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

        Walk(_table);
        return n;
    }

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

    private void EmitL2()
    {
        var (fps, p95, p99, frameMax, jank, drops) = FrameStats();
        int realized = CountRealized<OssRow>();
        int cells = CountRealized<TextBlock>();
        int realizedCols = realized > 0 ? (int)Math.Round(cells / (double)realized) : cells;
        int gc0 = GC.CollectionCount(0), gc1 = GC.CollectionCount(1), gc2 = GC.CollectionCount(2);
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        double heapMb = GC.GetTotalMemory(true) / (1024.0 * 1024.0);
        string I(int v) => v.ToString(CultureInfo.InvariantCulture);

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
            ("api", "oss-flat"), ("shape", "flat"), ("op_ms", F(_opMs)), ("setup_expand_ms", ""),
            ("op_sync_ms", F(_opSyncMs)), ("layout_ms", F(_layoutMs)), ("render_ms", F(_renderMs)),
            ("visible_rows_before", I(_rowsBefore)), ("visible_rows_after", I(_rowsAfter)), ("error", (_error ?? "").Replace(',', ';')),
        };

        try
        {
            File.WriteAllText(_options.EmitFile,
                string.Join(",", row.Select(r => r.Item1)) + Environment.NewLine + string.Join(",", row.Select(r => r.Item2)) + Environment.NewLine);
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
        Control = Arg("control", "WinUITableView");
        Scenario = Arg("scenario", "load");
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
    public int Rows { get; }
    public int Cols { get; }
    public int SettleMs { get; }
    public int ScrollMs { get; }
    public string EmitFile { get; }
    public string ReadyFile { get; }
    public string LogFile { get; }

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
            map[name] = i + 1 < argv.Length && !argv[i + 1].StartsWith("--", StringComparison.Ordinal) ? argv[++i] : "true";
        }

        return new BenchmarkOptions(map, baseDirectory);
    }

    private string Arg(string name, string fallback) => _args.TryGetValue(name, out var v) && v.Length > 0 ? v : fallback;

    private int ArgInt(string name, int fallback) => int.TryParse(Arg(name, ""), NumberStyles.Integer, CultureInfo.InvariantCulture, out var v) ? v : fallback;
}
