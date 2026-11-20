using System.Numerics;
using Microsoft.UI;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.AnimatedVisuals;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Shapes;
using Windows.Graphics;
using Windows.Media.Core;
using Windows.Media.Editing;
using Windows.Storage;
using Windows.UI;
using Windows.UI.Text;

namespace RenderBench;

// S12: Lottie. Built-in ProgressRings and AnimatedIcons (state changes on input) must stay immediate; an
// AnimatedVisualPlayer playing a custom IAnimatedVisualSource (a ShapeVisual rotated by an expression on Progress) is
// expected to make the window fall back to Composition. --no-player (S12-noplayer) replaces the player with a static
// square of the same size.
sealed class LottieScene : IScene
{
    readonly List<FrameworkElement> _targets = [];
    readonly List<AnimatedIcon> _icons = [];
    readonly AnimatedVisualPlayer? _player;
    readonly bool _withPlayer;
    static readonly string[] s_states = ["PointerOver", "Pressed", "Normal"];
    int _state;

    public string Name => _withPlayer ? "Lottie" : "Lottie-NoPlayer";
    public bool IsDynamic => false;
    public bool AnimatesWhileIdle => true; // ProgressRings
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => _targets;

    public LottieScene(BenchOptions options)
    {
        _withPlayer = !options.NoPlayer;
        var rings = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 24 };
        foreach (double size in new[] { 32.0, 48, 64 })
        {
            rings.Children.Add(new ProgressRing { IsActive = true, Width = size, Height = size });
        }
        var determinate = new ProgressRing { IsIndeterminate = false, Value = 60, Width = 48, Height = 48 };
        rings.Children.Add(determinate);

        var icons = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 12 };
        IAnimatedVisualSource2[] sources =
        [
            new AnimatedSettingsVisualSource(), new AnimatedBackVisualSource(), new AnimatedGlobalNavigationButtonVisualSource(),
            new AnimatedChevronDownSmallVisualSource(), new AnimatedFindVisualSource(), new AnimatedAcceptVisualSource(),
        ];
        foreach (var source in sources)
        {
            var icon = new AnimatedIcon { Source = source, Width = 32, Height = 32 };
            _icons.Add(icon);
            var button = new Button { Content = icon, Padding = new Thickness(8) };
            _targets.Add(button);
            icons.Children.Add(button);
        }
        var check = new CheckBox { Content = "CheckBox (animated glyph)" };
        _targets.Add(check);

        FrameworkElement playerSlot;
        if (_withPlayer)
        {
            _player = new AnimatedVisualPlayer { Source = new RotatingSquareSource(), Width = 120, Height = 120, AutoPlay = false };
            playerSlot = _player;
        }
        else
        {
            playerSlot = new Grid
            {
                Width = 120,
                Height = 120,
                Children = { new Rectangle { Width = 72, Height = 72, Fill = new SolidColorBrush(Colors.MediumSeaGreen) } },
            };
        }

        Root = new Grid
        {
            Background = new SolidColorBrush(Color.FromArgb(255, 0xFA, 0xFA, 0xFA)),
            Children =
            {
                new StackPanel
                {
                    Spacing = 20,
                    Padding = new Thickness(24),
                    Children =
                    {
                        new TextBlock { Text = "Lottie", Style = SettingsScene.Style("TitleTextBlockStyle") },
                        FullBackend.Title("ProgressRing"), rings,
                        FullBackend.Title("AnimatedIcon"), icons, check,
                        FullBackend.Title(_withPlayer ? "AnimatedVisualPlayer (custom source)" : "Static placeholder"), playerSlot,
                    },
                },
            },
        };
    }

    public void Attach(ISceneHost host) => _ = _player?.PlayAsync(0, 1, looped: true);

    public int ApplyUpdate() => 0;

    public void Activate(FrameworkElement target)
    {
        if (target is Button { Content: AnimatedIcon icon })
        {
            AnimatedIcon.SetState(icon, s_states[_state++ % s_states.Length]);
            return;
        }
        ProgrammaticInput.Activate(target);
    }

    public bool OnPhase(BenchPhase phase)
    {
        foreach (var icon in _icons)
        {
            AnimatedIcon.SetState(icon, "Normal");
        }
        return phase is BenchPhase.Steady or BenchPhase.Recovery;
    }

    public void Detach() => _player?.Stop();

    // A hand-written IAnimatedVisualSource: one ShapeVisual whose rotation is an expression on the player's Progress.
    sealed partial class RotatingSquareSource : IAnimatedVisualSource
    {
        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            diagnostics = null!;
            return new RotatingSquare(compositor);
        }
    }

    sealed partial class RotatingSquare : IAnimatedVisual
    {
        readonly ContainerVisual _root;

        public RotatingSquare(Compositor compositor)
        {
            _root = compositor.CreateContainerVisual();
            _root.Size = new Vector2(100, 100);
            _root.Properties.InsertScalar("Progress", 0);
            var shape = compositor.CreateShapeVisual();
            shape.Size = new Vector2(100, 100);
            shape.CenterPoint = new Vector3(50, 50, 0);
            var geometry = compositor.CreateRectangleGeometry();
            geometry.Size = new Vector2(60, 60);
            geometry.Offset = new Vector2(20, 20);
            var sprite = compositor.CreateSpriteShape(geometry);
            sprite.FillBrush = compositor.CreateColorBrush(Colors.MediumSeaGreen);
            shape.Shapes.Add(sprite);
            var rotation = compositor.CreateExpressionAnimation("_.Progress * 360");
            rotation.SetReferenceParameter("_", _root);
            shape.StartAnimation("RotationAngleInDegrees", rotation);
            _root.Children.InsertAtTop(shape);
        }

        public TimeSpan Duration => TimeSpan.FromSeconds(2);
        public Visual RootVisual => _root;
        public Vector2 Size => new(100, 100);
        public void Dispose() => _root.Dispose();
    }
}

