// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the T class.
//

#pragma once

#include "T.g.h"

namespace winrt::LinkedMDSubControlsCppWinRT::implementation
{
    struct T : TT<T>
    {
        T();
        hstring StringPropertyOnT();
    };
}

namespace winrt::LinkedMDSubControlsCppWinRT::factory_implementation
{
    struct T : TT<T, implementation::T>
    {
    };
}
