// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewGroupHeaderAutomationPeer.h"
#include "TableViewGroupHeader.h"
#include "TableViewGroupInfo.h"
#include "TableView.h"
#include "TableViewAutomationHelpers.h"

#include "ResourceAccessor.h"

// Pulls in the generated activation factory (CppWinRTActivatableClassWithBasicFactory), the
// same way TableViewRowAutomationPeer does.
#include "TableViewGroupHeaderAutomationPeer.properties.cpp"

TableViewGroupHeaderAutomationPeer::TableViewGroupHeaderAutomationPeer(winrt::TableViewGroupHeader const& owner)
    : ReferenceTracker(owner)
{
    winrt::get_self<TableViewGroupHeader>(owner)->SyncAutomationPeerToGroupInternal(this);
}

TableViewGroupHeaderAutomationPeer::TableViewGroupHeaderAutomationPeer(winrt::TableViewGroupHeader const& owner, GroupPeerTag)
    : ReferenceTracker(owner)
{
    m_ownerHeader.set(owner);
}

winrt::TableViewGroupHeader TableViewGroupHeaderAutomationPeer::GetHeader() const
{
    return Owner().try_as<winrt::TableViewGroupHeader>();
}

winrt::UIElement TableViewGroupHeaderAutomationPeer::ResolveBoundHeaderInternal() const
{
    auto const container = m_binding.Resolve(
        [](winrt::UIElement const& element)
        {
            auto const header = element.try_as<winrt::TableViewGroupHeader>();
            return header ? winrt::get_self<TableViewGroupHeader>(header)->GroupIdentityInternal() : winrt::hstring{};
        },
        [this](winrt::AutomationPeer const& source)
        {
            auto const peer = source ? source.try_as<winrt::TableViewGroupHeaderAutomationPeer>() : nullptr;
            return peer && winrt::get_self<TableViewGroupHeaderAutomationPeer>(peer) == this;
        });

    // A header the owner's current repeater does not hold (its template was reapplied) shows nothing.
    auto const tableView = container ? winrt::get_self<TableViewGroupHeader>(container.as<winrt::TableViewGroupHeader>())->GetOwningTableView() : nullptr;
    auto const repeater = tableView ? winrt::get_self<TableView>(tableView)->GetRowsRepeaterInternal() : nullptr;
    return repeater && repeater.GetElementIndex(container) >= 0 ? container : nullptr;
}

winrt::TableViewGroupHeader TableViewGroupHeaderAutomationPeer::GetBoundHeader() const
{
    if (!m_binding.IsItemPeer())
    {
        return GetHeader();
    }

    auto const container = ResolveBoundHeaderInternal();
    if (!container)
    {
        ThrowElementNotAvailable();
    }
    return container.as<winrt::TableViewGroupHeader>();
}

winrt::AutomationPeer TableViewGroupHeaderAutomationPeer::GetForwardingPeer() const
{
    if (!m_binding.IsItemPeer())
    {
        return nullptr;
    }
    return winrt::FrameworkElementAutomationPeer::FromElement(GetBoundHeader());
}

winrt::IInspectable TableViewGroupHeaderAutomationPeer::NavigateCore(winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationNavigationDirection const& direction)
{
    // The framework's tree holds the header's own peer, not this one.
    if (auto const peer = GetForwardingPeer())
    {
        return peer.Navigate(direction);
    }
    return __super::NavigateCore(direction);
}

// A group peer forwards to its bound header's own peer; a header's own peer answers for itself.
#define TABLEVIEW_GROUP_HEADER_PEER_ROUTE(forwarded, selfValue) \
    if (auto const forwardTo = GetForwardingPeer()) { return forwarded; } \
    return selfValue;
