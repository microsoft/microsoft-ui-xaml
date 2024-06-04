// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CommandBar.h"
#include "CommandBarElementCollection.h"
#include "StableXbfIndexes.g.h"
#include <RuntimeProfiler.h>

_Check_return_ HRESULT CCommandBar::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    IFC_RETURN(__super::EnterImpl(pNamescopeOwner, params));

    CValue value;
    IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::CommandBar_SecondaryCommands), &value));
    CCommandBarElementCollection* const secondaryCommands = do_pointer_cast<CCommandBarElementCollection>(value.AsObject());
    
    if (secondaryCommands)
    {
        //This is dead enter to register any keyboard accelerators that may be present in the CommandBar SecondaryCommands
        //to the list of live accelerators
        params.fIsForKeyboardAccelerator = TRUE;
        params.fIsLive = FALSE;
        params.fSkipNameRegistration = TRUE;
        params.fUseLayoutRounding = FALSE;
        params.fCoercedIsEnabled = FALSE;

        for (CDependencyObject* secondaryCommand : *secondaryCommands)
        {
            IFC_RETURN(secondaryCommand->Enter(pNamescopeOwner, params));
        }
    }
    
    return S_OK;
}

_Check_return_ HRESULT CCommandBar::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(__super::LeaveImpl(pNamescopeOwner, params));

    CValue value;
    IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::CommandBar_SecondaryCommands), &value));
    CCommandBarElementCollection* const secondaryCommands = do_pointer_cast<CCommandBarElementCollection>(value.AsObject());

    if (secondaryCommands)
    {
        //This is a dead leave to remove any keyboard accelerators that may be present in the CommandBar SecondaryCommands
        //from the list of live accelerators
        params.fIsForKeyboardAccelerator = TRUE;
        params.fIsLive = FALSE;
        params.fSkipNameRegistration = TRUE;
        params.fUseLayoutRounding = FALSE;
        params.fCoercedIsEnabled = FALSE;

        for (CDependencyObject* secondaryCommand : *secondaryCommands)
        {
            IFC_RETURN(secondaryCommand->Leave(pNamescopeOwner, params));
        }
    }

    return S_OK;
}