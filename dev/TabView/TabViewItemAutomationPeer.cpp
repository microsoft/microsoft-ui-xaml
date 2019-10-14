// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "TabViewItem.h"
#include "TabViewItemAutomationPeer.h"
#include "Utils.h"

TabViewItemAutomationPeer::TabViewItemAutomationPeer(winrt::TabViewItem const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
hstring TabViewItemAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::TabViewItem>();
}

winrt::AutomationControlType TabViewItemAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::TabItem;
}


winrt::hstring TabViewItemAutomationPeer::GetNameCore()
{
    winrt::hstring returnHString = __super::GetNameCore();

    // If a name hasn't been provided by AutomationProperties.Name in markup:
    if (returnHString.empty())
    {
        if (auto tvi = Owner().try_as<winrt::TabViewItem>())
        {
            if (auto content = tvi.Header())
            {
                if (auto stringableName = content.try_as<winrt::IStringable>())
                {
                    returnHString = stringableName.ToString();
                }
            }
        }
    }

    return returnHString;
}
