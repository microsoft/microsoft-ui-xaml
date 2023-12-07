// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RoutedEventArgs.h"

class CLosingFocusEventArgs : public CRoutedEventArgs
{
public:
    CLosingFocusEventArgs (
        _In_ CDependencyObject* oldFocusedElement,
        _In_ CDependencyObject* newFocusedElement,
        _In_ DirectUI::FocusState focusState,
        _In_ DirectUI::FocusNavigationDirection focusDirection,
        _In_ DirectUI::FocusInputDeviceKind deviceKind,
        _In_ bool canCancelFocus,
        _In_ GUID correlationId
    ) :
    m_oldFocusedElement(oldFocusedElement),
    m_newFocusedElement(newFocusedElement),
    m_focusState(focusState),
    m_focusNavigationDirection(focusDirection),
    m_inputDevice(deviceKind),
    m_bCancel(false),
    m_bCanCancelOrRedirectFocus(canCancelFocus),
    m_correlationId(correlationId)

    {  }

    CDependencyObject* GetNewFocusedElementNoRef() const
    {
        return m_newFocusedElement;
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_OldFocusedElement(_Outptr_ CDependencyObject** oldFocusedElement);

    _Check_return_ HRESULT get_NewFocusedElement(_Outptr_ CDependencyObject** newFocusedElement);
    _Check_return_ HRESULT put_NewFocusedElement(_In_ CDependencyObject* newFocusedElement);

    _Check_return_ HRESULT get_FocusState(_Out_ DirectUI::FocusState* focusState);

    _Check_return_ HRESULT get_Direction(_Out_ DirectUI::FocusNavigationDirection* focusNavigationDirection);

    _Check_return_ HRESULT get_Handled(_Out_ BOOLEAN *handled);
    _Check_return_ HRESULT put_Handled(_In_ BOOLEAN handled);

    _Check_return_ HRESULT get_InputDevice(_Out_ DirectUI::FocusInputDeviceKind* inputDevice);

    _Check_return_ HRESULT get_CorrelationId(_Out_ GUID* correlationId);
    _Check_return_ HRESULT put_CorrelationId(_In_ GUID correlationId);

    _Check_return_ HRESULT get_Cancel(_Out_ BOOLEAN* cancel);
    _Check_return_ HRESULT put_Cancel(BOOLEAN cancel);

    bool CanCancelOrRedirectFocus() const { return m_bCanCancelOrRedirectFocus; }

    xref_ptr<CDependencyObject> m_oldFocusedElement;
    xref_ptr<CDependencyObject> m_newFocusedElement;
    DirectUI::FocusState m_focusState;
    DirectUI::FocusNavigationDirection m_focusNavigationDirection;
    DirectUI::FocusInputDeviceKind m_inputDevice;
    bool m_bCancel;

private:
    bool m_bCanCancelOrRedirectFocus;
    GUID m_correlationId;
};