#define TABLEVIEW_GROUP_HEADER_PEER_FORWARD(name) TABLEVIEW_GROUP_HEADER_PEER_ROUTE(forwardTo.name(), __super::name##Core())
// For overrides with no public AutomationPeer counterpart.
#define TABLEVIEW_GROUP_HEADER_PEER_FORWARD_CORE(name) \
    TABLEVIEW_GROUP_HEADER_PEER_ROUTE(forwardTo.as<winrt::Microsoft::UI::Xaml::Automation::Peers::IAutomationPeerOverrides>().name##Core(), __super::name##Core())

winrt::IVector<winrt::AutomationPeer> TableViewGroupHeaderAutomationPeer::GetChildrenCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetChildren) }
winrt::Rect TableViewGroupHeaderAutomationPeer::GetBoundingRectangleCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetBoundingRectangle) }
bool TableViewGroupHeaderAutomationPeer::IsOffscreenCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(IsOffscreen) }
bool TableViewGroupHeaderAutomationPeer::IsEnabledCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(IsEnabled) }
bool TableViewGroupHeaderAutomationPeer::HasKeyboardFocusCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(HasKeyboardFocus) }
bool TableViewGroupHeaderAutomationPeer::IsKeyboardFocusableCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(IsKeyboardFocusable) }
winrt::Point TableViewGroupHeaderAutomationPeer::GetClickablePointCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetClickablePoint) }
winrt::hstring TableViewGroupHeaderAutomationPeer::GetAutomationIdCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetAutomationId) }
winrt::hstring TableViewGroupHeaderAutomationPeer::GetHelpTextCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetHelpText) }
winrt::hstring TableViewGroupHeaderAutomationPeer::GetItemStatusCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetItemStatus) }
winrt::hstring TableViewGroupHeaderAutomationPeer::GetItemTypeCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetItemType) }
winrt::hstring TableViewGroupHeaderAutomationPeer::GetFullDescriptionCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetFullDescription) }
winrt::Windows::Foundation::Collections::IIterable<winrt::AutomationPeer> TableViewGroupHeaderAutomationPeer::GetDescribedByCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD_CORE(GetDescribedBy) }
winrt::hstring TableViewGroupHeaderAutomationPeer::GetAccessKeyCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetAccessKey) }
winrt::hstring TableViewGroupHeaderAutomationPeer::GetAcceleratorKeyCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetAcceleratorKey) }
winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationLiveSetting TableViewGroupHeaderAutomationPeer::GetLiveSettingCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetLiveSetting) }
winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationHeadingLevel TableViewGroupHeaderAutomationPeer::GetHeadingLevelCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetHeadingLevel) }
int32_t TableViewGroupHeaderAutomationPeer::GetCultureCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetCulture) }
bool TableViewGroupHeaderAutomationPeer::IsRequiredForFormCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(IsRequiredForForm) }
bool TableViewGroupHeaderAutomationPeer::IsControlElementCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(IsControlElement) }
bool TableViewGroupHeaderAutomationPeer::IsContentElementCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(IsContentElement) }
int32_t TableViewGroupHeaderAutomationPeer::GetPositionInSetCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetPositionInSet) }
int32_t TableViewGroupHeaderAutomationPeer::GetSizeOfSetCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetSizeOfSet) }
winrt::Windows::Foundation::Collections::IVectorView<winrt::AutomationPeer> TableViewGroupHeaderAutomationPeer::GetControlledPeersCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetControlledPeers) }
winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationLandmarkType TableViewGroupHeaderAutomationPeer::GetLandmarkTypeCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetLandmarkType) }
winrt::hstring TableViewGroupHeaderAutomationPeer::GetLocalizedLandmarkTypeCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetLocalizedLandmarkType) }
winrt::Windows::Foundation::Collections::IIterable<winrt::AutomationPeer> TableViewGroupHeaderAutomationPeer::GetFlowsToCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD_CORE(GetFlowsTo) }
winrt::Windows::Foundation::Collections::IIterable<winrt::AutomationPeer> TableViewGroupHeaderAutomationPeer::GetFlowsFromCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD_CORE(GetFlowsFrom) }
bool TableViewGroupHeaderAutomationPeer::IsPeripheralCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(IsPeripheral) }
bool TableViewGroupHeaderAutomationPeer::IsDataValidForFormCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(IsDataValidForForm) }
winrt::IVector<winrt::Microsoft::UI::Xaml::Automation::Peers::AutomationPeerAnnotation> TableViewGroupHeaderAutomationPeer::GetAnnotationsCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetAnnotations) }
bool TableViewGroupHeaderAutomationPeer::IsDialogCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(IsDialog) }
winrt::AutomationControlType TableViewGroupHeaderAutomationPeer::GetAutomationControlTypeCore() { TABLEVIEW_GROUP_HEADER_PEER_ROUTE(forwardTo.GetAutomationControlType(), winrt::AutomationControlType::Group) }
winrt::hstring TableViewGroupHeaderAutomationPeer::GetClassNameCore() { TABLEVIEW_GROUP_HEADER_PEER_ROUTE(forwardTo.GetClassName(), winrt::hstring{ L"TableViewGroupHeader" }) }
winrt::hstring TableViewGroupHeaderAutomationPeer::GetLocalizedControlTypeCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetLocalizedControlType) }
winrt::AutomationPeer TableViewGroupHeaderAutomationPeer::GetLabeledByCore() { TABLEVIEW_GROUP_HEADER_PEER_FORWARD(GetLabeledBy) }

