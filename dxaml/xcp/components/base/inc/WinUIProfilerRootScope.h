// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <VisualTree.h>

namespace WinUIProfiler
{
    namespace Hooks
    {
        inline CContentRoot* AssociatedRoot(CDependencyObject* element)
        {
            // The general content-root helper falls back to CoreWindow for unattached objects.
            auto tree = VisualTree::GetForElementNoRef(element, LookupOptions::NoFallback);
            return tree ? tree->GetContentRootNoRef() : nullptr;
        }
    }
}
