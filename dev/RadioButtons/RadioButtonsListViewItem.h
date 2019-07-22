// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RadioButtonsListViewItem.g.h"

class RadioButtonsListViewItem :
    public ReferenceTracker<RadioButtonsListViewItem, winrt::implementation::RadioButtonsListViewItemT>
{
public:
    RadioButtonsListViewItem() = default;

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer();
};