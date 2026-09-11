// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewGroupHeader.h"
#include "TableView.h"
#include "TableViewGroupHeaderToggleRequestedEventArgs.h"
#include "TableViewGroupInfo.h"
#include "TableViewGroupHeaderAutomationPeer.h"

TableViewGroupHeader::TableViewGroupHeader()
{
    SetDefaultStyleKey(this);
}

winrt::AutomationPeer TableViewGroupHeader::OnCreateAutomationPeer()
{
    return winrt::make<TableViewGroupHeaderAutomationPeer>(*this);
}

winrt::Size TableViewGroupHeader::MeasureOverride(winrt::Size const& availableSize)
{
    // Same sum TableViewCellsPanel uses for a row, evaluated at the same point in the layout
    // pass, so the band and the rows can never disagree.
    double columnsTotal = 0.0;
    if (auto const owner = GetOwningTableView())
    {
        if (auto const columns = owner.Columns())
        {
            for (auto const& column : columns)
            {
                if (column && column.Visibility() == winrt::Visibility::Visible)
                {
                    columnsTotal += std::max(0.0, column.ActualWidth());
                }
            }
        }
    }

    if (columnsTotal <= 0.0)
    {
        // Before any column has resolved (first pass, or no owner yet) fall back to the natural
        // measure rather than reporting zero, which would collapse the band.
        return __super::MeasureOverride(availableSize);
    }

    const auto width = static_cast<float>(columnsTotal);
    auto const desired = __super::MeasureOverride({ width, availableSize.Height });
    return { width, desired.Height };
}

void TableViewGroupHeader::SetOwningTableViewInternal(winrt::TableView const& owner)
{
    // Weak: the TableView owns this container through the repeater, so a strong ref here would
    // be a cycle. Mirrors TableViewRow::SetOwningTableViewInternal.
    m_owningTableView = owner ? winrt::make_weak(owner) : nullptr;
}

winrt::TableView TableViewGroupHeader::GetOwningTableView() const
{
    return m_owningTableView.get();
}

void TableViewGroupHeader::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    m_isEnabledChangedRevoker.revoke();

    // Keep CommonStates in sync with IsEnabled so Disabled activates when a consumer toggles it
    // at runtime, not only when it happens to be false at template time.
    m_isEnabledChangedRevoker = IsEnabledChanged(
        winrt::auto_revoke,
        [weakThis = get_weak()](auto const&, auto const&)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->UpdateVisualStates(true /* useTransitions */);
            }
        });

    UpdateVisualStates(false /* useTransitions */);
}

void TableViewGroupHeader::OnContentChanged(winrt::IInspectable const& oldContent, winrt::IInspectable const& newContent)
{
    __super::OnContentChanged(oldContent, newContent);

    // PrepareGroupHeaderElement assigns the projection, then pushes expansion via the DPs. A
    // projection swapped in later must not be left reporting stale expansion, so seed it here.
    SyncExpansionToContent();
}

void TableViewGroupHeader::SyncExpansionToContent()
{
    if (auto const info = Content().try_as<winrt::TableViewGroupInfo>())
    {
        winrt::get_self<TableViewGroupInfo>(info)->SetExpansionInternal(IsExpandable(), IsExpanded());
    }
}

void TableViewGroupHeader::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto const property = args.Property();

    if (property == s_IsExpandedProperty || property == s_IsExpandableProperty)
    {
        // The DPs are authoritative: they drive the VisualStates and are mirrored onto the
        // projection here, so there is a single write path for expansion state.
        SyncExpansionToContent();
        UpdateVisualStates(true /* useTransitions */);
    }

}

void TableViewGroupHeader::RaiseExpandCollapseStateChanged(winrt::ExpandCollapseState oldState, winrt::ExpandCollapseState newState)
{
    // Route through the peer the client is connected to. FromElement returns the already-created
    // peer; CreatePeerForElement is the fallback because a container freshly prepared out of the
    // recycle pool may not have had its peer created yet, and XAML caches the peer per element so
    // this returns that same instance rather than a disconnected one.
    auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this);
    if (!peer)
    {
        peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(*this);
    }

    if (auto const headerPeer = peer ? peer.try_as<winrt::TableViewGroupHeaderAutomationPeer>() : nullptr)
    {
        winrt::get_self<TableViewGroupHeaderAutomationPeer>(headerPeer)->RaiseExpandCollapseAutomationEvent(
            oldState, newState);
    }
}

void TableViewGroupHeader::UpdateVisualStates(bool useTransitions)
{
    // Glyph, band chrome and chevron visibility are all declared in the ControlTemplate and
    // selected by state -- no visual property is assigned from here.
    winrt::VisualStateManager::GoToState(
        *this,
        IsExpandable() ? L"Expandable" : L"NotExpandable",
        useTransitions);

    winrt::VisualStateManager::GoToState(
        *this,
        IsExpanded() ? L"Expanded" : L"Collapsed",
        useTransitions);

    winrt::VisualStateManager::GoToState(
        *this,
        !IsEnabled() ? L"Disabled" : (m_isPressed ? L"Pressed" : (m_isPointerOver ? L"PointerOver" : L"Normal")),
        useTransitions);
}

