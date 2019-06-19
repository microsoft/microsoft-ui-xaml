// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TabView.h"
#include "TabViewAutomationPeer.g.h"

class TabViewAutomationPeer :
    public ReferenceTracker<TabViewAutomationPeer, winrt::implementation::TabViewAutomationPeerT>
{

public:
    TabViewAutomationPeer(winrt::TabView const& owner);
    ~TabViewAutomationPeer() {}

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
};

CppWinRTActivatableClassWithBasicFactory(TabViewAutomationPeer);
