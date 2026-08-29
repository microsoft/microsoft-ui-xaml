// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "MenuBarAutomationPeer.h"
#include "Utils.h"

#include "MenuBarAutomationPeer.properties.cpp"

MenuBarAutomationPeer::MenuBarAutomationPeer(winrt::MenuBar const& owner) : ReferenceTracker(owner)
{
}

winrt::AutomationControlType MenuBarAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::MenuBar;
}
