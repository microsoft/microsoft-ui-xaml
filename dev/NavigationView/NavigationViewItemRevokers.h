// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class NavigationViewItemRevokers : public winrt::implements<NavigationViewItemRevokers, winrt::IInspectable>
{
public:
    winrt::NavigationViewItem::NavigationViewItemInvoked_revoker pointerPressedRevoker{};
    PropertyChanged_revoker isSelectedRevoker{};
};
