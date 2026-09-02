// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ResizeGripper.h"
#include "ResizeGripperAutomationPeer.g.h"

class ResizeGripperAutomationPeer :
    public ReferenceTracker<ResizeGripperAutomationPeer, winrt::implementation::ResizeGripperAutomationPeerT>
{
public:
    ResizeGripperAutomationPeer(winrt::ResizeGripper const& owner);

    // IAutomationPeerOverrides
    hstring GetClassNameCore();
    hstring GetNameCore();
    hstring GetAutomationIdCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
};
