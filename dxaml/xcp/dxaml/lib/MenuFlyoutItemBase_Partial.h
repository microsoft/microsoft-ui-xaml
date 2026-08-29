// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MenuFlyoutItemBase.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(MenuFlyoutItemBase)
    {
        // Give MenuFlyoutPresenter friend access so it can call private method SetParentMenuFlyoutPresenter().
        friend class MenuFlyoutPresenter;

    private:
        // Weak reference to the MenuFlyoutPresenter that contains this MenuFlyoutItemBase.
        ctl::WeakRefPtr m_wrParentMenuFlyoutPresenter;
        
        // Flag indicating whether this item has a style set by a parent menu (SplitMenuFlyoutItem).
        bool m_isStyleSetFromParentMenu = false;

    public:
        // Get the parent MenuFlyoutPresenter.
        _Check_return_ HRESULT GetParentMenuFlyoutPresenter(
            _Outptr_ MenuFlyoutPresenter** ppParentMenuFlyoutPresenter);

        // Check whether this MenuFlyoutItemBase has a Style set by a parent menu.
        bool GetIsStyleSetFromParentMenu();

        // Sets a flag showing whether this MenuFlyoutItemBase had a style set by a parent menu.
        void SetIsStyleSetFromParentMenu(bool value);

    protected:
        // Gets a value indicating whether or not this item
        // should be vertically narrow or wide.
        _Check_return_ HRESULT GetShouldBeNarrow(
            _Out_ bool *pShouldBeNarrow);

    private:
        // Sets the parent MenuFlyoutPresenter.
        _Check_return_ HRESULT SetParentMenuFlyoutPresenter(
            _In_opt_ MenuFlyoutPresenter* pParentMenuFlyoutPresenter);

    };
}
