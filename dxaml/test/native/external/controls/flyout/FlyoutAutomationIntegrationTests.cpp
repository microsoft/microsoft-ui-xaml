// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include "FlyoutAutomationIntegrationTests.h"
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <AutomationClient\AutomationClientInitializer.h>
#include "TestCleanupWrapper.h"

#include <CommonInputHelper.h>
#include <ControlHelper.h>
#include <FlyoutHelper.h>
#include <PopupHelper.h>
#include <UIAutomationHelper.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Flyout {

    WCHAR FlyoutAutomationIntegrationTests::s_automationName[] = L"TestFlyoutName";
    WCHAR FlyoutAutomationIntegrationTests::s_automationId[] = L"TestFlyoutId";
    WCHAR FlyoutAutomationIntegrationTests::s_automationItemStatus[] = L"TestFlyout";
    CONTROLTYPEID FlyoutAutomationIntegrationTests::s_automationTypeId = UIA_WindowControlTypeId;

    bool FlyoutAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool FlyoutAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool FlyoutAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void FlyoutAutomationIntegrationTests::VerifyAutomationProperties()
    {
        TestCleanupWrapper cleanup;
        auto flyoutWithName = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Left);
        auto flyoutWithContent = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Left);
        auto flyoutWithLongContent = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Left);
        auto flyoutWithWithContentWithCarriageReturn = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Left);
        auto flyoutWithNothing = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Left);

        RunOnUIThread([&]()
        {
            xaml_automation::AutomationProperties::SetName(flyoutWithName, ref new Platform::String(s_automationName));

            flyoutWithName->ShouldConstrainToRootBounds = false;

            auto textBlock = ref new xaml_controls::TextBlock();
            textBlock->Text = ref new Platform::String(s_automationName);
            flyoutWithContent->Content = textBlock;
            flyoutWithContent->ShouldConstrainToRootBounds = false;

            textBlock = ref new xaml_controls::TextBlock();
            textBlock->Text = ref new Platform::String(L"    1  2    3  4    5  6    7  8    9  10    11  12    13  14    15  16    17  18    19  20    21  22");
            flyoutWithLongContent->Content = textBlock;
            flyoutWithLongContent->ShouldConstrainToRootBounds = false;

            textBlock = ref new xaml_controls::TextBlock();
            textBlock->Text = ref new Platform::String(L"1 2 3 4 5\r\n6 7 8 9 10");
            flyoutWithWithContentWithCarriageReturn->Content = textBlock;
            flyoutWithWithContentWithCarriageReturn->ShouldConstrainToRootBounds = false;

            flyoutWithNothing->Content = nullptr;
            flyoutWithNothing->ShouldConstrainToRootBounds = false;
        });

        auto target = FlyoutHelper::CreateTarget(
            300 /*width*/, 300 /*height*/,
            ThicknessHelper::FromUniformLength(100),
            xaml::HorizontalAlignment::Left,
            xaml::VerticalAlignment::Center);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Performing verification on the Flyout with AutomationProperties.Name set.");
        VerifyFlyoutAutomationName(flyoutWithName, target, s_automationName);
        LOG_OUTPUT(L"Performing verification on the Flyout with content.");
        VerifyFlyoutAutomationName(flyoutWithContent, target, s_automationName);
        LOG_OUTPUT(L"Performing verification on the Flyout with long content.");
        VerifyFlyoutAutomationName(flyoutWithLongContent, target, L"    1  2    3  4    5  6    7  8    9  10    11  12    13  14    15  16    17  18    19  20...");
        LOG_OUTPUT(L"Performing verification on the Flyout with content containing a carriage return..");
        VerifyFlyoutAutomationName(flyoutWithWithContentWithCarriageReturn, target, L"1 2 3 4 5...");

        LOG_OUTPUT(L"Performing verification on the Flyout with nothing set. We have nothing with which to retrieve the ContentDialog, so we expect the automation peer to be unretrievable.");

        RunOnUIThread([&]()
        {
            xaml_controls::Flyout::SetAttachedFlyout(target, flyoutWithNothing);
        });

        FlyoutHelper::OpenFlyout(flyoutWithNothing, target, FlyoutOpenMethod::Programmatic_ShowAttachedFlyout);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(target);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spFlyout;

            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = s_automationName;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, target);
                
            spAutomationClientManager->GetCurrentUIAutomationElement(&spFlyout);

            LOG_OUTPUT(L"Verifying that we couldn't get the UIA client-side node for Flyout.");
            VERIFY_IS_NULL(spFlyout);
        });

        FlyoutHelper::HideFlyout(flyoutWithNothing);
    }

    void FlyoutAutomationIntegrationTests::VerifyFlyoutAutomationName(xaml_controls::Flyout^ flyout, xaml::FrameworkElement^ target, const WCHAR *expectedName)
    {
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = expectedName;

        RunOnUIThread([&]()
        {
            xaml_controls::Flyout::SetAttachedFlyout(target, flyout);
        });

        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAttachedFlyout);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(target);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spFlyout;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            CONTROLTYPEID controlTypeId;
            AutoBSTR name;
            AutoBSTR className;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, target);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spFlyout);

            LOG_OUTPUT(L"Verifying that the UIA client-side node for Flyout exists.");
            VERIFY_IS_NOT_NULL(spFlyout);

            LOG_OUTPUT(L"Verifying that this client-side node is the right ControlType.");
            spFlyout->get_CurrentControlType(&controlTypeId);
            VERIFY_ARE_EQUAL(UIA_PaneControlTypeId, controlTypeId);

            LOG_OUTPUT(L"Verifying the UIA Name property from client-side node for ContentDialog.");
            spFlyout->get_CurrentName(name.ReleaseAndGetAddressOf());
            AutoBSTR::VerifyAreEqual(expectedName, name);

            LOG_OUTPUT(L"Verifying the UIA class name property from client-side node for FlyoutPresenter.");
            spFlyout->get_CurrentClassName(className.ReleaseAndGetAddressOf());
            AutoBSTR::VerifyAreEqual(L"Flyout", className);
        });

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutAutomationIntegrationTests::VerifyUIAFocusEntersFlyoutWithContentControl()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ flyoutButton = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto openedEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Loaded);
        auto openedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

        auto focusChangeEvent = std::make_shared<Event>();
        wrl::ComPtr<AutomationClient::AutomationFocusChangeHandler> automationFocusChangeHandler;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"FlyoutButtonName";

        RunOnUIThread([&]()
        {
            flyoutButton = safe_cast<xaml_controls::Button^> (xaml_markup::XamlReader::Load(
                LR"(<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' AutomationProperties.Name='FlyoutButton' Content='Button'>
                    <Button.Flyout>
                        <Flyout ShouldConstrainToRootBounds='True'>
                            <ContentControl>
                                <TextBlock Text='Hello World'/>
                            </ContentControl>
                        </Flyout>
                    </Button.Flyout>
                </Button>)"));

            loadedRegistration.Attach(flyoutButton, [loadedEvent]() { loadedEvent->Set(); });
            openedRegistration.Attach(flyoutButton->Flyout, [openedEvent]() { openedEvent->Set(); });
            closedRegistration.Attach(flyoutButton->Flyout, [closedEvent]() { closedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = flyoutButton;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            automationFocusChangeHandler.Attach(new AutomationClient::AutomationFocusChangeHandler(automationClientManager, focusChangeEvent, TreeScope_Subtree));
            automationFocusChangeHandler->Init();
        });

        ControlHelper::EnsureFocused(flyoutButton);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            automationFocusChangeHandler->AttachEventHandler();
        });

        LOG_OUTPUT(L"Open the flyout with the keyboard.");
        CommonInputHelper::Accept(InputDevice::Keyboard);

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            automationFocusChangeHandler->Confirm();

            Common::AutoBSTR focusedElementClassName;

            automationFocusChangeHandler->GetLastFocusedElement()->get_CurrentClassName(focusedElementClassName.ReleaseAndGetAddressOf());

            LOG_OUTPUT(L"The currently focused element should be the popup now.");
            Common::AutoBSTR::VerifyAreEqual(L"Popup", focusedElementClassName);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Close the flyout.");
            flyoutButton->Flyout->Hide();
        });

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            LOG_OUTPUT(L"Focus should have moved back to the button.");
            automationFocusChangeHandler->Confirm();

            Common::AutoBSTR focusedElementClassName;

            automationFocusChangeHandler->GetLastFocusedElement()->get_CurrentClassName(focusedElementClassName.ReleaseAndGetAddressOf());

            Common::AutoBSTR::VerifyAreEqual(L"Button", focusedElementClassName);
        });

        LOG_OUTPUT(L"Open the flyout with the keyboard again.");
        CommonInputHelper::Accept(InputDevice::Keyboard);

        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            automationFocusChangeHandler->Confirm();

            Common::AutoBSTR focusedElementClassName;

            automationFocusChangeHandler->GetLastFocusedElement()->get_CurrentClassName(focusedElementClassName.ReleaseAndGetAddressOf());

            LOG_OUTPUT(L"The currently focused element should be the popup again.");
            Common::AutoBSTR::VerifyAreEqual(L"Popup", focusedElementClassName);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Close the flyout.");
            flyoutButton->Flyout->Hide();
        });

        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            automationFocusChangeHandler->RemoveEventHandler();
        });
    }

    void FlyoutAutomationIntegrationTests::DoesFlyoutSetIsDialogPropertyToTrueByDefault()
    {
        TestCleanupWrapper cleanup;

        auto flyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Left);

        auto target = FlyoutHelper::CreateTarget(
            300 /*width*/, 300 /*height*/,
            ThicknessHelper::FromUniformLength(100),
            xaml::HorizontalAlignment::Left,
            xaml::VerticalAlignment::Center);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);
            flyout->ShouldConstrainToRootBounds = false;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(target);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = L"Popup";
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, target);

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

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutAutomationIntegrationTests::VerifyAutomationNameAndIdPropagateToPresenter()
    {
        Platform::String^ flyoutName = L"FlyoutName";
        Platform::String^ flyoutId = L"FlyoutId";

        TestCleanupWrapper cleanup;

        auto flyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Left);

        auto target = FlyoutHelper::CreateTarget(
            300 /*width*/, 300 /*height*/,
            ThicknessHelper::FromUniformLength(100),
            xaml::HorizontalAlignment::Left,
            xaml::VerticalAlignment::Center);

        RunOnUIThread([&]()
        {
            xaml_automation::AutomationProperties::SetName(flyout, flyoutName);
            xaml_automation::AutomationProperties::SetAutomationId(flyout, flyoutId);

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);
            flyout->ShouldConstrainToRootBounds = false;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(target);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = flyoutName->Data();
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, target);

            VERIFY_IS_NOT_NULL(automationClientManager);

            wrl::ComPtr<IUIAutomationElement> element;
            automationClientManager->GetCurrentUIAutomationElement(&element);
            VERIFY_IS_NOT_NULL(element);

            AutoBSTR automationId;
            VERIFY_SUCCEEDED(element->get_CurrentAutomationId(automationId.ReleaseAndGetAddressOf()));
            AutoBSTR::VerifyAreEqual(flyoutId->Data(), automationId);
        });

        FlyoutHelper::HideFlyout(flyout);
    }

    void FlyoutAutomationIntegrationTests::VerifyAutomationNameAndIdDoNotOverwritePresenterValues()
    {
        Platform::String^ flyoutName = L"FlyoutName";
        Platform::String^ flyoutId = L"FlyoutId";
        Platform::String^ flyoutPresenterName = L"FlyoutPresenterName";
        Platform::String^ flyoutPresenterId = L"FlyoutPresenterId";
        Platform::String^ styleXaml = L"<Style"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"    TargetType='FlyoutPresenter'"
                L"    BasedOn='{StaticResource DefaultFlyoutPresenterStyle}'>"
                L"    <Setter Property='AutomationProperties.AutomationId' Value='" + flyoutPresenterId + "' />"
                L"    <Setter Property='AutomationProperties.Name' Value='" + flyoutPresenterName + "' />"
                L"</Style>";

        TestCleanupWrapper cleanup;

        auto flyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Left);

        RunOnUIThread([&]()
        {
            auto style = safe_cast<Style^>(xaml_markup::XamlReader::Load(styleXaml));
            flyout->FlyoutPresenterStyle = style;
        });

        auto target = FlyoutHelper::CreateTarget(
            300 /*width*/, 300 /*height*/,
            ThicknessHelper::FromUniformLength(100),
            xaml::HorizontalAlignment::Left,
            xaml::VerticalAlignment::Center);

        RunOnUIThread([&]()
        {
            xaml_automation::AutomationProperties::SetName(flyout, flyoutName);
            xaml_automation::AutomationProperties::SetAutomationId(flyout, flyoutId);

            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);
            VERIFY_IS_NOT_NULL(rootPanel);
            flyout->ShouldConstrainToRootBounds = false;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(target);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            Automation::AutomationClient::UIAElementInfo uiaInfo;
            uiaInfo.m_Name = flyoutPresenterName->Data();
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, target);

            VERIFY_IS_NOT_NULL(automationClientManager);

            wrl::ComPtr<IUIAutomationElement> element;
            automationClientManager->GetCurrentUIAutomationElement(&element);
            VERIFY_IS_NOT_NULL(element);

            AutoBSTR automationId;
            VERIFY_SUCCEEDED(element->get_CurrentAutomationId(automationId.ReleaseAndGetAddressOf()));
            AutoBSTR::VerifyAreEqual(flyoutPresenterId->Data(), automationId);
        });

        FlyoutHelper::HideFlyout(flyout);
    } 

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Flyout
