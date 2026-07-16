// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InkToolBarAutomationPeer.h"
#include "InkToolBar.h"

InkToolBarAutomationPeer::InkToolBarAutomationPeer(winrt::InkToolBar const& owner)
    : ReferenceTracker(owner)
{
}

hstring InkToolBarAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::InkToolBar>();
}

// Matches the WinUI 2 InkToolbar peer, which reports the bar as a Pane. The default
// GetChildrenCore already returns the tool/menu buttons, so no override is needed there.
winrt::AutomationControlType InkToolBarAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Pane;
}
