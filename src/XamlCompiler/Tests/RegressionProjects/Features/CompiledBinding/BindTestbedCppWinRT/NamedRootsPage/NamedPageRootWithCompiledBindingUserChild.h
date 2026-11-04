// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "NamedRootsPage/NamedPageRootWithCompiledBindingUserChild.g.h"

namespace winrt::BindTestbed::NamedRootsPage::implementation
{
    struct NamedPageRootWithCompiledBindingUserChild : NamedPageRootWithCompiledBindingUserChildT<NamedPageRootWithCompiledBindingUserChild>
    {
        NamedPageRootWithCompiledBindingUserChild();
    };
}

namespace winrt::BindTestbed::NamedRootsPage::factory_implementation
{
    struct NamedPageRootWithCompiledBindingUserChild : NamedPageRootWithCompiledBindingUserChildT<NamedPageRootWithCompiledBindingUserChild, implementation::NamedPageRootWithCompiledBindingUserChild>
    {
    };
}
