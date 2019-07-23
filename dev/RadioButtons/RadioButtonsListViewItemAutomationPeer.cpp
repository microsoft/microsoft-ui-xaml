// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RadioButtonsListViewItem.h"
#include "RadioButtonsListViewItemAutomationPeer.h"
#include "ResourceAccessor.h"
#include "common.h"

CppWinRTActivatableClassWithBasicFactory(RadioButtonsListViewItemAutomationPeer);

RadioButtonsListViewItemAutomationPeer::RadioButtonsListViewItemAutomationPeer(winrt::RadioButtonsListViewItem  /*unused*/const& owner) :
    ReferenceTracker(owner)
{
}

winrt::AutomationControlType RadioButtonsListViewItemAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::RadioButton;
}