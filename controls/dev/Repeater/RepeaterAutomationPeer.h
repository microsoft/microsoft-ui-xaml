// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RepeaterAutomationPeer.g.h"

class RepeaterAutomationPeer :
    public ReferenceTracker<RepeaterAutomationPeer, winrt::implementation::RepeaterAutomationPeerT>
{
public:
    RepeaterAutomationPeer(winrt::ItemsRepeater const& owner);

#pragma region IAutomationPeerOverrides

    winrt::IVector<winrt::AutomationPeer> GetChildrenCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

#pragma endregion

private:
    winrt::UIElement GetElement(const winrt::AutomationPeer& peer, const winrt::ItemsRepeater& repeater);
};