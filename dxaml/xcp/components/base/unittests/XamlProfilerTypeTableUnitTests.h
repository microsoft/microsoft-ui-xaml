// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Base {

    class XamlProfilerTypeTableUnitTests
    {
    public:
        BEGIN_TEST_CLASS(XamlProfilerTypeTableUnitTests)
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateTypeIsDescribedOnce)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that only the first DescribeType call for a type writes its ElementTypeInfo row")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSessionStartResendsRows)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that enabling the provider or a capture-state request re-sends the rows of described types, and that disabling it doesn't")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSessionStartDuringRowWrite)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a session that starts while a type's row is being written, before the type is marked, still gets the row")
        END_TEST_METHOD()
    };

} } } } }