// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "SplitButton.h"
#include "SplitButtonAutomationPeer.h"
#include "Utils.h"

SplitButtonAutomationPeer::SplitButtonAutomationPeer(winrt::SplitButton const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable SplitButtonAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::ExpandCollapse ||
        patternInterface == winrt::PatternInterface::Invoke)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring SplitButtonAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::SplitButton>();
}

winrt::AutomationControlType SplitButtonAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::SplitButton;
}

com_ptr<SplitButton> SplitButtonAutomationPeer::GetImpl()
{
    com_ptr<SplitButton> impl = nullptr;

    if (auto splitButton = safe_try_cast<winrt::SplitButton>(Owner()))
    {
        impl = winrt::get_self<SplitButton>(splitButton)->get_strong();
    }

    return impl;
}

// IExpandCollapseProvider 
winrt::ExpandCollapseState SplitButtonAutomationPeer::ExpandCollapseState()
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

void SplitButtonAutomationPeer::Expand()
{
    if (auto splitButton = GetImpl())
    {
        splitButton->OpenFlyout();
    }
}

void SplitButtonAutomationPeer::Collapse()
{
    if (auto splitButton = GetImpl())
    {
        splitButton->CloseFlyout();
    }
}

// IInvokeProvider
void SplitButtonAutomationPeer::Invoke()
{
    if (auto splitButton = GetImpl())
    {
        splitButton->OnClickPrimary(nullptr, nullptr);
    }
}
