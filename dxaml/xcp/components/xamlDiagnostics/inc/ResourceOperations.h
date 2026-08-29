// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/Microsoft.UI.Xaml.h>

namespace Diagnostics
{
    struct ResourceGraphKey;
    struct ResourceGraphKeyWithParent;

    namespace ResourceOperations
    {
        // Common helper methods for when adding/removing items from a dictionary, or
        // when adding/removing dictionaries from a collection
        _Check_return_ HRESULT UpdateDependentItemsOnAdd(
            const Diagnostics::ResourceGraphKey& currentResolution,
            _In_ xaml::IResourceDictionary* newDictionary);

        _Check_return_ HRESULT UpdateDependentItemsOnRemove(
            const Diagnostics::ResourceGraphKeyWithParent& removedGraphKey);

    }
}