// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDownloadProgressEventArgs final : public CEventArgs
{
public:
    CDownloadProgressEventArgs()
    {
        m_downloadProgress = 0;
        m_bHandled = false;
    }

    // Destructor
    ~CDownloadProgressEventArgs() override
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_Progress(_Out_ INT* pbValue)
    {
        // NOTE: m_downloadProgress is in range 0.0f to 1.0f but the external
        //       API exposes this as a 0.0f to 100.0f range.
        *pbValue = (INT)(m_downloadProgress * 100.0f);
        RRETURN(S_OK);
    }
    _Check_return_ HRESULT put_Progress(_In_ INT value)
    {
        m_downloadProgress = (XFLOAT)(value / 100.0f);
        RRETURN(S_OK);
    }

    XFLOAT m_downloadProgress;
    XINT32 m_bHandled;
};
