// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <DOCollection.h>
#include <weakref_ptr.h>

class CVisualStateCollection final : public CDOCollection
{
public:
    // Creation method
    DECLARE_CREATE(CVisualStateCollection);

    KnownTypeIndex GetTypeIndex() const override;

    // this collection is used as an on-demand property and
    // it will be hard to find a straight line to its parent
    // so use this flag to maintain namescope resolution.
    bool ShouldEnsureNameResolution() override { return true; }
private:
    CVisualStateCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}
};
