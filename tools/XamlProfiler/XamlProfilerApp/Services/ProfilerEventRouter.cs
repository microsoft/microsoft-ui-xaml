using XamlProfiler.Models;

namespace XamlProfiler.Services;

/// <summary>
/// Translates snapshotted XamlProfilerTracing events into <see cref="ProfilerTreeStore"/>
/// mutations. This is pure domain logic with no ETW / threading dependency — the
/// <see cref="EtwListenerService"/> owns the transport (session, pump thread, batching)
/// and hands each drained event here via <see cref="Dispatch"/>.
///
/// Because it only depends on <see cref="EtwEvent"/> and the store, the router can be
/// unit-tested without an ETW session or Administrator rights: construct one over a
/// store, feed synthetic <see cref="EtwEvent"/> values, and assert the resulting tree state.
///
/// <c>ElementEnteredTree</c>, <c>WucVisualChildInserted</c> and <c>WucVisualRootSet</c> carry
/// scalar payloads only, so the details they used to carry come from granular events: element
/// labels from the once-per-type <c>ElementTypeInfo</c> and the Microsoft-Windows-XAML
/// <c>ElementSetName</c> (x:Name), peer handles from <c>PeerAssociated</c>, and visual properties
/// from the <c>WucVisual*Changed</c> events. The router holds that state and applies it when the
/// element enters the tree or the visual is inserted. It expects events in timestamp order
/// (see <see cref="EventReorderBuffer{T}"/>).
/// </summary>
public sealed class ProfilerEventRouter
{
    private readonly ProfilerTreeStore _store;

    // Granular state, keyed by TypeIndex, element id (core object address) and IVisual id.
    private readonly Dictionary<uint, string> _types = new();
    private readonly Dictionary<ulong, string> _names = new();
    private readonly Dictionary<ulong, ulong> _peers = new();
    private readonly Dictionary<ulong, WucVisualPropertyBag> _bags = new();
    // Elements that entered the tree before their type's ElementTypeInfo arrived, with the TypeIndex
    // they entered with. Around a session start, ElementTypeInfo can trail the first enters of its
    // type, so the type name is applied when it arrives.
    private readonly Dictionary<ulong, uint> _untyped = new();

    public ProfilerEventRouter(ProfilerTreeStore store) => _store = store;

    public void Dispatch(in EtwEvent e)
    {
        switch (e.Name)
        {
            case "ChildAdded":            HandleChildAdded(e); break;
            case "ChildInserted":         HandleChildInserted(e); break;
            case "ChildRemoved":          HandleChildRemoved(e); break;
            case "ElementEnteredTree":    HandleElementEnteredTree(e); break;
            case "ElementLeftTree":       HandleElementLeftTree(e); break;
            case "PeerAssociated":        HandlePeerAssociated(e); break;
            case "ContentChanged":        HandleContentChanged(e); break;
            case "PopupOpened":           HandlePopupOpened(e); break;
            case "PopupClosed":           HandlePopupClosed(e); break;
            case "CompPeerLinked":        HandleCompPeerLinked(e); break;
            case "CompPeerUnlinked":      HandleCompPeerUnlinked(e); break;
            case "CompNodeChildInserted": HandleCompNodeChildInserted(e); break;
            case "CompNodeChildRemoved":  HandleCompNodeChildRemoved(e); break;
            case "WucVisualChildInserted":   HandleWucVisualChildInserted(e); break;
            case "WucVisualChildRemoved":    HandleWucVisualChildRemoved(e); break;
            case "WucVisualChildrenCleared": HandleWucVisualChildrenCleared(e); break;
            case "WucVisualRootSet":         HandleWucVisualRootSet(e); break;
            case "WucVisualRootCleared":     HandleWucVisualRootCleared(e); break;
            case "ElementTypeInfo":           HandleElementTypeInfo(e); break;
            case "WucVisualCreated":          HandleWucVisualCreated(e); break;
            case "WucVisualPropertyChanged":  HandleWucVisualPropertyChanged(e); break;
            case "WucVisualTransformChanged": HandleWucVisualTransformChanged(e); break;
            case "WucVisualObjectChanged":    HandleWucVisualObjectChanged(e); break;
            case ProfilerProviders.ElementCreatedEvent: HandleElementCreated(e); break;
            case ProfilerProviders.ElementSetNameEvent: HandleElementSetName(e); break;
            default: /* unknown — ignore */ break;
        }
    }

