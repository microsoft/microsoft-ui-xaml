// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SelectorBarItem.h"
#include "ItemContainerAutomationPeer.h"
#include "SelectorBarItemAutomationPeer.g.h"

class SelectorBarItemAutomationPeer :
    public winrt::implementation::SelectorBarItemAutomationPeerT<SelectorBarItemAutomationPeer, ItemContainerAutomationPeer>
{
public:
    ForwardRefToBaseReferenceTracker(ItemContainerAutomationPeer)

    SelectorBarItemAutomationPeer(winrt::FrameworkElement const& owner);

    // IAutomationPeerOverrides 
    winrt::hstring GetNameCore() override;
    winrt::hstring GetLocalizedControlTypeCore() override;
    winrt::hstring GetClassNameCore() override;
};
