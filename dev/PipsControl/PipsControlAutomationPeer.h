// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PipsControl.h"
#include "PipsControlAutomationPeer.g.h"

class PipsControlAutomationPeer :
    public ReferenceTracker<PipsControlAutomationPeer, winrt::implementation::PipsControlAutomationPeerT, winrt::ISelectionProvider>
{

public:
    PipsControlAutomationPeer(winrt::PipsControl const& owner);

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
    com_ptr<PipsControl> GetImpl();
};

