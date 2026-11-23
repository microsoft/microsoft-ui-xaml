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
                Text = LongText + " " + LongText, TextWrapping = TextWrapping.Wrap, AcceptsReturn = true, Height = 90, Width = 300,
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
            Text = string.Join("\r", Enumerable.Range(1, 20).Select(i => $"Line {i} of a multi-line TextBox")),
            AcceptsReturn = true, Height = 110, Width = 300, HorizontalAlignment = HorizontalAlignment.Left,
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
        var box = new TextBox { Text = "Thiss sentense has misspeled words", IsSpellCheckEnabled = true, Width = 360, HorizontalAlignment = HorizontalAlignment.Left };
        return new()
        {
            Root = Page(box, new TextBox { Text = "No squiggles here: wrongg", IsSpellCheckEnabled = false, Width = 360, HorizontalAlignment = HorizontalAlignment.Left }),
            Exercise = async () =>
            {
                // The spell checker runs asynchronously after the text is edited in a focused box.
                box.Focus(FocusState.Programmatic);
                box.Select(box.Text.Length, 0);
                box.Text += " ";
                await Task.Delay(1000);
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
                document.SetText(TextSetOptions.None, "Bold, italic, underlined, colored and large text.\rSecond paragraph with a highlighted word.");
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
                var highlighted = document.GetRange(79, 90);
                highlighted.CharacterFormat.BackgroundColor = Colors.Yellow;
                box.Focus(FocusState.Programmatic);
                document.Selection.SetRange(50, 56);
                await Task.Delay(150);
            },
        };
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
