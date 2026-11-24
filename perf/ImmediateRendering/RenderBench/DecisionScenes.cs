using System.Diagnostics;
using System.Runtime.InteropServices.WindowsRuntime;
using Microsoft.UI;
using Microsoft.UI.Text;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Animation;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Xaml.Shapes;
using Windows.Graphics;
using Windows.UI;

namespace RenderBench;

// Backend decision scenes S24-S32 (test plan, "Backend decision scenarios"). Each one is a deterministic step sweep run
// entirely in Interaction (Steady stays idle): host.Step marks the start of a step, the scene applies the step's change
// and then holds it. MetricsCalculator reads each step's settled values from the second half of its window, so a step
// holds at least as long as its change took (Hold). Step names are what MetricsCalculator.ComputeDecision expects.
// Derived scenes re-declare IScene so their Attach/WarmupAsync/ContinuousInteraction map to the interface.
abstract class DecisionScene(BenchOptions options) : IScene
{
    protected readonly BenchOptions Options = options;
    protected readonly Random Random = new(options.Seed);
    static readonly Process s_self = Process.GetCurrentProcess();
    readonly DwmSampler _dwm = new();

    public abstract string Name { get; }
    public bool IsDynamic => false;
    public abstract FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => [];
    public int ApplyUpdate() => 0;
    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token) => RunStepsAsync(host);
    protected abstract Task RunStepsAsync(ISceneHost host);

    // Holds the current step for StepSeconds, and at least as long as the change took (started measures it) so the
    // settled half of the step window starts after the change.
    protected Task Hold(Stopwatch? started = null, double? seconds = null)
    {
        double hold = seconds ?? Options.StepSeconds;
        if (started is not null)
        {
            hold = Math.Max(hold, started.Elapsed.TotalSeconds + 0.5);
        }
        return Task.Delay(TimeSpan.FromSeconds(hold));
    }

    protected static async Task Frames(int count = 2)
    {
        for (int i = 0; i < count; i++)
        {
            await FullBackend.NextFrameAsync();
        }
    }

    protected static void Collect()
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
    }

    // Process CPU and session dwm.exe CPU (ms), for per-frame costs measured inside a step.
    protected (double Process, double Dwm) Cpu()
    {
        s_self.Refresh();
        return (s_self.TotalProcessorTime.TotalMilliseconds, _dwm.Take().CpuMs);
    }

    protected static void Value(string name, double value) => BenchEventSource.Log.SceneValue(name, value);

    protected static void EmitClientSize(ISceneHost host)
    {
        var (width, height) = Win32.ClientSize(host.Hwnd);
        Value("ClientWidth", width);
        Value("ClientHeight", height);
    }

    // Toggles root's background `count` times, one full-window invalidation per frame, and reports the CPU per frame.
    protected async Task MeasureFullFramesAsync(Panel root, string prefix, int count = 5)
    {
        var brushes = new[] { new SolidColorBrush(Color.FromArgb(255, 0xF3, 0xF3, 0xF3)), new SolidColorBrush(Color.FromArgb(255, 0xEE, 0xF4, 0xFA)) };
        await Frames(2);
        var before = Cpu();
        var clock = Stopwatch.StartNew();
        for (int i = 0; i < count; i++)
        {
            root.Background = brushes[(i + 1) % 2];
            await Frames(2);
        }
        var after = Cpu();
        Value(prefix + "CpuMs", (after.Process - before.Process) / count);
        Value(prefix + "DwmCpuMs", (after.Dwm - before.Dwm) / count);
        Value(prefix + "WallMs", clock.Elapsed.TotalMilliseconds / count);
        root.Background = brushes[0];
    }

    protected static Grid Page(UIElement content) => new()
    {
        Background = new SolidColorBrush(Color.FromArgb(255, 0xF3, 0xF3, 0xF3)),
        Children = { content },
    };

    public virtual void Detach() { }
}

// S24: window area. The S1 settings page at client sizes 640x480 .. 3840x2160 (Items: how many of the sizes, default all).
// The window is placed at (0,0) and may extend past the 1024x768 VM desktop; WM_GETMINMAXINFO is raised so Windows
// doesn't clamp it. Each step reports ClientWidth/ClientHeight (the size actually reached).
sealed class WindowSizeScene(BenchOptions options) : DecisionScene(options), IScene
{
    public static readonly (int Width, int Height)[] Sizes = [(640, 480), (1280, 800), (1920, 1080), (2560, 1440), (3840, 2160)];
    readonly Grid _root = Page(new SettingsScene(options).Root);

