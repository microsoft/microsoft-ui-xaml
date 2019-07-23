// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ResourceAccessor.h"
#include "TabView.h"
#include "TabViewAutomationPeer.h"
#include "Utils.h"
#include "common.h"

TabViewAutomationPeer::TabViewAutomationPeer(winrt::TabView  /*unused*/const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable TabViewAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    return __super::GetPatternCore(patternInterface);
}

hstring TabViewAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::TabView>();
}

winrt::AutomationControlType TabViewAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Tab;
}

