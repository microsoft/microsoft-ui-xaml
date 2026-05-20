// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <TestEvent.h>
#include <TreeHelper.h>
#include <PopupHelper.h>
#include <AutomationClient\AutomationClientInitializer.h>
#include <AutomationClient\AutomationClientManager.h>
#include <TimePickerFlyoutAutomationIntegrationTests.h>
#include <UIAutomationHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace TimePickerFlyout {

    bool TimePickerFlyoutAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool TimePickerFlyoutAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool TimePickerFlyoutAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    xaml_controls::TimePicker^ TimePickerFlyoutAutomationIntegrationTests::SetupTimePickerTest()
    {
        xaml_controls::TimePicker^ timePicker = nullptr;
        xaml_controls::Button^ button = nullptr;
        wf::TimeSpan timeSpanOriginal = {};

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::TimePicker, Loaded);

        RunOnUIThread([&]()
        {
            auto rootGrid = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = rootGrid;

            timePicker = ref new xaml_controls::TimePicker();
            timePicker->Header = L"TimePickerTest";

            // Set the time to 6:00AM.
            timeSpanOriginal.Duration = (int64)10000000 * (6 * 60) * 60;
            timePicker->Time = timeSpanOriginal;

            loadedRegistration.Attach(
                timePicker,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            rootGrid->Children->Append(timePicker);
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            button = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(timePicker, "FlyoutButton"));
            VERIFY_IS_NOT_NULL(button);
        });

        // Open the flyout using AP
        std::shared_ptr<Event> clickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

        RunOnUIThread([&]()
        {
            clickRegistration.Attach(
                button,
                ref new xaml::RoutedEventHandler([clickEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                clickEvent->Set();
            }));

            auto buttonAP = safe_cast<xaml_automation_peers::ButtonAutomationPeer^>(
                xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(button)
                );

            buttonAP->Invoke();
        });

        clickEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // For some reason, we can end up in the situation where even after waiting for both the event and for idle,
        // the popup still hasn't shown up.  We'll explicitly wait until we have a popup before continuing.
        PopupHelper::WaitForOpenPopup(button);

        return timePicker;
    }

    //
    // Test Cases
    //
    void TimePickerFlyoutAutomationIntegrationTests::VerifyCanNavigateThroughAllItems()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"timepicker";
        xaml_controls::TimePicker^ timePicker = SetupTimePickerTest();

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(timePicker);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            wrl::ComPtr<IUIAutomationElement> spRoot;
            wrl::ComPtr<IUIAutomationElement> spHourList;
            wrl::ComPtr<IUIAutomationElement> spMinuteList;
            wrl::ComPtr<IUIAutomationElement> spAMPMList;
            wrl::ComPtr<IUIAutomationElement> spAcceptButton;
            wrl::ComPtr<IUIAutomationElement> spDismissButton;
            wrl::ComPtr<IUIAutomationElement> spNull;
            wrl::ComPtr<IUIAutomationElement> spListItem;
            wrl::ComPtr<IUIAutomationElement> spNextListItem;
            Common::AutoVariant autoVar;

            auto spAutomationClientManager =
                PopupHelper::AreWindowedPopupsEnabled() ?
                AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, timePicker) :
                AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spRoot);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"TimePickerFlyoutAutomationIntegrationTests::VerifyCanNavigateThroughAllItems: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"TimePickerFlyoutAutomationIntegrationTests::VerifyCanNavigateThroughAllItems: Failed in creating TreeWalker.");

            VERIFY_IS_NOT_NULL(spRoot);
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spRoot.Get(), &spHourList));

            VERIFY_SUCCEEDED(spHourList->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL(wcscmp(L"hour", (autoVar.Storage())->bstrVal), 0);
            VERIFY_SUCCEEDED(spHourList->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL((autoVar.Storage())->lVal, UIA_ListControlTypeId);

            // Loop through hour items
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spHourList.Get(), &spListItem));
            for (int i = 0; i < 12; i++)
            {
                VERIFY_IS_NOT_NULL(spListItem);

                VERIFY_SUCCEEDED(spListItem->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf()));
                wchar_t hourString[] = L"12";
                if (i > 0)
                {
                    VERIFY_ARE_EQUAL(_itow_s(i, hourString, 10), 0);
                }
                VERIFY_ARE_EQUAL(wcscmp(hourString, (autoVar.Storage())->bstrVal), 0);

                VERIFY_SUCCEEDED(spListItem->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL((autoVar.Storage())->lVal, UIA_ListItemControlTypeId);

                spNextListItem = nullptr;
                VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spListItem.Get(), &spNextListItem));
                spListItem = spNextListItem;
            }

            // Move to minute list
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spHourList.Get(), &spMinuteList));
            VERIFY_IS_NOT_NULL(spMinuteList);
            VERIFY_SUCCEEDED(spMinuteList->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL(wcscmp(L"minute", (autoVar.Storage())->bstrVal), 0);
            VERIFY_SUCCEEDED(spMinuteList->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL((autoVar.Storage())->lVal, UIA_ListControlTypeId);

            // Loop through minute items
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spMinuteList.Get(), &spListItem));
            for (int i = 0; i < 60; i++)
            {
                VERIFY_IS_NOT_NULL(spListItem);

                VERIFY_SUCCEEDED(spListItem->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf()));
                wchar_t minuteString[] = L"00";
                VERIFY_ARE_EQUAL(_itow_s(i, i >= 10 ? minuteString : &minuteString[1], 3, 10), 0);
                VERIFY_ARE_EQUAL(wcscmp(minuteString, (autoVar.Storage())->bstrVal), 0);

                VERIFY_SUCCEEDED(spListItem->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL((autoVar.Storage())->lVal, UIA_ListItemControlTypeId);

                spNextListItem = nullptr;
                VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spListItem.Get(), &spNextListItem));
                spListItem = spNextListItem;
            }

            // Move to AM/PM list
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spMinuteList.Get(), &spAMPMList));
            VERIFY_IS_NOT_NULL(spAMPMList);
            VERIFY_SUCCEEDED(spAMPMList->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL(wcscmp(L"period", (autoVar.Storage())->bstrVal), 0);
            VERIFY_SUCCEEDED(spAMPMList->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL((autoVar.Storage())->lVal, UIA_ListControlTypeId);

            // Loop through AM/PM items
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spAMPMList.Get(), &spListItem));
            for (int i = 0; i < 2; i++)
            {
                VERIFY_IS_NOT_NULL(spListItem);

                VERIFY_SUCCEEDED(spListItem->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL(wcscmp(i == 0 ? L"AM" : L"PM", (autoVar.Storage())->bstrVal), 0);

                VERIFY_SUCCEEDED(spListItem->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf()));
                VERIFY_ARE_EQUAL((autoVar.Storage())->lVal, UIA_ListItemControlTypeId);

                spNextListItem = nullptr;
                VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spListItem.Get(), &spNextListItem));
                spListItem = spNextListItem;
            }

            // Move to Accept button
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spAMPMList.Get(), &spAcceptButton));
            VERIFY_IS_NOT_NULL(spAcceptButton);
            VERIFY_SUCCEEDED(spAcceptButton->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL(wcscmp(L"Accept", (autoVar.Storage())->bstrVal), 0);
            VERIFY_SUCCEEDED(spAcceptButton->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL((autoVar.Storage())->lVal, UIA_ButtonControlTypeId);

            // Move to Dismiss button
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spAcceptButton.Get(), &spDismissButton));
            VERIFY_IS_NOT_NULL(spDismissButton);
            VERIFY_SUCCEEDED(spDismissButton->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL(wcscmp(L"Dismiss", (autoVar.Storage())->bstrVal), 0);
            VERIFY_SUCCEEDED(spDismissButton->GetCurrentPropertyValue(UIA_ControlTypePropertyId, autoVar.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL((autoVar.Storage())->lVal, UIA_ButtonControlTypeId);

            // Verify no next sibling
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spDismissButton.Get(), &spNull));
            VERIFY_IS_NULL(spNull);
        });
    }

    void TimePickerFlyoutAutomationIntegrationTests::VerifyCanSelectItems()
    {

        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"timepicker";
        xaml_controls::TimePicker^ timePicker = SetupTimePickerTest();

        auto timeChangedEvent = std::make_shared<Event>();
        auto timeChangedRegistration = CreateSafeEventRegistration(xaml_controls::TimePicker, TimeChanged);
        RunOnUIThread([&]()
        {
            timeChangedRegistration.Attach(
                timePicker,
                ref new wf::EventHandler<xaml_controls::TimePickerValueChangedEventArgs^>(
                    [timeChangedEvent](Platform::Object^ sender, xaml_controls::TimePickerValueChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"VerifyCanSelectItems: TimePickerValueChanged event fired.");
                timeChangedEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        wrl::ComPtr<IUIAutomationElement> spAMPMList;
        wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(timePicker);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            wrl::ComPtr<IUIAutomationElement> spRoot;
            wrl::ComPtr<IUIAutomationElement> spHourList;
            wrl::ComPtr<IUIAutomationElement> spMinuteList;
            wrl::ComPtr<IUIAutomationElement> spListItem;
            wrl::ComPtr<IUIAutomationElement> spNextListItem;
            wrl::ComPtr<IUIAutomationSelectionItemPattern> spSelectionItemPattern;

            auto spAutomationClientManager =
                PopupHelper::AreWindowedPopupsEnabled() ?
                AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, timePicker) :
                AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spRoot);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"TimePickerFlyoutAutomationIntegrationTests::VerifyCanSelectItems: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"TimePickerFlyoutAutomationIntegrationTests::VerifyCanSelectItems: Failed in creating TreeWalker.");

            VERIFY_IS_NOT_NULL(spRoot);
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spRoot.Get(), &spHourList));

            // Select hour '5'
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spHourList.Get(), &spListItem));
            for (int i = 0; i < 5; i++)
            {
                VERIFY_IS_NOT_NULL(spListItem);
                spNextListItem = nullptr;
                VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spListItem.Get(), &spNextListItem));
                spListItem = spNextListItem;
            }
            VERIFY_SUCCEEDED(spListItem->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &spSelectionItemPattern));
            VERIFY_IS_NOT_NULL(spSelectionItemPattern);
            VERIFY_SUCCEEDED(spSelectionItemPattern->Select());
            spListItem = nullptr;

            // Select minute '30' (go far enough to an item that is not initially realized)
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spHourList.Get(), &spMinuteList));
            VERIFY_IS_NOT_NULL(spMinuteList);
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spMinuteList.Get(), &spListItem));
            for (int i = 0; i < 30; i++)
            {
                VERIFY_IS_NOT_NULL(spListItem);
                spNextListItem = nullptr;
                VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spListItem.Get(), &spNextListItem));
                spListItem = spNextListItem;
            }
            VERIFY_SUCCEEDED(spListItem->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &spSelectionItemPattern));
            VERIFY_IS_NOT_NULL(spSelectionItemPattern);
            VERIFY_SUCCEEDED(spSelectionItemPattern->Select());
            spListItem = nullptr;

            // Select 'PM'
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spMinuteList.Get(), &spAMPMList));
            VERIFY_IS_NOT_NULL(spAMPMList);
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spAMPMList.Get(), &spListItem));
            VERIFY_IS_NOT_NULL(spListItem);
            spNextListItem = nullptr;
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spListItem.Get(), &spNextListItem));
            spListItem = spNextListItem;
            VERIFY_SUCCEEDED(spListItem->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &spSelectionItemPattern));
            VERIFY_IS_NOT_NULL(spSelectionItemPattern);
            VERIFY_SUCCEEDED(spSelectionItemPattern->Select());
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spAcceptButton;
            wrl::ComPtr<IUIAutomationInvokePattern> spInvokePattern;

            // Invoke the Accept button with UIA
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spAMPMList.Get(), &spAcceptButton));
            VERIFY_IS_NOT_NULL(spAcceptButton);
            VERIFY_SUCCEEDED(spAcceptButton->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern), &spInvokePattern));
            VERIFY_SUCCEEDED(spInvokePattern->Invoke());
        });

        timeChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            int64 fiveThirtyPM = (int64)10000000 * (17 * 60 + 30) * 60;
            VERIFY_ARE_EQUAL(timePicker->Time.Duration, fiveThirtyPM);
        });
    }

    void TimePickerFlyoutAutomationIntegrationTests::VerifySelectedItemInScrolled()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"timepicker";
        xaml_controls::TimePicker^ timePicker = SetupTimePickerTest();

        wrl::ComPtr<IUIAutomationElement> spHourListItem;
        wrl::ComPtr<IUIAutomationElement> spMinuteListItem;
        wrl::ComPtr<IUIAutomationElement> spHourList;
        wrl::ComPtr<IUIAutomationElement> spMinuteList;

        if (PopupHelper::AreWindowedPopupsEnabled())
        {
            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForOpenWindowedPopup(timePicker);
        }

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomationElement> spRoot;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            wrl::ComPtr<IUIAutomationElement> spNextListItem;
            wrl::ComPtr<IUIAutomationSelectionItemPattern> spSelectionItemPattern;

            auto spAutomationClientManager =
                PopupHelper::AreWindowedPopupsEnabled() ?
                AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromOpenPopup(uiaInfo, timePicker) :
                AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spRoot);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"TimePickerFlyoutAutomationIntegrationTests::VerifySelectedItemInScrolled: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"TimePickerFlyoutAutomationIntegrationTests::VerifySelectedItemInScrolled: Failed in creating TreeWalker.");

            VERIFY_IS_NOT_NULL(spRoot);
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spRoot.Get(), &spHourList));

            // Select hour '12'
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spHourList.Get(), &spHourListItem));
            VERIFY_SUCCEEDED(spHourListItem->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &spSelectionItemPattern));
            VERIFY_IS_NOT_NULL(spSelectionItemPattern);
            VERIFY_SUCCEEDED(spSelectionItemPattern->Select());

            // Select minute '30' (go far enough to an item that is not initially realized)
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spHourList.Get(), &spMinuteList));
            VERIFY_IS_NOT_NULL(spMinuteList);
            VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetFirstChildElement(spMinuteList.Get(), &spMinuteListItem));
            for (int i = 0; i < 30; i++)
            {
                VERIFY_IS_NOT_NULL(spMinuteListItem);
                spNextListItem = nullptr;
                VERIFY_SUCCEEDED(spUIAutomationTreeWalker->GetNextSiblingElement(spMinuteListItem.Get(), &spNextListItem));
                spMinuteListItem = spNextListItem;
            }
            VERIFY_SUCCEEDED(spMinuteListItem->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &spSelectionItemPattern));
            VERIFY_IS_NOT_NULL(spSelectionItemPattern);
            VERIFY_SUCCEEDED(spSelectionItemPattern->Select());
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            RECT hourListBoundingRect;
            RECT minuteListBoundingRect;
            RECT hourListItemBoundingRect;
            RECT minuteListItemBoundingRect;

            VERIFY_SUCCEEDED(spHourList->get_CurrentBoundingRectangle(&hourListBoundingRect));
            VERIFY_SUCCEEDED(spMinuteList->get_CurrentBoundingRectangle(&minuteListBoundingRect));
            VERIFY_SUCCEEDED(spHourListItem->get_CurrentBoundingRectangle(&hourListItemBoundingRect));
            VERIFY_SUCCEEDED(spMinuteList->get_CurrentBoundingRectangle(&minuteListItemBoundingRect));

            // Check the the item's bounding rects are entirely contained within their respective list's
            VERIFY_IS_TRUE(hourListBoundingRect.left >= hourListBoundingRect.left && hourListBoundingRect.top >= hourListBoundingRect.top &&
                hourListBoundingRect.right <= hourListBoundingRect.right && hourListBoundingRect.bottom <= hourListBoundingRect.bottom);
            VERIFY_IS_TRUE(minuteListItemBoundingRect.left >= minuteListBoundingRect.left && minuteListItemBoundingRect.top >= minuteListBoundingRect.top &&
                minuteListItemBoundingRect.right <= minuteListBoundingRect.right && minuteListItemBoundingRect.bottom <= minuteListBoundingRect.bottom);
        });
    }

}

} } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
