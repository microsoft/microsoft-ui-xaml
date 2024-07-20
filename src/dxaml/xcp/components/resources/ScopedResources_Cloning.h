// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScopedResources_Types.h"
#include <gsl/span>

namespace Resources { namespace ScopedResources
{
    struct CloneableType
    {
        KnownTypeIndex      index               {};
        uint32_t            customTypeNameHash  {};
        xstring_ptr_storage customTypeName      {};
    };

    _Check_return_ HRESULT CreateOverrideNoRef(
        _In_ const CDependencyObject* const referencingResource,
        const stack_vector_t<FoundOverride>& overrides,
        _Outptr_ CDependencyObject** resultCore,
        _Outptr_ xaml::IDependencyObject** resultFx);

    const gsl::span<const CloneableType> GetCloneableTypes();
} }