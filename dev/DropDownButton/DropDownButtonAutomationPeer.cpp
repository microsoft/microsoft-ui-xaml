// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "DropDownButton.h"
#include "DropDownButtonAutomationPeer.h"
#include "Utils.h"


#include "DropDownButtonAutomationPeer.properties.cpp"

DropDownButtonAutomationPeer::DropDownButtonAutomationPeer(winrt::DropDownButton const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable DropDownButtonAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::ExpandCollapse)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring DropDownButtonAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::DropDownButton>();
}

com_ptr<DropDownButton> DropDownButtonAutomationPeer::GetImpl()
{
    com_ptr<DropDownButton> impl;

    if (auto dropDownButton = Owner().try_as<winrt::DropDownButton>())
    {
        impl = winrt::get_self<DropDownButton>(dropDownButton)->get_strong();
    }

    return impl;
}

// IExpandCollapseProvider 
winrt::ExpandCollapseState DropDownButtonAutomationPeer::ExpandCollapseState()
{
    winrt::ExpandCollapseState currentState = winrt::ExpandCollapseState::Collapsed;

    if (auto dropDownButton = GetImpl())
    {
        if (dropDownButton->IsFlyoutOpen())
        {
            currentState = winrt::ExpandCollapseState::Expanded;
        }
    }

    return currentState;
}

void DropDownButtonAutomationPeer::Expand()
{
    if (auto dropDownButton = GetImpl())
    {
        dropDownButton->OpenFlyout();
    }
}

void DropDownButtonAutomationPeer::Collapse()
{
    if (auto dropDownButton = GetImpl())
    {
        dropDownButton->CloseFlyout();
    }
}

