using System.Diagnostics;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;
using Windows.Graphics;

namespace RenderBench;

// Drives one run: Warmup -> Steady -> Interaction -> Recovery, emitting Microsoft-WinUI-RenderBench markers that the
// analyzer uses to window the native frame/paint/resource events (test plan, "Run protocol").
sealed class BenchRunner(BenchOptions options) : ISceneHost
{
    readonly Window _window = new() { Title = "RenderBench" };
    readonly DispatcherQueue _queue = DispatcherQueue.GetForCurrentThread();
    readonly MemorySampler _sampler = new();
    readonly DwmSampler _dwmSampler = new();
    readonly ThreadCpuSampler _threadSampler = new(); // immediate-rendering-d2d-sw: W1/W2 CPU split
    readonly object _sampleLock = new();
    IScene? _scene;
    nint _hwnd;
    volatile int _phase;
    Timer? _sampleTimer;
    DispatcherQueueTimer? _counterTimer;
    long _inputId;
    long _tick;
    InputMode _inputMode = InputMode.Programmatic;
    // Outside every phase: long enough for a state reset (and short close animations) to render.
    const int SettleAfterPhaseChangeMs = 500;

    public void Start() => _ = RunAsync();

    // ---- ISceneHost ----
    public BenchOptions Options => options;
    public Window Window => _window;
    public nint Hwnd => _hwnd;
    public InputMode InputMode => _inputMode;
    public DispatcherQueue Queue => _queue;

