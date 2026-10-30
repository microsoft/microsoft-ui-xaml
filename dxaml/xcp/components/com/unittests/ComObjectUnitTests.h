// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Com {
    class ComObjectUnitTests
    {
    public:
        BEGIN_TEST_CLASS(ComObjectUnitTests)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(CanInstantiateComObject)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can instantiate a ComObject instance.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RuntimeClassNamePreservesOwnership)
            TEST_METHOD_PROPERTY(L"Description", L"Validates runtime class name contents, ownership, and null output handling.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RuntimeClassNameHandlesEmptyAndEmbeddedNulls)
            TEST_METHOD_PROPERTY(L"Description", L"Validates empty names and exact UTF-16 contents and lengths.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AggregatedRuntimeClassNameUsesOuter)
            TEST_METHOD_PROPERTY(L"Description", L"Validates delegating and non-delegating runtime class names.")
        END_TEST_METHOD()
    };

} } } } }
