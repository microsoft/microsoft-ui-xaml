// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include "PopupAutomationIntegrationTests.h"
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <AutomationClient\AutomationClientInitializer.h>
#include "TestCleanupWrapper.h"

#include <ControlHelper.h>
#include <TreeHelper.h>
#include <UIAutomationHelper.h>
#include <FlyoutHelper.h>
#include <PopupHelper.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;

using namespace test_infra;
using namespace WEX::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace Popup {

WCHAR PopupAutomationIntegrationTests::s_automationName[] = L"TestPopupName";
WCHAR PopupAutomationIntegrationTests::s_automationId[] = L"TestPopupId";
WCHAR PopupAutomationIntegrationTests::s_automationPopupButtonName[] = L"TestPopupButtonName";
WCHAR PopupAutomationIntegrationTests::s_automationPopupButtonId[] = L"TestPopupButtonId";

WCHAR PopupAutomationIntegrationTests::s_automationName2[] = L"TestPopupName2";
WCHAR PopupAutomationIntegrationTests::s_automationId2[] = L"TestPopupId2";
WCHAR PopupAutomationIntegrationTests::s_automationPopupButtonName2[] = L"TestPopupButtonName2";
WCHAR PopupAutomationIntegrationTests::s_automationPopupButtonId2[] = L"TestPopupButtonId2";

bool PopupAutomationIntegrationTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool PopupAutomationIntegrationTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool PopupAutomationIntegrationTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

wrl::ComPtr<IRawElementProviderFragment> PopupAutomationIntegrationTests::GetUIAWindow()
{
    const auto& wh = TestServices::WindowHelper;

    Platform::Object^ providerAsObject;
    wh->GetUIAWindow(&providerAsObject);

    wrl::ComPtr<IInspectable> providerAsInspectable(reinterpret_cast<IInspectable*>(providerAsObject));

    wrl::ComPtr<IRawElementProviderFragment> provider;
    providerAsInspectable.As(&provider);

    return provider;
}

void PopupAutomationIntegrationTests::CheckUIAElementName(_In_ wrl::ComPtr<IRawElementProviderFragment> uiaElement, _In_ const WCHAR* expectedName)
{
    wrl::ComPtr<IRawElementProviderSimple> simple;
    uiaElement.As(&simple);

    Common::AutoVariant autoVar;
    simple->GetPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());

    // Convert to AutoBSTR because the failure message is better
    Common::AutoBSTR autoBstr((autoVar.Storage())->bstrVal);

    Common::AutoBSTR::VerifyAreEqual(expectedName, autoBstr);
}

// Test Cases

void PopupAutomationIntegrationTests::VerifyParentlessPopupAutomationTree_NoFrameAP()
{
    VerifyParentlessPopupAutomationTreeCommon(false /* hasFrameAP */);
}

void PopupAutomationIntegrationTests::VerifyParentlessPopupAutomationTree_FrameAP()
{
    VerifyParentlessPopupAutomationTreeCommon(true /* hasFrameAP */);
}

