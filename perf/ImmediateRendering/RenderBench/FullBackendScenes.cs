using System.Diagnostics;
using System.Numerics;
using Microsoft.UI;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Animation;
using Microsoft.UI.Xaml.Shapes;
using Windows.Foundation;
using Windows.Graphics;
using Windows.UI;

namespace RenderBench;

// Shared helpers for the full-backend scenes S11-S23 (test plan, "Full-backend scenes").
static class FullBackend
{
    // Completes on the next CompositionTarget.Rendering (the next UI frame), or after timeoutMs.
    public static Task NextFrameAsync(int timeoutMs = 2000)
    {
        var done = new TaskCompletionSource();
        EventHandler<object>? handler = null;
        handler = (_, _) =>
        {
            CompositionTarget.Rendering -= handler;
            done.TrySetResult();
        };
        CompositionTarget.Rendering += handler;
        return Task.WhenAny(done.Task, Task.Delay(timeoutMs));
    }

    public static Border Card(UIElement child, double padding = 16) => new()
    {
        Child = child,
        Padding = new Thickness(padding),
        CornerRadius = new CornerRadius(8),
        BorderThickness = new Thickness(1),
        BorderBrush = new SolidColorBrush(Color.FromArgb(0x30, 0, 0, 0)),
        Background = new SolidColorBrush(Color.FromArgb(0xFF, 0xFB, 0xFB, 0xFB)),
    };

    public static TextBlock Title(string text) => new() { Text = text, Style = SettingsScene.Style("SubtitleTextBlockStyle") };

    public static Color RandomColor(Random random) =>
        Color.FromArgb(255, (byte)random.Next(40, 230), (byte)random.Next(40, 230), (byte)random.Next(40, 230));
}

// S11: Composition interop. A SpriteVisual child (SetElementChildVisual) rotated by an expression animation, cards with
// implicit Offset animations on their handoff visuals (GetElementVisual), and a ConnectedAnimation between a thumbnail
// view and a detail view. Immediate modes are expected to switch this window to Composition (WindowFallback).
// --no-interop (S11-nointerop) keeps the layout and the state changes without any interop; it must stay immediate.
sealed class InteropScene : IScene
{
    readonly bool _interop;
    readonly Border _visualHost;
    readonly StackPanel _cards = new() { Spacing = 8 };
    readonly Grid _thumbnails = new() { ColumnSpacing = 8, RowSpacing = 8 };
    readonly Border _detail;
    readonly Border _detailImage;
    readonly List<Border> _thumbnailImages = [];
    readonly Button _reorder = new() { Content = "Reorder cards" };
    readonly Button _switchView = new() { Content = "Open detail" };
    int _selected;

    public string Name => _interop ? "CompositionInterop" : "CompositionInterop-NoInterop";
    public bool IsDynamic => false;
    public bool AnimatesWhileIdle => _interop; // the child visual rotates continuously
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => [_reorder, _switchView];

    public InteropScene(BenchOptions options)
    {
        _interop = !options.NoInterop;
        var random = new Random(options.Seed);
        var grid = new Grid { Padding = new Thickness(24), ColumnSpacing = 24, RowSpacing = 16 };
        for (int i = 0; i < 3; i++)
        {
            grid.ColumnDefinitions.Add(new ColumnDefinition());
        }
        grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        grid.RowDefinitions.Add(new RowDefinition());

        var toolbar = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 12 };
        toolbar.Children.Add(new TextBlock { Text = "Composition interop", Style = SettingsScene.Style("TitleTextBlockStyle") });
        toolbar.Children.Add(_reorder);
        toolbar.Children.Add(_switchView);
        Grid.SetColumnSpan(toolbar, 3);
        grid.Children.Add(toolbar);

        // Column 1: host of the child SpriteVisual (no-interop: an identical static square).
        _visualHost = new Border
        {
            Width = 200,
            Height = 200,
            Background = new SolidColorBrush(Color.FromArgb(255, 0xEE, 0xEE, 0xF4)),
            Child = _interop ? null : new Rectangle { Width = 120, Height = 120, Fill = new SolidColorBrush(Colors.MediumPurple) },
            VerticalAlignment = VerticalAlignment.Top,
        };
        Place(grid, new StackPanel { Spacing = 8, Children = { FullBackend.Title("Child visual"), _visualHost } }, 1, 0);

