// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBarElementContainer {

    class AppBarElementContainerIntegrationTests : public WEX::TestClass<AppBarElementContainerIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AppBarElementContainerIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create an AppBarElementContainer.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove an AppBarElementContainer from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanContainStringContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting string content correctly creates a TextBlock to hold it.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanContainUIElementContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting UIElement content correctly inserts it into the visual tree.")
        END_TEST_METHOD()
    };

} } } } } }
