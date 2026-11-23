using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using static RenderBench.Coverage.CoveragePageKit;

namespace RenderBench.Coverage;

// F14 theming and accessibility (coverage branch). HighContrast is a system-wide setting (SystemParametersInfo
// SPI_SETHIGHCONTRAST) with no per-process or per-window override, so it is not part of the automated corpus; see the
// test plan's coverage-branch subsection.
static class CoverageTheming
{
    const string F = "F14-theming";

    static UIElement[] Sample() =>
    [
        new TextBlock { Text = "Heading text", FontSize = 20 },
        new Button { Content = "Button" },
        new CheckBox { Content = "Check box", IsChecked = true },
        new ToggleSwitch { Header = "Toggle", IsOn = true },
        new Slider { Width = 220, Value = 30 },
        new ComboBox { Width = 200, ItemsSource = Words(3), SelectedIndex = 0 },
        new ProgressBar { Width = 220, Value = 60 },
        new TextBox { Width = 220, Text = "Text box" },
    ];

    [CoveragePage(F, "rtl")]
    public static CoveragePage RightToLeft(CoverageContext c)
    {
        var root = Surface(ElementTheme.Light, Sample(), [Label("RTL list"), new ListView { Width = 200, Height = 120, ItemsSource = Words(4), SelectedIndex = 1 },
            new RadioButtons { ItemsSource = Words(2, "Radio"), SelectedIndex = 0 }, new RatingControl { Value = 2 }]);
        root.FlowDirection = FlowDirection.RightToLeft;
        return new() { Root = root };
    }

    [CoveragePage(F, "requestedtheme-subtree")]
    public static CoveragePage SubtreeTheme(CoverageContext c)
    {
        Border Panel(ElementTheme theme, string title)
        {
            var inner = Surface(theme, [Label(title), .. Sample()]);
            return ThemedBorder(null, "ControlStrokeColorDefaultBrush", inner).With(b => b.RequestedTheme = theme);
        }
        var outer = Surface(ElementTheme.Light, [Panel(ElementTheme.Light, "Light")], [Panel(ElementTheme.Dark, "Dark")],
            [new Border { RequestedTheme = ElementTheme.Dark, Child = Surface(ElementTheme.Default, [Label("Default inside Dark"), new Button { Content = "Inherits dark" },
                new Border { RequestedTheme = ElementTheme.Light, Child = Surface(ElementTheme.Default, [Label("Light inside Dark"), new Button { Content = "Light again" }]) }]) }]);
        return new() { Root = outer };
    }

    [CoveragePage(F, "theme-switch-runtime")]
    public static CoveragePage ThemeSwitch(CoverageContext c)
    {
        var root = Surface(ElementTheme.Light, Sample(), [Label("Switches Light -> Dark -> Light -> Dark"), new ListView { Width = 200, Height = 120, ItemsSource = Words(4), SelectedIndex = 2 }]);
        return new()
        {
            Root = root,
            Exercise = async () =>
            {
                foreach (var theme in new[] { ElementTheme.Dark, ElementTheme.Light, ElementTheme.Dark })
                {
                    root.RequestedTheme = theme;
                    await Settle(400);
                }
            },
        };
    }

    [CoveragePage(F, "viewbox-scale")]
    public static CoveragePage ViewboxScale(CoverageContext c)
    {
        var content = Surface(ElementTheme.Light, Sample());
        content.Width = 300;
        content.Height = 440;
        var root = Surface(ElementTheme.Light, [new Viewbox { Width = 450, Height = 660, Child = content }], [new Viewbox { Width = 180, Height = 264, Child = Surface(ElementTheme.Light, Sample()) }]);
        return new() { Root = root };
    }

    [CoveragePage(F, "rasterization-scale")]
    public static CoveragePage RasterizationScale(CoverageContext c)
    {
        var scaled = Surface(ElementTheme.Light, Sample());
        scaled.RasterizationScale = 2.0;
        var root = Surface(ElementTheme.Light, [Label("RasterizationScale = 2"), scaled], [Label("Scaled 1.5x by RenderTransform"),
            new Border { RenderTransform = new ScaleTransform { ScaleX = 1.5, ScaleY = 1.5 }, Child = Surface(ElementTheme.Light, Sample()[..4]) }]);
        return new() { Root = root };
    }

    // A focus rect covers ~0.3% of the window, so a missing one moves the mean diff by only ~6e-4: the focus pages use
    // thresholds just above their measured diff (2-5e-4; reveal 1.3e-3 without its glow) so a lost focus rect fails.
    static CoveragePage FocusPage(FocusVisualKind kind, ElementTheme theme)
    {
        var target = new Button { Content = "Keyboard focus" };
        var check = new CheckBox { Content = "Other control" };
        var root = Surface(theme, [Label($"FocusVisualKind.{kind}"), target, check, new Slider { Width = 200, Value = 40 }]);
        var previous = Application.Current.FocusVisualKind;
        return new()
        {
            Root = root,
            Exercise = async () =>
            {
                Application.Current.FocusVisualKind = kind;
                target.Focus(FocusState.Keyboard);
                await Settle();
            },
            Detach = () => Application.Current.FocusVisualKind = previous,
        };
    }

    [CoveragePage(F, "focus-highvisibility", MaxMeanAbsDiff = 0.001)] public static CoveragePage FocusHighVisibility(CoverageContext c) => FocusPage(FocusVisualKind.HighVisibility, ElementTheme.Light);
    [CoveragePage(F, "focus-highvisibility-dark", MaxMeanAbsDiff = 0.001)] public static CoveragePage FocusHighVisibilityDark(CoverageContext c) => FocusPage(FocusVisualKind.HighVisibility, ElementTheme.Dark);
    [CoveragePage(F, "focus-reveal", MaxMeanAbsDiff = 0.0015, ExpectedUnsupported = ["Reveal focus glow:approximated"])] public static CoveragePage FocusReveal(CoverageContext c) => FocusPage(FocusVisualKind.Reveal, ElementTheme.Light);
    [CoveragePage(F, "focus-dottedline", MaxMeanAbsDiff = 0.0006)] public static CoveragePage FocusDotted(CoverageContext c) => FocusPage(FocusVisualKind.DottedLine, ElementTheme.Light);

    [CoveragePage(F, "focus-custom-brushes", MaxMeanAbsDiff = 0.0008)]
    public static CoveragePage FocusCustom(CoverageContext c)
    {
        var target = new Button
        {
            Content = "Custom focus visual", FocusVisualPrimaryBrush = new SolidColorBrush(Microsoft.UI.Colors.OrangeRed),
            FocusVisualSecondaryBrush = new SolidColorBrush(Microsoft.UI.Colors.Yellow),
            FocusVisualPrimaryThickness = new Thickness(3), FocusVisualSecondaryThickness = new Thickness(2), FocusVisualMargin = new Thickness(-4),
        };
        var root = Surface(ElementTheme.Light, [target]);
        return new() { Root = root, Exercise = async () => { target.Focus(FocusState.Keyboard); await Settle(); } };
    }
}
