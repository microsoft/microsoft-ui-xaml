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
    m_isFlyoutOpen = true;
    SharedHelpers::RaiseAutomationPropertyChangedEvent(*this, winrt::ExpandCollapseState::Collapsed, winrt::ExpandCollapseState::Expanded);
}

void DropDownButton::OnFlyoutClosed(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    m_isFlyoutOpen = false;
    SharedHelpers::RaiseAutomationPropertyChangedEvent(*this, winrt::ExpandCollapseState::Expanded, winrt::ExpandCollapseState::Collapsed);
}

winrt::AutomationPeer DropDownButton::OnCreateAutomationPeer()
{
    return winrt::make<DropDownButtonAutomationPeer>(*this);
}
