// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SliderAutomationIntegrationTests.h"

#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\RangeBaseTests.h>

#include <ControlHelper.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Slider {

    bool SliderAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool SliderAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool SliderAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //

    void SliderAutomationIntegrationTests::VerifyDefaultAutomationName()
    {
        TestCleanupWrapper cleanup;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"sliderAutoName";

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Width="200" >
                        <Slider x:Name="slider" AutomationProperties.Name="sliderAutoName" Header="sliderAutoNameHeader"/>
                        <Slider x:Name="sliderWithHeader" Header="sliderWithHeader" />
                        <Slider x:Name="sliderWithNoName" />
                    </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spSliderWithName;
            wrl::ComPtr<IUIAutomationElement> spSliderWithHeader;
            wrl::ComPtr<IUIAutomationElement> spSliderWithNoName;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            Common::AutoVariant autoVar;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spSliderWithName);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for SliderWithName exists.");
            VERIFY_IS_NOT_NULL(spSliderWithName);

            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spSliderWithName.");
            spSliderWithName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the second slider.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spSliderWithName.Get(), &spSliderWithHeader);
            VERIFY_IS_NOT_NULL(spSliderWithHeader);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spSliderWithHeader.");
            spSliderWithHeader->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"sliderWithHeader", (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the third slider.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spSliderWithHeader.Get(), &spSliderWithNoName);
            VERIFY_IS_NOT_NULL(spSliderWithNoName);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spSliderWithNoName.");
            spSliderWithNoName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"", (autoVar.Storage())->bstrVal));
        });
    }

    void SliderAutomationIntegrationTests::SetupAutomateSlider(xaml_controls::Slider^* slider)
    {
        xaml_controls::Slider^ NewSliderElement = nullptr;
        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Width="200" >
                        <Slider x:Name="slider" AutomationProperties.Name="slider" />
                    </StackPanel>)"));

            NewSliderElement = safe_cast<xaml_controls::Slider^>(rootPanel->FindName(L"slider"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        *slider = NewSliderElement;
    }


    void SliderAutomationIntegrationTests::VerifyValuePropertyChangedEventIsRaised()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Slider^ slider = nullptr;

        auto valuePropertyChangedEvent = std::make_shared<Event>();
        wrl::ComPtr<AutomationClient::AutomationPropertyChangedHandler<1>> spAutomationPropertyChangedEventHandler;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"slider";

        SetupAutomateSlider(&slider);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            const std::array<PROPERTYID, 1> propertyIds = { UIA_RangeValueValuePropertyId };
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationPropertyChangedEventHandler.Attach(new AutomationClient::AutomationPropertyChangedHandler<1>(spAutomationClientManager, valuePropertyChangedEvent, TreeScope_Subtree, propertyIds));
            spAutomationPropertyChangedEventHandler->AttachEventHandler();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            slider->Value = 40;
        });
        TestServices::WindowHelper->WaitForIdle();

        valuePropertyChangedEvent->WaitForDefault();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            spAutomationPropertyChangedEventHandler->RemoveEventHandler();
        });
    }

    void SliderAutomationIntegrationTests::VerifyValuePropertyChangedEventIsRaisedMultipleTimes()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Slider^ slider = nullptr;

        auto valuePropertyChangedEvent = std::make_shared<Event>();
        wrl::ComPtr<AutomationClient::AutomationPropertyChangedHandler<1>> spAutomationPropertyChangedEventHandler;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"slider";

        SetupAutomateSlider(&slider);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            const std::array<PROPERTYID, 1> propertyIds = { UIA_RangeValueValuePropertyId };
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationPropertyChangedEventHandler.Attach(new AutomationClient::AutomationPropertyChangedHandler<1>(spAutomationClientManager, valuePropertyChangedEvent, TreeScope_Subtree, propertyIds));
            spAutomationPropertyChangedEventHandler->AttachEventHandler();
        });
        TestServices::WindowHelper->WaitForIdle();

        // N.B. We expect the number of property change notifications from UIA is double the actual
        //      number of changes, when we register listeners directly on this background thread. 
        //      This is due to a long standing UIA bug, where notifications are doubled for in-proc 
        //      listeners off the UI thread.
        constexpr int ChangeCount = 20;
        int expectedNotificationCount = 
            UIAutomationHelper::DoesInProcClientRunOnBackgroundThread() ? ChangeCount * 2 : ChangeCount;

        valuePropertyChangedEvent->Reset();
        RunOnUIThread([&]()
        {
            for (int i = 0; i < ChangeCount; i++)
            {
                slider->Value = i + 1;
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        valuePropertyChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(expectedNotificationCount, valuePropertyChangedEvent->TimesFired());

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            spAutomationPropertyChangedEventHandler->RemoveEventHandler();
        });
    }

    void SliderAutomationIntegrationTests::VerifyValuePropertyChangedEventCoalescingonMTC()
    {
        // TODO: Convert ME tests to MPE.
#if false
        TestCleanupWrapper cleanup;

        MediaElement^ testMediaElement = nullptr;
        xaml_controls::Slider^ slider = nullptr;

        wrl::ComPtr<AutomationClient::AutomationPropertyChangedHandler<1>> spAutomationPropertyChangedEventHandler;
        auto valuePropertyChangedEvent = std::make_shared<Event>();

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"Seek";

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Width="200" >
                        <MediaElement x:Name="testMediaElement" AreTransportControlsEnabled="true" />
                    </StackPanel>)"));

            testMediaElement = safe_cast<xaml_controls::MediaElement^>(rootPanel->FindName(L"testMediaElement"));
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto testMediaTransportControls = testMediaElement->TransportControls;
            LOG_OUTPUT(L"Verfiy for Seek Progressbar visibility by default");
            slider = safe_cast<xaml_controls::Slider^>(TreeHelper::GetVisualChildByName(testMediaElement, L"ProgressSlider"));
            VERIFY_IS_NOT_NULL(slider);
            VERIFY_IS_TRUE(slider->Visibility == xaml::Visibility::Visible);
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            const std::array<PROPERTYID, 1> propertyIds = { UIA_RangeValueValuePropertyId };
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationPropertyChangedEventHandler.Attach(new AutomationClient::AutomationPropertyChangedHandler<1>(spAutomationClientManager, valuePropertyChangedEvent, TreeScope_Subtree, propertyIds));
            spAutomationPropertyChangedEventHandler->AttachEventHandler();
        });
        TestServices::WindowHelper->WaitForIdle();

        valuePropertyChangedEvent->Reset();
        RunOnUIThread([&]()
        {
            for (int i = 1; i < 20; i++)
            {
                slider->Value = i;
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        valuePropertyChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(1, valuePropertyChangedEvent->TimesFired());

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            spAutomationPropertyChangedEventHandler->RemoveEventHandler();
        });
#endif
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Slider
