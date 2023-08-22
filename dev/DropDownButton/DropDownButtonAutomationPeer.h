// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DropDownButton.h"
#include "DropDownButtonAutomationPeer.g.h"

class DropDownButtonAutomationPeer :
    public ReferenceTracker<DropDownButtonAutomationPeer, winrt::implementation::DropDownButtonAutomationPeerT>
{

public:
    DropDownButtonAutomationPeer(winrt::DropDownButton const& owner);
    
    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();

    // IExpandCollapseProvider
    winrt::ExpandCollapseState ExpandCollapseState();
    void Expand();
    void Collapse();

private:
    com_ptr<DropDownButton> GetImpl();
};
