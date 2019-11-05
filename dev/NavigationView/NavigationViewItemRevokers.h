// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemRevokers.g.h"

class NavigationViewItemRevokers :
    public ReferenceTracker<NavigationViewItemRevokers, winrt::implementation::NavigationViewItemRevokersT, winrt::composing, winrt::composable>
{
public:
    winrt::NavigationViewItem::NavigationViewItemInvoked_revoker pointerPressedRevoker{};
};
