// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewGroupHeaderAutomationPeer.g.h"
#include "TableViewItemPeerCache.h"

// Automation peer for the group-header band.
//
// The header used to be a TableViewRow flipped into group mode, so its peer was
// TableViewRowAutomationPeer answering as either a row or a group depending on state. Now that
// the header is its own container, the peer is its own type: no adaptive branch, no GridItem
// coordinates for a band that spans every column, and the ExpandCollapse pattern is
// unconditional rather than state-gated.
class TableViewGroupHeaderAutomationPeer :
    public ReferenceTracker<
        TableViewGroupHeaderAutomationPeer,
        winrt::implementation::TableViewGroupHeaderAutomationPeerT>
{
public:
    struct GroupPeerTag {};

    // A header's own peer, which the header's OnCreateAutomationPeer may replace with a derived one.
    explicit TableViewGroupHeaderAutomationPeer(winrt::TableViewGroupHeader const& owner);
    // The peer of the group a header shows; see TableViewGroupHeader::SyncAutomationPeerToGroupInternal.
    TableViewGroupHeaderAutomationPeer(winrt::TableViewGroupHeader const& owner, GroupPeerTag);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::hstring GetLocalizedControlTypeCore();
    winrt::hstring GetClassNameCore();
    winrt::hstring GetNameCore();
    winrt::AutomationPeer GetLabeledByCore();

    // Lands keyboard focus for UIA SetFocus; see the definition.
    void SetFocusCore();

    // No menu while a cell is editing.
    void ShowContextMenuCore();

    // IExpandCollapseProvider
    void Expand();
    void Collapse();
    winrt::ExpandCollapseState ExpandCollapseState();

    // A header's own peer (possibly an app-derived type) is an internal forwarder; UIA is handed
    // its group's peer, always this base type, through EventsSource. A group peer answers only
    // while a header shows its group: it keeps ExpandCollapse and GridItem, forwards the rest
    // below to that header's own peer, and otherwise throws ElementNotAvailable. Forwarded patterns
    // come from that header's peer, as a ListView item peer's come from its container's.
    winrt::IInspectable NavigateCore(winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection const& direction);
    winrt::IVector<winrt::AutomationPeer> GetChildrenCore();
    winrt::Rect GetBoundingRectangleCore();
    bool IsOffscreenCore();
    bool IsEnabledCore();
    bool HasKeyboardFocusCore();
    bool IsKeyboardFocusableCore();
    winrt::Point GetClickablePointCore();
    winrt::hstring GetAutomationIdCore();
    winrt::hstring GetHelpTextCore();
    winrt::hstring GetItemStatusCore();
    winrt::hstring GetItemTypeCore();
    winrt::hstring GetFullDescriptionCore();
    winrt::Windows::Foundation::Collections::IIterable<winrt::AutomationPeer> GetDescribedByCore();
    winrt::hstring GetAccessKeyCore();
    winrt::hstring GetAcceleratorKeyCore();
    winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationLiveSetting GetLiveSettingCore();
    winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationHeadingLevel GetHeadingLevelCore();
    int32_t GetCultureCore();
    bool IsRequiredForFormCore();
    bool IsControlElementCore();
    bool IsContentElementCore();
    int32_t GetPositionInSetCore();
    int32_t GetSizeOfSetCore();
    winrt::Windows::Foundation::Collections::IVectorView<winrt::AutomationPeer> GetControlledPeersCore();
    winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationLandmarkType GetLandmarkTypeCore();
    winrt::hstring GetLocalizedLandmarkTypeCore();
    winrt::Windows::Foundation::Collections::IIterable<winrt::AutomationPeer> GetFlowsToCore();
    winrt::Windows::Foundation::Collections::IIterable<winrt::AutomationPeer> GetFlowsFromCore();
    bool IsPeripheralCore();
    bool IsDataValidForFormCore();
    winrt::IVector<winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationPeerAnnotation> GetAnnotationsCore();
    bool IsDialogCore();

    TableViewItemPeerBinding& ItemPeerBindingInternal() noexcept { return m_binding; }
    // The header still showing this group peer's group and forwarding to it; else null.
    winrt::UIElement ResolveBoundHeaderInternal() const;

    // Grouping is hierarchical, and Level is how an ExpandCollapse container conveys depth.
    // Without it Narrator hears two nested groups as siblings.
    int32_t GetLevelCore();

    // Internal: announce a state change from THIS peer. UIA delivers property-changed events
    // through the peer the client is connected to, so the raise has to happen on the peer
    // instance itself -- raising from a peer obtained via CreatePeerForElement can hand back a
    // fresh instance that no client is listening to, and the event goes nowhere. Same shape as
    // NavigationViewItemAutomationPeer::RaiseExpandCollapseAutomationEvent.
    void RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState oldState, winrt::ExpandCollapseState newState);

    // IGridItemProvider — the band is a merged cell spanning every visible column, which is how
    // TableViewAutomationPeer::GetItem reports it. Without this the span would not be queryable
    // from the element the client actually reached.
    int32_t Row();
    int32_t Column();
    int32_t RowSpan();
    int32_t ColumnSpan();
    winrt::IRawElementProviderSimple ContainingGrid();

private:
    winrt::TableViewGroupHeader GetHeader() const;
    // The header showing this peer's group (for a header's own peer, its owner); throws ElementNotAvailable if stale.
    winrt::TableViewGroupHeader GetBoundHeader() const;
    // For a group peer, its bound header's own peer; else null.
    winrt::AutomationPeer GetForwardingPeer() const;
    winrt::TableView GetOwningTableView();
    void SetExpansion(bool expand);

    TableViewItemPeerBinding m_binding;
    // A group peer keeps the header that created it, its Owner, alive: the framework holds the owner
    // unreferenced, and this peer outlives that header whenever the group moves to another one.
    tracker_ref<winrt::TableViewGroupHeader> m_ownerHeader{ this };
};
