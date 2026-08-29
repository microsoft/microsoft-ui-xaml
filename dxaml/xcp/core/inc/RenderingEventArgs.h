// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CRenderingEventArgs final : public CEventArgs
{
public:
    CRenderingEventArgs() : m_renderTime()
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_RenderingTime(_In_ wf::TimeSpan* pRenderTime)
    {
        *pRenderTime = m_renderTime;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT put_RenderingTime(_In_ wf::TimeSpan renderTime)
    {
        m_renderTime = renderTime;
        RRETURN(S_OK);
    }

    wf::TimeSpan m_renderTime;
};

