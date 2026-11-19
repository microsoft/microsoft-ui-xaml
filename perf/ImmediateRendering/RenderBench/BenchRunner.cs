using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;
using Windows.Graphics;

namespace RenderBench;

// Drives one run: Warmup -> Steady -> Interaction -> Recovery, emitting Microsoft-WinUI-RenderBench markers that the
// analyzer uses to window the native frame/paint/resource events (test plan, "Run protocol").
sealed class BenchRunner(BenchOptions options)
{
    readonly Window _window = new() { Title = "RenderBench" };
    readonly DispatcherQueue _queue = DispatcherQueue.GetForCurrentThread();
    readonly MemorySampler _sampler = new();
    readonly ThreadCpuSampler _threadSampler = new(); // immediate-rendering-d2d-sw: W1/W2 CPU split
    IScene? _scene;
    nint _hwnd;
    volatile int _phase;
    Timer? _sampleTimer;
    long _inputId;
    long _tick;

    public void Start() => _ = RunAsync();

    async Task RunAsync()
    {
        int status = 0;
        string detail = "ok";
        try
        {
            _hwnd = WinRT.Interop.WindowNative.GetWindowHandle(_window);
            _window.AppWindow.MoveAndResize(new RectInt32(40, 40, options.Width, options.Height));
            _scene = SceneFactory.Create(options);

            var ready = new TaskCompletionSource();
            _scene.Root.Loaded += (_, _) => _queue.TryEnqueue(DispatcherQueuePriority.Low, () => ready.TrySetResult());
            _window.Content = _scene.Root;
            _window.Activate();
            await ready.Task;
            Win32.ForceForeground(_hwnd);

            BenchEventSource.Log.RunStarted(options.SceneLabel, Environment.GetEnvironmentVariable(BackendModes.EnvironmentVariable) ?? "",
                options.Seed, options.Width, options.Height, _scene.Root.XamlRoot.RasterizationScale);
            BenchEventSource.Log.SceneReady(_scene.Name, CountElements(_scene.Root));
            _threadSampler.CaptureUiThread(); // immediate-rendering-d2d-sw
            _sampleTimer = new Timer(_ => SampleMemory(), null, 0, options.SampleIntervalMs);

            // Warmup touches every input target once and applies a few updates so lazy caches (glyphs, brushes,
            // templates' visual states) exist before Steady is measured.
            await Phase(BenchPhase.Warmup, options.WarmupSeconds, async () =>
            {
                foreach (var target in _scene.InputTargets)
                {
                    Inject(target, "warmup");
                    await Task.Delay(50);
                }
                for (int i = 0; i < 5 && _scene.IsDynamic; i++)
                {
                    Update();
                    await Task.Delay(50);
                }
            });

            await Phase(BenchPhase.Steady, options.SettleSeconds, () => Task.CompletedTask);

            await Phase(BenchPhase.Interaction, options.InteractionSeconds, async () =>
            {
                using var stop = new CancellationTokenSource(TimeSpan.FromSeconds(options.InteractionSeconds));
                var updates = _scene.IsDynamic ? Repeat(options.UpdateHz, Update) : null;
                _tick = 0;
                int next = 0;
                var inputs = options.InputHz <= 0 || _scene.InputTargets.Count == 0
                    ? null
                    : Repeat(options.InputHz, () => Inject(_scene.InputTargets[next++ % _scene.InputTargets.Count], "click"));
                updates?.Start();
                inputs?.Start();
                try
                {
                    await Task.Delay(Timeout.Infinite, stop.Token);
                }
                catch (OperationCanceledException)
                {
                }
                updates?.Stop();
                inputs?.Stop();
            });

            await Phase(BenchPhase.Recovery, options.RecoverySeconds, () => Task.CompletedTask);

            if (options.ScreenshotPath is { } path)
            {
                var (width, height) = Win32.SaveClientScreenshot(_hwnd, path);
                BenchEventSource.Log.ScreenshotSaved(Path.GetFullPath(path), width, height, "Screen");
            }
        }
        catch (Exception ex)
        {
            status = 1;
            detail = ex.ToString();
            Console.Error.WriteLine(ex);
        }
        finally
        {
            if (_sampleTimer is not null)
            {
                using var stopped = new ManualResetEvent(false);
                _sampleTimer.Dispose(stopped);
                stopped.WaitOne(1000);
            }
            EmitThreadSummary(); // immediate-rendering-d2d-sw
            BenchEventSource.Log.RunCompleted(status, detail);
            _sampler.Dispose();
            if (options.Exit)
            {
                Application.Current.Exit();
            }
        }
    }

