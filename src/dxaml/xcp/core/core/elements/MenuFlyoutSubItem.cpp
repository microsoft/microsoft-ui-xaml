// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MenuFlyoutSubItem.h"
#include "MenuFlyout.h"
#include "StableXbfIndexes.g.h"

_Check_return_ HRESULT CMenuFlyoutSubItem::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    IFC_RETURN(__super::EnterImpl(pNamescopeOwner, params));
    IFC_RETURN(CMenuFlyout::KeyboardAcceleratorFlyoutItemEnter(this, pNamescopeOwner, KnownPropertyIndex::MenuFlyoutSubItem_Items, params));

    return S_OK;
}

_Check_return_ HRESULT CMenuFlyoutSubItem::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(__super::LeaveImpl(pNamescopeOwner, params));
    IFC_RETURN(CMenuFlyout::KeyboardAcceleratorFlyoutItemLeave(this, pNamescopeOwner, KnownPropertyIndex::MenuFlyoutSubItem_Items, params));

    return S_OK;
}