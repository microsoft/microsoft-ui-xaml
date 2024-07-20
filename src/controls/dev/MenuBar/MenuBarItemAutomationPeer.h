// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MenuBarItem.h"
#include "MenuBarItemAutomationPeer.g.h"

class MenuBarItemAutomationPeer :
    public ReferenceTracker<MenuBarItemAutomationPeer, winrt::implementation::MenuBarItemAutomationPeerT>
{

public:
    MenuBarItemAutomationPeer(winrt::MenuBarItem const& owner);

    // IAutomationPeerOverrides 
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::hstring GetNameCore();
    winrt::hstring GetClassNameCore();

    // IInvokeProvider
    void Invoke();

    // IExpandCollapseProvider
    winrt::ExpandCollapseState ExpandCollapseState();
    void Collapse();
    void Expand();

};