void PopupAutomationIntegrationTests::VerifyParentlessPopupAutomationTreeCommon(bool hasFrameAP)
{
    const auto& wh = TestServices::WindowHelper;

    xaml_primitives::Popup^ popup = nullptr;
    xaml_primitives::Popup^ popup2 = nullptr;

    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting up tree");

        xaml_shapes::Rectangle^ button = ref new xaml_shapes::Rectangle();
        xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(s_automationPopupButtonName));

        popup = ref new xaml_primitives::Popup();
        popup->Child = button;
        xaml_automation::AutomationProperties::SetName(popup, ref new Platform::String(s_automationName));

        xaml_shapes::Rectangle^ button2 = ref new xaml_shapes::Rectangle();
        xaml_automation::AutomationProperties::SetName(button2, ref new Platform::String(s_automationPopupButtonName2));

        popup2 = ref new xaml_primitives::Popup();
        popup2->Child = button2;
        xaml_automation::AutomationProperties::SetName(popup2, ref new Platform::String(s_automationName2));

        Page^ page = ref new Page();
        xaml_automation::AutomationProperties::SetName(page, ref new Platform::String(L"RootPageName"));

        Frame^ frame = ref new Frame();
        frame->Content = page;
        if (hasFrameAP)
        {
            xaml_automation::AutomationProperties::SetName(frame, ref new Platform::String(L"RootFrameName"));
        }

        wh->WindowContent = frame;

        LOG_OUTPUT(L"> Opening popups");
        popup->XamlRoot = frame->XamlRoot;
        popup2->XamlRoot = frame->XamlRoot;
        popup->IsOpen = true;
        popup2->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wrl::ComPtr<IRawElementProviderFragment> window = GetUIAWindow();
        wrl::ComPtr<IRawElementProviderFragment> uiaPopup1;
        wrl::ComPtr<IRawElementProviderFragment> uiaPopup2;
        wrl::ComPtr<IRawElementProviderFragment> uiaPage;
        wrl::ComPtr<IRawElementProviderFragment> uiaNull;
        wrl::ComPtr<IRawElementProviderFragment> uiaParent;
        wrl::ComPtr<IRawElementProviderFragment> uiaButton1;
        wrl::ComPtr<IRawElementProviderFragment> uiaButton2;

        LOG_OUTPUT(L"  > Window's first child = Popup1");
        LogThrow_IfFailed(window->Navigate(NavigateDirection_FirstChild, &uiaPopup1));
        CheckUIAElementName(uiaPopup1, s_automationName);

        LOG_OUTPUT(L"  > Popup1's parent = window");
        LogThrow_IfFailed(uiaPopup1->Navigate(NavigateDirection_Parent, &uiaParent));
        VERIFY_ARE_EQUAL(window, uiaParent);

        LOG_OUTPUT(L"  > Popup1's next sibling = Popup2");
        LogThrow_IfFailed(uiaPopup1->Navigate(NavigateDirection_NextSibling, &uiaPopup2));
        CheckUIAElementName(uiaPopup2, s_automationName2);

        LOG_OUTPUT(L"  > Popup2's parent = window");
        LogThrow_IfFailed(uiaPopup2->Navigate(NavigateDirection_Parent, &uiaParent));
        VERIFY_ARE_EQUAL(window, uiaParent);

        if (hasFrameAP)
        {
            wrl::ComPtr<IRawElementProviderFragment> uiaFrame;

            LOG_OUTPUT(L"  > Popup2's next sibling = Frame");
            LogThrow_IfFailed(uiaPopup2->Navigate(NavigateDirection_NextSibling, &uiaFrame));
            CheckUIAElementName(uiaFrame, L"RootFrameName");

            LOG_OUTPUT(L"  > Frame's parent = window");
            LogThrow_IfFailed(uiaFrame->Navigate(NavigateDirection_Parent, &uiaParent));
            VERIFY_ARE_EQUAL(window, uiaParent);

            LOG_OUTPUT(L"  > Frame's next sibling = null");
            LogThrow_IfFailed(uiaFrame->Navigate(NavigateDirection_NextSibling, &uiaNull));
            VERIFY_IS_NULL(uiaNull);

            LOG_OUTPUT(L"  > Frame's first child = Page");
            LogThrow_IfFailed(uiaFrame->Navigate(NavigateDirection_FirstChild, &uiaPage));
            CheckUIAElementName(uiaPage, L"RootPageName");

            LOG_OUTPUT(L"  > Page's parent = Frame");
            LogThrow_IfFailed(uiaPage->Navigate(NavigateDirection_Parent, &uiaParent));
            VERIFY_ARE_EQUAL(uiaFrame, uiaParent);

            LOG_OUTPUT(L"  > Page's next sibling = null");
            LogThrow_IfFailed(uiaPage->Navigate(NavigateDirection_NextSibling, &uiaNull));
            VERIFY_IS_NULL(uiaNull);
        }
        else
        {
            LOG_OUTPUT(L"  > Popup2's next sibling = Page");
            LogThrow_IfFailed(uiaPopup2->Navigate(NavigateDirection_NextSibling, &uiaPage));
            CheckUIAElementName(uiaPage, L"RootPageName");

            LOG_OUTPUT(L"  > Page's parent = window");
            LogThrow_IfFailed(uiaPage->Navigate(NavigateDirection_Parent, &uiaParent));
            VERIFY_ARE_EQUAL(window, uiaParent);

            LOG_OUTPUT(L"  > Page's next sibling = null");
            LogThrow_IfFailed(uiaPage->Navigate(NavigateDirection_NextSibling, &uiaNull));
            VERIFY_IS_NULL(uiaNull);
        }

        LOG_OUTPUT(L"  > Popup1's first child = Button1");
        LogThrow_IfFailed(uiaPopup1->Navigate(NavigateDirection_FirstChild, &uiaButton1));
        CheckUIAElementName(uiaButton1, s_automationPopupButtonName);

        LOG_OUTPUT(L"  > Button1's parent = Popup1");
        LogThrow_IfFailed(uiaButton1->Navigate(NavigateDirection_Parent, &uiaPopup1));
        CheckUIAElementName(uiaPopup1, s_automationName);

        LOG_OUTPUT(L"  > Popup2's first child = Button2");
        LogThrow_IfFailed(uiaPopup2->Navigate(NavigateDirection_FirstChild, &uiaButton2));
        CheckUIAElementName(uiaButton2, s_automationPopupButtonName2);

        LOG_OUTPUT(L"  > Button2's parent = Popup2");
        LogThrow_IfFailed(uiaButton2->Navigate(NavigateDirection_Parent, &uiaPopup2));
        CheckUIAElementName(uiaPopup2, s_automationName2);
    });
    wh->WaitForIdle();
}

