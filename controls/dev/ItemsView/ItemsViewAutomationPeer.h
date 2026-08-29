// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsView.h"
#include "ItemsViewAutomationPeer.g.h"

class ItemsViewAutomationPeer :
    public ReferenceTracker<ItemsViewAutomationPeer, winrt::implementation::ItemsViewAutomationPeerT>
{

public:
    ItemsViewAutomationPeer(winrt::ItemsView const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    // ISelectionProvider
    bool CanSelectMultiple();
    bool IsSelectionRequired() { return false; };
    winrt::com_array<winrt::IRawElementProviderSimple> GetSelection();

    void RaiseSelectionChanged(double oldIndex, double newIndex);

private:
    com_ptr<ItemsView> GetImpl();
};

