// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PipsPager.h"
#include "PipsPagerAutomationPeer.g.h"

class PipsPagerAutomationPeer :
    public ReferenceTracker<PipsPagerAutomationPeer, winrt::implementation::PipsPagerAutomationPeerT, winrt::ISelectionProvider>
{

public:
    PipsPagerAutomationPeer(winrt::PipsPager const& owner);

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
    com_ptr<PipsPager> GetImpl();
};