        // Column 2: cards that move when reordered (implicit Offset animation with interop).
        for (int i = 0; i < 6; i++)
        {
            _cards.Children.Add(new Border
            {
                Height = 48,
                CornerRadius = new CornerRadius(6),
                Background = new SolidColorBrush(FullBackend.RandomColor(random)),
                Child = new TextBlock { Text = $"Card {i + 1}", Margin = new Thickness(12), Foreground = new SolidColorBrush(Colors.White) },
            });
        }
        Place(grid, new StackPanel { Spacing = 8, Children = { FullBackend.Title("Implicit offset"), _cards } }, 1, 1);

        // Column 3: thumbnails and a detail view (connected animation between them with interop).
        for (int i = 0; i < 3; i++)
        {
            _thumbnails.ColumnDefinitions.Add(new ColumnDefinition());
            _thumbnails.RowDefinitions.Add(new RowDefinition { Height = new GridLength(80) });
        }
        for (int i = 0; i < 9; i++)
        {
            var image = new Border { CornerRadius = new CornerRadius(4), Background = new SolidColorBrush(FullBackend.RandomColor(random)) };
            Grid.SetRow(image, i / 3);
            Grid.SetColumn(image, i % 3);
            _thumbnails.Children.Add(image);
            _thumbnailImages.Add(image);
        }
        _detailImage = new Border { Height = 200, CornerRadius = new CornerRadius(4) };
        _detail = new Border
        {
            Visibility = Visibility.Collapsed,
            Child = new StackPanel { Spacing = 8, Children = { _detailImage, new TextBlock { Text = "Detail view", FontSize = 18 } } },
        };
        Place(grid, new StackPanel { Spacing = 8, Children = { FullBackend.Title("Connected animation"), new Grid { Children = { _thumbnails, _detail } } } }, 1, 2);

        _reorder.Click += (_, _) => Activate(_reorder);
        _switchView.Click += (_, _) => Activate(_switchView);
        Root = new Grid { Background = new SolidColorBrush(Color.FromArgb(255, 0xF7, 0xF7, 0xF7)), Children = { grid } };
    }

    static void Place(Grid grid, FrameworkElement element, int row, int column)
    {
        Grid.SetRow(element, row);
        Grid.SetColumn(element, column);
        grid.Children.Add(element);
    }

    public void Attach(ISceneHost host)
    {
        if (!_interop)
        {
            return;
        }
        var hostVisual = ElementCompositionPreview.GetElementVisual(_visualHost);
        var compositor = hostVisual.Compositor;
        var sprite = compositor.CreateSpriteVisual();
        sprite.Size = new Vector2(120, 120);
        sprite.Offset = new Vector3(40, 40, 0);
        sprite.CenterPoint = new Vector3(60, 60, 0);
        sprite.Brush = compositor.CreateColorBrush(Colors.MediumPurple);
        var clock = compositor.CreatePropertySet();
        clock.InsertScalar("t", 0);
        var tick = compositor.CreateScalarKeyFrameAnimation();
        tick.InsertKeyFrame(1, 1, compositor.CreateLinearEasingFunction());
        tick.Duration = TimeSpan.FromSeconds(2);
        tick.IterationBehavior = AnimationIterationBehavior.Forever;
        clock.StartAnimation("t", tick);
        var rotation = compositor.CreateExpressionAnimation("clock.t * 360");
        rotation.SetReferenceParameter("clock", clock);
        sprite.StartAnimation("RotationAngleInDegrees", rotation);
        ElementCompositionPreview.SetElementChildVisual(_visualHost, sprite);

        foreach (UIElement card in _cards.Children)
        {
            var offset = compositor.CreateVector3KeyFrameAnimation();
            offset.Target = "Offset";
            offset.InsertExpressionKeyFrame(1, "this.FinalValue");
            offset.Duration = TimeSpan.FromMilliseconds(300);
            var implicitAnimations = compositor.CreateImplicitAnimationCollection();
            implicitAnimations["Offset"] = offset;
            ElementCompositionPreview.GetElementVisual(card).ImplicitAnimations = implicitAnimations;
        }
    }

    public int ApplyUpdate() => 0;

    public void Activate(FrameworkElement target)
    {
        if (target == _reorder)
        {
            var first = _cards.Children[0];
            _cards.Children.RemoveAt(0);
            _cards.Children.Add(first);
            return;
        }
        bool toDetail = _detail.Visibility == Visibility.Collapsed;
        var source = toDetail ? _thumbnailImages[_selected] : _detailImage;
        ConnectedAnimation? animation = null;
        if (_interop)
        {
            try
            {
                animation = ConnectedAnimationService.GetForCurrentView().PrepareToAnimate("image", source);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"S11: ConnectedAnimationService unavailable: {ex.Message}");
            }
        }
        if (toDetail)
        {
            _detailImage.Background = _thumbnailImages[_selected].Background;
            _selected = (_selected + 1) % _thumbnailImages.Count;
        }
        _detail.Visibility = toDetail ? Visibility.Visible : Visibility.Collapsed;
        _thumbnails.Visibility = toDetail ? Visibility.Collapsed : Visibility.Visible;
        _switchView.Content = toDetail ? "Back to thumbnails" : "Open detail";
        animation?.TryStart(toDetail ? _detailImage : _thumbnailImages[(_selected + _thumbnailImages.Count - 1) % _thumbnailImages.Count]);
    }

    // Steady starts from the thumbnail view.
    public bool OnPhase(BenchPhase phase)
    {
        if (phase is BenchPhase.Steady or BenchPhase.Recovery && _detail.Visibility == Visibility.Visible)
        {
            Activate(_switchView);
            return true;
        }
        return false;
    }
}

