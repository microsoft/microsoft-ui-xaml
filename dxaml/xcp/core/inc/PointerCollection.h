// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <DOCollection.h>

class CPointerCollection final : public CDOCollection
{
protected:
    CPointerCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:
    DECLARE_CREATE(CPointerCollection);

public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPointerCollection>::Index;
    }
};