    // =====================================================================
    // Logical tree events
    // =====================================================================

    private void HandleChildAdded(in EtwEvent e)
    {
        var parentId = e.U64(0);
        var childId = e.U64(1);
        // payload[2] = Index
        var childLabel = e.Str(3);
        var isTemplateChild = e.Bool(4);
        var childPeerHandle = e.U64(5);
        var isInfra = ProfilerTreeStore.IsInfrastructureLabel(childLabel);

        // Always add to visual tree
        _store.AddVisualChild(parentId, childId, childLabel);
        _store.RecordVisualEvent(childId, "ChildAdded",
            $"parent={_store.ResolveName(parentId)}{(isTemplateChild ? " [template]" : isInfra ? " [infra]" : "")}");

        if (!isTemplateChild && !isInfra)
        {
            // Developer-authored child → add to logical tree
            var logicalParentId = _store.FindNearestLogicalAncestor(parentId);
            if (logicalParentId == parentId)
            {
                _store.AddLogicalChild(parentId, childId, childLabel);
            }
            else if (logicalParentId != 0)
            {
                _store.AddLogicalChild(logicalParentId, childId, childLabel);
            }
            else
            {
                var node = _store.GetOrCreateLogicalNode(childId, childLabel);
                _store.EnsureLogicalRoot(node);
            }
            _store.RecordLogicalEvent(childId, "ChildAdded", $"parent={_store.ResolveName(parentId)}");
        }

        _store.SetPeerHandle(childId, childPeerHandle);
    }

    private void HandleChildInserted(in EtwEvent e)
    {
        var parentId = e.U64(0);
        var childId = e.U64(1);
        // payload[2] = Index
        var childLabel = e.Str(3);
        var isTemplateChild = e.Bool(4);
        var childPeerHandle = e.U64(5);
        var isInfra = ProfilerTreeStore.IsInfrastructureLabel(childLabel);

        _store.AddVisualChild(parentId, childId, childLabel);
        _store.RecordVisualEvent(childId, "ChildInserted",
            $"parent={_store.ResolveName(parentId)}{(isTemplateChild ? " [template]" : isInfra ? " [infra]" : "")}");

        if (!isTemplateChild && !isInfra)
        {
            var logicalParentId = _store.FindNearestLogicalAncestor(parentId);
            if (logicalParentId == parentId)
            {
                _store.AddLogicalChild(parentId, childId, childLabel);
            }
            else if (logicalParentId != 0)
            {
                _store.AddLogicalChild(logicalParentId, childId, childLabel);
            }
            else
            {
                var node = _store.GetOrCreateLogicalNode(childId, childLabel);
                _store.EnsureLogicalRoot(node);
            }
            _store.RecordLogicalEvent(childId, "ChildInserted", $"parent={_store.ResolveName(parentId)}");
        }

        _store.SetPeerHandle(childId, childPeerHandle);
    }

    private void HandleChildRemoved(in EtwEvent e)
    {
        var parentId = e.U64(0);
        var childId = e.U64(1);

        // Always remove from visual tree
        _store.RemoveVisualChild(parentId, childId);
        _store.RecordVisualEvent(childId, "ChildRemoved", $"parent={_store.ResolveName(parentId)}");

        // Also remove from logical tree if it's there
        var logicalNode = _store.FindLogicalNode(childId);
        if (logicalNode != null)
        {
            _store.RecordLogicalEvent(childId, "ChildRemoved", $"parent={_store.ResolveName(parentId)}");
            _store.RemoveLogicalChild(logicalNode.ParentId, childId);
        }
    }