// S13: materials. Mica window backdrop, in-app acrylic panes over colorful shapes, ThemeShadow cards and a
// flyout-like panel (Translation Z), layered-border elevation, and LinearGradientBrush fills (also as a text
// foreground). Interaction toggles elevation and acrylic (input) and moves gradient stops (updates). Immediate modes
// must render all of it without UnsupportedContent; only Mica may be approximated (detail ":approximated").
sealed class MaterialsScene : IScene
{
    readonly List<FrameworkElement> _targets = [];
    readonly List<Border> _shadowCards = [];
    readonly List<UIElement> _acrylicPanes = [];
    readonly List<LinearGradientBrush> _gradients = [];
    int _update;

    public string Name => "Materials";
    public bool IsDynamic => true;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => _targets;

    public MaterialsScene(BenchOptions options)
    {
        var random = new Random(options.Seed);
        var grid = new Grid { Padding = new Thickness(24), ColumnSpacing = 24, RowSpacing = 24 };
        for (int i = 0; i < 3; i++)
        {
            grid.ColumnDefinitions.Add(new ColumnDefinition());
        }
        grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        grid.RowDefinitions.Add(new RowDefinition());
        grid.RowDefinitions.Add(new RowDefinition());

        var elevate = new ToggleButton { Content = "Elevated", IsChecked = true };
        elevate.Checked += (_, _) => SetElevation(32);
        elevate.Unchecked += (_, _) => SetElevation(0);
        var acrylic = new CheckBox { Content = "Acrylic panes", IsChecked = true };
        acrylic.Checked += (_, _) => SetAcrylic(true);
        acrylic.Unchecked += (_, _) => SetAcrylic(false);
        _targets.Add(elevate);
        _targets.Add(acrylic);
        var gradientTitle = new TextBlock { Text = "Materials", FontSize = 28, FontWeight = Microsoft.UI.Text.FontWeights.SemiBold,
            Foreground = Gradient(new Point(0, 0), new Point(1, 0), Colors.DeepSkyBlue, Colors.MediumVioletRed) };
        var toolbar = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 12, Children = { gradientTitle, elevate, acrylic } };
        Grid.SetColumnSpan(toolbar, 3);
        grid.Children.Add(toolbar);

