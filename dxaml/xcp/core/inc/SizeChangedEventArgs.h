// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CSizeChangedEventArgs final : public CRoutedEventArgs
{
public:
    CSizeChangedEventArgs(_In_ const XSIZEF& previousSize, _In_ const XSIZEF& newSize)
        : m_previousSize(previousSize), m_newSize(newSize)
    {}

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_PreviousSize(_Out_ wf::Size* pValue)
    {
        pValue->Width = m_previousSize.width;
        pValue->Height = m_previousSize.height;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT get_NewSize(_Out_ wf::Size* pValue)
    {
        pValue->Width = m_newSize.width;
        pValue->Height = m_newSize.height;
        RRETURN(S_OK);
    }

    XSIZEF m_previousSize;
    XSIZEF m_newSize;
};
