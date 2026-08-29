// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CSvgImageSourceFailedEventArgs : public CEventArgs
{
public:
    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    // Handled property.
    _Check_return_ HRESULT get_Status(_Out_ DirectUI::SvgImageSourceLoadStatus* pbValue)
    {
        *pbValue = m_status;
        RRETURN(S_OK);
    }
    _Check_return_ HRESULT put_Status(_In_ DirectUI::SvgImageSourceLoadStatus value)
    {
        m_status = value;
        RRETURN(S_OK);
    }

    // CTextControlPasteEventArgs fields
    DirectUI::SvgImageSourceLoadStatus m_status = DirectUI::SvgImageSourceLoadStatus::Success;
};

