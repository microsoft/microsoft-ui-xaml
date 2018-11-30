// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RadioButtonsListViewItem.h"
#include "RadioButtonsListViewItemAutomationPeer.h"

CppWinRTActivatableClassWithBasicFactory(RadioButtonsListViewItem);

winrt::AutomationPeer RadioButtonsListViewItem::OnCreateAutomationPeer()
{
    return winrt::make<RadioButtonsListViewItemAutomationPeer>(*this);
}