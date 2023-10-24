// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CTextControlCopyingToClipboardEventArgs final : public CEventArgs
{
public:
    CTextControlCopyingToClipboardEventArgs() = default;

    // Destructor
    ~CTextControlCopyingToClipboardEventArgs() override = default;

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    // Handled property.
    _Check_return_ HRESULT get_Handled(_Out_ BOOLEAN* pbValue)
    {
        *pbValue = (BOOLEAN)m_bHandled;
        return S_OK;
    }
    _Check_return_ HRESULT put_Handled(_In_ BOOLEAN value)
    {
        m_bHandled = !!value;
        return S_OK;
    }

    // CTextControlCopyingToClipboardEventArgs fields
    bool m_bHandled = false;
};

