// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the B class.
//

#pragma once

#include "B.g.h"

namespace winrt::LinkedMDControlsCppWinRT::implementation
{
    struct B : BT<B>
    {
        B();
        hstring StringPropertyOnB();
    };
}

namespace winrt::LinkedMDControlsCppWinRT::factory_implementation
{
    struct B : BT<B, implementation::B>
    {
    };
}
