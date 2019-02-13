// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CommandBarFlyoutCommandBar.h"
#include "CommandBarFlyoutCommandBarAutomationPeer.g.h"

class CommandBarFlyoutCommandBarAutomationPeer :
    public ReferenceTracker<CommandBarFlyoutCommandBarAutomationPeer, winrt::implementation::CommandBarFlyoutCommandBarAutomationPeerT>
{
public:
    CommandBarFlyoutCommandBarAutomationPeer(winrt::CommandBarFlyoutCommandBar const& owner);

    // IAutomationPeerOverrides 
    winrt::IVector<winrt::AutomationPeer> GetChildrenCore();
};

CppWinRTActivatableClassWithBasicFactory(CommandBarFlyoutCommandBarAutomationPeer)