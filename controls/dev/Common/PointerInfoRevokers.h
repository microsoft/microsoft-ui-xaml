// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class PointerInfoRevokers : public winrt::implements<PointerInfoRevokers, winrt::IInspectable>
{
public:
    void RevokeAll()
    {
        if (m_pointerEnteredRevoker)
        {
            m_pointerEnteredRevoker.revoke();
        }

        if (m_pointerMovedRevoker)
        {
            m_pointerMovedRevoker.revoke();
        }

        if (m_pointerExitedRevoker)
        {
            m_pointerExitedRevoker.revoke();
        }

        if (m_pointerPressedRevoker)
        {
            m_pointerPressedRevoker.revoke();
        }

        if (m_pointerReleasedRevoker)
        {
            m_pointerReleasedRevoker.revoke();
        }

        if (m_pointerCanceledRevoker)
        {
            m_pointerCanceledRevoker.revoke();
        }

        if (m_pointerCaptureLostRevoker)
        {
            m_pointerCaptureLostRevoker.revoke();
        }

        if (m_gettingFocusRevoker)
        {
            m_gettingFocusRevoker.revoke();
        }

        if (m_losingFocusRevoker)
        {
            m_losingFocusRevoker.revoke();
        }

        if (m_tappedRevoker)
        {
            m_tappedRevoker.revoke();
        }

        if (m_doubleTappedRevoker)
        {
            m_doubleTappedRevoker.revoke();
        }

        if (m_isEnabledChangedRevoker)
        {
            m_isEnabledChangedRevoker.revoke();
        }
    }

    RoutedEventHandler_revoker m_pointerExitedRevoker{};
    RoutedEventHandler_revoker m_pointerPressedRevoker{};
    RoutedEventHandler_revoker m_pointerReleasedRevoker{};
    RoutedEventHandler_revoker m_pointerCanceledRevoker{};
    RoutedEventHandler_revoker m_pointerCaptureLostRevoker{};

    winrt::Control::IsEnabledChanged_revoker m_isEnabledChangedRevoker{};

    winrt::UIElement::Tapped_revoker m_tappedRevoker{};
    winrt::UIElement::DoubleTapped_revoker m_doubleTappedRevoker{};
    winrt::UIElement::PointerEntered_revoker m_pointerEnteredRevoker{};
    winrt::UIElement::PointerMoved_revoker m_pointerMovedRevoker{};
    winrt::UIElement::GettingFocus_revoker m_gettingFocusRevoker{};
    winrt::UIElement::LosingFocus_revoker m_losingFocusRevoker{};
};
