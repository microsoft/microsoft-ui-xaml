// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationView.h"
#include "NavigationViewAutomationPeer.g.h"

class NavigationViewAutomationPeer :
    public ReferenceTracker<
        NavigationViewAutomationPeer,
        winrt::implementation::NavigationViewAutomationPeerT,
        winrt::ISelectionProvider>
{
public:
    NavigationViewAutomationPeer(winrt::NavigationView const& owner);

    // IAutomationPeerOverrides 
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);

    // ISelectionProvider
    bool CanSelectMultiple();
    bool IsSelectionRequired();
    winrt::com_array<winrt::Windows::UI::Xaml::Automation::Provider::IRawElementProviderSimple> GetSelection();

    void RaiseSelectionChangedEvent(winrt::IInspectable const& oldSelection, winrt::IInspectable const& newSelecttion);

private:
};
