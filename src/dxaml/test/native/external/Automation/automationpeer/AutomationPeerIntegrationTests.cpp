// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <AutomationPeerIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <TestCleanupWrapper.h>
#include <UIAutomationHelper.h>
#include <WUCRenderingScopeGuard.h>
#include <FlyoutHelper.h>
#include <ChangeDPI.h>

#include <experimental/resumable>
#include <pplawait.h>

#include "DisableDCompLeakDetectionScopeGuard.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Media3D;
using namespace Microsoft::UI::Xaml::Controls;
using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::UI::WindowManagement;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

bool AutomationPeerIntegrationTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool AutomationPeerIntegrationTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool AutomationPeerIntegrationTests::TestCleanup()
{
    DisableDCompLeakDetectionScopeGuard disableLeakGuard;
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

//
// Test Cases
//
void AutomationPeerIntegrationTests::VerifyCommonUiaPropertyChangesBeingRaised()
{
    TestCleanupWrapper cleanup([]()
    {
        RunOnUIThread([&]()
        {
            TestServices::Utilities->ClearMockUIAClientsListening();
        });

        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    Platform::Object^ providerAsObject;
    xaml_controls::Button^ button;
    xaml_automation_peers::ButtonAutomationPeer^ buttonAP;
    wrl::ComPtr<AutomationClient::AutomationPropertyChangedHandler<3>> spAutomationPropertyChangedEventHandler;
    Automation::AutomationClient::UIAElementInfo uiaInfo;
    uiaInfo.m_Name = L"TestButton";
    uiaInfo.m_AutomationID = L"TestButton";
    uiaInfo.m_ItemStatus = L"ItemStatus-0";
    uiaInfo.m_cType = UIA_ButtonControlTypeId;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setup button in xaml tree");
        button = ref new xaml_controls::Button();
        xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));
        xaml_automation::AutomationProperties::SetItemStatus(button, ref new Platform::String(uiaInfo.m_ItemStatus));
        buttonAP = safe_cast<xaml_automation_peers::ButtonAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(button));
        TestServices::WindowHelper->WindowContent = button;
        button->Content = ref new Platform::String(uiaInfo.m_Name);

        // Ensure m_pUIAWindow is not null by using GetUIAWindow to get/create the UIAWindow
        // This ensures bUIAClientsListeningToProperty is set to true when UIAClientsAreListening is called
        TestServices::WindowHelper->GetUIAWindow(&providerAsObject);
        TestServices::Utilities->SetMockUIAClientsListening();
    });

    TestServices::WindowHelper->WaitForIdle();

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        LOG_OUTPUT(L"Setup UIA Event Handler");
        auto commonUIAPropertyChangedEvent = std::make_shared<Event>();
        const std::array<PROPERTYID, 3> saPropertyIds = { UIA_NamePropertyId, UIA_ItemStatusPropertyId, UIA_IsEnabledPropertyId };
        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
        spAutomationPropertyChangedEventHandler.Attach(new AutomationClient::AutomationPropertyChangedHandler<3>(spAutomationClientManager, commonUIAPropertyChangedEvent, TreeScope_Subtree, saPropertyIds));
        spAutomationPropertyChangedEventHandler->AttachEventHandler();
    });

    std::shared_ptr<Event> spClickEvent = std::make_shared<Event>();
    auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setup click handler on the Button");
        clickRegistration.Attach(button, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
        {
            xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(L"new Name"));
            xaml_automation::AutomationProperties::SetItemStatus(button, ref new Platform::String(L"ItemStatus-1"));
            button->Width = 300;
            button->Height = 300;

            spClickEvent->Set();
        }));
    });

    TestServices::InputHelper->Tap(button);
    spClickEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        // As soon as handler gets called for any of the above three properties, success and bail.
        spAutomationPropertyChangedEventHandler->ConfirmAndUnregister();
    });
}

