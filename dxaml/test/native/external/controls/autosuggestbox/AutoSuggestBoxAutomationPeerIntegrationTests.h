// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AutoSuggestBox {

    class AutoSuggestBoxAutomationPeerIntegrationTests : public WEX::TestClass<AutoSuggestBoxAutomationPeerIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AutoSuggestBoxAutomationPeerIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"b914a3e3-0d78-4274-88b9-46a4773bb21b")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyControlledPeersPropertyChangedEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that it successfully raises ControllerFor PropertyChanged event.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAutomationProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies AutomationProperties for AutoSuggestBox.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDefaultAutomationName)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies the default automation name for AutoSuggestBox.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseInvokePatternToSubmitQuery)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the InvokePattern will submit a query with the AutoSuggestBox.")
        END_TEST_METHOD()

    private:
        xaml_controls::AutoSuggestBox^ SetupTest(Platform::String^ headerText, Platform::String^ automationName, Platform::String^ automationId);

    };

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::AutoSuggestBox
