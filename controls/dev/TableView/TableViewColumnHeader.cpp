// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewColumnHeader.h"
#include "TableView.h"
#include "TableViewContextMenu.h"
#include "TableViewColumnHeaderAutomationPeer.h"
#include "TableViewAutomationHelpers.h"

static constexpr std::wstring_view s_ContentPresenterPartName{ L"PART_ContentPresenter"sv };
static constexpr std::wstring_view s_GridLinePartName{ L"PART_GridLine"sv };
static constexpr std::wstring_view s_SortIndicatorHostPartName{ L"PART_SortIndicatorHost"sv };
static constexpr std::wstring_view s_ResizeGripperHostPartName{ L"PART_ResizeGripperHost"sv };
static constexpr std::wstring_view s_SortIndicatorName{ L"TableViewSortIndicator"sv };
static constexpr std::wstring_view s_FlyoutButtonPartName{ L"PART_HeaderFlyoutButton"sv };

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
    ReleaseFlyoutInternal();

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
    m_flyoutButton.set(GetTemplateChild(hstring{ s_FlyoutButtonPartName }).try_as<winrt::DropDownButton>());

    m_templateContentMargin = {};
    if (auto const presenter = m_contentPresenter.get())
    {
        m_templateContentMargin = presenter.Margin();
    }
    UpdateContentReserve();
    HostSortIndicator();
    if (auto const button = m_flyoutButton.get())
    {
        m_flyoutPointerEnteredRevoker = button.PointerEntered(winrt::auto_revoke,
            [weakThis = get_weak()](auto&&, auto&&)
            {
                if (auto self = weakThis.get())
                {
                    self->m_isFlyoutPointerOver = true;
                    self->UpdateVisualStates(true);
                }
            });
        m_flyoutPointerExitedRevoker = button.PointerExited(winrt::auto_revoke,
            [weakThis = get_weak()](auto&&, auto&&)
            {
                if (auto self = weakThis.get())
                {
                    self->m_isFlyoutPointerOver = false;
                    self->UpdateVisualStates(true);
                }
            });
        RefreshFlyoutInternal();
    }
    m_flyoutUnloadedRevoker = Unloaded(winrt::auto_revoke,
        [weakThis = get_weak()](auto&&, auto&&)
        {
            if (auto self = weakThis.get())
            {
                if (auto const button = self->m_flyoutButton.get())
                {
                    if (auto const flyout = button.Flyout();
                        flyout && flyout.IsOpen() && flyout.Target() == button)
                    {
                        flyout.Hide();
                    }
                }
            }
        });

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
                strongThis->RefreshFlyoutInternal();
                strongThis->UpdateVisualStates(true /* useTransitions */);
            }
        });

    UpdateVisualStates(false /* useTransitions */);

    if (auto const owner = GetOwningTableView())
    {
        winrt::get_self<TableView>(owner)->OnColumnHeaderTemplateApplied(*this);
    }
}

void TableViewColumnHeader::ReleaseFlyoutInternal()
{
    m_flyoutPointerEnteredRevoker.revoke();
    m_flyoutPointerExitedRevoker.revoke();
    m_flyoutOpenedRevoker.revoke();
    m_flyoutClosedRevoker.revoke();
    m_flyoutUnloadedRevoker.revoke();
    m_isFlyoutOpen = false;
    m_isFlyoutPointerOver = false;
    auto const button = m_flyoutButton.get();
    m_flyoutButton.set(nullptr);
    if (button)
    {
        auto const flyout = button.Flyout();
        button.Flyout(nullptr);
        if (flyout && flyout.IsOpen() && flyout.Target() == button)
        {
            flyout.Hide();
        }
    }
}

void TableViewColumnHeader::RefreshFlyoutInternal()
{
    auto const button = m_flyoutButton.get();
    if (!button)
    {
        return;
    }
    auto const column = Column();
    auto const owner = GetOwningTableView();
    const bool available = IsEnabled() && owner &&
        !winrt::get_self<TableView>(owner)->AreContextMenusSuppressed();
    auto const oldFlyout = button.Flyout();
    auto flyout = available && column ? column.HeaderFlyout() : nullptr;
    if (oldFlyout != flyout)
    {
        m_flyoutOpenedRevoker.revoke();
        m_flyoutClosedRevoker.revoke();
        button.Flyout(nullptr);
        m_isFlyoutOpen = false;
        winrt::VisualStateManager::GoToState(button, L"FlyoutClosed", false);
        if (oldFlyout && oldFlyout.IsOpen() && oldFlyout.Target() == button)
        {
            oldFlyout.Hide();
        }
        // Closing invokes app code, which can replace this header or update its flyout.
        if (m_flyoutButton.get() != button || Column() != column || GetOwningTableView() != owner ||
            button.Flyout())
        {
            return;
        }
        flyout = IsEnabled() && owner && column &&
            !winrt::get_self<TableView>(owner)->AreContextMenusSuppressed() ? column.HeaderFlyout() : nullptr;
        button.Flyout(flyout);
        if (flyout)
        {
            m_flyoutOpenedRevoker = flyout.Opened(winrt::auto_revoke,
                [weakThis = get_weak()](auto&& sender, auto&&)
                {
                    if (auto self = weakThis.get())
                    {
                        auto const button = self->m_flyoutButton.get();
                        self->m_isFlyoutOpen = button && sender.template as<winrt::FlyoutBase>().Target() == button;
                        if (self->m_isFlyoutOpen)
                        {
                            winrt::VisualStateManager::GoToState(button, L"FlyoutOpen", false);
                            self->UpdateVisualStates(true);
                        }
                    }
                });
            m_flyoutClosedRevoker = flyout.Closed(winrt::auto_revoke,
                [weakThis = get_weak()](auto&&, auto&&)
                {
                    if (auto self = weakThis.get())
                    {
                        self->m_isFlyoutOpen = false;
                        if (auto const button = self->m_flyoutButton.get())
                        {
                            winrt::VisualStateManager::GoToState(button, L"FlyoutClosed", false);
                        }
                        self->UpdateVisualStates(true);
                    }
                });
        }
    }
    button.Visibility(column && column.HeaderFlyout() ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    button.IsEnabled(IsEnabled() && owner && !winrt::get_self<TableView>(owner)->AreContextMenusSuppressed());
    if (column && column.HeaderFlyout())
    {
        auto headerName = winrt::AutomationProperties::GetName(*this);
        if (headerName.empty())
        {
            if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(*this))
            {
                headerName = peer.GetName();
            }
        }
        const auto name = headerName.empty()
            ? LocalizedOrFallbackForTableViewAutomation(SR_TableViewHeaderOptionsGenericName, L"Column options")
            : StringUtil::FormatString(LocalizedOrFallbackForTableViewAutomation(
                SR_TableViewHeaderOptionsName, L"Options for %1!s!"), headerName.c_str());
        winrt::AutomationProperties::SetName(button, name);
        winrt::ToolTipService::SetToolTip(button, winrt::box_value(name));
    }
    UpdateVisualStates(false);
}