void AutomationPeerIntegrationTests::VerifyControllerForPropertyChanged()
{
    TestCleanupWrapper cleanup;
    xaml_controls::Button^ button;
    wrl::ComPtr<AutomationClient::AutomationPropertyChangedHandler<1>> spControllerForPropertyChangedHandler;
    Automation::AutomationClient::UIAElementInfo uiaInfo;
    uiaInfo.m_Name = L"TestButton";
    uiaInfo.m_AutomationID = L"TestButton";
    uiaInfo.m_ItemStatus = L"ItemStatus-0";
    uiaInfo.m_cType = UIA_ButtonControlTypeId;

    auto loadedEvent = std::make_shared<Event>();
    auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setup the button in Xaml tree");
        xaml_controls::Grid^ rootPanel = nullptr;

        // grouped ListView
        Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
            L"   <Button x:Name='button' Margin='200,200,0,0' Width='200' Height='200'>" \
            L"   </Button>" \
            L"</Grid>";

        rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
        button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
        xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));
        xaml_automation::AutomationProperties::SetItemStatus(button, ref new Platform::String(uiaInfo.m_ItemStatus));

        loadedRegistration.Attach(rootPanel, [loadedEvent]()
        {
            loadedEvent->Set();
        });

        TestServices::WindowHelper->WindowContent = rootPanel;
        button->Content = ref new Platform::String(uiaInfo.m_Name);
    });
    LOG_OUTPUT(L"Waiting for rootPanel to be loaded...");
    loadedEvent->WaitForDefault();

    TestServices::WindowHelper->WaitForIdle();

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        LOG_OUTPUT(L"Setup UIA Event Handler");
        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
        auto controllerForUIAPropertyChangedEvent = std::make_shared<Event>();
        const std::array<PROPERTYID, 1> saControllerForPropertyIdPropertyId = { UIA_ControllerForPropertyId };

        spControllerForPropertyChangedHandler.Attach(new AutomationClient::AutomationPropertyChangedHandler<1>(spAutomationClientManager, controllerForUIAPropertyChangedEvent, TreeScope_Subtree, saControllerForPropertyIdPropertyId));
        spControllerForPropertyChangedHandler->AttachEventHandler();
    });

    std::shared_ptr<Event> spClickEvent = std::make_shared<Event>();
    auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setup click handler on the Button");
        clickRegistration.Attach(button, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
            auto buttonPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(button);

            // As Narrator doesn't care about Old and new values bsing passed for ControllerFor property, framework simply ignores it,
            // so it can be any inspectable.
            LOG_OUTPUT(L"Click handler is called, now raise ControllerPeers property changed that in UIA is ControllerFor.");
            buttonPeer->RaisePropertyChangedEvent(xaml_automation::AutomationElementIdentifiers::ControlledPeersProperty, ref new Platform::Collections::VectorView<xaml_automation_peers::AutomationPeer^>(), ref new Platform::Collections::VectorView<xaml_automation_peers::AutomationPeer^>());

            spClickEvent->Set();
        }));
    });

    TestServices::InputHelper->Tap(button);
    spClickEvent->WaitForDefault();

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        // If ControllerForProperty Changed event is raised successfully, success and bail.
        spControllerForPropertyChangedHandler->ConfirmAndUnregister();
    });
}

void AutomationPeerIntegrationTests::VerifyUniqueRuntimeIds()
{
    RunOnUIThread([&]()
    {
        wfc::IVector<unsigned int>^ IdVector = ref new Platform::Collections::Vector<unsigned int>();
        for (int i = 0; i < 1000; i++)
        {
            unsigned int index = 0;
            bool found = false;

            // The first item in runtime id vector is constant. It's processId for for phone and UiaAppendRuntimeId for desktop.
            Microsoft::UI::Xaml::Automation::Peers::RawElementProviderRuntimeId runtimeId = Microsoft::UI::Xaml::Automation::Peers::AutomationPeer::GenerateRawElementProviderRuntimeId();
            found = IdVector->IndexOf(runtimeId.Part2, &index);
            VERIFY_IS_FALSE(found);
            IdVector->Append(runtimeId.Part2);
        }
    });
}

void AutomationPeerIntegrationTests::VerifyPeerFromPoint()
{
    RunOnUIThread([&]()
    {
        auto panel = ref new xaml_controls::TextBlock();
        auto automationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(panel);

        ::Windows::Foundation::Point p = { 0, 0 };
        auto result = automationPeer->GetPeerFromPoint(p);
        VERIFY_ARE_EQUAL(automationPeer, result);
    });
}