void PopupAutomationIntegrationTests::VerifyOpenPopupPreservesMainTree()
{
    const auto& wh = TestServices::WindowHelper;

    xaml_primitives::Popup^ popup = nullptr;

    TestCleanupWrapper cleanup;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting up tree");

        xaml_shapes::Rectangle^ button = ref new xaml_shapes::Rectangle();
        xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(s_automationPopupButtonName));

        popup = ref new xaml_primitives::Popup();
        popup->Child = button;
        xaml_automation::AutomationProperties::SetName(popup, ref new Platform::String(s_automationName));

        xaml_shapes::Rectangle^ button2 = ref new xaml_shapes::Rectangle();
        xaml_automation::AutomationProperties::SetName(button2, ref new Platform::String(s_automationPopupButtonName2));

        Button^ buttonOnPage = ref new Button();
        xaml_automation::AutomationProperties::SetName(buttonOnPage, ref new Platform::String(L"ButtonOnPageName"));

        Page^ page = ref new Page();
        xaml_automation::AutomationProperties::SetName(page, ref new Platform::String(L"RootPageName"));
        page->Content = buttonOnPage;

        Frame^ frame = ref new Frame();
        frame->Content = page;

        wh->WindowContent = frame;

        LOG_OUTPUT(L"> Opening popup");
        popup->XamlRoot = frame->XamlRoot;
        popup->IsOpen = true;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        wrl::ComPtr<IRawElementProviderFragment> window = GetUIAWindow();
        wrl::ComPtr<IRawElementProviderFragment> uiaPopup1;
        wrl::ComPtr<IRawElementProviderFragment> uiaPage;
        wrl::ComPtr<IRawElementProviderFragment> uiaNull;
        wrl::ComPtr<IRawElementProviderFragment> uiaParent;
        wrl::ComPtr<IRawElementProviderFragment> uiaButton1;
        wrl::ComPtr<IRawElementProviderFragment> uiaButtonOnPage;

        LOG_OUTPUT(L"  > Window's first child = Popup1");
        LogThrow_IfFailed(window->Navigate(NavigateDirection_FirstChild, &uiaPopup1));
        CheckUIAElementName(uiaPopup1, s_automationName);

        LOG_OUTPUT(L"  > Popup1's next sibling = Page");
        LogThrow_IfFailed(uiaPopup1->Navigate(NavigateDirection_NextSibling, &uiaPage));
        CheckUIAElementName(uiaPage, L"RootPageName");

        LOG_OUTPUT(L"  > Page's first child = ButtonOnPage");
        LogThrow_IfFailed(uiaPage->Navigate(NavigateDirection_FirstChild, &uiaButtonOnPage));
        CheckUIAElementName(uiaButtonOnPage, L"ButtonOnPageName");
    });
    wh->WaitForIdle();
}


