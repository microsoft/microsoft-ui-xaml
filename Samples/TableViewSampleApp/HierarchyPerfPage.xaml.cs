using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Media;

namespace TableViewSampleApp;

// Object key with reference identity, so the "object" key type exercises address-based node keys.
public sealed class PerfKey
{
    public PerfKey(int id) => Id = id;
    public int Id { get; }
    public override string ToString() => "K" + Id;
}

// One flat row; the parent key is carried in three forms so each key type can be measured on the
// same tree.
public sealed class PerfNode
{
    public int Id { get; set; }
    public int? ParentId { get; set; }
    public string IdText { get; set; } = string.Empty;
    public string? ParentText { get; set; }
    public PerfKey KeyObject { get; set; } = null!;
    public PerfKey? ParentKeyObject { get; set; }
    public string Name { get; set; } = string.Empty;
    public string Group { get; set; } = string.Empty;
    public int Value { get; set; }

    // Only populated for the children-selector API (every node gets a collection, empty for a leaf,
    // so later child additions are observable).
    public ObservableCollection<PerfNode>? Children { get; set; }
}

// Measures the hierarchy pipeline step by step on a fresh source per run.
//
// Every run writes a folder <output>\<yyyyMMdd-HHmmss>_<variant>\ with results.json (schema
// "tableview-hierarchy-perf/1", the input for comparisons), results.csv, results.txt, run.log and a
// self-contained report.html. "Compare runs" writes <output>\compare.html over every results.json
// under the output folder; report.html also accepts more results.json files by picker or drag-drop.
//
// Unattended: an "autorun-hierarchy-perf" file in the data directory opens this page, runs, and
// exits. Its content holds whitespace-separated options (values must not contain spaces):
//   n=1000,10000,50000  shape=all|wide,deep,...  key=all|int,string,object  runs=3
//   variant=<label>  out=<folder>  compare=1
// An empty file runs the full matrix into the default output folder.
public sealed partial class HierarchyPerfPage : Page
{
    public const string TriggerFileName = "autorun-hierarchy-perf";
    public const string Schema = "tableview-hierarchy-perf/1";

    // The hierarchy API this build targets (TableViewSampleApp.csproj, HierarchyApi property). The
    // same page source is built once per API so every variant runs identical steps and reporting;
    // only the hierarchy declaration, the data layout and the source edits differ.
#if HIERARCHY_API_CHILDREN
    public const string HierarchyApi = "children";
#elif HIERARCHY_API_PARENTKEY2
    public const string HierarchyApi = "parentkey2";
#else
    public const string HierarchyApi = "keyby";
#endif

    // The children-selector API has no row key, so it runs once per shape under this key label;
    // the HTML report matches it against every key type of the other variants.
    private const string NoKey = "n/a";

    public static string? AutoRunArgs { get; set; }

    // LocalFolder when the app has package identity; otherwise the exe's directory (the build
    // produces an unpackaged app, where ApplicationData.Current throws).
    public static string DataDirectory
    {
        get
        {
            try
            {
                return Windows.Storage.ApplicationData.Current.LocalFolder.Path;
            }
            catch (Exception)
            {
                return AppContext.BaseDirectory;
            }
        }
    }

    private static readonly int[] Counts = { 1000, 10000, 50000 };
    private static readonly string[] Shapes = { "wide", "deep", "balanced", "shallow" };
    private static readonly string[] KeyTypes = { "int", "string", "object" };
    private const int FullMatrixRuns = 3;
    private const int GroupCount = 8;
    private const int FrameTimeoutMs = 2000;

    private sealed record Options(int[] Counts, string[] Shapes, string[] KeyTypes, int Runs, string Variant, string OutputRoot, bool Compare);

    private sealed record Sample(
        double Call, double Layout, double Frame,
        int KeyCalls, int ParentCalls, int OtherCalls,
        long ManagedKb, long PrivateKb, int Rows, string? Error);

    private sealed class StepResult
    {
        public required string Step { get; init; }
        public List<Sample> Runs { get; } = new();
    }

    private sealed class ConfigResult
    {
        public required int N { get; init; }
        public required string Shape { get; init; }
        public required string Key { get; init; }
        public List<StepResult> Steps { get; } = new();
    }

    private readonly List<ConfigResult> _results = new();
    private List<(string Step, Sample Sample)> _current = new();
    private int _keyCalls, _parentCalls, _otherCalls;
    private bool _running, _cancelRequested;
    private ItemsRepeater? _repeater;
    private StreamWriter? _log;
    private string? _lastReport;

    public static string DefaultOutputRoot => Path.Combine(DataDirectory, "hierarchy-perf");

