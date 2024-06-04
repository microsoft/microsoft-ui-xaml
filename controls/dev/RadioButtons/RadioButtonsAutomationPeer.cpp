// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "RadioButtons.h"
#include "RadioButtonsAutomationPeer.h"
#include "Utils.h"

#include "RadioButtonsAutomationPeer.properties.cpp"

RadioButtonsAutomationPeer::RadioButtonsAutomationPeer(winrt::RadioButtons const& owner)
    : ReferenceTracker(owner)
{
}

hstring RadioButtonsAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::RadioButtons>();
}

hstring RadioButtonsAutomationPeer::GetNameCore()
{
    winrt::hstring name = __super::GetNameCore();

    if (name.empty())
    {
        if (auto radioButtons = Owner().try_as<winrt::RadioButtons>())
        {
            name = SharedHelpers::TryGetStringRepresentationFromObject(radioButtons.Header());
        }
    }

    return name;
}

winrt::AutomationControlType RadioButtonsAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Group;
}
