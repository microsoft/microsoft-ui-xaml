using Microsoft.UI.Text;
using Microsoft.UI.Xaml;
using Windows.UI.Text;

namespace XamlProfiler.Helpers;

/// <summary>
/// Static helpers consumed by x:Bind function bindings in the detail-pane
/// properties panel item template. A <c>WucPropertyRow</c> is either a section
/// header (bold, full-width, no value) or a normal name/value pair.
///
/// <para>Function bindings are used (not an <c>IValueConverter</c>) for the same
/// reason as <see cref="LinkHighlightHelper"/>: <c>MainWindow</c> derives from
/// <c>Window</c>, so converter resource lookup isn't available.</para>
/// </summary>
public static class PropertyRowHelper
{
    /// <summary>Header rows render bold; value rows render normal weight.</summary>
    public static FontWeight GetNameWeight(bool isHeader) =>
        isHeader ? FontWeights.SemiBold : FontWeights.Normal;

    /// <summary>The value column is hidden for header rows.</summary>
    public static Visibility GetValueVisibility(bool isHeader) =>
        isHeader ? Visibility.Collapsed : Visibility.Visible;

    /// <summary>A little top margin separates each section header from the rows above it.</summary>
    public static Thickness GetRowMargin(bool isHeader) =>
        isHeader ? new Thickness(0, 6, 0, 1) : new Thickness(0);
}
