using Microsoft.UI;
using Microsoft.UI.Xaml.Media;
using Windows.UI;
using XamlProfiler.Models;

namespace XamlProfiler.Helpers;

/// <summary>
/// Static helpers consumed by x:Bind function bindings in the tree item
/// templates: <c>{x:Bind LinkHighlightHelper.GetBackgroundBrush(LinkHighlight)}</c>
/// and <c>{x:Bind LinkHighlightHelper.GetBorderBrush(LinkHighlight)}</c>.
///
/// <para>
/// Why function bindings instead of an <c>IValueConverter</c>: <see cref="MainWindow"/>
/// derives from <c>Window</c> (not <c>FrameworkElement</c>), and the WinUI 3
/// x:Bind codegen requires a FrameworkElement root for converter resource
/// lookup. Function bindings dodge that path entirely.
/// </para>
///
/// <para>
/// Visual treatment:
/// <list type="bullet">
///   <item><b>Peer</b> — soft amber tint + bright opaque amber ring. The ring is
///   what makes the linked peer pop; the fill is kept low-alpha so the green/blue/orange
///   text (especially orange Composition text) stays readable.</item>
///   <item><b>Path</b> — very subtle amber tint, no ring. Reads as a dim breadcrumb
///   on every ancestor leading down to the Peer.</item>
///   <item><b>None</b> — transparent fill, transparent ring.</item>
/// </list>
/// The ring uses a constant <c>BorderThickness="1"</c> in XAML so layout never
/// shifts as the highlight toggles — only the brush colors change.
/// </para>
///
/// <para>
/// Brushes are lazily constructed on first access, which always happens from a
/// XAML binding evaluation on the UI thread, so the DependencyObject thread
/// affinity rule is respected.
/// </para>
/// </summary>
public static class LinkHighlightHelper
{
    private static SolidColorBrush? _peerFill;
    private static SolidColorBrush? _pathFill;
    private static SolidColorBrush? _peerRing;
    private static SolidColorBrush? _newFill;
    private static SolidColorBrush? _newRing;
    private static SolidColorBrush? _transparent;

    public static Brush GetBackgroundBrush(LinkHighlightKind kind) => kind switch
    {
        LinkHighlightKind.Peer    => _peerFill ??= new SolidColorBrush(Color.FromArgb(0x55, 0xFF, 0xB3, 0x00)),
        LinkHighlightKind.Path    => _pathFill ??= new SolidColorBrush(Color.FromArgb(0x28, 0xFF, 0xB3, 0x00)),
        // NewNode — soft emerald tint, distinct from the amber peer/path highlight.
        LinkHighlightKind.NewNode => _newFill ??= new SolidColorBrush(Color.FromArgb(0x55, 0x2E, 0xE6, 0x6E)),
        _                         => _transparent ??= new SolidColorBrush(Colors.Transparent),
    };

    public static Brush GetBorderBrush(LinkHighlightKind kind) => kind switch
    {
        LinkHighlightKind.Peer    => _peerRing ??= new SolidColorBrush(Color.FromArgb(0xFF, 0xFF, 0xB3, 0x00)),
        // NewNode — bright emerald ring so freshly-added nodes pop as they scroll into view.
        LinkHighlightKind.NewNode => _newRing ??= new SolidColorBrush(Color.FromArgb(0xFF, 0x2E, 0xE6, 0x6E)),
        _                         => _transparent ??= new SolidColorBrush(Colors.Transparent),
    };
}
