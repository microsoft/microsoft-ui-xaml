// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include "ptr_base.h"

namespace ext
{
    template <typename T, template <typename> typename ExtData1, template <typename> typename ExtData0>
    using shared_ptr = details::smart_ptr<std::shared_ptr<T>, T, 0, details::heap_alloc<ExtData1>, details::typealigned_alloc<ExtData0, T>>;
}