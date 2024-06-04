// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <DOCollection.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>

class CVisualTransitionCollection final : public CDOCollection
{
public:
    CVisualTransitionCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

    // Creation method
    DECLARE_CREATE(CVisualTransitionCollection);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CVisualTransitionCollection>::Index;
    }

    // This dumb override ensures that the collection processes Enter/Leave
    // on children added to it without the annoyance of calling SetOwner
    // yourself.
    bool ShouldEnsureNameResolution() override { return true; }
};
