// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "DropDownButton.g.h"

class DropDownButton :
    public ReferenceTracker<DropDownButton, winrt::implementation::DropDownButtonT>
{

public:
    DropDownButton();

    // IFrameworkElement
    void OnApplyTemplate();
    
    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    // Internal -- for DropDownButtonAutomationPeer.
    bool IsFlyoutOpen();
    void OpenFlyout();
    void CloseFlyout();

private:
    void RegisterFlyoutEvents();

    void OnFlyoutPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void OnFlyoutOpened(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnFlyoutClosed(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    bool m_isFlyoutOpen{ false };

    PropertyChanged_revoker m_flyoutPropertyChangedRevoker{};
    winrt::FlyoutBase::Opened_revoker m_flyoutOpenedRevoker{};
    winrt::FlyoutBase::Closed_revoker m_flyoutClosedRevoker{};
};

