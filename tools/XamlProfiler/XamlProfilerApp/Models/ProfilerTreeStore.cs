using System.Collections.ObjectModel;

namespace XamlProfiler.Models;

/// <summary>
/// Holds the three profiler trees and provides lookup/mutation APIs.
/// All mutations must happen on the UI thread (DispatcherQueue).
/// </summary>
public class ProfilerTreeStore
{
    // Flat lookup tables: pointer ID → node
    private readonly Dictionary<ulong, TreeNode> _logicalNodes = new();
    private readonly Dictionary<ulong, TreeNode> _visualNodes = new();
    private readonly Dictionary<ulong, TreeNode> _compNodes = new();

    // WUC (Windows.UI.Composition) IVisual tree: IVisual* → node.
    private readonly Dictionary<ulong, TreeNode> _wucNodes = new();

    // Reverse index: owning HWCompNode* → the WUC visuals it owns. Lets a
    // comp-node selection light up every WUC visual that belongs to it, and
    // supports defensive cascade cleanup when a comp node disappears.
    private readonly Dictionary<ulong, List<TreeNode>> _wucByOwnerComp = new();

    // Island/target id → root WUC visual id. Used to resolve WucVisualRootCleared,
    // which only carries the target id.
    private readonly Dictionary<ulong, ulong> _wucRootByTarget = new();

    // Reverse index: UIElement pointer → its linked composition peer.
    // Maintained by LinkCompPeer/UnlinkCompPeer. Lets GetOrCreateLogicalNode
    // and GetOrCreateVisualNode back-fill the LinkedCompNodeId when the
    // logical/visual node is created AFTER the comp peer was linked.
    private readonly Dictionary<ulong, TreeNode> _compByElement = new();

    // Reverse index: PeerHandle (XAML-diagnostics InstanceHandle) → the best tree node
    // to select for it (Logical preferred, else Visual). Populated by SetPeerHandle.
    // Used by pick mode to map an app-side clicked element back to a profiler node.
    private readonly Dictionary<ulong, TreeNode> _nodeByPeerHandle = new();

    // Pick-mode (app→profiler) deferred IVisual highlight. When the user picks an element the
    // tap resolves its GetElementVisual (hand-in) visual id. That visual is frequently created
    // lazily by the GetElementVisual call itself, so the producer has NOT emitted it over ETW
    // yet — its node isn't in _wucNodes at pick time (which is why FindWucNode missed even
    // though the live-tree dump shows it). We remember the id here and apply the subtree glow
    // the moment the producer emits that node (the next render frame). Reset on every new
    // selection via ClearLinkHighlights so a stale pending can't fire after a later pick.
    private ulong _pendingVisualHighlight;

    // Pick-mode (element by PeerHandle): when an app→profiler pick arrives before the element's
    // DXaml-peer handle has been stamped onto its node (the producer emitted the element's
    // tree events with peerHandle==0 because the peer didn't exist yet, and the tap only
    // force-created it on pick), we can't resolve the node immediately. Arm the handle here and
    // resolve it the instant SetPeerHandle stamps a matching node. Reset on every new selection.
    private ulong _pendingPickHandle;

    /// <summary>
    /// Raised when a deferred element pick (see <c>_pendingPickHandle</c>) finally resolves —
    /// i.e. the picked element's PeerHandle arrived over ETW just after the pick. Lets the view
    /// glow/expand/scroll to the node exactly as it would have on an immediate hit.
    /// </summary>
    public event Action<TreeNode>? PendingPickResolved;

    /// <summary>
    /// Arm a deferred element pick for <paramref name="peerHandle"/>. Call this when an
    /// app→profiler pick can't yet be resolved to a node (handle not stamped). It will fire
    /// <see cref="PendingPickResolved"/> the moment the handle is stamped onto a node.
    /// </summary>
    public void ArmPendingPick(ulong peerHandle) => _pendingPickHandle = peerHandle;

    /// <summary>
    /// Raised when a deferred visual-subtree highlight (see <c>_pendingVisualHighlight</c>) is
    /// finally applied — i.e. the picked IVisual arrived over ETW after the pick. Lets the view
    /// select/scroll to the node that just appeared.
    /// </summary>
    public event Action<TreeNode>? VisualSubtreeHighlightApplied;

    /// <summary>
    /// Raised at the end of a highlight operation (Ctrl+Click linkage, app→profiler element
    /// pick, or IVisual subtree pick) with the per-tree "anchor" nodes the view should reveal:
    /// expand the path to them AND scroll them into view. There is at most one anchor per tree
    /// kind (the deepest/most-specific glowed peer). The view (<c>MainWindow</c>) is responsible
    /// for the actual realize+scroll, because expansion alone does not bring a deeply virtualized
    /// node on-screen — without a scroll the row stays collapsed/off-screen until touched.
    /// </summary>
    /// The <c>bool</c> is <c>passive</c>: <c>true</c> when the reveal was triggered by nodes
    /// streaming in (the "spotlight new nodes" auto-scroll), <c>false</c> when the user explicitly
    /// asked to jump to a node (pick / Ctrl+Click linkage). The view uses this to avoid yanking a
    /// tree the user has manually scrolled away from during a passive reveal.
    public event Action<IReadOnlyList<TreeNode>, bool>? RevealRequested;

    /// <summary>
    /// Raised once for every <see cref="TreeNode"/> the moment it is first created in any of
    /// the four trees. The view buffers these to spotlight (glow + auto-scroll) freshly-added
    /// nodes, while skipping the initial connect-time flood.
    /// </summary>
    public event Action<TreeNode>? NodeAdded;

    // Per-operation scratch list of nodes to reveal/scroll-to (one per tree). Cleared at the
    // start of each top-level highlight operation and snapshotted into RevealRequested.
    private readonly List<TreeNode> _revealAnchors = new();

    // Infrastructure class names — framework-created elements that are NOT
    // template children (GetTemplatedParent()==null) but also NOT developer-
    // authored. These are excluded from the logical tree.
    private static readonly HashSet<string> _infraLabels = new(StringComparer.OrdinalIgnoreCase)
    {
        "XamlIslandRoot",
        "XamlIslandRootCollection",
        "RootScrollViewer",
        "ScrollContentPresenter",
        "PopupRoot",
        "ConnectedAnimationRoot",
        "TransitionRoot",
        "PrintRoot",
        "ClientAreaPresenter",
        "WindowChrome",
        "FullWindowMediaRoot",
        "LayoutTransitionElement",
    };

    /// <summary>
    /// Returns true if the label matches a known XAML framework infrastructure
    /// element that should be excluded from the logical (developer) tree.
    /// </summary>
    public static bool IsInfrastructureLabel(string? label)
    {
        if (string.IsNullOrEmpty(label)) return false;
        return _infraLabels.Contains(label);
    }

