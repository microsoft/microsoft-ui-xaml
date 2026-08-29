// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Regression {

    class RegressionTests : public WEX::TestClass<RegressionTests>
    {
    public:

        TEST_CLASS_SETUP(ClassSetup)

        BEGIN_TEST_CLASS(RegressionTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"dd3493a5-54ef-4337-93bc-89b726406385")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()
        
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //

        BEGIN_TEST_METHOD(ValidateEmptyItemInContainer)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies Scroll Pattern Properties")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateAutomationPeerSetup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates proper handling in AutomationPeer if corresponding UIElement is dtor'd")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateAutomationPeerHasFocusInAutomationFocusChangedEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a custom AutomationPeer reports that is has focus when its AutomationFocusChanged event is raised.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        // Test currently supports desktop only since it requires NavigationHelperProxy, which has CAPs issues on mobile platforms.
        // These problems are expected to be fixed by RS1.
        BEGIN_TEST_METHOD(VerifyAutomationFocusAfterSuspend)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies automation follows focus on page navigation after app suspension.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

    private:
        void ValidateAutomationFocus(Automation::AutomationClient::UIAElementInfo uiaInfo, wrl::ComPtr<AutomationClient::AutomationFocusChangeHandler> spAutomationFocusChangeHandler, bool waitingFocusChange);
    };
} } } } } }


namespace Tests { namespace Native { namespace External { namespace Automation { namespace Regression 
{
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class CustomAutomationPeer sealed : public xaml_automation_peers::AutomationPeer
    {
    public:
        CustomAutomationPeer(){}

    protected:
        virtual bool IsKeyboardFocusableCore() override
        {
            return true;
        }
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class ControlWithCustomAutomationPeer sealed : public Microsoft::UI::Xaml::FrameworkElement
    {
    protected:
        xaml_automation_peers::AutomationPeer^ OnCreateAutomationPeer() override
        {
            return ref new CustomAutomationPeer();
        }
    };
}}}}}
