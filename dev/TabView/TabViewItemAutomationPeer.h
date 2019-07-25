// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TabViewItem.h"
#include "TabViewItemAutomationPeer.g.h"

class TabViewItemAutomationPeer :
    public ReferenceTracker<TabViewItemAutomationPeer, winrt::implementation::TabViewItemAutomationPeerT>
{

public:
    TabViewItemAutomationPeer(winrt::TabViewItem const& owner);

    // IAutomationPeerOverrides
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
};

CppWinRTActivatableClassWithBasicFactory(TabViewItemAutomationPeer);
