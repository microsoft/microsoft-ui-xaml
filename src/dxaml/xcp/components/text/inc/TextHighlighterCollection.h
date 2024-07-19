// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DOCollection.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

class CTextHighlighterCollection : public CDOCollection
{
public:
    DECLARE_CREATE(CTextHighlighterCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextHighlighterCollection>::Index;
    }

    _Check_return_ bool NeedsOwnerInfo() override { return true; }

    void OnCollectionChanged();

protected:
    _Check_return_ HRESULT OnAddToCollection(
        _In_ CDependencyObject* dependencyObject
        ) override;
    _Check_return_ HRESULT OnRemoveFromCollection(
        _In_ CDependencyObject *dependencyObject,
        int32_t previousIndex)
        override;
    _Check_return_ HRESULT OnClear() override;

private:
    explicit CTextHighlighterCollection(_In_ CCoreServices* core)
        : CDOCollection(core)
    {}
};