void AutomationPeerIntegrationTests::VerifyScrollItemPattern()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

    xaml_controls::Button^ button = nullptr;
    xaml_controls::ScrollViewer^ scrollViewer = nullptr;
    xaml_automation_peers::AutomationPeer^ automationPeer = nullptr;

    auto viewChangedEvent = std::make_shared<Event>();
    auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

    auto loadedEvent = std::make_shared<Event>();
    auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, Loaded);

    Automation::AutomationClient::UIAElementInfo uiaInfo;
    uiaInfo.m_Name = L"MyButton";
    uiaInfo.m_AutomationID = L"MyButton";
    uiaInfo.m_cType = UIA_ButtonControlTypeId;

    RunOnUIThread([&]()
    {
        xaml_controls::Grid^ rootPanel = nullptr;

        Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
            L"    <ScrollViewer x:Name='scrollViewer' Height='500' MaxHeight='500'>"
            L"        <StackPanel >"
            L"          <Button x:Name='button1' Content='Button' Width='200' Height='200'/> "
            L"          <Button x:Name='button2' Content='Button' Width='200' Height='200'/> "
            L"          <Button x:Name='button3' Content='Button' Width='200' Height='200'/> "
            L"          <Button x:Name='button3' Content='Button' Width='200' Height='200'/> "
            L"          <Button x:Name='button4' Content='Button' Width='200' Height='200'/> "
            L"          <Button x:Name='button5' Content='Button' Width='200' Height='200'/> "
            L"          <Button x:Name='button6' Content='Button' Width='200' Height='200'/> "
            L"          <Button x:Name='button7' Content='Button' Width='200' Height='200'/> "
            L"          <Button x:Name='button8' Content='Button' Width='200' Height='200'/> "
            L"          <Button x:Name='button9' Content='Button' Width='200' Height='200'/> "
            L"          <Button x:Name='button10' Content='Button' Width='200' Height='200'/> "
            L"        </StackPanel >"
            L"    </ScrollViewer >"
            L"</Grid>";

        rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
        button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button10"));
        scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(rootPanel->FindName(L"scrollViewer"));

        xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(uiaInfo.m_Name));
        automationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(button);

        viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
            [viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
        {
            LOG_OUTPUT(L"ViewChanged raised. IsIntermediate=%d.", args->IsIntermediate);
            if (args->IsIntermediate == false)
            {
                viewChangedEvent->Set();
            }
        }));

        loadedRegistration.Attach(scrollViewer, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
        {
            loadedEvent->Set();
        }));

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();
    LOG_OUTPUT(L"Waiting for ScrollViewer to be loaded...");
    loadedEvent->WaitForDefault();

    wrl::ComPtr<IUIAutomationElement> spUIButton;
    wrl::ComPtr<IUIAutomationScrollItemPattern> spUIButtonAsScrollItemPattern;

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {

        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
        spAutomationClientManager->GetCurrentUIAutomationElement(&spUIButton);

        LOG_OUTPUT(L"Verify that the Buttons's peer supports the ScrollItem patterns.");
        LogThrow_IfFailedWithMessage(spUIButton->GetCurrentPatternAs(UIA_ScrollItemPatternId, __uuidof(IUIAutomationScrollItemPattern), &spUIButtonAsScrollItemPattern),
             L"Failed in fetching Button's ScrollItem pattern.");
        VERIFY_IS_NOT_NULL(spUIButtonAsScrollItemPattern);

        LOG_OUTPUT(L"Scrolling into view");
        spUIButtonAsScrollItemPattern->ScrollIntoView();
    });

    LOG_OUTPUT(L"Waiting for the scrollViewer to finish scrolling.");
    viewChangedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Verify that we scroll the Button into view.");
    RunOnUIThread([&]()
    {
        VERIFY_IS_FALSE(automationPeer->IsOffscreen());
    });
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void AutomationPeerIntegrationTests::VerifyContainerWithAutomationNameHasAP()
{
    VerifyContainerWithAutomationNameHasAPMarkup();
    LOG_OUTPUT(L"\r\n");
    VerifyContainerWithAutomationNameHasAPCodebehind();
}

void AutomationPeerIntegrationTests::VerifyContainerWithLabeledByHasAP()
{
    VerifyContainerWithLabeledByHasAPMarkup();
    LOG_OUTPUT(L"\r\n");
    VerifyContainerWithLabeledByHasAPCodebehind();
}

void AutomationPeerIntegrationTests::VerifyContainerWithAutomationNameHasAPMarkup()
{
    LOG_OUTPUT(L"Verify adding AutomationProperties.Name from markup");

    Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
    uiaInfoTarget.m_Name = L"Interesting container";

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = static_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(
            "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' AutomationProperties.Name='Interesting container' />"));
    });

    TestServices::WindowHelper->WaitForIdle();

    VerifyContainerWithLabeledByHasUiaElement(uiaInfoTarget);
}

void AutomationPeerIntegrationTests::VerifyContainerWithAutomationNameHasAPCodebehind()
{
    LOG_OUTPUT(L"Verify adding AutomationProperties.Name from codebehind");

    Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
    uiaInfoTarget.m_Name = L"Interesting container";
    xaml_controls::StackPanel^ stackPanel;

    RunOnUIThread([&]()
    {
        stackPanel = ref new xaml_controls::StackPanel;
        xaml_automation::AutomationProperties::SetName(stackPanel, ref new Platform::String(uiaInfoTarget.m_Name));
        TestServices::WindowHelper->WindowContent = stackPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    VerifyContainerWithLabeledByHasUiaElement(uiaInfoTarget);
}

void AutomationPeerIntegrationTests::VerifyContainerWithLabeledByHasAPMarkup()
{
    LOG_OUTPUT(L"Verify adding AutomationProperties.Name from markup");

    Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
    uiaInfoTarget.m_Name = L"Interesting container";

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = static_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(
            "<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' AutomationProperties.LabeledBy='{Binding ElementName=childButton}'>"
            "    <Button x:Name='childButton' AutomationProperties.Name='Interesting container' />"
            "</StackPanel>"));
    });

    TestServices::WindowHelper->WaitForIdle();

    VerifyContainerWithLabeledByHasUiaElement(uiaInfoTarget);
}

