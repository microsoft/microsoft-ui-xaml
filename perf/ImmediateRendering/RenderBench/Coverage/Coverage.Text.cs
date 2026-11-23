using Microsoft.UI;
using Microsoft.UI.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.UI.Text;

namespace RenderBench.Coverage;

// F5 text: TextBlock and RichTextBlock features. Text draws from XAML's own DirectWrite glyph runs, lines (decorations,
// selection) and highlight rects, so every page should match Comp to within glyph antialiasing.
static class TextCoverage
{
    static StackPanel Page(params UIElement[] children)
    {
        var panel = new StackPanel { Spacing = 8, Padding = new Thickness(16), Width = 520, HorizontalAlignment = HorizontalAlignment.Left };
        foreach (var child in children)
        {
            panel.Children.Add(child);
        }
        return panel;
    }

    static TextBlock Text(string text, double size = 16) => new() { Text = text, FontSize = size };

    [CoveragePage("F5-text", "decorations")]
    public static CoveragePage Decorations(CoverageContext ctx) => new()
    {
        Root = Page(
            new TextBlock { Text = "Underlined text", FontSize = 18, TextDecorations = TextDecorations.Underline },
            new TextBlock { Text = "Strikethrough text", FontSize = 18, TextDecorations = TextDecorations.Strikethrough },
            new TextBlock { Text = "Both decorations", FontSize = 24, TextDecorations = TextDecorations.Underline | TextDecorations.Strikethrough, Foreground = new SolidColorBrush(Colors.DarkRed) },
            Rich(p =>
            {
                p.Inlines.Add(new Run { Text = "Mixed " });
                p.Inlines.Add(new Underline { Inlines = { new Run { Text = "underline" } } });
                p.Inlines.Add(new Run { Text = " and ", TextDecorations = TextDecorations.Strikethrough });
                p.Inlines.Add(new Bold { Inlines = { new Italic { Inlines = { new Run { Text = "bold italic" } } } } });
            })),
    };

    [CoveragePage("F5-text", "highlighters")]
    public static CoveragePage Highlighters(CoverageContext ctx)
    {
        var block = new TextBlock { Text = "The quick brown fox jumps over the lazy dog.", FontSize = 20, TextWrapping = TextWrapping.Wrap };
        var yellow = new TextHighlighter { Background = new SolidColorBrush(Colors.Yellow) };
        yellow.Ranges.Add(new TextRange { StartIndex = 4, Length = 5 });
        var blue = new TextHighlighter { Background = new SolidColorBrush(Colors.Navy), Foreground = new SolidColorBrush(Colors.White) };
        blue.Ranges.Add(new TextRange { StartIndex = 16, Length = 9 });
        block.TextHighlighters.Add(yellow);
        block.TextHighlighters.Add(blue);

        var rich = new RichTextBlock { FontSize = 16 };
        var paragraph = new Paragraph();
        paragraph.Inlines.Add(new Run { Text = "Highlighted across a paragraph in a RichTextBlock." });
        rich.Blocks.Add(paragraph);
        var green = new TextHighlighter { Background = new SolidColorBrush(Colors.LightGreen) };
        green.Ranges.Add(new TextRange { StartIndex = 0, Length = 11 });
        rich.TextHighlighters.Add(green);
        return new() { Root = Page(block, rich) };
    }

    [CoveragePage("F5-text", "typography")]
    public static CoveragePage Typography(CoverageContext ctx)
    {
        var ligatures = new TextBlock { Text = "office fifty flame", FontFamily = new FontFamily("Gabriola"), FontSize = 28 };
        var noLigatures = new TextBlock { Text = "office fifty flame", FontFamily = new FontFamily("Gabriola"), FontSize = 28 };
        Microsoft.UI.Xaml.Documents.Typography.SetStandardLigatures(noLigatures, false);
        var stylistic = new TextBlock { Text = "Stylistic set 6", FontFamily = new FontFamily("Gabriola"), FontSize = 28 };
        Microsoft.UI.Xaml.Documents.Typography.SetStylisticSet6(stylistic, true);
        var smallCaps = new TextBlock { Text = "Small Caps 1/2 0123", FontFamily = new FontFamily("Calibri"), FontSize = 22 };
        Microsoft.UI.Xaml.Documents.Typography.SetCapitals(smallCaps, FontCapitals.SmallCaps);
        Microsoft.UI.Xaml.Documents.Typography.SetFraction(smallCaps, FontFraction.Stacked);
        return new()
        {
            Root = Page(
                ligatures, noLigatures, stylistic, smallCaps,
                new TextBlock { Text = "Character spacing 200", FontSize = 18, CharacterSpacing = 200 },
                new TextBlock { Text = "Character spacing -50", FontSize = 18, CharacterSpacing = -50 },
                new TextBlock { Text = "Light", FontSize = 22, FontWeight = FontWeights.Light, FontStyle = FontStyle.Italic },
                new TextBlock { Text = "Black", FontSize = 22, FontWeight = FontWeights.Black, FontStretch = FontStretch.Condensed }),
        };
    }

