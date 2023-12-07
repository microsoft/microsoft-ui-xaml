// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <stack_vector.h>

#define HASHED_XSTRING_PTR_STORAGE(str) ct_hash::hash(str), XSTRING_PTR_STORAGE(str)

class xstring_ptr;
class CDependencyObject;
class CResourceDictionary;
enum class KnownPropertyIndex : UINT16;

namespace Resources { namespace ScopedResources
{
    static constexpr size_t DefaultVectorSize = 16;

    template <typename T>
    using stack_vector_t = typename Jupiter::stack_vector<T, DefaultVectorSize>::vector_t;

    struct FoundOverride
    {
        CDependencyObject*      value           {};
        CResourceDictionary*    dictionary      {};
        uint16_t                distance        {};
        KnownPropertyIndex      propertyIndex   {};
    };
} }