    public override string Name => "WindowSize";
    public override FrameworkElement Root => _root;

    public void Attach(ISceneHost host) => Win32.AllowLargerThanScreen(host.Hwnd);

    protected override async Task RunStepsAsync(ISceneHost host)
    {
        int count = Options.Items > 0 ? Math.Min(Options.Items, Sizes.Length) : Sizes.Length;
        for (int i = 0; i < count; i++)
        {
            var (width, height) = Sizes[i];
            host.Step(i, $"size-{width}x{height}", width * height);
            var started = Stopwatch.StartNew();
            host.MarkInput("resize", "Window", programmatic: true);
            host.Window.AppWindow.Move(new PointInt32(0, 0));
            host.Window.AppWindow.ResizeClient(new SizeInt32(width, height));
            await Frames(3);
            EmitClientSize(host);
            await Hold(started);
        }
    }
}

// S25: visible element density. Items (default 32000) caps the sweep 250, 1k, 4k, 16k, 32k. Each step replaces a full-window
// Canvas with N items in a grid (a third each Border+TextBlock tiles, Rectangles, Ellipses; no virtualization), then
// measures five full-window invalidations (FullFrameCpuMs) and holds. FirstFrameCpuMs is the build + first render.
sealed class DensityScene(BenchOptions options) : DecisionScene(options), IScene
{
    public static readonly int[] Counts = [250, 1000, 4000, 16000, 32000];
    readonly Grid _root = Page(new Canvas());

    public override string Name => "Density";
    public override FrameworkElement Root => _root;

    protected override async Task RunStepsAsync(ISceneHost host)
    {
        var counts = Counts.Where(c => c <= Math.Max(Options.Items, Counts[0])).ToArray();
        for (int i = 0; i < counts.Length; i++)
        {
            int n = counts[i];
            host.Step(i, $"n-{n}", n);
            var started = Stopwatch.StartNew();
            _root.Children.Clear();
            Collect();
            var before = Cpu();
            var canvas = Build(n, _root.ActualWidth, _root.ActualHeight);
            _root.Children.Add(canvas);
            await Frames(2);
            var after = Cpu();
            Value("FirstFrameCpuMs", after.Process - before.Process);
            Value("Elements", n);
            await MeasureFullFramesAsync(_root, "FullFrame");
            await Hold(started);
        }
    }

    Canvas Build(int n, double width, double height)
    {
        int columns = Math.Max(1, (int)Math.Ceiling(Math.Sqrt(n * width / height)));
        int rows = (int)Math.Ceiling(n / (double)columns);
        double cellW = width / columns, cellH = height / rows;
        double w = Math.Max(1, cellW - 1), h = Math.Max(1, cellH - 1);
        var canvas = new Canvas();
        for (int i = 0; i < n; i++)
        {
            var fill = new SolidColorBrush(FullBackend.RandomColor(Random));
            FrameworkElement element = (i % 3) switch
            {
                0 => new Border
                {
                    Width = w, Height = h, Background = fill, CornerRadius = new CornerRadius(Math.Min(w, h) / 6),
                    Child = new TextBlock { Text = (i % 100).ToString(), FontSize = Math.Max(1, h * 0.5), Foreground = new SolidColorBrush(Colors.Black) },
                },
                1 => new Rectangle { Width = w, Height = h, Fill = fill },
                _ => new Ellipse { Width = w, Height = h, Fill = fill },
            };
            Canvas.SetLeft(element, (i % columns) * cellW);
            Canvas.SetTop(element, (i / columns) * cellH);
            canvas.Children.Add(element);
        }
        return canvas;
    }
}

// S26: window count. The 400x300 main window plus 400x300 tool windows with light content, totals 1, 2, 4, 8, 16 (Items
// caps the total). Step windows-N is held with N windows open. Warmup opens and closes one window (first-use costs).
sealed class WindowCountScene(BenchOptions options) : DecisionScene(options), IScene
{
    public static readonly int[] Totals = [1, 2, 4, 8, 16];
    readonly Grid _root = Page(Light(0));
    readonly List<Window> _windows = [];

    public override string Name => "WindowCount";
    public override FrameworkElement Root => _root;

