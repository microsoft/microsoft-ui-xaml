// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "SelectorBarItem.h"
#include "SelectorBarItemAutomationPeer.h"
#include "SelectorBarItemAutomationPeer.properties.cpp"
#include "../ResourceHelper/ResourceAccessor.h"

SelectorBarItemAutomationPeer::SelectorBarItemAutomationPeer(winrt::FrameworkElement const& owner)
    : base_type(owner)
{
}

// IAutomationPeerOverrides
winrt::hstring SelectorBarItemAutomationPeer::GetNameCore()
{
    // First choice given to the AutomationProperties.Name.
    winrt::hstring returnHString = winrt::AutomationProperties::GetName(Owner());

    if (returnHString.empty())
    {
        // Second choice given to the SelectorBarItem.Text property.
        if (const auto selectorBarItem = Owner().try_as<winrt::SelectorBarItem>())
        {
            returnHString = selectorBarItem.Text();
        }
    }

    if (returnHString.empty())
    {
        // Third choice given to the ItemContainer.Child property.
        if (const auto itemContainer = Owner().try_as<winrt::ItemContainer>())
        {
            returnHString = SharedHelpers::TryGetStringRepresentationFromObject(itemContainer.Child());
        }
    }

    if (returnHString.empty())
    {
        // Fourth choice given to the localized control name.
        returnHString = ResourceAccessor::GetLocalizedStringResource(SR_SelectorBarItemDefaultControlName);
    }

    return returnHString;
}

winrt::hstring SelectorBarItemAutomationPeer::GetLocalizedControlTypeCore()
{
    return ResourceAccessor::GetLocalizedStringResource(SR_SelectorBarItemDefaultControlName);
}

winrt::hstring SelectorBarItemAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::SelectorBarItem>();
}
