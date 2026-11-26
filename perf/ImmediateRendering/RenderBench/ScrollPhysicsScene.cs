using System.Diagnostics;
using System.Globalization;
using System.Runtime.InteropServices;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using RenderBench.Coverage;
using Windows.Foundation;
using Windows.UI;

namespace RenderBench;

// S33: scroll physics. DirectManipulation-driven ScrollViewer motion (wheel, touch flick with inertia, pinch zoom,
// keyboard PgDn, mandatory snap points, BringIntoView/ChangeView animations), MUXC ScrollView (InteractionTracker) wheel
// and flick, and a flick during which the UI thread blocks for 200 ms. Input is real (SendInput / InjectTouchInput);
// a step whose injection fails reports Injected=0. One step per case; each step resets the view, injects, records the
// offset trajectory (ScrollViewer.ViewChanging / ScrollView.ViewChanged with QPC timestamps) until 500 ms without
// change, then idles 1.5 s. Values per step (MetricsCalculator.Decision reads them):
//   Injected, Samples, StartLatencyMs, MotionMs, UpdateHz, P95IntervalMs, MaxIntervalMs, FinalOffset, FinalZoom,
//   MotionCpuMsPerSecond, IdleCpuMsPerSecond, PrivateMB, PeakPrivateMB, and from the native logs when set
//   (WINUI_IMMEDIATE_DMANIP_LOG, WINUI_IMMEDIATE_STATS): MotionDManipUpdates, IdleDManipUpdates, MotionPaintedFrames,
//   IdlePaintedFrames. The blocked step adds BlockScreenChanges/BlockScreenSamples (screen readbacks of a viewport column
//   that differed from the previous one, and the readback count, while the UI thread sleeps; one readback takes ~50 ms on
//   a GPU-less VM), BlockDManipUpdates and OffsetJumpAfterBlock; the unblocked flick reports the same values for the
//   200 ms after the same delay (the sampler's reference).
// The trajectory of every step is written to trajectory.csv next to --screenshot (step,ms since input,offset,zoom).
sealed class ScrollPhysicsScene : DecisionScene, IScene
{
    public const double RowHeight = 60, SnapRowHeight = 120;
    public const int Rows = 300;
    public static readonly string[] Steps =
    [
        "wheel-1", "wheel-3", "flick", "pinch", "pgdn", "snap", "bringintoview", "changeview", "sv-wheel", "sv-flick",
        "flick-blocked",
    ];

    readonly ScrollViewer _viewer;
    readonly ScrollViewer _snapViewer;
    readonly ScrollView _scrollView;
    readonly StackPanel _rows;
    readonly Grid _root;
    readonly List<string> _csv = ["step,ms,offset,zoom"];
    readonly string? _statsPath = Environment.GetEnvironmentVariable("WINUI_IMMEDIATE_STATS");
    readonly string? _dmanipPath = Environment.GetEnvironmentVariable("WINUI_IMMEDIATE_DMANIP_LOG");
    ISceneHost? _host;

    public ScrollPhysicsScene(BenchOptions options) : base(options)
    {
        _rows = RowsPanel(RowHeight);
        _viewer = new ScrollViewer
        {
            Content = _rows, ZoomMode = ZoomMode.Enabled, MinZoomFactor = 0.5f, MaxZoomFactor = 4f, IsTabStop = true,
            HorizontalScrollMode = ScrollMode.Enabled, HorizontalScrollBarVisibility = ScrollBarVisibility.Auto,
            VerticalScrollMode = ScrollMode.Enabled, VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
        };
        _snapViewer = new ScrollViewer
        {
            Content = RowsPanel(SnapRowHeight), VerticalSnapPointsType = SnapPointsType.Mandatory,
            VerticalSnapPointsAlignment = SnapPointsAlignment.Near, Visibility = Visibility.Collapsed,
        };
        _scrollView = new ScrollView { Content = RowsPanel(RowHeight), Visibility = Visibility.Collapsed };
        _root = Page(new Grid { Children = { _viewer, _snapViewer, _scrollView } });
    }

    static StackPanel RowsPanel(double height)
    {
        var panel = new StackPanel();
        for (int i = 0; i < Rows; i++)
        {
            panel.Children.Add(new Border
            {
                Height = height,
                Background = new SolidColorBrush(Color.FromArgb(255, (byte)(60 + i * 37 % 180), (byte)(60 + i * 71 % 180), (byte)(60 + i * 113 % 180))),
                BorderBrush = new SolidColorBrush(Colors.White), BorderThickness = new Thickness(0, 0, 0, 1),
                Child = new TextBlock { Text = $"Row {i}", FontSize = 20, Margin = new Thickness(16, 12, 0, 0), Foreground = new SolidColorBrush(Colors.White) },
            });
        }
        return panel;
    }

