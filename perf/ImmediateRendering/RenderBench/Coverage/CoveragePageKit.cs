using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;

namespace RenderBench.Coverage;

// Shared building blocks for coverage pages (coverage branch). Pages stay deterministic: fixed sizes, fixed data,
// no endless animations unless the page declares a looser threshold.
static class CoveragePageKit
{
    // A page surface with the theme's page background, laid out as columns of stacked samples.
    public static Grid Surface(ElementTheme theme, params UIElement[][] columns)
    {
        var grid = (Grid)XamlReader.Load($"<Grid {Xmlns} Background='{{ThemeResource ApplicationPageBackgroundThemeBrush}}'/>");
        grid.RequestedTheme = theme;
        grid.Padding = new Thickness(16);
        grid.ColumnSpacing = 24;
        for (int i = 0; i < columns.Length; i++)
        {
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
            var stack = new StackPanel { Spacing = 10 };
            foreach (var child in columns[i])
            {
                stack.Children.Add(child);
            }
            Grid.SetColumn(stack, i);
            grid.Children.Add(stack);
        }
        return grid;
    }

    // Light / dark / disabled variants of the same content.
    public static CoveragePage Light(Func<UIElement[][]> columns) => new() { Root = Surface(ElementTheme.Light, columns()) };
    public static CoveragePage Dark(Func<UIElement[][]> columns) => new() { Root = Surface(ElementTheme.Dark, columns()) };
    public static CoveragePage Disabled(Func<UIElement[][]> columns)
    {
        var surface = Surface(ElementTheme.Light, []);
        surface.Children.Add(new ContentControl
        {
            IsEnabled = false,
            HorizontalContentAlignment = HorizontalAlignment.Stretch,
            VerticalContentAlignment = VerticalAlignment.Stretch,
            Content = Surface(ElementTheme.Light, columns()),
        });
        surface.Padding = new Thickness(0);
        return new() { Root = surface };
    }

    const string Xmlns = "xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'";

    // A Border whose brushes are {ThemeResource} references, so they follow RequestedTheme and runtime theme changes
    // (a brush looked up in code is the app theme's brush and never changes).
    public static Border ThemedBorder(string? background, string? border = null, UIElement? child = null)
    {
        string attributes = (background is null ? "" : $" Background='{{ThemeResource {background}}}'")
            + (border is null ? "" : $" BorderBrush='{{ThemeResource {border}}}' BorderThickness='1'");
        var result = (Border)XamlReader.Load($"<Border {Xmlns}{attributes}/>");
        result.Child = child;
        return result;
    }

    public static TextBlock Label(string text) => new() { Text = text, FontSize = 12, Opacity = 0.7 };

    public static DataTemplate Template(string body) => (DataTemplate)XamlReader.Load(
        "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' " +
        "xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>" + body + "</DataTemplate>");

    public static string[] Words(int count, string prefix = "Item") => [.. Enumerable.Range(1, count).Select(i => $"{prefix} {i}")];

    // Lets layout, template application and open/close transitions finish before the next step.
    public static Task Settle(int ms = 700) => Task.Delay(ms);
}
