// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <AutomationClient\AutomationClientManager.h>
#include <XamlMetadataProviderOverrider.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    class LandmarksIntegrationTests : public WEX::TestClass<LandmarksIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(LandmarksIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyCustomBoth)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element with LandmarkType=Custom and providing a value for LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFormBoth)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element with LandmarkType=Form and providing a value for LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyMainBoth)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element with LandmarkType=Main and providing a value for LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyNavigationBoth)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element with LandmarkType=Navigation and providing a value for LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySearchBoth)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element with LandmarkType=Search and providing a value for LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCustomLTOnly)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element with LandmarkType=Custom and not providing a value for LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFormLTOnly)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element with LandmarkType=Form and not providing a value for LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyMainLTOnly)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element with LandmarkType=Main and not providing a value for LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyNavigationLTOnly)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element with LandmarkType=Navigation and not providing a value for LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySearchLTOnly)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element with LandmarkType=Search and not providing a value for LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLLTOnly)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element not providing a LandmarkType but providing a value for LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyNoLandmarks)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element providing neither a LandmarkType nor a LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLandmarksOnContainerBoth)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on a container element providing a LandmarkType and a LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLandmarksOnContainerLTOnly)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on a container element providing only a LandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLandmarksOnContainerLLTOnly)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on a container element providing only a LocalizedLandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLTOutOfRange)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on an element providing an invalid LandmarkType")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyEmptyLLT)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for landmarks on a element with an empty LocalizedLandmarkType string")
        END_TEST_METHOD()

    private:
        enum TestCase
        {
            CustomBoth = 0,
            FormBoth,
            MainBoth,
            NavigationBoth,
            SearchBoth,
            CustomLTOnly,
            FormLTOnly,
            MainLTOnly,
            NavigationLTOnly,
            SearchLTOnly,
            LLTOnly,
            NoLandmarks,
            LandmarksOnContainerBoth,
            LandmarksOnContainerLTOnly,
            LandmarksOnContainerLLTOnly,
            LTOutOfRange,
            EmptyLLT,
        };

        ref class CustomButtonAP sealed : public xaml_automation_peers::ButtonAutomationPeer
        {
        public:
            CustomButtonAP(xaml_controls::Button^ owner, int tc) :
                xaml_automation_peers::ButtonAutomationPeer(owner),
                m_tc((TestCase)tc)
            {}

        protected:
            xaml_automation_peers::AutomationLandmarkType GetLandmarkTypeCore() override
            {
                return _GetLandmarkTypeCore(m_tc);
            }
            Platform::String^ GetLocalizedLandmarkTypeCore() override
            {
                return _GetLocalizedLandmarkTypeCore(m_tc);
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

        ref class CustomStackPanelAP sealed : public xaml_automation_peers::FrameworkElementAutomationPeer
        {
        public:
            CustomStackPanelAP(xaml::FrameworkElement^ owner, int tc) :
                xaml_automation_peers::FrameworkElementAutomationPeer(owner),
                m_tc((TestCase)tc)
            {}

        protected:
            xaml_automation_peers::AutomationLandmarkType GetLandmarkTypeCore() override
            {
                return _GetLandmarkTypeCore(m_tc);
            }
            Platform::String^ GetLocalizedLandmarkTypeCore() override
            {
                return _GetLocalizedLandmarkTypeCore(m_tc);
            }
            xaml_automation_peers::AutomationControlType GetAutomationControlTypeCore() override
            {
                return xaml_automation_peers::AutomationControlType::Pane;
            }

        private:
            TestCase m_tc;
        };

        ref class CustomStackPanel sealed : public xaml_controls::StackPanel
        {
        public:
            CustomStackPanel(int tc) :
                m_tc((TestCase)tc)
            {}

        protected:
            xaml_automation_peers::AutomationPeer^ OnCreateAutomationPeer() override
            {
                if (m_ap == nullptr)
                {
                    m_ap = ref new CustomStackPanelAP(this, m_tc);
                }
                return m_ap;
            }

        private:
            CustomStackPanelAP^ m_ap;
            TestCase m_tc;
        };

        void _VerifyAutomationPropertiesMarkup(TestCase test);
        void _VerifyAutomationPropertiesCodebehind(TestCase test);
        void _VerifyAutomationPeer(TestCase test);
        void _VerifyClient(TestCase test, const Automation::AutomationClient::UIAElementInfo& uiaInfoTarget);

        static xaml_automation_peers::AutomationLandmarkType _GetLandmarkTypeCore(TestCase test);
        static Platform::String^ _GetLocalizedLandmarkTypeCore(TestCase test);
    };

} } } } } }
