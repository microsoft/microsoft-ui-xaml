﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class NavigationViewItemBaseRevokers : public winrt::implements<NavigationViewItemBaseRevokers, winrt::IInspectable>
{
public:
    winrt::UIElement::KeyDown_revoker keyDownRevoker{};
    winrt::UIElement::GotFocus_revoker gotFocusRevoker{};
    PropertyChanged_revoker isSelectedRevoker{};
    PropertyChanged_revoker isExpandedRevoker{};
    PropertyChanged_revoker visibilityRevoker{};

    void RevokeAll() {
        if (keyDownRevoker) keyDownRevoker.revoke();
        if (gotFocusRevoker) gotFocusRevoker.revoke();
        if (visibilityRevoker) visibilityRevoker.revoke();
        if (isSelectedRevoker) isSelectedRevoker.revoke();
        if (isExpandedRevoker) isExpandedRevoker.revoke();
    }
};
