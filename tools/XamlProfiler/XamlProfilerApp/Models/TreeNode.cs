using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace XamlProfiler.Models;

/// <summary>
/// Represents a node in one of the three profiler trees (Logical, Visual, Composition).
/// Each node carries the raw pointer-based ID from ETW events and optional cross-tree linkage.
/// </summary>
public class TreeNode : INotifyPropertyChanged
{
    /// <summary>Hard cap on per-node event history; oldest entries are evicted.</summary>
    private const int MaxHistoryEntries = 30;

    private string _displayName = string.Empty;
    private bool _isExpanded = true;
    private bool _isHighlighted;
    private LinkHighlightKind _linkHighlight = LinkHighlightKind.None;
    private string? _createdByEvent;
    private string? _label;
    private string _fallbackPrefix = string.Empty;
    private string _suffix = string.Empty;

    public TreeNode(ulong id, string displayName, TreeNodeKind kind)
    {
        Id = id;
        _displayName = displayName;
        Kind = kind;
        // Seed a sensible fallback prefix per tree so the address can be toggled off even
        // for nodes that never receive an explicit UpdateDisplayName call.
        _fallbackPrefix = kind switch
        {
            TreeNodeKind.Logical => "Element",
            TreeNodeKind.Visual => "Visual",
            TreeNodeKind.Composition => "CompNode",
            TreeNodeKind.WucVisual => "WucVisual",
            _ => string.Empty,
        };

        // A node is often added to the tree BEFORE its children stream in (e.g. the visual
        // tree's XamlIslandRootCollection root arrives empty, then gets AddChild /
        // ElementEnteredTree). Its container's Loaded fires while it has 0 children, so WinUI
        // drops the IsExpanded=true (an item with no children can't expand). Because the
        // IsExpanded binding is OneWay and the model value never changes, nothing re-pushes
        // expansion once children finally arrive. So when Children transitions empty -> non-empty
        // while we're expanded, re-raise IsExpanded to make the OneWay binding re-assert it onto
        // the now-child-bearing container.
        Children.CollectionChanged += OnChildrenChanged;
    }

