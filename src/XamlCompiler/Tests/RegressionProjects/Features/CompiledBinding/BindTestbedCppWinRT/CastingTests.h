// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "CastingTests.g.h"
#include "CastingModel.h"

namespace winrt::BindTestbed::implementation
{
    struct CastingTests : CastingTestsT<CastingTests>
    {
        CastingTests();
        hstring Dummy() { return L""; }
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct CastingTests : CastingTestsT<CastingTests, implementation::CastingTests>
    {
    };
}
