// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ToggleSplitButton.h"
#include "ToggleSplitButtonAutomationPeer.g.h"

class ToggleSplitButtonAutomationPeer :
    public ReferenceTracker<ToggleSplitButtonAutomationPeer, winrt::implementation::ToggleSplitButtonAutomationPeerT>
{

public:
    ToggleSplitButtonAutomationPeer(winrt::ToggleSplitButton const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    // IExpandCollapseProvider
    winrt::ExpandCollapseState ExpandCollapseState();
    void Expand();
    void Collapse();

    // IToggleProvider
    winrt::ToggleState ToggleState();
    void Toggle();

private:
    com_ptr<ToggleSplitButton> GetImpl();
};