    public HierarchyPerfPage()
    {
        this.InitializeComponent();
        Table.Columns.Add(SampleColumns.Text("Name", nameof(PerfNode.Name), SampleColumns.Star(2)));
        Table.Columns.Add(SampleColumns.Text("Id", nameof(PerfNode.Id), SampleColumns.Pixels(80)));
        Table.Columns.Add(SampleColumns.Text("Group", nameof(PerfNode.Group), SampleColumns.Pixels(90)));
        Table.Columns.Add(SampleColumns.Text("Value", nameof(PerfNode.Value), SampleColumns.Pixels(70)));
        OutputBox.Text = DefaultOutputRoot;

        Loaded += async (_, _) =>
        {
            if (AutoRunArgs is { } args)
            {
                AutoRunArgs = null;
                Options options;
                try
                {
                    options = ParseArgs(args);
                }
                catch (Exception ex)
                {
                    SetStatus("bad autorun options: " + ex.Message);
                    WriteCrashNote("bad autorun options '" + args + "': " + ex);
                    Application.Current.Exit();
                    return;
                }

                await RunAsync(options);
                Application.Current.Exit();
            }
        };
    }

    // ---- UI ----

    private async void Run_Click(object sender, RoutedEventArgs e) => await RunAsync(OptionsFromUi(fullMatrix: false));

    private async void AutoRun_Click(object sender, RoutedEventArgs e) => await RunAsync(OptionsFromUi(fullMatrix: true));

    private void Cancel_Click(object sender, RoutedEventArgs e) => _cancelRequested = true;

    private void CopyCsv_Click(object sender, RoutedEventArgs e)
    {
        var package = new Windows.ApplicationModel.DataTransfer.DataPackage();
        package.SetText(BuildCsv());
        Windows.ApplicationModel.DataTransfer.Clipboard.SetContent(package);
    }

    private void OpenReport_Click(object sender, RoutedEventArgs e)
    {
        if (_lastReport is not null)
        {
            Open(_lastReport);
        }
    }

