// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// native peer of Hub.Sections

#pragma once

#include <FxCallbacks.h>

// This class is the native peer of Hub.Sections
class CHubSectionCollection final : public CDependencyObjectCollection
{
private:
    CHubSectionCollection(_In_ CCoreServices *pCore)
        : CDependencyObjectCollection(pCore)
    {}

public:
    DECLARE_CREATE(CHubSectionCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CHubSectionCollection>::Index;
    }

protected:
    // Use AddPeerReferenceToItem, whether or not acting as a parent to items.
    // HubSectionCollection needs to keep a ref to HubSection so the HubSection will be still alive even if it is removed from Panel.Children
    bool ShouldKeepPeerReference() override { return true; }

    _Check_return_ HRESULT ValidateItem(_In_ CDependencyObject *pObject) override
    {
        IFC_RETURN(CDependencyObjectCollection::ValidateItem(pObject));
        IFC_RETURN(FxCallbacks::HubSectionCollection_ValidateItem(pObject));

        return S_OK;
    }
};
