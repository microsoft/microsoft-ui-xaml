// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GettingFocusEventArgs.g.h"
#include "GettingFocusEventArgs.h"

_Check_return_ HRESULT DirectUI::GettingFocusEventArgs::TryCancelImpl(_Out_ BOOLEAN* pReturnValue)
{
    CEventArgs* pCoreEventArgsNoRef = nullptr;
    IFC_RETURN(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    const bool canCancelFocus = static_cast<CGettingFocusEventArgs*>(pCoreEventArgsNoRef)->CanCancelOrRedirectFocus();

    if (canCancelFocus)
    {
        IFCFAILFAST(put_Cancel(TRUE));
    }

    *pReturnValue = canCancelFocus;
    return S_OK;
}

_Check_return_ HRESULT DirectUI::GettingFocusEventArgs::TrySetNewFocusedElementImpl(_In_ xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pReturnValue)
{
    CEventArgs* pCoreEventArgsNoRef = nullptr;
    IFC_RETURN(GetCorePeerNoRefWithValidation(&pCoreEventArgsNoRef));
    const bool canRedirectFocus = static_cast<CGettingFocusEventArgs*>(pCoreEventArgsNoRef)->CanCancelOrRedirectFocus();

    if (canRedirectFocus)
    {
        IFCFAILFAST(put_NewFocusedElement(pElement));
    }

    *pReturnValue = canRedirectFocus;
    return S_OK;
}