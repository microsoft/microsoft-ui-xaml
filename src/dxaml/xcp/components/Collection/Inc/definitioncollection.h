// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DOCollection.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

class CDefinitionCollectionBase : public CDOCollection
{

protected:
    CDefinitionCollectionBase(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

    _Check_return_ HRESULT OnAddToCollection(_In_ CDependencyObject *pDO) override;
    _Check_return_ HRESULT OnRemoveFromCollection(_In_ CDependencyObject *pDO, _In_ INT32 previousIndex) override;
    _Check_return_ HRESULT OnClear() override;

private:
    void OnCollectionChanged();
};


//------------------------------------------------------------------------
//
//  Class:  CRowDefinitionCollection
//
//  Synopsis:
//      Collection of Grid Rows
//
//------------------------------------------------------------------------
class CRowDefinitionCollection final : public CDefinitionCollectionBase
{
private:
    explicit CRowDefinitionCollection(_In_ CCoreServices *pCore)
        : CDefinitionCollectionBase(pCore)
    {}

public:

    DECLARE_CREATE_WITH_TYPECONVERTER(CRowDefinitionCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRowDefinitionCollection>::Index;
    }

    //enabling setting of owner and layout dirty propagation on collection change
    _Check_return_ bool NeedsOwnerInfo() override
    {
        return true;
    }

private:
    _Check_return_ HRESULT FromString(_In_ CREATEPARAMETERS* pCreate);

};


//------------------------------------------------------------------------
//
//  Class:  CColumnDefinitionCollection
//
//  Synopsis:
//      Collection of Grid Columns
//
//------------------------------------------------------------------------
class CColumnDefinitionCollection final : public CDefinitionCollectionBase
{
private:
    explicit CColumnDefinitionCollection(_In_ CCoreServices *pCore)
        : CDefinitionCollectionBase(pCore)
    {}

public:
    DECLARE_CREATE_WITH_TYPECONVERTER(CColumnDefinitionCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CColumnDefinitionCollection>::Index;
    }

    //enabling setting of owner and layout dirty propagation on collection change
    _Check_return_ bool NeedsOwnerInfo() override
    {
        return true;
    }

private:
    _Check_return_ HRESULT FromString(_In_ CREATEPARAMETERS* pCreate);
};