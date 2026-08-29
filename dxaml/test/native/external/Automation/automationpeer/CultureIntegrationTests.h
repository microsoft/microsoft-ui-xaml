// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <AutomationClient\AutomationClientManager.h>
#include <XamlMetadataProviderOverrider.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    class CultureIntegrationTests : public WEX::TestClass<CultureIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(CultureIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"dd3493a5-54ef-4337-93bc-89b726406385")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)


        BEGIN_TEST_METHOD(VerifyCultureSet)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for Culture on an element with Culture='123'")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCultureNotSet)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for Culture on an element not providing a value for Culture")
        END_TEST_METHOD()

    private:
        enum TestCase
        {
            CultureSet = 0,
            CultureNotSet
        };

        ref class CustomButtonAP sealed : public xaml_automation_peers::ButtonAutomationPeer
        {
        public:
            CustomButtonAP(xaml_controls::Button^ owner, int tc) :
                xaml_automation_peers::ButtonAutomationPeer(owner),
                m_tc((TestCase)tc)
            {}

        protected:
            int GetCultureCore() override
            {
                return _GetCultureCore(m_tc);
            }
          
        private:
            TestCase m_tc;
        };

        ref class CustomButton sealed : public xaml_controls::Button
        {
        public:
            CustomButton(int tc) :
                m_tc((TestCase)tc)
            {}

        protected:
            xaml_automation_peers::AutomationPeer^ OnCreateAutomationPeer() override
            {
                if (m_ap == nullptr)
                {
                    m_ap = ref new CustomButtonAP(this, m_tc);
                }
                return m_ap;
            }

        private:
            CustomButtonAP^ m_ap;
            TestCase m_tc;
        };

        void _VerifyAutomationPropertiesMarkup(TestCase test);
        void _VerifyAutomationPropertiesCodebehind(TestCase test);
        void _VerifyAutomationPeer(TestCase test);
        void _VerifyClient(TestCase test, const Automation::AutomationClient::UIAElementInfo& uiaInfoTarget);

        static int _GetCultureCore(TestCase test);
    };

} } } } } }
