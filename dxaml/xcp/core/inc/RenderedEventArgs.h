// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CRenderedEventArgs final : public CEventArgs
{
public:
    CRenderedEventArgs() : m_frameDuration()
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_FrameDuration(_In_ wf::TimeSpan* pFrameDuration)
    {
        *pFrameDuration = m_frameDuration;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT put_FrameDuration(_In_ wf::TimeSpan frameDuration)
    {
        m_frameDuration = frameDuration;
        RRETURN(S_OK);
    }

    wf::TimeSpan m_frameDuration;
};

