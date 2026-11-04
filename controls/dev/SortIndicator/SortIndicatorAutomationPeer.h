// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "SortIndicator.h"
#include "SortIndicatorAutomationPeer.g.h"

class SortIndicatorAutomationPeer :
    public ReferenceTracker<SortIndicatorAutomationPeer, winrt::implementation::SortIndicatorAutomationPeerT>
{
public:
    SortIndicatorAutomationPeer(winrt::SortIndicator const& owner);

    // FrameworkElementAutomationPeer overrides
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
    bool IsControlElementCore();
    bool IsContentElementCore();
};
