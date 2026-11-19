using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.UI;

namespace RenderBench;

// Scenes are built in code from a seed so every backend renders the identical tree (test plan, "Target scenes").
// S1/S2 constraints: no animations that run while idle (no indeterminate progress, no caret focus), no popups, no
// acrylic/mica, so Steady can be idle and every backend can draw the content with plain fills, strokes and text. The
// extended scenes (S3-S10) relax this on purpose and declare it (AnimatesWhileIdle, expected unsupported content).
interface IScene
{
    string Name { get; }
    FrameworkElement Root { get; }
    IReadOnlyList<FrameworkElement> InputTargets { get; }
    bool IsDynamic { get; }
    // Returns how many elements changed; 0 for static scenes.
    int ApplyUpdate();

    // ---- Extended scenes ----
    // Draws during Steady by design (S6); the Steady gate is waived and Steady CPU is reported as the animation tax.
    bool AnimatesWhileIdle => false;
    // Interaction animates every frame (scrolling), so every frame gap counts toward dropped frames.
    bool ContinuousInteraction => false;
    // Before the content is shown (e.g. generate S5's images).
    Task PrepareAsync() => Task.CompletedTask;
    // After the content is loaded, before RunStarted (backdrops, always-on timers).
    void Attach(ISceneHost host) { }
    // Programmatic equivalent of clicking target.
    void Activate(FrameworkElement target) => ProgrammaticInput.Activate(target);
    // Custom Warmup/Interaction activity; null runs the default (touch every input target / clicks at InputHz).
    Task? WarmupAsync(ISceneHost host) => null;
    Task? RunInteractionAsync(ISceneHost host, CancellationToken token) => null;
    // Called on the UI thread before each phase begins. Return true after changing visible state (e.g. clearing a
    // selection before Steady): the runner then lets it render before the phase's PhaseBegin marker.
    bool OnPhase(BenchPhase phase) => false;
    // Scene counters, sampled on the UI thread every SampleIntervalMs (SceneValue events).
    IEnumerable<(string Name, double Value)> Counters() => [];
    // Close secondary windows, stop timers.
    void Detach() { }
}

static class SceneFactory
{
    public static IScene Create(BenchOptions options) => options.Scene switch
    {
        SceneId.Dashboard => new DashboardScene(options),
        _ => new SettingsScene(options),
    };
}

// S1: settings form. Cards of common controls; interaction toggles/selects them round-robin (state-change repaints).
sealed class SettingsScene : IScene
{
    readonly List<FrameworkElement> _targets = [];

    public string Name => "Settings";
    public bool IsDynamic => false;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => _targets;

    public SettingsScene(BenchOptions options)
    {
        var random = new Random(options.Seed);
        var columns = new Grid { ColumnSpacing = 16, Padding = new Thickness(24) };
        columns.ColumnDefinitions.Add(new ColumnDefinition());
        columns.ColumnDefinitions.Add(new ColumnDefinition());

        var left = new StackPanel { Spacing = 12 };
        var right = new StackPanel { Spacing = 12 };
        Grid.SetColumn(right, 1);
        columns.Children.Add(left);
        columns.Children.Add(right);

        left.Children.Add(new TextBlock { Text = "Settings", Style = Style("TitleTextBlockStyle") });
        for (int card = 0; card < 4; card++)
        {
            var panel = new StackPanel { Spacing = 8 };
            panel.Children.Add(new TextBlock { Text = $"Section {card + 1}", Style = Style("SubtitleTextBlockStyle") });
            for (int row = 0; row < 3; row++)
            {
                var toggle = new ToggleSwitch { Header = $"Option {card}.{row}", IsOn = random.Next(2) == 0 };
                _targets.Add(toggle);
                panel.Children.Add(toggle);
            }
            var check = new CheckBox { Content = $"Enable feature {card}", IsChecked = random.Next(2) == 0 };
            _targets.Add(check);
            panel.Children.Add(check);
            (card % 2 == 0 ? left : right).Children.Add(Card(panel));
        }

        var radios = new RadioButtons { Header = "Mode" };
        foreach (var label in new[] { "Balanced", "Performance", "Battery saver" })
        {
            var radio = new RadioButton { Content = label };
            radios.Items.Add(radio);
            _targets.Add(radio);
        }
        radios.SelectedIndex = 0;

        var slider = new Slider { Header = "Volume", Minimum = 0, Maximum = 100, Value = 40, Width = 240, HorizontalAlignment = HorizontalAlignment.Left };
        _targets.Add(slider);

        var misc = new StackPanel { Spacing = 8 };
        misc.Children.Add(radios);
        misc.Children.Add(slider);
        misc.Children.Add(new TextBox { Header = "Device name", Text = "RENDERBENCH-01", Width = 240, HorizontalAlignment = HorizontalAlignment.Left, IsTabStop = false });
        misc.Children.Add(new ProgressBar { Value = 65, Maximum = 100, Width = 240, HorizontalAlignment = HorizontalAlignment.Left });
        var buttons = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8 };
        buttons.Children.Add(new Button { Content = "Apply" });
        buttons.Children.Add(new Button { Content = "Reset", Style = Style("AccentButtonStyle") });
        buttons.Children.Add(new HyperlinkButton { Content = "Learn more" });
        misc.Children.Add(buttons);
        right.Children.Add(Card(misc));

