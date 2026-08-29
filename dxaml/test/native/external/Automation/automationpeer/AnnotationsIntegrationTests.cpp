// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <AnnotationsIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    bool AnnotationsIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool AnnotationsIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AnnotationsIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Case Wrappers
    //
    void AnnotationsIntegrationTests::VerifyNoAnnotations()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"Button";
        uiaInfo.m_AutomationID = L"Button";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button;
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)0);
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spAnnotationObjects.Get() == nullptr || length == 0);
        });
    }

    void AnnotationsIntegrationTests::VerifyOneAnnotationNoObject()
    {
        _VerifyOneAnnotationNoObject_AutomationPropertiesMarkup();
        LOG_OUTPUT(L"\r\n");
        _VerifyOneAnnotationNoObject_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyOneAnnotationNoObject_AutomationPeer();
    }

    void AnnotationsIntegrationTests::VerifyOneAnnotationOneObject()
    {
        _VerifyOneAnnotationOneObject_AutomationPropertiesMarkup();
        LOG_OUTPUT(L"\r\n");
        _VerifyOneAnnotationOneObject_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyOneAnnotationOneObject_AutomationPeer();
    }

    void AnnotationsIntegrationTests::VerifyXAnnotationsNoObjects()
    {
        _VerifyXAnnotationsNoObjects_AutomationPropertiesMarkup();
        LOG_OUTPUT(L"\r\n");
        _VerifyXAnnotationsNoObjects_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyXAnnotationsNoObjects_AutomationPeer();
    }

    void AnnotationsIntegrationTests::VerifyXAnnotationsAllWithObjects()
    {
        // This test fails because of a UIA bug. Reactivate after bug is fixed.
        //_VerifyXAnnotationsAllWithObjects_AutomationPropertiesMarkup();
        //LOG_OUTPUT(L"\r\n");
        //_VerifyXAnnotationsAllWithObjects_AutomationPropertiesCodebehind();
        //LOG_OUTPUT(L"\r\n");
        _VerifyXAnnotationsAllWithObjects_AutomationPeer();
    }

    void AnnotationsIntegrationTests::VerifyXAnnotationsSomeWithObjects()
    {
        // This test fails because of a UIA bug. Reactivate after bug is fixed.
        //_VerifyXAnnotationsSomeWithObjects_AutomationPropertiesMarkup();
        //LOG_OUTPUT(L"\r\n");
        //_VerifyXAnnotationsSomeWithObjects_AutomationPropertiesCodebehind();
        //LOG_OUTPUT(L"\r\n");
        _VerifyXAnnotationsSomeWithObjects_AutomationPeer();
    }

    void AnnotationsIntegrationTests::VerifyAllAnnotationTypes()
    {
        _VerifyAllAnnotationTypes_AutomationPropertiesMarkup();
        LOG_OUTPUT(L"\r\n");
        _VerifyAllAnnotationTypes_AutomationPropertiesCodebehind();
        LOG_OUTPUT(L"\r\n");
        _VerifyAllAnnotationTypes_AutomationPeer();
    }

    //
    // Test Cases
    //
    void AnnotationsIntegrationTests::_VerifyOneAnnotationNoObject_AutomationPropertiesMarkup()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AnnotatedButton";
        uiaInfo.m_AutomationID = L"AnnotatedButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            Platform::String^ xaml =
                "<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' AutomationProperties.Name='AnnotatedButton'>"
                "    <AutomationProperties.Annotations>"
                "        <AutomationAnnotation Type='SpellingError' />"
                "    </AutomationProperties.Annotations>"
                "</Button>";

            TestServices::WindowHelper->WindowContent = static_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(xaml));
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)1);
            LONG zero = 0;
            int annotationType;
            SafeArrayGetElement(psaAnnotationTypes, &zero, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spAnnotationObjects.Get() == nullptr || length == 0);
        });
    }

    void AnnotationsIntegrationTests::_VerifyOneAnnotationNoObject_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationProperties in codebehind");

        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AnnotatedButton";
        uiaInfo.m_AutomationID = L"AnnotatedButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button;
            xaml_automation::AutomationAnnotation^ automationAnnotation = ref new xaml_automation::AutomationAnnotation;
            automationAnnotation->Type = xaml_automation::AnnotationType::SpellingError;
            auto annotationsVector = xaml_automation::AutomationProperties::GetAnnotations(button);
            VERIFY_IS_NOT_NULL(annotationsVector);
            annotationsVector->Append(automationAnnotation);
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)1);
            LONG zero = 0;
            int annotationType;
            SafeArrayGetElement(psaAnnotationTypes, &zero, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spAnnotationObjects.Get() == nullptr || length == 0);
        });
    }

    void AnnotationsIntegrationTests::_VerifyOneAnnotationNoObject_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationPeer");

        CustomButton^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AnnotatedButton";
        uiaInfo.m_AutomationID = L"AnnotatedButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new CustomButton;
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));
            button->SetTestCase(OneAnnotationNoObject);

            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(button);
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)1);
            LONG zero = 0;
            int annotationType;
            SafeArrayGetElement(psaAnnotationTypes, &zero, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spAnnotationObjects.Get() == nullptr || length == 0);
        });
    }


    void AnnotationsIntegrationTests::_VerifyOneAnnotationOneObject_AutomationPropertiesMarkup()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"AnnotatedButton";
        uiaInfoTarget.m_AutomationID = L"AnnotatedButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        Automation::AutomationClient::UIAElementInfo uiaInfoAnnotation;
        uiaInfoAnnotation.m_Name = L"AnnotationTextBlock";
        uiaInfoAnnotation.m_AutomationID = L"AnnotationTextBlock";
        uiaInfoAnnotation.m_cType = UIA_TextControlTypeId;

        RunOnUIThread([&]()
        {
            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "    <Button AutomationProperties.Name='AnnotatedButton' x:Name='AnnotatedButton'>"
                "        <AutomationProperties.Annotations>"
                "            <AutomationAnnotation Type='Comment' Element='{Binding ElementName=AnnotationTextBlock}' />"
                "        </AutomationProperties.Annotations>"
                "    </Button>"
                "    <TextBlock AutomationProperties.Name='AnnotationTextBlock' x:Name='AnnotationTextBlock' />"
                "</StackPanel>";

            TestServices::WindowHelper->WindowContent = static_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(xaml));
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            auto spAutomationClientManagerAnnotation = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoAnnotation);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementAnnotation;
            spAutomationClientManagerAnnotation->GetCurrentUIAutomationElement(&spUIAutomationElementAnnotation);
            WEX::Common::Throw::IfNull(spUIAutomationElementAnnotation.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4Target));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)1);
            LONG zero = 0;
            int annotationType;
            SafeArrayGetElement(psaAnnotationTypes, &zero, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Comment);
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, 1);

                wrl::ComPtr<IUIAutomationElement> spElement;
                VERIFY_SUCCEEDED(spAnnotationObjects->GetElement(0, &spElement));
                WEX::Common::Throw::IfNull(spElement.Get());
                VERIFY_IS_TRUE(spAutomationClientManagerAnnotation->IsElementSame(spElement.Get()));
            }
            else
            {
                VERIFY_IS_NOT_NULL(spAnnotationObjects.Get());
            }
        });
    }

    void AnnotationsIntegrationTests::_VerifyOneAnnotationOneObject_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;
        xaml_controls::TextBlock^ textBlock;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"AnnotatedButton";
        uiaInfoTarget.m_AutomationID = L"AnnotatedButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        Automation::AutomationClient::UIAElementInfo uiaInfoAnnotation;
        uiaInfoAnnotation.m_Name = L"AnnotationTextBlock";
        uiaInfoAnnotation.m_AutomationID = L"AnnotationTextBlock";
        uiaInfoAnnotation.m_cType = UIA_TextControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;
            textBlock = ref new xaml_controls::TextBlock;

            xaml_automation::AutomationAnnotation^ automationAnnotation = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::Comment, textBlock);
            auto annotationsVector = xaml_automation::AutomationProperties::GetAnnotations(button);
            VERIFY_IS_NOT_NULL(annotationsVector);
            annotationsVector->Append(automationAnnotation);

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfoAnnotation.m_Name));
            textBlock->Name = ref new Platform::String(uiaInfoAnnotation.m_Name);

            stackPanel->Children->Append(button);
            stackPanel->Children->Append(textBlock);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            auto spAutomationClientManagerAnnotation = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoAnnotation);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementAnnotation;
            spAutomationClientManagerAnnotation->GetCurrentUIAutomationElement(&spUIAutomationElementAnnotation);
            WEX::Common::Throw::IfNull(spUIAutomationElementAnnotation.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4Target));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)1);
            LONG zero = 0;
            int annotationType;
            SafeArrayGetElement(psaAnnotationTypes, &zero, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Comment);
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, 1);

                wrl::ComPtr<IUIAutomationElement> spElement;
                VERIFY_SUCCEEDED(spAnnotationObjects->GetElement(0, &spElement));
                WEX::Common::Throw::IfNull(spElement.Get());
                VERIFY_IS_TRUE(spAutomationClientManagerAnnotation->IsElementSame(spElement.Get()));
            }
            else
            {
                VERIFY_IS_NOT_NULL(spAnnotationObjects.Get());
            }
        });
    }

    void AnnotationsIntegrationTests::_VerifyOneAnnotationOneObject_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        CustomButton^ button;
        xaml_controls::TextBlock^ textBlock;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"AnnotatedButton";
        uiaInfoTarget.m_AutomationID = L"AnnotatedButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        Automation::AutomationClient::UIAElementInfo uiaInfoAnnotation;
        uiaInfoAnnotation.m_Name = L"AnnotationTextBlock";
        uiaInfoAnnotation.m_AutomationID = L"AnnotationTextBlock";
        uiaInfoAnnotation.m_cType = UIA_TextControlTypeId;

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new CustomButton;
            textBlock = ref new xaml_controls::TextBlock;

            wfc::IVector<xaml::UIElement^>^ annotations = ref new Platform::Collections::Vector<xaml::UIElement^>;
            annotations->Append(textBlock);
            button->SetAPAnnotationObjects(annotations);
            button->SetTestCase(OneAnnotationOneObject);

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfoAnnotation.m_Name));
            textBlock->Name = ref new Platform::String(uiaInfoAnnotation.m_Name);

            stackPanel->Children->Append(button);
            stackPanel->Children->Append(textBlock);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            auto spAutomationClientManagerAnnotation = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoAnnotation);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementAnnotation;
            spAutomationClientManagerAnnotation->GetCurrentUIAutomationElement(&spUIAutomationElementAnnotation);
            WEX::Common::Throw::IfNull(spUIAutomationElementAnnotation.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4Target));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)1);
            LONG zero = 0;
            int annotationType;
            SafeArrayGetElement(psaAnnotationTypes, &zero, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Comment);
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, 1);

                wrl::ComPtr<IUIAutomationElement> spElement;
                VERIFY_SUCCEEDED(spAnnotationObjects->GetElement(0, &spElement));
                WEX::Common::Throw::IfNull(spElement.Get());
                VERIFY_IS_TRUE(spAutomationClientManagerAnnotation->IsElementSame(spElement.Get()));
            }
            else
            {
                VERIFY_IS_NOT_NULL(spAnnotationObjects.Get());
            }
        });
    }


    void AnnotationsIntegrationTests::_VerifyXAnnotationsNoObjects_AutomationPropertiesMarkup()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AnnotatedButton";
        uiaInfo.m_AutomationID = L"AnnotatedButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            Platform::String^ xaml =
                "<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' AutomationProperties.Name='AnnotatedButton'>"
                "    <AutomationProperties.Annotations>"
                "        <AutomationAnnotation Type='SpellingError' />"
                "        <AutomationAnnotation Type='GrammarError' />"
                "        <AutomationAnnotation Type='SpellingError' />"
                "    </AutomationProperties.Annotations>"
                "</Button>";

            TestServices::WindowHelper->WindowContent = static_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(xaml));
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)3);

            LONG idx = 0;
            int annotationType;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::GrammarError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spAnnotationObjects.Get() == nullptr || length == 0);
        });
    }

    void AnnotationsIntegrationTests::_VerifyXAnnotationsNoObjects_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationProperties in codebehind");

        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AnnotatedButton";
        uiaInfo.m_AutomationID = L"AnnotatedButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button;
            xaml_automation::AutomationAnnotation^ automationAnnotation1 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::SpellingError);
            xaml_automation::AutomationAnnotation^ automationAnnotation2 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::GrammarError);
            xaml_automation::AutomationAnnotation^ automationAnnotation3 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::SpellingError);
            auto annotationsVector = xaml_automation::AutomationProperties::GetAnnotations(button);
            VERIFY_IS_NOT_NULL(annotationsVector);
            annotationsVector->Append(automationAnnotation1);
            annotationsVector->Append(automationAnnotation2);
            annotationsVector->Append(automationAnnotation3);
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)3);

            LONG idx = 0;
            int annotationType;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::GrammarError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spAnnotationObjects.Get() == nullptr || length == 0);
        });
    }

    void AnnotationsIntegrationTests::_VerifyXAnnotationsNoObjects_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationPeer");

        CustomButton^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AnnotatedButton";
        uiaInfo.m_AutomationID = L"AnnotatedButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new CustomButton;
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));
            button->SetTestCase(XAnnotationsNoObjects);

            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(button);
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)3);

            LONG idx = 0;
            int annotationType;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::GrammarError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spAnnotationObjects.Get() == nullptr || length == 0);
        });
    }


    void AnnotationsIntegrationTests::_VerifyXAnnotationsAllWithObjects_AutomationPropertiesMarkup()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"AnnotatedButton";
        uiaInfoTarget.m_AutomationID = L"AnnotatedButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        WCHAR *pszNames[4];
        std::vector<Automation::AutomationClient::UIAElementInfo> annotationElementInfos;
        for (int i = 0; i < 4; i++)
        {
            Automation::AutomationClient::UIAElementInfo uiaInfoAnnotation;
            pszNames[i] = new WCHAR[wcslen(L"AnnotationTextBlockx") + 1];
            StringCchPrintf(pszNames[i], wcslen(L"AnnotationTextBlockx") + 1, L"AnnotationTextBlock%d", i + 1);
            uiaInfoAnnotation.m_Name = pszNames[i];
            uiaInfoAnnotation.m_AutomationID = pszNames[i];
            uiaInfoAnnotation.m_cType = UIA_TextControlTypeId;
            annotationElementInfos.push_back(uiaInfoAnnotation);
        }

        RunOnUIThread([&]()
        {
            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "    <Button AutomationProperties.Name='AnnotatedButton'>"
                "        <AutomationProperties.Annotations>"
                "            <AutomationAnnotation Type='Comment' Element='{Binding ElementName=AnnotationTextBlock1}' />"
                "            <AutomationAnnotation Type='Comment' Element='{Binding ElementName=AnnotationTextBlock2}' />"
                "            <AutomationAnnotation Type='Comment' Element='{Binding ElementName=AnnotationTextBlock3}' />"
                "            <AutomationAnnotation Type='Comment' Element='{Binding ElementName=AnnotationTextBlock4}' />"
                "        </AutomationProperties.Annotations>"
                "    </Button>"
                "    <TextBlock AutomationProperties.Name='AnnotationTextBlock1' x:Name='AnnotationTextBlock1' />"
                "    <TextBlock AutomationProperties.Name='AnnotationTextBlock2' x:Name='AnnotationTextBlock2' />"
                "    <TextBlock AutomationProperties.Name='AnnotationTextBlock3' x:Name='AnnotationTextBlock3' />"
                "    <TextBlock AutomationProperties.Name='AnnotationTextBlock4' x:Name='AnnotationTextBlock4' />"
                "</StackPanel>";

            TestServices::WindowHelper->WindowContent = static_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(xaml));
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4Target));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)annotationElementInfos.size());
            for (LONG i = 0; i < (LONG)annotationElementInfos.size(); i++)
            {
                int annotationType;
                SafeArrayGetElement(psaAnnotationTypes, &i, (void*)&annotationType);
                VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Comment);
            }
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, (int)annotationElementInfos.size());

                for (LONG i = 0; i < (LONG)annotationElementInfos.size(); i++)
                {
                    auto spAutomationClientManagerAnnotation = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(annotationElementInfos[i]);
                    wrl::ComPtr<IUIAutomationElement> spUIAutomationElementAnnotation;
                    spAutomationClientManagerAnnotation->GetCurrentUIAutomationElement(&spUIAutomationElementAnnotation);
                    WEX::Common::Throw::IfNull(spUIAutomationElementAnnotation.Get());

                    wrl::ComPtr<IUIAutomationElement> spElement;
                    VERIFY_SUCCEEDED(spAnnotationObjects->GetElement(i, &spElement));
                    WEX::Common::Throw::IfNull(spElement.Get());
                    VERIFY_IS_TRUE(spAutomationClientManagerAnnotation->IsElementSame(spElement.Get()));
                }
            }
            else
            {
                VERIFY_IS_NOT_NULL(spAnnotationObjects.Get());
            }
        });

        for (int i = 0; i < 4; i++)
        {
            delete[] pszNames[i];
        }
    }

    void AnnotationsIntegrationTests::_VerifyXAnnotationsAllWithObjects_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"AnnotatedButton";
        uiaInfoTarget.m_AutomationID = L"AnnotatedButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        WCHAR *pszNames[4];
        std::vector<Automation::AutomationClient::UIAElementInfo> annotationElementInfos;
        for (int i = 0; i < 4; i++)
        {
            Automation::AutomationClient::UIAElementInfo uiaInfoAnnotation;
            pszNames[i] = new WCHAR[wcslen(L"AnnotationTextBlockx") + 1];
            StringCchPrintf(pszNames[i], wcslen(L"AnnotationTextBlockx") + 1, L"AnnotationTextBlock%d", i + 1);
            uiaInfoAnnotation.m_Name = pszNames[i];
            uiaInfoAnnotation.m_AutomationID = pszNames[i];
            uiaInfoAnnotation.m_cType = UIA_TextControlTypeId;
            annotationElementInfos.push_back(uiaInfoAnnotation);
        }

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            auto annotationsVector = xaml_automation::AutomationProperties::GetAnnotations(button);
            VERIFY_IS_NOT_NULL(annotationsVector);

            for (size_t i = 0; i < annotationElementInfos.size(); i++)
            {
                xaml_controls::TextBlock^ textBlock = ref new xaml_controls::TextBlock;
                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(annotationElementInfos[i].m_Name));
                textBlock->Name = ref new Platform::String(annotationElementInfos[i].m_Name);
                stackPanel->Children->Append(textBlock);
                xaml_automation::AutomationAnnotation^ automationAnnotation = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::Comment, textBlock);
                annotationsVector->Append(automationAnnotation);
            }

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            stackPanel->Children->Append(button);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4Target));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)annotationElementInfos.size());
            for (LONG i = 0; i < (LONG)annotationElementInfos.size(); i++)
            {
                int annotationType;
                SafeArrayGetElement(psaAnnotationTypes, &i, (void*)&annotationType);
                VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Comment);
            }
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, (int)annotationElementInfos.size());

                for (LONG i = 0; i < (LONG)annotationElementInfos.size(); i++)
                {
                    auto spAutomationClientManagerAnnotation = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(annotationElementInfos[i]);
                    wrl::ComPtr<IUIAutomationElement> spUIAutomationElementAnnotation;
                    spAutomationClientManagerAnnotation->GetCurrentUIAutomationElement(&spUIAutomationElementAnnotation);
                    WEX::Common::Throw::IfNull(spUIAutomationElementAnnotation.Get());

                    wrl::ComPtr<IUIAutomationElement> spElement;
                    VERIFY_SUCCEEDED(spAnnotationObjects->GetElement(i, &spElement));
                    WEX::Common::Throw::IfNull(spElement.Get());
                    VERIFY_IS_TRUE(spAutomationClientManagerAnnotation->IsElementSame(spElement.Get()));
                }
            }
            else
            {
                VERIFY_IS_NOT_NULL(spAnnotationObjects.Get());
            }
        });

        for (int i = 0; i < 4; i++)
        {
            delete[] pszNames[i];
        }
    }

    void AnnotationsIntegrationTests::_VerifyXAnnotationsAllWithObjects_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        CustomButton^ button;
        wfc::IVector<xaml::UIElement^>^ textBlocks;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"AnnotatedButton";
        uiaInfoTarget.m_AutomationID = L"AnnotatedButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        WCHAR *pszNames[4];
        std::vector<Automation::AutomationClient::UIAElementInfo> annotationElementInfos;
        for (int i = 0; i < 4; i++)
        {
            Automation::AutomationClient::UIAElementInfo uiaInfoAnnotation;
            pszNames[i] = new WCHAR[wcslen(L"AnnotationTextBlockx") + 1];
            StringCchPrintf(pszNames[i], wcslen(L"AnnotationTextBlockx") + 1, L"AnnotationTextBlock%d", i + 1);
            uiaInfoAnnotation.m_Name = pszNames[i];
            uiaInfoAnnotation.m_AutomationID = pszNames[i];
            uiaInfoAnnotation.m_cType = UIA_TextControlTypeId;
            annotationElementInfos.push_back(uiaInfoAnnotation);
        }

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new CustomButton;
            textBlocks = ref new Platform::Collections::Vector<xaml::UIElement^>;

            for (size_t i = 0; i < annotationElementInfos.size(); i++)
            {
                xaml_controls::TextBlock^ textBlock = ref new xaml_controls::TextBlock;
                xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(annotationElementInfos[i].m_Name));
                textBlock->Name = ref new Platform::String(annotationElementInfos[i].m_Name);
                stackPanel->Children->Append(textBlock);
                textBlocks->Append(textBlock);
            }

            button->SetAPAnnotationObjects(textBlocks);
            button->SetTestCase(XAnnotationsAllWithObjects);
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            stackPanel->Children->Append(button);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4Target));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)annotationElementInfos.size());
            for (LONG i = 0; i < (LONG)annotationElementInfos.size(); i++)
            {
                int annotationType;
                SafeArrayGetElement(psaAnnotationTypes, &i, (void*)&annotationType);
                VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Comment);
            }
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, (int)annotationElementInfos.size());

                for (LONG i = 0; i < (LONG)annotationElementInfos.size(); i++)
                {
                    auto spAutomationClientManagerAnnotation = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(annotationElementInfos[i]);
                    wrl::ComPtr<IUIAutomationElement> spUIAutomationElementAnnotation;
                    spAutomationClientManagerAnnotation->GetCurrentUIAutomationElement(&spUIAutomationElementAnnotation);
                    WEX::Common::Throw::IfNull(spUIAutomationElementAnnotation.Get());

                    wrl::ComPtr<IUIAutomationElement> spElement;
                    VERIFY_SUCCEEDED(spAnnotationObjects->GetElement(i, &spElement));
                    WEX::Common::Throw::IfNull(spElement.Get());
                    VERIFY_IS_TRUE(spAutomationClientManagerAnnotation->IsElementSame(spElement.Get()));
                }
            }
            else
            {
                VERIFY_IS_NOT_NULL(spAnnotationObjects.Get());
            }
        });

        for (int i = 0; i < 4; i++)
        {
            delete[] pszNames[i];
        }
    }


    void AnnotationsIntegrationTests::_VerifyXAnnotationsSomeWithObjects_AutomationPropertiesMarkup()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"AnnotatedButton";
        uiaInfoTarget.m_AutomationID = L"AnnotatedButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        WCHAR *pszNames[2];
        std::vector<Automation::AutomationClient::UIAElementInfo> annotationElementInfos;
        for (int i = 0; i < 4; i++)
        {
            Automation::AutomationClient::UIAElementInfo uiaInfoAnnotation;
            if (i % 2 == 0)
            {
                uiaInfoAnnotation.m_Name = nullptr;
                uiaInfoAnnotation.m_AutomationID = nullptr;
            }
            else
            {
                pszNames[i / 2] = new WCHAR[wcslen(L"AnnotationTextBlockx") + 1];
                StringCchPrintf(pszNames[i / 2], wcslen(L"AnnotationTextBlockx") + 1, L"AnnotationTextBlock%d", i + 1);
                uiaInfoAnnotation.m_Name = pszNames[i / 2];
                uiaInfoAnnotation.m_AutomationID = pszNames[i / 2];
                uiaInfoAnnotation.m_cType = UIA_TextControlTypeId;
            }
            annotationElementInfos.push_back(uiaInfoAnnotation);
        }

        RunOnUIThread([&]()
        {
            Platform::String^ xaml =
                "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                "    <Button AutomationProperties.Name='AnnotatedButton'>"
                "        <AutomationProperties.Annotations>"
                "            <AutomationAnnotation Type='SpellingError' />"
                "            <AutomationAnnotation Type='Comment' Element='{Binding ElementName=AnnotationTextBlock2}' />"
                "            <AutomationAnnotation Type='SpellingError' />"
                "            <AutomationAnnotation Type='Comment' Element='{Binding ElementName=AnnotationTextBlock4}' />"
                "        </AutomationProperties.Annotations>"
                "    </Button>"
                "    <TextBlock AutomationProperties.Name='AnnotationTextBlock2' x:Name='AnnotationTextBlock2' />"
                "    <TextBlock AutomationProperties.Name='AnnotationTextBlock4' x:Name='AnnotationTextBlock4' />"
                "</StackPanel>";

            TestServices::WindowHelper->WindowContent = static_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(xaml));
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4Target));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)annotationElementInfos.size());
            for (LONG i = 0; i < (LONG)annotationElementInfos.size(); i++)
            {
                int annotationType;
                SafeArrayGetElement(psaAnnotationTypes, &i, (void*)&annotationType);
                if (annotationElementInfos[i].m_Name != nullptr)
                {
                    VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Comment);
                }
                else
                {
                    VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);
                }
            }
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, (int)annotationElementInfos.size());

                for (LONG i = 0; i < (LONG)annotationElementInfos.size(); i++)
                {
                    wrl::ComPtr<IUIAutomationElement> spElement;
                    VERIFY_SUCCEEDED(spAnnotationObjects->GetElement(i, &spElement));

                    if (annotationElementInfos[i].m_Name != nullptr)
                    {
                        auto spAutomationClientManagerAnnotation = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(annotationElementInfos[i]);
                        wrl::ComPtr<IUIAutomationElement> spUIAutomationElementAnnotation;
                        spAutomationClientManagerAnnotation->GetCurrentUIAutomationElement(&spUIAutomationElementAnnotation);
                        WEX::Common::Throw::IfNull(spUIAutomationElementAnnotation.Get());

                        WEX::Common::Throw::IfNull(spElement.Get());
                        VERIFY_IS_TRUE(spAutomationClientManagerAnnotation->IsElementSame(spElement.Get()));
                    }
                    else
                    {
                        VERIFY_ARE_EQUAL(spElement.Get(), nullptr);
                    }
                }
            }
            else
            {
                VERIFY_IS_NOT_NULL(spAnnotationObjects.Get());
            }
        });

        for (int i = 0; i < 2; i++)
        {
            delete[] pszNames[i];
        }
    }

    void AnnotationsIntegrationTests::_VerifyXAnnotationsSomeWithObjects_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationProperties in codebehind");

        xaml_controls::StackPanel^ stackPanel;
        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"AnnotatedButton";
        uiaInfoTarget.m_AutomationID = L"AnnotatedButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        WCHAR *pszNames[2];
        std::vector<Automation::AutomationClient::UIAElementInfo> annotationElementInfos;
        for (int i = 0; i < 4; i++)
        {
            Automation::AutomationClient::UIAElementInfo uiaInfoAnnotation;
            if (i % 2 == 0)
            {
                uiaInfoAnnotation.m_Name = nullptr;
                uiaInfoAnnotation.m_AutomationID = nullptr;
            }
            else
            {
                pszNames[i / 2] = new WCHAR[wcslen(L"AnnotationTextBlockx") + 1];
                StringCchPrintf(pszNames[i / 2], wcslen(L"AnnotationTextBlockx") + 1, L"AnnotationTextBlock%d", i + 1);
                uiaInfoAnnotation.m_Name = pszNames[i / 2];
                uiaInfoAnnotation.m_AutomationID = pszNames[i / 2];
                uiaInfoAnnotation.m_cType = UIA_TextControlTypeId;
            }
            annotationElementInfos.push_back(uiaInfoAnnotation);
        }

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new xaml_controls::Button;

            auto annotationsVector = xaml_automation::AutomationProperties::GetAnnotations(button);
            VERIFY_IS_NOT_NULL(annotationsVector);

            for (size_t i = 0; i < annotationElementInfos.size(); i++)
            {
                xaml_automation::AutomationAnnotation^ automationAnnotation;
                if (annotationElementInfos[i].m_Name != nullptr)
                {
                    xaml_controls::TextBlock^ textBlock = ref new xaml_controls::TextBlock;
                    xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(annotationElementInfos[i].m_Name));
                    textBlock->Name = ref new Platform::String(annotationElementInfos[i].m_Name);
                    stackPanel->Children->Append(textBlock);
                    automationAnnotation = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::Comment, textBlock);
                }
                else
                {
                    automationAnnotation = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::SpellingError);
                }

                annotationsVector->Append(automationAnnotation);
            }

            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            stackPanel->Children->Append(button);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4Target));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)annotationElementInfos.size());
            for (LONG i = 0; i < (LONG)annotationElementInfos.size(); i++)
            {
                int annotationType;
                SafeArrayGetElement(psaAnnotationTypes, &i, (void*)&annotationType);
                if (annotationElementInfos[i].m_Name != nullptr)
                {
                    VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Comment);
                }
                else
                {
                    VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);
                }
            }
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, (int)annotationElementInfos.size());

                for (LONG i = 0; i < (LONG)annotationElementInfos.size(); i++)
                {
                    wrl::ComPtr<IUIAutomationElement> spElement;
                    VERIFY_SUCCEEDED(spAnnotationObjects->GetElement(i, &spElement));

                    if (annotationElementInfos[i].m_Name != nullptr)
                    {
                        auto spAutomationClientManagerAnnotation = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(annotationElementInfos[i]);
                        wrl::ComPtr<IUIAutomationElement> spUIAutomationElementAnnotation;
                        spAutomationClientManagerAnnotation->GetCurrentUIAutomationElement(&spUIAutomationElementAnnotation);
                        WEX::Common::Throw::IfNull(spUIAutomationElementAnnotation.Get());

                        WEX::Common::Throw::IfNull(spElement.Get());
                        VERIFY_IS_TRUE(spAutomationClientManagerAnnotation->IsElementSame(spElement.Get()));
                    }
                    else
                    {
                        VERIFY_ARE_EQUAL(spElement.Get(), nullptr);
                    }
                }
            }
            else
            {
                VERIFY_IS_NOT_NULL(spAnnotationObjects.Get());
            }
        });

        for (int i = 0; i < 2; i++)
        {
            delete[] pszNames[i];
        }
    }

    void AnnotationsIntegrationTests::_VerifyXAnnotationsSomeWithObjects_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationPeer");

        xaml_controls::StackPanel^ stackPanel;
        CustomButton^ button;
        wfc::IVector<xaml::UIElement^>^ textBlocks;

        Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
        uiaInfoTarget.m_Name = L"AnnotatedButton";
        uiaInfoTarget.m_AutomationID = L"AnnotatedButton";
        uiaInfoTarget.m_cType = UIA_ButtonControlTypeId;

        WCHAR *pszNames[2];
        std::vector<Automation::AutomationClient::UIAElementInfo> annotationElementInfos;
        for (int i = 0; i < 4; i++)
        {
            Automation::AutomationClient::UIAElementInfo uiaInfoAnnotation;
            if (i % 2 == 0)
            {
                uiaInfoAnnotation.m_Name = nullptr;
                uiaInfoAnnotation.m_AutomationID = nullptr;
            }
            else
            {
                pszNames[i / 2] = new WCHAR[wcslen(L"AnnotationTextBlockx") + 1];
                StringCchPrintf(pszNames[i / 2], wcslen(L"AnnotationTextBlockx") + 1, L"AnnotationTextBlock%d", i + 1);
                uiaInfoAnnotation.m_Name = pszNames[i / 2];
                uiaInfoAnnotation.m_AutomationID = pszNames[i / 2];
                uiaInfoAnnotation.m_cType = UIA_TextControlTypeId;
            }
            annotationElementInfos.push_back(uiaInfoAnnotation);
        }

        RunOnUIThread([&]()
        {
            stackPanel = ref new xaml_controls::StackPanel;
            button = ref new CustomButton;
            textBlocks = ref new Platform::Collections::Vector<xaml::UIElement^>;

            for (size_t i = 0; i < annotationElementInfos.size(); i++)
            {
                xaml_controls::TextBlock^ textBlock = nullptr;

                if (annotationElementInfos[i].m_Name != nullptr)
                {
                    textBlock = ref new xaml_controls::TextBlock;
                    xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(annotationElementInfos[i].m_Name));
                    textBlock->Name = ref new Platform::String(annotationElementInfos[i].m_Name);
                    stackPanel->Children->Append(textBlock);
                }

                textBlocks->Append(textBlock);
            }

            button->SetAPAnnotationObjects(textBlocks);
            button->SetTestCase(XAnnotationsSomeWithObjects);
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfoTarget.m_Name));
            stackPanel->Children->Append(button);

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTarget);
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementTarget;
            spAutomationClientManagerTarget->GetCurrentUIAutomationElement(&spUIAutomationElementTarget);
            WEX::Common::Throw::IfNull(spUIAutomationElementTarget.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4Target;
            VERIFY_SUCCEEDED(spUIAutomationElementTarget.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4Target));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)annotationElementInfos.size());
            for (LONG i = 0; i < (LONG)annotationElementInfos.size(); i++)
            {
                int annotationType;
                SafeArrayGetElement(psaAnnotationTypes, &i, (void*)&annotationType);
                if (annotationElementInfos[i].m_Name != nullptr)
                {
                    VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Comment);
                }
                else
                {
                    VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);
                }
            }
            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4Target->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
                VERIFY_ARE_EQUAL(length, (int)annotationElementInfos.size());

                for (LONG i = 0; i < (LONG)annotationElementInfos.size(); i++)
                {
                    wrl::ComPtr<IUIAutomationElement> spElement;
                    VERIFY_SUCCEEDED(spAnnotationObjects->GetElement(i, &spElement));

                    if (annotationElementInfos[i].m_Name != nullptr)
                    {
                        auto spAutomationClientManagerAnnotation = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(annotationElementInfos[i]);
                        wrl::ComPtr<IUIAutomationElement> spUIAutomationElementAnnotation;
                        spAutomationClientManagerAnnotation->GetCurrentUIAutomationElement(&spUIAutomationElementAnnotation);
                        WEX::Common::Throw::IfNull(spUIAutomationElementAnnotation.Get());

                        WEX::Common::Throw::IfNull(spElement.Get());
                        VERIFY_IS_TRUE(spAutomationClientManagerAnnotation->IsElementSame(spElement.Get()));
                    }
                    else
                    {
                        VERIFY_ARE_EQUAL(spElement.Get(), nullptr);
                    }
                }
            }
            else
            {
                VERIFY_IS_NOT_NULL(spAnnotationObjects.Get());
            }
        });

        for (int i = 0; i < 2; i++)
        {
            delete[] pszNames[i];
        }
    }


    void AnnotationsIntegrationTests::_VerifyAllAnnotationTypes_AutomationPropertiesMarkup()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationProperties in markup");

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AnnotatedButton";
        uiaInfo.m_AutomationID = L"AnnotatedButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            Platform::String^ xaml =
                "<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' AutomationProperties.Name='AnnotatedButton'>"
                "    <AutomationProperties.Annotations>"
                "        <AutomationAnnotation Type='Unknown' />"
                "        <AutomationAnnotation Type='SpellingError' />"
                "        <AutomationAnnotation Type='GrammarError' />"
                "        <AutomationAnnotation Type='Comment' />"
                "        <AutomationAnnotation Type='FormulaError' />"
                "        <AutomationAnnotation Type='TrackChanges' />"
                "        <AutomationAnnotation Type='Header' />"
                "        <AutomationAnnotation Type='Footer' />"
                "        <AutomationAnnotation Type='Highlighted' />"
                "        <AutomationAnnotation Type='Endnote' />"
                "        <AutomationAnnotation Type='Footnote' />"
                "        <AutomationAnnotation Type='InsertionChange' />"
                "        <AutomationAnnotation Type='DeletionChange' />"
                "        <AutomationAnnotation Type='MoveChange' />"
                "        <AutomationAnnotation Type='FormatChange' />"
                "        <AutomationAnnotation Type='UnsyncedChange' />"
                "        <AutomationAnnotation Type='EditingLockedChange' />"
                "        <AutomationAnnotation Type='ExternalChange' />"
                "        <AutomationAnnotation Type='ConflictingChange' />"
                "        <AutomationAnnotation Type='Author' />"
                "        <AutomationAnnotation Type='AdvancedProofingIssue' />"
                "        <AutomationAnnotation Type='DataValidationError' />"
                "        <AutomationAnnotation Type='CircularReferenceError' />"
                "    </AutomationProperties.Annotations>"
                "</Button>";

            TestServices::WindowHelper->WindowContent = static_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(xaml));
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)23);

            LONG idx = 0;
            int annotationType;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Unknown);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::GrammarError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Comment);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::FormulaError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::TrackChanges);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Header);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Footer);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Highlighted);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Endnote);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Footnote);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::InsertionChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::DeletionChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::MoveChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::FormatChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::UnsyncedChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::EditingLockedChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::ExternalChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::ConflictingChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Author);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::AdvancedProofingIssue);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::DataValidationError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::CircularReferenceError);

            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spAnnotationObjects.Get() == nullptr || length == 0);
        });
    }

    void AnnotationsIntegrationTests::_VerifyAllAnnotationTypes_AutomationPropertiesCodebehind()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationProperties in codebehind");

        xaml_controls::Button^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AnnotatedButton";
        uiaInfo.m_AutomationID = L"AnnotatedButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button;
            xaml_automation::AutomationAnnotation^ automationAnnotation1 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::Unknown);
            xaml_automation::AutomationAnnotation^ automationAnnotation2 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::SpellingError);
            xaml_automation::AutomationAnnotation^ automationAnnotation3 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::GrammarError);
            xaml_automation::AutomationAnnotation^ automationAnnotation4 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::Comment);
            xaml_automation::AutomationAnnotation^ automationAnnotation5 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::FormulaError);
            xaml_automation::AutomationAnnotation^ automationAnnotation6 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::TrackChanges);
            xaml_automation::AutomationAnnotation^ automationAnnotation7 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::Header);
            xaml_automation::AutomationAnnotation^ automationAnnotation8 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::Footer);
            xaml_automation::AutomationAnnotation^ automationAnnotation9 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::Highlighted);
            xaml_automation::AutomationAnnotation^ automationAnnotation10 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::Endnote);
            xaml_automation::AutomationAnnotation^ automationAnnotation11 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::Footnote);
            xaml_automation::AutomationAnnotation^ automationAnnotation12 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::InsertionChange);
            xaml_automation::AutomationAnnotation^ automationAnnotation13 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::DeletionChange);
            xaml_automation::AutomationAnnotation^ automationAnnotation14 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::MoveChange);
            xaml_automation::AutomationAnnotation^ automationAnnotation15 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::FormatChange);
            xaml_automation::AutomationAnnotation^ automationAnnotation16 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::UnsyncedChange);
            xaml_automation::AutomationAnnotation^ automationAnnotation17 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::EditingLockedChange);
            xaml_automation::AutomationAnnotation^ automationAnnotation18 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::ExternalChange);
            xaml_automation::AutomationAnnotation^ automationAnnotation19 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::ConflictingChange);
            xaml_automation::AutomationAnnotation^ automationAnnotation20 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::Author);
            xaml_automation::AutomationAnnotation^ automationAnnotation21 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::AdvancedProofingIssue);
            xaml_automation::AutomationAnnotation^ automationAnnotation22 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::DataValidationError);
            xaml_automation::AutomationAnnotation^ automationAnnotation23 = ref new xaml_automation::AutomationAnnotation(xaml_automation::AnnotationType::CircularReferenceError);
            auto annotationsVector = xaml_automation::AutomationProperties::GetAnnotations(button);
            VERIFY_IS_NOT_NULL(annotationsVector);
            annotationsVector->Append(automationAnnotation1);
            annotationsVector->Append(automationAnnotation2);
            annotationsVector->Append(automationAnnotation3);
            annotationsVector->Append(automationAnnotation4);
            annotationsVector->Append(automationAnnotation5);
            annotationsVector->Append(automationAnnotation6);
            annotationsVector->Append(automationAnnotation7);
            annotationsVector->Append(automationAnnotation8);
            annotationsVector->Append(automationAnnotation9);
            annotationsVector->Append(automationAnnotation10);
            annotationsVector->Append(automationAnnotation11);
            annotationsVector->Append(automationAnnotation12);
            annotationsVector->Append(automationAnnotation13);
            annotationsVector->Append(automationAnnotation14);
            annotationsVector->Append(automationAnnotation15);
            annotationsVector->Append(automationAnnotation16);
            annotationsVector->Append(automationAnnotation17);
            annotationsVector->Append(automationAnnotation18);
            annotationsVector->Append(automationAnnotation19);
            annotationsVector->Append(automationAnnotation20);
            annotationsVector->Append(automationAnnotation21);
            annotationsVector->Append(automationAnnotation22);
            annotationsVector->Append(automationAnnotation23);
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)23);

            LONG idx = 0;
            int annotationType;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Unknown);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::GrammarError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Comment);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::FormulaError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::TrackChanges);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Header);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Footer);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Highlighted);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Endnote);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Footnote);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::InsertionChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::DeletionChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::MoveChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::FormatChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::UnsyncedChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::EditingLockedChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::ExternalChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::ConflictingChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Author);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::AdvancedProofingIssue);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::DataValidationError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::CircularReferenceError);

            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spAnnotationObjects.Get() == nullptr || length == 0);
        });
    }

    void AnnotationsIntegrationTests::_VerifyAllAnnotationTypes_AutomationPeer()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        LOG_OUTPUT(L"Verifying annotations added from AutomationPeer");

        CustomButton^ button;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AnnotatedButton";
        uiaInfo.m_AutomationID = L"AnnotatedButton";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            button = ref new CustomButton;
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));
            button->SetTestCase(AllAnnotationTypes);

            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(button);
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            wrl::ComPtr<IUIAutomationElement4> spUIAutomationElement4;
            VERIFY_SUCCEEDED(spUIAutomationElement.Get()->QueryInterface(__uuidof(IUIAutomationElement4), &spUIAutomationElement4));

            SAFEARRAY *psaAnnotationTypes;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationTypes(&psaAnnotationTypes));
            WEX::Common::Throw::IfNull(psaAnnotationTypes);
            VERIFY_ARE_EQUAL(_GetSafeArrayItemCount(psaAnnotationTypes), (ULONG)23);

            LONG idx = 0;
            int annotationType;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Unknown);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::SpellingError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::GrammarError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Comment);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::FormulaError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::TrackChanges);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Header);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Footer);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Highlighted);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Endnote);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Footnote);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::InsertionChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::DeletionChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::MoveChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::FormatChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::UnsyncedChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::EditingLockedChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::ExternalChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::ConflictingChange);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::Author);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::AdvancedProofingIssue);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::DataValidationError);

            idx++;
            SafeArrayGetElement(psaAnnotationTypes, &idx, (void*)&annotationType);
            VERIFY_ARE_EQUAL((xaml_automation::AnnotationType)annotationType, xaml_automation::AnnotationType::CircularReferenceError);

            SafeArrayDestroy(psaAnnotationTypes);

            wrl::ComPtr<IUIAutomationElementArray> spAnnotationObjects;
            int length = 0;
            VERIFY_SUCCEEDED(spUIAutomationElement4->get_CurrentAnnotationObjects(&spAnnotationObjects));
            if (spAnnotationObjects.Get() != nullptr)
            {
                VERIFY_SUCCEEDED(spAnnotationObjects->get_Length(&length));
            }
            VERIFY_IS_TRUE(spAnnotationObjects.Get() == nullptr || length == 0);
        });
    }

    ULONG AnnotationsIntegrationTests::_GetSafeArrayItemCount(SAFEARRAY *psa)
    {
        LONG lBound;
        LONG uBound;
        VERIFY_SUCCEEDED(SafeArrayGetLBound(psa, 1, &lBound));
        VERIFY_SUCCEEDED(SafeArrayGetUBound(psa, 1, &uBound));

        if (uBound != 0xffffffff)
        {
            return uBound - lBound + 1;
        }

        return 0;
    }


    wfc::IVector<xaml_automation_peers::AutomationPeerAnnotation^>^ AnnotationsIntegrationTests::CustomButtonAP::GetAnnotationsCore()
    {
        wfc::IVector<xaml_automation_peers::AutomationPeerAnnotation^>^ annotations =
            ref new Platform::Collections::Vector<xaml_automation_peers::AutomationPeerAnnotation^>;

        switch (m_tc)
        {
            case OneAnnotationNoObject:
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::SpellingError));
                break;

            case OneAnnotationOneObject:
            case XAnnotationsAllWithObjects:
            case XAnnotationsSomeWithObjects:
                for (unsigned int i = 0; i < m_objects->Size; i++)
                {
                    if (m_objects->GetAt(i) == nullptr)
                    {
                        annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::SpellingError));
                    }
                    else
                    {
                        annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::Comment, xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(m_objects->GetAt(i))));
                    }
                }
                break;

            case XAnnotationsNoObjects:
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::SpellingError));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::GrammarError));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::SpellingError));
                break;

            case AllAnnotationTypes:
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::Unknown));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::SpellingError));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::GrammarError));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::Comment));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::FormulaError));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::TrackChanges));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::Header));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::Footer));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::Highlighted));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::Endnote));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::Footnote));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::InsertionChange));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::DeletionChange));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::MoveChange));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::FormatChange));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::UnsyncedChange));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::EditingLockedChange));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::ExternalChange));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::ConflictingChange));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::Author));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::AdvancedProofingIssue));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::DataValidationError));
                annotations->Append(ref new xaml_automation_peers::AutomationPeerAnnotation(xaml_automation::AnnotationType::CircularReferenceError));
                break;

        }

        return annotations;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
