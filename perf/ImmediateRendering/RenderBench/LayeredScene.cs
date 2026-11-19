using System.Numerics;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.UI;

namespace RenderBench;

// S7: layering and effects. Mica window backdrop, an acrylic card over colorful shapes, ThemeShadow cards
// (Translation Z=32) next to identical cards without a shadow, nested group opacity (0.8 over 0.6 with overlapping
// children), translucent overlays and a translucent rounded border over text. Interaction toggles overlay visibility
// (input) and cycles overlay opacity (updates). Purpose: group-opacity correctness, and how immediate modes report
// Composition-only effects (acrylic, shadow, mica) - see UnsupportedContentPolicy.
sealed class LayeredScene : IScene
{
    readonly List<FrameworkElement> _targets = [];
    readonly List<UIElement> _overlays = [];
    readonly Border _roundedOverlay;
    static readonly double[] s_opacities = [0.25, 0.45, 0.65, 0.85];
    int _update;

    public string Name => "Layered";
    public bool IsDynamic => true;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => _targets;

    public LayeredScene(BenchOptions options)
    {
        var random = new Random(options.Seed);
        var canvas = new Grid { Padding = new Thickness(24), ColumnSpacing = 24, RowSpacing = 24 };
        for (int i = 0; i < 3; i++)
        {
            canvas.ColumnDefinitions.Add(new ColumnDefinition());
        }
        canvas.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        canvas.RowDefinitions.Add(new RowDefinition());
        canvas.RowDefinitions.Add(new RowDefinition());

        var toolbar = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 12 };
        var overlayToggle = new ToggleButton { Content = "Overlays", IsChecked = true };
        overlayToggle.Checked += (_, _) => SetOverlays(true);
        overlayToggle.Unchecked += (_, _) => SetOverlays(false);
        var roundedToggle = new CheckBox { Content = "Rounded overlay", IsChecked = true };
        _targets.Add(overlayToggle);
        _targets.Add(roundedToggle);
        toolbar.Children.Add(new TextBlock { Text = "Layers", Style = SettingsScene.Style("TitleTextBlockStyle"), Margin = new Thickness(0, 0, 24, 0) });
        toolbar.Children.Add(overlayToggle);
        toolbar.Children.Add(roundedToggle);
        Grid.SetColumnSpan(toolbar, 3);
        canvas.Children.Add(toolbar);

        // Acrylic card over shapes so the blur has something to show.
        var acrylicHost = new Grid { MinHeight = 220 };
        for (int i = 0; i < 6; i++)
        {
            acrylicHost.Children.Add(new Ellipse
            {
                Width = 60 + random.Next(80),
                Height = 60 + random.Next(80),
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top,
                Margin = new Thickness(random.Next(200), random.Next(120), 0, 0),
                Fill = new SolidColorBrush(Color.FromArgb(255, (byte)random.Next(256), (byte)random.Next(256), (byte)random.Next(256))),
            });
        }
        acrylicHost.Children.Add(new Border
        {
            Margin = new Thickness(30),
            CornerRadius = new CornerRadius(8),
            Background = new AcrylicBrush { TintColor = Colors.White, TintOpacity = 0.4, FallbackColor = Color.FromArgb(255, 0xEE, 0xEE, 0xEE) },
            Child = new TextBlock { Text = "Acrylic card", Margin = new Thickness(16), FontSize = 18 },
        });
        Place(canvas, acrylicHost, 1, 0);

        // ThemeShadow cards cast onto the receiver grid behind them; the column beside has identical cards without.
        var shadowReceiver = new Grid();
        var shadowCards = new StackPanel { Spacing = 24, Padding = new Thickness(16) };
        var shadow = new ThemeShadow();
        shadow.Receivers.Add(shadowReceiver);
        var plainCards = new StackPanel { Spacing = 24, Padding = new Thickness(16) };
        for (int i = 0; i < 2; i++)
        {
            var card = Card($"Shadow card {i + 1}");
            card.Shadow = shadow;
            card.Translation = new Vector3(0, 0, 32);
            shadowCards.Children.Add(card);
            plainCards.Children.Add(Card($"No-shadow card {i + 1}"));
        }
        var shadowColumn = new Grid { Children = { shadowReceiver, shadowCards } };
        Place(canvas, shadowColumn, 1, 1);
        Place(canvas, plainCards, 1, 2);

