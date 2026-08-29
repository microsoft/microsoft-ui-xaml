// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "Class.g.h"

namespace winrt::StaticLibInRuntimeComponent::implementation
{
    struct Class : ClassT<Class>
    {
        Class() = default;

        int32_t Dummy();
        void Dummy(int32_t value);
    };
}

namespace winrt::StaticLibInRuntimeComponent::factory_implementation
{
    struct Class : ClassT<Class, implementation::Class>
    {
    };
}