void TableViewColumnHeader::OnKeyDown(winrt::KeyRoutedEventArgs const& args)
{
    auto const button = m_flyoutButton.get();
    const auto isKeyDown = [](winrt::VirtualKey key)
    {
        return (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(key) &
            winrt::CoreVirtualKeyStates::Down) != winrt::CoreVirtualKeyStates::None;
    };
    const bool alt = isKeyDown(winrt::VirtualKey::Menu);
    if (!args.Handled() &&
        (args.OriginalSource().try_as<winrt::DependencyObject>() == *this ||
            IsInsideFlyoutButton(args.OriginalSource())) &&
        button && button.IsEnabled() && button.Visibility() == winrt::Visibility::Visible &&
        !isKeyDown(winrt::VirtualKey::Control) && !isKeyDown(winrt::VirtualKey::Shift) &&
        ((args.Key() == winrt::VirtualKey::F4 && !alt) || (args.Key() == winrt::VirtualKey::Down && alt)))
    {
        if (auto const flyout = button.Flyout())
        {
            button.Focus(winrt::FocusState::Keyboard);
            flyout.ShowAt(button);
            args.Handled(true);
        }
    }
    __super::OnKeyDown(args);
}

bool TableViewColumnHeader::IsInsideFlyoutButton(winrt::IInspectable const& source) const
{
    auto const button = m_flyoutButton.get();
    for (auto current = source.try_as<winrt::DependencyObject>(); current && current != *this;
        current = winrt::VisualTreeHelper::GetParent(current))
    {
        if (button && current == button)
        {
            return true;
        }
    }
    return false;
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
            if (auto const host = m_sortIndicatorHost.get())
            {
                const auto padding = host.Padding();
                margin.Right += padding.Left + padding.Right;
            }
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

    // The indicator's style sets Foreground, so bind it to the header text instead of inheriting.
    // Keep the binding live across style, theme, disabled, and High Contrast changes.
    auto const presenter = m_contentPresenter.get();
    if (presenter)
    {
        // Rebinding on every state change would rebuild an identical expression.
        auto const bound = indicator.GetBindingExpression(winrt::Control::ForegroundProperty());
        if (!bound || bound.ParentBinding().Source() != presenter)
        {
            winrt::Binding binding;
            binding.Source(presenter);
            binding.Path(winrt::PropertyPath(L"Foreground"));
            indicator.SetBinding(winrt::Control::ForegroundProperty(), binding);
        }
    }
    else
    {
        indicator.ClearValue(winrt::Control::ForegroundProperty());
    }

    if (highlighted)
    {
        indicator.HighContrastAdjustment(winrt::ElementHighContrastAdjustment::None);
    }
    else
    {
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
    else if (m_isSortable && m_isPressed && !m_isFlyoutPointerOver && !m_isFlyoutOpen)
    {
        commonState = L"Pressed";
    }
    else if (m_isSortable && m_isPointerOver && !m_isFlyoutPointerOver && !m_isFlyoutOpen)
    {
        commonState = L"PointerOver";
    }

    const wchar_t* highContrastState = L"HighContrastNormal";
    if (commonState != std::wstring_view{ L"Normal" } && commonState != std::wstring_view{ L"Disabled" })
    {
        if (auto const owner = GetOwningTableView(); owner && winrt::get_self<TableView>(owner)->IsHighContrast())
        {
            highContrastState = m_isPressed ? L"HighContrastPressed" : L"HighContrastPointerOver";
        }
    }

    // Both groups set the text Foreground, and leaving a state clears it whichever group set it,
    // so the group that sets it must transition last.
    if (IsEnabled())
    {
        winrt::VisualStateManager::GoToState(*this, commonState, useTransitions);
        winrt::VisualStateManager::GoToState(*this, highContrastState, useTransitions);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, highContrastState, useTransitions);
        winrt::VisualStateManager::GoToState(*this, commonState, useTransitions);
    }
    const bool highlighted = highContrastState != std::wstring_view{ L"HighContrastNormal" };
    SyncSortIndicatorForeground(highlighted);

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
    m_isPressed = !IsInsideFlyoutButton(args.OriginalSource());
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
