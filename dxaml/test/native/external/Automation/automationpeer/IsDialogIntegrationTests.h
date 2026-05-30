// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <AutomationClient\AutomationClientManager.h>
#include <XamlMetadataProviderOverrider.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    class IsDialogIntegrationTests : public WEX::TestClass<IsDialogIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(IsDialogIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
            TEST_CLASS_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // Requires IUIAutomationElement9 added in RS5
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyIsDialogSupplied_True)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for IsDialog on an element with IsDialog=true")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyIsDialogSupplied_False)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for IsDialog on an element with IsDialog=false")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyIsDialogNotSupplied)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for IsDialog on an element and not providing a value for IsDialog")
        END_TEST_METHOD()

    private:
        enum TestCase
        {
            IsDialogSupplied_True,
            IsDialogSupplied_False,
            IsDialogNotSupplied,
        };

        ref class CustomButtonAP sealed : public xaml_automation_peers::ButtonAutomationPeer
        {
        public:
            CustomButtonAP(xaml_controls::Button^ owner, int testCase) :
                xaml_automation_peers::ButtonAutomationPeer(owner),
                m_testCase((TestCase)testCase)
            {}

        protected:
            bool IsDialogCore() override
            {
                return _IsDialogCore(m_testCase);
            }

        private:
            TestCase m_testCase;
        };

        ref class CustomButton sealed : public xaml_controls::Button
        {
        public:
            CustomButton(int testCase) :
                m_testCase((TestCase)testCase)
            {}

        protected:
            xaml_automation_peers::AutomationPeer^ OnCreateAutomationPeer() override
            {
                if (m_automationPeer == nullptr)
                {
                    m_automationPeer = ref new CustomButtonAP(this, m_testCase);
                }
                return m_automationPeer;
            }

        private:
            CustomButtonAP^ m_automationPeer;
            TestCase m_testCase;
        };

        void _VerifyAutomationPropertiesMarkup(TestCase testCase);
        void _VerifyAutomationPropertiesCodebehind(TestCase testCase);
        void _VerifyAutomationPeer(TestCase testCase);
        void _VerifyClient(TestCase testCase, const Automation::AutomationClient::UIAElementInfo& uiaInfoTarget);

        static bool _IsDialogCore(TestCase testCase);
    };

} } } } } }
