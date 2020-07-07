// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "TabViewItem.h"
#include "TabViewItemAutomationPeer.h"
#include "Utils.h"
#include "SharedHelpers.h"

#include "TabViewItemAutomationPeer.properties.cpp"

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
            returnHString = SharedHelpers::TryGetStringRepresentationFromObject(tvi.Header());
        }
    }

    return returnHString;
}


bool TabViewItemAutomationPeer::IsSelected()
{
    return false;
}

winrt::IRawElementProviderSimple TabViewItemAutomationPeer::SelectionContainer()
{
    return nullptr;
}

void TabViewItemAutomationPeer::AddToSelection()
{
}

void TabViewItemAutomationPeer::RemoveFromSelection()
{
}

void TabViewItemAutomationPeer::Select()
{
}

winrt::TabView TabViewItemAutomationPeer::GetParenTabView()
{
    if (auto tvi = Owner().try_as<winrt::TabViewItem>())
    {
        return winrt::get_self<TabViewItem>(tvi)->GetParentTabView();
    }
    return nullptr;
}
