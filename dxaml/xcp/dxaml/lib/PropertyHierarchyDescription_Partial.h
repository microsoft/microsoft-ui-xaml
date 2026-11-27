// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PropertyHierarchyDescription.g.h"

namespace DirectUI
{
    // A tree by key and parent key. The view reads ItemKeyPath and ParentKeyPath once per recompute; a change to either
    // rebuilds the views that use it.
    PARTIAL_CLASS(PropertyHierarchyDescription)
    {
    protected:
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;
    };
}
