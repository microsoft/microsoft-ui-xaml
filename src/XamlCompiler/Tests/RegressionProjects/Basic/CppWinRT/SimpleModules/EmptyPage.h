// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "EmptyPage.g.h"

namespace winrt::Simple::implementation
{
    struct EmptyPage : EmptyPageT<EmptyPage>
    {
        EmptyPage();

        hstring Marker() const
        {
            return L"EmptyPage";
        }
    };
}

namespace winrt::Simple::factory_implementation
{
    struct EmptyPage : EmptyPageT<EmptyPage, implementation::EmptyPage>
    {
    };
}