        // Two acrylic panes over shapes.
        for (int pane = 0; pane < 2; pane++)
        {
            var host = new Grid { MinHeight = 200 };
            for (int i = 0; i < 5; i++)
            {
                host.Children.Add(new Ellipse
                {
                    Width = 50 + random.Next(80),
                    Height = 50 + random.Next(80),
                    HorizontalAlignment = HorizontalAlignment.Left,
                    VerticalAlignment = VerticalAlignment.Top,
                    Margin = new Thickness(random.Next(220), random.Next(120), 0, 0),
                    Fill = new SolidColorBrush(FullBackend.RandomColor(random)),
                });
            }
            var paneBorder = new Border
            {
                Margin = new Thickness(24),
                CornerRadius = new CornerRadius(8),
                Background = new AcrylicBrush
                {
                    TintColor = pane == 0 ? Colors.White : Color.FromArgb(255, 0x20, 0x20, 0x30),
                    TintOpacity = pane == 0 ? 0.4 : 0.6,
                    FallbackColor = pane == 0 ? Color.FromArgb(255, 0xEE, 0xEE, 0xEE) : Color.FromArgb(255, 0x30, 0x30, 0x40),
                },
                Child = new TextBlock { Text = $"Acrylic pane {pane + 1}", Margin = new Thickness(16), FontSize = 18,
                    Foreground = new SolidColorBrush(pane == 0 ? Colors.Black : Colors.White) },
            };
            _acrylicPanes.Add(paneBorder);
            host.Children.Add(paneBorder);
            Place(grid, host, 1 + pane, 0);
        }

        // ThemeShadow cards casting onto a receiver, plus a flyout-like panel with a deeper shadow.
        var receiver = new Grid();
        var shadow = new ThemeShadow();
        shadow.Receivers.Add(receiver);
        var cards = new StackPanel { Spacing = 24, Padding = new Thickness(16) };
        for (int i = 0; i < 3; i++)
        {
            var card = Card($"Shadow card {i + 1}", Colors.White);
            card.Shadow = shadow;
            card.Translation = new Vector3(0, 0, 32);
            _shadowCards.Add(card);
            cards.Children.Add(card);
        }
        var flyoutLike = Card("Flyout-like panel (Z=64)", Color.FromArgb(255, 0xF9, 0xF9, 0xF9));
        flyoutLike.Shadow = shadow;
        flyoutLike.Translation = new Vector3(0, 0, 64);
        flyoutLike.Width = 220;
        flyoutLike.HorizontalAlignment = HorizontalAlignment.Right;
        cards.Children.Add(flyoutLike);
        Place(grid, new Grid { Children = { receiver, cards } }, 1, 1);
        Grid.SetRowSpan((FrameworkElement)grid.Children[^1], 2);

        // DropShadow-like elevation drawn with stacked translucent borders (no effect), next to gradient fills.
        var elevation = new Grid { Width = 220, Height = 100, HorizontalAlignment = HorizontalAlignment.Left };
        for (int i = 4; i >= 1; i--)
        {
            elevation.Children.Add(new Border
            {
                CornerRadius = new CornerRadius(8 + i),
                Margin = new Thickness(-i, -i + 2, -i, -i - 2),
                Background = new SolidColorBrush(Color.FromArgb((byte)(10 + 6 * (4 - i)), 0, 0, 0)),
            });
        }
        elevation.Children.Add(Card("Layered elevation", Colors.White));
        var gradients = new StackPanel { Spacing = 12, Children = { elevation } };
        (Point, Point, Color[])[] fills =
        [
            (new Point(0, 0), new Point(1, 0), [Colors.Orange, Colors.Gold]),
            (new Point(0, 0), new Point(0, 1), [Colors.SeaGreen, Colors.LightGreen]),
            (new Point(0, 0), new Point(1, 1), [Colors.RoyalBlue, Colors.MediumPurple, Colors.HotPink]),
        ];
        foreach (var (start, end, stops) in fills)
        {
            gradients.Children.Add(new Rectangle { Height = 48, RadiusX = 6, RadiusY = 6, Fill = Gradient(start, end, stops) });
        }
        gradients.Children.Add(new Border
        {
            Height = 48,
            BorderThickness = new Thickness(4),
            CornerRadius = new CornerRadius(6),
            BorderBrush = Gradient(new Point(0, 0), new Point(1, 0), Colors.Teal, Colors.Coral),
            Child = new TextBlock { Text = "Gradient border", Margin = new Thickness(12, 8, 12, 8) },
        });
        Place(grid, gradients, 1, 2);
        Grid.SetRowSpan(gradients, 2);

