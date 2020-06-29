// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ProgressRing.h"
#include "ProgressRingAutomationPeer.h"
#include "Utils.h"

#include "ProgressRingAutomationPeer.properties.cpp"

ProgressRingAutomationPeer::ProgressRingAutomationPeer(winrt::ProgressRing const& owner)
    : ReferenceTracker(owner)
{
}

winrt::hstring ProgressRingAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::ProgressRing>();
}

winrt::hstring ProgressRingAutomationPeer::GetNameCore()
{
    //Check to see if the item has a defined AutomationProperties.Name
    winrt::hstring name = __super::GetNameCore();

    if (auto progressRing = Owner().try_as<winrt::ProgressRing>())
    {
        if (progressRing.IsActive())
        {
            return winrt::hstring{ ResourceAccessor::GetLocalizedStringResource(SR_ProgressRingIndeterminateStatus) + name };
        }
    }
    return name;
}

winrt::AutomationControlType ProgressRingAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::ProgressBar;
}

winrt::hstring ProgressRingAutomationPeer::GetLocalizedControlTypeCore()
{
    return ResourceAccessor::GetLocalizedStringResource(SR_ProgressRingName);
}
