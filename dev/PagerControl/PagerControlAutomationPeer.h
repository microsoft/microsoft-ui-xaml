// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PagerControl.h"
#include "PagerControlAutomationPeer.g.h"

class PagerControlAutomationPeer :
    public ReferenceTracker<PagerControlAutomationPeer, winrt::implementation::PagerControlAutomationPeerT, winrt::ISelectionProvider>
{

public:
    PagerControlAutomationPeer(winrt::PagerControl const& owner);

    /* IAutomationPeerOverrides */
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    hstring GetNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    // ISelectionProvider
    bool CanSelectMultiple() { return false; };
    bool IsSelectionRequired() { return true; };
    winrt::com_array<winrt::IInspectable> GetSelection();

    void RaiseSelectionChanged(double oldIndex, double newIndex);

private:
    com_ptr<PagerControl> GetImpl();
};