        // Group opacity: the overlap of the children must not show through when the group is translucent.
        var outer = new Grid { Opacity = 0.8, MinHeight = 200 };
        var inner = new Grid { Opacity = 0.6 };
        inner.Children.Add(new Rectangle { Width = 160, Height = 110, Fill = new SolidColorBrush(Colors.RoyalBlue),
            HorizontalAlignment = HorizontalAlignment.Left, VerticalAlignment = VerticalAlignment.Top, Margin = new Thickness(10, 10, 0, 0) });
        inner.Children.Add(new Rectangle { Width = 160, Height = 110, Fill = new SolidColorBrush(Colors.OrangeRed),
            HorizontalAlignment = HorizontalAlignment.Left, VerticalAlignment = VerticalAlignment.Top, Margin = new Thickness(80, 60, 0, 0) });
        outer.Children.Add(new Rectangle { Fill = new SolidColorBrush(Colors.Gold), Width = 80, Height = 180,
            HorizontalAlignment = HorizontalAlignment.Right });
        outer.Children.Add(inner);
        Place(canvas, outer, 2, 0);

        // Text under translucent overlays.
        var textPanel = new StackPanel { Spacing = 4 };
        for (int i = 0; i < 6; i++)
        {
            textPanel.Children.Add(new TextBlock { Text = $"Line {i + 1}: overlays must blend with the text underneath.", FontSize = 14 });
        }
        var overlay = new Rectangle { Fill = new SolidColorBrush(Color.FromArgb(0x80, 0xFF, 0x30, 0x30)), Margin = new Thickness(0, 20, 60, 40) };
        _overlays.Add(overlay);
        var textHost = new Grid { Children = { textPanel, overlay } };
        Place(canvas, textHost, 2, 1);

        var roundedText = new StackPanel { Spacing = 4, Padding = new Thickness(8) };
        for (int i = 0; i < 6; i++)
        {
            roundedText.Children.Add(new TextBlock { Text = $"Rounded overlay over text, row {i + 1}", FontSize = 14 });
        }
        _roundedOverlay = new Border
        {
            CornerRadius = new CornerRadius(16),
            Background = new SolidColorBrush(Color.FromArgb(0x50, 0x00, 0x80, 0xFF)),
            BorderBrush = new SolidColorBrush(Color.FromArgb(0xA0, 0x00, 0x50, 0xC0)),
            BorderThickness = new Thickness(2),
            Margin = new Thickness(20, 16, 20, 16),
        };
        _overlays.Add(_roundedOverlay);
        roundedToggle.Checked += (_, _) => _roundedOverlay.Visibility = Visibility.Visible;
        roundedToggle.Unchecked += (_, _) => _roundedOverlay.Visibility = Visibility.Collapsed;
        Place(canvas, new Grid { Children = { roundedText, _roundedOverlay } }, 2, 2);

        // Transparent root so the Mica backdrop shows around the content.
        Root = new ScrollViewer { Content = canvas, Background = new SolidColorBrush(Colors.Transparent) };
    }

    static void Place(Grid grid, FrameworkElement element, int row, int column)
    {
        Grid.SetRow(element, row);
        Grid.SetColumn(element, column);
        grid.Children.Add(element);
    }

    static Border Card(string title) => new()
    {
        Height = 80,
        CornerRadius = new CornerRadius(8),
        Background = new SolidColorBrush(Colors.White),
        BorderBrush = new SolidColorBrush(Color.FromArgb(0x30, 0, 0, 0)),
        BorderThickness = new Thickness(1),
        Child = new TextBlock { Text = title, Margin = new Thickness(16), FontSize = 16 },
    };

    void SetOverlays(bool visible)
    {
        foreach (var overlay in _overlays)
        {
            overlay.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
        }
    }

    public void Attach(ISceneHost host) => host.Window.SystemBackdrop = new MicaBackdrop();

    public int ApplyUpdate()
    {
        double opacity = s_opacities[_update++ % s_opacities.Length];
        foreach (var overlay in _overlays)
        {
            overlay.Opacity = opacity;
        }
        return _overlays.Count;
    }

    public void Detach() { }
}