    [CoveragePage("F5-text", "line-stacking")]
    public static CoveragePage LineStacking(CoverageContext ctx)
    {
        const string text = "Line stacking strategy with an explicit line height across several wrapped lines of text.";
        TextBlock Block(LineStackingStrategy strategy, double lineHeight) => new()
        {
            Text = text, FontSize = 15, TextWrapping = TextWrapping.Wrap, LineStackingStrategy = strategy, LineHeight = lineHeight,
            Width = 240, Margin = new Thickness(0, 0, 0, 4),
        };
        var grid = new Grid { ColumnSpacing = 12 };
        grid.ColumnDefinitions.Add(new ColumnDefinition());
        grid.ColumnDefinitions.Add(new ColumnDefinition());
        var left = Page(Block(LineStackingStrategy.MaxHeight, 30), Block(LineStackingStrategy.BlockLineHeight, 12));
        var right = Page(Block(LineStackingStrategy.BaselineToBaseline, 28));
        left.Padding = right.Padding = new Thickness(0);
        left.Width = right.Width = 250;
        Grid.SetColumn(right, 1);
        grid.Children.Add(left);
        grid.Children.Add(right);
        return new() { Root = Page(grid, new TextBlock { Text = "Tight line bounds", FontSize = 18, TextLineBounds = TextLineBounds.Tight }) };
    }

    [CoveragePage("F5-text", "trimming-wrapping")]
    public static CoveragePage TrimmingWrapping(CoverageContext ctx)
    {
        const string text = "A long sentence that will not fit in the width it was given and must be trimmed or wrapped.";
        return new()
        {
            Root = Page(
                new TextBlock { Text = text, FontSize = 16, TextTrimming = TextTrimming.CharacterEllipsis, Width = 300 },
                new TextBlock { Text = text, FontSize = 16, TextTrimming = TextTrimming.WordEllipsis, Width = 300 },
                new TextBlock { Text = text, FontSize = 16, TextTrimming = TextTrimming.Clip, Width = 300 },
                new TextBlock { Text = text + " " + text, FontSize = 16, TextWrapping = TextWrapping.Wrap, MaxLines = 2, TextTrimming = TextTrimming.CharacterEllipsis, Width = 300 },
                new TextBlock { Text = "Supercalifragilisticexpialidocious words", FontSize = 16, TextWrapping = TextWrapping.WrapWholeWords, Width = 150 },
                new TextBlock { Text = text, FontSize = 16, TextWrapping = TextWrapping.Wrap, TextAlignment = TextAlignment.Justify, Width = 300 },
                new TextBlock { Text = "Centered", FontSize = 16, TextAlignment = TextAlignment.Center, Width = 300 },
                new TextBlock { Text = "Right", FontSize = 16, TextAlignment = TextAlignment.Right, Width = 300 }),
        };
    }

    [CoveragePage("F5-text", "flow-direction")]
    public static CoveragePage FlowDirectionPage(CoverageContext ctx) => new()
    {
        Root = Page(
            new TextBlock { Text = "Right-to-left paragraph (English).", FontSize = 16, FlowDirection = FlowDirection.RightToLeft, Width = 400 },
            new TextBlock { Text = "שלום עולם — Hebrew text", FontSize = 18, FlowDirection = FlowDirection.RightToLeft, Width = 400 },
            new TextBlock { Text = "مرحبا بالعالم", FontSize = 20, TextReadingOrder = TextReadingOrder.DetectFromContent, Width = 400 },
            new TextBlock { Text = "Mixed English and עברית text", FontSize = 18, TextReadingOrder = TextReadingOrder.UseFlowDirection, Width = 400 },
            new TextBlock { Text = "Optical margin: Trim side bearings", FontSize = 28, OpticalMarginAlignment = OpticalMarginAlignment.TrimSideBearings },
            new TextBlock { Text = "Not scaled by text scale factor", FontSize = 16, IsTextScaleFactorEnabled = false }),
    };

