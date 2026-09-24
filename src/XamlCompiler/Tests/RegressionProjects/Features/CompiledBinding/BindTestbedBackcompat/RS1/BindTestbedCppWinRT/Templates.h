// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "Templates.g.h"

namespace winrt::BindTestbed::implementation
{
    struct Templates : TemplatesT<Templates>
    {
        Templates();
        hstring Dummy() { return L""; }
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct Templates : TemplatesT<Templates, implementation::Templates>
    {
    };
}