    /// <summary>
    /// Walks up the visual tree from <paramref name="startParentId"/> to find
    /// a suitable logical parent.
    /// 1. If an ancestor is already in the logical tree → return it
    /// 2. Otherwise, the topmost non-infra visual ancestor (with a known label)
    ///    is promoted into the logical tree and returned.
    /// 3. If every ancestor is infra (or label-less) → returns 0.
    /// </summary>
    public ulong FindNearestLogicalAncestor(ulong startParentId)
    {
        if (startParentId == 0) return 0;

        var visited = new HashSet<ulong>();
        var currentId = startParentId;
        ulong lastNonInfraId = 0;
        TreeNode? lastNonInfraVisual = null;

        while (currentId != 0 && visited.Add(currentId))
        {
            if (_logicalNodes.ContainsKey(currentId))
                return currentId;

            var vNode = FindVisualNode(currentId);
            if (vNode == null) break;

            // Only treat a node as a candidate non-infra ancestor when we
            // ACTUALLY know its class — a label-less node could be infra we
            // simply haven't seen labelled yet, and promoting it would create
            // a phantom logical root like "Element 0xADDR".
            if (!string.IsNullOrEmpty(vNode.Label) && !IsInfrastructureLabel(vNode.Label))
            {
                lastNonInfraId = currentId;
                lastNonInfraVisual = vNode;
            }

            currentId = vNode.ParentId;
        }

        // Promote the last verified non-infra visual ancestor
        if (lastNonInfraId != 0 && lastNonInfraVisual != null)
        {
            var node = GetOrCreateLogicalNode(lastNonInfraId, lastNonInfraVisual.Label);
            if (node.ParentId == 0 && !LogicalRoots.Contains(node))
            {
                LogicalRoots.Add(node);
            }
            return lastNonInfraId;
        }

        return 0;
    }

    /// <summary>Root-level nodes for the Logical tree (TreeView binding source).</summary>
    public ObservableCollection<TreeNode> LogicalRoots { get; } = new();

    /// <summary>Root-level nodes for the Visual tree (TreeView binding source).</summary>
    public ObservableCollection<TreeNode> VisualRoots { get; } = new();

    /// <summary>Root-level nodes for the Composition tree (TreeView binding source).</summary>
    public ObservableCollection<TreeNode> CompositionRoots { get; } = new();

    /// <summary>Root-level nodes for the WUC IVisual tree (TreeView binding source).</summary>
    public ObservableCollection<TreeNode> WucVisualRoots { get; } = new();

    /// <summary>Running count of events processed.</summary>
    public int EventCount { get; set; }

    /// <summary>
    /// Recompute every node's <see cref="TreeNode.DisplayName"/> across all four trees.
    /// Call after flipping <see cref="TreeNode.IncludeMemoryAddress"/> so existing rows
    /// pick up (or drop) the <c>0x...</c> memory address.
    /// </summary>
    public void RefreshAllDisplayNames()
    {
        foreach (var n in _logicalNodes.Values) n.RefreshDisplayName();
        foreach (var n in _visualNodes.Values) n.RefreshDisplayName();
        foreach (var n in _compNodes.Values) n.RefreshDisplayName();
        foreach (var n in _wucNodes.Values) n.RefreshDisplayName();
    }

    // =====================================================================
    // Logical Tree
    // =====================================================================

    public TreeNode GetOrCreateLogicalNode(ulong id, string? label = null)
    {
        if (!_logicalNodes.TryGetValue(id, out var node))
        {
            node = new TreeNode(id, $"Element 0x{id:X}", TreeNodeKind.Logical);
            _logicalNodes[id] = node;
            NodeAdded?.Invoke(node);
        }

        // First non-empty label wins; refresh DisplayName if the label was just set
        // OR if the node still shows the placeholder (no label has ever arrived).
        if (node.SetLabelIfEmpty(label) || string.IsNullOrEmpty(node.Label))
        {
            node.UpdateDisplayName("Element");
        }

        // Back-fill cross-tree links from peers that already exist. This is
        // the symmetric half of the linking done in GetOrCreateVisualNode and
        // LinkCompPeer — together they guarantee links are established the
        // moment both sides exist, regardless of event order.
        if (_visualNodes.TryGetValue(id, out var v))
        {
            node.LinkedVisualNodeId = id;
            v.LinkedLogicalNodeId = id;
            // Logical and visual share x:Name — propagate either direction.
            if (v.SetLabelIfEmpty(node.Label)) v.UpdateDisplayName("Visual");
            if (node.SetLabelIfEmpty(v.Label)) node.UpdateDisplayName("Element");
        }
        if (_compByElement.TryGetValue(id, out var c))
        {
            node.LinkedCompNodeId = c.Id;
        }

        return node;
    }

    public TreeNode? FindLogicalNode(ulong id) =>
        _logicalNodes.TryGetValue(id, out var n) ? n : null;

    public void AddLogicalChild(ulong parentId, ulong childId, string? childLabel = null)
    {
        var parent = GetOrCreateLogicalNode(parentId);
        var child = GetOrCreateLogicalNode(childId, childLabel);

        RemoveFromCurrentParent(child, LogicalRoots);
        child.ParentId = parentId;
        if (!parent.Children.Contains(child))
            parent.Children.Add(child);
        EnsureRooted(parent, LogicalRoots);
    }

    public void RemoveLogicalChild(ulong parentId, ulong childId)
    {
        var parent = FindLogicalNode(parentId);
        var child = FindLogicalNode(childId);
        if (parent != null && child != null)
        {
            parent.Children.Remove(child);
            child.ParentId = 0;
        }
    }

    /// <summary>
    /// Adds a child to the visual tree only (no logical tree entry).
    /// Used for template-generated elements (IsTemplateChild == true).
    /// </summary>
    public void AddVisualChild(ulong parentId, ulong childId, string? childLabel = null)
    {
        var parent = GetOrCreateVisualNode(parentId);
        var child = GetOrCreateVisualNode(childId, childLabel);

        RemoveFromCurrentParent(child, VisualRoots);
        child.ParentId = parentId;
        if (!parent.Children.Contains(child))
            parent.Children.Add(child);
        EnsureRooted(parent, VisualRoots);
    }

    /// <summary>
    /// Removes a child from the visual tree.
    /// </summary>
    public void RemoveVisualChild(ulong parentId, ulong childId)
    {
        var parent = FindVisualNode(parentId);
        var child = FindVisualNode(childId);
        if (parent != null && child != null)
        {
            parent.Children.Remove(child);
            child.ParentId = 0;
        }
    }