void PopupAutomationIntegrationTests::VerifyAutomationProperties()
{
    TestCleanupWrapper cleanup;
    xaml_primitives::Popup^ popup = nullptr;

    std::shared_ptr<Event> loadedEvent = std::make_shared<Event>();
    auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

    RunOnUIThread([&]()
    {
        popup = ref new xaml_primitives::Popup();
        popup->Child = ref new xaml_controls::Button();

        xaml_automation::AutomationProperties::SetName(popup, ref new Platform::String(s_automationName));
        xaml_automation::AutomationProperties::SetAutomationId(popup, ref new Platform::String(s_automationId));

        auto rootPanel = ref new xaml_controls::StackPanel();
        loadedRegistration.Attach(rootPanel, [&]() { loadedEvent->Set(); });

        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    loadedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        popup->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
    });

    PopupHelper::OpenPopup(popup);

    Automation::AutomationClient::UIAElementInfo uiaInfo;
    uiaInfo.m_Name = s_automationName;
    uiaInfo.m_AutomationID = s_automationId;

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        wrl::ComPtr<IUIAutomation> spUIAutomation;
        wrl::ComPtr<IUIAutomationElement> spUIAutomationPopup;

        AutoBSTR propertyValue;

        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoPopup(AutomationClient::AutomationClientManager::GetHwndFromWindowedPopup(popup), uiaInfo);

        spAutomationClientManager->GetAutomation(&spUIAutomation);
        spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationPopup);

        LOG_OUTPUT(L"Verifying that UIA client-side node for Popup exists.");
        VERIFY_IS_NOT_NULL(spUIAutomationPopup);

        LOG_OUTPUT(L"Verifying the UIA name property from client-side node for Popup.");
        spUIAutomationPopup->get_CurrentName(propertyValue.ReleaseAndGetAddressOf());
        LOG_OUTPUT(L"Name = '%s', Expected = '%s'.", propertyValue.Get(), s_automationName);
        VERIFY_IS_TRUE(!wcscmp(s_automationName, propertyValue));

        LOG_OUTPUT(L"Verifying the UIA class name property from client-side node for Popup.");
        spUIAutomationPopup->get_CurrentClassName(propertyValue.ReleaseAndGetAddressOf());
        LOG_OUTPUT(L"ClassName = '%s', Expected = '%s'.", propertyValue.Get(), L"Popup");
        VERIFY_IS_TRUE(!wcscmp(L"Popup", propertyValue));
    });
}

void PopupAutomationIntegrationTests::VerifyBarePopupDoesNotImplementWindowPattern()
{
    TestCleanupWrapper cleanup;
    xaml_primitives::Popup^ popup = nullptr;

    std::shared_ptr<Event> loadedEvent = std::make_shared<Event>();
    auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

    RunOnUIThread([&]()
    {
        popup = ref new xaml_primitives::Popup();
        popup->Child = ref new xaml_controls::Button();

        xaml_automation::AutomationProperties::SetName(popup, ref new Platform::String(s_automationName));
        xaml_automation::AutomationProperties::SetAutomationId(popup, ref new Platform::String(s_automationId));

        auto rootPanel = ref new xaml_controls::StackPanel();
        loadedRegistration.Attach(rootPanel, [&]() { loadedEvent->Set(); });

        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    loadedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        popup->XamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;
    });

    PopupHelper::OpenPopup(popup);

    Automation::AutomationClient::UIAElementInfo uiaInfo;
    uiaInfo.m_Name = s_automationName;
    uiaInfo.m_AutomationID = s_automationId;

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        wrl::ComPtr<IUIAutomation> spUIAutomation;
        wrl::ComPtr<IUIAutomationElement> spUIAutomationPopup;
        wrl::ComPtr<IUIAutomationWindowPattern> spUIAutomationPopupAsWindowPattern;

        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfoPopup(AutomationClient::AutomationClientManager::GetHwndFromWindowedPopup(popup), uiaInfo);

        spAutomationClientManager->GetAutomation(&spUIAutomation);
        spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationPopup);

        LOG_OUTPUT(L"Verifying that a loose Popup does not report that it supports the Window pattern.");
        spUIAutomationPopup->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spUIAutomationPopupAsWindowPattern);
        VERIFY_IS_NULL(spUIAutomationPopupAsWindowPattern);
    });
}

