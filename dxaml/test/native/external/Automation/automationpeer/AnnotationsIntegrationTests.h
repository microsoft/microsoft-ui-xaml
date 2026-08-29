// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <AutomationClient\AutomationClientManager.h>
#include <XamlMetadataProviderOverrider.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    class AnnotationsIntegrationTests : public WEX::TestClass<AnnotationsIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AnnotationsIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"dd3493a5-54ef-4337-93bc-89b726406385")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") // Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyNoAnnotations)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for annotations on an element that has no annotations.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyOneAnnotationNoObject)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for annotations on an element that has one annotation with no accompanying object")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyOneAnnotationOneObject)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for annotations on an element that has one annotation with an accompanying object")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyXAnnotationsNoObjects)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for annotations on an element that has several annotations, all with no accompanying object")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyXAnnotationsAllWithObjects)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for annotations on an element that has several annotations, all with an accompanying object")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyXAnnotationsSomeWithObjects)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for annotations on an element that has several annotations, some with an accompanying object, some without")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAllAnnotationTypes)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the case where UIA asks for annotations on an element that has several annotations covering all possible annotation types")
        END_TEST_METHOD()

    private:
        enum TestCase
        {
            NoAnnotations,
            OneAnnotationNoObject,
            OneAnnotationOneObject,
            XAnnotationsNoObjects,
            XAnnotationsAllWithObjects,
            XAnnotationsSomeWithObjects,
            AllAnnotationTypes,
        };

        ref class CustomButtonAP sealed : public xaml_automation_peers::ButtonAutomationPeer
        {
        public:
            CustomButtonAP(xaml_controls::Button^ owner) :
                xaml_automation_peers::ButtonAutomationPeer(owner),
                m_tc(NoAnnotations)
            {}

            void SetTestCase(int tc)
            {
                m_tc = (TestCase)tc;
            }

            void SetAnnotationObjects(wfc::IVector<xaml::UIElement^>^ objects)
            {
                m_objects = objects;
            }

        protected:
            wfc::IVector<xaml_automation_peers::AutomationPeerAnnotation^>^ GetAnnotationsCore() override;

        private:
            TestCase m_tc;
            wfc::IVector<xaml::UIElement^>^ m_objects;
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
            void SetAPAnnotationObjects(wfc::IVector<xaml::UIElement^>^ objects)
            {
                if (m_ap == nullptr)
                {
                    m_ap = ref new CustomButtonAP(this);
                }
                m_ap->SetAnnotationObjects(objects);
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

        void _VerifyOneAnnotationNoObject_AutomationPropertiesMarkup();
        void _VerifyOneAnnotationNoObject_AutomationPropertiesCodebehind();
        void _VerifyOneAnnotationNoObject_AutomationPeer();

        void _VerifyOneAnnotationOneObject_AutomationPropertiesMarkup();
        void _VerifyOneAnnotationOneObject_AutomationPropertiesCodebehind();
        void _VerifyOneAnnotationOneObject_AutomationPeer();

        void _VerifyXAnnotationsNoObjects_AutomationPropertiesMarkup();
        void _VerifyXAnnotationsNoObjects_AutomationPropertiesCodebehind();
        void _VerifyXAnnotationsNoObjects_AutomationPeer();

        void _VerifyXAnnotationsAllWithObjects_AutomationPropertiesMarkup();
        void _VerifyXAnnotationsAllWithObjects_AutomationPropertiesCodebehind();
        void _VerifyXAnnotationsAllWithObjects_AutomationPeer();

        void _VerifyXAnnotationsSomeWithObjects_AutomationPropertiesMarkup();
        void _VerifyXAnnotationsSomeWithObjects_AutomationPropertiesCodebehind();
        void _VerifyXAnnotationsSomeWithObjects_AutomationPeer();

        void _VerifyAllAnnotationTypes_AutomationPropertiesMarkup();
        void _VerifyAllAnnotationTypes_AutomationPropertiesCodebehind();
        void _VerifyAllAnnotationTypes_AutomationPeer();

        ULONG _GetSafeArrayItemCount(SAFEARRAY *psa);
    };

} } } } } }
