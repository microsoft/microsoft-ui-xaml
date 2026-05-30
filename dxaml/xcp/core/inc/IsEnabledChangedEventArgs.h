// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CIsEnabledChangedEventArgs final : public CEventArgs
{
public:
    CIsEnabledChangedEventArgs()
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    // OldValue property.
    _Check_return_ HRESULT get_OldValue(_Out_ BOOLEAN* pbValue)
    {
        *pbValue = (BOOLEAN)m_fOldValue;
        RRETURN(S_OK);
    }
    _Check_return_ HRESULT put_OldValue(_In_ BOOLEAN value)
    {
        m_fOldValue = !!value;
        RRETURN(S_OK);
    }

    // NewValue property.
    _Check_return_ HRESULT get_NewValue(_Out_ BOOLEAN* pbValue)
    {
        *pbValue = (BOOLEAN)m_fNewValue;
        RRETURN(S_OK);
    }
    _Check_return_ HRESULT put_NewValue(_In_ BOOLEAN value)
    {
        m_fNewValue = !!value;
        RRETURN(S_OK);
    }

    bool    m_fOldValue{};             // Old value of IsEnabled property
    bool    m_fNewValue{};             // New value of IsEnabled property
};
