// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "FocusManagerGotFocusEventArgs.h"

_Check_return_ HRESULT CFocusManagerGotFocusEventArgs::get_NewFocusedElement(_Outptr_ CDependencyObject** newFocusedElement)
{
    m_newFocusedElement.CopyTo(newFocusedElement);
    return S_OK;
}

_Check_return_ HRESULT CFocusManagerGotFocusEventArgs::get_CorrelationId(_Out_ GUID* pValue)
{
    *pValue = m_correlationId;
    return S_OK;
}

_Check_return_ HRESULT CFocusManagerGotFocusEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    IFC_RETURN(DirectUI::OnFrameworkCreateFocusManagerGotFocusEventArgs(this, ppPeer));

    return S_OK;
}