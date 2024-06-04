// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Flyout.h"
#include "StableXbfIndexes.g.h"
#include <RuntimeProfiler.h>

_Check_return_ HRESULT CMenuFlyout::KeyboardAcceleratorFlyoutItemEnter(
    _In_ CDependencyObject* element,
    _In_ CDependencyObject *pNamescopeOwner,
    _In_ KnownPropertyIndex collectionPropertyIndex,
    _In_ EnterParams params)
{
    CValue value;
    IFC_RETURN(element->GetValueByIndex(collectionPropertyIndex, &value));

    if (CMenuFlyoutItemBaseCollection* const items = do_pointer_cast<CMenuFlyoutItemBaseCollection>(value.AsObject()))
    {
        // This is a dead enter to register any keyboard accelerators that may be present in the MenuFlyout items
        // to the list of live accelerators
        params.fIsForKeyboardAccelerator = TRUE;
        params.fIsLive = FALSE;
        params.fSkipNameRegistration = TRUE;
        params.fUseLayoutRounding = FALSE;
        params.fCoercedIsEnabled = FALSE;

        for (CDependencyObject* item : *items)
        {
            IFC_RETURN(item->Enter(pNamescopeOwner, params));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CMenuFlyout::KeyboardAcceleratorFlyoutItemLeave(
    _In_ CDependencyObject* element,
    _In_ CDependencyObject *pNamescopeOwner,
    _In_ KnownPropertyIndex collectionPropertyIndex,
    _In_ LeaveParams params)
{
    CValue value;
    IFC_RETURN(element->GetValueByIndex(collectionPropertyIndex, &value));

    if (CMenuFlyoutItemBaseCollection* const items = do_pointer_cast<CMenuFlyoutItemBaseCollection>(value.AsObject()))
    {
        // This is a dead leave to remove any keyboard accelerators that may be present in the MenuFlyout items
        // from the list of live accelerators
        params.fIsForKeyboardAccelerator = TRUE;
        params.fIsLive = FALSE;
        params.fSkipNameRegistration = TRUE;
        params.fUseLayoutRounding = FALSE;
        params.fCoercedIsEnabled = FALSE;

        for (CDependencyObject* item : *items)
        {
            IFC_RETURN(item->Leave(pNamescopeOwner, params));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CMenuFlyout::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    IFC_RETURN(__super::EnterImpl(pNamescopeOwner, params));
    IFC_RETURN(KeyboardAcceleratorFlyoutItemEnter(this, pNamescopeOwner, KnownPropertyIndex::MenuFlyout_Items, params));

    return S_OK;
}

_Check_return_ HRESULT CMenuFlyout::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(__super::LeaveImpl(pNamescopeOwner, params));
    IFC_RETURN(KeyboardAcceleratorFlyoutItemLeave(this, pNamescopeOwner, KnownPropertyIndex::MenuFlyout_Items, params));

    return S_OK;
}