void AutomationPeerIntegrationTests::VerifyContainerWithLabeledByHasAPCodebehind()
{
    LOG_OUTPUT(L"Verify adding AutomationProperties.Name from codebehind");

    Automation::AutomationClient::UIAElementInfo uiaInfoTarget;
    uiaInfoTarget.m_Name = L"Interesting container";
    xaml_controls::StackPanel^ stackPanel;
    xaml_controls::Button^ childButton;

    RunOnUIThread([&]()
    {
        childButton = ref new xaml_controls::Button;
        xaml_automation::AutomationProperties::SetName(childButton, ref new Platform::String(uiaInfoTarget.m_Name));

        stackPanel = ref new xaml_controls::StackPanel;
        xaml_automation::AutomationProperties::SetLabeledBy(stackPanel, childButton);

        stackPanel->Children->Append(childButton);
        TestServices::WindowHelper->WindowContent = stackPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    VerifyContainerWithLabeledByHasUiaElement(uiaInfoTarget);
}

void AutomationPeerIntegrationTests::VerifyContainerWithLabeledByHasUiaElement(const Automation::AutomationClient::UIAElementInfo& uiaInfo)
{
    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        // XAML will populate the Name property for the container with the Name of the LabeledBy target ("Interesting container").
        auto automationClientManagerTarget = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
        wrl::ComPtr<IUIAutomationElement> targetUiaElement;
        automationClientManagerTarget->GetCurrentUIAutomationElement(&targetUiaElement);
        WEX::Common::Throw::IfNull(targetUiaElement.Get());

        CONTROLTYPEID controlType;
        VERIFY_SUCCEEDED(targetUiaElement->get_CurrentControlType(&controlType));
        VERIFY_ARE_EQUAL(controlType, UIA_GroupControlTypeId);
    });
}

void AutomationPeerIntegrationTests::VerifyAccessKeysDeliverCorrectMessage()
{
    TestCleanupWrapper cleanup;
    xaml_controls::Button^ button1 = nullptr;
    Microsoft::UI::Xaml::Automation::Peers::AutomationPeer^ button1AP = nullptr;
    Platform::String^ button1Message = "Alt, H";


    xaml_controls::Button^ button2 = nullptr;
    Microsoft::UI::Xaml::Automation::Peers::AutomationPeer^ button2AP = nullptr;
    Platform::String^ button2Message = "Alt, J, I";

    xaml_controls::Button^ button3 = nullptr;
    Microsoft::UI::Xaml::Automation::Peers::AutomationPeer^ button3AP = nullptr;
    Platform::String^ button3Message = "Alt, H, L";

    RunOnUIThread([&]()
    {
        auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
            L"<Button  x:Name='button1' IsAccessKeyScope='true' AccessKey='H'>button1</Button>"
            L"<Button  x:Name='button2' AccessKey='JI'>button2</Button>"
            L"<Button  x:Name='button3' AccessKey='L' AccessKeyScopeOwner='{Binding ElementName=button1}' >button3</Button>"
            L"</StackPanel>"));

        button1 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName("button1"));
        button1AP = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(button1);

        button2 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName("button2"));
        button2AP = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(button2);

        button3 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName("button3"));
        button3AP = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(button3);

        TestServices::WindowHelper->WindowContent = rootPanel;
    });


    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Checking message for single character access keys.");
        VERIFY_ARE_EQUAL(button1Message, button1AP->GetAccessKey());

        LOG_OUTPUT(L"Checking message for multi character access keys.");
        VERIFY_ARE_EQUAL(button2Message, button2AP->GetAccessKey());

        LOG_OUTPUT(L"Checking message for access keys with scope owners.");
        VERIFY_ARE_EQUAL(button3Message, button3AP->GetAccessKey());
    });
}

void AutomationPeerIntegrationTests::VerifyNoAccessKeyDeliversEmptyAccessKeyMessage()
{
    TestCleanupWrapper cleanup;
    xaml_controls::Button^ button1 = nullptr;
    Microsoft::UI::Xaml::Automation::Peers::AutomationPeer^ button1AP = nullptr;
    Platform::String^ button1Message = "";

    RunOnUIThread([&]()
    {
        auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
            L"<Button  x:Name='button1'>button1</Button>"
            L"</StackPanel>"));

        button1 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName("button1"));
        button1AP = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(button1);
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(button1Message, button1AP->GetAccessKey());
    });
}

void AutomationPeerIntegrationTests::VerifyCorrectAccessKeyMessageDeliveredForMultiLevelScope()
{
    TestCleanupWrapper cleanup;

    xaml_controls::Button^ button3 = nullptr;
    Microsoft::UI::Xaml::Automation::Peers::AutomationPeer^ button3AP = nullptr;
    Platform::String^ button3Message = "Alt, H, J, I, L";

    RunOnUIThread([&]()
    {
        auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
            L"<Button  x:Name='button1' IsAccessKeyScope='true' AccessKey='H'>button1</Button>"
            L"<Button  x:Name='button2' IsAccessKeyScope='true' AccessKey='JI' AccessKeyScopeOwner='{Binding ElementName=button1}'>button2</Button>"
            L"<Button  x:Name='button3' AccessKey='L' AccessKeyScopeOwner='{Binding ElementName=button2}' >button3</Button>"
            L"</StackPanel>"));

        button3 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName("button3"));
        button3AP = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(button3);

        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Checking message for access keys with multiple scope owners.");
        VERIFY_ARE_EQUAL(button3Message, button3AP->GetAccessKey());
    });
}

