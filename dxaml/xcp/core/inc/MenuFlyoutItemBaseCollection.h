// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <FxCallbacks.h>

// This class is the native peer of MenuFlyoutItemBaseCollection
class CMenuFlyoutItemBaseCollection final : public CDependencyObjectCollection
{
private:
    CMenuFlyoutItemBaseCollection(_In_ CCoreServices *pCore)
        : CDependencyObjectCollection(pCore)
    {}

public:
    DECLARE_CREATE(CMenuFlyoutItemBaseCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CMenuFlyoutItemBaseCollection>::Index;
    }

protected:
    // Use AddPeerReferenceToItem, whether or not acting as a parent to items.
    // MenuFlyoutItemBaseCollection needs to keep a ref to MenuFlyoutItemBase so the MenuFlyoutItemBase will be still alive even if it is removed from Panel.Children
    bool ShouldKeepPeerReference() override { return true; }

    _Check_return_ HRESULT ValidateItem(_In_ CDependencyObject *pObject) override
    {
        IFC_RETURN(CDependencyObjectCollection::ValidateItem(pObject));
        IFC_RETURN(FxCallbacks::MenuFlyoutItemBaseCollection_ValidateItem(pObject));

        return S_OK;
    }
};
