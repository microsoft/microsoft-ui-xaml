// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ContextMenuEventArgs {

    class ContextMenuEventArgsIntegrationTests : public WEX::TestClass<ContextMenuEventArgsIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ContextMenuEventArgsIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //

        //
        // Platform:Desktop
        //
        BEGIN_TEST_METHOD(CanTextBoxFireContextMenuEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully fire a ContextMenuEvent on TextBox.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        //
        // Platform:Phone
        //

    };

} } } } } }
