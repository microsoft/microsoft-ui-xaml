// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "FocusManagerLostFocusEventArgs.h"

_Check_return_ HRESULT CFocusManagerLostFocusEventArgs::get_OldFocusedElement(_Outptr_ CDependencyObject** oldFocusedElement)
{
    m_oldFocusedElement.CopyTo(oldFocusedElement);
    return S_OK;
}

_Check_return_ HRESULT CFocusManagerLostFocusEventArgs::get_CorrelationId(_Out_ GUID* pValue)
{
    *pValue = m_correlationId;
    return S_OK;
}

_Check_return_ HRESULT CFocusManagerLostFocusEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    IFC_RETURN(DirectUI::OnFrameworkCreateFocusManagerLostFocusEventArgs(this, ppPeer));

    return S_OK;
}