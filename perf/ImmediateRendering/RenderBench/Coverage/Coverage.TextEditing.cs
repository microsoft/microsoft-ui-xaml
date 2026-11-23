using Microsoft.UI;
using Microsoft.UI.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

namespace RenderBench.Coverage;

// F6 text editing: TextBox, PasswordBox, RichEditBox, AutoSuggestBox, NumberBox. RichEdit draws itself into XAML's
// recording render target (glyph runs, selection/composition rects, squiggle images) and the walk replays it; the caret
// is an exact destination invert. Pages that focus a box show a blinking caret, so captures are taken mid-blink and the
// caret may or may not be in either rendering; its few pixels stay well under the threshold.
static class TextEditingCoverage
{
    static StackPanel Page(params UIElement[] children)
    {
        var panel = new StackPanel { Spacing = 10, Padding = new Thickness(16), Width = 480, HorizontalAlignment = HorizontalAlignment.Left };
        foreach (var child in children)
        {
            panel.Children.Add(child);
        }
        return panel;
    }

    const string LongText = "The quick brown fox jumps over the lazy dog while the editor keeps scrolling to show the end of a long line.";

    [CoveragePage("F6-editing", "textbox")]
    public static CoveragePage TextBoxes(CoverageContext ctx) => new()
    {
        Root = Page(
            new TextBox { Text = "Single line", Header = "Header", Width = 300, HorizontalAlignment = HorizontalAlignment.Left },
            new TextBox { PlaceholderText = "Placeholder text", Width = 300, HorizontalAlignment = HorizontalAlignment.Left },
            new TextBox { Text = "Centered", TextAlignment = TextAlignment.Center, Width = 300, HorizontalAlignment = HorizontalAlignment.Left },
            new TextBox { Text = "Right aligned", TextAlignment = TextAlignment.Right, Width = 300, HorizontalAlignment = HorizontalAlignment.Left },
            new TextBox { Text = "Read only", IsReadOnly = true, Width = 300, HorizontalAlignment = HorizontalAlignment.Left },
            new TextBox { Text = "Disabled", IsEnabled = false, Width = 300, HorizontalAlignment = HorizontalAlignment.Left },
            new TextBox
            {
                AcceptsReturn = true, TextWrapping = TextWrapping.Wrap, Text = LongText + " " + LongText, Height = 90, Width = 300,
                HorizontalAlignment = HorizontalAlignment.Left, FontFamily = new FontFamily("Georgia"), Foreground = new SolidColorBrush(Colors.DarkBlue),
            }),
    };

    [CoveragePage("F6-editing", "textbox-selection")]
    public static CoveragePage Selection(CoverageContext ctx)
    {
        var box = new TextBox { Text = "Select part of this text", Width = 300, HorizontalAlignment = HorizontalAlignment.Left };
        var custom = new TextBox
        {
            Text = "Custom highlight color", Width = 300, HorizontalAlignment = HorizontalAlignment.Left,
            SelectionHighlightColor = new SolidColorBrush(Colors.DarkOrange),
        };
        var readOnly = new TextBox { Text = "Read-only selection", IsReadOnly = true, Width = 300, HorizontalAlignment = HorizontalAlignment.Left };
        return new()
        {
            Root = Page(box, custom, readOnly),
            Exercise = async () =>
            {
                // Selection highlight shows only in the focused box unless the selection is kept on focus loss.
                custom.SelectionHighlightColorWhenNotFocused = new SolidColorBrush(Colors.SteelBlue);
                readOnly.SelectionHighlightColorWhenNotFocused = new SolidColorBrush(Colors.SteelBlue);
                custom.Select(0, 6);
                readOnly.Select(5, 9);
                box.Focus(FocusState.Programmatic);
                box.Select(7, 4);
                await Task.Delay(150);
            },
        };
    }

