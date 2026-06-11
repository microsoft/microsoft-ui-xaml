// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MenuBar.h"
#include "MenuBarItem.h"
#include "ResourceAccessor.h"
#include "MenuBarItemAutomationPeer.h"
#include "Utils.h"

#include "MenuBarItemAutomationPeer.properties.cpp"

MenuBarItemAutomationPeer::MenuBarItemAutomationPeer(winrt::MenuBarItem const& owner) : ReferenceTracker(owner)
{
}

winrt::AutomationControlType MenuBarItemAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::MenuItem;
}

winrt::IInspectable MenuBarItemAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::Invoke || patternInterface == winrt::PatternInterface::ExpandCollapse)
    {
        return *this;
    }
    return __super::GetPatternCore(patternInterface);
}

winrt::hstring MenuBarItemAutomationPeer::GetNameCore()
{
    //Check to see if the item has a defined AutomationProperties.Name
    winrt::hstring name = __super::GetNameCore();

    if (name.empty())
    {
        auto owner = Owner().as<winrt::MenuBarItem>();
        name = owner.Title();
    }

    return name;
}

winrt::hstring MenuBarItemAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::MenuBarItem>();
}

void MenuBarItemAutomationPeer::Invoke()
{
    auto owner = Owner().as<winrt::MenuBarItem>();
    winrt::get_self<MenuBarItem>(owner)->Invoke();
}

winrt::ExpandCollapseState MenuBarItemAutomationPeer::ExpandCollapseState()
{
    winrt::UIElement owner = Owner();
    auto menuBarItem = owner.as<winrt::MenuBarItem>();
    if (winrt::get_self<MenuBarItem>(menuBarItem)->IsFlyoutOpen())
    {
        return winrt::ExpandCollapseState::Expanded;
    }
    else
    {
        return winrt::ExpandCollapseState::Collapsed;
    }
} 

void MenuBarItemAutomationPeer::Collapse()
{
    auto owner = Owner().as<winrt::MenuBarItem>();
    winrt::get_self<MenuBarItem>(owner)->CloseMenuFlyout();
}

void MenuBarItemAutomationPeer::Expand()
{
    auto owner = Owner().as<winrt::MenuBarItem>();
    winrt::get_self<MenuBarItem>(owner)->ShowMenuFlyout();
} 