    [CoveragePage("F5-text", "rich-text")]
    public static CoveragePage RichText(CoverageContext ctx)
    {
        var rich = new RichTextBlock { FontSize = 15, Width = 480, TextWrapping = TextWrapping.Wrap };
        var first = new Paragraph { TextIndent = 16 };
        first.Inlines.Add(new Run { Text = "A RichTextBlock with " });
        first.Inlines.Add(new Bold { Inlines = { new Run { Text = "bold" } } });
        first.Inlines.Add(new Run { Text = ", " });
        first.Inlines.Add(new Italic { Inlines = { new Run { Text = "italic", Foreground = new SolidColorBrush(Colors.SeaGreen) } } });
        first.Inlines.Add(new Run { Text = ", a " });
        first.Inlines.Add(new Hyperlink { NavigateUri = new Uri("https://example.com"), Inlines = { new Run { Text = "hyperlink" } } });
        first.Inlines.Add(new Run { Text = " and an inline UI container " });
        first.Inlines.Add(new InlineUIContainer
        {
            Child = new Rectangle { Width = 24, Height = 14, Fill = new SolidColorBrush(Colors.OrangeRed), RadiusX = 3, RadiusY = 3 },
        });
        first.Inlines.Add(new Run { Text = " inside the text flow." });
        var second = new Paragraph { Margin = new Thickness(0, 8, 0, 0) };
        second.Inlines.Add(new Run { Text = "Second paragraph, larger.", FontSize = 22, FontFamily = new FontFamily("Georgia") });
        second.Inlines.Add(new LineBreak());
        second.Inlines.Add(new InlineUIContainer { Child = new Button { Content = "Embedded button" } });
        rich.Blocks.Add(first);
        rich.Blocks.Add(second);

        // Overflow: the first column takes the first lines and the overflow continues the text.
        var source = new RichTextBlock { FontSize = 14, Width = 230, Height = 60, TextWrapping = TextWrapping.Wrap };
        var flow = new Paragraph();
        flow.Inlines.Add(new Run { Text = "Overflowing text continues from the RichTextBlock into a RichTextBlockOverflow placed in the second column of the grid." });
        source.Blocks.Add(flow);
        var overflow = new RichTextBlockOverflow { Width = 230, Height = 60 };
        source.OverflowContentTarget = overflow;
        var columns = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 16, Children = { source, overflow } };
        return new() { Root = Page(rich, columns) };
    }

    [CoveragePage("F5-text", "selection")]
    public static CoveragePage Selection(CoverageContext ctx)
    {
        var block = new TextBlock { Text = "Selected TextBlock text", FontSize = 20, IsTextSelectionEnabled = true };
        var rich = new RichTextBlock
        {
            FontSize = 18, IsTextSelectionEnabled = true, Width = 420, TextWrapping = TextWrapping.Wrap,
            SelectionHighlightColor = new SolidColorBrush(Colors.Orange),
        };
        var paragraph = new Paragraph();
        paragraph.Inlines.Add(new Run { Text = "A selection in a RichTextBlock that spans the end of one line and the start of the next." });
        rich.Blocks.Add(paragraph);
        return new()
        {
            Root = Page(block, rich),
            Exercise = async () =>
            {
                block.SelectAll();
                rich.Select(rich.ContentStart.GetPositionAtOffset(10, LogicalDirection.Forward), rich.ContentStart.GetPositionAtOffset(60, LogicalDirection.Forward));
                await Task.Delay(100);
            },
        };
    }

    // GDI+ antialiases large color/variable glyphs differently (measured 0.014).
    [CoveragePage("F5-text", "fonts", MaxMeanAbsDiff = 0.015)]
    public static CoveragePage Fonts(CoverageContext ctx) => new()
    {
        Root = Page(
            new TextBlock { Text = "Color emoji 😀🎉🌍👍🏽", FontSize = 28, FontFamily = new FontFamily("Segoe UI Emoji") },
            new TextBlock { Text = "Monochrome emoji 😀🎉", FontSize = 28, FontFamily = new FontFamily("Segoe UI Emoji"), IsColorFontEnabled = false },
            new TextBlock { Text = "Segoe UI Variable Light", FontSize = 24, FontFamily = new FontFamily("Segoe UI Variable Display"), FontWeight = FontWeights.Light },
            new TextBlock { Text = "Segoe UI Variable SemiBold", FontSize = 24, FontFamily = new FontFamily("Segoe UI Variable Display"), FontWeight = FontWeights.SemiBold },
            new TextBlock { Text = "Symbol \uE700 \uE721 \uE80F", FontSize = 24, FontFamily = new FontFamily("Segoe Fluent Icons, Segoe MDL2 Assets") },
            new TextBlock { Text = "Consolas monospace 0O1lI", FontSize = 20, FontFamily = new FontFamily("Consolas") },
            new TextBlock { Text = "日本語のテキスト 中文 한국어", FontSize = 22 }),
    };

    static RichTextBlock Rich(Action<Paragraph> fill)
    {
        var rich = new RichTextBlock { FontSize = 18 };
        var paragraph = new Paragraph();
        fill(paragraph);
        rich.Blocks.Add(paragraph);
        return rich;
    }
}