void TableViewGroupHeader::RequestToggle()
{
    if (!IsExpandable())
    {
        return;
    }

    auto const info = Content().try_as<winrt::TableViewGroupInfo>();
    auto const args = winrt::make<TableViewGroupHeaderToggleRequestedEventArgs>(info ? info.Key() : nullptr);

    // Raised inline, deliberately. The owner resolves the target group synchronously and
    // defers only the structural reshape, so nothing is destroyed while XAML is still
    // unwinding pointer dispatch.
    //
    // Posting HERE is what created the bug it appeared to prevent: by the time a queued raise
    // ran, a scroll could have recycled this header onto another group, so the toggle hit
    // the wrong one -- or the header had left the viewport and it was dropped. Resolution has to
    // happen while the gesture's row index is still current, which means the raise cannot be
    // deferred; only the reshape can.
    //
    // Strong self across the raise keeps THIS object alive if a handler releases it mid-invoke.
    // It does not protect XAML's in-flight pointer dispatch -- that safety comes from the owner
    // deferring the reshape, and from RequestToggle being the last statement in
    // OnPointerReleased so nothing here touches a member after the raise.
    auto const strongThis = get_strong();
    auto const self = strongThis.as<winrt::TableViewGroupHeader>();

    m_toggleRequestedEventSource(self, args);
}

void TableViewGroupHeader::RequestExpansion(bool expand)
{
    if (!IsExpandable())
    {
        return;
    }

    // Mirror the ExpandCollapse peer: pass the direction through to the owner unresolved (the
    // mutation is idempotent and applied on a later turn) and resolve through the stored owner
    // rather than an ancestor walk, so a header that has been recycled/unparented still works.
    if (auto const owner = GetOwningTableView())
    {
        auto const self = get_strong().as<winrt::TableViewGroupHeader>();
        winrt::get_self<TableView>(owner)->SetGroupExpansion(self, expand);
    }
}

void TableViewGroupHeader::OnKeyDown(winrt::KeyRoutedEventArgs const& args)
{
    if (!args.Handled() && IsExpandable())
    {
        const auto key = args.Key();
        const bool isRtl = FlowDirection() == winrt::FlowDirection::RightToLeft;

        switch (key)
        {
        case winrt::VirtualKey::Enter:
        case winrt::VirtualKey::Space:
            RequestToggle();
            args.Handled(true);
            break;

        case winrt::VirtualKey::Right:
            // Right expands in LTR, collapses in RTL -- the TreeViewItem / Expander convention.
            RequestExpansion(!isRtl);
            args.Handled(true);
            break;

        case winrt::VirtualKey::Left:
            RequestExpansion(isRtl);
            args.Handled(true);
            break;

        default:
            break;
        }
    }

    __super::OnKeyDown(args);
}

void TableViewGroupHeader::OnPointerEntered(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerEntered(args);
    m_isPointerOver = true;
    UpdateVisualStates(true /* useTransitions */);
}

void TableViewGroupHeader::OnPointerExited(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerExited(args);
    m_isPointerOver = false;
    m_isPressed = false;
    UpdateVisualStates(true /* useTransitions */);
}

void TableViewGroupHeader::OnPointerPressed(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerPressed(args);

    // Left-button only, matching TableViewRow. Pointer events fire for secondary and middle
    // buttons too, and the matching release would reach RequestToggle -- so without this a
    // right-click on the band would toggle the group and swallow the context-menu gesture.
    // Despite the name, IsLeftButtonPressed covers the primary action regardless of device.
    if (!args.GetCurrentPoint(*this).Properties().IsLeftButtonPressed())
    {
        return;
    }

    m_isPressed = true;

    // The band owns the gesture: mark it handled so a press on a group header never reaches the
    // row's selection handling.
    if (IsExpandable())
    {
        args.Handled(true);
    }

    UpdateVisualStates(true /* useTransitions */);
}

void TableViewGroupHeader::OnPointerReleased(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerReleased(args);

    const bool wasPressed = m_isPressed;
    m_isPressed = false;
    UpdateVisualStates(true /* useTransitions */);

    // Toggle on release-inside, the standard click semantic: a press that drags off the band
    // does not activate.
    if (wasPressed && m_isPointerOver && IsExpandable())
    {
        args.Handled(true);
        RequestToggle();
    }
}

void TableViewGroupHeader::OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerCaptureLost(args);
    m_isPressed = false;
    m_isPointerOver = false;
    UpdateVisualStates(true /* useTransitions */);
}