    [CoveragePage("F6-editing", "textbox-scrolled")]
    public static CoveragePage Scrolled(CoverageContext ctx)
    {
        var single = new TextBox { Text = LongText, Width = 260, HorizontalAlignment = HorizontalAlignment.Left };
        var multi = new TextBox
        {
            // AcceptsReturn before Text: a single-line box keeps only the first line.
            AcceptsReturn = true,
            Text = string.Join("\r", Enumerable.Range(1, 20).Select(i => $"Line {i} of a multi-line TextBox")),
            Height = 110, Width = 300, HorizontalAlignment = HorizontalAlignment.Left,
        };
        ScrollViewer.SetVerticalScrollBarVisibility(multi, ScrollBarVisibility.Auto);
        return new()
        {
            Root = Page(single, multi),
            Exercise = async () =>
            {
                // The caret at the end scrolls the single-line view horizontally; the multi-line box scrolls vertically.
                multi.Focus(FocusState.Programmatic);
                multi.Select(multi.Text.Length, 0);
                await Task.Delay(100);
                single.Focus(FocusState.Programmatic);
                single.Select(single.Text.Length, 0);
                await Task.Delay(150);
            },
        };
    }

    [CoveragePage("F6-editing", "spell-check")]
    public static CoveragePage SpellCheck(CoverageContext ctx)
    {
        var box = new TextBox { IsSpellCheckEnabled = true, Width = 360, HorizontalAlignment = HorizontalAlignment.Left };
        return new()
        {
            Root = Page(box, new TextBox { Text = "No squiggles here: wrongg", IsSpellCheckEnabled = false, Width = 360, HorizontalAlignment = HorizontalAlignment.Left }),
            Exercise = async () =>
            {
                // The spell checker runs asynchronously on edits made in a focused box (like typing).
                box.Focus(FocusState.Programmatic);
                box.SelectedText = "Thiss sentense has misspeled words ";
                box.Select(box.Text.Length, 0);
                await Task.Delay(1500);
            },
        };
    }

    [CoveragePage("F6-editing", "passwordbox")]
    public static CoveragePage Passwords(CoverageContext ctx)
    {
        var revealed = new PasswordBox { Password = "revealed", PasswordRevealMode = PasswordRevealMode.Visible, Width = 260, HorizontalAlignment = HorizontalAlignment.Left };
        return new()
        {
            Root = Page(
                new PasswordBox { Password = "secret", Header = "Password", Width = 260, HorizontalAlignment = HorizontalAlignment.Left },
                new PasswordBox { Password = "custom", PasswordChar = "#", Width = 260, HorizontalAlignment = HorizontalAlignment.Left },
                new PasswordBox { PlaceholderText = "Enter password", Width = 260, HorizontalAlignment = HorizontalAlignment.Left },
                revealed),
        };
    }

    [CoveragePage("F6-editing", "richeditbox")]
    public static CoveragePage RichEdit(CoverageContext ctx)
    {
        var box = new RichEditBox { Width = 400, Height = 150, HorizontalAlignment = HorizontalAlignment.Left, IsSpellCheckEnabled = false };
        var plain = new RichEditBox { Width = 400, Height = 40, HorizontalAlignment = HorizontalAlignment.Left, PlaceholderText = "Empty RichEditBox" };
        return new()
        {
            Root = Page(box, plain),
            Exercise = async () =>
            {
                var document = box.Document;
                document.SetText(TextSetOptions.None, "Bold, italic, underlined, colored and large text.\rSecond paragraph with a highlighted word and a wavy underline.");
                var bold = document.GetRange(0, 4);
                bold.CharacterFormat.Bold = FormatEffect.On;
                var italic = document.GetRange(6, 12);
                italic.CharacterFormat.Italic = FormatEffect.On;
                var underline = document.GetRange(14, 24);
                underline.CharacterFormat.Underline = UnderlineType.Single;
                var colored = document.GetRange(26, 33);
                colored.CharacterFormat.ForegroundColor = Colors.Crimson;
                var large = document.GetRange(38, 43);
                large.CharacterFormat.Size = 24;
                var highlighted = document.GetRange(74, 85);
                highlighted.CharacterFormat.BackgroundColor = Colors.Yellow;
                // Wavy underlines are recorded as images (the same path as spell-check squiggles).
                var wavy = document.GetRange(97, 111);
                wavy.CharacterFormat.Underline = UnderlineType.Wave;
                var thick = document.GetRange(38, 43);
                thick.CharacterFormat.Underline = UnderlineType.Thick;
                // An inline image (RichEdit draws it as a bitmap into the text render target).
                // RichEdit reads the stream lazily; keep it alive for the process lifetime.
                s_swatch ??= await SwatchStream();
                document.GetRange(49, 49).InsertImage(24, 16, 12, VerticalCharacterAlignment.Baseline, "swatch", s_swatch);
                box.Focus(FocusState.Programmatic);
                document.Selection.SetRange(51, 57); // "Second", after the image
                await Task.Delay(150);
            },
        };
    }