    // A phase lasts at least `seconds`; scripted activity runs inside it from the start.
    async Task Phase(BenchPhase phase, double seconds, Func<Task> activity)
    {
        _phase = (int)phase;
        BenchEventSource.Log.PhaseBegin((int)phase, phase.ToString());
        SampleMemory();
        var minimum = Task.Delay(TimeSpan.FromSeconds(seconds));
        await activity();
        await minimum;
        SampleMemory();
        BenchEventSource.Log.PhaseEnd((int)phase, phase.ToString());
    }

    DispatcherQueueTimer Repeat(double hz, Action action)
    {
        var timer = _queue.CreateTimer();
        timer.Interval = TimeSpan.FromSeconds(1.0 / hz);
        timer.IsRepeating = true;
        timer.Tick += (_, _) => action();
        return timer;
    }

    void Update()
    {
        int changed = _scene!.ApplyUpdate();
        BenchEventSource.Log.UpdateApplied(_tick++, changed);
    }

    void Inject(FrameworkElement target, string kind)
    {
        var (x, y) = Win32.ElementCenterOnScreen(target, _hwnd);
        BenchEventSource.Log.InputInjected(++_inputId, kind, target.GetType().Name);
        Win32.Click(x, y);
    }

    void SampleMemory()
    {
        var s = _sampler.Take();
        BenchEventSource.Log.MemorySample(_phase, s.PrivateBytes, s.WorkingSetBytes, s.PeakWorkingSetBytes, s.GpuLocalBytes,
            s.GpuNonLocalBytes, s.GdiObjects, s.UserObjects, s.CpuUserMs, s.CpuKernelMs);
        // D2D-HW branch: per-adapter GPU memory, so M3 can be read for the adapter WinUI renders with.
        foreach (var adapter in s.Adapters)
        {
            BenchEventSource.Log.GpuAdapterMemorySample(_phase, adapter.Luid, adapter.LocalBytes, adapter.NonLocalBytes);
        }
        // ---- Added by immediate-rendering-d2d-sw: per-thread CPU split ----
        var (ms, threads) = _threadSampler.Take();
        BenchEventSource.Log.ThreadCpuSample(_phase, ms[(int)ThreadCategory.Ui], ms[(int)ThreadCategory.WarpRasterizer],
            ms[(int)ThreadCategory.D2DDWrite], ms[(int)ThreadCategory.Composition], ms[(int)ThreadCategory.Other], threads);
        // ---- End immediate-rendering-d2d-sw ----
    }

    // Added by immediate-rendering-d2d-sw: what actually ran, for checking ThreadCategories against reality.
    void EmitThreadSummary()
    {
        try
        {
            _threadSampler.Take();
            foreach (var m in _threadSampler.Modules())
            {
                BenchEventSource.Log.ThreadModuleSummary(m.Module, m.Category.ToString(), m.Threads, m.CpuMs);
            }
            foreach (var (module, version) in ThreadCpuSampler.RasterModuleVersions())
            {
                BenchEventSource.Log.RasterModuleVersion(module, version);
            }
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine(ex);
        }
    }

    static int CountElements(DependencyObject root)
    {
        int count = 1;
        int children = VisualTreeHelper.GetChildrenCount(root);
        for (int i = 0; i < children; i++)
        {
            count += CountElements(VisualTreeHelper.GetChild(root, i));
        }
        return count;
    }
}
