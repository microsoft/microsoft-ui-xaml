using System.Diagnostics;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.Graphics;

namespace RenderBench.Coverage;

// RenderBench --coverage <page id> --coverage-out <dir>: renders one coverage page in a fixed-size window, runs its
// Exercise step, waits until two consecutive window captures match, and writes capture.bmp + app.json
// (scripts\Run-Coverage.ps1 runs one process per page and mode so fallback and unsupported state can't leak).
sealed class CoverageRunner(BenchOptions options)
{
    readonly Window _window = new() { Title = "RenderBench coverage" };
    readonly DispatcherQueue _queue = DispatcherQueue.GetForCurrentThread();
    static readonly TimeSpan s_exerciseTimeout = TimeSpan.FromSeconds(20);
    const int IdlePollMs = 300, IdleMinimumMs = 600, IdleDeadlineMs = 6000;

    public void Start() => _ = RunAsync();

    async Task RunAsync()
    {
        string mode = Environment.GetEnvironmentVariable(BackendModes.EnvironmentVariable) is { Length: > 0 } b ? b : "composition";
        string outDir = options.CoverageOut ?? Path.Combine(Environment.CurrentDirectory, "coverage-run");
        Directory.CreateDirectory(outDir);
        var result = new CoverageAppResult { PageId = options.CoveragePage!, Mode = mode, CaptureMethod = Win32.WindowSetCaptureMethod };
        CoveragePage? page = null;
        try
        {
            var (pages, _) = CoverageCatalog.Discover(typeof(CoverageRunner).Assembly.GetTypes());
            var info = pages.FirstOrDefault(p => string.Equals(p.Id, options.CoveragePage, StringComparison.OrdinalIgnoreCase))
                ?? throw new ArgumentException($"no coverage page '{options.CoveragePage}'");
            nint hwnd = WinRT.Interop.WindowNative.GetWindowHandle(_window);
            _window.Title = $"RenderBench coverage {info.Id} [{mode}]";
            _window.AppWindow.Move(new PointInt32(40, 40));
            _window.AppWindow.ResizeClient(new SizeInt32(options.Width, options.Height));

            var clock = Stopwatch.StartNew();
            page = (CoveragePage)info.Method!.Invoke(null, [new CoverageContext { Window = _window, Seed = options.Seed }])!;
            // Initial focus goes to an invisible sink instead of the page's first control, so no page starts with a
            // focused TextBox (blinking caret) or a focus visual unless its Exercise step asks for one.
            var sink = new Button { Width = 1, Height = 1, Opacity = 0, HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top, IsTabStop = true };
            var host = new Grid();
            host.Children.Add(page.Root);
            host.Children.Add(sink);
            var loaded = new TaskCompletionSource();
            page.Root.Loaded += (_, _) => _queue.TryEnqueue(DispatcherQueuePriority.Low, () => loaded.TrySetResult());
            _window.Content = host;
            _window.Activate();
            Win32.ParkCursor(hwnd);
            await loaded.Task;
            sink.Focus(FocusState.Programmatic);
            result = result with { Loaded = true, LoadMs = clock.Elapsed.TotalMilliseconds, RasterizationScale = page.Root.XamlRoot.RasterizationScale };
            await Task.Delay(200);

            if (page.Exercise is { } exercise)
            {
                var run = exercise();
                if (await Task.WhenAny(run, Task.Delay(s_exerciseTimeout)) != run)
                {
                    throw new TimeoutException($"Exercise did not complete within {s_exerciseTimeout.TotalSeconds} s");
                }
                await run;
            }
            result = result with { ExerciseCompleted = true };
            Win32.ParkCursor(hwnd);

            // Idle: two consecutive captures IdlePollMs apart match (after a minimum settle), or the deadline passes.
            var idle = Stopwatch.StartNew();
            await Task.Delay(IdleMinimumMs);
            var (previous, _) = Win32.CaptureWindowSet(hwnd);
            BgraImage capture = previous;
            int popups = 0;
            bool stable = false;
            while (idle.ElapsedMilliseconds < IdleDeadlineMs)
            {
                await Task.Delay(IdlePollMs);
                (capture, popups) = Win32.CaptureWindowSet(hwnd);
                if (capture.Pixels.AsSpan().SequenceEqual(previous.Pixels))
                {
                    stable = true;
                    break;
                }
                previous = capture;
            }
            Win32.WriteBmp(Path.Combine(outDir, CoverageFiles.Capture), capture.Width, capture.Height, capture.Pixels);
            using var process = Process.GetCurrentProcess();
            result = result with
            {
                Width = capture.Width, Height = capture.Height, Stable = stable, IdleMs = (int)idle.ElapsedMilliseconds, PopupWindows = popups,
                PrivateMBIdle = Math.Round(process.PrivateMemorySize64 / (1024.0 * 1024.0), 1),
            };
        }
        catch (Exception ex)
        {
            var inner = ex is System.Reflection.TargetInvocationException { InnerException: { } e } ? e : ex;
            result = result with { Status = "error", Error = inner.ToString() };
            Console.Error.WriteLine(inner);
        }
        finally
        {
            File.WriteAllText(Path.Combine(outDir, CoverageFiles.App), result.Serialize());
            try
            {
                page?.Detach?.Invoke();
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine(ex);
            }
            if (options.Exit)
            {
                Application.Current.Exit();
            }
        }
    }
}
