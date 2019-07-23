// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RadioButtonsListViewItem.h"
#include "RadioButtonsListViewItemAutomationPeer.h"
#include "common.h"

CppWinRTActivatableClassWithBasicFactory(RadioButtonsListViewItem);

winrt::AutomationPeer RadioButtonsListViewItem::OnCreateAutomationPeer()
{
    return winrt::make<RadioButtonsListViewItemAutomationPeer>(*this);
}