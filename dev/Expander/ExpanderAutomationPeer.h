// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Expander.h"
#include "ExpanderAutomationPeer.g.h"

class ExpanderAutomationPeer :
    public ReferenceTracker<ExpanderAutomationPeer, winrt::implementation::ExpanderAutomationPeerT>
{

public:
    ExpanderAutomationPeer(winrt::Expander const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    hstring GetNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    // IExpandCollapseProvider
    winrt::ExpandCollapseState ExpandCollapseState();
    void Collapse();
    void Expand();
    void RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState newState);


private:
    com_ptr<Expander> GetImpl();
};