    static StackPanel Light(int index) => new()
    {
        Padding = new Thickness(16), Spacing = 6,
        Children =
        {
            new TextBlock { Text = $"Tool window {index + 1}", Style = SettingsScene.Style("SubtitleTextBlockStyle") },
            new TextBlock { Text = "Status: ready", Opacity = 0.7 },
            new ProgressBar { Value = 20 + index * 5 % 80, Maximum = 100, Width = 200, HorizontalAlignment = HorizontalAlignment.Left },
            new Button { Content = "Refresh" },
        },
    };

    Window Open(int index)
    {
        var window = new Window { Title = $"RenderBench tool {index + 1}", Content = Page(Light(index)) };
        // Cascade on the 1024x768 desktop: 5 columns x 3 rows, overlapping.
        int slot = index % 15;
        window.AppWindow.MoveAndResize(new RectInt32(20 + slot % 5 * 120, 20 + slot / 5 * 140, Options.Width, Options.Height));
        window.Activate();
        return window;
    }

    public Task? WarmupAsync(ISceneHost host) => WarmupWindowAsync();

    async Task WarmupWindowAsync()
    {
        var window = Open(99);
        await Task.Delay(500);
        window.Close();
        await Task.Delay(200);
    }

    protected override async Task RunStepsAsync(ISceneHost host)
    {
        int max = Options.Items > 0 ? Options.Items : Totals[^1];
        var totals = Totals.Where(t => t <= max).ToArray();
        for (int i = 0; i < totals.Length; i++)
        {
            host.Step(i, $"windows-{totals[i]}", totals[i]);
            var started = Stopwatch.StartNew();
            while (_windows.Count + 1 < totals[i])
            {
                _windows.Add(Open(_windows.Count + 1));
                await Frames(2);
            }
            Value("OpenWindows", _windows.Count + 1);
            await Hold(started);
        }
    }

    public override void Detach()
    {
        foreach (var window in _windows)
        {
            window.Close();
        }
        _windows.Clear();
    }
}

// S27: background. The S1 settings page visible, minimized, restored, hidden (AppWindow.Hide), shown, occluded by a
// topmost opaque popup, unoccluded. Invisible states hold BackgroundHoldSeconds (> the framework's 2 s background
// resource timer); visible ones hold StepSeconds. Reports Iconic/Visible per step.
sealed class BackgroundScene(BenchOptions options) : DecisionScene(options), IScene
{
    public const double BackgroundHoldSeconds = 12;
    readonly Grid _root = Page(new SettingsScene(options).Root);
    nint _occluder;

    public override string Name => "Background";
    public override FrameworkElement Root => _root;

    protected override async Task RunStepsAsync(ISceneHost host)
    {
        var window = host.Window.AppWindow;
        var presenter = (OverlappedPresenter)window.Presenter;
        int step = 0;

        async Task State(string name, Action change, bool hidden)
        {
            host.Step(step++, name, 1);
            host.MarkInput(name, "Window", programmatic: true);
            change();
            await Frames(2);
            Value("Iconic", Win32.IsIconicWindow(host.Hwnd) ? 1 : 0);
            Value("Visible", window.IsVisible ? 1 : 0);
            await Hold(seconds: hidden ? BackgroundHoldSeconds : Options.StepSeconds);
        }

        await State("visible", () => _root.Background = new SolidColorBrush(Color.FromArgb(255, 0xF2, 0xF2, 0xF2)), false);
        await State("minimized", presenter.Minimize, true);
        await State("restored", () => { presenter.Restore(); Win32.ForceForeground(host.Hwnd); }, false);
        await State("hidden", window.Hide, true);
        await State("shown", () => { window.Show(); Win32.ForceForeground(host.Hwnd); }, false);
        await State("occluded", () => _occluder = Win32.CreateOccluder(host.Hwnd), true);
        await State("unoccluded", RemoveOccluder, false);
    }

    void RemoveOccluder()
    {
        Win32.DestroyOccluder(_occluder);
        _occluder = 0;
    }

    public override void Detach() => RemoveOccluder();
}

// S28: navigation churn. A Frame (CacheSize 0, NavigationCacheMode Disabled, back stack cleared) navigates Items (200)
// times round-robin through ten page types; every page gets a unique 128x128 WriteableBitmap and seeded text. After every
// 20 navigations a full GC, then step nav-N. NavPath is 1 for Frame.Navigate, 0 when it fell back to Frame.Content.
sealed class NavChurnScene : DecisionScene, IScene
{
    public const int PageTypes = 10;
    public const int NavigationsPerStep = 20;
    readonly Frame _frame = new() { CacheSize = 0 };
    readonly Grid _root;
    bool _navigate = true;
    int _navigations;