    static Windows.Storage.Streams.IRandomAccessStream? s_swatch;

    // A 24x16 BMP (blue-to-orange stripes) in memory, for RichEditBox.InsertImage.
    static async Task<Windows.Storage.Streams.IRandomAccessStream> SwatchStream()
    {
        const int w = 24, h = 16;
        var bmp = new byte[54 + w * h * 4];
        bmp[0] = (byte)'B'; bmp[1] = (byte)'M';
        BitConverter.GetBytes(bmp.Length).CopyTo(bmp, 2);
        BitConverter.GetBytes(54).CopyTo(bmp, 10);
        BitConverter.GetBytes(40).CopyTo(bmp, 14);
        BitConverter.GetBytes(w).CopyTo(bmp, 18);
        BitConverter.GetBytes(h).CopyTo(bmp, 22);
        BitConverter.GetBytes((short)1).CopyTo(bmp, 26);
        BitConverter.GetBytes((short)32).CopyTo(bmp, 28);
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                int i = 54 + (y * w + x) * 4;
                bool stripe = (x / 4) % 2 == 0;
                bmp[i] = (byte)(stripe ? 200 : 0); bmp[i + 1] = (byte)(stripe ? 80 : 140); bmp[i + 2] = (byte)(stripe ? 20 : 255); bmp[i + 3] = 255;
            }
        }
        var stream = new Windows.Storage.Streams.InMemoryRandomAccessStream();
        using (var writer = new Windows.Storage.Streams.DataWriter(stream.GetOutputStreamAt(0)))
        {
            writer.WriteBytes(bmp);
            await writer.StoreAsync();
            await writer.FlushAsync();
            writer.DetachStream();
        }
        stream.Seek(0);
        return stream;
    }

    [CoveragePage("F6-editing", "suggest-number")]
    public static CoveragePage SuggestAndNumber(CoverageContext ctx) => new()
    {
        Root = Page(
            new AutoSuggestBox { Text = "Search text", PlaceholderText = "Search", QueryIcon = new SymbolIcon(Symbol.Find), Width = 300, HorizontalAlignment = HorizontalAlignment.Left },
            new AutoSuggestBox { PlaceholderText = "Empty search", Width = 300, HorizontalAlignment = HorizontalAlignment.Left },
            new NumberBox { Value = 42.5, Header = "NumberBox", Width = 200, HorizontalAlignment = HorizontalAlignment.Left },
            new NumberBox { Value = 7, SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Inline, Width = 200, HorizontalAlignment = HorizontalAlignment.Left }),
    };

    [CoveragePage("F6-editing", "caret")]
    public static CoveragePage Caret(CoverageContext ctx)
    {
        var ltr = new TextBox { Text = "Caret after this", Width = 260, HorizontalAlignment = HorizontalAlignment.Left };
        return new()
        {
            Root = Page(ltr, new TextBox { Text = "Unfocused box", Width = 260, HorizontalAlignment = HorizontalAlignment.Left }),
            Exercise = async () =>
            {
                ltr.Focus(FocusState.Programmatic);
                ltr.Select(6, 0);
                await Task.Delay(150);
            },
        };
    }
}
