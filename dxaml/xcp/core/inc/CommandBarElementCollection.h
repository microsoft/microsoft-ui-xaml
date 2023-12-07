// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Native peer of CommandBar.PrimaryCommands & SecondaryCommands

#pragma once

#include <FxCallbacks.h>

// This class is the native peer of CommandBar.PrimaryCommands & SecondaryCommands
class CCommandBarElementCollection final : public CDependencyObjectCollection
{
private:
    CCommandBarElementCollection(_In_ CCoreServices *pCore)
        : CDependencyObjectCollection(pCore)
    {
    }

public:
    DECLARE_CREATE(CCommandBarElementCollection);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CCommandBarElementCollection>::Index;
    }

protected:
    // Use AddPeerReferenceToItem, whether or not acting as a parent to items.
    bool ShouldKeepPeerReference() override { return true; }

    _Check_return_ HRESULT ValidateItem(_In_ CDependencyObject *pObject) override
    {
        IFC_RETURN(CDependencyObjectCollection::ValidateItem(pObject));

        IFC_RETURN(FxCallbacks::CommandBarElementCollection_ValidateItem(pObject));

        return S_OK;
    }

};