void PopupAutomationIntegrationTests::VerifyFlyoutPopupImplementsWindowPattern()
{
    TestCleanupWrapper cleanup;
    auto flyout = FlyoutHelper::CreateDefaultFlyout(xaml_primitives::FlyoutPlacementMode::Top);

    auto flyoutTarget = FlyoutHelper::CreateTarget(
        300 /*width*/, 300 /*height*/,
        ThicknessHelper::FromUniformLength(100),
        xaml::HorizontalAlignment::Center,
        xaml::VerticalAlignment::Center);

    std::shared_ptr<Event> targetLoadedEvent = std::make_shared<Event>();
    auto targetLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Border, Loaded);

    RunOnUIThread([&]()
    {
        targetLoadedRegistration.Attach(flyoutTarget, [&](){ targetLoadedEvent->Set(); });

        TestServices::WindowHelper->WindowContent = flyoutTarget;
    });

    targetLoadedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    FlyoutHelper::OpenFlyout(flyout, flyoutTarget, FlyoutOpenMethod::Programmatic_ShowAt);

    RunOnUIThread([&]()
    {
        auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(flyoutTarget->XamlRoot);
        auto flyoutPopup = popups->GetAt(0);

        xaml_automation::AutomationProperties::SetName(flyoutPopup, ref new Platform::String(s_automationName));
        xaml_automation::AutomationProperties::SetAutomationId(flyoutPopup, ref new Platform::String(s_automationId));
    });

    TestServices::WindowHelper->WaitForIdle();

    Automation::AutomationClient::UIAElementInfo uiaInfo;
    uiaInfo.m_Name = s_automationName;
    uiaInfo.m_AutomationID = s_automationId;

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        wrl::ComPtr<IUIAutomation> spUIAutomation;
        wrl::ComPtr<IUIAutomationElement> spUIAutomationPopup;
        wrl::ComPtr<IUIAutomationWindowPattern> spUIAutomationPopupAsWindowPattern;

        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, flyoutTarget);

        spAutomationClientManager->GetAutomation(&spUIAutomation);
        spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationPopup);

        LOG_OUTPUT(L"Verifying that the Popup for a Flyout reports that it supports the Window pattern.");
        spUIAutomationPopup->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spUIAutomationPopupAsWindowPattern);
        VERIFY_IS_NOT_NULL(spUIAutomationPopupAsWindowPattern);
    });

    FlyoutHelper::HideFlyout(flyout);
}

