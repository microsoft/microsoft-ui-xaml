// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "precomp.h"
#include <XamlLogging.h>
#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace XamlDiagnostics {
    class HandleMapUnitTests : public WEX::TestClass<HandleMapUnitTests>
    {
    public:
            BEGIN_TEST_CLASS(HandleMapUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(TestOrphanedHandleBasic)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that we don't leak old property handles when setting a new one.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestOrphanedHandleMultipleOwners)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that when overwriting a property handle owned by multiple elements, we don't wrongly clean it up.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PropertyGetsRemovedFromCreatedAfterSet)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that when a property is set, if it was cached in the created map, that it gets removed .")
            END_TEST_METHOD()
    };
}}}}}