    /// <summary>
    /// Marks an element as entering the visual tree only (no logical tree entry).
    /// Used for template-generated elements (IsTemplateChild == true).
    /// </summary>
    public void MarkVisualEnter(ulong elementId, ulong parentId, bool isLive, string? label = null)
    {
        var vNode = GetOrCreateVisualNode(elementId, label);
        vNode.UpdateDisplayName("Visual");
        vNode.IsHighlighted = true;

        if (parentId != 0)
        {
            var vParent = GetOrCreateVisualNode(parentId);
            if (!vParent.Children.Contains(vNode))
            {
                RemoveFromCurrentParent(vNode, VisualRoots);
                vNode.ParentId = parentId;
                vParent.Children.Add(vNode);
                EnsureRooted(vParent, VisualRoots);
            }
        }
        else
        {
            EnsureRooted(vNode, VisualRoots);
        }
    }

    /// <summary>
    /// Marks an element as leaving the visual tree only.
    /// Used for template-generated elements (IsTemplateChild == true).
    /// </summary>
    public void MarkVisualLeave(ulong elementId, ulong parentId)
    {
        var vNode = FindVisualNode(elementId);
        if (vNode != null)
        {
            if (vNode.ParentId != 0)
            {
                FindVisualNode(vNode.ParentId)?.Children.Remove(vNode);
            }
            VisualRoots.Remove(vNode);
            vNode.ParentId = 0;
            vNode.LinkedLogicalNodeId = null;
            vNode.LinkedCompNodeId = null;
        }
    }

    public void MarkLogicalEnter(ulong elementId, ulong parentId, bool isLive, string? label = null)
    {
        var node = GetOrCreateLogicalNode(elementId, label);
        node.UpdateDisplayName("Element");
        node.IsHighlighted = true;

        if (parentId != 0)
        {
            var parent = GetOrCreateLogicalNode(parentId);
            if (!parent.Children.Contains(node))
            {
                RemoveFromCurrentParent(node, LogicalRoots);
                node.ParentId = parentId;
                parent.Children.Add(node);
                EnsureRooted(parent, LogicalRoots);
            }
        }
        else
        {
            EnsureRooted(node, LogicalRoots);
        }
    }

    public void MarkLogicalLeave(ulong elementId, ulong parentId, bool isLive)
    {
        // Logical side only — visual leave is handled separately by MarkVisualLeave.
        var node = FindLogicalNode(elementId);
        if (node != null)
        {
            if (node.ParentId != 0)
            {
                FindLogicalNode(node.ParentId)?.Children.Remove(node);
            }
            LogicalRoots.Remove(node);
            node.ParentId = 0;

            node.LinkedVisualNodeId = null;
            node.LinkedCompNodeId = null;
        }
    }

    // =====================================================================
    // Visual Tree (mirrors logical tree for UIElements, plus popups)
    // =====================================================================

    public TreeNode GetOrCreateVisualNode(ulong id, string? label = null)
    {
        if (!_visualNodes.TryGetValue(id, out var node))
        {
            node = new TreeNode(id, $"Visual 0x{id:X}", TreeNodeKind.Visual);
            _visualNodes[id] = node;
            NodeAdded?.Invoke(node);
        }

        if (node.SetLabelIfEmpty(label) || string.IsNullOrEmpty(node.Label))
        {
            node.UpdateDisplayName("Visual");
        }

        // Logical and Visual share the same ID space (both keyed by CUIElement*),
        // so we can always back-link to the logical peer if one exists. The
        // Composition tree uses a DIFFERENT pointer (HWCompTreeNode*) — find the
        // comp peer via the _compByElement reverse index instead of by ID.
        if (_logicalNodes.TryGetValue(id, out var logical))
        {
            logical.LinkedVisualNodeId = id;
            node.LinkedLogicalNodeId = id;
            if (logical.SetLabelIfEmpty(node.Label)) logical.UpdateDisplayName("Element");
            if (node.SetLabelIfEmpty(logical.Label)) node.UpdateDisplayName("Visual");
        }
        if (_compByElement.TryGetValue(id, out var c))
        {
            node.LinkedCompNodeId = c.Id;
        }

        return node;
    }

    public TreeNode? FindVisualNode(ulong id) =>
        _visualNodes.TryGetValue(id, out var n) ? n : null;

    /// <summary>
    /// Stamp the DXaml-peer <c>InstanceHandle</c> (from the producer) onto every node that
    /// represents element <paramref name="id"/> — its logical node, its visual node, and (if
    /// already linked) the composition node it owns — so a Ctrl+Click in any of those trees can
    /// resolve the handle and ask a tap to highlight the live element in the target app.
    /// "First non-zero wins": a later 0 (e.g. an event fired before the peer existed) never erases
    /// a handle we already captured.
    /// </summary>
    public void SetPeerHandle(ulong id, ulong peerHandle)
    {
        if (id == 0 || peerHandle == 0) return;
        if (_logicalNodes.TryGetValue(id, out var l) && l.PeerHandle == 0) l.PeerHandle = peerHandle;
        if (_visualNodes.TryGetValue(id, out var v) && v.PeerHandle == 0) v.PeerHandle = peerHandle;
        if (_compByElement.TryGetValue(id, out var c) && c.PeerHandle == 0) c.PeerHandle = peerHandle;

        // Maintain the PeerHandle -> node index for pick mode. Prefer the Logical node
        // (the element the user thinks of), falling back to the Visual node.
        var preferred = (l ?? v) ?? c;
        if (preferred != null &&
            (!_nodeByPeerHandle.TryGetValue(peerHandle, out var existing) ||
             existing.Kind != TreeNodeKind.Logical))
        {
            _nodeByPeerHandle[peerHandle] = preferred;
        }

        // Resolve a deferred app→profiler pick that was waiting for this handle to be stamped
        // (first-pick race: the tap force-created the DXaml peer, the re-stamp landed now).
        if (_pendingPickHandle != 0 && peerHandle == _pendingPickHandle && preferred != null)
        {
            _pendingPickHandle = 0;
            PendingPickResolved?.Invoke(preferred);
        }
    }

    /// <summary>
    /// Map a live element's PeerHandle (from app-side pick mode / XAML Diagnostics) back
    /// to the tree node to select. Returns null if no node carries that handle yet.
    /// </summary>
    public TreeNode? FindByPeerHandle(ulong peerHandle)
    {
        if (peerHandle == 0) return null;
        if (_nodeByPeerHandle.TryGetValue(peerHandle, out var node))
            return node;
        // Fallback linear scan (handle set after the index entry, or on a node kind we
        // didn't index): prefer Logical, then Visual, then anything.
        TreeNode? best = null;
        foreach (var n in _logicalNodes.Values)
            if (n.PeerHandle == peerHandle) return n;
        foreach (var n in _visualNodes.Values)
            if (n.PeerHandle == peerHandle) { best = n; break; }
        return best;
    }