// S19: text stress. RTL Arabic/Hebrew, Devanagari/Thai, color emoji, a CFF-outline OpenType font when one is installed
// (CffFontPresent scene value; the variant is skipped otherwise), Segoe UI Variable weights, ClearType (opaque
// background) vs grayscale (transparent/translucent background) text, a RichTextBlock with mixed runs, 72pt text and
// Items small TextBlocks in an ItemsRepeater. Interaction scrolls at ScrollSpeed.
sealed class TextStressScene : IScene
{
    readonly ScrollViewer _scroller;
    readonly ScrollDriver _driver;
    readonly FontProbe.CffFont? _cff;

    public string Name => "TextStress";
    public bool IsDynamic => false;
    public bool ContinuousInteraction => true;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => [];

    public TextStressScene(BenchOptions options)
    {
        _cff = FontProbe.FindCffFont(FontProbe.SystemFontFiles());
        var panel = new StackPanel { Spacing = 16, Padding = new Thickness(24) };
        panel.Children.Add(new TextBlock { Text = "Text stress", Style = SettingsScene.Style("TitleTextBlockStyle") });

        panel.Children.Add(Section("Right to left",
            Para("العربية: هذا نص تجريبي لاختبار الاتجاه من اليمين إلى اليسار مع أرقام 12345 وعلامات ترقيم، ونص English مضمن.", rtl: true),
            Para("עברית: זהו טקסט לדוגמה לבדיקת כיווניות מימין לשמאל, כולל מספרים 678 ומילים באנגלית mixed in.", rtl: true)));
        panel.Children.Add(Section("Complex scripts",
            Para("हिन्दी: यह जटिल लिपि का परीक्षण है — संयुक्ताक्षर क्ष त्र ज्ञ श्र और मात्राएँ कि की कु कू।"),
            Para("ไทย: นี่คือข้อความทดสอบภาษาไทย ที่มีวรรณยุกต์ ก่ ก้ ก๊ ก๋ และสระบนล่าง")));
        panel.Children.Add(Section("Color emoji",
            new TextBlock { Text = "😀 🎉 👩‍💻 🧑🏽‍🚀 🏳️‍🌈 ❤️ 🚀 🌍 🍕 🐱", FontFamily = new FontFamily("Segoe UI Emoji"), FontSize = 32 }));
        if (_cff is { } cff)
        {
            panel.Children.Add(Section($"CFF font: {cff.Family}",
                new TextBlock { Text = "The quick brown fox jumps over the lazy dog 0123456789", FontFamily = new FontFamily(cff.Family), FontSize = 24 }));
        }

        var weights = new StackPanel { Spacing = 2 };
        foreach (var (name, weight) in new[] { ("Light", 300), ("Regular", 400), ("SemiBold", 600), ("Bold", 700) })
        {
            weights.Children.Add(new TextBlock
            {
                Text = $"Segoe UI Variable {name}: The quick brown fox jumps over the lazy dog",
                FontFamily = new FontFamily("Segoe UI Variable Text"),
                FontWeight = new FontWeight { Weight = (ushort)weight },
                FontSize = 18,
            });
        }
        panel.Children.Add(Section("Variable weights", weights));

        // ClearType needs an opaque background under the text; on transparent/translucent backgrounds text is grayscale.
        var antialias = new Grid { ColumnSpacing = 12 };
        antialias.ColumnDefinitions.Add(new ColumnDefinition());
        antialias.ColumnDefinitions.Add(new ColumnDefinition());
        antialias.Children.Add(new Border { Background = new SolidColorBrush(Colors.White), Padding = new Thickness(8),
            Child = new TextBlock { Text = "Opaque background (ClearType eligible)", FontSize = 14 } });
        var translucent = new Border { Background = new SolidColorBrush(Color.FromArgb(0x60, 0x80, 0xC0, 0xFF)), Padding = new Thickness(8),
            Child = new TextBlock { Text = "Translucent background (grayscale)", FontSize = 14 } };
        Grid.SetColumn(translucent, 1);
        antialias.Children.Add(translucent);
        panel.Children.Add(Section("ClearType vs grayscale", antialias));

        var rich = new RichTextBlock { FontSize = 16, TextWrapping = TextWrapping.Wrap };
        var paragraph = new Paragraph();
        paragraph.Inlines.Add(new Run { Text = "Mixed runs: " });
        paragraph.Inlines.Add(new Bold { Inlines = { new Run { Text = "bold, " } } });
        paragraph.Inlines.Add(new Italic { Inlines = { new Run { Text = "italic, " } } });
        paragraph.Inlines.Add(new Underline { Inlines = { new Run { Text = "underlined, " } } });
        paragraph.Inlines.Add(new Run { Text = "colored, ", Foreground = new SolidColorBrush(Colors.Crimson) });
        paragraph.Inlines.Add(new Run { Text = "monospace, ", FontFamily = new FontFamily("Consolas") });
        paragraph.Inlines.Add(new Run { Text = "large ", FontSize = 28 });
        paragraph.Inlines.Add(new Hyperlink { Inlines = { new Run { Text = "a hyperlink" } } });
        paragraph.Inlines.Add(new Run { Text = " and עברית inline with 😀." });
        rich.Blocks.Add(paragraph);
        panel.Children.Add(Section("RichTextBlock", rich));

        panel.Children.Add(new TextBlock { Text = "Large 72pt Ag", FontSize = 96 }); // 72pt = 96 DIPs

        var items = Enumerable.Range(0, Math.Max(1, options.Items)).Select(i => $"Item {i:0000}").ToList();
        var repeater = new ItemsRepeater
        {
            ItemsSource = items,
            Layout = new UniformGridLayout { MinItemWidth = 96, MinItemHeight = 20, MinColumnSpacing = 4, MinRowSpacing = 2 },
            ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='{Binding}' FontSize='11'/></DataTemplate>"),
        };
        panel.Children.Add(Section($"{items.Count} small TextBlocks (ItemsRepeater)", repeater));

        _scroller = new ScrollViewer { Content = panel };
        _driver = new ScrollDriver(_scroller, Math.Max(1, options.ScrollSpeed));
        Root = new Grid { Background = new SolidColorBrush(Color.FromArgb(255, 0xFA, 0xFA, 0xFA)), Children = { _scroller } };
    }

