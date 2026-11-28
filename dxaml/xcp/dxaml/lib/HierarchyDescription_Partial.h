// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "HierarchyDescription.g.h"

namespace DirectUI
{
    class VectorCollectionView;

    // How a view builds its tree (ICollectionViewHierarchy). A view that uses the description rebuilds when it changes,
    // as with a GroupDescription.
    PARTIAL_CLASS(HierarchyDescription)
    {
    public:
        _Check_return_ HRESULT AddOwner(_In_ VectorCollectionView* pView);
        _Check_return_ HRESULT NotifyOwners();

    private:
        std::vector<ctl::WeakRefPtr> m_owners;
    };
}