    public void AddPopupChild(ulong popupId, ulong childId, bool opened, string? popupLabel = null, string? childLabel = null)
    {
        var popup = GetOrCreateVisualNode(popupId, popupLabel);
        popup.UpdateDisplayName("Popup", opened ? "[Open]" : "[Closed]");
        popup.IsHighlighted = true;

        if (opened)
        {
            var child = GetOrCreateVisualNode(childId, childLabel);
            RemoveFromCurrentParent(child, VisualRoots);
            child.ParentId = popupId;
            if (!popup.Children.Contains(child))
                popup.Children.Add(child);
            EnsureRooted(popup, VisualRoots);
        }
        else
        {
            var child = FindVisualNode(childId);
            if (child != null)
            {
                popup.Children.Remove(child);
            }
        }
    }

    // =====================================================================
    // Composition Tree
    // =====================================================================

    public TreeNode GetOrCreateCompNode(ulong id)
    {
        if (!_compNodes.TryGetValue(id, out var node))
        {
            node = new TreeNode(id, $"CompNode 0x{id:X}", TreeNodeKind.Composition);
            _compNodes[id] = node;
            NodeAdded?.Invoke(node);
        }
        return node;
    }

    public TreeNode? FindCompNode(ulong id) =>
        _compNodes.TryGetValue(id, out var n) ? n : null;

    public void LinkCompPeer(ulong uiElementId, ulong compNodeId, ulong parentCompNodeId, string? elementLabel = null)
    {
        var compNode = GetOrCreateCompNode(compNodeId);
        compNode.LinkedVisualNodeId = uiElementId;
        compNode.LinkedLogicalNodeId = uiElementId; // UIElement: visual ID == logical ID

        // Stamp the element's label onto the comp node so the comp tree row reads
        // "Button (0x<compId>) → Element 0x<elementId>" instead of an anonymous pointer.
        // Recording ResponsibleElementId lets the "responsible element address" toggle
        // (TreeNode.ShowResponsibleElementAddress) add/drop the "→ Element 0x..." annotation.
        compNode.SetLabelIfEmpty(elementLabel);
        compNode.ResponsibleElementId = uiElementId;
        compNode.UpdateDisplayName("CompNode");
        compNode.IsHighlighted = true;

        // Maintain the reverse index so GetOrCreateLogicalNode/VisualNode can
        // back-fill LinkedCompNodeId when the logical/visual node is created
        // LATER than this comp link (event-order independence).
        _compByElement[uiElementId] = compNode;

        // Bidirectional link: tell the logical and visual peers about this comp node,
        // and propagate the label if they don't have one yet (CompPeerLinked may be
        // the first event we see for this element when capture started mid-flight).
        var lNode = FindLogicalNode(uiElementId);
        if (lNode != null)
        {
            lNode.LinkedCompNodeId = compNodeId;
            if (lNode.SetLabelIfEmpty(elementLabel)) lNode.UpdateDisplayName("Element");
        }
        var vNode = FindVisualNode(uiElementId);
        if (vNode != null)
        {
            vNode.LinkedCompNodeId = compNodeId;
            if (vNode.SetLabelIfEmpty(elementLabel)) vNode.UpdateDisplayName("Visual");
        }

        if (parentCompNodeId != 0)
        {
            var parent = GetOrCreateCompNode(parentCompNodeId);
            if (!parent.Children.Contains(compNode))
            {
                RemoveFromCurrentParent(compNode, CompositionRoots);
                compNode.ParentId = parentCompNodeId;
                parent.Children.Add(compNode);
                EnsureRooted(parent, CompositionRoots);
            }
        }
        else
        {
            EnsureRooted(compNode, CompositionRoots);
        }
    }

    public void UnlinkCompPeer(ulong uiElementId, ulong compNodeId)
    {
        var compNode = FindCompNode(compNodeId);
        if (compNode != null)
        {
            compNode.ResponsibleElementId = null;
            compNode.UpdateDisplayName("CompNode", "[Unlinked]");
            compNode.IsHighlighted = true;

            // Clear back-links on logical/visual peers so they don't keep
            // pointing at a now-stale comp node ID.
            var lNode = FindLogicalNode(uiElementId);
            if (lNode != null && lNode.LinkedCompNodeId == compNodeId) lNode.LinkedCompNodeId = null;
            var vNode = FindVisualNode(uiElementId);
            if (vNode != null && vNode.LinkedCompNodeId == compNodeId) vNode.LinkedCompNodeId = null;

            // Drop the reverse index entry if it still points at this comp node.
            // Guard against the case where a newer LinkCompPeer for the same
            // element already replaced the entry.
            if (_compByElement.TryGetValue(uiElementId, out var existing) && existing == compNode)
            {
                _compByElement.Remove(uiElementId);
            }

            // Remove from parent
            var parent = compNode.ParentId != 0 ? FindCompNode(compNode.ParentId) : null;
            parent?.Children.Remove(compNode);
            CompositionRoots.Remove(compNode);
            _compNodes.Remove(compNodeId);
        }

        // When an element's composition peer is unlinked, also detach that element's
        // node from the profiler's Visual tree. Runs unconditionally (even if we never
        // saw the comp node) so the Visual row always disappears on CompPeerUnlinked.
        // Detaching the node from its parent/roots takes its whole subtree out of view;
        // it remains in _visualNodes so a later re-link/re-enter can reattach it.
        RemoveVisualNodeFromTree(uiElementId);
    }

    /// <summary>
    /// Detach the Visual-tree node for <paramref name="elementId"/> from its parent and the
    /// roots collection, removing it (and its subtree) from the displayed Visual tree. The
    /// node object is kept in <c>_visualNodes</c> so it can be reattached if the element
    /// re-enters the tree later.
    /// </summary>
    private void RemoveVisualNodeFromTree(ulong elementId)
    {
        var vNode = FindVisualNode(elementId);
        if (vNode == null) return;

        if (vNode.ParentId != 0)
            FindVisualNode(vNode.ParentId)?.Children.Remove(vNode);
        VisualRoots.Remove(vNode);
        vNode.ParentId = 0;
    }

    public void AddCompChild(ulong parentCompNodeId, ulong childCompNodeId)
    {
        var parent = GetOrCreateCompNode(parentCompNodeId);
        var child = GetOrCreateCompNode(childCompNodeId);

        RemoveFromCurrentParent(child, CompositionRoots);
        child.ParentId = parentCompNodeId;
        if (!parent.Children.Contains(child))
            parent.Children.Add(child);
        EnsureRooted(parent, CompositionRoots);
    }

    public void RemoveCompChild(ulong parentCompNodeId, ulong childCompNodeId)
    {
        var parent = FindCompNode(parentCompNodeId);
        var child = FindCompNode(childCompNodeId);
        if (parent != null && child != null)
        {
            parent.Children.Remove(child);
            child.ParentId = 0;
        }
    }

