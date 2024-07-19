// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Flyout.h"
#include "StableXbfIndexes.g.h"
#include <RuntimeProfiler.h>

_Check_return_ HRESULT CFlyout::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    IFC_RETURN(__super::EnterImpl(pNamescopeOwner, params));

    CValue value;
    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::Flyout_Content, &value));
    CUIElement* const pContent = do_pointer_cast<CUIElement>(value.AsObject());
    
    if (pContent)
    {
        //This is a dead enter to register any keyboard accelerators that may be present in the Flyout Content 
        //to the list of live accelerators
        params.fIsForKeyboardAccelerator = TRUE;
        params.fIsLive = FALSE;
        params.fSkipNameRegistration = TRUE;
        params.fUseLayoutRounding = FALSE;
        params.fCoercedIsEnabled = FALSE;
        IFC_RETURN(pContent->Enter(pNamescopeOwner, params));
    }

    return S_OK;
}

_Check_return_ HRESULT CFlyout::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(__super::LeaveImpl(pNamescopeOwner, params));

    CValue value;
    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::Flyout_Content, &value));
    CUIElement* const pContent = do_pointer_cast<CUIElement>(value.AsObject());
    if (pContent)
    {
        //This is a dead leave to remove any keyboard accelerators that may be present in the Flyout Content
        //from the list of live accelerators
        params.fIsForKeyboardAccelerator = TRUE;
        params.fIsLive = FALSE;
        params.fSkipNameRegistration = TRUE;
        params.fUseLayoutRounding = FALSE;
        params.fCoercedIsEnabled = FALSE;
        IFC_RETURN(pContent->Leave(pNamescopeOwner, params));
    }
 
    return S_OK;
}