    private void Compare_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var path = WriteCompareReport(OutputRoot());
            SetStatus("compare report: " + path);
            Open(path);
        }
        catch (Exception ex)
        {
            SetStatus("compare failed: " + ex.Message);
        }
    }

    private static void Open(string path) =>
        Process.Start(new ProcessStartInfo(path) { UseShellExecute = true })?.Dispose();

    private static string Selected(ComboBox combo) => ((ComboBoxItem)combo.SelectedItem).Content.ToString()!;

    private string OutputRoot() => string.IsNullOrWhiteSpace(OutputBox.Text) ? DefaultOutputRoot : OutputBox.Text.Trim();

    private Options OptionsFromUi(bool fullMatrix)
    {
        string variant = string.IsNullOrWhiteSpace(VariantBox.Text) ? DefaultVariant() : VariantBox.Text.Trim();
        if (fullMatrix)
        {
            return new Options(Counts, Shapes, KeyTypes, FullMatrixRuns, variant, OutputRoot(), Compare: false);
        }

        string count = Selected(CountCombo), shape = Selected(ShapeCombo), key = Selected(KeyCombo);
        return new Options(
            count == "all" ? Counts : new[] { int.Parse(count, CultureInfo.InvariantCulture) },
            shape == "all" ? Shapes : new[] { shape },
            key == "all" ? KeyTypes : new[] { key },
            int.Parse(Selected(RunsCombo), CultureInfo.InvariantCulture),
            variant, OutputRoot(), Compare: false);
    }

    private static Options ParseArgs(string args)
    {
        var map = args.Split((char[]?)null, StringSplitOptions.RemoveEmptyEntries)
            .Select(p => p.Split('=', 2))
            .Where(p => p.Length == 2)
            .ToDictionary(p => p[0].Trim().ToLowerInvariant(), p => p[1].Trim());

        string Get(string name, string fallback) => map.TryGetValue(name, out var v) && v.Length > 0 ? v : fallback;
        string[] List(string name, string[] all)
        {
            var v = Get(name, "all").ToLowerInvariant();
            var list = v == "all" ? all : v.Split(',', StringSplitOptions.RemoveEmptyEntries);
            foreach (var item in list)
            {
                if (!all.Contains(item))
                {
                    throw new ArgumentException($"unknown {name} '{item}'");
                }
            }

            return list;
        }

        var countText = Get("n", "all");
        return new Options(
            countText == "all" ? Counts : countText.Split(',').Select(c => int.Parse(c, CultureInfo.InvariantCulture)).ToArray(),
            List("shape", Shapes),
            List("key", KeyTypes),
            int.Parse(Get("runs", FullMatrixRuns.ToString(CultureInfo.InvariantCulture)), CultureInfo.InvariantCulture),
            Get("variant", DefaultVariant()),
            Get("out", DefaultOutputRoot),
            Get("compare", "0") is "1" or "true");
    }

    // ---- Driver ----

    private async Task RunAsync(Options options)
    {
        if (_running)
        {
            return;
        }

        _running = true;
        _cancelRequested = false;
#if HIERARCHY_API_CHILDREN
        options = options with { KeyTypes = new[] { NoKey } };
#endif
        RunButton.IsEnabled = AutoRunButton.IsEnabled = CompareButton.IsEnabled = false;
        CancelButton.IsEnabled = true;
        _results.Clear();

        var started = DateTimeOffset.Now;
        var runDir = Path.Combine(options.OutputRoot, $"{started:yyyyMMdd-HHmmss}_{Sanitize(options.Variant)}");
        string status = "running";
        try
        {
            Directory.CreateDirectory(runDir);
            _log = new StreamWriter(Path.Combine(runDir, "run.log"), append: false, new UTF8Encoding(false)) { AutoFlush = true };
        }
        catch (Exception ex)
        {
            SetStatus("cannot create output folder: " + ex.Message);
            ResetButtons();
            return;
        }

        Log($"variant={options.Variant} output={runDir}");
        Log($"options n={string.Join(",", options.Counts)} shapes={string.Join(",", options.Shapes)} keys={string.Join(",", options.KeyTypes)} runs={options.Runs} (+1 warm-up)");
        foreach (var (k, v) in Environment())
        {
            Log($"env {k}={v}");
        }

        int configs = options.Counts.Length * options.Shapes.Length * options.KeyTypes.Length, done = 0;
        try
        {
            foreach (var n in options.Counts)
            {
                foreach (var shape in options.Shapes)
                {
                    foreach (var keyType in options.KeyTypes)
                    {
                        var config = new ConfigResult { N = n, Shape = shape, Key = keyType };
                        Log($"config {done + 1}/{configs} n={n} shape={shape} key={keyType}");
                        for (int run = 0; run <= options.Runs; run++)
                        {
                            SetStatus($"[{done + 1}/{configs}] n={n} {shape}/{keyType} run {run}/{options.Runs}{(run == 0 ? " (warm-up)" : "")}  -> {runDir}");
                            var samples = await RunOnceAsync(n, shape, keyType, run);
                            if (run == 0)
                            {
                                continue;
                            }

                            for (int i = 0; i < samples.Count; i++)
                            {
                                if (config.Steps.Count <= i)
                                {
                                    config.Steps.Add(new StepResult { Step = samples[i].Step });
                                }

                                config.Steps[i].Runs.Add(samples[i].Sample);
                            }
                        }

                        _results.Add(config);
                        done++;
                        ResultsBox.Text = BuildText(options);
                        WriteOutputs(runDir, options, started, status);
                    }
                }
            }

            status = "done";
        }
        catch (OperationCanceledException)
        {
            status = "cancelled";
        }
        catch (Exception ex)
        {
            status = "error";
            Log("ERROR " + ex);
        }
        finally
        {
            Table.ItemsSource = null;
            ResultsBox.Text = BuildText(options);
            WriteOutputs(runDir, options, started, status);
            Log($"status={status} elapsed={(DateTimeOffset.Now - started).TotalSeconds:F1}s");
            try
            {
                File.WriteAllText(Path.Combine(options.OutputRoot, "latest.txt"), runDir);
                if (options.Compare)
                {
                    Log("compare report: " + WriteCompareReport(options.OutputRoot));
                }
            }
            catch (Exception ex)
            {
                Log("could not write latest/compare: " + ex.Message);
            }

            _log?.Dispose();
            _log = null;
            _lastReport = Path.Combine(runDir, "report.html");
            OpenReportButton.IsEnabled = true;
            SetStatus($"{status}: {runDir}");
            ResetButtons();
        }
    }

    private void ResetButtons()
    {
        RunButton.IsEnabled = AutoRunButton.IsEnabled = CompareButton.IsEnabled = true;
        CancelButton.IsEnabled = false;
        _running = false;
    }

    private async Task<List<(string Step, Sample Sample)>> RunOnceAsync(int n, string shape, string keyType, int run)
    {
        _current = new List<(string, Sample)>();
        var nodes = Generate(shape, n);
        var items = new ObservableCollection<PerfNode>(nodes);
        int nextId = n + 1;
        int half = n / 2;
        int midLeaf = MidLeafIndex(nodes, half + 1);
        var root = nodes[0];

        var sortBy = new TableViewKeySelector(o => { _otherCalls++; return ((PerfNode)o).Name; });
        var filter = new TableViewPredicate(o => { _otherCalls++; return ((PerfNode)o).Value < 10; });
        var groupBy = new TableViewKeySelector(o => { _otherCalls++; return ((PerfNode)o).Group; });

        TableViewSource source = null!;
        Untimed(() => Table.ItemsSource = null);

        // Build. S1/S2 are the same flat baseline for every API.
        await Step(run, "S1 flat: assign source", () => { source = TableViewSource.From(items); Table.ItemsSource = source; });
#if HIERARCHY_API_KEYBY
        var key = KeySelector(keyType);
        await Step(run, "S2 flat: KeyBy", () => source.KeyBy(key));
#endif
        await Step(run, "S2 flat: Sort(Name)", () => source.Sort(sortBy, SortDirection.Ascending));
        Untimed(() => source.ClearSort());

#if HIERARCHY_API_CHILDREN
        // The children API takes the roots and discovers the rest through the selector.
        BuildChildren(nodes);
        var roots = new ObservableCollection<PerfNode>(nodes.Where(x => x.ParentId is null));
        Untimed(() => { source = TableViewSource.From(roots); Table.ItemsSource = source; });
        // No HasChildren predicate: without it every node's child collection is realized and
        // observed, so a leaf gaining its first child is seen (the predicate path has no channel
        // to report that its answer changed).
        var children = new TableViewChildrenSelector(o => { _parentCalls++; return ((PerfNode)o).Children!; });
        await Step(run, "S3 declare hierarchy (collapsed)", () => source.WithChildren(children));
#elif HIERARCHY_API_PARENTKEY2
        var key = KeySelector(keyType);
        var parent = ParentSelector(keyType);
        await Step(run, "S3 declare hierarchy (collapsed)", () => source.ParentBy(key, parent));
#else
        var parent = ParentSelector(keyType);
        await Step(run, "S3 declare hierarchy (collapsed)", () => source.ParentBy(parent));
#endif

        // Shaping on the collapsed tree.
        await Step(run, "S4 collapsed: Sort(Name)", () => source.Sort(sortBy, SortDirection.Ascending));
        Untimed(() => source.ClearSort());
        await Step(run, "S4 collapsed: Filter(10%)", () => source.Filter(filter));
        Untimed(() => source.ClearFilter());
        await Step(run, $"S4 collapsed: GroupBy({GroupCount})", () => source.GroupBy(groupBy));
        Untimed(() => source.ClearGroupBy());

        // Expand / collapse.
        await Step(run, "S5 expand root 0 (one level)", () => ToggleRow(root, expand: true));
        await Step(run, "S6 ExpandAllRows", () => Table.ExpandAllRows());
        await Step(run, "S5 collapse root 0 (expanded subtree)", () => ToggleRow(root, expand: false));
        Untimed(() => Table.ExpandAllRows());

        // Single-item source changes on the expanded tree. Same logical edits for every API: the
        // flat APIs edit the one flat collection, the children API edits the owning child collection.
        var host = nodes[half];
        var inserted = NewNode(nextId++, host);
        var leaf = nodes[midLeaf];
#if HIERARCHY_API_CHILDREN
        var leafOwner = leaf.ParentId is int ownerId ? nodes[ownerId - 1].Children! : roots;
        await Step(run, "S9 append leaf", () => host.Children!.Add(NewNode(nextId++, host)), expectRowDelta: 1);
        await Step(run, "S9 insert leaf (middle)", () => host.Children!.Insert(0, inserted), expectRowDelta: 1);
        await Step(run, "S9 remove leaf (middle)", () => host.Children!.RemoveAt(0), expectRowDelta: -1);
        await Step(run, "S9 reparent leaf (middle)", () => { leafOwner.Remove(leaf); root.Children!.Add(leaf); });
        await Step(run, "S9 append root", () => roots.Add(NewNode(nextId++, null)), expectRowDelta: 1);
#else
        await Step(run, "S9 append leaf", () => items.Add(NewNode(nextId++, host)), expectRowDelta: 1);
        await Step(run, "S9 insert leaf (middle)", () => items.Insert(half, inserted), expectRowDelta: 1);
        await Step(run, "S9 remove leaf (middle)", () => items.RemoveAt(half), expectRowDelta: -1);
        await Step(run, "S9 reparent leaf (middle)", () => items[midLeaf] = Reparented(leaf, root));
        await Step(run, "S9 append root", () => items.Add(NewNode(nextId++, null)), expectRowDelta: 1);
#endif

        // Reshaping the expanded tree.
        await Step(run, "S10 expanded: Sort(Name)", () => source.Sort(sortBy, SortDirection.Ascending));
        Untimed(() => source.ClearSort());
        await Step(run, "S10 expanded: Filter(10%)", () => source.Filter(filter));
        Untimed(() => { source.ClearFilter(); Table.ExpandAllRows(); });
        await Step(run, $"S10 expanded: GroupBy({GroupCount})", () => source.GroupBy(groupBy));
        Untimed(() => { source.ClearGroupBy(); Table.ExpandAllRows(); });
        await Step(run, "S6 CollapseAllRows", () => Table.CollapseAllRows());
        Untimed(() => Table.ExpandAllRows());
#if HIERARCHY_API_CHILDREN
        await Step(run, "S10 expanded: clear hierarchy", () => source.ClearChildren());
#else
        await Step(run, "S10 expanded: clear hierarchy", () => source.ClearParentBy());
#endif

        Untimed(() => Table.ItemsSource = null);
        return _current;
    }

    // Children-selector layout of the same tree: every node owns a collection (empty for a leaf).
    private static void BuildChildren(List<PerfNode> nodes)
    {
        foreach (var node in nodes)
        {
            node.Children = new ObservableCollection<PerfNode>();
        }

        foreach (var node in nodes)
        {
            if (node.ParentId is int parentId)
            {
                nodes[parentId - 1].Children!.Add(node);
            }
        }
    }

    // ---- Measurement ----

    private async Task Step(int run, string name, Action op, int? expectRowDelta = null)
    {
        if (_cancelRequested)
        {
            throw new OperationCanceledException();
        }

        Collect();
        long managed0 = GC.GetTotalMemory(true), private0 = PrivateBytes();
        Table.UpdateLayout();
        _keyCalls = _parentCalls = _otherCalls = 0;
        int rows0 = VisibleRows();

        double call = double.NaN, layout = double.NaN, frame = double.NaN;
        string? error = null;
        var sw = Stopwatch.StartNew();
        try
        {
            op();
            call = sw.Elapsed.TotalMilliseconds;
            Table.UpdateLayout();
            layout = sw.Elapsed.TotalMilliseconds;
            if (await NextFrame())
            {
                frame = sw.Elapsed.TotalMilliseconds;
            }
            else
            {
                error = "no frame tick within " + FrameTimeoutMs + " ms (window hidden or minimized?)";
            }
        }
        catch (Exception ex)
        {
            error = ex.GetType().Name + ": " + ex.Message.Replace('\n', ' ').Replace('\r', ' ');
        }

        int keyCalls = _keyCalls, parentCalls = _parentCalls, otherCalls = _otherCalls;
        Collect();
        long managed1 = GC.GetTotalMemory(true), private1 = PrivateBytes();
        var sample = new Sample(call, layout, frame, keyCalls, parentCalls, otherCalls,
            (managed1 - managed0) / 1024, (private1 - private0) / 1024, VisibleRows(), error);
        // A source edit the control did not pick up costs nothing, so its timing must not be compared.
        if (error is null && expectRowDelta is int delta && sample.Rows - rows0 != delta)
        {
            sample = sample with { Error = $"edit not reflected: rows {rows0} -> {sample.Rows}, expected {delta:+0;-0;0}" };
        }
        _current.Add((name, sample));
        Log($"  run {run} {name,-40} call={call,8:F2} layout={layout,8:F2} frame={frame,8:F2} key={keyCalls} parent={parentCalls} other={otherCalls} rows={sample.Rows}{(sample.Error is null ? "" : " ERROR " + sample.Error)}");
    }

    private void Untimed(Action op)
    {
        op();
        Table.UpdateLayout();
    }

    // True when a frame tick arrived; false when the window is not rendering (e.g. minimized).
    private static async Task<bool> NextFrame()
    {
        var tcs = new TaskCompletionSource();
        EventHandler<object>? handler = null;
        handler = (_, _) =>
        {
            CompositionTarget.Rendering -= handler;
            tcs.TrySetResult();
        };
        CompositionTarget.Rendering += handler;
        var winner = await Task.WhenAny(tcs.Task, Task.Delay(FrameTimeoutMs));
        if (winner != tcs.Task)
        {
            CompositionTarget.Rendering -= handler;
            return false;
        }

        return true;
    }

    private static void Collect()
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
    }

    private static long PrivateBytes()
    {
        using var process = Process.GetCurrentProcess();
        return process.PrivateMemorySize64;
    }

    private int VisibleRows()
    {
        _repeater ??= FindDescendant<ItemsRepeater>(Table, "PART_RowsRepeater");
        return _repeater?.ItemsSourceView?.Count ?? -1;
    }

    // Expands or collapses one row through its ExpandCollapse automation pattern, the same path the
    // chevron and keyboard use. The row must be realized.
    private void ToggleRow(PerfNode node, bool expand)
    {
        var row = FindRealizedRow(node) ?? throw new InvalidOperationException($"row {node.Id} is not realized");
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

    private TableViewRow? FindRealizedRow(PerfNode node)
    {
        _repeater ??= FindDescendant<ItemsRepeater>(Table, "PART_RowsRepeater");
        if (_repeater is null)
        {
            return null;
        }

        int count = VisualTreeHelper.GetChildrenCount(_repeater);
        for (int i = 0; i < count; i++)
        {
            if (VisualTreeHelper.GetChild(_repeater, i) is TableViewRow row &&
                _repeater.GetElementIndex(row) >= 0 &&
                ReferenceEquals(row.DataContext, node))
            {
                return row;
            }
        }

        return null;
    }

    private static T? FindDescendant<T>(DependencyObject root, string name) where T : FrameworkElement
    {
        int count = VisualTreeHelper.GetChildrenCount(root);
        for (int i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(root, i);
            if (child is T match && match.Name == name)
            {
                return match;
            }

            if (FindDescendant<T>(child, name) is { } found)
            {
                return found;
            }
        }

        return null;
    }

    // ---- Selectors (counted) ----

    private TableViewKeySelector KeySelector(string keyType) => keyType switch
    {
        "int" => new(o => { _keyCalls++; return ((PerfNode)o).Id; }),
        "string" => new(o => { _keyCalls++; return ((PerfNode)o).IdText; }),
        "object" => new(o => { _keyCalls++; return ((PerfNode)o).KeyObject; }),
        _ => throw new ArgumentException("unknown key type " + keyType),
    };

    private TableViewKeySelector ParentSelector(string keyType) => keyType switch
    {
        "int" => new(o => { _parentCalls++; return ((PerfNode)o).ParentId!; }),
        "string" => new(o => { _parentCalls++; return ((PerfNode)o).ParentText!; }),
        "object" => new(o => { _parentCalls++; return ((PerfNode)o).ParentKeyObject!; }),
        _ => throw new ArgumentException("unknown key type " + keyType),
    };

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

    private static List<PerfNode> Generate(string shape, int n)
    {
        var keys = new PerfKey[n];
        for (int i = 0; i < n; i++)
        {
            keys[i] = new PerfKey(i + 1);
        }

        var list = new List<PerfNode>(n);
        for (int i = 0; i < n; i++)
        {
            int? p = ParentIndex(shape, i);
            list.Add(new PerfNode
            {
                Id = i + 1,
                ParentId = p + 1,
                IdText = "k" + (i + 1),
                ParentText = p is int pi ? "k" + (pi + 1) : null,
                KeyObject = keys[i],
                ParentKeyObject = p is int po ? keys[po] : null,
                // A scrambled but deterministic sort key, so Sort really reorders siblings.
                Name = ScrambledName(i),
                Group = "g" + (i % GroupCount),
                Value = (int)((uint)(i * 40503u) % 100),
            });
        }

        return list;
    }

    private static string ScrambledName(int i) =>
        "n" + ((uint)(i * 2654435761u) % 1_000_000).ToString("D6", CultureInfo.InvariantCulture);

    private static int MidLeafIndex(List<PerfNode> nodes, int start)
    {
        var parents = new HashSet<int>(nodes.Where(x => x.ParentId is not null).Select(x => x.ParentId!.Value));
        for (int i = start; i < nodes.Count; i++)
        {
            if (!parents.Contains(nodes[i].Id))
            {
                return i;
            }
        }

        return nodes.Count - 1;
    }

    private static PerfNode NewNode(int id, PerfNode? parent) => new()
    {
        Id = id,
        ParentId = parent?.Id,
        IdText = "k" + id,
        ParentText = parent?.IdText,
        KeyObject = new PerfKey(id),
        ParentKeyObject = parent?.KeyObject,
        Name = ScrambledName(id),
        Group = "g" + (id % GroupCount),
        Value = id % 100,
        Children = HierarchyApi == "children" ? new ObservableCollection<PerfNode>() : null,
    };

    // Same row identity (same Id / IdText / KeyObject) under a different parent.
    private static PerfNode Reparented(PerfNode node, PerfNode newParent) => new()
    {
        Id = node.Id,
        ParentId = newParent.Id,
        IdText = node.IdText,
        ParentText = newParent.IdText,
        KeyObject = node.KeyObject,
        ParentKeyObject = newParent.KeyObject,
        Name = node.Name,
        Group = node.Group,
        Value = node.Value,
    };

    // ---- Aggregation ----

    private static double Median(IEnumerable<double> values)
    {
        var sorted = values.Where(v => !double.IsNaN(v)).OrderBy(v => v).ToArray();
        return sorted.Length == 0 ? double.NaN : sorted[sorted.Length / 2];
    }

    private static Sample MedianOf(StepResult step)
    {
        var r = step.Runs;
        return new Sample(
            Median(r.Select(s => s.Call)), Median(r.Select(s => s.Layout)), Median(r.Select(s => s.Frame)),
            (int)Median(r.Select(s => (double)s.KeyCalls)), (int)Median(r.Select(s => (double)s.ParentCalls)),
            (int)Median(r.Select(s => (double)s.OtherCalls)),
            (long)Median(r.Select(s => (double)s.ManagedKb)), (long)Median(r.Select(s => (double)s.PrivateKb)),
            (int)Median(r.Select(s => (double)s.Rows)),
            r.Select(s => s.Error).FirstOrDefault(e => e is not null));
    }

    private static double FrameMax(StepResult step) =>
        step.Runs.Select(s => s.Frame).Where(v => !double.IsNaN(v)).DefaultIfEmpty(double.NaN).Max();

    // ---- Outputs ----

    private void WriteOutputs(string runDir, Options options, DateTimeOffset started, string status)
    {
        try
        {
            var json = BuildJson(options, started, status);
            var utf8 = new UTF8Encoding(false);
            File.WriteAllText(Path.Combine(runDir, "results.json"), json, utf8);
            File.WriteAllText(Path.Combine(runDir, "results.csv"), BuildCsv(), utf8);
            File.WriteAllText(Path.Combine(runDir, "results.txt"), BuildText(options), utf8);
            File.WriteAllText(Path.Combine(runDir, "report.html"), HierarchyPerfReport.Build($"Hierarchy perf: {options.Variant}", new[] { json }), utf8);
        }
        catch (Exception ex)
        {
            Log("could not write outputs: " + ex.Message);
        }
    }

    private static string WriteCompareReport(string root)
    {
        var files = Directory.Exists(root)
            ? Directory.GetFiles(root, "results.json", SearchOption.AllDirectories).OrderBy(f => f, StringComparer.OrdinalIgnoreCase).ToList()
            : new List<string>();
        if (files.Count == 0)
        {
            throw new InvalidOperationException("no results.json under " + root);
        }

        var path = Path.Combine(root, "compare.html");
        File.WriteAllText(path, HierarchyPerfReport.Build($"Hierarchy perf comparison ({files.Count} runs)", files.Select(File.ReadAllText)), new UTF8Encoding(false));
        return path;
    }

    private string BuildJson(Options options, DateTimeOffset started, string status)
    {
        using var stream = new MemoryStream();
        using (var w = new Utf8JsonWriter(stream, new JsonWriterOptions { Indented = true }))
        {
            w.WriteStartObject();
            w.WriteString("schema", Schema);
            w.WriteString("variant", options.Variant);
            w.WriteString("status", status);
            w.WriteString("started", started.ToString("o", CultureInfo.InvariantCulture));
            w.WriteString("written", DateTimeOffset.Now.ToString("o", CultureInfo.InvariantCulture));

            w.WriteStartObject("environment");
            foreach (var (k, v) in Environment())
            {
                w.WriteString(k, v);
            }

            w.WriteEndObject();

            w.WriteStartObject("options");
            w.WriteStartArray("counts");
            foreach (var c in options.Counts)
            {
                w.WriteNumberValue(c);
            }

            w.WriteEndArray();
            WriteStrings(w, "shapes", options.Shapes);
            WriteStrings(w, "keys", options.KeyTypes);
            w.WriteNumber("runs", options.Runs);
            w.WriteNumber("warmupRuns", 1);
            w.WriteEndObject();

            w.WriteStartArray("results");
            foreach (var config in _results)
            {
                foreach (var step in config.Steps)
                {
                    w.WriteStartObject();
                    w.WriteNumber("n", config.N);
                    w.WriteString("shape", config.Shape);
                    w.WriteString("key", config.Key);
                    w.WriteString("step", step.Step);
                    var median = MedianOf(step);
                    w.WritePropertyName("median");
                    WriteSample(w, median, FrameMax(step));
                    w.WriteStartArray("runs");
                    foreach (var s in step.Runs)
                    {
                        WriteSample(w, s, null);
                    }

                    w.WriteEndArray();
                    w.WriteEndObject();
                }
            }

            w.WriteEndArray();
            w.WriteEndObject();
        }

        return Encoding.UTF8.GetString(stream.ToArray());
    }

    private static void WriteStrings(Utf8JsonWriter w, string name, IEnumerable<string> values)
    {
        w.WriteStartArray(name);
        foreach (var v in values)
        {
            w.WriteStringValue(v);
        }

        w.WriteEndArray();
    }

    private static void WriteSample(Utf8JsonWriter w, Sample s, double? frameMax)
    {
        static void Number(Utf8JsonWriter w, string name, double v)
        {
            if (double.IsNaN(v))
            {
                w.WriteNull(name);
            }
            else
            {
                w.WriteNumber(name, Math.Round(v, 3));
            }
        }

        w.WriteStartObject();
        Number(w, "call", s.Call);
        Number(w, "layout", s.Layout);
        Number(w, "frame", s.Frame);
        if (frameMax is double max)
        {
            Number(w, "frameMax", max);
        }

        w.WriteNumber("keyCalls", s.KeyCalls);
        w.WriteNumber("parentCalls", s.ParentCalls);
        w.WriteNumber("otherCalls", s.OtherCalls);
        w.WriteNumber("managedKb", s.ManagedKb);
        w.WriteNumber("privateKb", s.PrivateKb);
        w.WriteNumber("rows", s.Rows);
        if (s.Error is not null)
        {
            w.WriteString("error", s.Error);
        }

        w.WriteEndObject();
    }

    private string BuildCsv()
    {
        var csv = new StringBuilder();
        csv.AppendLine("n,shape,key,runs,step,call_ms,layout_ms,frame_ms,frame_max_ms,key_calls,parent_calls,other_calls,managed_kb,private_kb,rows,error");
        foreach (var config in _results)
        {
            foreach (var step in config.Steps)
            {
                var m = MedianOf(step);
                csv.AppendLine(string.Join(",",
                    config.N, config.Shape, config.Key, step.Runs.Count, Csv(step.Step),
                    F(m.Call), F(m.Layout), F(m.Frame), F(FrameMax(step)),
                    m.KeyCalls, m.ParentCalls, m.OtherCalls, m.ManagedKb, m.PrivateKb, m.Rows, Csv(m.Error ?? string.Empty)));
            }
        }

        return csv.ToString();
    }

    private string BuildText(Options options)
    {
        var text = new StringBuilder();
        text.AppendLine($"hierarchy perf  variant={options.Variant}  runs={options.Runs} (+1 warm-up, medians)  time=ms memory=KB");
        foreach (var config in _results)
        {
            text.AppendLine();
            text.AppendLine($"== n={config.N} shape={config.Shape} key={config.Key}");
            text.AppendLine($"{"step",-40}{"call",9}{"layout",9}{"frame",9}{"fmax",9}{"key",8}{"parent",8}{"other",8}{"dMgdKB",9}{"dPrvKB",9}{"rows",8}  error");
            foreach (var step in config.Steps)
            {
                var m = MedianOf(step);
                text.AppendLine($"{step.Step,-40}{m.Call,9:F1}{m.Layout,9:F1}{m.Frame,9:F1}{FrameMax(step),9:F1}{m.KeyCalls,8}{m.ParentCalls,8}{m.OtherCalls,8}{m.ManagedKb,9}{m.PrivateKb,9}{m.Rows,8}  {m.Error}");
            }
        }

        return text.ToString();
    }

    private static string F(double v) => double.IsNaN(v) ? string.Empty : v.ToString("F2", CultureInfo.InvariantCulture);

    private static string Csv(string value) =>
        value.IndexOfAny(new[] { ',', '"' }) >= 0 ? "\"" + value.Replace("\"", "\"\"") + "\"" : value;

    // ---- Environment / logging ----

    private static IEnumerable<(string, string)> Environment()
    {
        yield return ("hierarchyApi", HierarchyApi);
        yield return ("tabularSha256", TabularSha256() ?? "(not found)");
        yield return ("machine", System.Environment.MachineName);
        yield return ("os", System.Environment.OSVersion.VersionString);
        yield return ("cpus", System.Environment.ProcessorCount.ToString(CultureInfo.InvariantCulture));
        yield return ("clr", System.Environment.Version.ToString());
        yield return ("process", System.Environment.Is64BitProcess ? "x64" : "x86");
        yield return ("sampleConfiguration", BuildFlavor());
        yield return ("appDirectory", AppContext.BaseDirectory);
        foreach (var dll in new[] { "Microsoft.UI.Xaml.Controls.Tabular.dll", "Microsoft.UI.Xaml.Controls.dll", "Microsoft.ui.xaml.dll" })
        {
            var info = DllInfo(dll);
            yield return (dll, info);
        }
    }

    private static string DllInfo(string name)
    {
        var path = Path.Combine(AppContext.BaseDirectory, name);
        if (!File.Exists(path))
        {
            return "(not found)";
        }

        var version = FileVersionInfo.GetVersionInfo(path);
        return $"{version.FileVersion} written={File.GetLastWriteTimeUtc(path):o}";
    }

    // "<api>-<first 8 hex of the Tabular DLL's SHA-256>": distinguishes builds even when the DLL's
    // version string is stale (it is stamped at init time, not per build).
    private static string DefaultVariant()
    {
        var sha = TabularSha256();
        return sha is null ? HierarchyApi : $"{HierarchyApi}-{sha.Substring(0, 8)}";
    }

    private static string? TabularSha256()
    {
        var path = Path.Combine(AppContext.BaseDirectory, "Microsoft.UI.Xaml.Controls.Tabular.dll");
        if (!File.Exists(path))
        {
            return null;
        }

        using var stream = File.OpenRead(path);
        return Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(stream)).ToLowerInvariant();
    }

    private static string Sanitize(string value)
    {
        var invalid = Path.GetInvalidFileNameChars();
        var cleaned = new string(value.Select(c => invalid.Contains(c) || char.IsWhiteSpace(c) ? '_' : c).ToArray());
        return cleaned.Length == 0 ? "unlabeled" : cleaned;
    }

    private void Log(string message)
    {
        try
        {
            _log?.WriteLine($"[{DateTime.Now:HH:mm:ss.fff}] {message}");
        }
        catch (IOException)
        {
        }
    }

    private static void WriteCrashNote(string message)
    {
        try
        {
            Directory.CreateDirectory(DefaultOutputRoot);
            File.AppendAllText(Path.Combine(DefaultOutputRoot, "autorun-errors.log"), $"[{DateTime.Now:o}] {message}{System.Environment.NewLine}");
        }
        catch (IOException)
        {
        }
    }

    private void SetStatus(string text) => StatusText.Text = text;

    private static string BuildFlavor()
    {
#if DEBUG
        return "Debug";
#else
        return "Release";
#endif
    }
}
