// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Event args for error based events implementation

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method: UpdateErrorMessage
//
//  Synopsis: 
//      Update the ErrorMessage based on the error code if error message
//      was not set.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CErrorEventArgs::UpdateErrorMessage(XINT32 bReplaceExistingMessage)
{
    IErrorService *pErrorService = NULL;

    // We could have an old message. Delete it if requested.
    if (bReplaceExistingMessage)
    {
        m_strErrorMessage.Reset();
    }

    // If there is no message, get the raw error message from the
    // lookup table.
    if (m_strErrorMessage.IsNullOrEmpty())
    {
        IFC_RETURN(m_pCore->getErrorService(&pErrorService));
        IFCPTR_RETURN(pErrorService);

        SuspendFailFastOnStowedException suspender;
        IFC_RETURN(pErrorService->GetMessageFromErrorCode(m_iErrorCode, &m_strErrorMessage));
    }

    return S_OK;
}