#undef TABLEVIEW_GROUP_HEADER_PEER_FORWARD_CORE
#undef TABLEVIEW_GROUP_HEADER_PEER_FORWARD
#undef TABLEVIEW_GROUP_HEADER_PEER_ROUTE

winrt::IInspectable TableViewGroupHeaderAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    // Unconditional, unlike the old adaptive row peer: this container is only ever a group
    // header, so the pattern never has to be withdrawn. Non-expandable groups are reported as
    // LeafNode by ExpandCollapseState rather than by hiding the pattern, which is what the
    // in-box Expander / NavigationViewItem peers do.
    GetBoundHeader();
    if (patternInterface == winrt::PatternInterface::ExpandCollapse ||
        patternInterface == winrt::PatternInterface::GridItem)
    {
        return *this;
    }

    if (auto const peer = GetForwardingPeer())
    {
        return peer.GetPattern(patternInterface);
    }
    return __super::GetPatternCore(patternInterface);
}

winrt::hstring TableViewGroupHeaderAutomationPeer::GetNameCore()
{
    // Read from the live projection rather than the visual tree, so the name reflects current
    // state instead of whatever was last rendered.
    //
    // ItemCountText already carries its own localized framing (SR_TableViewGroupHeaderCountFormat,
    // "({0})"), so the two parts are joined with the same single space the default template puts
    // between them. Anything richer belongs in a dedicated localized format string rather than
    // being assembled here.
    if (auto const peer = GetForwardingPeer())
    {
        return peer.GetName();
    }
    if (auto const header = GetHeader())
    {
        if (auto const name = winrt::AutomationProperties::GetName(header); !name.empty())
        {
            return name;
        }
        if (auto const label = GetLabeledBy())
        {
            if (auto const name = label.GetName(); !name.empty())
            {
                return name;
            }
        }
        if (auto const info = header.Content().try_as<winrt::TableViewGroupInfo>())
        {
            if (auto const name = GroupInfoToName(info); !name.empty())
            {
                return name;
            }
        }
    }

    return __super::GetNameCore();
}

void TableViewGroupHeaderAutomationPeer::SetFocusCore()
{
    // Base peer focuses as Programmatic, which FocusManager coerces (e.g. to Pointer: no focus
    // visual, and group-expansion focus restore skips it), hence explicit Keyboard.
    if (auto const header = GetBoundHeader())
    {
        header.Focus(winrt::FocusState::Keyboard);
        return;
    }
    __super::SetFocusCore();
}

void TableViewGroupHeaderAutomationPeer::ShowContextMenuCore()
{
    Row(); // Reject a pooled header before the native owner-menu path can run.

    // The base raises ContextRequested on the header itself, which shows the header's own
    // ContextFlyout before any TableView suppression hook can see it.
    if (auto const tableView = GetOwningTableView(); tableView && winrt::get_self<TableView>(tableView)->AreContextMenusSuppressed())
    {
        return;
    }

    if (auto const peer = GetForwardingPeer())
    {
        peer.ShowContextMenu();
        return;
    }
    __super::ShowContextMenuCore();
}

int32_t TableViewGroupHeaderAutomationPeer::GetLevelCore()
{
    if (auto const peer = GetForwardingPeer())
    {
        return peer.GetLevel();
    }

    // An app-set AutomationProperties.Level wins, as in the dxaml peers that compute this.
    if (const auto provided = __super::GetLevelCore(); provided > 0)
    {
        return provided;
    }

    // Grouping is single-level: no synthesized Level (0 = not specified) unless the app sets one.
    return 0;
}

