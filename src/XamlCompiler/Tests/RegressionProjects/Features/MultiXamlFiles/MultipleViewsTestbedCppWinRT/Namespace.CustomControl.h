// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the Namespace class.
//

#pragma once

#include "Namespace.g.h"

namespace winrt::MultipleViewsTestbedCppWinRT::implementation
{
    struct Namespace : NamespaceT<Namespace>
    {
        Namespace();
    };
}

namespace winrt::MultipleViewsTestbedCppWinRT::factory_implementation
{
    struct Namespace : NamespaceT<Namespace, implementation::Namespace>
    {
    };
}