    // [0]elementId u64, [1]parentId u64, [2]isLive bool, [3]isTemplateChild bool, [4]typeIndex u32.
    // The label is the element's x:Name, else its class name. The class name is provisional: the
    // x:Name of an element named before the session started is never seen here, but a later
    // ChildAdded carries it and replaces the class name.
    private void HandleElementEnteredTree(in EtwEvent e)
    {
        var elementId = e.U64(0);
        var parentId = e.U64(1);
        var isLive = e.Bool(2);
        var isTemplateChild = e.Bool(3);
        var typeIndex = e.U32(4);
        var label = string.Empty;
        var provisional = false;
        _untyped.Remove(elementId);
        if (_names.TryGetValue(elementId, out var name) && !string.IsNullOrEmpty(name))
        {
            label = name;
        }
        else if (_types.TryGetValue(typeIndex, out var typeName))
        {
            label = typeName;
            provisional = true;
        }
        else
        {
            _untyped[elementId] = typeIndex;
        }
        _peers.TryGetValue(elementId, out var peerHandle);
        var isInfra = ProfilerTreeStore.IsInfrastructureLabel(label);
        var summary = $"parent={_store.ResolveName(parentId)}, live={isLive}";

        // Always enter visual tree
        _store.MarkVisualEnter(elementId, parentId, isLive, label, provisional);
        _store.RecordVisualEvent(elementId, "ElementEnteredTree",
            summary + (isTemplateChild ? " [template]" : isInfra ? " [infra]" : ""));

        if (!isTemplateChild && !isInfra)
        {
            // Developer-authored → logical tree with corrected parent
            var logicalParentId = _store.FindNearestLogicalAncestor(parentId);
            if (logicalParentId == parentId)
            {
                _store.MarkLogicalEnter(elementId, parentId, isLive, label, provisional);
            }
            else
            {
                // Parent is template/infra — use nearest logical ancestor or 0 (root)
                _store.MarkLogicalEnter(elementId, logicalParentId, isLive, label, provisional);
            }
            _store.RecordLogicalEvent(elementId, "ElementEnteredTree", summary);
        }

        _store.SetPeerHandle(elementId, peerHandle);
    }

    private void HandleElementLeftTree(in EtwEvent e)
    {
        var elementId = e.U64(0);
        var parentId = e.U64(1);
        var isLive = e.Bool(2);
        var summary = $"parent={_store.ResolveName(parentId)}, live={isLive}";

        // Always leave visual tree
        _store.MarkVisualLeave(elementId, parentId);
        _store.RecordVisualEvent(elementId, "ElementLeftTree", summary);

        // Also leave logical tree if present
        var logicalNode = _store.FindLogicalNode(elementId);
        if (logicalNode != null)
        {
            _store.RecordLogicalEvent(elementId, "ElementLeftTree", summary);
            _store.MarkLogicalLeave(elementId, parentId, isLive);
        }
    }

    // Fired by the producer the instant a DXaml peer is created and bound to its core object.
    // Lazy-peer elements (e.g. ContentPresenter) get their peer after they enter the tree; this
    // back-fills the real InstanceHandle so they become live-highlightable after the fact.
    // SetPeerHandle is "first non-zero wins" and no-ops for ids we don't track, so this is safe
    // even for peers that never made it into a tree. ElementEnteredTree doesn't carry the peer,
    // so the handle is also held for a later enter (peer-first objects).
    private void HandlePeerAssociated(in EtwEvent e)
    {
        var elementId = e.U64(0);
        var peerHandle = e.U64(1);
        if (elementId != 0 && peerHandle != 0) _peers[elementId] = peerHandle;
        _store.SetPeerHandle(elementId, peerHandle);
    }

    // =====================================================================
    // Element details (ElementTypeInfo + Microsoft-Windows-XAML events)
    // =====================================================================

    // Written before the first enter of a TypeIndex and re-sent when a session starts, so it can
    // repeat: [0]typeIndex u32, [1]typeName str, [2]coreSize u64 (unused). Around a session start
    // it can trail the first enters of its type, so those elements get their label now.
    private void HandleElementTypeInfo(in EtwEvent e)
    {
        var typeIndex = e.U32(0);
        var typeName = e.Str(1);
        _types[typeIndex] = typeName;
        if (_untyped.Count == 0) return;
        foreach (var elementId in _untyped.Where(p => p.Value == typeIndex).Select(p => p.Key).ToList())
        {
            _untyped.Remove(elementId);
            _store.SetLateTypeName(elementId, typeName);
        }
    }