    async Task RunAsync()
    {
        int status = 0;
        string detail = "ok";
        try
        {
            _hwnd = WinRT.Interop.WindowNative.GetWindowHandle(_window);
            string backend = Environment.GetEnvironmentVariable(BackendModes.EnvironmentVariable) is { Length: > 0 } b
                ? b : $"comp-{options.CompositionEngine.ToString().ToLowerInvariant()}";
            _window.Title = $"RenderBench {SceneCatalog.Get(options.Scene).PlanId} {options.SceneLabel} [{backend}]";
            _window.AppWindow.MoveAndResize(new RectInt32(40, 40, options.Width, options.Height));
            _scene = SceneFactory.Create(options);
            await _scene.PrepareAsync();

            var ready = new TaskCompletionSource();
            _scene.Root.Loaded += (_, _) => _queue.TryEnqueue(DispatcherQueuePriority.Low, () => ready.TrySetResult());
            _window.Content = _scene.Root;
            _window.Activate();
            await ready.Task;
            Win32.ForceForeground(_hwnd);
            var (inputMode, inputReason) = ResolveInputMode();
            _inputMode = inputMode;
            _scene.Attach(this);

            BenchEventSource.Log.RunStarted(options.SceneLabel, Environment.GetEnvironmentVariable(BackendModes.EnvironmentVariable) ?? "",
                options.Seed, options.Width, options.Height, _scene.Root.XamlRoot.RasterizationScale);
            BenchEventSource.Log.SceneTraits(_scene.AnimatesWhileIdle, _scene.ContinuousInteraction);
            BenchEventSource.Log.InputModeSelected(_inputMode.ToString(), inputReason);
            if (Win32.RefreshRate(_hwnd) is { } hz)
            {
                BenchEventSource.Log.DisplayInfo(hz);
            }
            BenchEventSource.Log.SceneReady(_scene.Name, CountElements(_scene.Root));
            _threadSampler.CaptureUiThread(); // immediate-rendering-d2d-sw
            _sampleTimer = new Timer(_ => SampleMemory(), null, 0, options.SampleIntervalMs);
            StartCounterSampling();

            // Warmup touches every input target once and applies a few updates so lazy caches (glyphs, brushes,
            // templates' visual states) exist before Steady is measured.
            await Phase(BenchPhase.Warmup, options.WarmupSeconds, async () =>
            {
                if (_scene.WarmupAsync(this) is { } custom)
                {
                    await custom;
                    return;
                }
                foreach (var target in _scene.InputTargets)
                {
                    Activate(target, "warmup");
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
                // Decision scenes (S24-S32) run their step sweep to completion; their length is not --interaction.
                using var stop = new CancellationTokenSource();
                if (!SceneCatalog.IsDecisionScene(options.Scene))
                {
                    stop.CancelAfter(TimeSpan.FromSeconds(options.InteractionSeconds));
                }
                _tick = 0;
                var updates = _scene.IsDynamic && options.UpdateHz > 0 ? Every(options.UpdateHz, Update, stop.Token) : Task.CompletedTask;
                Task activity;
                if (_scene.RunInteractionAsync(this, stop.Token) is { } custom)
                {
                    activity = custom;
                }
                else if (options.InputHz > 0 && _scene.InputTargets.Count > 0)
                {
                    int next = 0;
                    activity = Every(options.InputHz, () => Activate(_scene.InputTargets[next++ % _scene.InputTargets.Count]), stop.Token);
                }
                else
                {
                    activity = Task.Delay(Timeout.Infinite, stop.Token).ContinueWith(_ => { }, TaskScheduler.Default);
                }
                await Task.WhenAll(updates, activity);
            });

            await Phase(BenchPhase.Recovery, options.RecoverySeconds, () => Task.CompletedTask);

            if (options.ScreenshotPath is { } path)
            {
                await SaveScreenshotAsync(path);
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
            _counterTimer?.Stop();
            if (_sampleTimer is not null)
            {
                // Dispose the event only once the timer has signalled it; if the wait times out (a sample is still
                // running) the timer signals it later, and a disposed handle would crash a --no-exit process.
                var stopped = new ManualResetEvent(false);
                _sampleTimer.Dispose(stopped);
                if (stopped.WaitOne(1000))
                {
                    stopped.Dispose();
                }
            }
            try
            {
                _scene?.Detach();
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine(ex);
            }
            EmitThreadSummary(); // immediate-rendering-d2d-sw
            BenchEventSource.Log.RunCompleted(status, detail);
            _sampler.Dispose();
            _dwmSampler.Dispose();
            if (options.Exit)
            {
                Application.Current.Exit();
            }
        }
    }

    async Task SaveScreenshotAsync(string path)
    {
        string method;
        int width, height;
        if (options.ScreenshotMethod == ScreenshotMethod.Xaml)
        {
            (width, height) = await SaveXamlScreenshotAsync(path);
            method = "Xaml";
        }
        else
        {
            (width, height, method, bool uniform) = Win32.SaveClientScreenshot(_hwnd, path, options.ScreenshotMethod);
            if (uniform && options.ScreenshotMethod == ScreenshotMethod.Auto)
            {
                try
                {
                    (width, height) = await SaveXamlScreenshotAsync(path);
                    method = "Xaml (screen and PrintWindow were uniform)";
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"RenderTargetBitmap fallback failed: {ex.GetType().Name} 0x{ex.HResult:X8} {ex.Message}");
                }
            }
        }
        BenchEventSource.Log.ScreenshotSaved(Path.GetFullPath(path), width, height, method);
    }

    // The framework's render of the scene root (premultiplied BGRA), independent of DWM.
    async Task<(int Width, int Height)> SaveXamlScreenshotAsync(string path)
    {
        var target = new Microsoft.UI.Xaml.Media.Imaging.RenderTargetBitmap();
        await target.RenderAsync(_scene!.Root);
        var buffer = await target.GetPixelsAsync();
        byte[] pixels = System.Runtime.InteropServices.WindowsRuntime.WindowsRuntimeBufferExtensions.ToArray(buffer);
        Win32.WriteBmp(path, target.PixelWidth, target.PixelHeight, pixels);
        return (target.PixelWidth, target.PixelHeight);
    }

    // Auto: SendInput only when this is the active console session and a probe input actually arrives; otherwise
    // (remote/disconnected session, locked desktop) drive the same state changes programmatically.
    (InputMode Mode, string Reason) ResolveInputMode()
    {
        if (options.InputMode != InputMode.Auto)
        {
            return (options.InputMode, "requested with --input-mode");
        }
        uint session = Win32.CurrentSessionId(), console = Win32.ActiveConsoleSessionId();
        if (session != console)
        {
            return (InputMode.Programmatic, $"session {session} is not the active console session ({console})");
        }
        var (x, y) = ScreenPoint(_scene!.Root);
        return Win32.ProbeSendInput(x, y, out string reason) ? (InputMode.SendInput, reason) : (InputMode.Programmatic, reason);
    }

    // A phase lasts at least `seconds`; scripted activity runs inside it from the start.
    async Task Phase(BenchPhase phase, double seconds, Func<Task> activity)
    {
        if (_scene?.OnPhase(phase) == true)
        {
            await Task.Delay(SettleAfterPhaseChangeMs);
        }
        _phase = (int)phase;
        BenchEventSource.Log.PhaseBegin((int)phase, phase.ToString());
        SampleMemory();
        var minimum = Task.Delay(TimeSpan.FromSeconds(seconds));
        await activity();
        await minimum;
        SampleMemory();
        BenchEventSource.Log.PhaseEnd((int)phase, phase.ToString());
    }

    public Task Every(double hz, Action action, CancellationToken token)
    {
        var done = new TaskCompletionSource();
        var timer = _queue.CreateTimer();
        timer.Interval = TimeSpan.FromSeconds(1.0 / hz);
        timer.IsRepeating = true;
        timer.Tick += (_, _) =>
        {
            if (token.IsCancellationRequested)
            {
                timer.Stop();
                done.TrySetResult();
                return;
            }
            action();
        };
        token.Register(() => _queue.TryEnqueue(() =>
        {
            timer.Stop();
            done.TrySetResult();
        }));
        timer.Start();
        return done.Task;
    }

    public Task EveryFrame(Action<double> onFrame, CancellationToken token)
    {
        var done = new TaskCompletionSource();
        var clock = Stopwatch.StartNew();
        double last = 0;
        EventHandler<object>? handler = null;
        handler = (_, _) =>
        {
            if (token.IsCancellationRequested)
            {
                CompositionTarget.Rendering -= handler;
                done.TrySetResult();
                return;
            }
            double now = clock.Elapsed.TotalSeconds;
            onFrame(now - last);
            last = now;
        };
        CompositionTarget.Rendering += handler;
        token.Register(() => _queue.TryEnqueue(() =>
        {
            CompositionTarget.Rendering -= handler;
            done.TrySetResult();
        }));
        return done.Task;
    }

    public void Update()
    {
        int changed = _scene!.ApplyUpdate();
        BenchEventSource.Log.UpdateApplied(_tick++, changed);
    }

    public long Activate(FrameworkElement target, string kind = "click")
    {
        if (_inputMode == InputMode.SendInput)
        {
            var (x, y) = ScreenPoint(target);
            long id = MarkInput(kind, target.GetType().Name, programmatic: false);
            Win32.Click(x, y);
            return id;
        }
        long programmaticId = MarkInput(kind, target.GetType().Name, programmatic: true);
        _scene!.Activate(target);
        return programmaticId;
    }

    public long MarkInput(string kind, string target, bool? programmatic = null)
    {
        bool isProgrammatic = programmatic ?? _inputMode == InputMode.Programmatic;
        long id = Interlocked.Increment(ref _inputId);
        BenchEventSource.Log.InputInjected(id, isProgrammatic ? "programmatic:" + kind : kind, target);
        return id;
    }

    public void Step(int step, string name, int windows)
    {
        SampleMemory();
        BenchEventSource.Log.SceneStep(step, name, windows);
    }

    public (int X, int Y) ScreenPoint(FrameworkElement element, double fx = 0.5, double fy = 0.5)
    {
        nint hwnd = element.XamlRoot?.ContentIslandEnvironment?.AppWindowId is { } id && id.Value != 0
            ? Microsoft.UI.Win32Interop.GetWindowFromWindowId(id)
            : _hwnd;
        return Win32.ElementPointOnScreen(element, hwnd, fx, fy);
    }

    void StartCounterSampling()
    {
        _counterTimer = _queue.CreateTimer();
        _counterTimer.Interval = TimeSpan.FromMilliseconds(options.SampleIntervalMs);
        _counterTimer.IsRepeating = true;
        _counterTimer.Tick += (_, _) =>
        {
            foreach (var (name, value) in _scene!.Counters())
            {
                BenchEventSource.Log.SceneValue(name, value);
            }
        };
        _counterTimer.Start();
    }

    void SampleMemory()
    {
        // Called from the sampling timer thread and from the UI thread (phase edges, S9 steps).
        lock (_sampleLock)
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
            var dwm = _dwmSampler.Take(); // decision scenes: DWM delta
            BenchEventSource.Log.DwmMemorySample(_phase, dwm.PrivateBytes, dwm.WorkingSetBytes, dwm.ProcessId, dwm.CpuMs);
        }
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
