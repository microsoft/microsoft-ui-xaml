// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <AutomationClient\AutomationClientManager.h>
#include <XamlMetadataProviderOverrider.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    class FlowsFromIntegrationTests : public WEX::TestClass<FlowsFromIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(FlowsFromIntegrationTests)
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

        BEGIN_TEST_METHOD(VerifyFlowsFromNotSet)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for FlowsFrom on an element that has FlowsFrom not set.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlowsFromEmptyList)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for FlowsFrom on an element that has an empty FlowsFrom list")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlowsFromOneElement)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for FlowsFrom on an element that has one FlowsFrom object")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlowsFromMultipleElements)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for FlowsFrom on an element that has several FlowsFrom objects")
        END_TEST_METHOD()

    private:
        enum TestCase
        {
            FlowsFromNotSet,
            FlowsFromEmptyList,
            FlowsFromOneElement,
            FlowsFromMultipleElements,
        };

        ref class CustomButtonAP sealed : public xaml_automation_peers::ButtonAutomationPeer
        {
        public:
            CustomButtonAP(xaml_controls::Button^ owner) :
                xaml_automation_peers::ButtonAutomationPeer(owner),
                m_tc(FlowsFromNotSet)
            {}

            void SetTestCase(int tc)
            {
                m_tc = (TestCase)tc;
            }

            void SetFlowsFromObjects(wfc::IVector<xaml::DependencyObject^>^ objects)
            {
                m_objects = objects;
            }

        protected:
            wfc::IIterable<xaml_automation_peers::AutomationPeer^>^ GetFlowsFromCore() override;

        private:
            TestCase m_tc;
            wfc::IVector<xaml::DependencyObject^>^ m_objects;
        };

        ref class CustomButton : public xaml_controls::Button
        {
        public:
            void SetTestCase(int tc)
            {
                if (m_ap == nullptr)
                {
                    m_ap = ref new CustomButtonAP(this);
                }
                m_ap->SetTestCase(tc);
            }

            // Call this to provide the automation peer with a list of automation peers to use
            void SetAPFlowsFromObjects(wfc::IVector<xaml::DependencyObject^>^ objects)
            {
                if (m_ap == nullptr)
                {
                    m_ap = ref new CustomButtonAP(this);
                }
                m_ap->SetFlowsFromObjects(objects);
            }

        protected:
            xaml_automation_peers::AutomationPeer^ OnCreateAutomationPeer() override
            {
                if (m_ap == nullptr)
                {
                    m_ap = ref new CustomButtonAP(this);
                }
                return m_ap;
            }

        private:
            CustomButtonAP^ m_ap;
        };


        void _VerifyFlowsFromEmptyList_AutomationPropertiesCodebehind();
        void _VerifyFlowsFromEmptyList_AutomationPeer();

        void _VerifyFlowsFromOneElement_AutomationPropertiesCodebehind();
        void _VerifyFlowsFromOneElement_AutomationPeer();

        void _VerifyFlowsFromMultipleElements_AutomationPropertiesCodebehind();
        void _VerifyFlowsFromMultipleElements_AutomationPeer();

    };

} } } } } }