void PopupAutomationIntegrationTests::VerifyMenuFlyoutPopupImplementsWindowPattern()
{
    TestCleanupWrapper cleanup;
    xaml_controls::MenuFlyout^ menuFlyout = nullptr;
    xaml_controls::MenuFlyoutSubItem^ menuFlyoutSubItem = nullptr;

    auto flyoutTarget = FlyoutHelper::CreateTarget(
        300 /*width*/, 300 /*height*/,
        ThicknessHelper::FromUniformLength(100),
        xaml::HorizontalAlignment::Center,
        xaml::VerticalAlignment::Center);

    std::shared_ptr<Event> targetLoadedEvent = std::make_shared<Event>();
    std::shared_ptr<Event> flyoutPresenterLostFocusEvent = std::make_shared<Event>();
    auto targetLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Border, Loaded);
    auto flyoutPresenterRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutPresenter, LostFocus);

    RunOnUIThread([&]()
    {
        menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(xaml_markup::XamlReader::Load(
            L"<MenuFlyout x:Name='menuFlyout' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
            L"    <MenuFlyoutSubItem x:Name='subItem1' Text='Menu sub item'>"
            L"        <MenuFlyoutItem>Menu item</MenuFlyoutItem>"
            L"    </MenuFlyoutSubItem>"
            L"</MenuFlyout>"));

        menuFlyoutSubItem = dynamic_cast<xaml_controls::MenuFlyoutSubItem^>(menuFlyout->Items->GetAt(0));

        targetLoadedRegistration.Attach(flyoutTarget, [&](){ targetLoadedEvent->Set(); });

        TestServices::WindowHelper->WindowContent = flyoutTarget;
    });

    targetLoadedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    FlyoutHelper::OpenFlyout(menuFlyout, flyoutTarget, FlyoutOpenMethod::Programmatic_ShowAt);

    if (PopupHelper::AreWindowedPopupsEnabled())
    {
        // Temporary fix for UiaEndpoint synchronization
        AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(flyoutTarget);
    }

    RunOnUIThread([&]()
    {
        wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(flyoutTarget->XamlRoot);

        auto popup = popups->GetAt(0);
        auto menuFlyoutPresenter = dynamic_cast<xaml_controls::MenuFlyoutPresenter^>(popup->Child);

        flyoutPresenterRegistration.Attach(menuFlyoutPresenter, [&](){ flyoutPresenterLostFocusEvent->Set(); });
    });

    TestServices::InputHelper->Tap(menuFlyoutSubItem);

    flyoutPresenterLostFocusEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    WCHAR menuFlyoutPopupAutomationName[] = L"MenuFlyoutPopupName";
    WCHAR menuFlyoutPopupAutomationId[] = L"MenuFlyoutPopupId";
    WCHAR menuFlyoutSubItemPopupAutomationName[] = L"SubItemPopupName";
    WCHAR menuFlyoutSubItemPopupAutomationId[] = L"SubItemPopupId";

    RunOnUIThread([&]()
    {
        auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(flyoutTarget->XamlRoot);
        auto flyoutPopup = popups->GetAt(0);
        auto flyoutSubItemPopup = popups->GetAt(1);

        xaml_automation::AutomationProperties::SetName(flyoutPopup, ref new Platform::String(menuFlyoutPopupAutomationName));
        xaml_automation::AutomationProperties::SetAutomationId(flyoutPopup, ref new Platform::String(menuFlyoutPopupAutomationId));
        xaml_automation::AutomationProperties::SetName(flyoutSubItemPopup, ref new Platform::String(menuFlyoutSubItemPopupAutomationName));
        xaml_automation::AutomationProperties::SetAutomationId(flyoutSubItemPopup, ref new Platform::String(menuFlyoutSubItemPopupAutomationId));
    });

    TestServices::WindowHelper->WaitForIdle();

    Automation::AutomationClient::UIAElementInfo uiaInfo;
    uiaInfo.m_Name = menuFlyoutPopupAutomationName;
    uiaInfo.m_AutomationID = menuFlyoutPopupAutomationId;

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        wrl::ComPtr<IUIAutomation> spUIAutomation;
        wrl::ComPtr<IUIAutomationElement> spUIAutomationPopup;
        wrl::ComPtr<IUIAutomationWindowPattern> spUIAutomationPopupAsWindowPattern;

        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, flyoutTarget, 0);

        spAutomationClientManager->GetAutomation(&spUIAutomation);
        spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationPopup);

        LOG_OUTPUT(L"Verifying that the Popup for a MenuFlyout reports that it supports the Window pattern.");
        spUIAutomationPopup->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spUIAutomationPopupAsWindowPattern);
        VERIFY_IS_NOT_NULL(spUIAutomationPopupAsWindowPattern);
    });

    uiaInfo.m_Name = menuFlyoutSubItemPopupAutomationName;
    uiaInfo.m_AutomationID = menuFlyoutSubItemPopupAutomationId;

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        wrl::ComPtr<IUIAutomation> spUIAutomation;
        wrl::ComPtr<IUIAutomationElement> spUIAutomationPopup;
        wrl::ComPtr<IUIAutomationWindowPattern> spUIAutomationPopupAsWindowPattern;

        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, flyoutTarget, 1);

        spAutomationClientManager->GetAutomation(&spUIAutomation);
        spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationPopup);

        LOG_OUTPUT(L"Verifying that the Popup for a MenuFlyoutSubItem reports that it supports the Window pattern.");
        spUIAutomationPopup->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spUIAutomationPopupAsWindowPattern);
        VERIFY_IS_NOT_NULL(spUIAutomationPopupAsWindowPattern);
    });

    FlyoutHelper::HideFlyout(menuFlyout);
}