    // =====================================================================
    // WUC (Windows.UI.Composition) IVisual tree
    //
    // The dense visual tree DCompTreeHelper dumps. Keyed by IVisual*. Every
    // insert/root event also carries OwnerCompNodeId (the HWCompNode* that owns
    // the visual) — that single field is the bridge that stitches this tree to
    // the composition tree (and through it, to the visual/logical trees).
    // OwnerCompNodeId == 0 is expected for sprites/leaves/roots; ownership is
    // then implied by the parent chain, so 0 is never treated as an error.
    // =====================================================================

    public TreeNode GetOrCreateWucNode(ulong id)
    {
        if (!_wucNodes.TryGetValue(id, out var node))
        {
            node = new TreeNode(id, $"IVisual 0x{id:X}", TreeNodeKind.WucVisual);
            _wucNodes[id] = node;
            NodeAdded?.Invoke(node);
        }
        return node;
    }

    public TreeNode? FindWucNode(ulong id) =>
        _wucNodes.TryGetValue(id, out var n) ? n : null;

    /// <summary>
    /// Connect a child IVisual under a parent IVisual (the producer fires this
    /// at InsertAtTop/Bottom/Above/Below and the comp-node spine builders). The
    /// child node is (re)stamped with its type, parsed properties and owner comp
    /// node, then reparented using the same get-or-create + EnsureRooted pattern
    /// as the other trees, so events can arrive in any order.
    /// </summary>
    public void AddWucChild(ulong parentId, ulong childId, ulong ownerCompNodeId, int index, string? visualType, string? properties)
    {
        var parent = GetOrCreateWucNode(parentId);
        var child = GetOrCreateWucNode(childId);

        ApplyWucPayload(child, visualType, properties);
        SetWucOwner(child, ownerCompNodeId);

        RemoveFromCurrentParent(child, WucVisualRoots);
        child.ParentId = parentId;
        if (!parent.Children.Contains(child))
            parent.Children.Add(child);
        EnsureRooted(parent, WucVisualRoots);
        child.IsHighlighted = true;

        // A pending app→profiler pick may be waiting for exactly this visual to arrive.
        TryApplyPendingVisualHighlight(child);
    }

    /// <summary>Detach a child IVisual from its parent (the node is kept so it can
    /// be re-inserted elsewhere, mirroring <see cref="RemoveCompChild"/>).</summary>
    public void RemoveWucChild(ulong parentId, ulong childId)
    {
        var child = FindWucNode(childId);
        if (child == null) return;

        var parent = FindWucNode(parentId);
        if (parent != null) parent.Children.Remove(child);
        else WucVisualRoots.Remove(child);
        child.ParentId = 0;
    }

    /// <summary>
    /// Remove every child of a parent IVisual (producer did a RemoveAll on the
    /// container). The whole child subtree is dropped from the lookup tables to
    /// keep this high-churn tree bounded.
    /// </summary>
    public void ClearWucChildren(ulong parentId)
    {
        var parent = FindWucNode(parentId);
        if (parent == null) return;
        foreach (var child in parent.Children.ToList())
            RemoveWucSubtree(child);
        parent.Children.Clear();
    }

    /// <summary>
    /// Set a root IVisual onto an island/content target. Records the
    /// target → root mapping so a later <see cref="ClearWucRoot"/> (which only
    /// carries the target id) can find and drop the root subtree.
    /// </summary>
    public void SetWucRoot(ulong visualId, ulong targetId, ulong ownerCompNodeId, string? visualType, string? properties)
    {
        var node = GetOrCreateWucNode(visualId);
        ApplyWucPayload(node, visualType, properties);
        SetWucOwner(node, ownerCompNodeId);
        node.IslandTargetId = targetId;
        node.DisplayName = $"{(string.IsNullOrEmpty(visualType) ? "IVisual" : visualType)} 0x{visualId:X} [root → 0x{targetId:X}]";

        RemoveFromCurrentParent(node, WucVisualRoots);
        node.ParentId = 0;
        EnsureRooted(node, WucVisualRoots);
        if (targetId != 0) _wucRootByTarget[targetId] = visualId;
        node.IsHighlighted = true;

        // A pending app→profiler pick may be waiting for exactly this visual to arrive.
        TryApplyPendingVisualHighlight(node);
    }

    /// <summary>Clear the root IVisual associated with an island/content target.</summary>
    public void ClearWucRoot(ulong targetId)
    {
        if (!_wucRootByTarget.TryGetValue(targetId, out var visualId)) return;
        _wucRootByTarget.Remove(targetId);
        var node = FindWucNode(visualId);
        if (node != null) RemoveWucSubtree(node);
    }

    /// <summary>Record an event on the WUC node with the given IVisual ID.</summary>
    public void RecordWucEvent(ulong id, string eventName, string summary = "")
    {
        if (_wucNodes.TryGetValue(id, out var node))
            node.RecordEvent(eventName, summary);
    }

    private static void ApplyWucPayload(TreeNode node, string? visualType, string? properties)
    {
        if (!string.IsNullOrEmpty(visualType)) node.VisualType = visualType;
        if (!string.IsNullOrEmpty(properties)) node.VisualProperties = WucVisualProperties.Parse(properties);

        var typeName = string.IsNullOrEmpty(node.VisualType) ? "IVisual" : node.VisualType;
        var owner = node.LinkedCompNodeId.GetValueOrDefault();
        node.DisplayName = owner != 0
            ? $"{typeName} 0x{node.Id:X} (comp 0x{owner:X})"
            : $"{typeName} 0x{node.Id:X}";
    }

    /// <summary>Maintain the owner-comp back-link and the reverse index.</summary>
    private void SetWucOwner(TreeNode node, ulong ownerCompNodeId)
    {
        var old = node.LinkedCompNodeId.GetValueOrDefault();
        if (old == ownerCompNodeId) return;

        if (old != 0 && _wucByOwnerComp.TryGetValue(old, out var oldList))
        {
            oldList.Remove(node);
            if (oldList.Count == 0) _wucByOwnerComp.Remove(old);
        }

        node.LinkedCompNodeId = ownerCompNodeId != 0 ? ownerCompNodeId : (ulong?)null;

        if (ownerCompNodeId != 0)
        {
            if (!_wucByOwnerComp.TryGetValue(ownerCompNodeId, out var list))
            {
                list = new List<TreeNode>();
                _wucByOwnerComp[ownerCompNodeId] = list;
            }
            if (!list.Contains(node)) list.Add(node);
        }
    }

    /// <summary>Recursively drop a WUC node and its descendants from all tables.</summary>
    private void RemoveWucSubtree(TreeNode node)
    {
        foreach (var child in node.Children.ToList())
            RemoveWucSubtree(child);
        node.Children.Clear();

        WucVisualRoots.Remove(node);
        SetWucOwner(node, 0);   // drops the reverse-index entry
        _wucNodes.Remove(node.Id);
        if (node.IslandTargetId is ulong t) _wucRootByTarget.Remove(t);
        node.ParentId = 0;
    }