        // Transparent root so the Mica backdrop shows around the content.
        Root = new ScrollViewer { Content = grid, Background = new SolidColorBrush(Colors.Transparent) };
    }

    LinearGradientBrush Gradient(Point start, Point end, params Color[] colors)
    {
        var brush = new LinearGradientBrush { StartPoint = start, EndPoint = end };
        for (int i = 0; i < colors.Length; i++)
        {
            brush.GradientStops.Add(new GradientStop { Color = colors[i], Offset = colors.Length == 1 ? 0 : i / (double)(colors.Length - 1) });
        }
        _gradients.Add(brush);
        return brush;
    }

    static Border Card(string title, Color background) => new()
    {
        Height = 72,
        CornerRadius = new CornerRadius(8),
        Background = new SolidColorBrush(background),
        BorderBrush = new SolidColorBrush(Color.FromArgb(0x30, 0, 0, 0)),
        BorderThickness = new Thickness(1),
        Child = new TextBlock { Text = title, Margin = new Thickness(16), FontSize = 16 },
    };

    static void Place(Grid grid, FrameworkElement element, int row, int column)
    {
        Grid.SetRow(element, row);
        Grid.SetColumn(element, column);
        grid.Children.Add(element);
    }

    void SetElevation(float z)
    {
        foreach (var card in _shadowCards)
        {
            card.Translation = new Vector3(0, 0, z);
        }
    }

    void SetAcrylic(bool visible)
    {
        foreach (var pane in _acrylicPanes)
        {
            pane.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
        }
    }

    public void Attach(ISceneHost host) => host.Window.SystemBackdrop = new MicaBackdrop();

    // Moves the first gradient stop of every fill (sparse damage on gradient-filled elements).
    public int ApplyUpdate()
    {
        double offset = 0.1 * (_update++ % 5);
        foreach (var brush in _gradients)
        {
            brush.GradientStops[0].Offset = offset;
        }
        return _gradients.Count;
    }
}

// S14: windowed popups in a small window, partially outside it: Flyout, MenuFlyout, ComboBox drop-down, TeachingTip
// and ToolTip (ShouldConstrainToRootBounds=false where the API has it) plus plain windowed Popups. Interaction first
// opens 1..N plain popups cumulatively (steps baseline, popups-k: private MB per open popup), closes them, then cycles
// open/close over every popup kind (PopupOpened: request to first frame, by kind).
sealed class PopupsScene : IScene
{
    const int WindowWidth = 560, WindowHeight = 400, PlainPopups = 6;
    readonly Button _flyoutButton = new() { Content = "Flyout" };
    readonly Button _menuButton = new() { Content = "Menu" };
    readonly Button _tipButton = new() { Content = "Tip" };
    readonly Button _toolTipButton = new() { Content = "ToolTip" };
    readonly ComboBox _combo = new() { Width = 160, PlaceholderText = "Pick one" };
    readonly Flyout _flyout;
    readonly MenuFlyout _menu = new() { ShouldConstrainToRootBounds = false, Placement = FlyoutPlacementMode.Bottom };
    readonly TeachingTip _tip;
    readonly ToolTip _toolTip = new() { Content = "A tooltip that extends past the window edge", Placement = PlacementMode.Bottom };
    readonly List<Popup> _plain = [];
    readonly Grid _root;
    long _pending;
    string _pendingKind = "";

    static readonly string[] s_kinds = ["Flyout", "MenuFlyout", "ComboBox", "TeachingTip", "ToolTip", "Popup"];

    public string Name => "WindowedPopups";
    public bool IsDynamic => false;
    public FrameworkElement Root => _root;
    public IReadOnlyList<FrameworkElement> InputTargets => [_flyoutButton, _menuButton, _combo, _tipButton, _toolTipButton];

