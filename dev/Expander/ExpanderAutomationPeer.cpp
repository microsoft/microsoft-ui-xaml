// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ExpanderAutomationPeer.h"
#include "Utils.h"

#include "ExpanderAutomationPeer.properties.cpp"

ExpanderAutomationPeer::ExpanderAutomationPeer(winrt::Expander const& owner)
    : ReferenceTracker(owner)
{
}

winrt::IInspectable ExpanderAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    return winrt::IInspectable();
}

hstring ExpanderAutomationPeer::GetClassNameCore()
{
    return hstring();
}

hstring ExpanderAutomationPeer::GetNameCore()
{
    return hstring();
}

winrt::AutomationControlType ExpanderAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType();
}

winrt::ExpandCollapseState ExpanderAutomationPeer::ExpandCollapseState()
{
    return winrt::ExpandCollapseState();
}

void ExpanderAutomationPeer::Expand()
{
}

void ExpanderAutomationPeer::Collapse()
{
}

com_ptr<Expander> ExpanderAutomationPeer::GetImpl()
{
    return com_ptr<Expander>();
}