    // =====================================================================
    // Cross-tree selection highlight
    //
    // When the user clicks a node in any tree we mark its linked peer in the
    // other two trees with LinkHighlight=Peer (bright glow) and every ancestor
    // on the path down to that peer with LinkHighlight=Path (dim breadcrumb).
    // The user can then expand the dim ancestors to find the bright peer.
    // =====================================================================

    // Nodes currently carrying any LinkHighlight value. Tracked so the next
    // selection only has to clear what was set, not walk every dictionary.
    private readonly HashSet<TreeNode> _linkHighlighted = new();

    // Nodes currently carrying the transient NewNode spotlight glow. Tracked
    // separately from _linkHighlighted so a spotlight and a pick highlight can
    // coexist and be cleared independently (spotlight clears on its own timer).
    private readonly HashSet<TreeNode> _spotlitNodes = new();

    /// <summary>
    /// Mark the cross-tree peer of <paramref name="source"/> (and the ancestor
    /// chain leading to it) in the OTHER two trees so the user can locate the
    /// peer even when it's nested inside collapsed subtrees.
    ///
    /// We deliberately never glow in the source's own tree — that tree already
    /// shows the TreeView's native selection on the clicked item, and a stale
    /// or self-referential <c>LinkedXxxNodeId</c> on the source could otherwise
    /// cause an unrelated sibling to light up.
    /// </summary>
    public void HighlightLinkedFor(TreeNode? source)
    {
        HighlightLinkedForCore(source);
        RaiseReveal();
    }

    // Collects the cross-tree peer glow + reveal anchors WITHOUT raising RevealRequested, so
    // callers (Ctrl+Click vs. app-pick) control exactly when the single reveal fires.
    private void HighlightLinkedForCore(TreeNode? source)
    {
        ClearLinkHighlights();
        _revealAnchors.Clear();
        if (source is null) return;

        // Resolve the two bridge ids that connect all four trees:
        //   elementId — the CUIElement* (shared key of the Logical & Visual trees)
        //   compId    — the HWCompNode* (key of the Composition tree)
        // The WUC tree hangs off compId via OwnerCompNodeId (LinkedCompNodeId).
        ulong elementId = 0, compId = 0;
        switch (source.Kind)
        {
            case TreeNodeKind.Logical:
            case TreeNodeKind.Visual:
                elementId = source.Id;
                compId = source.LinkedCompNodeId
                         ?? (_compByElement.TryGetValue(elementId, out var c) ? c.Id : 0);
                break;
            case TreeNodeKind.Composition:
                compId = source.Id;
                elementId = source.LinkedVisualNodeId ?? source.LinkedLogicalNodeId ?? 0;
                break;
            case TreeNodeKind.WucVisual:
                compId = source.LinkedCompNodeId ?? 0;        // owning comp node
                var ownerComp = compId != 0 ? FindCompNode(compId) : null;
                elementId = ownerComp?.LinkedVisualNodeId ?? ownerComp?.LinkedLogicalNodeId ?? 0;
                break;
        }

        // Glow the peer (and its ancestor breadcrumb) in every tree except the
        // source's own tree.
        if (source.Kind != TreeNodeKind.Logical)
            HighlightPathTo(elementId, _logicalNodes, TreeNodeKind.Logical);
        if (source.Kind != TreeNodeKind.Visual)
            HighlightPathTo(elementId, _visualNodes, TreeNodeKind.Visual);
        if (source.Kind != TreeNodeKind.Composition)
            HighlightPathTo(compId, _compNodes, TreeNodeKind.Composition);
        if (source.Kind != TreeNodeKind.WucVisual)
            HighlightWucOwnedBy(compId);
    }

    /// <summary>
    /// Glow the COMPLETE IVisual subtree owned by the given comp node: for each WUC visual
    /// the comp node owns, glow that visual AND every descendant (and expand the path to it
    /// plus the subtree itself so it is realized/visible). This is the same full-subtree glow
    /// used by the app→profiler pick (<see cref="HighlightSubtreeForVisual"/>), so Ctrl+Click
    /// on a Composition/Visual/Logical node lights up the entire corresponding slice of the
    /// dense IVisual tree rather than just an ancestor breadcrumb.
    /// </summary>
    private void HighlightWucOwnedBy(ulong compId)
    {
        if (compId == 0) return;
        if (!_wucByOwnerComp.TryGetValue(compId, out var owned)) return;

        foreach (var peer in owned)
        {
            // Skip detached/orphan visuals (no parent and not a real tree root).
            if (peer.ParentId == 0 && !IsRootOf(peer, TreeNodeKind.WucVisual)) continue;

            GlowSubtree(peer);        // Peer-glow this visual + all descendants
            ExpandAncestors(peer);    // reveal the path down to it
            ExpandSubtree(peer);      // reveal the whole glowed subtree
            AddAnchor(peer);          // scroll target in the IVisual tree (last wins)
        }
    }

    /// <summary>
    /// Pick-mode selection: glow <paramref name="source"/> itself and expand its
    /// ancestors so it is realized/visible, then glow its cross-tree peers (same as a
    /// Ctrl+Click). Used when the user picks a live element in the target app.
    /// </summary>
    public void HighlightAndExpandFor(TreeNode? source)
    {
        if (source is null) return;

        // Glow peers in the OTHER trees (this also clears prior link highlights and collects
        // their reveal anchors). Use the Core variant so we raise RevealRequested only ONCE,
        // after the source-node anchor below is included.
        HighlightLinkedForCore(source);

        // Glow the picked node itself in its own tree and expand the path to it.
        if (source.LinkHighlight != LinkHighlightKind.Peer)
        {
            source.LinkHighlight = LinkHighlightKind.Peer;
            _linkHighlighted.Add(source);
        }
        ExpandAncestors(source);
        AddAnchor(source);   // also scroll the picked node into view in its own tree

        RaiseReveal();
    }

