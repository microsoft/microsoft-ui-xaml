// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace WrapGrid {

    class WrapGridIntegrationTests : public WEX::TestClass<WrapGridIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(WrapGridIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286;bdc5c68b-03c1-4217-832c-4c09f99946f4")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a WrapGrid.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a WrapGrid from the live tree in an ItemsControl.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanWrapItemsHorizontally)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that WrapGrids can wrap items in a horizontal orientation.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanWrapItemsVertically)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that WrapGrids can wrap items in a vertical orientation.")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    private:
        static const int s_itemCount = 8;

    };

} } } } } }
