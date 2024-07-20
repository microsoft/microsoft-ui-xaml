// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Activators.g.h"
#include "FocusedElementRemovedEventArgs.h"

_Check_return_ HRESULT CFocusedElementRemovedEventArgs::get_OldFocusedElement(_Outptr_ CDependencyObject** oldFocusedElement)
{
    m_oldFocusedElement.CopyTo(oldFocusedElement);
    
    return S_OK;
}

_Check_return_ HRESULT CFocusedElementRemovedEventArgs::get_NewFocusedElement(_Outptr_ CDependencyObject** newFocusedElement)
{
    m_newFocusedElement.CopyTo(newFocusedElement);

    return S_OK;
}

_Check_return_ HRESULT CFocusedElementRemovedEventArgs::put_NewFocusedElement(_In_ CDependencyObject* newFocusedElement)
{
    m_newFocusedElement = newFocusedElement;

    return S_OK;
}

_Check_return_ HRESULT CFocusedElementRemovedEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    IFC_RETURN(DirectUI::OnFrameworkCreateFocusedElementRemovedEventArgs(this, ppPeer));

    return S_OK;
}