void PopupAutomationIntegrationTests::VerifyContentDialogPopupImplementsWindowPattern()
{
    // Leak: TemplateContent peer not being unpegged
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

    TestCleanupWrapper cleanup;
    xaml_controls::ContentDialog^ contentDialog = nullptr;

    auto openedEvent = std::make_shared<Event>();
    auto loadedEvent = std::make_shared<Event>();
    auto closedEvent = std::make_shared<Event>();
    auto unloadedEvent = std::make_shared<Event>();
    auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Opened);
    auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Loaded);
    auto closedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Closed);
    auto unloadedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Unloaded);

    RunOnUIThread([&] ()
    {
        contentDialog = ref new xaml_controls::ContentDialog();
        contentDialog->Title = L"ContentDialog Title";
        contentDialog->PrimaryButtonText = L"OK";
        contentDialog->SecondaryButtonText = L"Cancel";

        loadedRegistration.Attach(contentDialog, [&]() { loadedEvent->Set(); });
        openedRegistration.Attach(contentDialog, [&]() { openedEvent->Set(); });
        closedRegistration.Attach(contentDialog, [&]() { closedEvent->Set(); });
        unloadedRegistration.Attach(contentDialog, [&]() { unloadedEvent->Set(); });

        TestServices::WindowHelper->WindowContent = ref new xaml_controls::StackPanel();
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto xamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;

        if (xamlRoot)
        {
            contentDialog->XamlRoot = xamlRoot;
        }
    });

    RunOnUIThread([&] ()
    {
        contentDialog->ShowAsync();
    });

    openedEvent->WaitForDefault();
    loadedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(contentDialog->XamlRoot);
        auto contentDialogPopup = popups->GetAt(0);

        xaml_automation::AutomationProperties::SetName(contentDialogPopup, ref new Platform::String(s_automationName));
        xaml_automation::AutomationProperties::SetAutomationId(contentDialogPopup, ref new Platform::String(s_automationId));
    });

    TestServices::WindowHelper->WaitForIdle();

    Automation::AutomationClient::UIAElementInfo uiaInfo;
    uiaInfo.m_Name = s_automationName;
    uiaInfo.m_AutomationID = s_automationId;

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        wrl::ComPtr<IUIAutomation> spUIAutomation;
        wrl::ComPtr<IUIAutomationElement> spUIAutomationPopup;
        wrl::ComPtr<IUIAutomationWindowPattern> spUIAutomationPopupAsWindowPattern;

        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

        spAutomationClientManager->GetAutomation(&spUIAutomation);
        spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationPopup);

        LOG_OUTPUT(L"Verifying that the Popup for a ContentDialog reports that it supports the Window pattern.");
        spUIAutomationPopup->GetCurrentPatternAs(UIA_WindowPatternId, __uuidof(IUIAutomationWindowPattern), &spUIAutomationPopupAsWindowPattern);
        VERIFY_IS_NOT_NULL(spUIAutomationPopupAsWindowPattern);
    });

    RunOnUIThread([&]()
    {
        contentDialog->Hide();
    });

    closedEvent->WaitForDefault();
    unloadedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();
}