    public NavChurnScene(BenchOptions options) : base(options)
    {
        _root = Page(_frame);
        _root.Loaded += (_, _) => Navigate();
    }

    public override string Name => "NavChurn";
    public override FrameworkElement Root => _root;

    public Task? WarmupAsync(ISceneHost host) => WarmupPagesAsync();

    // Visit every page type once so first-use costs are not in nav-0.
    async Task WarmupPagesAsync()
    {
        for (int i = 0; i < PageTypes; i++)
        {
            Navigate();
            await Frames(2);
        }
    }

    void Navigate()
    {
        int index = _navigations++;
        int kind = index % PageTypes;
        if (_navigate)
        {
            try
            {
                if (_frame.Navigate(ChurnPage.Types[kind], index))
                {
                    _frame.BackStack.Clear();
                    return;
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"S28: Frame.Navigate failed, using Frame.Content: {ex.Message}");
            }
            _navigate = false;
        }
        var page = (ChurnPage)Activator.CreateInstance(ChurnPage.Types[kind])!;
        page.Build(index);
        _frame.Content = page;
    }

    protected override async Task RunStepsAsync(ISceneHost host)
    {
        int total = Options.Items > 0 ? Options.Items : 200;
        int step = 0;
        Collect();
        host.Step(step++, "nav-0", 0);
        Value("NavPath", _navigate ? 1 : 0);
        await Hold();
        for (int done = 0; done < total;)
        {
            int batch = Math.Min(NavigationsPerStep, total - done);
            for (int i = 0; i < batch; i++)
            {
                host.MarkInput("navigate", "Frame", programmatic: true);
                Navigate();
                await Frames(2);
            }
            done += batch;
            Collect();
            host.Step(step++, $"nav-{done}", done);
            Value("NavPath", _navigate ? 1 : 0);
            Value("BackStack", _frame.BackStackDepth);
            await Hold();
        }
    }
}

// The ten S28 page types. Content depends on the page kind and the navigation index (unique bitmap and text).
abstract class ChurnPage : Page
{
    public static readonly Type[] Types =
    [
        typeof(ChurnPage0), typeof(ChurnPage1), typeof(ChurnPage2), typeof(ChurnPage3), typeof(ChurnPage4),
        typeof(ChurnPage5), typeof(ChurnPage6), typeof(ChurnPage7), typeof(ChurnPage8), typeof(ChurnPage9),
    ];

    readonly int _kind;

    protected ChurnPage(int kind)
    {
        _kind = kind;
        NavigationCacheMode = NavigationCacheMode.Disabled;
    }

    protected override void OnNavigatedTo(NavigationEventArgs e) => Build(e.Parameter is int index ? index : 0);