    private void OnChildrenChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        if (e.Action == NotifyCollectionChangedAction.Add && Children.Count == e.NewItems?.Count && _isExpanded)
        {
            // First child(ren) added to a previously-empty, expanded node: re-push IsExpanded.
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(IsExpanded)));
        }
    }

    /// <summary>
    /// Global toggle controlling whether the raw memory address (<c>0x{Id}</c>) is shown in
    /// every node's <see cref="DisplayName"/>. Flip it and call
    /// <c>ProfilerTreeStore.RefreshAllDisplayNames()</c> to recompute existing rows.
    /// </summary>
    public static bool IncludeMemoryAddress { get; set; } = true;

    /// <summary>
    /// Composition-tree-only toggle: whether a comp node row shows the responsible
    /// UIElement it is linked to (the <c>→ Element 0x...</c> annotation). Flip it and call
    /// <c>ProfilerTreeStore.RefreshAllDisplayNames()</c> to recompute existing rows.
    /// </summary>
    public static bool ShowResponsibleElementAddress { get; set; } = true;

    /// <summary>
    /// For Composition nodes, the id of the responsible UIElement this comp node is linked to
    /// (set by <c>LinkCompPeer</c>). <c>null</c> when the comp node is unlinked or non-composition.
    /// </summary>
    public ulong? ResponsibleElementId { get; set; }

    /// <summary>Raw pointer ID from ETW (e.g., CUIElement* or HWCompTreeNode*).</summary>
    public ulong Id { get; }

    /// <summary>
    /// The DXaml-peer <c>InstanceHandle</c> for the element this node represents, emitted by the
    /// producer (<c>XamlProfilerGetPeerHandle</c>). This is the identity XAML Diagnostics / the
    /// WinUISnoop tap use to highlight the element live in the target app — distinct from
    /// <see cref="Id"/> (the core <c>CUIElement*</c> that stitches the trees together).
    /// <c>0</c> means unknown / no peer. Meaningful for Logical/Visual element nodes and for a
    /// Composition node routed through its owning UIElement.
    /// </summary>
    public ulong PeerHandle { get; set; }

    /// <summary>Which tree this node belongs to.</summary>
    public TreeNodeKind Kind { get; }

    /// <summary>
    /// Friendly label emitted by the producer (<c>GetDebugLabel()</c> in the runtime):
    /// the developer-set <c>x:Name</c> if any, otherwise the runtime class name
    /// (e.g. <c>"OkButton"</c>, <c>"Button"</c>, <c>"Grid"</c>).
    /// <para>
    /// Set on the first event that carries a non-empty label for this node and
    /// then frozen — subsequent events that don't carry a label (e.g. removal
    /// events) do not erase it. See <see cref="SetLabelIfEmpty"/>.
    /// </para>
    /// </summary>
    public string? Label
    {
        get => _label;
        private set => SetProperty(ref _label, value);
    }

    /// <summary>
    /// Stamp the label once. The "first non-empty wins" rule matters because a
    /// node can first appear via a label-carrying event (ChildAdded, EnteredTree)
    /// OR via a label-less one (CompPeerUnlinked for an element we never saw
    /// added). We don't want a later label-less event to wipe a real label.
    /// </summary>
    public bool SetLabelIfEmpty(string? label)
    {
        if (string.IsNullOrEmpty(label)) return false;
        if (!string.IsNullOrEmpty(_label)) return false;
        Label = label;
        return true;
    }

    /// <summary>
    /// Recompute <see cref="DisplayName"/> from the current <see cref="Label"/>,
    /// <see cref="Id"/>, and an optional state suffix (e.g. <c>"[Live]"</c>).
    /// When a label is set the row reads as <c>"Button (0x1D2F4A03B40)"</c>;
    /// otherwise it falls back to the legacy <c>"{prefix} 0x..."</c> format. The
    /// <c>0x...</c> address is omitted entirely when <see cref="IncludeMemoryAddress"/>
    /// is <c>false</c>.
    /// </summary>
    public void UpdateDisplayName(string fallbackPrefix, string suffix = "")
    {
        _fallbackPrefix = fallbackPrefix;
        _suffix = suffix;
        DisplayName = ComposeDisplayName();
    }

    /// <summary>
    /// Recompute <see cref="DisplayName"/> from the last prefix/suffix, honoring the current
    /// <see cref="IncludeMemoryAddress"/> toggle. Cheap; called on every node when the toggle
    /// flips.
    /// </summary>
    public void RefreshDisplayName() => DisplayName = ComposeDisplayName();

    private string ComposeDisplayName()
    {
        string core;
        if (string.IsNullOrEmpty(_label))
            core = IncludeMemoryAddress ? $"{_fallbackPrefix} 0x{Id:X}" : _fallbackPrefix;
        else
            core = IncludeMemoryAddress ? $"{_label} (0x{Id:X})" : _label;

        // Composition rows annotate the responsible UIElement they are linked to.
        if (Kind == TreeNodeKind.Composition && ResponsibleElementId is ulong respId
            && ShowResponsibleElementAddress)
        {
            core += $" → Element 0x{respId:X}";
        }

        return string.IsNullOrEmpty(_suffix) ? core : $"{core} {_suffix}";
    }

    /// <summary>Human-readable label shown in the TreeView.</summary>
    public string DisplayName
    {
        get => _displayName;
        set => SetProperty(ref _displayName, value);
    }

    /// <summary>Whether this node is expanded in the TreeView.</summary>
    /// <remarks>
    /// Defaults to <c>true</c> so all four trees show fully expanded by default
    /// (new nodes appear expanded as events stream in). Collapsing cascades to
    /// descendants: when the user collapses a node, every child's
    /// <see cref="IsExpanded"/> is also reset to <c>false</c>. Next re-expansion
    /// shows just the immediate children, giving a "fresh" view instead of
    /// restoring previous deep expansions.
    /// </remarks>
    public bool IsExpanded
    {
        get => _isExpanded;
        set
        {
            if (!SetProperty(ref _isExpanded, value)) return;
            if (!value)
            {
                foreach (var child in Children)
                {
                    child.IsExpanded = false;
                }
            }
        }
    }

    /// <summary>Whether this node is highlighted (e.g., just changed).</summary>
    public bool IsHighlighted
    {
        get => _isHighlighted;
        set => SetProperty(ref _isHighlighted, value);
    }

    /// <summary>
    /// Cross-tree selection highlight state:
    ///   - Peer: this node is the linked peer of the user's current selection.
    ///   - Path: this node is an ancestor on the way down to the linked peer.
    ///   - None: not part of any active highlight.
    /// Driven by <see cref="ProfilerTreeStore.HighlightLinkedFor"/> when the user
    /// clicks a node in any tree.
    /// </summary>
    public LinkHighlightKind LinkHighlight
    {
        get => _linkHighlight;
        set => SetProperty(ref _linkHighlight, value);
    }

    /// <summary>
    /// The name of the ETW event that first caused this node to exist in its
    /// tree (provenance). Set once on the node's first sighting and never
    /// overwritten — subsequent events update <see cref="EventHistory"/> only.
    /// Examples: <c>"ChildAdded"</c>, <c>"ElementEnteredTree"</c>,
    /// <c>"PopupOpened"</c>, <c>"CompPeerLinked"</c>.
    /// Rendered as a colored badge next to <see cref="DisplayName"/>.
    /// </summary>
    public string? CreatedByEvent
    {
        get => _createdByEvent;
        set => SetProperty(ref _createdByEvent, value);
    }

    /// <summary>
    /// Time-ordered ring of ETW events that have touched this node, capped at
    /// <see cref="MaxHistoryEntries"/> to keep memory bounded. Rendered in the
    /// node detail pane when the user inspects a node.
    /// </summary>
    public ObservableCollection<NodeEventLogEntry> EventHistory { get; } = new();

    /// <summary>
    /// Append a new event to this node's history. The first call also stamps
    /// <see cref="CreatedByEvent"/> so the badge persists for the node's lifetime.
    /// When the cap is reached, the oldest entry is dropped (FIFO).
    /// </summary>
    public void RecordEvent(string eventName, string summary = "")
    {
        if (string.IsNullOrEmpty(_createdByEvent))
        {
            CreatedByEvent = eventName;
        }

        if (EventHistory.Count >= MaxHistoryEntries)
        {
            EventHistory.RemoveAt(0);
        }

        EventHistory.Add(new NodeEventLogEntry(DateTime.Now, eventName, summary));
    }

    /// <summary>Cross-tree link: ID of the corresponding node in the Logical tree.</summary>
    public ulong? LinkedLogicalNodeId { get; set; }

    /// <summary>Cross-tree link: ID of the corresponding node in the Visual tree.</summary>
    public ulong? LinkedVisualNodeId { get; set; }

    /// <summary>Cross-tree link: ID of the corresponding node in the Composition tree.</summary>
    public ulong? LinkedCompNodeId { get; set; }

    /// <summary>
    /// For a <see cref="TreeNodeKind.WucVisual"/> node: the parsed WUC IVisual
    /// property payload (offset/size/opacity/transform/brush/clip/shadow...).
    /// Null for the other tree kinds.
    /// </summary>
    public WucVisualProperties? VisualProperties { get; set; }

    /// <summary>
    /// For a WucVisual node: the concrete visual class name (SpriteVisual,
    /// ContainerVisual, RedirectVisual, ...). Null otherwise.
    /// </summary>
    public string? VisualType { get; set; }

    /// <summary>
    /// For a WucVisual root node: the island/target id the root visual is set
    /// onto (used to resolve <c>WucVisualRootCleared</c>). Null otherwise.
    /// </summary>
    public ulong? IslandTargetId { get; set; }

    /// <summary>Parent node ID (0 if root).</summary>
    public ulong ParentId { get; set; }

    /// <summary>Child nodes.</summary>
    public ObservableCollection<TreeNode> Children { get; } = new();

    public event PropertyChangedEventHandler? PropertyChanged;

    protected bool SetProperty<T>(ref T storage, T value, [CallerMemberName] string? propertyName = null)
    {
        if (EqualityComparer<T>.Default.Equals(storage, value)) return false;
        storage = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        return true;
    }
}

