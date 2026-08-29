// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class ItemContainerRevokers : public winrt::implements<ItemContainerRevokers, winrt::IInspectable>
{
public:
    void RevokeAll(const winrt::ItemContainer& itemContainer)
    {
        if (m_isSelectedPropertyChangedRevoker)
        {
            m_isSelectedPropertyChangedRevoker.revoke();
        }

        if (m_gettingFocusRevoker)
        {
            m_gettingFocusRevoker.revoke();
        }

        if (m_losingFocusRevoker)
        {
            m_losingFocusRevoker.revoke();
        }

        if (m_keyDownRevoker)
        {
            m_keyDownRevoker.revoke();
        }

        if (m_itemInvokedRevoker)
        {
            m_itemInvokedRevoker.revoke();
        }

#ifdef DBG_VERBOSE
        if (m_sizeChangedRevokerDbg)
        {
            m_sizeChangedRevokerDbg.revoke();
        }
#endif
    }

    winrt::ItemContainer::ItemInvoked_revoker m_itemInvokedRevoker{};

    winrt::UIElement::KeyDown_revoker m_keyDownRevoker{};
    winrt::UIElement::GettingFocus_revoker m_gettingFocusRevoker{};
    winrt::UIElement::LosingFocus_revoker m_losingFocusRevoker{};

    PropertyChanged_revoker m_isSelectedPropertyChangedRevoker{};

#ifdef DBG_VERBOSE
    winrt::FrameworkElement::SizeChanged_revoker m_sizeChangedRevokerDbg;
#endif
};