void AutomationPeerIntegrationTests::VerifyCorrectAccessKeyMessageDeliveredForCustomAP()
{
    TestCleanupWrapper cleanup;
    CustomButton^ button1 = nullptr;
    Microsoft::UI::Xaml::Automation::Peers::AutomationPeer^ button1AP = nullptr;
    Platform::String^ button1Message = "A";

    xaml_controls::Button^ button2 = nullptr;
    Microsoft::UI::Xaml::Automation::Peers::AutomationPeer^ button2AP = nullptr;
    Platform::String^ button2Message = "A, L";

    RunOnUIThread([&]()
    {
        auto rootPanel = ref new xaml_controls::StackPanel;

        button1 = ref new CustomButton;
        button1->AccessKey = L"H";
        button1->IsAccessKeyScope = true;
        button1AP = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(button1);

        button2 = ref new xaml_controls::Button;
        button2->AccessKey = L"L";
        button2->AccessKeyScopeOwner = button1;
        button2AP = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(button2);

        rootPanel->Children->Append(button1);
        rootPanel->Children->Append(button2);
        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Checking message directly from Custom AP.");
        VERIFY_ARE_EQUAL(button1Message, button1AP->GetAccessKey());

        LOG_OUTPUT(L"Checking message when custom AP is attached to AKScopeOwner.");
        VERIFY_ARE_EQUAL(button2Message, button2AP->GetAccessKey());
    });
}

void AutomationPeerIntegrationTests::VerifyAcceleratorKeysDeliverCorrectMessage()
{
    TestCleanupWrapper cleanup;
    xaml_controls::Button^ button1 = nullptr;
    Microsoft::UI::Xaml::Automation::Peers::AutomationPeer^ button1AP = nullptr;
    Platform::String^ button1Message = "";

    xaml_controls::Button^ button2 = nullptr;
    Microsoft::UI::Xaml::Automation::Peers::AutomationPeer^ button2AP = nullptr;
    Platform::String^ button2Message = "";

    xaml_controls::Button^ button3 = nullptr;
    Microsoft::UI::Xaml::Automation::Peers::AutomationPeer^ button3AP = nullptr;
    Platform::String^ button3Message = "";

    RunOnUIThread([&]()
    {
        auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
            L"<StackPanel x:Name='root' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
            L"<Button  x:Name='button1' IsAccessKeyScope='true' AccessKey='H'>button1</Button>"
            L"<Button  x:Name='button2' AccessKey='JI'>button2</Button>"
            L"<Button  x:Name='button3' AccessKey='L' AccessKeyScopeOwner='{Binding ElementName=button1}'>button3</Button>"
            L"</StackPanel>"));

        button1 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName("button1"));
        button1AP = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(button1);

        button2 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName("button2"));
        button2AP = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(button2);

        button3 = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName("button3"));
        button3AP = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(button3);

        TestServices::WindowHelper->WindowContent = rootPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Checking message for single character access keys.");
        VERIFY_ARE_EQUAL(button1Message, button1AP->GetAcceleratorKey());

        LOG_OUTPUT(L"Checking message for multi character access keys.");
        VERIFY_ARE_EQUAL(button2Message, button2AP->GetAcceleratorKey());

        LOG_OUTPUT(L"Checking message for access keys with scope owners.");
        VERIFY_ARE_EQUAL(button3Message, button3AP->GetAcceleratorKey());
    });
}

