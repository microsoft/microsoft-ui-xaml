// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <DOCollection.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>

class CStoryboardCollection final : public CDOCollection
{
public:
    CStoryboardCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

    DECLARE_CREATE(CStoryboardCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CStoryboardCollection>::Index;
    }

    // This dumb override ensures that the collection processes Enter/Leave
    // on children added to it without the annoyance of calling SetOwner
    // yourself.
    bool ShouldEnsureNameResolution() override { return true; }
};
