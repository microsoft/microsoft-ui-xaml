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
/// </summary>
public sealed class ProfilerEventRouter
{
    private readonly ProfilerTreeStore _store;

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

    private void HandleElementEnteredTree(in EtwEvent e)
    {
        var elementId = e.U64(0);
        var parentId = e.U64(1);
        var isLive = e.Bool(2);
        var label = e.Str(3);
        var isTemplateChild = e.Bool(4);
        var peerHandle = e.U64(5);
        var isInfra = ProfilerTreeStore.IsInfrastructureLabel(label);
        var summary = $"parent={_store.ResolveName(parentId)}, live={isLive}";

        // Always enter visual tree
        _store.MarkVisualEnter(elementId, parentId, isLive, label);
        _store.RecordVisualEvent(elementId, "ElementEnteredTree",
            summary + (isTemplateChild ? " [template]" : isInfra ? " [infra]" : ""));

        if (!isTemplateChild && !isInfra)
        {
            // Developer-authored → logical tree with corrected parent
            var logicalParentId = _store.FindNearestLogicalAncestor(parentId);
            if (logicalParentId == parentId)
            {
                _store.MarkLogicalEnter(elementId, parentId, isLive, label);
            }
            else
            {
                // Parent is template/infra — use nearest logical ancestor or 0 (root)
                _store.MarkLogicalEnter(elementId, logicalParentId, isLive, label);
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
    // Lazy-peer elements (e.g. ContentPresenter) traced PeerHandle=0 at enter time; this
    // back-fills the real InstanceHandle so they become live-highlightable after the fact.
    // SetPeerHandle is "first non-zero wins" and no-ops for ids we don't track, so this is safe
    // even for peers that never made it into a tree.
    private void HandlePeerAssociated(in EtwEvent e)
    {
        var elementId = e.U64(0);
        var peerHandle = e.U64(1);
        _store.SetPeerHandle(elementId, peerHandle);
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
    //   WucVisualChildInserted  : [0]parentVisualId u64, [1]childVisualId u64,
    //                             [2]ownerCompNodeId u64, [3]index i32,
    //                             [4]visualType str, [5]properties str
    //   WucVisualChildRemoved   : [0]parentVisualId u64, [1]childVisualId u64
    //   WucVisualChildrenCleared: [0]parentVisualId u64
    //   WucVisualRootSet        : [0]visualId u64, [1]targetId u64,
    //                             [2]ownerCompNodeId u64, [3]visualType str,
    //                             [4]properties str
    //   WucVisualRootCleared    : [0]targetId u64
    // =====================================================================

    private void HandleWucVisualChildInserted(in EtwEvent e)
    {
        var parentId = e.U64(0);
        var childId = e.U64(1);
        var ownerCompNodeId = e.U64(2);
        var index = e.I32(3);
        var visualType = e.Str(4);
        var properties = e.Str(5);
        _store.AddWucChild(parentId, childId, ownerCompNodeId, index, visualType, properties);
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
        var visualType = e.Str(3);
        var properties = e.Str(4);
        _store.SetWucRoot(visualId, targetId, ownerCompNodeId, visualType, properties);
        _store.RecordWucEvent(visualId, "WucVisualRootSet",
            $"target=0x{targetId:X} owner={_store.ResolveName(ownerCompNodeId)} type={visualType}");
    }

    private void HandleWucVisualRootCleared(in EtwEvent e)
    {
        var targetId = e.U64(0);
        _store.ClearWucRoot(targetId);
    }
}
