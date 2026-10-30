// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InkToolbarAutomationPeer.h"
#include "InkToolbar.h"

InkToolbarAutomationPeer::InkToolbarAutomationPeer(winrt::InkToolbar const& owner)
    : ReferenceTracker(owner)
{
}

hstring InkToolbarAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::InkToolbar>();
}

// Matches the WinUI 2 InkToolbar peer, which reports the bar as a Pane. The default
// GetChildrenCore already returns the tool/menu buttons, so no override is needed there.
winrt::AutomationControlType InkToolbarAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Pane;
}