    // Microsoft-Windows-XAML ElementCreated, from the CUIElement constructor: [0]elementId u64.
    // A new element can reuse a destroyed element's address, so drop what was held for the old one.
    private void HandleElementCreated(in EtwEvent e)
    {
        var elementId = e.U64(0);
        _names.Remove(elementId);
        _peers.Remove(elementId);
        _untyped.Remove(elementId);
    }

    // Microsoft-Windows-XAML ElementSetName: [0]elementId u64, [1]name str. An empty name clears
    // x:Name, so it's stored too. Used as the label when the element next enters the tree.
    private void HandleElementSetName(in EtwEvent e)
    {
        var elementId = e.U64(0);
        if (elementId != 0) _names[elementId] = e.Str(1);
    }

    // =====================================================================
    // Content events
    // =====================================================================

    private void HandleContentChanged(in EtwEvent e)
    {
        var parentId = e.U64(0);
        var oldId = e.U64(1);
        var newId = e.U64(2);
        var newLabel = e.Str(3);
        var newContentPeerHandle = e.U64(4);

        // Capture the new content's peer handle on whichever of its nodes already exist; other
        // events (ChildAdded / ElementEnteredTree) stamp the rest. No-ops when newId is 0.
        _store.SetPeerHandle(newId, newContentPeerHandle);

        var parentVisual = _store.FindVisualNode(parentId);
        var parentLabel = parentVisual?.Label;
        var parentIsInfra = ProfilerTreeStore.IsInfrastructureLabel(parentLabel);
        var newIsInfra = ProfilerTreeStore.IsInfrastructureLabel(newLabel);
        var parentLabelKnown = !string.IsNullOrEmpty(parentLabel);

        // Always record event for history
        var infraTag = (parentIsInfra || newIsInfra) ? " [infra]" : "";
        _store.RecordLogicalEvent(parentId, "ContentChanged",
            $"old={_store.ResolveName(oldId)} → new={_store.ResolveName(newId)}{infraTag}");
        if (newId != 0)
        {
            _store.RecordLogicalEvent(newId, "ContentChanged",
                $"set as content of {_store.ResolveName(parentId)}{infraTag}");
        }

        // Decide whether to mutate logical tree
        if (newIsInfra) return;

        // Remove old content from logical tree (if it was there)
        if (oldId != 0)
        {
            var oldNode = _store.FindLogicalNode(oldId);
            if (oldNode != null)
            {
                _store.RemoveLogicalChild(oldNode.ParentId, oldId);
            }
        }

        if (newId == 0) return;

        // If parent label is unknown, defer logical-tree mutation —
        // ChildAdded / ElementEnteredTree will eventually parent the new content
        // with full knowledge of its label and template/infra status.
        if (!parentLabelKnown) return;

        if (parentIsInfra)
        {
            // Parent is infra but new content is developer-authored —
            // walk up to find nearest developer ancestor for logical parenting.
            var logicalParentId = _store.FindNearestLogicalAncestor(parentId);
            if (logicalParentId != 0)
            {
                _store.AddLogicalChild(logicalParentId, newId, newLabel);
            }
            else
            {
                var node = _store.GetOrCreateLogicalNode(newId, newLabel);
                _store.EnsureLogicalRoot(node);
            }
        }
        else
        {
            _store.AddLogicalChild(parentId, newId, newLabel);
        }
    }

    // =====================================================================
    // Popup events
    // =====================================================================

    private void HandlePopupOpened(in EtwEvent e)
    {
        var popupId = e.U64(0);
        var childId = e.U64(1);
        var popupLabel = e.Str(2);
        var childLabel = e.Str(3);
        var childPeerHandle = e.U64(4);
        _store.AddPopupChild(popupId, childId, opened: true, popupLabel, childLabel);
        _store.SetPeerHandle(childId, childPeerHandle);
        _store.RecordLogicalEvent(childId, "PopupOpened", $"popup={_store.ResolveName(popupId)}");
        _store.RecordVisualEvent(childId, "PopupOpened", $"popup={_store.ResolveName(popupId)}");
    }

