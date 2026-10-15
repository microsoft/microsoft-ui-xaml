// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewColumnHeader.h"
#include "TableView.h"
#include "TableViewContextMenu.h"
#include "TableViewColumnHeaderAutomationPeer.h"

static constexpr std::wstring_view s_ContentPresenterPartName{ L"PART_ContentPresenter"sv };
static constexpr std::wstring_view s_GridLinePartName{ L"PART_GridLine"sv };
static constexpr std::wstring_view s_SortIndicatorHostPartName{ L"PART_SortIndicatorHost"sv };
static constexpr std::wstring_view s_ResizeGripperHostPartName{ L"PART_ResizeGripperHost"sv };
static constexpr std::wstring_view s_SortIndicatorName{ L"TableViewSortIndicator"sv };

TableViewColumnHeader::TableViewColumnHeader()
{
    SetDefaultStyleKey(this);
}

void TableViewColumnHeader::SetOwnerInternal(winrt::TableView const& table, winrt::TableViewColumn const& column)
{
    m_table = table ? winrt::make_weak(table) : nullptr;
    m_column = column ? winrt::make_weak(column) : nullptr;
}

winrt::AutomationPeer TableViewColumnHeader::OnCreateAutomationPeer()
{
    auto const table = m_table.get();
    auto const column = m_column.get();
    if (!table || !column)
    {
        return __super::OnCreateAutomationPeer();
    }

    winrt::AutomationPeer const peer = winrt::make<TableViewColumnHeaderAutomationPeer>(*this, table, column);
    m_automationPeer = winrt::make_weak(peer);
    return peer;
}

void TableViewColumnHeader::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    m_isEnabledChangedRevoker.revoke();
    m_templateRootContextRequestedRevoker.revoke();

    if (auto const oldGripperHost = m_resizeGripperHost.get())
    {
        oldGripperHost.Child(nullptr);
    }
    if (auto const oldIndicatorHost = m_sortIndicatorHost.get())
    {
        oldIndicatorHost.Child(nullptr);
    }

    m_contentPresenter.set(GetTemplateChild(hstring{ s_ContentPresenterPartName }).try_as<winrt::ContentPresenter>());
    m_gridLine.set(GetTemplateChild(hstring{ s_GridLinePartName }).try_as<winrt::Border>());
    m_sortIndicatorHost.set(GetTemplateChild(hstring{ s_SortIndicatorHostPartName }).try_as<winrt::Border>());
    m_resizeGripperHost.set(GetTemplateChild(hstring{ s_ResizeGripperHostPartName }).try_as<winrt::Border>());

    m_templateContentMargin = {};
    if (auto const presenter = m_contentPresenter.get())
    {
        m_templateContentMargin = presenter.Margin();
    }
    UpdateContentReserve();
    HostSortIndicator();

    if (auto const gripperHost = m_resizeGripperHost.get())
    {
        if (auto const gripper = m_resizeGripper.get())
        {
            gripperHost.Child(gripper);
        }
    }

    // Keeps a Style-set ContextFlyout closed while a cell is editing.
    m_templateRootContextRequestedRevoker = TableViewDetails::HookTemplateRootContextRequested(*this,
        [weakThis = get_weak()](auto&&, winrt::ContextRequestedEventArgs const& args)
        {
            if (auto strongThis = weakThis.get())
            {
                if (auto const owner = strongThis->GetOwningTableView())
                {
                    winrt::get_self<TableView>(owner)->SuppressContextRequestWhileEditing(args);
                }
            }
        });

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

    if (auto const owner = GetOwningTableView())
    {
        winrt::get_self<TableView>(owner)->OnColumnHeaderTemplateApplied(*this);
    }
}

void TableViewColumnHeader::SetSortableInternal(bool sortable, double indicatorReserve)
{
    m_isSortable = sortable;
    m_indicatorReserve = sortable ? indicatorReserve : 0.0;
    UpdateContentReserve();
    HostSortIndicator();
    UpdateVisualStates(false /* useTransitions */);
}

void TableViewColumnHeader::SetSortDirectionInternal(winrt::SortDirection direction)
{
    m_sortDirection = direction;
    if (auto const indicator = m_sortIndicator.get())
    {
        indicator.Direction(TableView::ToSortIndicatorDirection(direction));
    }
    UpdateVisualStates(true /* useTransitions */);
}

void TableViewColumnHeader::SetResizeGripperInternal(winrt::ResizeGripper const& gripper)
{
    if (auto const host = m_resizeGripperHost.get())
    {
        host.Child(gripper);
    }
    m_resizeGripper.set(gripper);
}

winrt::ResizeGripper TableViewColumnHeader::GetResizeGripperInternal() const
{
    auto const gripper = m_resizeGripper.get();
    auto const host = m_resizeGripperHost.get();
    if (gripper && host && host.Child() == gripper)
    {
        return gripper;
    }
    return nullptr;
}

void TableViewColumnHeader::UpdateContentReserve()
{
    if (auto const presenter = m_contentPresenter.get())
    {
        auto margin = m_templateContentMargin;
        if (m_isSortable)
        {
            margin.Right += m_indicatorReserve;
        }
        if (presenter.Margin() != margin)
        {
            presenter.Margin(margin);
        }
    }
}

