// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CMenuFlyoutItem.g.h"
// #include <DeclareMacros.h>
// #include <Indexes.g.h>
// #include <minxcptypes.h>

class CSplitMenuFlyoutItem : public CMenuFlyoutItem
{
protected:
    CSplitMenuFlyoutItem(_In_ CCoreServices *pCore)
        : CMenuFlyoutItem(pCore)
    {
        SetIsCustomType();
    }

    ~CSplitMenuFlyoutItem() override = default;

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params) final;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params) final;


public:
    DECLARE_CREATE(CSplitMenuFlyoutItem);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::SplitMenuFlyoutItem;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};
