// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace BitmapIcon {

    class BitmapIconIntegrationTests : public WEX::TestClass<BitmapIconIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(BitmapIconIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"b914a3e3-0d78-4274-88b9-46a4773bb21b;a69ddfa4-5142-4bed-887d-6d0ca14a3473;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a BitmapIcon.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a BitmapIcon from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set/get the BitmapIcon properties.")
        END_TEST_METHOD()
    };

} } } } } }
