// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <collection.h>
#include <TestEvent.h>
#include <CalendarViewAutomationPeerIntegrationTests.h>
#include <SafeEventRegistration.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <TestCleanupWrapper.h>
#include <Patterns\SelectionItemPatternHandler.h>
#include "CalendarHelper.h"
#include <UIAutomationHelper.h>
#include <TreeHelper.h>

using namespace std;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Common::CalendarHelper;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace local {
    // PeerFromProvider is protected even though conceptually it's a static method.
    // BridgeAutomationPeer is a little hack to allow us to call PeerFromProvider in our tests.
    ref class BridgeAutomationPeer : public xaml_automation_peers::AutomationPeer
    {
    public:
        xaml_automation_peers::AutomationPeer^ CallPeerFromProvider(xaml_automation::Provider::IRawElementProviderSimple^ provider)
        {
            return PeerFromProvider(provider);
        }
    };
}


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace CalendarView {

    bool CalendarViewAutomationPeerIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool CalendarViewAutomationPeerIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void CalendarViewAutomationPeerIntegrationTests::VerifyUIATree()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarViewHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarView^ cv = helper.GetCalendarView();

        wrl::ComPtr<AutomationClient::AutomationPropertyChangedHandler<3>> spAutomationPropertyChangedEventHandler;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"CalendarView";
        uiaInfo.m_AutomationID = L"CalendarView";
        uiaInfo.m_cType = UIA_CalendarControlTypeId;

        wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
        wrl::ComPtr<IUIAutomation> spAutomation;
        wrl::ComPtr<IUIAutomationElement> spAutomationHeaderElement;
        wrl::ComPtr<IUIAutomationInvokePattern> spInvokePattern;

        CreateTestResources(rootPanel);

        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2000, 11, 15);
            cv->MaxDate = ConvertToDateTime(1, 2001, 1, 15);
            xaml_automation::AutomationProperties::SetName(cv, ref new Platform::String(uiaInfo.m_Name));
            rootPanel->Children->Append(cv);
        });

        helper.WaitForLoaded();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            spAutomationClientManager->GetAutomation(&spAutomation);
            VERIFY_IS_NOT_NULL(spAutomation.Get());

            wrl::ComPtr<IUIAutomationCondition> spUIAutomationMonthViewSVCondition;
            Common::AutoVariant autoMonthViewSV;
            autoMonthViewSV.SetString(L"MonthViewScrollViewer");
            VERIFY_SUCCEEDED(spAutomation->CreatePropertyCondition(UIA_AutomationIdPropertyId, *(autoMonthViewSV.Storage()), &spUIAutomationMonthViewSVCondition));

            wrl::ComPtr<IUIAutomationElement> spAutomationMonthViewSVElement;
            VERIFY_SUCCEEDED(spUIAutomationElement->FindFirst(TreeScope::TreeScope_Children, spUIAutomationMonthViewSVCondition.Get(), &spAutomationMonthViewSVElement));
            VERIFY_IS_NOT_NULL(spAutomationMonthViewSVElement);

            wrl::ComPtr<IUIAutomationCondition> spUIAutomationDatesCondition;
            Common::AutoVariant autoVarType;
            autoVarType.SetInt(UIA_DataItemControlTypeId);
            VERIFY_SUCCEEDED(spAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, *(autoVarType.Storage()), &spUIAutomationDatesCondition));
            wrl::ComPtr<IUIAutomationElementArray> spDateItems;
            VERIFY_SUCCEEDED(spAutomationMonthViewSVElement->FindAll(TreeScope::TreeScope_Children, spUIAutomationDatesCondition.Get(), &spDateItems));
            int size = 0;
            VERIFY_SUCCEEDED(spDateItems->get_Length(&size));
            WEX::Logging::Log::Comment(L"Visible Dates are less than or equal to 42");
            VERIFY_IS_LESS_THAN_OR_EQUAL(size, 42);

            wrl::ComPtr<IUIAutomationCondition> spUIAutomationAutomationIDCondition;
            Common::AutoVariant autoHeaderButton;
            autoHeaderButton.SetString(L"HeaderButton");
            VERIFY_SUCCEEDED(spAutomation->CreatePropertyCondition(UIA_AutomationIdPropertyId, *(autoHeaderButton.Storage()), &spUIAutomationAutomationIDCondition));

            VERIFY_SUCCEEDED(spUIAutomationElement->FindFirst(TreeScope::TreeScope_Children, spUIAutomationAutomationIDCondition.Get(), &spAutomationHeaderElement));
            VERIFY_IS_NOT_NULL(spAutomationHeaderElement);

            VERIFY_SUCCEEDED(spAutomationHeaderElement->GetCurrentPatternAs(UIA_InvokePatternId, __uuidof(IUIAutomationInvokePattern), &spInvokePattern));
            VERIFY_IS_NOT_NULL(spInvokePattern);

            VERIFY_SUCCEEDED(spInvokePattern->Invoke());
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationYearViewSVCondition;
            Common::AutoVariant autoYearViewSV;
            autoYearViewSV.SetString(L"YearViewScrollViewer");
            VERIFY_SUCCEEDED(spAutomation->CreatePropertyCondition(UIA_AutomationIdPropertyId, *(autoYearViewSV.Storage()), &spUIAutomationYearViewSVCondition));

            wrl::ComPtr<IUIAutomationElement> spAutomationYearViewSVElement;
            VERIFY_SUCCEEDED(spUIAutomationElement->FindFirst(TreeScope::TreeScope_Children, spUIAutomationYearViewSVCondition.Get(), &spAutomationYearViewSVElement));
            VERIFY_IS_NOT_NULL(spAutomationYearViewSVElement);

            wrl::ComPtr<IUIAutomationCondition> spUIAutomationDatesCondition;
            Common::AutoVariant autoVarType;
            autoVarType.SetInt(UIA_ButtonControlTypeId);
            VERIFY_SUCCEEDED(spAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, *(autoVarType.Storage()), &spUIAutomationDatesCondition));
            wrl::ComPtr<IUIAutomationElementArray> spDateItems;
            VERIFY_SUCCEEDED(spAutomationYearViewSVElement->FindAll(TreeScope::TreeScope_Children, spUIAutomationDatesCondition.Get(), &spDateItems));
            int size = 0;
            VERIFY_SUCCEEDED(spDateItems->get_Length(&size));
            WEX::Logging::Log::Comment(L"There are 3 month in the view between 2000-11-15 and 2001-1-15");
            VERIFY_ARE_EQUAL(size, 3);

            VERIFY_SUCCEEDED(spInvokePattern->Invoke());
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationDecadeViewSVCondition;
            Common::AutoVariant autoDecadeViewSV;
            autoDecadeViewSV.SetString(L"DecadeViewScrollViewer");
            VERIFY_SUCCEEDED(spAutomation->CreatePropertyCondition(UIA_AutomationIdPropertyId, *(autoDecadeViewSV.Storage()), &spUIAutomationDecadeViewSVCondition));

            wrl::ComPtr<IUIAutomationElement> spAutomationDecadeViewSVElement;
            VERIFY_SUCCEEDED(spUIAutomationElement->FindFirst(TreeScope::TreeScope_Children, spUIAutomationDecadeViewSVCondition.Get(), &spAutomationDecadeViewSVElement));
            VERIFY_IS_NOT_NULL(spAutomationDecadeViewSVElement);

            wrl::ComPtr<IUIAutomationCondition> spUIAutomationDatesCondition;
            Common::AutoVariant autoVarType;
            autoVarType.SetInt(UIA_ButtonControlTypeId);
            VERIFY_SUCCEEDED(spAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, *(autoVarType.Storage()), &spUIAutomationDatesCondition));
            wrl::ComPtr<IUIAutomationElementArray> spDateItems;
            VERIFY_SUCCEEDED(spAutomationDecadeViewSVElement->FindAll(TreeScope::TreeScope_Children, spUIAutomationDatesCondition.Get(), &spDateItems));
            int size = 0;
            VERIFY_SUCCEEDED(spDateItems->get_Length(&size));
            WEX::Logging::Log::Comment(L"There are 2 years in the view between 2000-11-15 and 2001-1-15");
            VERIFY_ARE_EQUAL(size, 2);
        });
    }

    void CalendarViewAutomationPeerIntegrationTests::VerifyElementPatterns()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarViewHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarView^ cv = helper.GetCalendarView();

        wrl::ComPtr<AutomationClient::AutomationPropertyChangedHandler<3>> spAutomationPropertyChangedEventHandler;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"CalendarView";
        uiaInfo.m_AutomationID = L"CalendarView";
        uiaInfo.m_cType = UIA_CalendarControlTypeId;

        wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
        wrl::ComPtr<IUIAutomation> spAutomation;
        wrl::ComPtr<IUIAutomationElement> spAutomationHeaderElement;
        wrl::ComPtr<IUIAutomationSelectionItemPattern> spSelectionItemPattern;
        ::Windows::Foundation::DateTime testdate = ConvertToDateTime(1, 2000, 11, 15);
        CreateTestResources(rootPanel);

        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2000, 11, 15);
            cv->MaxDate = ConvertToDateTime(1, 2001, 1, 15);
            xaml_automation::AutomationProperties::SetName(cv, ref new Platform::String(uiaInfo.m_Name));
            rootPanel->Children->Append(cv);
        });

        helper.WaitForLoaded();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            spAutomationClientManager->GetAutomation(&spAutomation);
            VERIFY_IS_NOT_NULL(spAutomation.Get());
        });

        RunOnUIThread([&]()
        {
            cv->SetDisplayDate(ConvertToDateTime(1, 2000, 11, 15));
            cv->UpdateLayout();
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationMonthViewSVCondition;
            Common::AutoVariant autoMonthViewSV;
            autoMonthViewSV.SetString(L"MonthViewScrollViewer");
            VERIFY_SUCCEEDED(spAutomation->CreatePropertyCondition(UIA_AutomationIdPropertyId, *(autoMonthViewSV.Storage()), &spUIAutomationMonthViewSVCondition));

            wrl::ComPtr<IUIAutomationElement> spAutomationMonthViewSVElement;
            VERIFY_SUCCEEDED(spUIAutomationElement->FindFirst(TreeScope::TreeScope_Children, spUIAutomationMonthViewSVCondition.Get(), &spAutomationMonthViewSVElement));
            VERIFY_IS_NOT_NULL(spAutomationMonthViewSVElement);

            wrl::ComPtr<IUIAutomationCondition> spUIAutomationDatesCondition;
            Common::AutoVariant autoVarType;
            autoVarType.SetInt(UIA_DataItemControlTypeId);
            VERIFY_SUCCEEDED(spAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, *(autoVarType.Storage()), &spUIAutomationDatesCondition));
            wrl::ComPtr<IUIAutomationElement> spAutomationFirstElement;
            VERIFY_SUCCEEDED(spAutomationMonthViewSVElement->FindFirst(TreeScope::TreeScope_Children, spUIAutomationDatesCondition.Get(), &spAutomationFirstElement));

            wrl::ComPtr<IUIAutomationGridItemPattern> spGridItemPattern;
            VERIFY_SUCCEEDED(spAutomationFirstElement->GetCurrentPatternAs(UIA_GridItemPatternId, __uuidof(IUIAutomationGridItemPattern), &spGridItemPattern));
            VERIFY_IS_NOT_NULL(spGridItemPattern);

            int col = 0, row = 0;
            VERIFY_SUCCEEDED(spGridItemPattern->get_CurrentColumn(&col));
            WEX::Logging::Log::Comment(L"2000-11-15 is on Wednesday");
            VERIFY_ARE_EQUAL(col, 3);
            VERIFY_SUCCEEDED(spGridItemPattern->get_CurrentRow(&row));
            VERIFY_ARE_EQUAL(row, 0);

            wrl::ComPtr<IUIAutomationTableItemPattern> spTableItemPattern;
            VERIFY_SUCCEEDED(spAutomationFirstElement->GetCurrentPatternAs(UIA_TableItemPatternId, __uuidof(IUIAutomationTableItemPattern), &spTableItemPattern));
            VERIFY_IS_NOT_NULL(spTableItemPattern);

            wrl::ComPtr<IUIAutomationElementArray> spRowHeaderItems;
            VERIFY_SUCCEEDED(spTableItemPattern->GetCurrentRowHeaderItems(&spRowHeaderItems));

            int rowSize = 0;
            VERIFY_SUCCEEDED(spRowHeaderItems->get_Length(&rowSize));
            VERIFY_ARE_EQUAL(rowSize, 1);

            wrl::ComPtr<IUIAutomationElement> spRowHeaderAutomationElement;
            VERIFY_SUCCEEDED(spRowHeaderItems->GetElement(0, &spRowHeaderAutomationElement));
            VERIFY_IS_NOT_NULL(spRowHeaderAutomationElement);

            AutoBSTR rowHeaderElementName;
            VERIFY_SUCCEEDED(spRowHeaderAutomationElement->get_CurrentName(rowHeaderElementName.ReleaseAndGetAddressOf()));
            Common::AutoBSTR::VerifyAreEqual(L"\u200eNovember\u200e \u200e2000", rowHeaderElementName);

            wrl::ComPtr<IUIAutomationElementArray> spHeaderItems;
            VERIFY_SUCCEEDED(spTableItemPattern->GetCurrentColumnHeaderItems(&spHeaderItems));

            int size = 0;
            VERIFY_SUCCEEDED(spHeaderItems->get_Length(&size));
            VERIFY_ARE_EQUAL(size, 1);

            wrl::ComPtr<IUIAutomationElement> spAutomationHeaderElement;
            VERIFY_SUCCEEDED(spHeaderItems->GetElement(0, &spAutomationHeaderElement));
            VERIFY_IS_NOT_NULL(spAutomationHeaderElement);

            AutoBSTR elementName;
            VERIFY_SUCCEEDED(spAutomationHeaderElement->get_CurrentName(elementName.ReleaseAndGetAddressOf()));
            WEX::Logging::Log::Comment(L"2000-11-15 is on Wednesday");
            VERIFY_ARE_EQUAL(wcscmp(elementName, L"Wednesday"), 0);

            VERIFY_SUCCEEDED(spAutomationFirstElement->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &spSelectionItemPattern));
            VERIFY_IS_NOT_NULL(spSelectionItemPattern);

            BOOL isSelected = TRUE;
            VERIFY_SUCCEEDED(spSelectionItemPattern->get_CurrentIsSelected(&isSelected));
            VERIFY_IS_FALSE(!!isSelected);
        });

        helper.PrepareSelectedDatesChangedEvent();
        helper.ExpectAddedDate(testdate);
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VERIFY_SUCCEEDED(spSelectionItemPattern->Select());
        });
        helper.WaitForSelectedDatesChanged();


        helper.PrepareSelectedDatesChangedEvent();
        helper.ExpectRemovedDate(testdate);
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            VERIFY_SUCCEEDED(spSelectionItemPattern->RemoveFromSelection());
        });
        helper.WaitForSelectedDatesChanged();

        RunOnUIThread([&]()
        {
            cv->SelectedDates->Append(testdate);
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            BOOL isSelected = TRUE;
            VERIFY_SUCCEEDED(spSelectionItemPattern->get_CurrentIsSelected(&isSelected));
            VERIFY_IS_TRUE(!!isSelected);
        });
    }


    void CalendarViewAutomationPeerIntegrationTests::VerifySelectionChangedEvent()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;

        CalendarViewHelper helper;
        helper.PrepareLoadedEvent();
        Microsoft::UI::Xaml::Controls::CalendarView^ cv = helper.GetCalendarView();

        wrl::ComPtr<AutomationClient::AutomationPropertyChangedHandler<3>> spAutomationPropertyChangedEventHandler;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"CalendarView";
        uiaInfo.m_AutomationID = L"CalendarView";
        uiaInfo.m_cType = UIA_CalendarControlTypeId;

        wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
        wrl::ComPtr<IUIAutomation> spAutomation;
        wrl::ComPtr<IUIAutomationElement> spAutomationHeaderElement;
        wrl::ComPtr<IUIAutomationSelectionItemPattern> spSelectionItemPattern;
        CreateTestResources(rootPanel);

        RunOnUIThread([&]()
        {
            cv->MinDate = ConvertToDateTime(1, 2000, 11, 15);
            cv->MaxDate = ConvertToDateTime(1, 2001, 1, 15);
            xaml_automation::AutomationProperties::SetName(cv, ref new Platform::String(uiaInfo.m_Name));
            rootPanel->Children->Append(cv);
        });

        helper.WaitForLoaded();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            cv->SetDisplayDate(ConvertToDateTime(1, 2000, 11, 15));
            cv->UpdateLayout();
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            spAutomationClientManager->GetAutomation(&spAutomation);
            VERIFY_IS_NOT_NULL(spAutomation.Get());

            wrl::ComPtr<IUIAutomationCondition> spUIAutomationMonthViewSVCondition;
            Common::AutoVariant autoMonthViewSV;
            autoMonthViewSV.SetString(L"MonthViewScrollViewer");
            VERIFY_SUCCEEDED(spAutomation->CreatePropertyCondition(UIA_AutomationIdPropertyId, *(autoMonthViewSV.Storage()), &spUIAutomationMonthViewSVCondition));

            wrl::ComPtr<IUIAutomationElement> spAutomationMonthViewSVElement;
            VERIFY_SUCCEEDED(spUIAutomationElement->FindFirst(TreeScope::TreeScope_Children, spUIAutomationMonthViewSVCondition.Get(), &spAutomationMonthViewSVElement));
            VERIFY_IS_NOT_NULL(spAutomationMonthViewSVElement);

            wrl::ComPtr<IUIAutomationCondition> spUIAutomationDatesCondition;
            Common::AutoVariant autoVarType;
            autoVarType.SetInt(UIA_DataItemControlTypeId);
            VERIFY_SUCCEEDED(spAutomation->CreatePropertyCondition(UIA_ControlTypePropertyId, *(autoVarType.Storage()), &spUIAutomationDatesCondition));
            wrl::ComPtr<IUIAutomationElement> spAutomationFirstElement;
            VERIFY_SUCCEEDED(spAutomationMonthViewSVElement->FindFirst(TreeScope::TreeScope_Children, spUIAutomationDatesCondition.Get(), &spAutomationFirstElement));

            wrl::ComPtr<IUIAutomationSelectionItemPattern> spSelectionItemPattern;
            VERIFY_SUCCEEDED(spAutomationFirstElement->GetCurrentPatternAs(UIA_SelectionItemPatternId, __uuidof(IUIAutomationSelectionItemPattern), &spSelectionItemPattern));
            VERIFY_IS_NOT_NULL(spSelectionItemPattern);

            RunOnUIThread([&]()
            {
                cv->SelectionMode = Microsoft::UI::Xaml::Controls::CalendarViewSelectionMode::Multiple;
            });
            auto spEvent = std::make_shared<Event>();
            wrl::ComPtr<Patterns::SelectionItemPatternHandler> spAutomationPatternHandler;

            spAutomationPatternHandler.Attach(new Patterns::SelectionItemPatternHandler(spAutomationClientManager, spEvent, TreeScope_Subtree, UIA_SelectionItem_ElementSelectedEventId));
            spAutomationPatternHandler->AttachEventHandler();
            ::Windows::Foundation::DateTime testdate = ConvertToDateTime(1, 2000, 11, 15);
            RunOnUIThread([&]()
            {
                WEX::Logging::Log::Comment(L"Selecting Date 2000-11-15");
                cv->SelectedDates->Append(testdate);
            });
            spAutomationPatternHandler->ConfirmAndUnregister();
            WEX::Logging::Log::Comment(L"UIA_SelectionItem_ElementSelectedEventId Recieved");

            spAutomationPatternHandler = nullptr;

            spAutomationPatternHandler.Attach(new Patterns::SelectionItemPatternHandler(spAutomationClientManager, spEvent, TreeScope_Subtree, UIA_SelectionItem_ElementAddedToSelectionEventId));
            spAutomationPatternHandler->AttachEventHandler();
            RunOnUIThread([&]()
            {
                testdate = ConvertToDateTime(1, 2000, 11, 16);
                WEX::Logging::Log::Comment(L"Selecting Date 2000-11-16");
                cv->SelectedDates->Append(testdate);
            });
            spAutomationPatternHandler->ConfirmAndUnregister();
            WEX::Logging::Log::Comment(L"UIA_SelectionItem_ElementAddedToSelectionEventId Recieved");

            spAutomationPatternHandler = nullptr;

            spAutomationPatternHandler.Attach(new Patterns::SelectionItemPatternHandler(spAutomationClientManager, spEvent, TreeScope_Subtree, UIA_SelectionItem_ElementRemovedFromSelectionEventId));
            spAutomationPatternHandler->AttachEventHandler();
            RunOnUIThread([&]()
            {
                WEX::Logging::Log::Comment(L"Removed one selection");
                cv->SelectedDates->RemoveAt(0);
            });
            spAutomationPatternHandler->ConfirmAndUnregister();
            WEX::Logging::Log::Comment(L"UIA_SelectionItem_ElementRemovedFromSelectionEventId Recieved");


            spAutomationPatternHandler = nullptr;

            spAutomationPatternHandler.Attach(new Patterns::SelectionItemPatternHandler(spAutomationClientManager, spEvent, TreeScope_Subtree, UIA_SelectionItem_ElementRemovedFromSelectionEventId));
            spAutomationPatternHandler->AttachEventHandler();
            RunOnUIThread([&]()
            {
                WEX::Logging::Log::Comment(L"Removed last selection");
                cv->SelectedDates->RemoveAt(0);
            });
            spAutomationPatternHandler->ConfirmAndUnregister();
            WEX::Logging::Log::Comment(L"UIA_SelectionItem_ElementRemovedFromSelectionEventId Recieved");
        });
    }

    void CalendarViewAutomationPeerIntegrationTests::VerifyDayItemRowHeaders()
    {
        TestCleanupWrapper cleanup;
        Microsoft::UI::Xaml::Controls::CalendarView^ calendar;

        RunOnUIThread([&]()
        {
            calendar = ref new Microsoft::UI::Xaml::Controls::CalendarView();
            calendar->SetDisplayDate(ConvertToDateTime(1, 2016, 04, 01));
            TestServices::WindowHelper->WindowContent = calendar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto dayItems = ref new Platform::Collections::Vector<xaml_controls::CalendarViewDayItem^>();
            TreeHelper::GetVisualChildrenByType(calendar, dayItems);

            // So since there is no mapping APIs on CalendarView, to get to the month in view, we divide by 2
            // To get to the previous/next months, we remove/add 30 days.
            auto marchDay = dayItems->GetAt(static_cast<unsigned>(dayItems->Size * 0.5 - 30));
            auto aprilDay = dayItems->GetAt(static_cast<unsigned>(dayItems->Size * 0.5));
            auto mayDay = dayItems->GetAt(static_cast<unsigned>(dayItems->Size * 0.5 + 30));

            // We want to validate we get different peers for day items that belong to different months.
            auto marchDayPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(marchDay);
            auto aprilDayPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(aprilDay);
            auto mayDayPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(mayDay);

            auto marchRowHeaders = ((xaml_automation::Provider::ITableItemProvider^)marchDayPeer->GetPattern(xaml_automation_peers::PatternInterface::TableItem))->GetRowHeaderItems();
            auto aprilRowHeaders = ((xaml_automation::Provider::ITableItemProvider^)aprilDayPeer->GetPattern(xaml_automation_peers::PatternInterface::TableItem))->GetRowHeaderItems();
            auto mayRowHeaders = ((xaml_automation::Provider::ITableItemProvider^)mayDayPeer->GetPattern(xaml_automation_peers::PatternInterface::TableItem))->GetRowHeaderItems();

            VERIFY_ARE_EQUAL(1u, marchRowHeaders->Length);
            VERIFY_ARE_EQUAL(1u, aprilRowHeaders->Length);
            VERIFY_ARE_EQUAL(1u, mayRowHeaders->Length);

            auto bridge = ref new local::BridgeAutomationPeer();
            VERIFY_ARE_EQUAL(ref new Platform::String(L"\u200eMarch\u200e \u200e2016"), bridge->CallPeerFromProvider(marchRowHeaders[0])->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"\u200eApril\u200e \u200e2016"), bridge->CallPeerFromProvider(aprilRowHeaders[0])->GetName());
            VERIFY_ARE_EQUAL(ref new Platform::String(L"\u200eMay\u200e \u200e2016"), bridge->CallPeerFromProvider(mayRowHeaders[0])->GetName());
        });
    }

    void CalendarViewAutomationPeerIntegrationTests::VerifyDayItemAutomationName()
    {
        TestCleanupWrapper cleanup;
        Microsoft::UI::Xaml::Controls::CalendarView^ calendar;

        RunOnUIThread([&]()
        {
            calendar = ref new Microsoft::UI::Xaml::Controls::CalendarView();
            TestServices::WindowHelper->WindowContent = calendar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto dayItems = ref new Platform::Collections::Vector<xaml_controls::CalendarViewDayItem^>();
            TreeHelper::GetVisualChildrenByType(calendar, dayItems);

            int todayCount = 0;

            // Iterate through a couple of months centered around the middle of the dayItems array.
            for (unsigned dayIndex = max(0, dayItems->Size / 2 - 30); dayIndex < min(dayItems->Size, dayItems->Size / 2 + 30); dayIndex++)
            {
                auto dayItem = dayItems->GetAt(dayIndex);
                auto dayItemPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(dayItem);
                auto dayAutomationName = dayItemPeer->GetName();
                auto dayAutomationNameLength = dayAutomationName->Length();
                LOG_OUTPUT(L"dayAutomationName: %s, %d", dayAutomationName->Data(), dayAutomationNameLength);
                if (dayAutomationNameLength > 2)
                {
                    VERIFY_IS_TRUE(8 == dayAutomationNameLength || 9 == dayAutomationNameLength);
                    todayCount++;
                }
            }

            // Exactly one day is expected to have an automation name like "XY, today". All others are like "XY".
            VERIFY_ARE_EQUAL(1, todayCount);
        });
    }

    void CalendarViewAutomationPeerIntegrationTests::VerifyOutOfViewItemsDoNotSupportGridItemPattern()
    {
        TestCleanupWrapper cleanup;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"CalendarView";
        uiaInfo.m_AutomationID = L"CalendarView";
        uiaInfo.m_cType = UIA_CalendarControlTypeId;

        xaml_controls::CalendarView^ calendarView = nullptr;

        RunOnUIThread([&]()
        {
            calendarView = ref new Microsoft::UI::Xaml::Controls::CalendarView();
            calendarView->SetDisplayDate(ConvertToDateTime(1, 2016, 04, 01));

            xaml_automation::AutomationProperties::SetName(calendarView, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = calendarView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            calendarView->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Get the automation element corresponding to the first visible day item.");
        wrl::ComPtr<IUIAutomationElement> monthScrollViewerChildElement;
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> automationElement;

            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            automationClientManager->GetCurrentUIAutomationElement(&automationElement);

            wrl::ComPtr<IUIAutomation> automation;
            automationClientManager->GetAutomation(&automation);

            wrl::ComPtr<IUIAutomationCondition> monthViewScrollViewerCondition;
            Common::AutoVariant autoMonthViewSV;
            autoMonthViewSV.SetString(L"MonthViewScrollViewer");
            LogThrow_IfFailed(automation->CreatePropertyCondition(UIA_AutomationIdPropertyId, *(autoMonthViewSV.Storage()), &monthViewScrollViewerCondition));

            wrl::ComPtr<IUIAutomationElement> monthViewScrollViewerElement;
            LogThrow_IfFailed(automationElement->FindFirst(TreeScope::TreeScope_Children, monthViewScrollViewerCondition.Get(), &monthViewScrollViewerElement));

            wrl::ComPtr<IUIAutomationCondition> trueCondition;
            LogThrow_IfFailed(automation->CreateTrueCondition(&trueCondition));
            LogThrow_IfFailed(monthViewScrollViewerElement->FindFirst(TreeScope::TreeScope_Children, trueCondition.Get(), &monthScrollViewerChildElement));
        });

        LOG_OUTPUT(L"Tab past the month header button.");
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tab past the previous month button.");
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tab past the next month button.");
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Focus is now on a day item.  Press PageDown to change the month and move our previously queried automation element out of view.");
        TestServices::KeyboardHelper->PageDown();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Attempt to query for the grid item pattern on our previously queried automation element, which should return null.");
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationGridItemPattern> gridItemPattern;
            LogThrow_IfFailed(monthScrollViewerChildElement->GetCurrentPatternAs(UIA_GridItemPatternId, __uuidof(IUIAutomationGridItemPattern), &gridItemPattern));

            VERIFY_IS_NULL(gridItemPattern);
        });
    }

    void CalendarViewAutomationPeerIntegrationTests::VerifyAutomationNotificationEventAfterClickingNavigationButton()
    {
        TestCleanupWrapper cleanup;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"CalendarView";
        uiaInfo.m_AutomationID = L"CalendarView";
        uiaInfo.m_cType = UIA_CalendarControlTypeId;

        xaml_controls::CalendarView^ calendarView = nullptr;
        xaml_controls::Button^ previousButton = nullptr;
        xaml_controls::Button^ nextButton = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto previousButtonClickedEvent = std::make_shared<Event>();
        auto nextButtonClickedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::CalendarView, Loaded);
        auto previousButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        auto nextButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

        RunOnUIThread([&]()
        {
            calendarView = ref new Microsoft::UI::Xaml::Controls::CalendarView();
            calendarView->SetDisplayDate(ConvertToDateTime(1 /*era*/, 2016 /*year*/, 04 /*month*/, 01 /*day*/));

            loadedRegistration.Attach(calendarView, [&]()
            {
                LOG_OUTPUT(L"calendarView raised Loaded event.");
                loadedEvent->Set();
            });

            xaml_automation::AutomationProperties::SetName(calendarView, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = calendarView;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // find the template parts
        RunOnUIThread([&]()
        {
            previousButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(calendarView, L"PreviousButton"));
            nextButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(calendarView, L"NextButton"));

            previousButtonClickRegistration.Attach(previousButton, [&]()
            {
                LOG_OUTPUT(L"previousButton raised Click event.");
                previousButtonClickedEvent->Set(); 
            });

            nextButtonClickRegistration.Attach(nextButton, [&]()
            {
                LOG_OUTPUT(L"nextButton raised Click event.");
                nextButtonClickedEvent->Set();
            });
        });

        // How to use and init AutomationNotificationHandler, please refer to the source code of following two files:
        // dxaml/test/native/external/Automation/AutomationClient/AutomationEventHandler.h
        // dxaml/test/native/external/Automation/events/NotificationEventTests.cpp
        wrl::ComPtr<AutomationClient::AutomationNotificationHandler> notificationEventHandler;

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto notifyEvent = std::make_shared<Event>();
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            notificationEventHandler.Attach(new AutomationClient::AutomationNotificationHandler(automationClientManager, notifyEvent, TreeScope_Subtree));
            notificationEventHandler->Init(
                NotificationKind_ActionCompleted,
                NotificationProcessing_MostRecent,
                nullptr, // ignore expectedDisplayString
                L"CalenderViewNavigationButtonCompleted");
            notificationEventHandler->AttachEventHandler();
        });

        LOG_OUTPUT(L"Left-mouse-click on previousButton.");
        TestServices::InputHelper->LeftMouseClick(previousButton);
        previousButtonClickedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        // WaitForIdle is not reliable on this test case. and Confirm would block the UI thread. SynchronouslyTickUIThread to give more time to make animation complete.
        TestServices::WindowHelper->SynchronouslyTickUIThread(3);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            notificationEventHandler->Confirm();
        });

        LOG_OUTPUT(L"Left-mouse-click on nextButton.");
        TestServices::InputHelper->LeftMouseClick(nextButton);
        nextButtonClickedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::WindowHelper->SynchronouslyTickUIThread(3);

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            notificationEventHandler->ConfirmAndUnregister();
        });
    }
} } } } } }