public enum TreeNodeKind
{
    Logical,
    Visual,
    Composition,
    WucVisual
}

/// <summary>
/// Cross-tree highlight state. Used to glow the linked peer (and the ancestor
/// chain leading to it) in the other two trees when the user selects a node.
/// </summary>
public enum LinkHighlightKind
{
    /// <summary>Not part of any active cross-tree highlight.</summary>
    None,

    /// <summary>This node is an ancestor of the linked peer — dim glow / breadcrumb.</summary>
    Path,

    /// <summary>This node IS the linked peer — bright glow.</summary>
    Peer,

    /// <summary>This node was just newly added to its tree — transient spotlight glow.</summary>
    NewNode
}

/// <summary>
/// One entry in a <see cref="TreeNode.EventHistory"/> — a snapshot of "this ETW
/// event touched this node at this time, with this payload summary".
/// </summary>
public sealed class NodeEventLogEntry
{
    public NodeEventLogEntry(DateTime timestamp, string eventName, string summary)
    {
        Timestamp = timestamp;
        EventName = eventName;
        Summary = summary;
    }

    public DateTime Timestamp { get; }
    public string EventName { get; }
    public string Summary { get; }

    /// <summary>Formatted for the detail pane: <c>HH:mm:ss.ffffff</c> (microsecond precision).</summary>
    public string TimeText => Timestamp.ToString("HH:mm:ss.ffffff");
}
