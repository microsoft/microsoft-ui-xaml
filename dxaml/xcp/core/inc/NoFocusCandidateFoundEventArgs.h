// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RoutedEventArgs.h"

class CNoFocusCandidateFoundEventArgs final : public CRoutedEventArgs
{
public:
    CNoFocusCandidateFoundEventArgs (
        _In_ DirectUI::FocusNavigationDirection focusDirection,
        _In_ DirectUI::FocusInputDeviceKind deviceKind
    ) :
    m_focusNavigationDirection(focusDirection),
    m_inputDevice(deviceKind)
    {  }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_Direction(_Out_ DirectUI::FocusNavigationDirection* focusNavigationDirection);

    _Check_return_ HRESULT get_Handled(_Out_ BOOLEAN *handled);
    _Check_return_ HRESULT put_Handled(_In_ BOOLEAN handled);

    _Check_return_ HRESULT get_InputDevice(_Out_ DirectUI::FocusInputDeviceKind* inputDevice);

    DirectUI::FocusNavigationDirection m_focusNavigationDirection;
    DirectUI::FocusInputDeviceKind m_inputDevice;
};