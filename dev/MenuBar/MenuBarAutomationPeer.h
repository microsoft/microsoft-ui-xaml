// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MenuBar.h"
#include "MenuBarAutomationPeer.g.h"

class MenuBarAutomationPeer :
    public ReferenceTracker<MenuBarAutomationPeer, winrt::implementation::MenuBarAutomationPeerT>
{

public:
    MenuBarAutomationPeer(winrt::MenuBar const& owner);

    // IAutomationPeerOverrides 
    winrt::AutomationControlType GetAutomationControlTypeCore();

};
