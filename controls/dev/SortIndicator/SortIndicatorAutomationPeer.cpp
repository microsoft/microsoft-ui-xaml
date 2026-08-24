// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SortIndicatorAutomationPeer.h"

#include "Utils.h"

#include "SortIndicatorAutomationPeer.properties.cpp"

SortIndicatorAutomationPeer::SortIndicatorAutomationPeer(winrt::SortIndicator const& owner) :
    ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides

hstring SortIndicatorAutomationPeer::GetClassNameCore()
{
    // Fixed string, not hstring_name_of<>: the internal namespace should not reach assistive tech.
    return L"SortIndicator";
}

winrt::AutomationControlType SortIndicatorAutomationPeer::GetAutomationControlTypeCore()
{
    // Decorative: the owning header announces sort state, so don't surface a separate control.
    return winrt::AutomationControlType::Image;
}

bool SortIndicatorAutomationPeer::IsControlElementCore()
{
    return false;
}

bool SortIndicatorAutomationPeer::IsContentElementCore()
{
    return false;
}
