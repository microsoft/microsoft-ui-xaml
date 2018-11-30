// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "RadioButtonsListViewItemAutomationPeer.h"
#include "RadioButtonsListViewItem.h"

CppWinRTActivatableClassWithBasicFactory(RadioButtonsListViewItemAutomationPeer);

RadioButtonsListViewItemAutomationPeer::RadioButtonsListViewItemAutomationPeer(winrt::RadioButtonsListViewItem const& owner) :
    ReferenceTracker(owner)
{
}

winrt::AutomationControlType RadioButtonsListViewItemAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::RadioButton;
}