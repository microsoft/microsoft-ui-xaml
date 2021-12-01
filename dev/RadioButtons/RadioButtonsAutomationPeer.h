// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RadioButtons.h"
#include "RadioButtonsAutomationPeer.g.h"

class RadioButtonsAutomationPeer :
    public ReferenceTracker<RadioButtonsAutomationPeer, winrt::implementation::RadioButtonsAutomationPeerT>
{

public:
    RadioButtonsAutomationPeer(winrt::RadioButtons const& owner);

    // IAutomationPeerOverrides
    hstring GetClassNameCore();
    hstring GetNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
};