    private void HandlePopupClosed(in EtwEvent e)
    {
        var popupId = e.U64(0);
        var childId = e.U64(1);
        _store.RecordLogicalEvent(childId, "PopupClosed", $"popup={_store.ResolveName(popupId)}");
        _store.RecordVisualEvent(childId, "PopupClosed", $"popup={_store.ResolveName(popupId)}");
        _store.AddPopupChild(popupId, childId, opened: false);
    }

    // =====================================================================
    // Composition tree events
    // =====================================================================

    private void HandleCompPeerLinked(in EtwEvent e)
    {
        var uiElementId = e.U64(0);
        var compNodeId = e.U64(1);
        var parentCompNodeId = e.U64(2);
        var elementLabel = e.Str(3);
        var uiElementPeerHandle = e.U64(4);
        _store.LinkCompPeer(uiElementId, compNodeId, parentCompNodeId, elementLabel);
        // After linking, _compByElement[uiElementId] exists, so this also stamps the comp node —
        // letting a Ctrl+Click on a composition node resolve its owning element's live handle.
        _store.SetPeerHandle(uiElementId, uiElementPeerHandle);
        var summary = $"element={_store.ResolveName(uiElementId)}, parentComp={_store.ResolveName(parentCompNodeId)}";
        _store.RecordCompEvent(compNodeId, "CompPeerLinked", summary);
        _store.RecordLogicalEvent(uiElementId, "CompPeerLinked", $"comp={_store.ResolveName(compNodeId)}");
        _store.RecordVisualEvent(uiElementId, "CompPeerLinked", $"comp={_store.ResolveName(compNodeId)}");
    }

    private void HandleCompPeerUnlinked(in EtwEvent e)
    {
        var uiElementId = e.U64(0);
        var compNodeId = e.U64(1);
        var summary = $"element={_store.ResolveName(uiElementId)}";
        _store.RecordCompEvent(compNodeId, "CompPeerUnlinked", summary);
        _store.RecordLogicalEvent(uiElementId, "CompPeerUnlinked", $"comp={_store.ResolveName(compNodeId)}");
        _store.RecordVisualEvent(uiElementId, "CompPeerUnlinked", $"comp={_store.ResolveName(compNodeId)}");
        _store.UnlinkCompPeer(uiElementId, compNodeId);
    }

    private void HandleCompNodeChildInserted(in EtwEvent e)
    {
        var parentCompNodeId = e.U64(0);
        var childCompNodeId = e.U64(1);
        _store.AddCompChild(parentCompNodeId, childCompNodeId);
        _store.RecordCompEvent(childCompNodeId, "CompNodeChildInserted",
            $"parentComp={_store.ResolveName(parentCompNodeId)}");
    }

    private void HandleCompNodeChildRemoved(in EtwEvent e)
    {
        var parentCompNodeId = e.U64(0);
        var childCompNodeId = e.U64(1);
        _store.RecordCompEvent(childCompNodeId, "CompNodeChildRemoved",
            $"parentComp={_store.ResolveName(parentCompNodeId)}");
        _store.RemoveCompChild(parentCompNodeId, childCompNodeId);
    }

    // =====================================================================
    // WUC (Windows.UI.Composition) IVisual tree events
    //
    // Payload layouts (see XamlProfilerTracing.h):
    //   WucVisualChildInserted   : [0]parentVisualId u64, [1]childVisualId u64,
    //                              [2]ownerCompNodeId u64, [3]index i32, [4]visualKind u32
    //   WucVisualChildRemoved    : [0]parentVisualId u64, [1]childVisualId u64
    //   WucVisualChildrenCleared : [0]parentVisualId u64
    //   WucVisualRootSet         : [0]visualId u64, [1]targetId u64,
    //                              [2]ownerCompNodeId u64, [3]visualKind u32
    //   WucVisualRootCleared     : [0]targetId u64
    //   WucVisualCreated         : [0]visualId u64, [1]visualKind u32
    //   WucVisualPropertyChanged : [0]visualId u64, [1]property u32, [2..5]X, Y, Z, W f32
    //   WucVisualTransformChanged: [0]visualId u64, [1..16]M11..M44 f32 (row-major)
    //   WucVisualObjectChanged   : [0]visualId u64, [1]property u32, [2]objectId u64 (0 = cleared),
    //                              [3]typeName str
    // Each visual's properties accumulate in a bag that the visual's tree node shares, so the
    // node always shows the latest values. Bags outlive the nodes (a removed visual keeps its
    // properties if it is re-inserted).
    // =====================================================================

