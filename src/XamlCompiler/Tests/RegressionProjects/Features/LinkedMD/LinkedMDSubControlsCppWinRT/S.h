// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the S class.
//

#pragma once

#include "S.g.h"
#include "T.h"

namespace winrt::LinkedMDSubControlsCppWinRT::implementation
{
    struct S : ST<S>
    {
        S();

        hstring StringPropertyOnS();
        ::winrt::LinkedMDSubControlsCppWinRT::T TPropertyOnS();
    };
}

namespace winrt::LinkedMDSubControlsCppWinRT::factory_implementation
{
    struct S : ST<S, implementation::S>
    {
    };
}
