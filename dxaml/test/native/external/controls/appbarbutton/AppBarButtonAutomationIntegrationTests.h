// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <AutomationClient\AutomationClientManager.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Button {

    class AppBarButtonAutomationIntegrationTests : public WEX::TestClass<AppBarButtonAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AppBarButtonAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"b914a3e3-0d78-4274-88b9-46a4773bb21b")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
            TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyDefaultAutomationName)
            TEST_METHOD_PROPERTY(L"Description", L"Validates supported UIA patterns for Button..")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHasNoUIAControlChildren)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBarButton does not report any UIA children that are control elements.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySupportsExpandCollapsePatternWithFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBarButton supports the ExpandCollapse pattern when it has a Flyout child.")
        END_TEST_METHOD()

    private:
        void SetupButton(Automation::AutomationClient::UIAElementInfo uiaInfo);
    };

} } } } } }