    public override string Name => "ScrollPhysics";
    public override FrameworkElement Root => _root;
    public bool ContinuousInteraction => true;

    public void Attach(ISceneHost host) => _host = host;

    // ---- recording ----
    sealed class Recorder
    {
        public readonly List<(long Ticks, double Offset, double Zoom)> Samples = [];
        public long InputTicks;
        public void Add(double offset, double zoom)
        {
            if (Samples.Count == 0 || Samples[^1].Offset != offset || Samples[^1].Zoom != zoom)
            {
                Samples.Add((Stopwatch.GetTimestamp(), offset, zoom));
            }
        }
        public long LastChange => Samples.Count == 0 ? 0 : Samples[^1].Ticks;
    }

    static double Ms(long from, long to) => (to - from) * 1000.0 / Stopwatch.Frequency;
    static long QpcMicroseconds() => (long)(Stopwatch.GetTimestamp() * (1_000_000.0 / Stopwatch.Frequency));

    // The native DManip log uses QPC microseconds too; counts "tick" lines in [fromUs, toUs).
    int CountTicks(long fromUs, long toUs)
    {
        if (_dmanipPath is null || !File.Exists(_dmanipPath))
        {
            return -1;
        }
        int count = 0;
        foreach (var line in ReadShared(_dmanipPath))
        {
            if (line.StartsWith("tick ", StringComparison.Ordinal) && long.TryParse(line.AsSpan(5, line.IndexOf(' ', 5) - 5), out long us) && us >= fromUs && us < toUs)
            {
                count++;
            }
        }
        return count;
    }

    int StatsFrames()
    {
        if (_statsPath is null || !File.Exists(_statsPath))
        {
            return -1;
        }
        return ReadShared(_statsPath).Count(l => l.Length > 0 && char.IsDigit(l[0]));
    }

