// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the A class.
//

#pragma once

#include "A.g.h"
#include "B.h"

namespace winrt::LinkedMDControlsCppWinRT::implementation
{
    struct A : AT<A>
    {
        A();

        hstring StringPropertyOnA();
        ::winrt::LinkedMDControlsCppWinRT::B BPropertyOnA();
    };
}

namespace winrt::LinkedMDControlsCppWinRT::factory_implementation
{
    struct A : AT<A, implementation::A>
    {
    };
}
