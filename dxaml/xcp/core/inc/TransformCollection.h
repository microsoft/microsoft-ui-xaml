// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DOCollection.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

class CTransformCollection final : public CDOCollection
{
    CTransformCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CTransformCollection() // !!! FOR UNIT TESTING ONLY !!!
        : CTransformCollection(nullptr)
    {}
#endif

    DECLARE_CREATE(CTransformCollection);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTransformCollection>::Index;
    }

    // CDOCollection overrides
    _Check_return_ bool NeedsOwnerInfo() override { return true; }

    _Check_return_ HRESULT OnAddToCollection(CDependencyObject *pDO) override
    {
        UNREFERENCED_PARAMETER(pDO);
        OnCollectionChanged();

        return S_OK;
    }

    _Check_return_ HRESULT OnRemoveFromCollection(CDependencyObject *pDO, _In_ XINT32 iPreviousIndex) override
    {
        UNREFERENCED_PARAMETER(pDO);
        OnCollectionChanged();

        return S_OK;
    }

    _Check_return_ HRESULT OnClear() override
    {
        OnCollectionChanged();

        return S_OK;
    }

    _Check_return_ HRESULT CycleCheck(_In_ CDependencyObject *pObject) override;

private:
    void OnCollectionChanged();
};
