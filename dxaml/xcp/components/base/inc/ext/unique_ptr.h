// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include "ptr_base.h"

namespace ext
{
    template <typename T, template <typename> typename ExtData>
    using unique_ptr = details::smart_ptr<std::unique_ptr<T>, T, 0, details::heap_alloc<ExtData>>;

    template <typename T, typename Deleter, template <typename> typename ExtData1, template <typename> typename ExtData0>
    using unique_ptr_del = details::smart_ptr<std::unique_ptr<T, Deleter>, T, 1, details::heap_alloc<ExtData1>, details::typealigned_alloc<ExtData0, void(*)(T*)>>;
}