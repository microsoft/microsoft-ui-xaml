// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

_Check_return_ HRESULT CStaggerFunctionBase::GetTransitionDelayValues(
    _In_ CDependencyObject* pStaggerFunction,
    _In_ XUINT32 cElements,
    _In_reads_(cElements) CUIElement** ppElements,
    _In_reads_(cElements) XRECTF* pBounds, 
    _Out_writes_(cElements) XFLOAT* pDelays)
{
    CCoreServices* pServices = NULL; 
    IFCEXPECT_RETURN(pDelays);
    IFCEXPECT_RETURN(ppElements);
    IFCEXPECT_RETURN(pBounds);
    IFCEXPECT_RETURN(pStaggerFunction);
    pServices = pStaggerFunction->GetContext();
    IFCEXPECT_RETURN(pServices);

    {
        CREATEPARAMETERS cp(pServices);        
        CStaggerFunctionBase* pStagger = do_pointer_cast<CStaggerFunctionBase>(pStaggerFunction);

        if (pStagger && !pStagger->IsCustomType())
        {
            IFC_RETURN(pStagger->GetTransitionDelays(
                cElements,
                ppElements,
                pBounds,
                pDelays));
        }
        else
        {
            // this is a custom type
            
            //NOTE: For a user managed stagger function or a custom class we need to call it with reverse PInvoke.
            //      This call is made synchronously and could cause reentrancy issues. Stagger functions are meant
            //      to be stateless so should ideally not change any state.
            IFC_RETURN(FxCallbacks::XcpImports_StaggerManaged(
                pStaggerFunction,
                cElements,
                ppElements,
                pBounds,
                pDelays
                ));
        }
    }

    return S_OK;
}
