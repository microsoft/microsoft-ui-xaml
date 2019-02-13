// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "CommandBarFlyoutCommandBarAutomationPeer.h"
#include "CommandBarFlyoutCommandBar.h"

#include "ResourceAccessor.h"
#include "Utils.h"

CommandBarFlyoutCommandBarAutomationPeer::CommandBarFlyoutCommandBarAutomationPeer(winrt::CommandBarFlyoutCommandBar const& owner) :
    ReferenceTracker(owner)
{
}

winrt::IVector<winrt::AutomationPeer> CommandBarFlyoutCommandBarAutomationPeer::GetChildrenCore()
{
    winrt::CommandBarFlyoutCommandBar commandBar = safe_cast<winrt::CommandBarFlyoutCommandBar>(Owner());
    return winrt::get_self<CommandBarFlyoutCommandBar>(commandBar)->GetAutomationChildren();
}