    static TextBlock Para(string text, bool rtl = false) => new()
    {
        Text = text,
        FontSize = 18,
        TextWrapping = TextWrapping.Wrap,
        FlowDirection = rtl ? FlowDirection.RightToLeft : FlowDirection.LeftToRight,
    };

    static StackPanel Section(string title, params UIElement[] content)
    {
        var section = new StackPanel { Spacing = 6 };
        section.Children.Add(FullBackend.Title(title));
        foreach (var element in content)
        {
            section.Children.Add(element);
        }
        return section;
    }

    public int ApplyUpdate() => 0;

    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token)
        => host.InputMode == InputMode.SendInput
            ? host.Every(10, () => _driver.Wheel(host, _scroller), token)
            : host.EveryFrame(_driver.Advance, token);

    public bool OnPhase(BenchPhase phase)
    {
        if (phase is BenchPhase.Steady or BenchPhase.Recovery)
        {
            _scroller.ChangeView(null, 0, null, disableAnimation: true);
            return true;
        }
        return false;
    }

    public IEnumerable<(string Name, double Value)> Counters() => [("CffFontPresent", _cff is null ? 0 : 1)];
}

// S16: SwapChainPanel (a D3D11 composition swap chain cleared to an animated color every frame through raw COM) and a
// MediaPlayerElement looping a generated local clip (MediaComposition of color clips under %LOCALAPPDATA%\RenderBench\media;
// MediaAvailable = 0 when generation fails). --no-media (S16-nomedia) keeps the SwapChainPanel only. Both variants are
// expected to make the window fall back to Composition.
sealed class SwapChainScene : IScene
{
    const int PanelWidth = 400, PanelHeight = 300;
    readonly SwapChainPanel _panel = new() { Width = PanelWidth, Height = PanelHeight };
    readonly MediaPlayerElement? _media;
    readonly bool _withMedia;
    D3DSwapChain? _swapChain;
    string? _clip;
    double _time;
    long _presents;
    bool _swapChainOk;
    readonly TextBlock _status = new() { Text = "XAML text next to the swap chain content." };
    readonly System.Diagnostics.Stopwatch _clock = new();

    public string Name => _withMedia ? "SwapChainMedia" : "SwapChainMedia-NoMedia";
    public bool IsDynamic => true;
    public bool AnimatesWhileIdle => true;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => [];