void TableViewGroupHeaderAutomationPeer::RaiseExpandCollapseAutomationEvent(
    winrt::ExpandCollapseState oldState,
    winrt::ExpandCollapseState newState)
{
    if (oldState == newState)
    {
        return;
    }

    if (!winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        return;
    }

    try
    {
        RaisePropertyChangedEvent(
            winrt::ExpandCollapsePatternIdentifiers::ExpandCollapseStateProperty(),
            box_value(oldState),
            box_value(newState));
    }
    catch (...)
    {
        // best-effort: the tree may be tearing down.
    }
}

void TableViewGroupHeaderAutomationPeer::Expand()
{
    SetExpansion(true);
}

void TableViewGroupHeaderAutomationPeer::Collapse()
{
    SetExpansion(false);
}

void TableViewGroupHeaderAutomationPeer::SetExpansion(bool expand)
{
    auto const header = GetBoundHeader();
    if (!header || !header.IsExpandable())
    {
        return;
    }

    // Direction is passed through rather than resolved here into a toggle. The mutation is
    // applied on a later turn, so a guard reading IsExpanded() (the last state pushed to this
    // container) cannot make a toggle directional -- two Expand() calls in one client turn
    // would both pass such a guard and queue two toggles, leaving the group collapsed.
    // ExpandCollapsePattern requires Expand/Collapse to be idempotent.
    //
    // Resolved through the header's stored owner, not an ancestor walk: AT clients routinely
    // hold a provider across a scroll, and a walk from an unparented container finds nothing
    // and silently no-ops.
    if (auto const owner = winrt::get_self<TableViewGroupHeader>(header)->GetOwningTableView())
    {
        winrt::get_self<TableView>(owner)->SetGroupExpansion(header, expand);
    }
}

winrt::ExpandCollapseState TableViewGroupHeaderAutomationPeer::ExpandCollapseState()
{
    if (auto const header = GetBoundHeader())
    {
        if (!header.IsExpandable())
        {
            return winrt::ExpandCollapseState::LeafNode;
        }
        return header.IsExpanded()
            ? winrt::ExpandCollapseState::Expanded
            : winrt::ExpandCollapseState::Collapsed;
    }
    return winrt::ExpandCollapseState::LeafNode;
}

winrt::TableView TableViewGroupHeaderAutomationPeer::GetOwningTableView()
{
    if (auto const header = GetBoundHeader())
    {
        return winrt::get_self<TableViewGroupHeader>(header)->GetOwningTableView();
    }
    return nullptr;
}

// The band occupies one grid row and spans every visible column -- the merged-cell shape that
// TableViewAutomationPeer::GetItem reports for a header row.
int32_t TableViewGroupHeaderAutomationPeer::Row()
{
    if (auto const header = GetBoundHeader())
    {
        if (auto const owner = GetOwningTableView())
        {
            if (auto const repeater = winrt::get_self<TableView>(owner)->GetRowsRepeaterForPeer())
            {
                if (const auto index = repeater.GetElementIndex(header); index >= 0)
                {
                    return index;
                }
            }
        }
    }
    // A recycled header has no row; -1 would read as a real coordinate.
    ThrowElementNotAvailable();
}

int32_t TableViewGroupHeaderAutomationPeer::Column()
{
    GetBoundHeader();
    return 0;
}

int32_t TableViewGroupHeaderAutomationPeer::RowSpan()
{
    GetBoundHeader();
    return 1;
}

int32_t TableViewGroupHeaderAutomationPeer::ColumnSpan()
{
    if (auto const owner = GetOwningTableView())
    {
        if (auto const columns = owner.Columns())
        {
            int32_t count = 0;
            for (auto const& column : columns)
            {
                if (column && column.Visibility() == winrt::Visibility::Visible)
                {
                    ++count;
                }
            }
            return count > 0 ? count : 1;
        }
    }
    return 1;
}

winrt::IRawElementProviderSimple TableViewGroupHeaderAutomationPeer::ContainingGrid()
{
    if (auto const owner = GetOwningTableView())
    {
        if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(owner))
        {
            return ProviderFromPeer(peer);
        }
    }
    return nullptr;
}
