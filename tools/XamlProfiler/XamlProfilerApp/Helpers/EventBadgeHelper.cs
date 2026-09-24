using Microsoft.UI;
using Microsoft.UI.Xaml.Media;
using Windows.UI;

namespace XamlProfiler.Helpers;

/// <summary>
/// Static helper consumed by x:Bind function bindings to render a small
/// colored badge next to each tree node showing the ETW event that first
/// created the node (its provenance).
///
/// <para>
/// Same function-binding pattern as <see cref="LinkHighlightHelper"/> — used
/// because <c>MainWindow</c> derives from <c>Window</c> (not FrameworkElement),
/// which breaks x:Bind converter codegen.
/// </para>
///
/// <para>
/// Color philosophy: each event family gets a distinct hue so that, at a
/// glance, a tree dense with badges shows you the "source mix" — mostly
/// green = bulk-added children, lots of purple = popup-heavy, lots of orange
/// = composition-driven, etc.
/// </para>
/// </summary>
public static class EventBadgeHelper
{
    private static SolidColorBrush? _addBrush;       // ChildAdded
    private static SolidColorBrush? _insertBrush;    // ChildInserted
    private static SolidColorBrush? _enterBrush;     // ElementEnteredTree
    private static SolidColorBrush? _popupBrush;     // PopupOpened
    private static SolidColorBrush? _contentBrush;   // ContentChanged
    private static SolidColorBrush? _compBrush;      // CompPeerLinked, CompNode*
    private static SolidColorBrush? _removeBrush;    // *Removed, *Left, *Closed, *Unlinked
    private static SolidColorBrush? _unknownBrush;
    private static SolidColorBrush? _transparent;

    /// <summary>Background brush for the badge. Transparent for nodes without
    /// provenance so they read as plain rows.</summary>
    public static Brush GetBrush(string? eventName) => eventName switch
    {
        "ChildAdded"            => _addBrush     ??= new SolidColorBrush(Color.FromArgb(0xFF, 0x4C, 0xAF, 0x50)),  // green
        "ChildInserted"         => _insertBrush  ??= new SolidColorBrush(Color.FromArgb(0xFF, 0x8B, 0xC3, 0x4A)),  // light green
        "ElementEnteredTree"    => _enterBrush   ??= new SolidColorBrush(Color.FromArgb(0xFF, 0x00, 0xBC, 0xD4)),  // cyan
        "PopupOpened"           => _popupBrush   ??= new SolidColorBrush(Color.FromArgb(0xFF, 0x9C, 0x27, 0xB0)),  // purple
        "PopupClosed"           => _removeBrush  ??= new SolidColorBrush(Color.FromArgb(0xFF, 0x75, 0x75, 0x75)),  // gray
        "ContentChanged"        => _contentBrush ??= new SolidColorBrush(Color.FromArgb(0xFF, 0x21, 0x96, 0xF3)),  // blue
        "CompPeerLinked"        => _compBrush    ??= new SolidColorBrush(Color.FromArgb(0xFF, 0xFF, 0x98, 0x00)),  // orange
        "CompNodeChildInserted" => _compBrush    ??= new SolidColorBrush(Color.FromArgb(0xFF, 0xFF, 0x98, 0x00)),
        "ChildRemoved"          => _removeBrush  ??= new SolidColorBrush(Color.FromArgb(0xFF, 0x75, 0x75, 0x75)),
        "ElementLeftTree"       => _removeBrush  ??= new SolidColorBrush(Color.FromArgb(0xFF, 0x75, 0x75, 0x75)),
        "CompPeerUnlinked"      => _removeBrush  ??= new SolidColorBrush(Color.FromArgb(0xFF, 0x75, 0x75, 0x75)),
        "CompNodeChildRemoved"  => _removeBrush  ??= new SolidColorBrush(Color.FromArgb(0xFF, 0x75, 0x75, 0x75)),
        null or ""              => _transparent  ??= new SolidColorBrush(Colors.Transparent),
        _                       => _unknownBrush ??= new SolidColorBrush(Color.FromArgb(0xFF, 0x60, 0x60, 0x60)),
    };
}
