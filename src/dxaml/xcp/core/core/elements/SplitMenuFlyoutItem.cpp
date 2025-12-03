// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SplitMenuFlyoutItem.h"
#include "MenuFlyout.h"
#include "StableXbfIndexes.g.h"

_Check_return_ HRESULT CSplitMenuFlyoutItem::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params)
{
    IFC_RETURN(__super::EnterImpl(pNamescopeOwner, params));
    IFC_RETURN(CMenuFlyout::KeyboardAcceleratorFlyoutItemEnter(this, pNamescopeOwner, KnownPropertyIndex::SplitMenuFlyoutItem_Items, params));

    return S_OK;
}

_Check_return_ HRESULT CSplitMenuFlyoutItem::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params)
{
    IFC_RETURN(__super::LeaveImpl(pNamescopeOwner, params));
    IFC_RETURN(CMenuFlyout::KeyboardAcceleratorFlyoutItemLeave(this, pNamescopeOwner, KnownPropertyIndex::SplitMenuFlyoutItem_Items, params));

    return S_OK;
}