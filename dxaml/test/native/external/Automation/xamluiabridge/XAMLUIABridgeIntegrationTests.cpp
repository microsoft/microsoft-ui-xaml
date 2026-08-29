// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <XamlUIABridgeIntegrationTests.h>
#include <XamlUIABridgeHostCanvas.h>
#include <SafeEventRegistration.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <Patterns\InvokePatternHandler.h>
#include "CustomTypes.XamlTypeInfo.g.h"
#include <TreeHelper.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;
using namespace Tests::Native::External::Automation::XamlUIABridge;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace XamlUIABridge {

    bool XamlUIABridgeIntegrationTests::ClassSetup()
    {
        // It's very important to call EnsureInitialized on TestServices
        // from ClassSetup. This method will wait for the window to be
        // activated on launch, which avoids a race condition that will block
        // input from being routed to the app. It will also wait for the
        // debugger to attach when the waitForDebugger runtime parameter is
        // specified.
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool XamlUIABridgeIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());
        return true;
    }

    bool XamlUIABridgeIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void XamlUIABridgeIntegrationTests::SetupXAMLUIABridge()
    {
        XamlUIABridgeHostCanvas^ xamlUIABridgeHostCanvas;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Canvas, Loaded);

        RunOnUIThread([&]()
        {
            xamlUIABridgeHostCanvas = ref new XamlUIABridgeHostCanvas();
            xamlUIABridgeHostCanvas->Width = 800;
            xamlUIABridgeHostCanvas->Height = 800;
            xamlUIABridgeHostCanvas->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
            xamlUIABridgeHostCanvas->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            xamlUIABridgeHostCanvas->VerticalAlignment = xaml::VerticalAlignment::Top;
            xaml_automation::AutomationProperties::SetName(xamlUIABridgeHostCanvas, ref new Platform::String(L"Hosting Canvas"));
            TestServices::WindowHelper->WindowContent = xamlUIABridgeHostCanvas;

            loadedRegistration.Attach(xamlUIABridgeHostCanvas, [loadedEvent]()
            {
                loadedEvent->Set();
            });
        });

        LOG_OUTPUT(L"Waiting for XamlUIABridgeHostCanvas to be loaded...");
        loadedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // This calls ensure that AP for the Canvas is the FocusedElement, otherwise XAML Focus Manager doesn't consider Canvas as Focusable.
            xaml_automation_peers::AutomationPeer^ ap = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(xamlUIABridgeHostCanvas);
            ap->SetFocus();
        });
    }

    //
    // Test Cases
    //
    void XamlUIABridgeIntegrationTests::VerifyUIAHitTesting()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AP Node A";
        uiaInfo.m_AutomationID = L"AP Node A";
        uiaInfo.m_cType = UIA_CustomControlTypeId;
        SetupXAMLUIABridge();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            POINT clickablePoint;
            BOOL gotClickable;
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spAutomationElement;
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            spUIAutomationElement->GetClickablePoint(&clickablePoint, &gotClickable);

            spAutomationClientManager->GetAutomation(&spAutomation);
            spAutomation->ElementFromPoint(clickablePoint, &spAutomationElement);
            VERIFY_IS_NOT_NULL(spAutomationElement);

            Common::AutoVariant autoVar;
            spAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"AP Node A", (autoVar.Storage())->bstrVal));

            spAutomationElement->GetCurrentPropertyValue(UIA_LocalizedControlTypePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"custom", (autoVar.Storage())->bstrVal));
        });
    }

    void XamlUIABridgeIntegrationTests::VerifyFocusedElement()
    {
        TestCleanupWrapper cleanup;
        SetupXAMLUIABridge();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationElement> spAutomationElement;

            auto spAutomationClientManager = std::make_shared<AutomationClient::AutomationClientManager>();
            spAutomationClientManager->GetAutomation(&spAutomation);

            spAutomation->GetFocusedElement(&spAutomationElement);
            VERIFY_IS_NOT_NULL(spAutomationElement);

            Common::AutoVariant autoVar;
            spAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.Storage());
            VERIFY_IS_TRUE(!wcscmp(L"Native Node 3", (autoVar.Storage())->bstrVal));
        });
    }

    void XamlUIABridgeIntegrationTests::VerifyNavigation()
    {
        TestCleanupWrapper cleanup;
        SetupXAMLUIABridge();
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AP Node A";
        uiaInfo.m_AutomationID = L"AP Node A";
        uiaInfo.m_cType = UIA_CustomControlTypeId;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spAutomationFoundElement;
            POINT clickablePoint;
            BOOL gotClickable;
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            spUIAutomationElement->GetClickablePoint(&clickablePoint, &gotClickable);

            spAutomationClientManager->GetAutomation(&spAutomation);
            spAutomation->ElementFromPoint(clickablePoint, &spAutomationElement);
            VERIFY_IS_NOT_NULL(spAutomationElement);
            WEX::Common::Throw::IfNull(spAutomationElement.Get());

            Common::AutoVariant autoVar;
            spAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"AP Node A", (autoVar.Storage())->bstrVal));

            wrl::ComPtr<IUIAutomationElement> spAutomationParent;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationNameCondition;
            Common::AutoVariant autoVarName;
            spAutomationParent.Attach(spAutomationClientManager->GetParent(spAutomationElement.Get()));
            VERIFY_IS_NOT_NULL(spAutomationParent);

            autoVarName.SetString(L"Native Node 3");
            LogThrow_IfFailed(spAutomation->CreatePropertyCondition(UIA_NamePropertyId, *(autoVarName.Storage()), &spUIAutomationNameCondition));
            spAutomationParent->FindFirst(TreeScope::TreeScope_Descendants, spUIAutomationNameCondition.Get(), &spAutomationFoundElement);
            VERIFY_IS_NOT_NULL(spAutomationFoundElement);
            spAutomationElement = spAutomationFoundElement;
            spAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"Native Node 3", (autoVar.Storage())->bstrVal));

            spAutomationElement->FindFirst(TreeScope::TreeScope_Children, nullptr, &spAutomationFoundElement);
            VERIFY_IS_NULL(spAutomationFoundElement);

            autoVarName.SetString(L"AP Node D");
            LogThrow_IfFailed(spAutomation->CreatePropertyCondition(UIA_NamePropertyId, *(autoVarName.Storage()), &spUIAutomationNameCondition));
            spAutomationParent->FindFirst(TreeScope::TreeScope_Descendants, spUIAutomationNameCondition.Get(), &spAutomationFoundElement);
            VERIFY_IS_NOT_NULL(spAutomationFoundElement);

            spAutomationParent.Attach(spAutomationClientManager->GetParent(spAutomationFoundElement.Get()));
            VERIFY_IS_NOT_NULL(spAutomationParent);
            spAutomationParent->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"AP Node A", (autoVar.Storage())->bstrVal));

            autoVarName.SetString(L"Native Node 5");
            LogThrow_IfFailed(spAutomation->CreatePropertyCondition(UIA_NamePropertyId, *(autoVarName.Storage()), &spUIAutomationNameCondition));
            spAutomationParent->FindFirst(TreeScope::TreeScope_Children, spUIAutomationNameCondition.Get(), &spAutomationFoundElement);
            VERIFY_IS_NOT_NULL(spAutomationFoundElement);
        });
    }

    void XamlUIABridgeIntegrationTests::VerifyCustomNavigation()
    {
        TestCleanupWrapper cleanup;
        SetupXAMLUIABridge();
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AP Node A";
        uiaInfo.m_AutomationID = L"AP Node A";
        uiaInfo.m_cType = UIA_CustomControlTypeId;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spAutomationFoundElement;
            wrl::ComPtr<IUIAutomationCustomNavigationPattern> spCustomNavigationPattern;
            POINT clickablePoint;
            BOOL gotClickable;
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            spUIAutomationElement->GetClickablePoint(&clickablePoint, &gotClickable);

            spAutomationClientManager->GetAutomation(&spAutomation);
            spAutomation->ElementFromPoint(clickablePoint, &spAutomationFoundElement);
            VERIFY_IS_NOT_NULL(spAutomationFoundElement);
            WEX::Common::Throw::IfNull(spAutomationFoundElement.Get());

            Common::AutoVariant autoVar;
            spAutomationFoundElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"AP Node A", (autoVar.Storage())->bstrVal));

            spAutomationFoundElement->GetCurrentPatternAs(UIA_CustomNavigationPatternId, __uuidof(IUIAutomationCustomNavigationPattern), &spCustomNavigationPattern);
            VERIFY_IS_NOT_NULL(spCustomNavigationPattern);
            LogThrow_IfFailed(spCustomNavigationPattern->Navigate(NavigateDirection_Parent, &spAutomationFoundElement));
            VERIFY_IS_NOT_NULL(spAutomationFoundElement);
            spAutomationFoundElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"Root Native", (autoVar.Storage())->bstrVal));

            spAutomationFoundElement->GetCurrentPatternAs(UIA_CustomNavigationPatternId, __uuidof(IUIAutomationCustomNavigationPattern), &spCustomNavigationPattern);
            VERIFY_IS_NOT_NULL(spCustomNavigationPattern);
            LogThrow_IfFailed(spCustomNavigationPattern->Navigate(NavigateDirection_FirstChild, &spAutomationFoundElement));
            VERIFY_IS_NOT_NULL(spAutomationFoundElement);
            spAutomationFoundElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"Native Node 2", (autoVar.Storage())->bstrVal));

            spAutomationFoundElement->GetCurrentPatternAs(UIA_CustomNavigationPatternId, __uuidof(IUIAutomationCustomNavigationPattern), &spCustomNavigationPattern);
            VERIFY_IS_NOT_NULL(spCustomNavigationPattern);
            LogThrow_IfFailed(spCustomNavigationPattern->Navigate(NavigateDirection_NextSibling, &spAutomationFoundElement));
            VERIFY_IS_NOT_NULL(spAutomationFoundElement);
            spAutomationFoundElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"AP Node A", (autoVar.Storage())->bstrVal));

            spAutomationFoundElement->GetCurrentPatternAs(UIA_CustomNavigationPatternId, __uuidof(IUIAutomationCustomNavigationPattern), &spCustomNavigationPattern);
            VERIFY_IS_NOT_NULL(spCustomNavigationPattern);
            LogThrow_IfFailed(spCustomNavigationPattern->Navigate(NavigateDirection_LastChild, &spAutomationFoundElement));
            VERIFY_IS_NOT_NULL(spAutomationFoundElement);
            spAutomationFoundElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"Native Node 5", (autoVar.Storage())->bstrVal));
        });
    }

    void XamlUIABridgeIntegrationTests::VerifyPropertyChangedEvents()
    {
        TestCleanupWrapper cleanup;
        wrl::ComPtr<AutomationClient::AutomationPropertyChangedHandler<1>> spAutomationPropertyChangedEventHandler;
        Automation::AutomationClient::UIAElementInfo uiaInfo;

        SetupXAMLUIABridge();
        uiaInfo.m_Name = L"Native Node 3";
        uiaInfo.m_AutomationID = L"Native Node 3";
        uiaInfo.m_ItemStatus = L"";
        uiaInfo.m_cType = UIA_PaneControlTypeId;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<Patterns::InvokePatternHandler> spAutomationInvokePatternHandler;
            auto commonUIAPropertyChangedEvent = std::make_shared<Event>();
            const std::array<PROPERTYID, 1> saPropertyIds = { UIA_NamePropertyId};
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationPropertyChangedEventHandler.Attach(new AutomationClient::AutomationPropertyChangedHandler<1>(spAutomationClientManager, commonUIAPropertyChangedEvent, TreeScope_Subtree, saPropertyIds));
            spAutomationPropertyChangedEventHandler->AttachEventHandler();

            // Name changes on Invoke and that will fire Property Changed event for Name
            auto invokeEvent = std::make_shared<Event>();
            spAutomationInvokePatternHandler.Attach(new Patterns::InvokePatternHandler(spAutomationClientManager, invokeEvent, TreeScope_Subtree));
            spAutomationInvokePatternHandler->Invoke();
        });

        RunOnUIThread([&]()
        {
            // As soon as handler gets called for any of the above three properties, success and bail.
            spAutomationPropertyChangedEventHandler->ConfirmAndUnregister();
        });
    }

    void XamlUIABridgeIntegrationTests::ValidateLandmarkTypeOnAutomationPeer()
    {
        TestCleanupWrapper cleanup;
        SetupXAMLUIABridge();
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"AP Node A";
        uiaInfo.m_AutomationID = L"AP Node A";
        uiaInfo.m_cType = UIA_CustomControlTypeId;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomation> spAutomation;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationElement> spAutomationFoundElement;
            POINT clickablePoint;
            BOOL gotClickable;
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());

            spUIAutomationElement->GetClickablePoint(&clickablePoint, &gotClickable);

            spAutomationClientManager->GetAutomation(&spAutomation);
            spAutomation->ElementFromPoint(clickablePoint, &spAutomationFoundElement);
            VERIFY_IS_NOT_NULL(spAutomationFoundElement);
            WEX::Common::Throw::IfNull(spAutomationFoundElement.Get());

            Common::AutoVariant autoVar;
            spAutomationFoundElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"AP Node A", (autoVar.Storage())->bstrVal));

            wrl::ComPtr<IUIAutomationElement5> spUIAutomationElement5Target;
            VERIFY_SUCCEEDED(spAutomationFoundElement.Get()->QueryInterface(__uuidof(IUIAutomationElement5), &spUIAutomationElement5Target));

            LANDMARKTYPEID landmarkType;
            AutoBSTR localizedLandmarkType;
            VERIFY_SUCCEEDED(spUIAutomationElement5Target->get_CurrentLandmarkType(&landmarkType));
            VERIFY_SUCCEEDED(spUIAutomationElement5Target->get_CurrentLocalizedLandmarkType(localizedLandmarkType.ReleaseAndGetAddressOf()));

            VERIFY_ARE_EQUAL(landmarkType, 0);
            VERIFY_ARE_EQUAL(0, wcscmp(localizedLandmarkType, L""));
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Automation::XamlUIABridge
