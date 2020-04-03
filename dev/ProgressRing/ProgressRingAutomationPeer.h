// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ProgressRing.h"
#include "ProgressRingAutomationPeer.g.h"

class ProgressRingAutomationPeer :
    public ReferenceTracker<ProgressRingAutomationPeer, winrt::implementation::ProgressRingAutomationPeerT>
{

public:
    ProgressRingAutomationPeer(winrt::ProgressRing const& owner);

    hstring GetClassNameCore();
    winrt::hstring GetNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
};

