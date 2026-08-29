// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBarSeparator {

    class AppBarSeparatorIntegrationTests : public WEX::TestClass<AppBarSeparatorIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AppBarSeparatorIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"ab3da967-c6a7-4e5b-b3c4-c53c6c46175c;a69ddfa4-5142-4bed-887d-6d0ca14a3473;cc5953d4-6553-42e5-8c02-80720aa9d842")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create an AppBarSeparator.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove an AppBarSeparator from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of AppBarSeparator in various visual states.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth & Height of AppBarSeparator in various configurations.")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    };

} } } } } }

