// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCollectionViewSource final : public CDependencyObject
{
protected:
    CCollectionViewSource(CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
    DECLARE_CREATE(CCollectionViewSource);

    // overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CCollectionViewSource>::Index;
    }

    // The CollectionViewSource needs to participate in the managed
    // tree so its managed peer is not lost
    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};
