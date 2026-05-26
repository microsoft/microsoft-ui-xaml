// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PageWithTemplatedParentBinding.g.h"
#include "ValueConverters.h" // include this here so XamlTypeInfo.g.cpp knows about the value converters

namespace Tests { namespace Tools { namespace Shared {

    public ref class PageWithTemplatedParentBinding sealed
    {
    public:
        PageWithTemplatedParentBinding();
    };
} } }
