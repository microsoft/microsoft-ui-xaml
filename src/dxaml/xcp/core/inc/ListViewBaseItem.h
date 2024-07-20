// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CSelectorItem.g.h"

class CListViewBaseItem : public CSelectorItem
{
protected:
    CListViewBaseItem(_In_ CCoreServices *pCore)
        : CSelectorItem(pCore)
    {
        SetIsCustomType();
    }

    ~CListViewBaseItem() override
    {
    }

public:
    DECLARE_CREATE(CListViewBaseItem);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::ListViewBaseItem;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};

