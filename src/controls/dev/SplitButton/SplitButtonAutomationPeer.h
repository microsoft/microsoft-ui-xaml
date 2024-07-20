// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SplitButton.h"
#include "SplitButtonAutomationPeer.g.h"

class SplitButtonAutomationPeer :
    public ReferenceTracker<SplitButtonAutomationPeer, winrt::implementation::SplitButtonAutomationPeerT>
{

public:
    SplitButtonAutomationPeer(winrt::SplitButton const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    // IExpandCollapseProvider
    winrt::ExpandCollapseState ExpandCollapseState();
    void Expand();
    void Collapse();

    // IInvokeProvider
    void Invoke();

private:
    com_ptr<SplitButton> GetImpl();
};

