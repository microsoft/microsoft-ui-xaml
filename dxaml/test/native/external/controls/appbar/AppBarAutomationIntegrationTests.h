// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBar {

    class AppBarAutomationIntegrationTests : public WEX::TestClass<AppBarAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AppBarAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"ab3da967-c6a7-4e5b-b3c4-c53c6c46175c")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyAutomationProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verify AutomationProperties for AppBar.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyNoLightDismissInTreeWhenCollapsed)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that there is no light dismiss button in the UIA tree when the AppBar is collapsed")
            TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP")    // WPF_HOSTING_MODE_FAILURE: Close Button is found in UIA tree.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLightDismissInTreeWhenExpanded)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that there is a light dismiss button inserted into the UIA tree when the AppBar is collapsed and that its invocation collapses the AppBar")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAutomationWindowPattern)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a AppBar implements the Window pattern when AppBar is opened.")
        END_TEST_METHOD()

    private:
        void VerifyAutomationProperties(bool useAppBarAutomationName);
        xaml_controls::AppBar^ SetupAppBar(bool isOpen, bool setAppBarAutomationName = true);

        static WCHAR s_automationName[];
        static WCHAR s_automationId[];

        Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride m_featureDisableTransitionsForTest;
    };

} } } } } }
