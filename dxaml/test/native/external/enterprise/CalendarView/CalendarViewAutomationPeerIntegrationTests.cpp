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

#undef GetClassName // Conflicts with automation peer method of the same name

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

    // The helpers below pin language, calendar and clock so expected dates and strings don't depend on the
    // machine's locale (CalendarView formats with its Language, CalendarIdentifier and a 24-hour clock).
    Platform::Collections::Vector<Platform::String^>^ GetEnUsLanguages()
    {
        auto languages = ref new Platform::Collections::Vector<Platform::String^>();
        languages->Append(L"en-US");
        return languages;
    }

    ::Windows::Globalization::Calendar^ CreateGregorianCalendar()
    {
        return ref new ::Windows::Globalization::Calendar(
            GetEnUsLanguages(),
            ::Windows::Globalization::CalendarIdentifiers::Gregorian,
            ::Windows::Globalization::ClockIdentifiers::TwentyFourHour);
    }

    ::Windows::Foundation::DateTime MakeGregorianDate(int year, int month, int day)
    {
        auto calendar = CreateGregorianCalendar();
        calendar->Day = 1; // keep the date valid while the year and month change
        calendar->Year = year;
        calendar->Month = month;
        calendar->Day = day;
        calendar->Hour = 12;
        calendar->Minute = 0;
        calendar->Second = 0;
        calendar->Nanosecond = 0;
        return calendar->GetDateTime();
    }

    bool IsSameGregorianDay(::Windows::Foundation::DateTime lhs, ::Windows::Foundation::DateTime rhs)
    {
        auto lhsCalendar = CreateGregorianCalendar();
        lhsCalendar->SetDateTime(lhs);
        auto rhsCalendar = CreateGregorianCalendar();
        rhsCalendar->SetDateTime(rhs);
        return lhsCalendar->Year == rhsCalendar->Year &&
            lhsCalendar->Month == rhsCalendar->Month &&
            lhsCalendar->Day == rhsCalendar->Day;
    }

    ::Windows::Globalization::DayOfWeek GetGregorianDayOfWeek(::Windows::Foundation::DateTime date)
    {
        auto calendar = CreateGregorianCalendar();
        calendar->SetDateTime(date);
        return calendar->DayOfWeek;
    }

    Platform::String^ FormatEnUsGregorian(Platform::String^ formatTemplate, ::Windows::Foundation::DateTime date)
    {
        auto formatter = ref new ::Windows::Globalization::DateTimeFormatting::DateTimeFormatter(
            formatTemplate,
            GetEnUsLanguages(),
            L"ZZ",
            ::Windows::Globalization::CalendarIdentifiers::Gregorian,
            ::Windows::Globalization::ClockIdentifiers::TwentyFourHour);
        return formatter->Format(date);
    }

    // Compares COM identity; different interface hats of the same object are not reference-equal.
    bool AreSameObject(Platform::Object^ lhs, Platform::Object^ rhs)
    {
        if (lhs == nullptr || rhs == nullptr)
        {
            return lhs == nullptr && rhs == nullptr;
        }

        wrl::ComPtr<IUnknown> lhsUnknown;
        wrl::ComPtr<IUnknown> rhsUnknown;
        VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(lhs)->QueryInterface(IID_PPV_ARGS(&lhsUnknown)));
        VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(rhs)->QueryInterface(IID_PPV_ARGS(&rhsUnknown)));
        return lhsUnknown.Get() == rhsUnknown.Get();
    }

    unsigned GetLength(Platform::Array<xaml_automation::Provider::IRawElementProviderSimple^>^ providers)
    {
        return providers == nullptr ? 0u : providers->Length;
    }

    xaml_automation_peers::AutomationPeer^ GetPeerFromProvider(xaml_automation::Provider::IRawElementProviderSimple^ provider)
    {
        VERIFY_IS_TRUE(provider != nullptr);
        auto bridge = ref new BridgeAutomationPeer();
        auto peer = bridge->CallPeerFromProvider(provider);
        VERIFY_IS_TRUE(peer != nullptr);
        return peer;
    }

    xaml::UIElement^ GetOwnerFromProvider(xaml_automation::Provider::IRawElementProviderSimple^ provider)
    {
        return safe_cast<xaml_automation_peers::FrameworkElementAutomationPeer^>(GetPeerFromProvider(provider))->Owner;
    }

    xaml_controls::CalendarView^ CreatePinnedCalendarView(::Windows::Foundation::DateTime minDate, ::Windows::Foundation::DateTime maxDate)
    {
        auto calendarView = ref new xaml_controls::CalendarView();
        calendarView->Language = L"en-US";
        calendarView->CalendarIdentifier = ::Windows::Globalization::CalendarIdentifiers::Gregorian;
        calendarView->FirstDayOfWeek = ::Windows::Globalization::DayOfWeek::Sunday;
        calendarView->MinDate = minDate;
        calendarView->MaxDate = maxDate;
        calendarView->SetDisplayDate(minDate);
        xaml_automation::AutomationProperties::SetName(calendarView, L"CalendarView");
        return calendarView;
    }

    xaml_controls::CalendarViewDayItem^ FindRealizedDayItem(xaml_controls::CalendarView^ calendarView, ::Windows::Foundation::DateTime date)
    {
        auto dayItems = ref new Platform::Collections::Vector<xaml_controls::CalendarViewDayItem^>();
        Microsoft::UI::Xaml::Tests::Common::TreeHelper::GetVisualChildrenByType(calendarView, dayItems);
        for (auto dayItem : dayItems)
        {
            if (IsSameGregorianDay(dayItem->Date, date))
            {
                return dayItem;
            }
        }
        return nullptr;
    }

    // Verifies GridProvider::GetItem(row, column) in month view returns the day item for expectedDate, that the
    // column matches the day of the week (FirstDayOfWeek is Sunday) and that the item reports the same grid position.
    void VerifyMonthViewGridItem(
        xaml_automation::Provider::IGridProvider^ gridProvider,
        int row,
        int column,
        ::Windows::Foundation::DateTime expectedDate)
    {
        LOG_OUTPUT(L"GetItem(%d, %d)", row, column);
        auto provider = gridProvider->GetItem(row, column);
        VERIFY_IS_TRUE(provider != nullptr);

        auto dayItem = safe_cast<xaml_controls::CalendarViewDayItem^>(GetOwnerFromProvider(provider));
        VERIFY_IS_TRUE(IsSameGregorianDay(expectedDate, dayItem->Date));
        VERIFY_ARE_EQUAL(column, static_cast<int>(GetGregorianDayOfWeek(dayItem->Date)));

        auto gridItemProvider = safe_cast<xaml_automation::Provider::IGridItemProvider^>(GetPeerFromProvider(provider));
        VERIFY_ARE_EQUAL(row, gridItemProvider->Row);
        VERIFY_ARE_EQUAL(column, gridItemProvider->Column);
    }
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

    void CalendarViewAutomationPeerIntegrationTests::VerifyCalendarViewPeerPatternsAndReadOnlyContract()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::CalendarView^ calendarView = nullptr;

        RunOnUIThread([&]()
        {
            calendarView = local::CreatePinnedCalendarView(local::MakeGregorianDate(2000, 11, 15), local::MakeGregorianDate(2001, 1, 15));
            TestServices::WindowHelper->WindowContent = calendarView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(calendarView);
            VERIFY_IS_TRUE(peer != nullptr);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"CalendarView"), peer->GetClassName());
            VERIFY_ARE_EQUAL(xaml_automation_peers::AutomationControlType::Calendar, peer->GetAutomationControlType());

            LOG_OUTPUT(L"The Grid, Table, Selection and Value patterns are implemented by the CalendarView peer itself.");
            const xaml_automation_peers::PatternInterface supportedPatterns[] =
            {
                xaml_automation_peers::PatternInterface::Grid,
                xaml_automation_peers::PatternInterface::Table,
                xaml_automation_peers::PatternInterface::Selection,
                xaml_automation_peers::PatternInterface::Value,
            };
            for (auto pattern : supportedPatterns)
            {
                VERIFY_IS_TRUE(local::AreSameObject(peer, peer->GetPattern(pattern)));
            }

            LOG_OUTPUT(L"Other patterns fall back to the base peer, which doesn't support them.");
            VERIFY_IS_TRUE(peer->GetPattern(xaml_automation_peers::PatternInterface::Invoke) == nullptr);
            VERIFY_IS_TRUE(peer->GetPattern(xaml_automation_peers::PatternInterface::Toggle) == nullptr);

            auto tableProvider = safe_cast<xaml_automation::Provider::ITableProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Table));
            VERIFY_ARE_EQUAL(xaml_automation::RowOrColumnMajor::RowMajor, tableProvider->RowOrColumnMajor);
            VERIFY_ARE_EQUAL(0u, local::GetLength(tableProvider->GetRowHeaders()));

            auto selectionProvider = safe_cast<xaml_automation::Provider::ISelectionProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Selection));
            VERIFY_IS_FALSE(selectionProvider->IsSelectionRequired);

            LOG_OUTPUT(L"The Value pattern is read-only: SetValue is not implemented and doesn't select a date.");
            auto valueProvider = safe_cast<xaml_automation::Provider::IValueProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Value));
            VERIFY_IS_TRUE(valueProvider->IsReadOnly);

            bool threwNotImplemented = false;
            try
            {
                valueProvider->SetValue(L"November 20, 2000");
            }
            catch (Platform::NotImplementedException^)
            {
                threwNotImplemented = true;
            }
            VERIFY_IS_TRUE(threwNotImplemented);
            VERIFY_ARE_EQUAL(0u, calendarView->SelectedDates->Size);
        });
    }

    void CalendarViewAutomationPeerIntegrationTests::VerifyCalendarViewPeerGridAndTableInMonthView()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::CalendarView^ calendarView = nullptr;

        RunOnUIThread([&]()
        {
            // 2000-11-15 is a Wednesday, so with Sunday as the first day of the week the first item is in column 3.
            calendarView = local::CreatePinnedCalendarView(local::MakeGregorianDate(2000, 11, 15), local::MakeGregorianDate(2001, 1, 15));
            TestServices::WindowHelper->WindowContent = calendarView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(calendarView);
            auto gridProvider = safe_cast<xaml_automation::Provider::IGridProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Grid));

            LOG_OUTPUT(L"Month view: 7 columns (days of the week) and NumberOfWeeksInView (default 6) rows.");
            VERIFY_ARE_EQUAL(7, gridProvider->ColumnCount);
            VERIFY_ARE_EQUAL(6, gridProvider->RowCount);

            LOG_OUTPUT(L"The first row starts before MinDate: the cells before it are empty and MinDate is in column 3.");
            VERIFY_IS_TRUE(gridProvider->GetItem(0, 0) == nullptr);
            VERIFY_IS_TRUE(gridProvider->GetItem(0, 2) == nullptr);
            local::VerifyMonthViewGridItem(gridProvider, 0, 3, local::MakeGregorianDate(2000, 11, 15));
            local::VerifyMonthViewGridItem(gridProvider, 0, 6, local::MakeGregorianDate(2000, 11, 18));
            local::VerifyMonthViewGridItem(gridProvider, 1, 0, local::MakeGregorianDate(2000, 11, 19));
            local::VerifyMonthViewGridItem(gridProvider, 2, 4, local::MakeGregorianDate(2000, 11, 30));

            LOG_OUTPUT(L"The column headers are the week day names, starting with FirstDayOfWeek.");
            auto tableProvider = safe_cast<xaml_automation::Provider::ITableProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Table));
            auto columnHeaders = tableProvider->GetColumnHeaders();
            const wchar_t* sundayFirstNames[] = { L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday" };
            VERIFY_ARE_EQUAL(7u, local::GetLength(columnHeaders));
            for (unsigned i = 0; i < 7; ++i)
            {
                VERIFY_ARE_EQUAL(ref new Platform::String(sundayFirstNames[i]), local::GetPeerFromProvider(columnHeaders[i])->GetName());
            }

            calendarView->FirstDayOfWeek = ::Windows::Globalization::DayOfWeek::Monday;
            calendarView->NumberOfWeeksInView = 3;
            calendarView->SetDisplayDate(local::MakeGregorianDate(2001, 1, 10));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(calendarView);
            auto gridProvider = safe_cast<xaml_automation::Provider::IGridProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Grid));

            LOG_OUTPUT(L"RowCount follows NumberOfWeeksInView.");
            VERIFY_ARE_EQUAL(7, gridProvider->ColumnCount);
            VERIFY_ARE_EQUAL(3, gridProvider->RowCount);

            LOG_OUTPUT(L"After scrolling away from the first row, each visible row is a full week starting on Monday.");
            const auto firstRowFirstItem = gridProvider->GetItem(0, 0);
            VERIFY_IS_TRUE(firstRowFirstItem != nullptr);
            auto firstDate = safe_cast<xaml_controls::CalendarViewDayItem^>(local::GetOwnerFromProvider(firstRowFirstItem))->Date;
            VERIFY_IS_GREATER_THAN(firstDate.UniversalTime, local::MakeGregorianDate(2000, 11, 20).UniversalTime);
            VERIFY_ARE_EQUAL(::Windows::Globalization::DayOfWeek::Monday, local::GetGregorianDayOfWeek(firstDate));

            const long long ticksPerDay = 24LL * 60 * 60 * 10000000;
            for (int row = 0; row < 2; ++row)
            {
                for (int column = 0; column < 7; ++column)
                {
                    auto provider = gridProvider->GetItem(row, column);
                    VERIFY_IS_TRUE(provider != nullptr);
                    auto date = safe_cast<xaml_controls::CalendarViewDayItem^>(local::GetOwnerFromProvider(provider))->Date;
                    ::Windows::Foundation::DateTime expected{ firstDate.UniversalTime + (row * 7 + column) * ticksPerDay };
                    VERIFY_IS_TRUE(local::IsSameGregorianDay(expected, date));

                    auto gridItemProvider = safe_cast<xaml_automation::Provider::IGridItemProvider^>(local::GetPeerFromProvider(provider));
                    VERIFY_ARE_EQUAL(row, gridItemProvider->Row);
                    VERIFY_ARE_EQUAL(column, gridItemProvider->Column);
                }
            }

            LOG_OUTPUT(L"The column headers follow the new FirstDayOfWeek.");
            auto tableProvider = safe_cast<xaml_automation::Provider::ITableProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Table));
            auto columnHeaders = tableProvider->GetColumnHeaders();
            const wchar_t* mondayFirstNames[] = { L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday", L"Sunday" };
            VERIFY_ARE_EQUAL(7u, local::GetLength(columnHeaders));
            for (unsigned i = 0; i < 7; ++i)
            {
                VERIFY_ARE_EQUAL(ref new Platform::String(mondayFirstNames[i]), local::GetPeerFromProvider(columnHeaders[i])->GetName());
            }
        });
    }

    void CalendarViewAutomationPeerIntegrationTests::VerifyCalendarViewPeerGridAndRowHeadersInYearView()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::CalendarView^ calendarView = nullptr;
        const auto november2000 = local::MakeGregorianDate(2000, 11, 15);
        const auto january2001 = local::MakeGregorianDate(2001, 1, 15);

        RunOnUIThread([&]()
        {
            // Three months (Nov 2000 - Jan 2001) shown on a 3 x 2 year view.
            calendarView = local::CreatePinnedCalendarView(november2000, january2001);
            calendarView->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
            TestServices::WindowHelper->WindowContent = calendarView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            calendarView->SetYearDecadeDisplayDimensions(3 /*columns*/, 2 /*rows*/);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(calendarView);
            auto gridProvider = safe_cast<xaml_automation::Provider::IGridProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Grid));
            auto tableProvider = safe_cast<xaml_automation::Provider::ITableProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Table));

            LOG_OUTPUT(L"Year view uses the year/decade display dimensions and has no column headers.");
            VERIFY_ARE_EQUAL(3, gridProvider->ColumnCount);
            VERIFY_ARE_EQUAL(2, gridProvider->RowCount);
            VERIFY_ARE_EQUAL(0u, local::GetLength(tableProvider->GetColumnHeaders()));

            LOG_OUTPUT(L"Items are the months in range, laid out from the first cell; there is no fourth month.");
            auto novemberItem = gridProvider->GetItem(0, 0);
            auto januaryItem = gridProvider->GetItem(0, 2);
            VERIFY_IS_TRUE(novemberItem != nullptr);
            VERIFY_IS_TRUE(januaryItem != nullptr);
            VERIFY_IS_TRUE(gridProvider->GetItem(1, 0) == nullptr);

            auto novemberPeer = local::GetPeerFromProvider(novemberItem);
            auto januaryPeer = local::GetPeerFromProvider(januaryItem);
            VERIFY_ARE_EQUAL(ref new Platform::String(L"CalendarViewItem"), novemberPeer->GetClassName());
            VERIFY_IS_FALSE(local::AreSameObject(novemberPeer, januaryPeer));

            auto januaryGridItem = safe_cast<xaml_automation::Provider::IGridItemProvider^>(januaryPeer);
            VERIFY_ARE_EQUAL(0, januaryGridItem->Row);
            VERIFY_ARE_EQUAL(2, januaryGridItem->Column);

            LOG_OUTPUT(L"In year view each item's row header is the year it belongs to.");
            auto novemberRowHeaders = safe_cast<xaml_automation::Provider::ITableItemProvider^>(novemberPeer->GetPattern(xaml_automation_peers::PatternInterface::TableItem))->GetRowHeaderItems();
            auto januaryRowHeaders = safe_cast<xaml_automation::Provider::ITableItemProvider^>(januaryPeer->GetPattern(xaml_automation_peers::PatternInterface::TableItem))->GetRowHeaderItems();
            VERIFY_ARE_EQUAL(1u, local::GetLength(novemberRowHeaders));
            VERIFY_ARE_EQUAL(1u, local::GetLength(januaryRowHeaders));
            VERIFY_ARE_EQUAL(local::FormatEnUsGregorian(L"year", november2000), local::GetPeerFromProvider(novemberRowHeaders[0])->GetName());
            VERIFY_ARE_EQUAL(local::FormatEnUsGregorian(L"year", january2001), local::GetPeerFromProvider(januaryRowHeaders[0])->GetName());

            calendarView->DisplayMode = xaml_controls::CalendarViewDisplayMode::Decade;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(calendarView);
            auto gridProvider = safe_cast<xaml_automation::Provider::IGridProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Grid));

            LOG_OUTPUT(L"Decade view: the items are the years 2000 and 2001, and they have no row headers.");
            VERIFY_ARE_EQUAL(3, gridProvider->ColumnCount);
            VERIFY_ARE_EQUAL(2, gridProvider->RowCount);
            auto yearItem = gridProvider->GetItem(0, 0);
            VERIFY_IS_TRUE(yearItem != nullptr);
            VERIFY_IS_TRUE(gridProvider->GetItem(0, 1) != nullptr);
            VERIFY_IS_TRUE(gridProvider->GetItem(0, 2) == nullptr);

            auto yearRowHeaders = safe_cast<xaml_automation::Provider::ITableItemProvider^>(local::GetPeerFromProvider(yearItem)->GetPattern(xaml_automation_peers::PatternInterface::TableItem))->GetRowHeaderItems();
            VERIFY_ARE_EQUAL(0u, local::GetLength(yearRowHeaders));
        });
    }

    void CalendarViewAutomationPeerIntegrationTests::VerifyCalendarViewPeerSelectionAndValue()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::CalendarView^ calendarView = nullptr;
        const auto november15 = local::MakeGregorianDate(2000, 11, 15);
        const auto november20 = local::MakeGregorianDate(2000, 11, 20);
        const auto farAwayDate = local::MakeGregorianDate(2009, 6, 10);

        RunOnUIThread([&]()
        {
            // A wide range, so that a selected date years away is not realized.
            calendarView = local::CreatePinnedCalendarView(november15, local::MakeGregorianDate(2010, 12, 31));
            TestServices::WindowHelper->WindowContent = calendarView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(calendarView);
            auto selectionProvider = safe_cast<xaml_automation::Provider::ISelectionProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Selection));
            auto valueProvider = safe_cast<xaml_automation::Provider::IValueProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Value));

            LOG_OUTPUT(L"CanSelectMultiple is true only in Multiple selection mode.");
            VERIFY_ARE_EQUAL(xaml_controls::CalendarViewSelectionMode::Single, calendarView->SelectionMode);
            VERIFY_IS_FALSE(selectionProvider->CanSelectMultiple);
            calendarView->SelectionMode = xaml_controls::CalendarViewSelectionMode::None;
            VERIFY_IS_FALSE(selectionProvider->CanSelectMultiple);
            calendarView->SelectionMode = xaml_controls::CalendarViewSelectionMode::Multiple;
            VERIFY_IS_TRUE(selectionProvider->CanSelectMultiple);

            LOG_OUTPUT(L"Nothing selected: no selection and the value is the header text of the current view.");
            auto headerText = calendarView->TemplateSettings->HeaderText;
            VERIFY_IS_TRUE(headerText != nullptr && headerText->Length() > 0);
            VERIFY_ARE_EQUAL(0u, local::GetLength(selectionProvider->GetSelection()));
            VERIFY_ARE_EQUAL(headerText, valueProvider->Value);

            LOG_OUTPUT(L"One date selected: the value is that date, formatted as 'day month.full year'.");
            calendarView->SelectedDates->Append(november15);
            auto expectedValue = local::FormatEnUsGregorian(L"day month.full year", november15);
            VERIFY_IS_TRUE(wcsstr(expectedValue->Data(), L"November") != nullptr);
            VERIFY_ARE_EQUAL(expectedValue, valueProvider->Value);
            auto selection = selectionProvider->GetSelection();
            VERIFY_ARE_EQUAL(1u, local::GetLength(selection));
            VERIFY_IS_TRUE(local::IsSameGregorianDay(november15, safe_cast<xaml_controls::CalendarViewDayItem^>(local::GetOwnerFromProvider(selection[0]))->Date));

            LOG_OUTPUT(L"Two dates selected: the selection lists both items in SelectedDates order, and the value falls back to the header text.");
            calendarView->SelectedDates->Append(november20);
            selection = selectionProvider->GetSelection();
            VERIFY_ARE_EQUAL(2u, local::GetLength(selection));
            VERIFY_IS_TRUE(local::IsSameGregorianDay(november15, safe_cast<xaml_controls::CalendarViewDayItem^>(local::GetOwnerFromProvider(selection[0]))->Date));
            VERIFY_IS_TRUE(local::IsSameGregorianDay(november20, safe_cast<xaml_controls::CalendarViewDayItem^>(local::GetOwnerFromProvider(selection[1]))->Date));
            VERIFY_ARE_EQUAL(headerText, valueProvider->Value);

            LOG_OUTPUT(L"A selected date whose item isn't realized is not part of the UIA selection.");
            calendarView->SelectedDates->Append(farAwayDate);
            VERIFY_ARE_EQUAL(3u, calendarView->SelectedDates->Size);
            VERIFY_IS_TRUE(local::FindRealizedDayItem(calendarView, farAwayDate) == nullptr);
            VERIFY_ARE_EQUAL(2u, local::GetLength(selectionProvider->GetSelection()));

            calendarView->SelectedDates->Clear();
            calendarView->SelectedDates->Append(november20);
            calendarView->DisplayMode = xaml_controls::CalendarViewDisplayMode::Year;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(calendarView);
            auto selectionProvider = safe_cast<xaml_automation::Provider::ISelectionProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Selection));
            auto valueProvider = safe_cast<xaml_automation::Provider::IValueProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Value));

            LOG_OUTPUT(L"Year view: the selected day has no item, so the selection is empty, but the value still is the selected date.");
            VERIFY_ARE_EQUAL(1u, calendarView->SelectedDates->Size);
            VERIFY_ARE_EQUAL(0u, local::GetLength(selectionProvider->GetSelection()));
            VERIFY_ARE_EQUAL(local::FormatEnUsGregorian(L"day month.full year", november20), valueProvider->Value);

            LOG_OUTPUT(L"Year view with nothing selected: the value is the year view's header text.");
            calendarView->SelectedDates->Clear();
            VERIFY_ARE_EQUAL(calendarView->TemplateSettings->HeaderText, valueProvider->Value);
        });
    }

    void CalendarViewAutomationPeerIntegrationTests::VerifySingleSelectionReplacementRaisesRemovedFromSelection()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::CalendarView^ calendarView = nullptr;
        const auto november15 = local::MakeGregorianDate(2000, 11, 15);
        const auto november16 = local::MakeGregorianDate(2000, 11, 16);

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"CalendarView";
        uiaInfo.m_AutomationID = L"CalendarView";
        uiaInfo.m_cType = UIA_CalendarControlTypeId;

        RunOnUIThread([&]()
        {
            calendarView = local::CreatePinnedCalendarView(november15, local::MakeGregorianDate(2001, 1, 15));
            VERIFY_ARE_EQUAL(xaml_controls::CalendarViewSelectionMode::Single, calendarView->SelectionMode);
            calendarView->SelectedDates->Append(november15);
            TestServices::WindowHelper->WindowContent = calendarView;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            auto removedEvent = std::make_shared<Event>();
            wrl::ComPtr<Patterns::SelectionItemPatternHandler> removedHandler;
            removedHandler.Attach(new Patterns::SelectionItemPatternHandler(automationClientManager, removedEvent, TreeScope_Subtree, UIA_SelectionItem_ElementRemovedFromSelectionEventId));
            removedHandler->AttachEventHandler();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Add 2000-11-16 to the selection through its SelectionItem pattern while 2000-11-15 is selected.");
                auto dayItem = local::FindRealizedDayItem(calendarView, november16);
                VERIFY_IS_TRUE(dayItem != nullptr);
                auto dayItemPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(dayItem);
                safe_cast<xaml_automation::Provider::ISelectionItemProvider^>(dayItemPeer->GetPattern(xaml_automation_peers::PatternInterface::SelectionItem))->AddToSelection();
            });

            LOG_OUTPUT(L"The previously selected item raises ElementRemovedFromSelection.");
            removedHandler->ConfirmAndUnregister();
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Single selection mode replaced the selected date.");
            VERIFY_ARE_EQUAL(1u, calendarView->SelectedDates->Size);
            VERIFY_IS_TRUE(local::IsSameGregorianDay(november16, calendarView->SelectedDates->GetAt(0)));

            auto oldItemPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(local::FindRealizedDayItem(calendarView, november15));
            auto newItemPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(local::FindRealizedDayItem(calendarView, november16));
            VERIFY_IS_FALSE(safe_cast<xaml_automation::Provider::ISelectionItemProvider^>(oldItemPeer->GetPattern(xaml_automation_peers::PatternInterface::SelectionItem))->IsSelected);
            VERIFY_IS_TRUE(safe_cast<xaml_automation::Provider::ISelectionItemProvider^>(newItemPeer->GetPattern(xaml_automation_peers::PatternInterface::SelectionItem))->IsSelected);
        });
    }

    void CalendarViewAutomationPeerIntegrationTests::VerifyBulkSelectionChangeRaisesSelectionInvalidated()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::CalendarView^ calendarView = nullptr;
        const auto firstDate = local::MakeGregorianDate(2000, 11, 15);

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"CalendarView";
        uiaInfo.m_AutomationID = L"CalendarView";
        uiaInfo.m_cType = UIA_CalendarControlTypeId;

        RunOnUIThread([&]()
        {
            calendarView = local::CreatePinnedCalendarView(firstDate, local::MakeGregorianDate(2001, 1, 15));
            calendarView->SelectionMode = xaml_controls::CalendarViewSelectionMode::Multiple;
            TestServices::WindowHelper->WindowContent = calendarView;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto selectConsecutiveDates = [&](unsigned count)
        {
            const long long ticksPerDay = 24LL * 60 * 60 * 10000000;
            for (unsigned i = 0; i < count; ++i)
            {
                calendarView->SelectedDates->Append(::Windows::Foundation::DateTime{ firstDate.UniversalTime + i * ticksPerDay });
            }
            VERIFY_ARE_EQUAL(count, calendarView->SelectedDates->Size);
        };

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto automationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            LOG_OUTPUT(L"Clearing 20 selected dates (the bulk limit) raises per-item events, not Selection Invalidated.");
            RunOnUIThread([&]() { selectConsecutiveDates(20); });
            auto twentyEvent = std::make_shared<Event>();
            wrl::ComPtr<AutomationClient::AutomationEventHandler> twentyHandler;
            twentyHandler.Attach(new AutomationClient::AutomationEventHandler(automationClientManager, twentyEvent, TreeScope_Element, UIA_Selection_InvalidatedEventId));
            twentyHandler->AttachEventHandler();
            RunOnUIThread([&]() { calendarView->SelectedDates->Clear(); });
            twentyHandler->ConfirmNegativeAndUnregister();

            LOG_OUTPUT(L"Clearing 21 selected dates (above the bulk limit) raises Selection Invalidated on the CalendarView.");
            RunOnUIThread([&]() { selectConsecutiveDates(21); });
            auto twentyOneEvent = std::make_shared<Event>();
            wrl::ComPtr<AutomationClient::AutomationEventHandler> twentyOneHandler;
            twentyOneHandler.Attach(new AutomationClient::AutomationEventHandler(automationClientManager, twentyOneEvent, TreeScope_Element, UIA_Selection_InvalidatedEventId));
            twentyOneHandler->AttachEventHandler();
            RunOnUIThread([&]() { calendarView->SelectedDates->Clear(); });
            twentyOneHandler->ConfirmAndUnregister();
        });

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0u, calendarView->SelectedDates->Size);
        });
    }
} } } } } }
