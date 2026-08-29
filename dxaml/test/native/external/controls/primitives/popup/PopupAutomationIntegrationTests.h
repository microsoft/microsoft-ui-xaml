// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <UIAutomation.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace Popup {

    class PopupAutomationIntegrationTests : public WEX::TestClass<PopupAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(PopupAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"2b4b21d5-e5c0-4fad-bed4-62030fe191a1;eadeac67-1552-4876-a67e-dc1fcb8a6e25;bdc5c68b-03c1-4217-832c-4c09f99946f4")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
            TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyParentlessPopupAutomationTree_NoFrameAP)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyParentlessPopupAutomationTree_FrameAP)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAutomationProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verify AutomationProperties for Popup.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyBarePopupDoesNotImplementWindowPattern)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a Popup by itself not owned by any control does not implement the Window UIA pattern.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlyoutPopupImplementsWindowPattern)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a Popup owned by a Flyout implements the Window UIA pattern.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyMenuFlyoutPopupImplementsWindowPattern)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a Popup owned by a MenuFlyout implements the Window UIA pattern.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyContentDialogPopupImplementsWindowPattern)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a Popup owned by a ContentDialog implements the Window UIA pattern.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPopupButtonAutomationPropertyChangedEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the a Popup Button gets the UI Automation property changed event.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // WPF_HOSTING_MODE_FAILURE: When WPF-hosted, automation change event isn't raised as expected.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyOpenPopupPreservesMainTree)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that when a parentless popup opens, the main tree is still accessible")
        END_TEST_METHOD()

    private:
        wrl::ComPtr<IRawElementProviderFragment> GetUIAWindow();
        void CheckUIAElementName(_In_ wrl::ComPtr<IRawElementProviderFragment> uiaElement, _In_ const WCHAR* expectedName);
        void VerifyParentlessPopupAutomationTreeCommon(bool hasFrameAP);

        static WCHAR s_automationName[];
        static WCHAR s_automationId[];
        static WCHAR s_automationPopupButtonName[];
        static WCHAR s_automationPopupButtonId[];
        static WCHAR s_automationName2[];
        static WCHAR s_automationId2[];
        static WCHAR s_automationPopupButtonName2[];
        static WCHAR s_automationPopupButtonId2[];
    };

} } } } } } }
