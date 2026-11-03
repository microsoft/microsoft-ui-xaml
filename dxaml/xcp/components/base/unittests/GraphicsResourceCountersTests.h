// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace GraphicsResourceCountersTests
{
    class CounterTests
    {
        BEGIN_TEST_CLASS(CounterTests)
        END_TEST_CLASS()

        TEST_METHOD(FactoryResults);
        TEST_METHOD(WrapperLifetime);
        TEST_METHOD(IndependentCategories);
        TEST_METHOD(ConcurrentUpdates);
        TEST_METHOD(SnapshotMetadata);
    };
}