    public SwapChainScene(BenchOptions options)
    {
        _withMedia = !options.NoMedia;
        var panels = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 24 };
        panels.Children.Add(new StackPanel { Spacing = 8, Children = { FullBackend.Title("SwapChainPanel (D3D11)"), _panel } });
        if (_withMedia)
        {
            _media = new MediaPlayerElement { Width = PanelWidth, Height = PanelHeight, AutoPlay = true, AreTransportControlsEnabled = false };
            panels.Children.Add(new StackPanel { Spacing = 8, Children = { FullBackend.Title("MediaPlayerElement"), _media } });
        }
        Root = new Grid
        {
            Background = new SolidColorBrush(Color.FromArgb(255, 0xFA, 0xFA, 0xFA)),
            Children =
            {
                new StackPanel
                {
                    Spacing = 16,
                    Padding = new Thickness(24),
                    Children =
                    {
                        new TextBlock { Text = "SwapChainPanel and media", Style = SettingsScene.Style("TitleTextBlockStyle") },
                        panels,
                        _status,
                    },
                },
            },
        };
    }

    public async Task PrepareAsync()
    {
        if (!_withMedia)
        {
            return;
        }
        try
        {
            string folder = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "RenderBench", "media");
            string path = System.IO.Path.Combine(folder, "colors.mp4");
            if (!File.Exists(path))
            {
                Directory.CreateDirectory(folder);
                var composition = new MediaComposition();
                foreach (var color in new[] { Colors.CornflowerBlue, Colors.Orange, Colors.MediumSeaGreen, Colors.Crimson })
                {
                    composition.Clips.Add(MediaClip.CreateFromColor(color, TimeSpan.FromSeconds(1)));
                }
                var storage = await StorageFolder.GetFolderFromPathAsync(folder);
                var file = await storage.CreateFileAsync("colors.tmp.mp4", CreationCollisionOption.ReplaceExisting);
                var render = composition.RenderToFileAsync(file, MediaTrimmingPreference.Fast).AsTask();
                if (await Task.WhenAny(render, Task.Delay(30000)) != render || await render != Windows.Media.Transcoding.TranscodeFailureReason.None)
                {
                    throw new InvalidOperationException("MediaComposition.RenderToFileAsync failed or timed out");
                }
                File.Move(file.Path, path, overwrite: true);
            }
            _clip = path;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"S16: no generated media clip, MediaPlayerElement stays empty: {ex.Message}");
        }
    }

    public void Attach(ISceneHost host)
    {
        try
        {
            double scale = _panel.XamlRoot.RasterizationScale;
            _swapChain = new D3DSwapChain((int)Math.Ceiling(PanelWidth * scale), (int)Math.Ceiling(PanelHeight * scale));
            _swapChain.Attach(_panel);
            _swapChainOk = true;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"S16: D3D11 swap chain unavailable: {ex}");
        }
        if (_media is not null && _clip is not null)
        {
            _media.Source = MediaSource.CreateFromUri(new Uri(_clip));
            if (_media.MediaPlayer is { } player)
            {
                player.IsLoopingEnabled = true;
                player.IsMuted = true;
            }
        }
        _clock.Start();
        CompositionTarget.Rendering += OnRendering;
    }

    void OnRendering(object? sender, object e)
    {
        if (_swapChain is null)
        {
            return;
        }
        _time = _clock.Elapsed.TotalSeconds;
        try
        {
            _swapChain.Present(
                (float)(0.5 + 0.5 * Math.Sin(_time * 2)),
                (float)(0.5 + 0.5 * Math.Sin(_time * 2 + 2.1)),
                (float)(0.5 + 0.5 * Math.Sin(_time * 2 + 4.2)));
            _presents++;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"S16: Present failed: {ex.Message}");
            _swapChain.Dispose();
            _swapChain = null;
        }
    }

    // XAML content next to the swap chain changes at UpdateHz so the window keeps drawing XAML frames too.
    public int ApplyUpdate()
    {
        _status.Text = $"XAML text next to the swap chain content: {_presents} presents.";
        return 1;
    }

    public IEnumerable<(string Name, double Value)> Counters() =>
        [("SwapChainAvailable", _swapChainOk ? 1 : 0), ("MediaAvailable", _clip is null ? 0 : 1)];

    public void Detach()
    {
        CompositionTarget.Rendering -= OnRendering;
        _media?.MediaPlayer?.Pause();
        _swapChain?.Dispose();
        _swapChain = null;
    }
}

// S20: DPI, theme and resize. Items (100) resize cycles through a deterministic list of sizes, a RequestedTheme toggle
// every 5th cycle (20) and, when monitors with different DPI exist, a move to the other monitor every 25th cycle
// (DpiChangeAvailable; otherwise DPI is manual). Each change is an InputInjected "programmatic:resize|theme|dpi" for
// change-to-frame; after cycles 10 and 100 a full GC precedes steps cycle-10 / cycle-100 (leak check, < 2 MB).
sealed class DpiThemeScene : IScene
{
    readonly Grid _root;
    readonly int _cycles;
    readonly Random _random;

    public string Name => "DpiTheme";
    public bool IsDynamic => false;
    public FrameworkElement Root => _root;
    public IReadOnlyList<FrameworkElement> InputTargets => [];

    public DpiThemeScene(BenchOptions options)
    {
        _random = new Random(options.Seed);
        _cycles = Math.Max(1, options.Items);
        _root = new Grid { RequestedTheme = ElementTheme.Light, Children = { new SettingsScene(options).Root } };
        ApplyBackground();
    }

    void ApplyBackground() => _root.Background = new SolidColorBrush(_root.RequestedTheme == ElementTheme.Dark
        ? Color.FromArgb(255, 0x20, 0x20, 0x20) : Color.FromArgb(255, 0xF3, 0xF3, 0xF3));

    public int ApplyUpdate() => 0;

    public Task? WarmupAsync(ISceneHost host) => WarmUpAsync(host);

    async Task WarmUpAsync(ISceneHost host)
    {
        ToggleTheme();
        await FullBackend.NextFrameAsync();
        ToggleTheme();
        await FullBackend.NextFrameAsync();
    }

