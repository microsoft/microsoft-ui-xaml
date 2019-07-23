// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "MenuBarAutomationPeer.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "common.h"

MenuBarAutomationPeer::MenuBarAutomationPeer(winrt::MenuBar  /*unused*/const& owner) : ReferenceTracker(owner)
{
}

winrt::AutomationControlType MenuBarAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::MenuBar;
}