    private void HandleWucVisualChildInserted(in EtwEvent e)
    {
        var parentId = e.U64(0);
        var childId = e.U64(1);
        var ownerCompNodeId = e.U64(2);
        var index = e.I32(3);
        var bag = BagWithKind(childId, (WucVisualKind)e.U32(4));
        var visualType = WucVisualPropertyBag.TypeNameFromKind(bag.Kind);
        _store.AddWucChild(parentId, childId, ownerCompNodeId, index, visualType, bag);
        _store.RecordWucEvent(childId, "WucVisualChildInserted",
            $"parent=0x{parentId:X} owner={_store.ResolveName(ownerCompNodeId)} type={visualType} idx={index}");
    }

    private void HandleWucVisualChildRemoved(in EtwEvent e)
    {
        var parentId = e.U64(0);
        var childId = e.U64(1);
        _store.RecordWucEvent(childId, "WucVisualChildRemoved", $"parent=0x{parentId:X}");
        _store.RemoveWucChild(parentId, childId);
    }

    private void HandleWucVisualChildrenCleared(in EtwEvent e)
    {
        var parentId = e.U64(0);
        _store.RecordWucEvent(parentId, "WucVisualChildrenCleared");
        _store.ClearWucChildren(parentId);
    }

    private void HandleWucVisualRootSet(in EtwEvent e)
    {
        var visualId = e.U64(0);
        var targetId = e.U64(1);
        var ownerCompNodeId = e.U64(2);
        var bag = BagWithKind(visualId, (WucVisualKind)e.U32(3));
        var visualType = WucVisualPropertyBag.TypeNameFromKind(bag.Kind);
        _store.SetWucRoot(visualId, targetId, ownerCompNodeId, visualType, bag);
        _store.RecordWucEvent(visualId, "WucVisualRootSet",
            $"target=0x{targetId:X} owner={_store.ResolveName(ownerCompNodeId)} type={visualType}");
    }

    private void HandleWucVisualRootCleared(in EtwEvent e)
    {
        var targetId = e.U64(0);
        _store.ClearWucRoot(targetId);
    }

    private void HandleWucVisualCreated(in EtwEvent e)
    {
        var visualId = e.U64(0);
        if (visualId != 0) Bag(visualId).Reset((WucVisualKind)e.U32(1));
    }

    private void HandleWucVisualPropertyChanged(in EtwEvent e)
    {
        var visualId = e.U64(0);
        if (visualId == 0) return;
        Bag(visualId).SetValue((WucVisualPropertyId)e.U32(1), e.F32(2), e.F32(3), e.F32(4), e.F32(5));
    }

    private void HandleWucVisualTransformChanged(in EtwEvent e)
    {
        var visualId = e.U64(0);
        if (visualId == 0) return;
        Span<float> matrix = stackalloc float[16];
        for (int i = 0; i < matrix.Length; i++) matrix[i] = e.F32(i + 1);
        Bag(visualId).SetTransform(matrix);
    }

    private void HandleWucVisualObjectChanged(in EtwEvent e)
    {
        var visualId = e.U64(0);
        if (visualId == 0) return;
        Bag(visualId).SetObject((WucVisualPropertyId)e.U32(1), e.U64(2), e.Str(3));
    }

    // Insert and root-set report the visual's kind (Unknown when the producer couldn't tell, which
    // keeps the kind already known). Every inserted visual gets a bag, so it shows properties
    // (composition defaults where XAML reported nothing), as the old full payload did.
    private WucVisualPropertyBag BagWithKind(ulong visualId, WucVisualKind kind)
    {
        var bag = Bag(visualId);
        if (kind != WucVisualKind.Unknown) bag.Kind = kind;
        return bag;
    }

    private WucVisualPropertyBag Bag(ulong visualId)
    {
        if (!_bags.TryGetValue(visualId, out var bag))
            _bags[visualId] = bag = new WucVisualPropertyBag();
        return bag;
    }
}
