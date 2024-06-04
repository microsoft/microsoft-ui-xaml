// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CTextControlPasteEventArgs : public CEventArgs
{
public:
    CTextControlPasteEventArgs()
    {
        m_bHandled = FALSE;
    }

    // Destructor
    ~CTextControlPasteEventArgs() override
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    // Handled property.
    _Check_return_ HRESULT get_Handled(_Out_ BOOLEAN* pbValue)
    {
        *pbValue = (BOOLEAN)m_bHandled;
        RRETURN(S_OK);
    }
    _Check_return_ HRESULT put_Handled(_In_ BOOLEAN value)
    {
        m_bHandled = !!value;
        RRETURN(S_OK);
    }

    // CTextControlPasteEventArgs fields
    bool m_bHandled;
};