    /// <summary>
    /// Pick-mode (IVisual subtree): given a composition visual id (an IVisual* — the clicked
    /// element's GetElementVisual, by its xpid Comment or raw pointer), find the matching node
    /// in the IVisual (WucVisual) tree, else the Composition tree, then glow that node AND its
    /// entire subtree, expanding the path to it and the subtree so the glow is visible.
    /// Returns the matched node, or null if no node with that id exists in either tree.
    /// Additive: does NOT clear existing link highlights (the pick handler clears once up
    /// front so the element-node selection and this subtree glow can coexist).
    /// </summary>
    public TreeNode? HighlightSubtreeForVisual(ulong visualId)
    {
        if (visualId == 0) return null;

        var node = FindWucNode(visualId) ?? FindCompNode(visualId);
        if (node is null)
        {
            // Not in the ETW-built store yet. The tap's GetElementVisual frequently CREATES the
            // element's hand-in visual on the spot, so the producer only emits (and xpid-stamps)
            // it on the next render frame. Arm a pending highlight that fires the instant that
            // node arrives over ETW (see TryApplyPendingVisualHighlight).
            _pendingVisualHighlight = visualId;
            return null;
        }

        GlowSubtree(node);
        ExpandAncestors(node);
        ExpandSubtree(node);

        // Reveal the matched IVisual/Composition node in its tree (additive: keep any peer
        // anchors the element-pick already revealed; this is a separate, later reveal).
        _revealAnchors.Clear();
        AddAnchor(node);
        RaiseReveal();
        return node;
    }

    /// <summary>
    /// If a deferred visual-subtree highlight is armed for this node's id, apply it now that the
    /// node exists and is rooted. Called right after a WUC node is created/parented from an ETW
    /// insert, so an app→profiler pick whose IVisual hadn't been emitted yet still lights up.
    /// </summary>
    private void TryApplyPendingVisualHighlight(TreeNode node)
    {
        if (_pendingVisualHighlight == 0 || node.Id != _pendingVisualHighlight) return;
        // Wait until the node is actually in the live tree (rooted), so ExpandAncestors has a
        // path to walk; otherwise leave it armed for the insert that roots it.
        if (node.ParentId == 0 && !IsRootOf(node, TreeNodeKind.WucVisual)) return;

        _pendingVisualHighlight = 0;
        GlowSubtree(node);
        ExpandAncestors(node);
        ExpandSubtree(node);
        VisualSubtreeHighlightApplied?.Invoke(node);

        // Now that the deferred IVisual finally arrived, reveal+scroll to it too.
        _revealAnchors.Clear();
        AddAnchor(node);
        RaiseReveal();
    }

    // Record a per-tree scroll/reveal target. At most one anchor per tree kind survives in the
    // view (last write wins), so the deepest/most-specific glowed node is the one scrolled to.
    private void AddAnchor(TreeNode? node)
    {
        if (node != null) _revealAnchors.Add(node);
    }

    // Snapshot the collected anchors to the view so it can expand+scroll each tree to its node.
    // passive=true marks a streaming/spotlight reveal (must not fight a user who scrolled away);
    // passive=false is an explicit user pick/link and always scrolls.
    private void RaiseReveal(bool passive = false)
    {
        if (_revealAnchors.Count == 0) return;
        RevealRequested?.Invoke(_revealAnchors.ToList(), passive);
    }

    /// <summary>
    /// Apply the transient "new node" spotlight to a freshly-added batch: clear any prior
    /// spotlight, glow each node that is actually attached to its live tree, expand its
    /// ancestors, and request a reveal/scroll to the deepest new node per tree. The glow is
    /// cleared later by <see cref="ClearSpotlight"/> (driven by the view's timer).
    /// </summary>
    public void SpotlightNewNodes(IReadOnlyList<TreeNode> nodes)
    {
        ClearSpotlight();
        _revealAnchors.Clear();

        // One reveal anchor per tree — last write wins, so the most recently added
        // node in each tree becomes that tree's scroll target.
        var anchorByKind = new Dictionary<TreeNodeKind, TreeNode>();
        foreach (var n in nodes)
        {
            // Only spotlight nodes that are genuinely in the live tree (rooted or
            // parented). Created-but-detached placeholders have nothing to scroll to.
            if (n.ParentId == 0 && !IsRootOf(n, n.Kind)) continue;

            n.LinkHighlight = LinkHighlightKind.NewNode;
            _spotlitNodes.Add(n);
            ExpandAncestors(n);
            anchorByKind[n.Kind] = n;
        }

        if (_spotlitNodes.Count == 0) return;

        _revealAnchors.AddRange(anchorByKind.Values);
        RaiseReveal(passive: true);
    }

    /// <summary>Clear the transient new-node spotlight glow set by <see cref="SpotlightNewNodes"/>.</summary>
    public void ClearSpotlight()
    {
        foreach (var n in _spotlitNodes)
        {
            // Don't stomp a real pick highlight that may have landed on the node since.
            if (n.LinkHighlight == LinkHighlightKind.NewNode)
                n.LinkHighlight = LinkHighlightKind.None;
        }
        _spotlitNodes.Clear();
    }

    // Glow (LinkHighlight = Peer) the node and every descendant in its own tree.
    private void GlowSubtree(TreeNode root)
    {
        var stack = new Stack<TreeNode>();
        stack.Push(root);
        var visited = new HashSet<TreeNode>();
        while (stack.Count > 0)
        {
            var n = stack.Pop();
            if (!visited.Add(n)) continue;   // cycle safety
            if (n.LinkHighlight != LinkHighlightKind.Peer)
            {
                n.LinkHighlight = LinkHighlightKind.Peer;
                _linkHighlighted.Add(n);
            }
            foreach (var c in n.Children) stack.Push(c);
        }
    }

    // Expand (IsExpanded = true) the node and every descendant so the whole glowed
    // subtree is realized and visible in the TreeView.
    private static void ExpandSubtree(TreeNode root)
    {
        var stack = new Stack<TreeNode>();
        stack.Push(root);
        var visited = new HashSet<TreeNode>();
        while (stack.Count > 0)
        {
            var n = stack.Pop();
            if (!visited.Add(n)) continue;   // cycle safety
            n.IsExpanded = true;
            foreach (var c in n.Children) stack.Push(c);
        }
    }

    // Expand (IsExpanded = true) every ancestor of the node in its own tree so the
    // TreeView realizes and reveals it.
    private void ExpandAncestors(TreeNode node)
    {
        Dictionary<ulong, TreeNode>? dict = node.Kind switch
        {
            TreeNodeKind.Logical => _logicalNodes,
            TreeNodeKind.Visual => _visualNodes,
            TreeNodeKind.Composition => _compNodes,
            TreeNodeKind.WucVisual => _wucNodes,
            _ => null
        };
        if (dict is null) return;

        var cursor = node;
        var visited = new HashSet<TreeNode> { cursor };
        while (cursor.ParentId != 0 && dict.TryGetValue(cursor.ParentId, out var parent))
        {
            if (!visited.Add(parent)) break;   // cycle safety
            parent.IsExpanded = true;
            cursor = parent;
        }
    }

