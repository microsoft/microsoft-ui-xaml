// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Class.h"

namespace winrt::StaticLibInRuntimeComponent::implementation
{
    int32_t Class::Dummy()
    {
        throw hresult_not_implemented();
    }

    void Class::Dummy(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