        Root = new ScrollViewer { Content = columns, VerticalScrollBarVisibility = ScrollBarVisibility.Hidden };
    }

    public int ApplyUpdate() => 0;

    static Border Card(UIElement child) => new()
    {
        Child = child,
        Padding = new Thickness(16),
        CornerRadius = new CornerRadius(8),
        BorderThickness = new Thickness(1),
        BorderBrush = new SolidColorBrush(Color.FromArgb(0x20, 0, 0, 0)),
        Background = new SolidColorBrush(Color.FromArgb(0xFF, 0xFB, 0xFB, 0xFB)),
    };

    internal static Style Style(string key) => (Style)Application.Current.Resources[key];
}

// S2: dashboard. A grid of tiles (Border + text + bar). During Interaction a fixed fraction of tiles change value at
// UpdateHz (sparse damage); input clicks toggle a tile highlight.
sealed class DashboardScene : IScene
{
    sealed record Tile(Border Border, TextBlock Value, Rectangle Bar, double MaxBarWidth);

    readonly List<Tile> _tiles = [];
    readonly List<FrameworkElement> _targets = [];
    readonly Random _random;
    readonly int _updateCount;
    static readonly SolidColorBrush s_highlight = new(Colors.DarkOrange);
    static readonly SolidColorBrush s_border = new(Color.FromArgb(0x30, 0, 0, 0));

    public string Name => "Dashboard";
    public bool IsDynamic => true;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => _targets;

    public DashboardScene(BenchOptions options)
    {
        _random = new Random(options.Seed);
        _updateCount = Math.Max(1, (int)Math.Round(options.Tiles * options.UpdateFraction));
        int columns = (int)Math.Ceiling(Math.Sqrt(options.Tiles * 2.0));
        int rows = (int)Math.Ceiling(options.Tiles / (double)columns);

        var grid = new Grid { Padding = new Thickness(12), RowSpacing = 6, ColumnSpacing = 6 };
        for (int c = 0; c < columns; c++)
        {
            grid.ColumnDefinitions.Add(new ColumnDefinition());
        }
        for (int r = 0; r < rows; r++)
        {
            grid.RowDefinitions.Add(new RowDefinition());
        }

        const double barWidth = 44;
        for (int i = 0; i < options.Tiles; i++)
        {
            double value = _random.NextDouble();
            var text = new TextBlock { Text = Format(value), FontSize = 14, FontWeight = Microsoft.UI.Text.FontWeights.SemiBold };
            var bar = new Rectangle { Height = 4, Width = barWidth * value, HorizontalAlignment = HorizontalAlignment.Left,
                Fill = new SolidColorBrush(Color.FromArgb(0xFF, 0x00, 0x67, 0xC0)), RadiusX = 2, RadiusY = 2 };
            var stack = new StackPanel { Spacing = 2 };
            stack.Children.Add(new TextBlock { Text = $"M{i:000}", FontSize = 10, Opacity = 0.7 });
            stack.Children.Add(text);
            stack.Children.Add(bar);
            var border = new Border
            {
                Child = stack,
                Padding = new Thickness(6),
                CornerRadius = new CornerRadius(4),
                BorderThickness = new Thickness(1),
                BorderBrush = s_border,
                Background = new SolidColorBrush(Colors.White),
            };
            border.Tapped += OnTileTapped;
            Grid.SetRow(border, i / columns);
            Grid.SetColumn(border, i % columns);
            grid.Children.Add(border);
            _tiles.Add(new Tile(border, text, bar, barWidth));
        }

        // Click targets spread across the grid, deterministic for the seed.
        var order = Enumerable.Range(0, _tiles.Count).OrderBy(_ => _random.Next()).Take(Math.Min(16, _tiles.Count));
        _targets.AddRange(order.Select(i => (FrameworkElement)_tiles[i].Border));

        Root = new Grid { Background = new SolidColorBrush(Color.FromArgb(0xFF, 0xF3, 0xF3, 0xF3)), Children = { grid } };
    }

    public int ApplyUpdate()
    {
        for (int k = 0; k < _updateCount; k++)
        {
            var tile = _tiles[_random.Next(_tiles.Count)];
            double value = _random.NextDouble();
            tile.Value.Text = Format(value);
            tile.Bar.Width = tile.MaxBarWidth * value;
        }
        return _updateCount;
    }

    static void OnTileTapped(object sender, TappedRoutedEventArgs e) => ToggleHighlight((Border)sender);

    public void Activate(FrameworkElement target) => ToggleHighlight((Border)target);

    static void ToggleHighlight(Border border) => border.BorderBrush = border.BorderBrush == s_highlight ? s_border : s_highlight;

    static string Format(double value) => (value * 1000).ToString("F1", System.Globalization.CultureInfo.InvariantCulture);
}
