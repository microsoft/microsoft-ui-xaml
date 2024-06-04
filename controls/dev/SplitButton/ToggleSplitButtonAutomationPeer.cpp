// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ToggleSplitButton.h"
#include "ToggleSplitButtonAutomationPeer.h"
#include "Utils.h"

#include "ToggleSplitButtonAutomationPeer.properties.cpp"

ToggleSplitButtonAutomationPeer::ToggleSplitButtonAutomationPeer(winrt::ToggleSplitButton const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable ToggleSplitButtonAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::ExpandCollapse ||
        patternInterface == winrt::PatternInterface::Toggle)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring ToggleSplitButtonAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::ToggleSplitButton>();
}

winrt::AutomationControlType ToggleSplitButtonAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::SplitButton;
}

com_ptr<ToggleSplitButton> ToggleSplitButtonAutomationPeer::GetImpl()
{
    com_ptr<ToggleSplitButton> impl = nullptr;

    if (auto splitButton = Owner().try_as<winrt::ToggleSplitButton>())
    {
        impl = winrt::get_self<ToggleSplitButton>(splitButton)->get_strong();
    }

    return impl;
}

// IExpandCollapseProvider 
winrt::ExpandCollapseState ToggleSplitButtonAutomationPeer::ExpandCollapseState()
{
    winrt::ExpandCollapseState currentState = winrt::ExpandCollapseState::Collapsed;

    if (auto splitButton = GetImpl())
    {
        if (splitButton->IsFlyoutOpen())
        {
            currentState = winrt::ExpandCollapseState::Expanded;
        }
    }

    return currentState;
}

void ToggleSplitButtonAutomationPeer::Expand()
{
    if (auto splitButton = GetImpl())
    {
        splitButton->OpenFlyout();
    }
}

void ToggleSplitButtonAutomationPeer::Collapse()
{
    if (auto splitButton = GetImpl())
    {
        splitButton->CloseFlyout();
    }
}

// IToggleProvider
winrt::ToggleState ToggleSplitButtonAutomationPeer::ToggleState()
{
    winrt::ToggleState state = winrt::ToggleState::Off;

    if (auto splitButton = GetImpl())
    {
        if (splitButton->IsChecked())
        {
            state = winrt::ToggleState::On;
        }
    }

    return state;
}

void ToggleSplitButtonAutomationPeer::Toggle()
{
    if (auto splitButton = GetImpl())
    {
        splitButton->Toggle();
    }
}