    static List<string> ReadShared(string path)
    {
        try
        {
            using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
            using var reader = new StreamReader(stream);
            var lines = new List<string>();
            while (reader.ReadLine() is { } line)
            {
                lines.Add(line);
            }
            return lines;
        }
        catch (IOException)
        {
            return [];
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    struct ProcessMemoryCounters
    {
        public uint Size, PageFaultCount;
        public nuint PeakWorkingSetSize, WorkingSetSize, QuotaPeakPagedPoolUsage, QuotaPagedPoolUsage;
        public nuint QuotaPeakNonPagedPoolUsage, QuotaNonPagedPoolUsage, PagefileUsage, PeakPagefileUsage;
    }

    struct ScreenPoint { public int X, Y; }

    [DllImport("psapi.dll")]
    static extern bool GetProcessMemoryInfo(nint process, out ProcessMemoryCounters counters, uint size);
    [DllImport("user32.dll")]
    static extern nint GetDC(nint hwnd);
    [DllImport("user32.dll")]
    static extern int ReleaseDC(nint hwnd, nint dc);
    [DllImport("gdi32.dll")]
    static extern nint CreateCompatibleDC(nint dc);
    [DllImport("gdi32.dll")]
    static extern nint CreateCompatibleBitmap(nint dc, int width, int height);
    [DllImport("gdi32.dll")]
    static extern nint SelectObject(nint dc, nint obj);
    [DllImport("gdi32.dll")]
    static extern bool DeleteObject(nint obj);
    [DllImport("gdi32.dll")]
    static extern bool DeleteDC(nint dc);
    [DllImport("gdi32.dll")]
    static extern bool BitBlt(nint dst, int x, int y, int width, int height, nint src, int srcX, int srcY, uint rop);
    [DllImport("gdi32.dll")]
    static extern int GetBitmapBits(nint bitmap, int bytes, byte[] bits);
    [DllImport("user32.dll")]
    static extern bool ClientToScreen(nint hwnd, ref ScreenPoint point);
    [DllImport("user32.dll")]
    static extern uint GetDpiForWindow(nint hwnd);

    static (double Private, double Peak) Memory()
    {
        GetProcessMemoryInfo(Process.GetCurrentProcess().Handle, out var c, (uint)Marshal.SizeOf<ProcessMemoryCounters>());
        return (c.PagefileUsage / 1048576.0, c.PeakPagefileUsage / 1048576.0);
    }

    // ---- steps ----
    protected override async Task RunStepsAsync(ISceneHost host)
    {
        await Frames(10);
        for (int i = 0; i < Steps.Length; i++)
        {
            await RunStepAsync(host, i, Steps[i]);
        }
        if (Options.ScreenshotPath is { } shot && Path.GetDirectoryName(Path.GetFullPath(shot)) is { } dir)
        {
            File.WriteAllLines(Path.Combine(dir, "trajectory.csv"), _csv);
        }
    }

    FrameworkElement Show(string name)
    {
        FrameworkElement target = name.StartsWith("sv-", StringComparison.Ordinal) ? _scrollView : name == "snap" ? _snapViewer : _viewer;
        foreach (var e in new FrameworkElement[] { _viewer, _snapViewer, _scrollView })
        {
            e.Visibility = e == target ? Visibility.Visible : Visibility.Collapsed;
        }
        return target;
    }

    Point Center(FrameworkElement target, double dy = 0) =>
        target.TransformToVisual(null).TransformPoint(new Point(target.ActualWidth / 2, target.ActualHeight / 2 + dy));

    async Task RunStepAsync(ISceneHost host, int index, string name)
    {
        var target = Show(name);
        _viewer.ChangeView(0, 0, 1, disableAnimation: true);
        _snapViewer.ChangeView(0, 0, null, disableAnimation: true);
        _scrollView.ScrollTo(0, 0, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled));
        await Frames(4);
        await Task.Delay(400);
        await Frames(2);

        host.Step(index, name, 1);
        var recorder = new Recorder();
        void Record()
        {
            if (target is ScrollViewer sv)
            {
                recorder.Add(sv.VerticalOffset, sv.ZoomFactor);
            }
            else
            {
                recorder.Add(_scrollView.VerticalOffset, _scrollView.ZoomFactor);
            }
        }
        EventHandler<ScrollViewerViewChangingEventArgs> changing = (_, e) =>
            recorder.Add(e.NextView.VerticalOffset, e.NextView.ZoomFactor);
        EventHandler<ScrollViewerViewChangedEventArgs> changed = (_, _) => Record();
        Windows.Foundation.TypedEventHandler<ScrollView, object> svChanged = (_, _) => Record();
        if (target is ScrollViewer viewer)
        {
            viewer.ViewChanging += changing;
            viewer.ViewChanged += changed;
        }
        else
        {
            _scrollView.ViewChanged += svChanged;
        }
        Record();
        recorder.Samples.Clear();

        int statsBefore = StatsFrames();
        var cpuBefore = Cpu();
        long inputUs = QpcMicroseconds();
        recorder.InputTicks = Stopwatch.GetTimestamp();
        bool injected = true;
        int blockChanges = -1, blockTicks = -1;
        double blockJump = double.NaN;
        var at = Center(target);
        switch (name)
        {
            case "wheel-1":
            case "sv-wheel":
                host.MarkInput("wheel", target.GetType().Name, programmatic: false);
                injected = InputInjection.Wheel(host.Window, at, -120);
                break;
            case "wheel-3":
                host.MarkInput("wheel", "ScrollViewer", programmatic: false);
                for (int n = 0; n < 3 && injected; n++)
                {
                    injected = InputInjection.Wheel(host.Window, at, -120);
                    await Task.Delay(50);
                }
                break;
            case "flick":
                host.MarkInput("touch", "ScrollViewer", programmatic: false);
                injected = await InputInjection.DragAsync(host.Window, Center(target, 120), 0, -240, steps: 6);
                await Task.Delay(60);
                (blockChanges, blockTicks) = await SampleScreenAsync(host, target, 200, block: false);
                break;
            case "snap":
            case "sv-flick":
                host.MarkInput("touch", target.GetType().Name, programmatic: false);
                injected = await InputInjection.DragAsync(host.Window, Center(target, 120), 0, -240, steps: 6);
                break;
            case "pinch":
                host.MarkInput("pinch", "ScrollViewer", programmatic: false);
                injected = await InputInjection.PinchAsync(host.Window, at, 100, 250, steps: 15);
                break;
            case "pgdn":
                _viewer.Focus(FocusState.Keyboard);
                await Frames(2);
                recorder.InputTicks = Stopwatch.GetTimestamp();
                host.MarkInput("key", "ScrollViewer", programmatic: false);
                injected = InputInjection.Key(0x22 /*VK_NEXT*/);
                break;
            case "bringintoview":
                host.MarkInput("bringintoview", "ScrollViewer", programmatic: true);
                ((FrameworkElement)_rows.Children[40]).StartBringIntoView(new BringIntoViewOptions { AnimationDesired = true, VerticalAlignmentRatio = 0 });
                break;
            case "changeview":
                host.MarkInput("changeview", "ScrollViewer", programmatic: true);
                _viewer.ChangeView(null, 2400, null, disableAnimation: false);
                break;
            case "flick-blocked":
            {
                host.MarkInput("touch", "ScrollViewer", programmatic: false);
                injected = await InputInjection.DragAsync(host.Window, Center(target, 120), 0, -240, steps: 6);
                await Task.Delay(60);
                double before = recorder.Samples.Count > 0 ? recorder.Samples[^1].Offset : 0;
                int sampleCountBefore = recorder.Samples.Count;
                (blockChanges, blockTicks) = await SampleScreenAsync(host, target, 200, block: true);
                await Frames(1);
                Record();
                if (recorder.Samples.Count > sampleCountBefore)
                {
                    blockJump = recorder.Samples[sampleCountBefore].Offset - before;
                }
                break;
            }
        }

        // Settled: 500 ms without a change (or 6 s).
        var timeout = Stopwatch.StartNew();
        while (timeout.Elapsed.TotalSeconds < 6)
        {
            await Task.Delay(50);
            Record();
            long last = Math.Max(recorder.LastChange, recorder.InputTicks);
            if (Ms(last, Stopwatch.GetTimestamp()) > 500)
            {
                break;
            }
        }
        long settleTicks = recorder.LastChange == 0 ? recorder.InputTicks : recorder.LastChange;
        long settleUs = (long)(settleTicks * (1_000_000.0 / Stopwatch.Frequency));
        var cpuSettled = Cpu();
        double motionWallMs = Ms(recorder.InputTicks, Stopwatch.GetTimestamp());
        int statsSettled = StatsFrames();
        long idleFromUs = QpcMicroseconds();
        await Task.Delay(1500);
        var cpuIdle = Cpu();
        long idleToUs = QpcMicroseconds();
        int statsIdle = StatsFrames();

        if (target is ScrollViewer v)
        {
            v.ViewChanging -= changing;
            v.ViewChanged -= changed;
        }
        else
        {
            _scrollView.ViewChanged -= svChanged;
        }

        var samples = recorder.Samples;
        var intervals = new List<double>();
        for (int s = 1; s < samples.Count; s++)
        {
            intervals.Add(Ms(samples[s - 1].Ticks, samples[s].Ticks));
        }
        intervals.Sort();
        double motionMs = samples.Count > 1 ? Ms(samples[0].Ticks, samples[^1].Ticks) : 0;
        foreach (var s in samples)
        {
            _csv.Add(string.Create(CultureInfo.InvariantCulture, $"{name},{Ms(recorder.InputTicks, s.Ticks):F1},{s.Offset:F2},{s.Zoom:F4}"));
        }
        var (privateMB, peakMB) = Memory();
        double finalOffset = target is ScrollViewer fv ? fv.VerticalOffset : _scrollView.VerticalOffset;
        double finalZoom = target is ScrollViewer fz ? fz.ZoomFactor : _scrollView.ZoomFactor;

        Value("Injected", injected ? 1 : 0);
        Value("Samples", samples.Count);
        Value("StartLatencyMs", samples.Count > 0 ? Ms(recorder.InputTicks, samples[0].Ticks) : double.NaN);
        Value("MotionMs", motionMs);
        Value("UpdateHz", motionMs > 0 ? (samples.Count - 1) / (motionMs / 1000) : 0);
        Value("P95IntervalMs", intervals.Count > 0 ? intervals[Math.Min(intervals.Count - 1, (int)Math.Ceiling(intervals.Count * 0.95) - 1)] : double.NaN);
        Value("MaxIntervalMs", intervals.Count > 0 ? intervals[^1] : double.NaN);
        Value("FinalOffset", finalOffset);
        Value("FinalZoom", finalZoom);
        Value("MotionCpuMsPerSecond", motionWallMs > 0 ? (cpuSettled.Process - cpuBefore.Process) / (motionWallMs / 1000) : double.NaN);
        Value("IdleCpuMsPerSecond", (cpuIdle.Process - cpuSettled.Process) / 1.5);
        Value("PrivateMB", privateMB);
        Value("PeakPrivateMB", peakMB);
        Value("MotionDManipUpdates", CountTicks(inputUs, settleUs + 1));
        Value("IdleDManipUpdates", CountTicks(idleFromUs, idleToUs));
        Value("MotionPaintedFrames", statsBefore < 0 ? -1 : statsSettled - statsBefore);
        Value("IdlePaintedFrames", statsSettled < 0 ? -1 : statsIdle - statsSettled);
        if (name is "flick-blocked" or "flick")
        {
            Value("BlockScreenChanges", blockChanges);
            Value("BlockScreenSamples", _lastScreenSamples);
            Value("BlockDManipUpdates", blockTicks);
            Value("OffsetJumpAfterBlock", blockJump);
        }
        Console.Error.WriteLine(string.Create(CultureInfo.InvariantCulture,
            $"S33 {name}: injected={injected} samples={samples.Count} motion={motionMs:F0}ms final={finalOffset:F1} zoom={finalZoom:F3} idleTicks={CountTicks(idleFromUs, idleToUs)} block={blockChanges}/{blockTicks}/{blockJump:F1}"));
        await Hold(seconds: 0.2);
    }

    // Samples 8 screen pixels down the viewport's left-third column every 4 ms on a background thread and counts the
    // distinct consecutive samples: frames that reached the screen, independent of which thread produced them.
    sealed class ScreenSampler : IDisposable
    {
        readonly ManualResetEventSlim _stop = new();
        readonly Thread _thread;
        int _changes, _samples;
        uint _firstPixel;
        string _where = "";

        public ScreenSampler(ISceneHost host, FrameworkElement target)
        {
            double scale = GetDpiForWindow(host.Hwnd) / 96.0;
            var origin = target.TransformToVisual(null).TransformPoint(new Point(target.ActualWidth / 3, 0));
            var client = new ScreenPoint();
            ClientToScreen(host.Hwnd, ref client);
            int x = client.X + (int)(origin.X * scale);
            int top = client.Y + (int)((origin.Y + 20) * scale);
            int height = Math.Max(8, (int)((target.ActualHeight - 40) * scale));
            const int width = 4;
            _where = $"{x},{top}+{height}";
            _thread = new Thread(() =>
            {
                nint screen = GetDC(0);
                nint memory = CreateCompatibleDC(screen);
                nint bitmap = CreateCompatibleBitmap(screen, width, height);
                nint old = SelectObject(memory, bitmap);
                var bits = new byte[width * height * 4];
                ulong previous = 0;
                bool first = true;
                while (!_stop.IsSet)
                {
                    // One readback per sample: per-pixel GetPixel on the screen DC takes tens of ms on this VM.
                    BitBlt(memory, 0, 0, width, height, screen, x, top, 0x00CC0020 /* SRCCOPY */);
                    GetBitmapBits(bitmap, bits.Length, bits);
                    ulong hash = 1469598103934665603;
                    foreach (byte b in bits)
                    {
                        hash = (hash ^ b) * 1099511628211;
                    }
                    if (!first && hash != previous)
                    {
                        _changes++;
                    }
                    if (first)
                    {
                        _firstPixel = BitConverter.ToUInt32(bits, 0);
                    }
                    _samples++;
                    previous = hash;
                    first = false;
                    Thread.Sleep(2);
                }
                SelectObject(memory, old);
                DeleteObject(bitmap);
                DeleteDC(memory);
                ReleaseDC(0, screen);
            }) { IsBackground = true };
            _thread.Start();
        }

        public int Stop()
        {
            _stop.Set();
            _thread.Join();
            Console.Error.WriteLine($"S33 sampler at {_where}: samples={_samples} changes={_changes} first=0x{_firstPixel:X8}");
            return _changes;
        }

        public int Samples => _samples;

        public void Dispose() => _stop.Dispose();
    }

    // Samples the screen for ms while the UI thread sleeps (block) or awaits (free); returns the screen changes and the
    // DManip ticks logged meanwhile.
    int _lastScreenSamples;

    async Task<(int ScreenChanges, int Ticks)> SampleScreenAsync(ISceneHost host, FrameworkElement target, int ms, bool block)
    {
        long fromUs = QpcMicroseconds();
        using var sampler = new ScreenSampler(host, target);
        if (block)
        {
            Thread.Sleep(ms);
        }
        else
        {
            await Task.Delay(ms);
        }
        int changes = sampler.Stop();
        _lastScreenSamples = sampler.Samples;
        return (changes, CountTicks(fromUs, QpcMicroseconds()));
    }
}