    void ToggleTheme()
    {
        Console.Error.WriteLine($"S20: theme -> {(_root.RequestedTheme == ElementTheme.Dark ? "Light" : "Dark")}");
        _root.RequestedTheme = _root.RequestedTheme == ElementTheme.Dark ? ElementTheme.Light : ElementTheme.Dark;
        ApplyBackground();
    }

    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token) => InteractAsync(host);

    // Runs all cycles regardless of the phase duration so the leak steps always exist.
    async Task InteractAsync(ISceneHost host)
    {
        var window = host.Window.AppWindow;
        var original = window.Size;
        var originalPosition = window.Position;
        uint dpi = Win32.DpiForWindow(host.Hwnd);
        var other = Win32.Monitors().FirstOrDefault(m => m.Dpi != dpi);
        BenchEventSource.Log.SceneValue("DpiChangeAvailable", other is null ? 0 : 1);
        bool onOther = false;
        int step = 0;
        for (int cycle = 1; cycle <= _cycles; cycle++)
        {
            var size = new SizeInt32(_random.Next(640, 1600), _random.Next(480, 1000));
            Console.Error.WriteLine($"S20: cycle {cycle} resize {size.Width}x{size.Height}");
            host.MarkInput("resize", "Window", programmatic: true);
            window.Resize(size);
            await Settle();
            if (cycle % 5 == 0)
            {
                host.MarkInput("theme", "Root", programmatic: true);
                ToggleTheme();
                await Settle();
            }
            if (other is not null && cycle % 25 == 0)
            {
                host.MarkInput("dpi", "Window", programmatic: true);
                window.Move(onOther ? originalPosition : new PointInt32(other.Left + 40, other.Top + 40));
                onOther = !onOther;
                await Settle();
            }
            if (cycle is MetricsLeakCycles.First or MetricsLeakCycles.Last)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                await Task.Delay(500);
                host.Step(step++, $"cycle-{cycle}", 1);
            }
        }
        if (onOther)
        {
            window.Move(originalPosition);
        }
        window.Resize(original);
        if (_root.RequestedTheme != ElementTheme.Light)
        {
            ToggleTheme();
        }
        await Settle();
    }

    static async Task Settle()
    {
        await FullBackend.NextFrameAsync();
        await Task.Delay(100);
    }
}

static class MetricsLeakCycles
{
    public const int First = RenderBench.Analysis.MetricsCalculator.LeakFirstCycle;
    public const int Last = RenderBench.Analysis.MetricsCalculator.LeakLastCycle;
}

// S21: device lost. The S2 dashboard updated at 30 Hz; in immediate modes Program.Main sets
// WINUI_IMMEDIATE_SIMULATE_DEVICE_LOST (--device-lost, default every:10) so the painter loses its device repeatedly.
// Metrics: DeviceLost to the next drawn frame (recovery), and private growth from the 10th to the 100th loss.
sealed class DeviceLostScene(BenchOptions options) : IScene
{
    readonly DashboardScene _dashboard = new(options);

    public string Name => "DeviceLost";
    public bool IsDynamic => true;
    public FrameworkElement Root => _dashboard.Root;
    public IReadOnlyList<FrameworkElement> InputTargets => _dashboard.InputTargets;
    public int ApplyUpdate() => _dashboard.ApplyUpdate();
    public void Activate(FrameworkElement target) => _dashboard.Activate(target);
    public IEnumerable<(string Name, double Value)> Counters() =>
        [("DeviceLostHookSet", string.IsNullOrEmpty(Environment.GetEnvironmentVariable(ImmediateHooks.SimulateDeviceLostVariable)) ? 0 : 1)];
}

// S22: accessibility and visual capture. Keyboard focus (focus visuals) moves across 50 controls at InputHz (Tab via
// SendInput, Focus(Keyboard) programmatically). Every 10th move, RenderTargetBitmap renders a static subtree and is
// compared with the same rectangle on screen: the WINUI_IMMEDIATE_CAPTURE BMP in immediate modes, a screen/PrintWindow
// capture otherwise (RtbSuccess, CaptureMeanAbsDiff). HighContrast is manual-only (test plan).
sealed class AccessibilityScene : IScene
{
    const int ControlCount = 50;
    readonly List<Control> _controls = [];
    readonly Border _captureTarget;
    int _focus = -1;
    int _moves;
    bool _capturing;

    public string Name => "Accessibility";
    public bool IsDynamic => false;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => _controls;