void TableViewColumnHeader::HostSortIndicator()
{
    auto const host = m_sortIndicatorHost.get();
    if (!host)
    {
        return;
    }

    if (!m_isSortable)
    {
        host.Child(nullptr);
        return;
    }

    auto indicator = m_sortIndicator.get();
    if (!indicator)
    {
        indicator = winrt::SortIndicator{};
        indicator.Name(winrt::hstring{ s_SortIndicatorName });
        indicator.VerticalAlignment(winrt::VerticalAlignment::Center);
        // The header peer already reports the sort state.
        winrt::AutomationProperties::SetAccessibilityView(indicator, winrt::AccessibilityView::Raw);
        m_sortIndicator.set(indicator);
    }
    indicator.Direction(TableView::ToSortIndicatorDirection(m_sortDirection));

    if (host.Child() != indicator)
    {
        host.Child(indicator);
    }
}

void TableViewColumnHeader::SyncSortIndicatorForeground(bool highlighted)
{
    auto const indicator = m_sortIndicator.get();
    if (!indicator)
    {
        return;
    }

    // The glyph's style sets Foreground, so it cannot inherit; under a High Contrast highlight it
    // must follow the text the template recolored, or it vanishes on the highlight fill.
    auto const presenter = m_contentPresenter.get();
    if (highlighted && presenter)
    {
        indicator.Foreground(presenter.Foreground());
        indicator.HighContrastAdjustment(winrt::ElementHighContrastAdjustment::None);
    }
    else
    {
        indicator.ClearValue(winrt::Control::ForegroundProperty());
        indicator.ClearValue(winrt::UIElement::HighContrastAdjustmentProperty());
    }
}

void TableViewColumnHeader::UpdateVisualStates(bool useTransitions)
{
    // Hover and press advertise click-to-sort, so a header that cannot sort stays Normal.
    const wchar_t* commonState = L"Normal";
    if (!IsEnabled())
    {
        commonState = L"Disabled";
    }
    else if (m_isSortable && m_isPressed)
    {
        commonState = L"Pressed";
    }
    else if (m_isSortable && m_isPointerOver)
    {
        commonState = L"PointerOver";
    }
    winrt::VisualStateManager::GoToState(*this, commonState, useTransitions);

    const wchar_t* highContrastState = L"HighContrastNormal";
    if (commonState != std::wstring_view{ L"Normal" } && commonState != std::wstring_view{ L"Disabled" })
    {
        if (auto const owner = GetOwningTableView(); owner && winrt::get_self<TableView>(owner)->IsHighContrast())
        {
            highContrastState = m_isPressed ? L"HighContrastPressed" : L"HighContrastPointerOver";
        }
    }
    winrt::VisualStateManager::GoToState(*this, highContrastState, useTransitions);
    SyncSortIndicatorForeground(highContrastState != std::wstring_view{ L"HighContrastNormal" });

    const wchar_t* sortState = L"Unsorted";
    if (m_sortDirection == winrt::SortDirection::Ascending)
    {
        sortState = L"SortAscending";
    }
    else if (m_sortDirection == winrt::SortDirection::Descending)
    {
        sortState = L"SortDescending";
    }
    winrt::VisualStateManager::GoToState(*this, sortState, useTransitions);
}

void TableViewColumnHeader::OnPointerEntered(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerEntered(args);
    m_isPointerOver = true;
    UpdateVisualStates(true /* useTransitions */);
}

void TableViewColumnHeader::OnPointerExited(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerExited(args);
    m_isPointerOver = false;
    m_isPressed = false;
    UpdateVisualStates(true /* useTransitions */);
}

void TableViewColumnHeader::OnPointerPressed(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerPressed(args);
    m_isPressed = true;
    UpdateVisualStates(true /* useTransitions */);
}

void TableViewColumnHeader::OnPointerReleased(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerReleased(args);
    m_isPressed = false;
    UpdateVisualStates(true /* useTransitions */);
}

void TableViewColumnHeader::OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerCaptureLost(args);
    ResetPointerState(args);
}

void TableViewColumnHeader::OnPointerCanceled(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerCanceled(args);
    ResetPointerState(args);
}

void TableViewColumnHeader::ResetPointerState(winrt::PointerRoutedEventArgs const& args)
{
    // The gripper captures the pointer for its drag; that capture ending bubbles here while the
    // pointer is still over the header, so it must not reset the hover state.
    if (IsInsideResizeGripper(args.OriginalSource()))
    {
        return;
    }

    m_isPointerOver = false;
    m_isPressed = false;
    UpdateVisualStates(true /* useTransitions */);
}

bool TableViewColumnHeader::IsInsideResizeGripper(winrt::IInspectable const& source)
{
    winrt::DependencyObject const self = *this;
    for (auto current = source.try_as<winrt::DependencyObject>();
         current && current != self;
         current = winrt::VisualTreeHelper::GetParent(current))
    {
        if (current.try_as<winrt::ResizeGripper>())
        {
            return true;
        }
    }
    return false;
}
