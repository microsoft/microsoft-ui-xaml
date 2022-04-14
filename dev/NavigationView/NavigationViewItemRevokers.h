// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class NavigationViewItemRevokers : public winrt::implements<NavigationViewItemRevokers, winrt::IInspectable>
{
public:
    winrt::UIElement::Tapped_revoker tappedRevoker{};
    winrt::UIElement::KeyDown_revoker keyDownRevoker{};
    winrt::UIElement::GotFocus_revoker gotFocusRevoker{};
    PropertyChanged_revoker isSelectedRevoker{};
    PropertyChanged_revoker isExpandedRevoker{};

    void RevokeAll() {
        if (tappedRevoker) tappedRevoker.revoke();
        if (keyDownRevoker) keyDownRevoker.revoke();
        if (gotFocusRevoker) gotFocusRevoker.revoke();
        if (isSelectedRevoker) isSelectedRevoker.revoke();
        if (isExpandedRevoker) isExpandedRevoker.revoke();
    }
};