    public AccessibilityScene(BenchOptions options)
    {
        var random = new Random(options.Seed);
        var grid = new Grid { ColumnSpacing = 8, RowSpacing = 8 };
        const int columns = 5;
        for (int c = 0; c < columns; c++)
        {
            grid.ColumnDefinitions.Add(new ColumnDefinition());
        }
        for (int r = 0; r < ControlCount / columns; r++)
        {
            grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        }
        for (int i = 0; i < ControlCount; i++)
        {
            Control control = (i % 6) switch
            {
                0 => new Button { Content = $"Button {i}" },
                1 => new CheckBox { Content = $"Check {i}" },
                2 => new ToggleSwitch { Header = $"Toggle {i}" },
                3 => new RadioButton { Content = $"Radio {i}", GroupName = $"g{i / 12}" },
                4 => new HyperlinkButton { Content = $"Link {i}" },
                _ => new ToggleButton { Content = $"Toggle button {i}" },
            };
            Grid.SetRow(control, i / columns);
            Grid.SetColumn(control, i % columns);
            grid.Children.Add(control);
            _controls.Add(control);
        }

        var shapes = new StackPanel { Spacing = 6, Padding = new Thickness(12) };
        shapes.Children.Add(new TextBlock { Text = "Capture target", FontSize = 18, Foreground = new SolidColorBrush(Colors.Black) });
        shapes.Children.Add(new TextBlock { Text = "RenderTargetBitmap vs on-screen pixels", Foreground = new SolidColorBrush(Colors.DimGray) });
        for (int i = 0; i < 3; i++)
        {
            shapes.Children.Add(new Rectangle { Height = 20, Width = 120 + random.Next(120), HorizontalAlignment = HorizontalAlignment.Left,
                Fill = new SolidColorBrush(FullBackend.RandomColor(random)) });
        }
        _captureTarget = new Border
        {
            Width = 300,
            Height = 200,
            Background = new SolidColorBrush(Colors.White),
            BorderBrush = new SolidColorBrush(Colors.Gray),
            BorderThickness = new Thickness(1),
            Child = shapes,
            VerticalAlignment = VerticalAlignment.Top,
        };

        var layout = new Grid { Padding = new Thickness(24), ColumnSpacing = 24 };
        layout.ColumnDefinitions.Add(new ColumnDefinition());
        layout.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        layout.Children.Add(new StackPanel { Spacing = 12, Children = { new TextBlock { Text = "Keyboard focus", Style = SettingsScene.Style("TitleTextBlockStyle") }, grid } });
        Grid.SetColumn(_captureTarget, 1);
        layout.Children.Add(_captureTarget);
        Root = new ScrollViewer { Content = layout, Background = new SolidColorBrush(Color.FromArgb(255, 0xF7, 0xF7, 0xF7)) };
    }

    public int ApplyUpdate() => 0;

    public Task? WarmupAsync(ISceneHost host) => WarmUpAsync(host);

    async Task WarmUpAsync(ISceneHost host)
    {
        for (int i = 0; i < 5; i++)
        {
            MoveFocus(host, real: false);
            await Task.Delay(50);
        }
        await CompareAsync(host, emit: false);
    }

    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token)
        => host.Every(Math.Max(0.5, host.Options.InputHz), () =>
        {
            MoveFocus(host, real: host.InputMode == InputMode.SendInput);
            if (++_moves % 10 == 0 && !_capturing)
            {
                _ = CompareAsync(host, emit: true);
            }
        }, token);

    void MoveFocus(ISceneHost host, bool real)
    {
        _focus = (_focus + 1) % _controls.Count;
        if (real)
        {
            host.MarkInput("key", "Tab", programmatic: false);
            Win32.Key(0x09); // VK_TAB
        }
        else
        {
            host.MarkInput("focus", _controls[_focus].GetType().Name, programmatic: true);
            _controls[_focus].Focus(FocusState.Keyboard);
        }
    }

    async Task CompareAsync(ISceneHost host, bool emit)
    {
        _capturing = true;
        try
        {
            BgraImage rendered;
            try
            {
                var target = new RenderTargetBitmap();
                await target.RenderAsync(_captureTarget);
                var buffer = await target.GetPixelsAsync();
                rendered = new BgraImage(target.PixelWidth, target.PixelHeight,
                    System.Runtime.InteropServices.WindowsRuntime.WindowsRuntimeBufferExtensions.ToArray(buffer));
                if (emit)
                {
                    BenchEventSource.Log.SceneValue(Analysis.SceneValueNames.RtbSuccess, 1);
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"S22: RenderTargetBitmap failed: 0x{ex.HResult:X8} {ex.GetType().Name} {ex.Message}");
                if (emit)
                {
                    BenchEventSource.Log.SceneValue(Analysis.SceneValueNames.RtbSuccess, 0);
                }
                return;
            }
            await FullBackend.NextFrameAsync();
            await Task.Delay(50);
            var (x, y, width, height) = Win32.ElementClientRect(_captureTarget);
            BgraImage? screen = await ReadCaptureAsync(host);
            if (screen is null || IsUniform(screen))
            {
                return;
            }
            double diff = PixelDiff.MeanAbsDiff(rendered, screen.Crop(x, y, width, height));
            if (emit && !double.IsNaN(diff))
            {
                BenchEventSource.Log.SceneValue(Analysis.SceneValueNames.CaptureMeanAbsDiff, diff);
            }
        }
        finally
        {
            _capturing = false;
        }
    }

    // Immediate modes: the native hook writes the last presented buffer (client area) to WINUI_IMMEDIATE_CAPTURE; it is
    // opened without sharing while being written, so retry. Otherwise the screen.
    static async Task<BgraImage?> ReadCaptureAsync(ISceneHost host)
    {
        if (Environment.GetEnvironmentVariable(ImmediateHooks.CaptureVariable) is { Length: > 0 } path)
        {
            for (int attempt = 0; attempt < 10; attempt++)
            {
                try
                {
                    if (File.Exists(path))
                    {
                        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
                        using var memory = new MemoryStream();
                        await stream.CopyToAsync(memory);
                        if (BmpCodec.TryDecode(memory.ToArray()) is { } image)
                        {
                            return image;
                        }
                    }
                }
                catch (IOException)
                {
                }
                await Task.Delay(50);
            }
            Console.Error.WriteLine($"S22: no readable capture at {path}; comparing with the screen instead");
        }
        return Win32.CaptureClient(host.Hwnd);
    }

    static bool IsUniform(BgraImage image)
    {
        var p = image.Pixels;
        for (int i = 4; i < p.Length; i += 4)
        {
            if (p[i] != p[0] || p[i + 1] != p[1] || p[i + 2] != p[2])
            {
                return false;
            }
        }
        return true;
    }
}