    public void Build(int index)
    {
        var random = new Random(index * 7919 + _kind);
        var stack = new StackPanel { Spacing = 8, Padding = new Thickness(24) };
        stack.Children.Add(new TextBlock { Text = $"Page {_kind} - visit {index}", Style = SettingsScene.Style("TitleTextBlockStyle") });
        stack.Children.Add(new Image { Source = Bitmap(random), Width = 128, Height = 128, HorizontalAlignment = HorizontalAlignment.Left });
        switch (_kind)
        {
            case 0:
                stack.Children.Add(new SettingsScene(new BenchOptions { Seed = index }).Root);
                break;
            case 1:
                for (int i = 0; i < 30; i++)
                {
                    stack.Children.Add(new TextBlock { Text = $"Row {i}: item {random.Next(100000)} - {Words(random, 6)}" });
                }
                break;
            case 2:
            {
                var text = new RichTextBlock { MaxWidth = 900, HorizontalAlignment = HorizontalAlignment.Left };
                for (int p = 0; p < 6; p++)
                {
                    var paragraph = new Microsoft.UI.Xaml.Documents.Paragraph();
                    paragraph.Inlines.Add(new Microsoft.UI.Xaml.Documents.Run { Text = Words(random, 60) });
                    text.Blocks.Add(paragraph);
                }
                stack.Children.Add(text);
                break;
            }
            case 3:
            {
                var canvas = new Canvas { Width = 800, Height = 300 };
                for (int i = 0; i < 60; i++)
                {
                    var shape = new Ellipse { Width = 20 + random.Next(60), Height = 20 + random.Next(60), Fill = new SolidColorBrush(FullBackend.RandomColor(random)) };
                    Canvas.SetLeft(shape, random.Next(760));
                    Canvas.SetTop(shape, random.Next(260));
                    canvas.Children.Add(shape);
                }
                stack.Children.Add(canvas);
                break;
            }
            case 4:
            {
                var grid = new Grid { ColumnSpacing = 6, RowSpacing = 6 };
                for (int c = 0; c < 8; c++)
                {
                    grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(90) });
                }
                for (int i = 0; i < 48; i++)
                {
                    if (i % 8 == 0)
                    {
                        grid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(48) });
                    }
                    var tile = new Border { Background = new SolidColorBrush(FullBackend.RandomColor(random)), CornerRadius = new CornerRadius(4),
                        Child = new TextBlock { Text = random.Next(1000).ToString(), Margin = new Thickness(6) } };
                    Grid.SetRow(tile, i / 8);
                    Grid.SetColumn(tile, i % 8);
                    grid.Children.Add(tile);
                }
                stack.Children.Add(grid);
                break;
            }
            case 5:
                stack.Children.Add(new TextBox { Header = "Name", Text = Words(random, 2), Width = 300, HorizontalAlignment = HorizontalAlignment.Left, IsTabStop = false });
                stack.Children.Add(new ComboBox { Header = "Choice", ItemsSource = new[] { "One", "Two", "Three" }, SelectedIndex = random.Next(3) });
                stack.Children.Add(new CheckBox { Content = "Remember", IsChecked = random.Next(2) == 0 });
                stack.Children.Add(new Slider { Header = "Level", Value = random.Next(100), Width = 300, HorizontalAlignment = HorizontalAlignment.Left });
                stack.Children.Add(new Button { Content = "Submit", Style = SettingsScene.Style("AccentButtonStyle") });
                break;
            case 6:
                for (int i = 0; i < 8; i++)
                {
                    var brush = new LinearGradientBrush { StartPoint = new Windows.Foundation.Point(0, 0), EndPoint = new Windows.Foundation.Point(1, 0) };
                    brush.GradientStops.Add(new GradientStop { Color = FullBackend.RandomColor(random), Offset = 0 });
                    brush.GradientStops.Add(new GradientStop { Color = FullBackend.RandomColor(random), Offset = 1 });
                    stack.Children.Add(new Rectangle { Height = 24, Width = 600, Fill = brush, HorizontalAlignment = HorizontalAlignment.Left, RadiusX = 6, RadiusY = 6 });
                }
                break;
            case 7:
                for (int i = 0; i < 6; i++)
                {
                    stack.Children.Add(FullBackend.Card(new TextBlock { Text = Words(random, 20), TextWrapping = TextWrapping.Wrap, MaxWidth = 700 }));
                }
                break;
            case 8:
                // Distinct sizes and weights: glyph cache churn.
                for (int i = 0; i < 16; i++)
                {
                    stack.Children.Add(new TextBlock { Text = Words(random, 8), FontSize = 10 + (index + i) % 23,
                        FontWeight = i % 2 == 0 ? FontWeights.Normal : FontWeights.SemiBold, FontStyle = i % 3 == 0 ? Windows.UI.Text.FontStyle.Italic : Windows.UI.Text.FontStyle.Normal });
                }
                break;
            default:
            {
                var path = new Microsoft.UI.Xaml.Shapes.Path { Stroke = new SolidColorBrush(Colors.SteelBlue), StrokeThickness = 3 };
                var figure = new PathFigure { StartPoint = new Windows.Foundation.Point(0, 150) };
                for (int x = 10; x <= 800; x += 10)
                {
                    figure.Segments.Add(new LineSegment { Point = new Windows.Foundation.Point(x, 150 + random.Next(-120, 120)) });
                }
                path.Data = new PathGeometry { Figures = { figure } };
                stack.Children.Add(new Border { Height = 300, Child = path });
                break;
            }
        }
        Content = new ScrollViewer { Content = stack, VerticalScrollBarVisibility = ScrollBarVisibility.Hidden };
    }

    static WriteableBitmap Bitmap(Random random)
    {
        const int size = 128;
        var bitmap = new WriteableBitmap(size, size);
        var pixels = new byte[size * size * 4];
        byte r = (byte)random.Next(256), g = (byte)random.Next(256), b = (byte)random.Next(256);
        for (int i = 0; i < size * size; i++)
        {
            pixels[i * 4] = (byte)(b ^ i);
            pixels[i * 4 + 1] = (byte)(g + i / size);
            pixels[i * 4 + 2] = r;
            pixels[i * 4 + 3] = 255;
        }
        using (var stream = bitmap.PixelBuffer.AsStream())
        {
            stream.Write(pixels, 0, pixels.Length);
        }
        bitmap.Invalidate();
        return bitmap;
    }

    static readonly string[] s_words = ["render", "window", "frame", "glyph", "brush", "layout", "surface", "pixel", "theme", "button", "canvas", "scroll"];

    static string Words(Random random, int count) => string.Join(' ', Enumerable.Range(0, count).Select(_ => s_words[random.Next(s_words.Length)]));
}

