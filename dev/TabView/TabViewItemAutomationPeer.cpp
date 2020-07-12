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
winrt::IInspectable TabViewItemAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::SelectionItem)
    {
        return *this;
    }
    return __super::GetPatternCore(patternInterface);
}

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
    if (auto tvi = Owner().try_as<TabViewItem>())
    {
        const bool returnValue = tvi->IsSelected();
        return returnValue;
    }
    return false;
}

winrt::IRawElementProviderSimple TabViewItemAutomationPeer::SelectionContainer()
{
    return nullptr;
}

void TabViewItemAutomationPeer::AddToSelection()
{
    Select();
}

void TabViewItemAutomationPeer::RemoveFromSelection()
{
    // Can't unselect in a TabView without knowing next selection
}

void TabViewItemAutomationPeer::Select()
{
    if (auto owner = Owner().try_as<TabViewItem>().get())
    {
        owner->IsSelected(true);
    }
}
