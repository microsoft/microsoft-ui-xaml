// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "Models/BindItem.g.h"

namespace winrt::Simple::Models::implementation
{
    struct BindItem : BindItemT<BindItem>
    {
        BindItem() = default;

        hstring Name() const
        {
            return L"Item";
        }
    };
}

namespace winrt::Simple::Models::factory_implementation
{
    struct BindItem : BindItemT<BindItem, implementation::BindItem>
    {
    };
}
