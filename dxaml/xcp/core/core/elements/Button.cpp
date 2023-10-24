// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Button.h"
#include <MenuFlyout.h>
#include "StableXbfIndexes.g.h"
#include <RuntimeProfiler.h>

_Check_return_ HRESULT CButton::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    CValue value;

    IFC_RETURN(__super::EnterImpl(pNamescopeOwner, params));

    if (IsPropertyDefault(GetPropertyByIndexInline(KnownPropertyIndex::Button_Flyout)))
    {
        return S_OK;
    }
    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::Button_Flyout, &value));
    CFlyoutBase *pFlyoutBase = do_pointer_cast<CFlyoutBase>(value.AsObject());
    if (pFlyoutBase)
    {
        
        IFC_RETURN(pFlyoutBase->Enter(pNamescopeOwner, params));
    }
    return S_OK;
}

_Check_return_ HRESULT CButton::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    CValue value;

    IFC_RETURN(__super::LeaveImpl(pNamescopeOwner, params));

    if (IsPropertyDefault(GetPropertyByIndexInline(KnownPropertyIndex::Button_Flyout)))
    {
        return S_OK;
    }
    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::Button_Flyout, &value));
    CFlyoutBase *pFlyoutBase = do_pointer_cast<CFlyoutBase>(value.AsObject());
    if (pFlyoutBase)
    {
        
        IFC_RETURN(pFlyoutBase->Leave(pNamescopeOwner, params));
    }
    return S_OK;
}