// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <AutomationClient\AutomationClientManager.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Button {

    class ButtonAutomationIntegrationTests : public WEX::TestClass<ButtonAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ButtonAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"b914a3e3-0d78-4274-88b9-46a4773bb21b")
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
        BEGIN_TEST_METHOD(DoesSupportEssentialPatterns)
            TEST_METHOD_PROPERTY(L"Description", L"Validates supported UIA patterns for Button..")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBeInvokedByUIA)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the button can be invoked by UIA Invoke pattern.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRaiseUIAInvokeEventE2E)
            TEST_METHOD_PROPERTY(L"Description", L"Validates End to End scenario for Button Invoke via UIA and Invoke Event Handler.")
            TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
            END_TEST_METHOD()

        //
        // Platform:Phone
        //

    private:
        void SetupButton(Automation::AutomationClient::UIAElementInfo uiaInfo);
    };

} } } } } }
