// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xref_ptr.h>
#include "EventArgs.h"

class CEffectiveViewportChangedEventArgs final : public CEventArgs
{
public:
    CEffectiveViewportChangedEventArgs(
        XRECTF effectiveViewport,
        XRECTF maxViewport,
        DOUBLE bringIntoViewDistanceX,
        DOUBLE bringIntoViewDistanceY)
        : m_effectiveViewport(effectiveViewport)
        , m_maxViewport(maxViewport)
        , m_bringIntoViewDistanceX(bringIntoViewDistanceX)
        , m_bringIntoViewDistanceY(bringIntoViewDistanceY)
    {
    }

    _Check_return_ HRESULT get_EffectiveViewport(_Out_ wf::Rect* value);
    _Check_return_ HRESULT get_MaxViewport(_Out_ wf::Rect* value);
    _Check_return_ HRESULT get_BringIntoViewDistanceX(_Out_ DOUBLE* value);
    _Check_return_ HRESULT get_BringIntoViewDistanceY(_Out_ DOUBLE* value);

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** peer) override;

private:
    XRECTF m_effectiveViewport;
    XRECTF m_maxViewport;
    DOUBLE m_bringIntoViewDistanceX;
    DOUBLE m_bringIntoViewDistanceY;
};