void AutomationPeerIntegrationTests::VerifyTrimmedKeyboardAcceleratorTextOverride()
{
    TestCleanupWrapper cleanup;

    xaml_controls::Button^ root = nullptr;
    xaml_controls::MenuFlyout^ menuFlyout = nullptr;
    xaml_controls::MenuFlyoutItem^ menuFlyoutItem0 = nullptr;
    xaml_controls::MenuFlyoutItem^ menuFlyoutItem1 = nullptr;

    auto loadedEvent = std::make_shared<Event>();
    auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Loaded);

    RunOnUIThread([&]()
    {
        root = safe_cast<xaml_controls::Button^>(xaml_markup::XamlReader::Load(
            LR"(<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                    Content='button.flyout'>
                    <Button.Flyout>
                        <MenuFlyout x:Name='menuFlyout'>
                            <MenuFlyoutItem>MenuFlyoutItem0</MenuFlyoutItem>
                            <MenuFlyoutItem>MenuFlyoutItem1</MenuFlyoutItem>
                        </MenuFlyout>
                    </Button.Flyout>
                </Button>)"));
        VERIFY_IS_NOT_NULL(root);

        loadedRegistration.Attach(root, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
        {
            LOG_OUTPUT(L"Button.Loaded raised.");
            loadedEvent->Set();
        }));

        TestServices::WindowHelper->WindowContent = root;
    });

    LOG_OUTPUT(L"Waiting for Button.Loaded event...");
    loadedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Accessing Button.MenuFlyout.");
        menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(root->Flyout);
        VERIFY_IS_NOT_NULL(menuFlyout);

        LOG_OUTPUT(L"Accessing 2 MenuFlyoutItem.");
        menuFlyoutItem0 = safe_cast<xaml_controls::MenuFlyoutItem^>(menuFlyout->Items->GetAt(0));
        VERIFY_IS_NOT_NULL(menuFlyoutItem0);

        menuFlyoutItem1 = safe_cast<xaml_controls::MenuFlyoutItem^>(menuFlyout->Items->GetAt(1));
        VERIFY_IS_NOT_NULL(menuFlyoutItem1);

        LOG_OUTPUT(L"Setting first MenuFlyoutItem.KeyboardAcceleratorTextOverride to single space.");
        menuFlyoutItem0->KeyboardAcceleratorTextOverride = ref new Platform::String(L" ");

        LOG_OUTPUT(L"Setting second MenuFlyoutItem.KeyboardAcceleratorTextOverride to custom string.");
        menuFlyoutItem1->KeyboardAcceleratorTextOverride = ref new Platform::String(L"ABC");

        LOG_OUTPUT(L"Accessing 2 MenuFlyoutItem's MenuFlyoutItemAutomationPeer.");
        Microsoft::UI::Xaml::Automation::Peers::MenuFlyoutItemAutomationPeer^ menuFlyoutItemAP0 = safe_cast<Microsoft::UI::Xaml::Automation::Peers::MenuFlyoutItemAutomationPeer^>(
            Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(menuFlyoutItem0));
        VERIFY_IS_NOT_NULL(menuFlyoutItemAP0);

        Microsoft::UI::Xaml::Automation::Peers::MenuFlyoutItemAutomationPeer^ menuFlyoutItemAP1 = safe_cast<Microsoft::UI::Xaml::Automation::Peers::MenuFlyoutItemAutomationPeer^>(
            Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(menuFlyoutItem1));
        VERIFY_IS_NOT_NULL(menuFlyoutItemAP1);

        LOG_OUTPUT(L"Accessing 2 MenuFlyoutItemAutomationPeer's AcceleratorKey.");
        Platform::String^ acceleratorKey0 = menuFlyoutItemAP0->GetAcceleratorKey();
        Platform::String^ acceleratorKey1 = menuFlyoutItemAP1->GetAcceleratorKey();

        LOG_OUTPUT(L"Verifying first AcceleratorKey=\"%s\" is empty.", acceleratorKey0->Data());
        VERIFY_IS_TRUE(acceleratorKey0->IsEmpty());

        LOG_OUTPUT(L"Verifying second AcceleratorKey=\"%s\" is not empty.", acceleratorKey1->Data());
        VERIFY_IS_FALSE(acceleratorKey1->IsEmpty());
    });

    LOG_OUTPUT(L"Opening the MenuFlyout.");
    FlyoutHelper::OpenFlyout(menuFlyout, root, FlyoutOpenMethod::Programmatic_ShowAt);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Accessing 2 KeyboardAcceleratorTextBlock's AutomationProperties.AccessibilityView attached property.");
        xaml_controls::Grid^ layoutRoot0 = dynamic_cast<xaml_controls::Grid^>(xaml_media::VisualTreeHelper::GetChild(menuFlyoutItem0, 0));
        VERIFY_IS_NOT_NULL(layoutRoot0);

        xaml_controls::Grid^ layoutRoot1 = dynamic_cast<xaml_controls::Grid^>(xaml_media::VisualTreeHelper::GetChild(menuFlyoutItem1, 0));
        VERIFY_IS_NOT_NULL(layoutRoot1);

        xaml_controls::TextBlock^ keyboardAcceleratorTextBlock0 = dynamic_cast<xaml_controls::TextBlock^>(xaml_media::VisualTreeHelper::GetChild(layoutRoot0, 2));
        VERIFY_IS_NOT_NULL(keyboardAcceleratorTextBlock0);

        xaml_controls::TextBlock^ keyboardAcceleratorTextBlock1 = dynamic_cast<xaml_controls::TextBlock^>(xaml_media::VisualTreeHelper::GetChild(layoutRoot1, 2));
        VERIFY_IS_NOT_NULL(keyboardAcceleratorTextBlock1);

        LOG_OUTPUT(L"Verifying 2 KeyboardAcceleratorTextBlock's AutomationProperties.AccessibilityView value is AccessibilityView.Raw.");
        xaml_automation::Peers::AccessibilityView accessibilityView0 = xaml_automation::AutomationProperties::GetAccessibilityView(keyboardAcceleratorTextBlock0);
        VERIFY_ARE_EQUAL(accessibilityView0, xaml_automation::Peers::AccessibilityView::Raw);

        xaml_automation::Peers::AccessibilityView accessibilityView1 = xaml_automation::AutomationProperties::GetAccessibilityView(keyboardAcceleratorTextBlock1);
        VERIFY_ARE_EQUAL(accessibilityView1, xaml_automation::Peers::AccessibilityView::Raw);
    });

    LOG_OUTPUT(L"Closing the MenuFlyout.");
    FlyoutHelper::HideFlyout(menuFlyout);
}

void AutomationPeerIntegrationTests::VerifyIsOffscreen_ChildHas3D()
{
    TestCleanupWrapper cleanup;
    auto wh = TestServices::WindowHelper;

    xaml_automation_peers::AutomationPeer^ automationPeer = nullptr;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        CompositeTransform3D^ composite = ref new CompositeTransform3D();
        composite->TranslateZ = 0.1;

        Grid^ element = ref new Grid();
        element->Width = 100;
        element->Height = 100;
        element->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
        element->Transform3D = composite;

        Button^ parent = ref new Button();
        parent->Content = element;

        Grid^ root = ref new Grid();
        root->Width = 400;
        root->Height = 300;
        root->Children->Append(parent);
        wh->WindowContent = root;

        automationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(parent);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Verify that the element with only 3D content is still in view.");
        VERIFY_IS_FALSE(automationPeer->IsOffscreen());
    });
}

