// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the BlankPage class.
//

#pragma once

#include "BlankPageBase.h"
#include "BlankPage.g.h"

namespace winrt::Simple::implementation
{
    struct BlankPage : BlankPageT<BlankPage>
    {
        BlankPage();
    };
}

namespace winrt::Simple::factory_implementation
{
    struct BlankPage : BlankPageT<BlankPage, implementation::BlankPage>
    {
    };
}