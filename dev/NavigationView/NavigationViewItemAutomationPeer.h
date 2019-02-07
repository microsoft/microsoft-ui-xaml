// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItem.h"

#include "NavigationViewItemAutomationPeer.g.h"

class NavigationViewItemAutomationPeer :
    public ReferenceTracker<
        NavigationViewItemAutomationPeer,
        winrt::implementation::NavigationViewItemAutomationPeerT,
        winrt::IInvokeProvider>
{
public:
    NavigationViewItemAutomationPeer(winrt::NavigationViewItem const& owner);

    // IAutomationPeerOverrides 
    winrt::hstring GetNameCore();
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);

    // IAutomationPeerOverrides3
    int32_t GetPositionInSetCore();
    int32_t GetSizeOfSetCore();

    // IInvokeProvider
    void Invoke();

private:

    enum class AutomationOutput
    {
        Position,
        Size,
    };

    winrt::NavigationView GetParentNavigationView();
    bool IsOnTopNavigation();
    bool IsOnTopNavigationOverflow();
    bool IsSettingsItem();
    NavigationViewListPosition GetNavigationViewListPosition();
    int32_t GetNavigationViewItemCountInPrimaryList();
    int32_t GetNavigationViewItemCountInTopNav();
    int32_t GetPositionOrSetCountInLeftNavHelper(AutomationOutput automationOutput);
    int32_t GetPositionOrSetCountInTopNavHelper(winrt::IVector<winrt::IInspectable> navigationViewElements, AutomationOutput automationOutput);
};