sealed class ChurnPage0() : ChurnPage(0);
sealed class ChurnPage1() : ChurnPage(1);
sealed class ChurnPage2() : ChurnPage(2);
sealed class ChurnPage3() : ChurnPage(3);
sealed class ChurnPage4() : ChurnPage(4);
sealed class ChurnPage5() : ChurnPage(5);
sealed class ChurnPage6() : ChurnPage(6);
sealed class ChurnPage7() : ChurnPage(7);
sealed class ChurnPage8() : ChurnPage(8);
sealed class ChurnPage9() : ChurnPage(9);

// S29: sustained full-window animation. Items (200) tiles, each rotating forever (RotateTransform storyboard, 2-4 s period)
// and a 20-tile ticker updated every frame. Steps: pre (idle), animating (InteractionSeconds, default 60), stopped.
sealed class SustainedScene : DecisionScene, IScene
{
    readonly Grid _root;
    readonly List<(Border Tile, TextBlock Text)> _tiles = [];
    readonly Storyboard _storyboard = new();

    public SustainedScene(BenchOptions options) : base(options)
    {
        int count = Math.Max(1, options.Items);
        int columns = (int)Math.Ceiling(Math.Sqrt(count * 1.6));
        var grid = new Grid { Padding = new Thickness(8), RowSpacing = 4, ColumnSpacing = 4 };
        for (int c = 0; c < columns; c++)
        {
            grid.ColumnDefinitions.Add(new ColumnDefinition());
        }
        for (int r = 0; r < (count + columns - 1) / columns; r++)
        {
            grid.RowDefinitions.Add(new RowDefinition());
        }
        for (int i = 0; i < count; i++)
        {
            var text = new TextBlock { Text = (Random.NextDouble() * 1000).ToString("F1"), HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center };
            var rotate = new RotateTransform();
            var tile = new Border
            {
                Background = new SolidColorBrush(FullBackend.RandomColor(Random)), CornerRadius = new CornerRadius(6), Child = text,
                RenderTransform = rotate, RenderTransformOrigin = new Windows.Foundation.Point(0.5, 0.5), Margin = new Thickness(6),
            };
            Grid.SetRow(tile, i / columns);
            Grid.SetColumn(tile, i % columns);
            grid.Children.Add(tile);
            _tiles.Add((tile, text));
            var spin = new DoubleAnimation { From = 0, To = 360, Duration = TimeSpan.FromSeconds(2 + i % 3), RepeatBehavior = RepeatBehavior.Forever };
            Storyboard.SetTarget(spin, rotate);
            Storyboard.SetTargetProperty(spin, "Angle");
            _storyboard.Children.Add(spin);
        }
        _root = Page(grid);
    }

    public override string Name => "Sustained";
    public override FrameworkElement Root => _root;
    public bool ContinuousInteraction => true;

    protected override async Task RunStepsAsync(ISceneHost host)
    {
        host.Step(0, "pre", 0);
        await Hold();
        host.Step(1, "animating", _tiles.Count);
        host.MarkInput("animate", "Storyboard", programmatic: true);
        using var stop = new CancellationTokenSource();
        _storyboard.Begin();
        int tick = 0;
        var ticker = host.EveryFrame(_ =>
        {
            for (int k = 0; k < 20; k++)
            {
                _tiles[(tick * 20 + k) % _tiles.Count].Text.Text = ((tick * 37 + k * 11) % 10000 / 10.0).ToString("F1");
            }
            tick++;
        }, stop.Token);
        await Task.Delay(TimeSpan.FromSeconds(Options.InteractionSeconds));
        stop.Cancel();
        await ticker;
        host.Step(2, "stopped", 0);
        _storyboard.Stop();
        Value("TickerFrames", tick);
        await Hold();
    }