void AutomationPeerIntegrationTests::VerifyBounds_WindowClip()
{
    TestCleanupWrapper cleanup;
    auto wh = TestServices::WindowHelper;
    wh->SetWindowSizeOverride(wf::Size(300, 300));

    xaml_automation_peers::AutomationPeer^ automationPeer = nullptr;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree.");

        //
        // The window bounds are 300x300. Create a 100x100 button and bottom-right align it. Then put a 4x scale on
        // the root, which pushes the button out-of-bounds.
        //
        // IsOffscreen should always report true, but under OneCoreTransforms we should be getting a non-zero-sized
        // (and out-of-bounds) rect from GetBoundingRectangle.
        //

        ScaleTransform^ scale = ref new ScaleTransform();
        scale->ScaleX = 4;
        scale->ScaleY = 4;

        Grid^ element = ref new Grid();
        element->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        Button^ parent = ref new Button();
        parent->Width = 100;
        parent->Height = 100;
        parent->Content = element;
        parent->HorizontalAlignment = HorizontalAlignment::Right;
        parent->VerticalAlignment = VerticalAlignment::Bottom;

        Grid^ root = ref new Grid();
        root->RenderTransform = scale;
        root->Children->Append(parent);
        wh->WindowContent = root;

        automationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(parent);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> IsOffscreen should always return true. It should always respect the window clip.");
        VERIFY_IS_TRUE(automationPeer->IsOffscreen());
    });

    if (XamlOneCoreTransforms::IsEnabled())
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> OneCoreTransforms mode - GetBoundingRectangle should return a non-zero rect despite the control being outside the window clip.");
            Rect boundingRect = automationPeer->GetBoundingRectangle();
            VERIFY_ARE_EQUAL(800, boundingRect.X);  // 100x100 bottom-right aligned to 300x300 gives (200,200) offset, scaled up by 4 gives 800
            VERIFY_ARE_EQUAL(800, boundingRect.Y);
            VERIFY_ARE_EQUAL(400, boundingRect.Width);
            VERIFY_ARE_EQUAL(400, boundingRect.Height);
        });
    }
    else
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"> Non-OneCoreTransforms mode - GetBoundingRectangle should return a zero-sized rect because the control is clipped out by the window.");
            Rect boundingRect = automationPeer->GetBoundingRectangle();
            VERIFY_ARE_EQUAL(0, boundingRect.Width);
            VERIFY_ARE_EQUAL(0, boundingRect.Height);
        });
    }
}

void AutomationPeerIntegrationTests::VerifyAutomationPeerOnRootChange()
{
    TestCleanupWrapper cleanup;

    xaml_controls::StackPanel^ stackPanel1;
    xaml_controls::StackPanel^ stackPanel2;

    RunOnUIThread([&]()
    {
        stackPanel1 = ref new xaml_controls::StackPanel();
        stackPanel2 = ref new xaml_controls::StackPanel();
        xaml_automation::AutomationProperties::SetName(stackPanel1, ref new Platform::String(L"stackPanel1"));
        xaml_automation::AutomationProperties::SetName(stackPanel2, ref new Platform::String(L"stackPanel2"));

        LOG_OUTPUT(L"Setting WindowContent to stackPanel1");
        TestServices::WindowHelper->WindowContent = stackPanel1;
    });
    TestServices::WindowHelper->WaitForIdle();

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"stackPanel1";

        LOG_OUTPUT(L"Verifying we can find stackPanel1 AutomationPeer");
        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Setting WindowContent to stackPanel2");
        TestServices::WindowHelper->WindowContent = stackPanel2;
    });
    TestServices::WindowHelper->WaitForIdle();

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"stackPanel2";

        LOG_OUTPUT(L"Verifying we can find stackPanel2 AutomationPeer");
        auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
    });

}

