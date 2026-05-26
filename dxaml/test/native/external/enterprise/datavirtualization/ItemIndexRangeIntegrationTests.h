// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace DataVirtualization {

    class ItemIndexRangeIntegrationTests : public WEX::TestClass < ItemIndexRangeIntegrationTests >
    {
    public:
        BEGIN_TEST_CLASS(ItemIndexRangeIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"309fb554-f012-4ac9-bcf1-833e4a372493;375cd7bd-e448-4315-b2a1-bc02d75b0c4f")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(ValidateFirstIndex)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the FirstIndex property returns the right information.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLength)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the Length property returns the right information.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLastIndex)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the LastIndex property returns the right information.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCreateAndUseOffUIThread)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can create and use the ItemIndexRange class off of the UI Thread.")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //
    };

} } } } } }
