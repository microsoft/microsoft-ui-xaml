// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <MetadataAPI.h>

struct ICollectionChangeCallback
{
    virtual ~ICollectionChangeCallback() {}

    virtual _Check_return_ HRESULT ElementInserted(
        _In_ UINT32 indexInChildrenCollection) = 0;

    virtual _Check_return_ HRESULT ElementRemoved(
        _In_ UINT32 indexInChildrenCollection) = 0;

    virtual _Check_return_ HRESULT ElementMoved(
        _In_ UINT32 oldIndexInChildrenCollection,
        _In_ UINT32 newIndexInChildrenCollection) = 0;

    virtual _Check_return_ HRESULT CollectionCleared() = 0;
};
