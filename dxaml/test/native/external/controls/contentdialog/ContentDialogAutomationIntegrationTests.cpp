// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ContentDialogAutomationIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <generic\ButtonBaseTests.h>
#include <AutomationClient\AutomationGenericTests.h>
#include <Patterns\InvokePatternHandler.h>
#include <TreeHelper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ContentDialog {

    WCHAR ContentDialogAutomationIntegrationTests::s_contentDialogName[] = L"TestDialog";

    bool ContentDialogAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ContentDialogAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ContentDialogAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    xaml_controls::ContentDialog^ ContentDialogAutomationIntegrationTests::SetupContentDialog()
    {
        xaml_controls::ContentDialog^ contentDialog = nullptr;

        RunOnUIThread([&]()
        {
            contentDialog = safe_cast<xaml_controls::ContentDialog^>(xaml_markup::XamlReader::Load(
                L"<ContentDialog xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    PrimaryButtonText='P'"
                L"    SecondaryButtonText='S'"
                L"    Title='TestDialog'>"
                L"    <StackPanel>"
                L"        <Button Content='B1'/>"
                L"    </StackPanel>"
                L"</ContentDialog>"
                ));

            auto rootGrid = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            contentDialog->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
        });

        return contentDialog;
    }

    wf::IAsyncOperation<xaml_controls::ContentDialogResult>^
        ContentDialogAutomationIntegrationTests::OpenContentDialog(xaml_controls::ContentDialog^ contentDialog)
    {
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Opened);
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Loaded);

        auto openedEvent = std::make_shared<Event>();
        auto loadedEvent = std::make_shared<Event>();

        wf::IAsyncOperation<xaml_controls::ContentDialogResult>^ showAsyncOperation = nullptr;

        openedRegistration.Attach(contentDialog,
            ref new wf::TypedEventHandler<xaml_controls::ContentDialog^, xaml_controls::ContentDialogOpenedEventArgs^>(
                [openedEvent](xaml_controls::ContentDialog^ sender, xaml_controls::ContentDialogOpenedEventArgs^ args)
        {
            openedEvent->Set();
        }));

        loadedRegistration.Attach(contentDialog, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
        {
            loadedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            showAsyncOperation = contentDialog->ShowAsync();
        });

        // There are three things that we need to wait for when opening a ContentDialog:
        //
        // First, the opened event needs to be fired, indicating that the ContentDialog has begun opening.
        // Second, the loaded event needs to be fired, indicating that the ContentDialog has been added to the visual tree.
        // Third, we need to wait for the transition-in animation to complete for the ContentDialog to be interactable.
        //
        openedEvent->WaitForDefault();
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        return showAsyncOperation;
    }

    void ContentDialogAutomationIntegrationTests::CloseContentDialog(xaml_controls::ContentDialog^ contentDialog)
    {
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Closed);
        auto unloadedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Unloaded);

        auto closedEvent = std::make_shared<Event>();
        auto unloadedEvent = std::make_shared<Event>();

        closedRegistration.Attach(contentDialog,
            ref new wf::TypedEventHandler<xaml_controls::ContentDialog^, xaml_controls::ContentDialogClosedEventArgs^>(
                [closedEvent](xaml_controls::ContentDialog^ sender, xaml_controls::ContentDialogClosedEventArgs^ args)
        {
            closedEvent->Set();
        }));

        unloadedRegistration.Attach(contentDialog, ref new xaml::RoutedEventHandler([unloadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
        {
            unloadedEvent->Set();
        }));

        RunOnUIThread([&]() { contentDialog->Hide(); });

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void ContentDialogAutomationIntegrationTests::VerifyContentDialogAutomationName(xaml_controls::ContentDialog^ contentDialog, const WCHAR *expectedName)
    {
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = expectedName;

        OpenContentDialog(contentDialog);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spContentDialog;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            CONTROLTYPEID controlTypeId;
            AutoBSTR name;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spContentDialog);

            LOG_OUTPUT(L"Verifying that the UIA client-side node for ContentDialog exists.");
            VERIFY_IS_NOT_NULL(spContentDialog);

            LOG_OUTPUT(L"Verifying that this client-side node is the right ControlType.");
            spContentDialog->get_CurrentControlType(&controlTypeId);
            VERIFY_ARE_EQUAL(UIA_WindowControlTypeId, controlTypeId);

            LOG_OUTPUT(L"Verifying the UIA Name property from client-side node for ContentDialog.");
            spContentDialog->get_CurrentName(name.ReleaseAndGetAddressOf());
            AutoBSTR::VerifyAreEqual(expectedName, name);
        });

        CloseContentDialog(contentDialog);
    }

    //
    // Test Cases
    //
    void ContentDialogAutomationIntegrationTests::VerifyDefaultAutomationName()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ContentDialog^ contentDialogWithName = SetupContentDialog();
        xaml_controls::ContentDialog^ contentDialogWithTitle = SetupContentDialog();
        xaml_controls::ContentDialog^ contentDialogWithContent = SetupContentDialog();
        xaml_controls::ContentDialog^ contentDialogWithLongContent = SetupContentDialog();
        xaml_controls::ContentDialog^ contentDialogWithContentWithCarriageReturn = SetupContentDialog();
        xaml_controls::ContentDialog^ contentDialogWithNothing = SetupContentDialog();

        RunOnUIThread([&]()
        {
            contentDialogWithName->Title = nullptr;
            xaml_automation::AutomationProperties::SetName(contentDialogWithName, ref new Platform::String(s_contentDialogName));

            contentDialogWithContent->Title = nullptr;
            contentDialogWithContent->Content = ref new Platform::String(s_contentDialogName);

            contentDialogWithLongContent->Title = nullptr;
            contentDialogWithLongContent->Content = ref new Platform::String(L"    1  2    3  4    5  6    7  8    9  10    11  12    13  14    15  16    17  18    19  20    21  22");

            contentDialogWithContentWithCarriageReturn->Title = nullptr;
            contentDialogWithContentWithCarriageReturn->Content = ref new Platform::String(L"1 2 3 4 5\r\n6 7 8 9 10");

            contentDialogWithNothing->Title = nullptr;
            contentDialogWithNothing->Content = nullptr;
        });

        LOG_OUTPUT(L"Performing verification on the ContentDialog with AutomationProperties.Name set.");
        VerifyContentDialogAutomationName(contentDialogWithName, s_contentDialogName);
        LOG_OUTPUT(L"Performing verification on the ContentDialog with a title.");
        VerifyContentDialogAutomationName(contentDialogWithTitle, s_contentDialogName);
        LOG_OUTPUT(L"Performing verification on the ContentDialog with content.");
        VerifyContentDialogAutomationName(contentDialogWithContent, s_contentDialogName);
        LOG_OUTPUT(L"Performing verification on the ContentDialog with long content.");
        VerifyContentDialogAutomationName(contentDialogWithLongContent, L"    1  2    3  4    5  6    7  8    9  10    11  12    13  14    15  16    17  18    19  20...");
        LOG_OUTPUT(L"Performing verification on the ContentDialog with content containing a carriage return.");
        VerifyContentDialogAutomationName(contentDialogWithContentWithCarriageReturn, L"1 2 3 4 5...");

        LOG_OUTPUT(L"Performing verification on the ContentDialog with nothing set. We have nothing with which to retrieve the ContentDialog, so we expect the automation peer to be unretrievable.");

        OpenContentDialog(contentDialogWithNothing);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spContentDialog;

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = s_contentDialogName;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spContentDialog);

            LOG_OUTPUT(L"Verifying that we couldn't get the UIA client-side node for ContentDialog.");
            VERIFY_IS_NULL(spContentDialog);
        });

        CloseContentDialog(contentDialogWithNothing);
    }

    void ContentDialogAutomationIntegrationTests::DoesSupportEssentialPatterns()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = s_contentDialogName;
        xaml_controls::ContentDialog^ contentDialog = nullptr;

        contentDialog = SetupContentDialog();
        OpenContentDialog(contentDialog);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spContentDialog;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            wrl::ComPtr<IUIAutomationWindowPattern> spUIAutomationWindowPattern;
            Common::AutoVariant autoVar;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spContentDialog);

            LOG_OUTPUT(L"Verifying UIA Client side node for ContentDialog exists.");
            VERIFY_IS_NOT_NULL(spContentDialog);
            LogThrow_IfFailedWithMessage(spContentDialog->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spUIAutomationWindowPattern), L"ComboBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed getting Selection Pattern.");
            VERIFY_IS_NOT_NULL(spUIAutomationWindowPattern);
        });

        CloseContentDialog(contentDialog);
    }

    void ContentDialogAutomationIntegrationTests::DoesPopupUseDialogsAutomationId()
    {
        TestCleanupWrapper cleanup;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = s_contentDialogName;

        xaml_controls::ContentDialog^ contentDialog = nullptr;
        const wchar_t* expectedAutomationId = L"CustomContentDialogAutomationId";

        contentDialog = SetupContentDialog();

        RunOnUIThread([&]()
        {
            xaml_automation::AutomationProperties::SetAutomationId(contentDialog, ref new Platform::String(expectedAutomationId));
        });

        OpenContentDialog(contentDialog);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            wrl::ComPtr<IUIAutomationElement> contentDialogPeer;
            automationClientManager->GetCurrentUIAutomationElement(&contentDialogPeer);

            AutoBSTR automationId;
            LogThrow_IfFailed(contentDialogPeer->get_CurrentAutomationId(automationId.ReleaseAndGetAddressOf()));
            AutoBSTR::VerifyAreEqual(expectedAutomationId, automationId);
        });

        CloseContentDialog(contentDialog);
    }

    void ContentDialogAutomationIntegrationTests::DoesAutomationNameUpdateWithInMarkupDialogs()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ContentDialog^ contentDialog = nullptr;
        const wchar_t* expectedFirstAutomationName = L"First Title";
        const wchar_t* expectedSecondAutomationName = L"Second Title";

        RunOnUIThread([&]()
        {
            contentDialog = safe_cast<xaml_controls::ContentDialog^>(xaml_markup::XamlReader::Load(
                LR"(<ContentDialog xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        PrimaryButtonText="Primary"
                        />)"));

            contentDialog->Title = ref new Platform::String(expectedFirstAutomationName);

            TestServices::WindowHelper->WindowContent = contentDialog;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the dialog with its initial title and verify that we can find the automation peer using that title.");
        OpenContentDialog(contentDialog);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = const_cast<wchar_t*>(expectedFirstAutomationName);

            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            LOG_OUTPUT(L"Verify that we can find the dialog using the title as the automation name.");
            wrl::ComPtr<IUIAutomationElement> contentDialogPeer;
            automationClientManager->GetCurrentUIAutomationElement(&contentDialogPeer);
            VERIFY_IS_NOT_NULL(contentDialogPeer.Get());

            LOG_OUTPUT(L"Make sure we found the actual dialog's peer and not just the title's textblock.");
            CONTROLTYPEID controlType;
            LogThrow_IfFailed(contentDialogPeer->get_CurrentControlType(&controlType));
            VERIFY_ARE_EQUAL(controlType, UIA_WindowControlTypeId);
        });

        LOG_OUTPUT(L"Make sure we found the actual dialog's peer and not just the title's textblock.");
        CloseContentDialog(contentDialog);

        RunOnUIThread([&]()
        {
            contentDialog->Title = ref new Platform::String(expectedSecondAutomationName);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Re-open the dialog with its second title and verify that we can find the automation peer using that title.");
        OpenContentDialog(contentDialog);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = const_cast<wchar_t*>(expectedSecondAutomationName);

            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            LOG_OUTPUT(L"Verify that we can find the dialog using the title as the automation name.");
            wrl::ComPtr<IUIAutomationElement> contentDialogPeer;
            automationClientManager->GetCurrentUIAutomationElement(&contentDialogPeer);
            VERIFY_IS_NOT_NULL(contentDialogPeer.Get());

            LOG_OUTPUT(L"Make sure we found the actual dialog's peer and not just the title's textblock.");
            CONTROLTYPEID controlType;
            LogThrow_IfFailed(contentDialogPeer->get_CurrentControlType(&controlType));
            VERIFY_ARE_EQUAL(controlType, UIA_WindowControlTypeId);
        });

        CloseContentDialog(contentDialog);
    }

    void ContentDialogAutomationIntegrationTests::DoesContentDialogSetIsDialogPropertyToTrueByDefault()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ContentDialog^ contentDialog = SetupContentDialog();
        VERIFY_IS_NOT_NULL(contentDialog);

        OpenContentDialog(contentDialog);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = s_contentDialogName;

            // Get the popup window where IsDialog is set
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            VERIFY_IS_NOT_NULL(automationClientManager);

            wrl::ComPtr<IUIAutomationElement> element;
            automationClientManager->GetCurrentUIAutomationElement(&element);
            VERIFY_IS_NOT_NULL(element);

            wrl::ComPtr<IUIAutomationElement9> element9;
            VERIFY_SUCCEEDED(element->QueryInterface(__uuidof(IUIAutomationElement9), &element9));
            VERIFY_IS_NOT_NULL(element9);

            BOOL isDialog = FALSE;
            VERIFY_SUCCEEDED(element9->get_CurrentIsDialog(&isDialog));
            VERIFY_IS_TRUE(isDialog == TRUE);
        });

        CloseContentDialog(contentDialog);
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Button
