// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xref_ptr.h>
#include "ptr_base.h"

namespace ext
{
    template <typename T, template <typename> typename ExtData>
    using xref_ptr = details::smart_ptr<xref_ptr<T>, T, 0, details::heap_alloc<ExtData>>;
}