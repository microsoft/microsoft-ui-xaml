// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NoParentShareableDependencyObject.h>

// A proxy object to support null-valued keyed resources in ResourceDictionary.
// Conceptually similar to ExternalObjectReference, but separate to avoid overloading
// the latter

class CNullKeyedResource final
    : public CNoParentShareableDependencyObject
{
    CNullKeyedResource(
        _In_ CCoreServices* core)
        : CNoParentShareableDependencyObject(core)
    {}

public:
    ~CNullKeyedResource() override = default;

    DECLARE_CREATE(CNullKeyedResource);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::NullKeyedResource;
    }
};