// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "EffectiveViewportChangedEventArgs.h"

_Check_return_ HRESULT CEffectiveViewportChangedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** peer)
{
    RRETURN(DirectUI::OnFrameworkCreateEffectiveViewportChangedEventArgs(this, peer));
}

_Check_return_ HRESULT CEffectiveViewportChangedEventArgs::get_EffectiveViewport(_Out_ wf::Rect* value)
{
    value->X = m_effectiveViewport.X;
    value->Y = m_effectiveViewport.Y;
    value->Width = m_effectiveViewport.Width;
    value->Height = m_effectiveViewport.Height;
    return S_OK;
}

_Check_return_ HRESULT CEffectiveViewportChangedEventArgs::get_MaxViewport(_Out_ wf::Rect* value)
{
    value->X = m_maxViewport.X;
    value->Y = m_maxViewport.Y;
    value->Width = m_maxViewport.Width;
    value->Height = m_maxViewport.Height;
    return S_OK;
}

_Check_return_ HRESULT CEffectiveViewportChangedEventArgs::get_BringIntoViewDistanceX(_Out_ DOUBLE* value)
{
    *value = m_bringIntoViewDistanceX;
    return S_OK;
}

_Check_return_ HRESULT CEffectiveViewportChangedEventArgs::get_BringIntoViewDistanceY(_Out_ DOUBLE* value)
{
    *value = m_bringIntoViewDistanceY;
    return S_OK;
}