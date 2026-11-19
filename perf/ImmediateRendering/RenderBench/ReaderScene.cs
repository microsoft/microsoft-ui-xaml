using Microsoft.UI.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Media;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Text;

namespace RenderBench;

// S4: reader. A RichTextBlock of Items seeded paragraphs with mixed font families (Segoe UI, Georgia, Consolas),
// weights, italics and sizes plus a Hyperlink, inside a ScrollViewer. Interaction: slow continuous scrolling and
// selection changes (RichTextBlock.Select on visible text, or a SendInput drag).
sealed class ReaderScene : IScene
{
    static readonly string[] s_words =
    [
        "render", "pipeline", "glyph", "layout", "measure", "arrange", "composition", "surface", "texture", "brush", "stroke",
        "geometry", "pixel", "frame", "present", "damage", "region", "clip", "opacity", "transform", "element", "visual",
        "window", "scroll", "viewport", "cache", "device", "driver", "shader", "batch", "atlas", "font", "kerning", "baseline",
        "the", "a", "of", "and", "to", "in", "with", "for", "is", "that", "on", "as", "by", "from", "each", "every",
    ];
    static readonly FontFamily[] s_families = [new("Segoe UI"), new("Georgia"), new("Consolas")];
    static readonly FontWeight[] s_weights = [FontWeights.Normal, FontWeights.Normal, FontWeights.SemiBold, FontWeights.Bold];
    static readonly double[] s_sizes = [14, 14, 15, 16, 18];

    readonly RichTextBlock _text;
    readonly ScrollViewer _viewer;
    readonly Random _random;
    readonly double _speed;
    ScrollDriver? _driver;

    public string Name => "Reader";
    public bool IsDynamic => false;
    public bool ContinuousInteraction => true;
    public FrameworkElement Root { get; }
    public IReadOnlyList<FrameworkElement> InputTargets => [];

    public ReaderScene(BenchOptions options)
    {
        _speed = options.ScrollSpeed;
        _random = new Random(options.Seed);
        var random = new Random(options.Seed);
        _text = new RichTextBlock { IsTextSelectionEnabled = true, Padding = new Thickness(48, 24, 48, 48), MaxWidth = 900,
            TextWrapping = TextWrapping.Wrap };

        var title = new Paragraph { FontSize = 30, FontFamily = s_families[1], Margin = new Thickness(0, 0, 0, 16) };
        title.Inlines.Add(new Run { Text = "On the Rendering of Text and Shapes" });
        _text.Blocks.Add(title);

        int hyperlinkParagraph = Math.Min(2, Math.Max(0, options.Items - 1));
        for (int p = 0; p < options.Items; p++)
        {
            var paragraph = new Paragraph { Margin = new Thickness(0, 0, 0, 12), LineHeight = 0 };
            int runs = 5 + random.Next(6);
            for (int r = 0; r < runs; r++)
            {
                int words = 4 + random.Next(14);
                string text = string.Join(' ', Enumerable.Range(0, words).Select(_ => s_words[random.Next(s_words.Length)])) + " ";
                var run = new Run
                {
                    Text = text,
                    FontFamily = s_families[random.Next(s_families.Length)],
                    FontWeight = s_weights[random.Next(s_weights.Length)],
                    FontStyle = random.Next(5) == 0 ? FontStyle.Italic : FontStyle.Normal,
                    FontSize = s_sizes[random.Next(s_sizes.Length)],
                };
                paragraph.Inlines.Add(run);
                if (p == hyperlinkParagraph && r == 1)
                {
                    var link = new Hyperlink();
                    link.Inlines.Add(new Run { Text = "rendering backend notes" });
                    paragraph.Inlines.Add(link);
                    paragraph.Inlines.Add(new Run { Text = " " });
                }
            }
            _text.Blocks.Add(paragraph);
        }

        _viewer = new ScrollViewer { Content = _text, VerticalScrollBarVisibility = ScrollBarVisibility.Auto };
        Root = new Grid { Background = new SolidColorBrush(Color.FromArgb(255, 0xFF, 0xFF, 0xFF)), Children = { _viewer } };
    }

    public int ApplyUpdate() => 0;

    public void Attach(ISceneHost host) => _driver = new ScrollDriver(_viewer, _speed);

    public Task? WarmupAsync(ISceneHost host) => WarmupAsync();

    async Task WarmupAsync()
    {
        SelectVisible();
        _viewer.ChangeView(null, _viewer.ScrollableHeight, null, disableAnimation: true);
        await Task.Delay(300);
        _viewer.ChangeView(null, 0, null, disableAnimation: true);
        await Task.Delay(300);
    }

    // Steady starts with nothing selected and no focus caret.
    public bool OnPhase(BenchPhase phase)
    {
        if (phase is BenchPhase.Steady or BenchPhase.Recovery)
        {
            _text.Select(_text.ContentStart, _text.ContentStart);
            return true;
        }
        return false;
    }

    public Task? RunInteractionAsync(ISceneHost host, CancellationToken token)
    {
        if (_driver is null)
        {
            return null;
        }
        double hz = Math.Max(0.5, host.Options.InputHz);
        return Task.WhenAll(
            host.EveryFrame(_driver.Advance, token),
            host.Every(hz, () => Select(host), token));
    }

    void Select(ISceneHost host)
    {
        if (host.InputMode == InputMode.SendInput)
        {
            double fy = 0.15 + _random.NextDouble() * 0.7;
            var (x1, y1) = host.ScreenPoint(_viewer, 0.12, fy);
            var (x2, y2) = host.ScreenPoint(_viewer, 0.35 + _random.NextDouble() * 0.4, fy);
            host.MarkInput("drag", "RichTextBlock", programmatic: false);
            Win32.Drag(x1, y1, x2, y2);
            return;
        }
        host.MarkInput("select", "RichTextBlock", programmatic: true);
        SelectVisible();
    }

    // Selects a seeded run of characters starting at a point in the visible part of the text.
    void SelectVisible()
    {
        var point = _viewer.TransformToVisual(_text).TransformPoint(
            new Point(_viewer.ActualWidth * (0.1 + _random.NextDouble() * 0.3), _viewer.ActualHeight * (0.1 + _random.NextDouble() * 0.7)));
        var start = _text.GetPositionFromPoint(point);
        var end = start?.GetPositionAtOffset(20 + _random.Next(180), LogicalDirection.Forward) ?? null;
        if (start is not null && end is not null)
        {
            _text.Select(start, end);
        }
    }
}
