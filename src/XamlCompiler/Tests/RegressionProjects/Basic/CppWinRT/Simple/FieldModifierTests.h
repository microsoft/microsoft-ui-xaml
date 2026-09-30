// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the FieldModifierTests class.
//

#pragma once

#include "FieldModifierTests.g.h"

namespace winrt::Simple::implementation
{
    struct FieldModifierTests : FieldModifierTestsT<FieldModifierTests>
    {
        FieldModifierTests();
    };
}

namespace winrt::Simple::factory_implementation
{
    struct FieldModifierTests : FieldModifierTestsT<FieldModifierTests, implementation::FieldModifierTests>
    {
    };
}