    public override void Detach() => _storyboard.Stop();
}

// S30: resize storm. The S1 settings page: pre, storm (Items = 150 resizes to seeded client sizes in 800x600..1600x1000,
// one frame each), settle (back to the start size), recovered (after a full GC).
sealed class ResizeScene(BenchOptions options) : DecisionScene(options), IScene
{
    readonly Grid _root = Page(new SettingsScene(options).Root);

    public override string Name => "Resize";
    public override FrameworkElement Root => _root;

    public void Attach(ISceneHost host) => Win32.AllowLargerThanScreen(host.Hwnd);

    protected override async Task RunStepsAsync(ISceneHost host)
    {
        var window = host.Window.AppWindow;
        var start = window.ClientSize;
        host.Step(0, "pre", start.Width * start.Height);
        await Hold();
        host.Step(1, "storm", 0);
        int count = Math.Max(1, Options.Items);
        var clock = Stopwatch.StartNew();
        for (int i = 0; i < count; i++)
        {
            var size = i % 2 == 0
                ? new SizeInt32(Random.Next(800, 1201), Random.Next(600, 801))
                : new SizeInt32(Random.Next(1200, 1601), Random.Next(800, 1001));
            host.MarkInput("resize", "Window", programmatic: true);
            window.ResizeClient(size);
            await FullBackend.NextFrameAsync();
        }
        Value("StormSeconds", clock.Elapsed.TotalSeconds);
        host.Step(2, "settle", start.Width * start.Height);
        window.ResizeClient(start);
        await Frames(2);
        EmitClientSize(host);
        await Hold(seconds: Options.StepSeconds * 2);
        Collect();
        host.Step(3, "recovered", start.Width * start.Height);
        await Hold();
    }
}

// S31: minimal tray-style window. 320x200, one TextBlock and one Button. Steps idle (after one click) and after-gc.
// StartupToReadyMs is process start to the scene's Attach (first layout done).
sealed class MinimalScene : DecisionScene, IScene
{
    readonly TextBlock _text = new() { Text = "RenderBench tray" };
    readonly Button _button = new() { Content = "Open" };
    readonly Grid _root;
    double _startupMs;

    public MinimalScene(BenchOptions options) : base(options)
    {
        _root = Page(new StackPanel { Padding = new Thickness(16), Spacing = 8, Children = { _text, _button } });
    }

    public override string Name => "Minimal";
    public override FrameworkElement Root => _root;

    public void Attach(ISceneHost host) => _startupMs = (DateTime.Now - Process.GetCurrentProcess().StartTime).TotalMilliseconds;

    protected override async Task RunStepsAsync(ISceneHost host)
    {
        host.Step(0, "idle", 1);
        host.MarkInput("click", "Button", programmatic: true);
        _text.Text = "RenderBench tray (opened)";
        Value("StartupToReadyMs", _startupMs);
        await Hold();
        Collect();
        host.Step(1, "after-gc", 1);
        await Hold();
    }
}

// S32: large zoomable canvas. A ScrollViewer (zoom 0.1..10) over a 10000x10000 Canvas diagram of Items (3000) items: a
// third Border+TextBlock nodes, a third Lines joining consecutive nodes, a third shapes. Steps jump with
// ChangeView(disableAnimation) and then pan smoothly for 60 frames (PanCpuMsPerFrame, PanDwmCpuMsPerFrame, PanFps).
sealed class LargeCanvasScene : DecisionScene, IScene
{
    public const double CanvasSize = 10000;
    readonly ScrollViewer _viewer;
    readonly Grid _root;