// S23: real app shell. A NavigationView with 10 pages: settings-like pages (cards, toggles, expanders), an
// Explorer-like details view (ListView, 5 columns x Items rows) and a Gallery-like grid of control samples.
// Interaction navigates to the next page at InputHz ("programmatic:navigate", change-to-frame by kind); each page is
// built fresh on navigation.
sealed class AppShellScene : IScene
{
    readonly NavigationView _nav;
    readonly List<(NavigationViewItem Item, Func<FrameworkElement> Build)> _pages = [];
    readonly BenchOptions _options;
    int _page;

    public string Name => "AppShell";
    public bool IsDynamic => false;
    public FrameworkElement Root => _nav;
    public IReadOnlyList<FrameworkElement> InputTargets => [];

    public AppShellScene(BenchOptions options)
    {
        _options = options;
        _nav = new NavigationView { PaneDisplayMode = NavigationViewPaneDisplayMode.Left, IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed, IsSettingsVisible = false };
        (string Name, Symbol Icon, Func<FrameworkElement> Build)[] pages =
        [
            ("Home", Symbol.Home, () => SettingsPage("Home", 1)),
            ("Files", Symbol.Folder, DetailsPage),
            ("Gallery", Symbol.Pictures, GalleryPage),
            ("Display", Symbol.View, () => SettingsPage("Display", 2)),
            ("Sound", Symbol.Volume, () => SettingsPage("Sound", 3)),
            ("Network", Symbol.World, () => SettingsPage("Network", 4)),
            ("Personalization", Symbol.Edit, () => SettingsPage("Personalization", 5)),
            ("Apps", Symbol.AllApps, GalleryPage),
            ("Accounts", Symbol.Contact, () => SettingsPage("Accounts", 6)),
            ("About", Symbol.Help, () => SettingsPage("About", 7)),
        ];
        foreach (var (name, icon, build) in pages)
        {
            var item = new NavigationViewItem { Content = name, Icon = new SymbolIcon(icon), Tag = name };
            _nav.MenuItems.Add(item);
            _pages.Add((item, build));
        }
        Navigate(0);
    }

    void Navigate(int index)
    {
        _page = index % _pages.Count;
        var (item, build) = _pages[_page];
        _nav.SelectedItem = item;
        _nav.Header = item.Content;
        _nav.Content = build();
    }

    FrameworkElement SettingsPage(string title, int seed)
    {
        var random = new Random(_options.Seed * 31 + seed);
        var panel = new StackPanel { Spacing = 8, Padding = new Thickness(24, 0, 24, 24), MaxWidth = 900, HorizontalAlignment = HorizontalAlignment.Left };
        for (int i = 0; i < 8; i++)
        {
            var row = new Grid { ColumnSpacing = 12 };
            row.ColumnDefinitions.Add(new ColumnDefinition());
            row.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
            row.Children.Add(new StackPanel
            {
                Children =
                {
                    new TextBlock { Text = $"{title} setting {i + 1}" },
                    new TextBlock { Text = "Description of what this setting changes.", FontSize = 12, Opacity = 0.7 },
                },
            });
            var toggle = new ToggleSwitch { IsOn = random.Next(2) == 0, OnContent = "On", OffContent = "Off" };
            Grid.SetColumn(toggle, 1);
            row.Children.Add(toggle);
            panel.Children.Add(FullBackend.Card(row, 12));
            if (i % 3 == 2)
            {
                var expander = new Expander
                {
                    Header = $"{title} advanced {i / 3 + 1}",
                    IsExpanded = i == 2,
                    HorizontalAlignment = HorizontalAlignment.Stretch,
                    HorizontalContentAlignment = HorizontalAlignment.Stretch,
                    Content = new StackPanel
                    {
                        Spacing = 6,
                        Children = { new CheckBox { Content = "Option A", IsChecked = true }, new CheckBox { Content = "Option B" },
                            new Slider { Minimum = 0, Maximum = 100, Value = random.Next(100), Width = 240, HorizontalAlignment = HorizontalAlignment.Left } },
                    },
                };
                panel.Children.Add(expander);
            }
        }
        return new ScrollViewer { Content = panel };
    }

