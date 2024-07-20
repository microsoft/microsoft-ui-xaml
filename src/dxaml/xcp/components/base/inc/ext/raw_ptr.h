// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ptr_base.h"

namespace ext
{
    template <typename T, template <typename> typename ExtData>
    using raw_heap_ptr = details::raw_ptr<T, details::heap_alloc<ExtData>>;

    template <typename T, template <typename> typename ExtData>
    using raw_ptr = details::raw_ptr<T, details::typealigned_alloc<ExtData, void*>>;
}