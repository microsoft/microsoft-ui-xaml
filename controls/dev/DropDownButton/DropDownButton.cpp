// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "DropDownButton.h"
#include "DropDownButtonAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

#include "DropDownButton.properties.cpp"

DropDownButton::DropDownButton()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_DropDownButton);

    SetDefaultStyleKey(this);
}

void DropDownButton::OnApplyTemplate()
{
    m_flyoutPropertyChangedRevoker = RegisterPropertyChanged(*this, winrt::Button::FlyoutProperty(), { this, &DropDownButton::OnFlyoutPropertyChanged });

    RegisterFlyoutEvents();
}

void DropDownButton::RegisterFlyoutEvents()
{
    m_flyoutOpenedRevoker.revoke();
    m_flyoutClosedRevoker.revoke();

    if (Flyout())
    {
        m_flyoutOpenedRevoker = Flyout().Opened(winrt::auto_revoke, { this, &DropDownButton::OnFlyoutOpened });
        m_flyoutClosedRevoker = Flyout().Closed(winrt::auto_revoke, { this, &DropDownButton::OnFlyoutClosed });
    }
    UpdateFlyoutState();
}

void DropDownButton::UpdateFlyoutState()
{
    auto const flyout = Flyout();
    const bool isOpen = flyout && flyout.IsOpen() && flyout.Target() == *this;
    if (m_isFlyoutOpen != isOpen)
    {
        m_isFlyoutOpen = isOpen;
        SharedHelpers::RaiseAutomationPropertyChangedEvent(*this,
            isOpen ? winrt::ExpandCollapseState::Collapsed : winrt::ExpandCollapseState::Expanded,
            isOpen ? winrt::ExpandCollapseState::Expanded : winrt::ExpandCollapseState::Collapsed);
    }
}

bool DropDownButton::IsFlyoutOpen()
{
    return m_isFlyoutOpen;
};

void DropDownButton::OpenFlyout()
{
    if (auto flyout = Flyout())
    {
        flyout.ShowAt(*this);
    }
}

void DropDownButton::CloseFlyout()
{
    if (auto flyout = Flyout())
    {
        flyout.Hide();
    }
}

void DropDownButton::OnFlyoutPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    RegisterFlyoutEvents();
}

void DropDownButton::OnFlyoutOpened(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    UpdateFlyoutState();
}

void DropDownButton::OnFlyoutClosed(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    UpdateFlyoutState();
}

winrt::AutomationPeer DropDownButton::OnCreateAutomationPeer()
{
    return winrt::make<DropDownButtonAutomationPeer>(*this);
}