void AutomationPeerIntegrationTests::FocusTest()
{
    TestCleanupWrapper cleanup;

    xaml_controls::StackPanel^ rootPanel;
    xaml_controls::Button^ button;
    xaml_controls::StackPanel^ sp;
    xaml_controls::TextBlock^ tb;
    xaml_controls::RichTextBlock^ rtb;
    xaml_automation_peers::AutomationPeer^ buttonPeer;
    xaml_automation_peers::AutomationPeer^ spPeer;
    xaml_automation_peers::AutomationPeer^ tbPeer;
    xaml_automation_peers::AutomationPeer^ rtbPeer;

    RunOnUIThread([&]()
    {
        rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
            L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
            L"  <Button x:Name='btn' Content='Button' IsTabStop='False'/>"
            L"  <StackPanel x:Name='sp' HorizontalAlignment = 'Left' Width='50' Height='50' Background='Red' UseSystemFocusVisuals='True'/>"
            L"  <TextBlock x:Name='tb' UseSystemFocusVisuals='True' Text='TextBlock'/>"
            L"  <RichTextBlock x:Name='rtb' UseSystemFocusVisuals='True'>"
            L"    <Paragraph>RichTextBlock</Paragraph>"
            L"  </RichTextBlock>"
            L"</StackPanel>"));

        VERIFY_IS_NOT_NULL(rootPanel);

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();
    
    RunOnUIThread([&]()
    {
        button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn"));
        VERIFY_IS_NOT_NULL(button);
        xaml_automation::AutomationProperties::SetName(button, ref new Platform::String(L"TestButton"));
        buttonPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(button);
        VERIFY_IS_FALSE(buttonPeer->IsKeyboardFocusable());
        
        sp = safe_cast<xaml_controls::StackPanel^>(rootPanel->FindName(L"sp"));
        VERIFY_IS_NOT_NULL(sp);
        xaml_automation::AutomationProperties::SetName(sp, ref new Platform::String(L"TestStackPanel"));
        spPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(sp);
        VERIFY_IS_FALSE(spPeer->IsKeyboardFocusable());
        
        tb = safe_cast<xaml_controls::TextBlock^>(rootPanel->FindName(L"tb"));
        VERIFY_IS_NOT_NULL(tb);
        xaml_automation::AutomationProperties::SetName(tb, ref new Platform::String(L"TestTextBlock"));
        tbPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(tb);
        VERIFY_IS_FALSE(tbPeer->IsKeyboardFocusable());        

        rtb = safe_cast<xaml_controls::RichTextBlock^>(rootPanel->FindName(L"rtb"));
        VERIFY_IS_NOT_NULL(rtb);
        xaml_automation::AutomationProperties::SetName(rtb, ref new Platform::String(L"TestRichTextBlock"));
        rtbPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(rtb);
        VERIFY_IS_FALSE(rtbPeer->IsKeyboardFocusable());        

        button->IsTabStop = true;
        sp->IsTabStop = true;
        tb->IsTabStop = true;
        rtb->IsTabStop = true;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(buttonPeer->IsKeyboardFocusable());
        buttonPeer->SetFocus();
        VERIFY_IS_TRUE(buttonPeer->HasKeyboardFocus());
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(spPeer->IsKeyboardFocusable());
        spPeer->SetFocus();
        VERIFY_IS_TRUE(spPeer->HasKeyboardFocus());
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(tbPeer->IsKeyboardFocusable());
        tbPeer->SetFocus();
        VERIFY_IS_TRUE(tbPeer->HasKeyboardFocus());
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(rtbPeer->IsKeyboardFocusable());
        rtbPeer->SetFocus();
        VERIFY_IS_TRUE(rtbPeer->HasKeyboardFocus());
    });
    TestServices::WindowHelper->WaitForIdle();
}

void AutomationPeerIntegrationTests::VerifyPeerFromPointWithNon100DpiIsCorrect()
{
    TestCleanupWrapper cleanup;
    
    RunOnUIThread([&]()
    {
        auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button Content='Button' AutomationProperties.Name='Button' HorizontalAlignment='Center' VerticalAlignment='Center'/>"
            L"</Grid>"));

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    
    TestServices::WindowHelper->WaitForIdle();

    // Sets the scale factor to 125% for the duration of this test.  Note that this needs to come
    // after we set WindowContent so we properly pick up the XamlRoot, and needs a WaitForIdle after it
    // to ensure we wait for the scale to change before proceeding.
    ChangeDPI changeDPI;
    
    TestServices::WindowHelper->WaitForIdle();

    UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
    {
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"Button";

        LOG_OUTPUT(L"Retrieve the button's automation peer.");
        auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
        
        wrl::ComPtr<IUIAutomation> uiAutomation;
        automationClientManager->GetAutomation(&uiAutomation);
        
        wrl::ComPtr<IUIAutomationElement> uiAutomationElement;
        automationClientManager->GetCurrentUIAutomationElement(&uiAutomationElement);
        
        LOG_OUTPUT(L"Retrieve the button's bounding rectangle.");
        RECT buttonRect;
        VERIFY_SUCCEEDED(uiAutomationElement->get_CurrentBoundingRectangle(&buttonRect));
        
        LOG_OUTPUT(L"(Left, Top, Right, Bottom) = (%d, %d, %d, %d).", buttonRect.left, buttonRect.top, buttonRect.right, buttonRect.bottom);
        
        LOG_OUTPUT(L"Now retrieve the element at the center of that rectangle.  We should get the same object.");
        POINT point{ buttonRect.left + (buttonRect.right - buttonRect.left) / 2, buttonRect.top + (buttonRect.bottom - buttonRect.top) / 2 };
        LOG_OUTPUT(L"Center = (%d, %d).", point.x, point.y);
        wrl::ComPtr<IUIAutomationElement> uiAutomationElementAtPoint;
        VERIFY_SUCCEEDED(uiAutomation->ElementFromPoint(point, &uiAutomationElementAtPoint));
        
        VERIFY_IS_TRUE(automationClientManager->IsElementSame(uiAutomationElementAtPoint.Get()));
    });
}


} } } } } }
