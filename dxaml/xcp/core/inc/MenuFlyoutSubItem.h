// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CMenuFlyoutItemBase.g.h"

class CMenuFlyoutSubItem: public CMenuFlyoutItemBase
{
protected:
    CMenuFlyoutSubItem(_In_ CCoreServices *pCore)
        : CMenuFlyoutItemBase(pCore)
    {
        SetIsCustomType();
    }

    ~CMenuFlyoutSubItem() override
    {
    }

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) final;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) final;

public:
    DECLARE_CREATE(CMenuFlyoutSubItem);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::MenuFlyoutSubItem;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};