    public PopupsScene(BenchOptions options)
    {
        var random = new Random(options.Seed);
        _flyout = new Flyout
        {
            ShouldConstrainToRootBounds = false,
            Placement = FlyoutPlacementMode.Right,
            Content = new StackPanel
            {
                Width = 260,
                Spacing = 8,
                Children =
                {
                    new TextBlock { Text = "Windowed flyout", FontSize = 16 },
                    new TextBlock { Text = "Extends past the right edge of the window.", TextWrapping = TextWrapping.Wrap },
                    new Button { Content = "OK" },
                },
            },
        };
        _flyoutButton.Flyout = _flyout;
        for (int i = 0; i < 8; i++)
        {
            _menu.Items.Add(new MenuFlyoutItem { Text = $"Command {i + 1}" });
        }
        _menuButton.Flyout = _menu;
        for (int i = 0; i < 30; i++)
        {
            _combo.Items.Add($"Item {i + 1}");
        }
        ToolTipService.SetToolTip(_toolTipButton, _toolTip);
        var tipContent = new TextBlock { Text = "Teaching tip outside the window", TextWrapping = TextWrapping.Wrap };
        _tip = new TeachingTip
        {
            Title = "Windowed TeachingTip",
            Subtitle = "ShouldConstrainToRootBounds = false",
            Content = tipContent,
            Target = _tipButton,
            PreferredPlacement = TeachingTipPlacementMode.Right,
            ShouldConstrainToRootBounds = false,
        };
        _flyout.Opened += (_, _) => Opened("Flyout");
        _menu.Opened += (_, _) => Opened("MenuFlyout");
        _combo.DropDownOpened += (_, _) => Opened("ComboBox");
        _toolTip.Opened += (_, _) => Opened("ToolTip");
        tipContent.Loaded += (_, _) => Opened("TeachingTip"); // TeachingTip has no Opened event

        for (int i = 0; i < PlainPopups; i++)
        {
            var popup = new Popup
            {
                ShouldConstrainToRootBounds = false,
                HorizontalOffset = WindowWidth - 200 + i * 24,
                VerticalOffset = 80 + i * 36,
                Child = new Border
                {
                    Width = 260,
                    Height = 160,
                    CornerRadius = new CornerRadius(8),
                    BorderThickness = new Thickness(1),
                    BorderBrush = new SolidColorBrush(Color.FromArgb(0x60, 0, 0, 0)),
                    Background = new SolidColorBrush(FullBackend.RandomColor(random)),
                    Child = new TextBlock { Text = $"Windowed popup {i + 1}", Margin = new Thickness(12), FontSize = 16 },
                },
            };
            popup.Opened += (_, _) => Opened("Popup");
            _plain.Add(popup);
        }

        // Controls in the bottom-right corner so every popup crosses the window edge.
        var controls = new StackPanel
        {
            Spacing = 8,
            HorizontalAlignment = HorizontalAlignment.Right,
            VerticalAlignment = VerticalAlignment.Bottom,
            Margin = new Thickness(12),
            Children =
            {
                new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, Children = { _flyoutButton, _menuButton } },
                new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, Children = { _tipButton, _toolTipButton } },
                _combo,
            },
        };
        _root = new Grid
        {
            Background = new SolidColorBrush(Color.FromArgb(255, 0xF5, 0xF5, 0xF5)),
            Children =
            {
                new TextBlock { Text = "Windowed popups", Style = SettingsScene.Style("TitleTextBlockStyle"), Margin = new Thickness(16) },
                controls,
                _tip,
            },
        };
    }

    public void Attach(ISceneHost host)
    {
        host.Window.AppWindow.Resize(new SizeInt32(WindowWidth, WindowHeight));
        foreach (var popup in _plain)
        {
            popup.XamlRoot = _root.XamlRoot;
        }
    }

    void Opened(string kind)
    {
        if (_pending != 0 && kind == _pendingKind)
        {
            BenchEventSource.Log.PopupOpened(_pending, kind);
            _pending = 0;
        }
    }

    void Open(string kind, int plainIndex = 0)
    {
        switch (kind)
        {
            case "Flyout": _flyout.ShowAt(_flyoutButton); break;
            case "MenuFlyout": _menu.ShowAt(_menuButton); break;
            case "ComboBox": _combo.IsDropDownOpen = true; break;
            case "TeachingTip": _tip.IsOpen = true; break;
            case "ToolTip": _toolTip.IsOpen = true; break;
            default: _plain[plainIndex].IsOpen = true; break;
        }
    }

    void CloseAll()
    {
        _flyout.Hide();
        _menu.Hide();
        _combo.IsDropDownOpen = false;
        _tip.IsOpen = false;
        _toolTip.IsOpen = false;
        foreach (var popup in _plain)
        {
            popup.IsOpen = false;
        }
    }

    public int ApplyUpdate() => 0;

    public Task? WarmupAsync(ISceneHost host) => WarmupAsync();

    async Task WarmupAsync()
    {
        foreach (string kind in s_kinds)
        {
            Open(kind);
            await Task.Delay(300);
            CloseAll();
            await Task.Delay(150);
        }
    }

    public bool OnPhase(BenchPhase phase)
    {
        CloseAll();
        return phase is BenchPhase.Steady or BenchPhase.Recovery;
    }

    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token) => InteractAsync(host, token);

    async Task InteractAsync(ISceneHost host, CancellationToken token)
    {
        int step = 0;
        host.Step(step++, "baseline", 0);
        await Task.Delay(1000);
        for (int i = 0; i < _plain.Count; i++)
        {
            _plain[i].IsOpen = true;
            await FullBackend.NextFrameAsync();
            host.Step(step++, $"popups-{i + 1}", i + 1);
            await Task.Delay(1000);
        }
        CloseAll();
        host.Step(step, "closed", 0);
        await Task.Delay(500);

        double period = 1000 / Math.Max(0.25, host.Options.InputHz);
        for (int n = 0; !token.IsCancellationRequested; n++)
        {
            string kind = s_kinds[n % s_kinds.Length];
            _pendingKind = kind;
            _pending = host.MarkInput("popup", kind, programmatic: true);
            Open(kind, n / s_kinds.Length % _plain.Count);
            await Task.Delay((int)(period * 0.6));
            CloseAll();
            _pending = 0;
            await Task.Delay((int)(period * 0.4));
        }
    }

    public void Detach() => CloseAll();
}

