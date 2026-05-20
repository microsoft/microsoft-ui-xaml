// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutoSuggest {

    class AutoSuggestIntegrationTests : public WEX::TestClass<AutoSuggestIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AutoSuggestIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"dd3493a5-54ef-4337-93bc-89b726406385;6945dbf6-cec1-4485-999c-920d441f6818;cbb6c59f-3ce2-4ed3-8eaa-f598566c2755")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
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

        BEGIN_TEST_METHOD(VerifyListItemsChangedEventForAutoSuggestBox)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully raise ListItemsChangedEvent automation event from AutoSuggestBox.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()


        //
        // Platform:Phone
        //

    };
} } } } } }

