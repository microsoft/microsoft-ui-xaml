// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RoutedEventArgs.h"
#include "enumdefs.g.h"

class CKeyEventArgs final : public CRoutedEventArgs
{
public:
    CKeyEventArgs()
    {
        m_platformKeyCode = wsy::VirtualKey::VirtualKey_None;
        m_originalKeyCode = wsy::VirtualKey::VirtualKey_None;
    }

    // Destructor
    ~CKeyEventArgs() override
    {
        m_platformKeyCode = wsy::VirtualKey::VirtualKey_None;
        m_originalKeyCode = wsy::VirtualKey::VirtualKey_None;
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_Key(_Out_ DirectUI::VirtualKey* pVirtualKey)
    {
        *pVirtualKey = static_cast<DirectUI::VirtualKey>(m_platformKeyCode);
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT get_OriginalKey(_Out_ DirectUI::VirtualKey* pVirtualKey)
    {
        *pVirtualKey = static_cast<DirectUI::VirtualKey>(m_originalKeyCode);
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT get_DeviceId(_Out_ xstring_ptr* value)
    {
        *value = m_deviceId;
        return S_OK;
    }

    // Shift property.
    bool IsShiftPressed()
    {
        return m_shift;
    }
    _Check_return_ HRESULT get_Shift(_Out_ BOOLEAN* pbValue)
    {
        *pbValue = static_cast<BOOLEAN>(m_shift);
        RRETURN(S_OK);
    }
    _Check_return_ HRESULT put_Shift(_In_ BOOLEAN value)
    {
        m_shift = !!value;
        RRETURN(S_OK);
    }

    // Ctrl property.
    bool IsCtrlPressed() const
    {
        return m_ctrl;
    }
    _Check_return_ HRESULT get_Ctrl(_Out_ BOOLEAN* pbValue)
    {
        *pbValue = static_cast<BOOLEAN>(m_ctrl);
        RRETURN(S_OK);
    }
    _Check_return_ HRESULT put_Ctrl(_In_ BOOLEAN value)
    {
        m_ctrl = !!value;
        RRETURN(S_OK);
    }

    bool IsAltPressed() const
    {
        return m_alt;
    }

    void put_Alt(_In_ BOOLEAN value)
    {
        m_alt = !!value;
    }

    _Check_return_ HRESULT get_HandledShouldNotImpedeTextInput(_Out_ BOOLEAN* pbValue)
    {
        *pbValue = (BOOLEAN)m_handledShouldNotImpedeTextInput;
        RRETURN(S_OK);
    }
    _Check_return_ HRESULT put_HandledShouldNotImpedeTextInput(_In_ BOOLEAN value)
    {
        m_handledShouldNotImpedeTextInput = !!value;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT GetKeyStatus(
        _Out_ PhysicalKeyStatus* pKeyStatus);
    void SetModifierKeys(
        _In_ const XUINT32 modifierKeys);

    wsy::VirtualKey  m_platformKeyCode;
    wsy::VirtualKey  m_originalKeyCode;
    XEDITKEY        m_xEditKey{};
    xstring_ptr     m_deviceId;

    PhysicalKeyStatus m_physicalKeyStatus;

private:
    // Modifier Keys
    bool      m_shift = false;
    bool      m_ctrl = false;
    bool      m_alt = false;
    bool      m_handledShouldNotImpedeTextInput = false;
};
