// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCandidateWindowBoundsChangedEventArgs final : public CEventArgs
{
public:
    CCandidateWindowBoundsChangedEventArgs()
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_Bounds(_Out_ wf::Rect* pValue)
    {
        pValue->X = m_bounds.X;
        pValue->Y = m_bounds.Y;
        pValue->Width = m_bounds.Width;
        pValue->Height = m_bounds.Height;
        return S_OK;
    }

    _Check_return_ HRESULT put_Bounds(_In_ wf::Rect value)
    {
        m_bounds.X = value.X;
        m_bounds.Y = value.Y;
        m_bounds.Width = value.Width;
        m_bounds.Height = value.Height;
        return S_OK;
    }

    XRECTF m_bounds;
};
