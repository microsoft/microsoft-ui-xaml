// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "Class.g.h"

namespace winrt::CppWinRTComponent::implementation
{
    struct Class : ClassT<Class>
    {
        Class() = default;

        int32_t MyProperty();
        void MyProperty(int32_t value);
    };
}

namespace winrt::CppWinRTComponent::factory_implementation
{
    struct Class : ClassT<Class, implementation::Class>
    {
    };
}
