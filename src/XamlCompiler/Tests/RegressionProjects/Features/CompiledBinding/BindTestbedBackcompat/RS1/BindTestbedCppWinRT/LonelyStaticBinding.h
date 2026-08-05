// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// LonelyStaticBinding.xaml.h
#pragma once
#include "LonelyStaticBinding.g.h"

namespace winrt::BindTestbed::implementation
{
    struct LonelyStaticBinding : LonelyStaticBindingT<LonelyStaticBinding>
    {
        LonelyStaticBinding();
        hstring Dummy() { return L""; }
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct LonelyStaticBinding : LonelyStaticBindingT<LonelyStaticBinding, implementation::LonelyStaticBinding>
    {
    };
}