    static readonly string[] s_types = ["Text Document", "PNG File", "Folder", "Application", "PDF Document"];

    FrameworkElement DetailsPage()
    {
        var random = new Random(_options.Seed);
        var rows = Enumerable.Range(0, Math.Max(1, _options.Items))
            .Select(i => new[]
            {
                $"File {i:0000}.dat",
                new DateTime(2024, 1, 1).AddMinutes(random.Next(500000)).ToString("yyyy-MM-dd HH:mm", System.Globalization.CultureInfo.InvariantCulture),
                s_types[random.Next(s_types.Length)],
                $"{random.Next(1, 90000):N0} KB",
                $"user{random.Next(10)}",
            })
            .ToList();
        var template = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
            "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
            "<Grid Height='28'><Grid.ColumnDefinitions><ColumnDefinition Width='3*'/><ColumnDefinition Width='2*'/>" +
            "<ColumnDefinition Width='2*'/><ColumnDefinition Width='*'/><ColumnDefinition Width='*'/></Grid.ColumnDefinitions>" +
            "<TextBlock Text='{Binding [0]}' VerticalAlignment='Center'/>" +
            "<TextBlock Grid.Column='1' Text='{Binding [1]}' VerticalAlignment='Center' Opacity='0.8'/>" +
            "<TextBlock Grid.Column='2' Text='{Binding [2]}' VerticalAlignment='Center' Opacity='0.8'/>" +
            "<TextBlock Grid.Column='3' Text='{Binding [3]}' VerticalAlignment='Center' HorizontalAlignment='Right' Opacity='0.8'/>" +
            "<TextBlock Grid.Column='4' Text='{Binding [4]}' VerticalAlignment='Center' Margin='12,0,0,0' Opacity='0.8'/></Grid></DataTemplate>");
        var header = new Grid { Padding = new Thickness(16, 4, 16, 4) };
        string[] columns = ["Name", "Date modified", "Type", "Size", "Owner"];
        double[] stars = [3, 2, 2, 1, 1];
        for (int c = 0; c < columns.Length; c++)
        {
            header.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(stars[c], GridUnitType.Star) });
            var text = new TextBlock { Text = columns[c], FontWeight = Microsoft.UI.Text.FontWeights.SemiBold };
            Grid.SetColumn(text, c);
            header.Children.Add(text);
        }
        var list = new ListView { ItemsSource = rows, ItemTemplate = template, SelectionMode = ListViewSelectionMode.Extended };
        var page = new Grid();
        page.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        page.RowDefinitions.Add(new RowDefinition());
        Grid.SetRow(list, 1);
        page.Children.Add(header);
        page.Children.Add(list);
        return page;
    }

    FrameworkElement GalleryPage()
    {
        var random = new Random(_options.Seed);
        var grid = new VariableSizedWrapGrid { Orientation = Orientation.Horizontal, ItemWidth = 220, ItemHeight = 120, Margin = new Thickness(24, 0, 24, 24) };
        Func<FrameworkElement>[] samples =
        [
            () => new Button { Content = "Button" },
            () => new CheckBox { Content = "CheckBox", IsChecked = true },
            () => new ToggleSwitch { IsOn = true },
            () => new Slider { Value = random.Next(100), Width = 160 },
            () => new ProgressBar { Value = random.Next(100), Width = 160 },
            () => new RatingControl { Value = 3 },
            () => new ComboBox { ItemsSource = new[] { "One", "Two", "Three" }, SelectedIndex = 0, Width = 160 },
            () => new RadioButtons { Items = { "Small", "Large" }, SelectedIndex = 0 },
            () => new NumberBox { Value = random.Next(100), SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Inline, Width = 160 },
            () => new HyperlinkButton { Content = "Hyperlink" },
            () => new InfoBar { IsOpen = true, Title = "Info", Severity = InfoBarSeverity.Informational, IsClosable = false },
            () => new ToggleButton { Content = "Toggle" },
        ];
        for (int i = 0; i < 24; i++)
        {
            var sample = samples[i % samples.Length]();
            sample.HorizontalAlignment = HorizontalAlignment.Left;
            grid.Children.Add(FullBackend.Card(new StackPanel
            {
                Spacing = 8,
                Children = { new TextBlock { Text = sample.GetType().Name, FontWeight = Microsoft.UI.Text.FontWeights.SemiBold }, sample },
            }, 12));
        }
        return new ScrollViewer { Content = grid };
    }

    public int ApplyUpdate() => 0;

    public Task? WarmupAsync(ISceneHost host) => WarmupAsync();

    async Task WarmupAsync()
    {
        for (int i = 1; i <= _pages.Count; i++)
        {
            Navigate(i);
            await FullBackend.NextFrameAsync();
            await Task.Delay(100);
        }
        // Let the last navigation's transitions (selection indicator, page entrance) finish before Steady.
        await Task.Delay(1500);
    }

    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token)
        => host.Every(Math.Max(0.1, host.Options.InputHz), () =>
        {
            int next = (_page + 1) % _pages.Count;
            host.MarkInput("navigate", (string)_pages[next].Item.Tag, programmatic: true);
            Navigate(next);
        }, token);
}
