// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Base {
    class ThreadLocalStorageUnitTests
    {
    public:
        BEGIN_TEST_CLASS(ThreadLocalStorageUnitTests)
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateBasicLifetime)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can create an RAII thread-local object and pick up the correct lifetime behavior")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateConstructorForwarding)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can create an RAII thread-local object via forwarding parameters to its constructor")
        END_TEST_METHOD()
    };

} } } } }
