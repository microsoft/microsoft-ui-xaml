// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CModernCollectionBasePanel.g.h"

class CItemsStackPanel: public CModernCollectionBasePanel
{
protected:
    CItemsStackPanel(_In_ CCoreServices *pCore)
        : CModernCollectionBasePanel(pCore)
    {
        SetIsCustomType();
    }

    ~CItemsStackPanel() override
    {
    }

public:
    DECLARE_CREATE(CItemsStackPanel);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::ItemsStackPanel;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};