    public LargeCanvasScene(BenchOptions options) : base(options)
    {
        var canvas = new Canvas { Width = CanvasSize, Height = CanvasSize, Background = new SolidColorBrush(Colors.White) };
        int count = Math.Max(3, options.Items);
        Windows.Foundation.Point? previous = null;
        for (int i = 0; i < count; i++)
        {
            double x = Random.NextDouble() * (CanvasSize - 200), y = Random.NextDouble() * (CanvasSize - 100);
            switch (i % 3)
            {
                case 0:
                {
                    var node = new Border
                    {
                        Width = 140, Height = 56, CornerRadius = new CornerRadius(6), BorderThickness = new Thickness(1),
                        BorderBrush = new SolidColorBrush(Colors.DimGray), Background = new SolidColorBrush(FullBackend.RandomColor(Random)),
                        Child = new TextBlock { Text = $"Node {i / 3}", Margin = new Thickness(8), FontSize = 14 },
                    };
                    Canvas.SetLeft(node, x);
                    Canvas.SetTop(node, y);
                    canvas.Children.Add(node);
                    previous = new Windows.Foundation.Point(x + 70, y + 28);
                    break;
                }
                case 1:
                {
                    var from = previous ?? new Windows.Foundation.Point(x, y);
                    var to = new Windows.Foundation.Point(Random.NextDouble() * CanvasSize, Random.NextDouble() * CanvasSize);
                    canvas.Children.Add(new Line { X1 = from.X, Y1 = from.Y, X2 = to.X, Y2 = to.Y, Stroke = new SolidColorBrush(Colors.SlateGray), StrokeThickness = 2 });
                    break;
                }
                default:
                {
                    Shape shape = i % 2 == 0 ? new Ellipse() : new Rectangle { RadiusX = 4, RadiusY = 4 };
                    shape.Width = 20 + Random.Next(80);
                    shape.Height = 20 + Random.Next(80);
                    shape.Fill = new SolidColorBrush(FullBackend.RandomColor(Random));
                    Canvas.SetLeft(shape, x);
                    Canvas.SetTop(shape, y);
                    canvas.Children.Add(shape);
                    break;
                }
            }
        }
        _viewer = new ScrollViewer
        {
            Content = canvas, ZoomMode = ZoomMode.Enabled, MinZoomFactor = 0.1f, MaxZoomFactor = 10f,
            HorizontalScrollMode = ScrollMode.Enabled, HorizontalScrollBarVisibility = ScrollBarVisibility.Hidden,
            VerticalScrollMode = ScrollMode.Enabled, VerticalScrollBarVisibility = ScrollBarVisibility.Hidden,
        };
        _root = Page(_viewer);
    }

    public override string Name => "LargeCanvas";
    public override FrameworkElement Root => _root;
    public bool ContinuousInteraction => true;

    // (name, zoom, x, y in unzoomed canvas coordinates of the viewport's top-left).
    public static readonly (string Name, float Zoom, double X, double Y)[] Steps =
    [
        ("z1-origin", 1, 0, 0), ("z1-pan1", 1, 2000, 1500), ("z1-pan2", 1, 6000, 4000), ("z1-pan3", 1, 8500, 9000),
        ("z0.25", 0.25f, 0, 0), ("z0.25-pan", 0.25f, 4000, 4000),
        ("z4", 4, 0, 0), ("z4-pan1", 4, 5000, 5000), ("z4-pan2", 4, 9500, 9600),
        ("z1-back", 1, 0, 0),
    ];

    protected override async Task RunStepsAsync(ISceneHost host)
    {
        for (int i = 0; i < Steps.Length; i++)
        {
            var (name, zoom, x, y) = Steps[i];
            host.Step(i, name, (int)(zoom * 100));
            var started = Stopwatch.StartNew();
            host.MarkInput("zoom", "ScrollViewer", programmatic: true);
            _viewer.ChangeView(x * zoom, y * zoom, zoom, disableAnimation: true);
            await Frames(3);
            Value("ZoomFactor", _viewer.ZoomFactor);
            if (name.Contains("pan", StringComparison.Ordinal))
            {
                await PanAsync(host, zoom);
            }
            await Hold(started);
        }
    }

    // 60 frames of smooth panning, 8 DIPs (screen) per frame down and right.
    async Task PanAsync(ISceneHost host, float zoom)
    {
        const int frames = 60;
        var before = Cpu();
        var clock = Stopwatch.StartNew();
        for (int f = 0; f < frames; f++)
        {
            host.MarkInput("pan", "ScrollViewer", programmatic: true);
            _viewer.ChangeView(_viewer.HorizontalOffset + 8, _viewer.VerticalOffset + 8, null, disableAnimation: true);
            await FullBackend.NextFrameAsync();
        }
        var after = Cpu();
        double seconds = clock.Elapsed.TotalSeconds;
        Value("PanCpuMsPerFrame", (after.Process - before.Process) / frames);
        Value("PanDwmCpuMsPerFrame", (after.Dwm - before.Dwm) / frames);
        Value("PanFps", frames / seconds);
    }
}