    /// <summary>
    /// Return the chain of nodes from this node's tree root down to (and including)
    /// <paramref name="node"/>, in root→node order. The view walks this chain to
    /// progressively realize and scroll a virtualized path: each tree is built from
    /// nested <c>TreeViewItem</c> ItemsControls, so a deep node's container does not
    /// exist until every ancestor has been realized and scrolled into view in turn.
    /// </summary>
    public IReadOnlyList<TreeNode> GetAncestorChain(TreeNode node)
    {
        Dictionary<ulong, TreeNode>? dict = node.Kind switch
        {
            TreeNodeKind.Logical => _logicalNodes,
            TreeNodeKind.Visual => _visualNodes,
            TreeNodeKind.Composition => _compNodes,
            TreeNodeKind.WucVisual => _wucNodes,
            _ => null
        };

        var chain = new List<TreeNode> { node };
        if (dict is null) return chain;

        var cursor = node;
        var visited = new HashSet<TreeNode> { cursor };
        while (cursor.ParentId != 0 && dict.TryGetValue(cursor.ParentId, out var parent))
        {
            if (!visited.Add(parent)) break;   // cycle safety
            chain.Add(parent);
            cursor = parent;
        }

        chain.Reverse();   // root → … → node
        return chain;
    }

    /// <summary>Clear all cross-tree highlights set by a previous selection.</summary>
    public void ClearLinkHighlights()
    {
        foreach (var n in _linkHighlighted)
        {
            n.LinkHighlight = LinkHighlightKind.None;
        }
        _linkHighlighted.Clear();

        // Cancel any armed deferred visual highlight so a stale pick can't fire later.
        _pendingVisualHighlight = 0;
        // Likewise cancel any armed deferred element pick.
        _pendingPickHandle = 0;
    }

    private void HighlightPathTo(ulong? targetId, Dictionary<ulong, TreeNode> dict, TreeNodeKind expectedKind)
    {
        if (!targetId.HasValue || targetId.Value == 0) return;
        if (!dict.TryGetValue(targetId.Value, out var peer)) return;

        // Defensive: the dict is keyed per-tree, so this should always match,
        // but if a bad LinkedXxxNodeId ever pointed across key spaces we'd
        // catch it here instead of glowing the wrong tree.
        if (peer.Kind != expectedKind) return;

        // If the peer isn't currently in the live tree (detached after
        // ElementLeftTree but kept in the dictionary for resurrection), don't
        // glow it — there's nothing visible for the user to navigate to.
        if (peer.ParentId == 0 && !IsRootOf(peer, expectedKind)) return;

        peer.LinkHighlight = LinkHighlightKind.Peer;
        _linkHighlighted.Add(peer);
        AddAnchor(peer);   // the deepest glowed node in this tree — scroll target

        // Walk up the parent chain marking each ancestor as on-path AND expanding it, so
        // the tree unfolds itself all the way down to the glowing peer (no manual expand).
        var cursor = peer;
        var visited = new HashSet<TreeNode> { cursor };
        while (cursor.ParentId != 0 && dict.TryGetValue(cursor.ParentId, out var parent))
        {
            if (!visited.Add(parent)) break;   // cycle safety
            if (parent.LinkHighlight != LinkHighlightKind.Peer)
            {
                parent.LinkHighlight = LinkHighlightKind.Path;
                _linkHighlighted.Add(parent);
            }
            parent.IsExpanded = true;
            cursor = parent;
        }
    }

    private bool IsRootOf(TreeNode node, TreeNodeKind kind) => kind switch
    {
        TreeNodeKind.Logical => LogicalRoots.Contains(node),
        TreeNodeKind.Visual => VisualRoots.Contains(node),
        TreeNodeKind.Composition => CompositionRoots.Contains(node),
        TreeNodeKind.WucVisual => WucVisualRoots.Contains(node),
        _ => false,
    };

    // =====================================================================
    // Helpers
    // =====================================================================

    private void RemoveFromCurrentParent(TreeNode node, ObservableCollection<TreeNode> roots)
    {
        if (node.ParentId != 0)
        {
            // The node is being reparented: detach it from its OLD parent's children
            // first, otherwise it would appear under both the old and the new parent.
            // Look the old parent up in the same tree the node belongs to.
            var oldParent = node.Kind switch
            {
                TreeNodeKind.Logical => FindLogicalNode(node.ParentId),
                TreeNodeKind.Visual => FindVisualNode(node.ParentId),
                TreeNodeKind.Composition => FindCompNode(node.ParentId),
                TreeNodeKind.WucVisual => FindWucNode(node.ParentId),
                _ => null,
            };
            oldParent?.Children.Remove(node);
            node.ParentId = 0;
            return;
        }
        roots.Remove(node);
    }

    private static void EnsureRooted(TreeNode node, ObservableCollection<TreeNode> roots)
    {
        if (node.ParentId == 0 && !roots.Contains(node))
        {
            roots.Add(node);
        }
    }

    /// <summary>
    /// Ensures the node appears as a root in the logical tree.
    /// Used when a developer-authored element has no logical ancestor.
    /// </summary>
    public void EnsureLogicalRoot(TreeNode node)
    {
        EnsureRooted(node, LogicalRoots);
    }

    // =====================================================================
    // Event provenance — append an ETW event to the touched node's history.
    // First call per node also stamps CreatedByEvent (rendered as a badge).
    // No-ops if the node isn't in the lookup table — robust against
    // out-of-order or unknown-target events.
    // =====================================================================

    /// <summary>Record an event on the logical node with the given UIElement ID.</summary>
    public void RecordLogicalEvent(ulong id, string eventName, string summary = "")
    {
        if (_logicalNodes.TryGetValue(id, out var node))
            node.RecordEvent(eventName, summary);
    }

    /// <summary>Record an event on the visual node with the given UIElement ID.</summary>
    public void RecordVisualEvent(ulong id, string eventName, string summary = "")
    {
        if (_visualNodes.TryGetValue(id, out var node))
            node.RecordEvent(eventName, summary);
    }

    /// <summary>Record an event on the comp node with the given HWCompTreeNode ID.</summary>
    public void RecordCompEvent(ulong id, string eventName, string summary = "")
    {
        if (_compNodes.TryGetValue(id, out var node))
            node.RecordEvent(eventName, summary);
    }

    /// <summary>
    /// Resolve an element pointer ID to a human-readable name for use in
    /// summary strings. Returns <c>"Label (0xADDR)"</c> when a label is
    /// available, otherwise falls back to <c>"0xADDR"</c>.
    /// Looks up logical nodes first (most events reference UIElement IDs),
    /// then visual, then composition.
    /// </summary>
    public string ResolveName(ulong id)
    {
        string? label = null;
        if (_logicalNodes.TryGetValue(id, out var n)) label = n.Label;
        else if (_visualNodes.TryGetValue(id, out n)) label = n.Label;
        else if (_compNodes.TryGetValue(id, out n)) label = n.Label;

        return string.IsNullOrEmpty(label)
            ? $"0x{id:X}"
            : $"{label} (0x{id:X})";
    }
}
