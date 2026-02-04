// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace SmartPointers {

class XRefPtrUnitTests
{
public:
    BEGIN_TEST_CLASS(XRefPtrUnitTests)
    END_TEST_CLASS()

    BEGIN_TEST_METHOD(ReleaseAndGetAddressOf)
        TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Detach)
        TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CopyTo)
        TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(WeakRefs)
        TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_METHOD()
};

} } } } }