// S15: independent animation and scroll while the UI thread is blocked. A looping Storyboard (opacity + translate,
// independent), a ProgressRing and an animated ScrollViewer.ChangeView over a long list; every 4 s the UI thread is
// blocked for 1000 ms (Thread.Sleep) between UiBlockBegin/UiBlockEnd. A background screen probe counts how often the
// animated lane changed on screen during the block (BlockScreenChanges / BlockMaxScreenGapMs scene values).
sealed class IndependentScene : IScene
{
    const int BlockMs = 1000, PeriodMs = 4000;
    readonly ScrollViewer _scroller;
    readonly Grid _lane;
    readonly Storyboard _storyboard = new();
    int _blockId;
    bool _scrollDown = true;

    public string Name => "IndependentAnimation";
    public bool IsDynamic => false;
    public bool AnimatesWhileIdle => true;
    public bool ContinuousInteraction => true;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => [];

    public IndependentScene(BenchOptions options)
    {
        var random = new Random(options.Seed);
        var list = new StackPanel();
        for (int i = 0; i < Math.Max(100, options.Items); i++)
        {
            list.Children.Add(new Grid
            {
                Height = 32,
                Background = new SolidColorBrush(i % 2 == 0 ? Colors.White : Color.FromArgb(255, 0xF2, 0xF2, 0xF2)),
                Children =
                {
                    new TextBlock { Text = $"Row {i + 1:0000}  {random.Next(100000):00000}", Margin = new Thickness(12, 6, 0, 0) },
                    new Rectangle { Width = random.Next(20, 120), Height = 8, HorizontalAlignment = HorizontalAlignment.Right,
                        Margin = new Thickness(0, 0, 12, 0), Fill = new SolidColorBrush(FullBackend.RandomColor(random)) },
                },
            });
        }
        _scroller = new ScrollViewer { Content = list };

        var square = new Rectangle
        {
            Width = 60,
            Height = 60,
            Fill = new SolidColorBrush(Colors.DodgerBlue),
            HorizontalAlignment = HorizontalAlignment.Left,
            RenderTransform = new TranslateTransform(),
        };
        var opacity = new DoubleAnimation { From = 1, To = 0.2, Duration = TimeSpan.FromSeconds(1), AutoReverse = true, RepeatBehavior = RepeatBehavior.Forever };
        Storyboard.SetTarget(opacity, square);
        Storyboard.SetTargetProperty(opacity, "Opacity");
        var translate = new DoubleAnimation { From = 0, To = 240, Duration = TimeSpan.FromSeconds(1.5), AutoReverse = true, RepeatBehavior = RepeatBehavior.Forever };
        Storyboard.SetTarget(translate, square.RenderTransform);
        Storyboard.SetTargetProperty(translate, "X");
        _storyboard.Children.Add(opacity);
        _storyboard.Children.Add(translate);
        _lane = new Grid { Width = 320, Height = 80, Background = new SolidColorBrush(Colors.White), Children = { square } };

        var side = new StackPanel
        {
            Spacing = 16,
            Padding = new Thickness(24),
            Children =
            {
                new TextBlock { Text = "Independent animation", Style = SettingsScene.Style("TitleTextBlockStyle") },
                _lane,
                new ProgressRing { IsActive = true, Width = 64, Height = 64, HorizontalAlignment = HorizontalAlignment.Left },
                new TextBlock { Text = $"The UI thread is blocked for {BlockMs} ms every {PeriodMs / 1000} s.", TextWrapping = TextWrapping.Wrap },
            },
        };
        var grid = new Grid { Background = new SolidColorBrush(Color.FromArgb(255, 0xFA, 0xFA, 0xFA)) };
        grid.ColumnDefinitions.Add(new ColumnDefinition());
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(380) });
        Grid.SetColumn(side, 1);
        grid.Children.Add(_scroller);
        grid.Children.Add(side);
        Root = grid;
    }

    public void Attach(ISceneHost host) => _storyboard.Begin();

    public int ApplyUpdate() => 0;

    public Task? WarmupAsync(ISceneHost host) => Task.Delay(500);

    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token) => InteractAsync(host, token);

    async Task InteractAsync(ISceneHost host, CancellationToken token)
    {
        while (!token.IsCancellationRequested)
        {
            host.MarkInput("scroll", "ScrollViewer", programmatic: true);
            _scroller.ChangeView(null, _scrollDown ? _scroller.ScrollableHeight : 0, null, disableAnimation: false);
            _scrollDown = !_scrollDown;
            await Task.Delay(300);
            Block(host);
            try
            {
                await Task.Delay(PeriodMs - 300 - BlockMs, token);
            }
            catch (TaskCanceledException)
            {
            }
        }
    }

    void Block(ISceneHost host)
    {
        var (x, y) = host.ScreenPoint(_lane, 0, 0);
        var (_, _, width, height) = Win32.ElementClientRect(_lane);
        var probe = new ScreenProbe(x, y, Math.Max(1, width), Math.Max(1, height));
        int id = ++_blockId;
        probe.Start();
        BenchEventSource.Log.UiBlockBegin(id, BlockMs);
        var clock = Stopwatch.StartNew();
        Thread.Sleep(BlockMs);
        double measured = clock.Elapsed.TotalMilliseconds;
        BenchEventSource.Log.UiBlockEnd(id, measured);
        var (changes, maxGap, valid) = probe.Stop(measured);
        BenchEventSource.Log.SceneValue("BlockScreenProbeValid", valid ? 1 : 0);
        if (valid)
        {
            BenchEventSource.Log.SceneValue("BlockScreenChanges", changes);
            BenchEventSource.Log.SceneValue("BlockMaxScreenGapMs", maxGap);
        }
    }

    public void Detach() => _storyboard.Stop();

    // Polls a screen rectangle on a background thread and records when its content changes.
    sealed class ScreenProbe(int x, int y, int width, int height)
    {
        readonly List<double> _changes = [];
        readonly Stopwatch _clock = new();
        Thread? _thread;
        volatile bool _stop;
        bool _sawContent;

        public void Start()
        {
            _clock.Start();
            _thread = new Thread(Run) { IsBackground = true, Name = "RenderBench screen probe" };
            _thread.Start();
        }

        void Run()
        {
            ulong previous = 0;
            while (!_stop)
            {
                byte[] pixels = Win32.CaptureScreen(x, y, width, height);
                if (!_sawContent && !pixels.AsSpan().SequenceEqual(new byte[pixels.Length]))
                {
                    _sawContent = true;
                }
                ulong hash = Hash(pixels);
                if (previous != 0 && hash != previous)
                {
                    lock (_changes)
                    {
                        _changes.Add(_clock.Elapsed.TotalMilliseconds);
                    }
                }
                previous = hash;
                Thread.Sleep(4);
            }
        }

        public (int Changes, double MaxGapMs, bool Valid) Stop(double blockMs)
        {
            _stop = true;
            _thread?.Join(1000);
            List<double> points;
            lock (_changes)
            {
                points = [0, .. _changes.Where(t => t <= blockMs), blockMs];
            }
            double gap = points.Zip(points.Skip(1), (a, b) => b - a).DefaultIfEmpty(blockMs).Max();
            return (points.Count - 2, gap, _sawContent);
        }

        static ulong Hash(byte[] pixels)
        {
            ulong hash = 14695981039346656037;
            for (int i = 0; i < pixels.Length; i += 4)
            {
                hash = (hash ^ BitConverter.ToUInt32(pixels, i)) * 1099511628211;
            }
            return hash == 0 ? 1 : hash;
        }
    }
}