void PopupAutomationIntegrationTests::VerifyPopupButtonAutomationPropertyChangedEvent()
{
    // Leak: TemplateContent peer not being unpegged
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

    TestCleanupWrapper cleanup([]()
    {
        RunOnUIThread([&]()
        {
            TestServices::Utilities->ClearMockUIAClientsListening();
        });

        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    xaml_controls::Button^ button1 = nullptr;
    xaml_controls::TextBlock^ textblock1 = nullptr;
    xaml_automation_peers::TextBlockAutomationPeer^ textblock1AP = nullptr;

    RunOnUIThread([&]()
    {
        auto root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                      Background="{ThemeResource SystemControlBackgroundAltHighBrush}" >
                    <Popup x:Name="popupName">
                        <Button x:Name="popupButtonName" AutomationProperties.Name="{Binding Tag, RelativeSource={RelativeSource Self}}" Click="ButtonName_Click" Tag="Test Popup ButtonName Tag">
                            <StackPanel>
                                <TextBlock x:Name="TextBlock1" Text="{Binding Tag, ElementName=popupButtonName}" />
                            </StackPanel>
                        </Button>
                    </Popup>
                    <Button x:Name="buttonName2" AutomationProperties.Name="{Binding Tag, RelativeSource={RelativeSource Self}}" Click="ButtonName2_Click" Tag="Test Button2">
                        <StackPanel>
                            <TextBlock Text="{Binding Tag, ElementName=buttonName2}" />
                        </StackPanel>
                    </Button>
                </Grid>)"));

        // Create AP for TextBlock to work around AutomationClientManager (Xaml test component) does not ensure AutomationPeer for objects being tested in some cases
        textblock1 = safe_cast<xaml_controls::TextBlock^>(root->FindName(L"TextBlock1"));
        textblock1AP = safe_cast<xaml_automation_peers::TextBlockAutomationPeer^>(
            xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(textblock1));

        button1 = safe_cast<xaml_controls::Button^>(root->FindName(L"popupButtonName"));
        TestServices::WindowHelper->WindowContent = root;

        // We need to open the popup after WindowContent is set.  If we set IsOpen="true" in the Popup's Xaml, the Popup is opened
        // during parse time, but the island doesn't exist yet so Xaml doesn't know what PopupRoot to use.
        dynamic_cast<xaml_primitives::Popup^>(root->FindName(L"popupName"))->IsOpen = true;

        TestServices::Utilities->SetMockUIAClientsListening();
    });

    TestServices::WindowHelper->WaitForIdle();


    auto spClickEvent = std::make_shared<Event>();
    auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

    RunOnUIThread([&]()
    {
        xaml_automation::AutomationProperties::SetName(button1, ref new Platform::String(s_automationPopupButtonName));
        xaml_automation::AutomationProperties::SetAutomationId(button1, ref new Platform::String(s_automationPopupButtonId));

        clickRegistration.Attach(button1, ref new xaml::RoutedEventHandler([button1, spClickEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
        {
            LOG_OUTPUT(L"Popup Button Click event fired!");
            button1->Tag = button1->Tag + " Clicked!";
            spClickEvent->Set();
        }));
    });
    TestServices::WindowHelper->WaitForIdle();

    wrl::ComPtr<AutomationClient::AutomationPropertyChangedHandler<1>> spAutomationPropertyChangedEventHandler;
    Automation::AutomationClient::UIAElementInfo uiaInfo;

    uiaInfo.m_Name = s_automationPopupButtonName;
    uiaInfo.m_AutomationID = s_automationPopupButtonId;

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        auto commonUIAPropertyChangedEvent = std::make_shared<Event>();
        const std::array<PROPERTYID, 1> saPropertyIds = { UIA_NamePropertyId };
        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
        spAutomationPropertyChangedEventHandler.Attach(new AutomationClient::AutomationPropertyChangedHandler<1>(spAutomationClientManager, commonUIAPropertyChangedEvent, TreeScope_Subtree, saPropertyIds));
        spAutomationPropertyChangedEventHandler->AttachEventHandler();
    });

    RunOnUIThread([&]()
    {
        auto button1AP = safe_cast<xaml_automation_peers::ButtonAutomationPeer^>(
            xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(button1));
        LOG_OUTPUT(L"Click the popup button using UIA and wait for the button event!");
        button1AP->Invoke();
    });
    spClickEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto button1AP = safe_cast<xaml_automation_peers::ButtonAutomationPeer^>(
            xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(button1));
        LOG_OUTPUT(L"Click the popup button using UIA and wait for Automation PropertyChanged event!");
        button1AP->Invoke();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        spAutomationPropertyChangedEventHandler->ConfirmAndUnregister();
    });
}

} } } } } } } // Microsoft::UI::Xaml::Tests::Controls::Primitives::LoopingSelector
