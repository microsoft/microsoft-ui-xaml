// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <AutomationClient\AutomationClientManager.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace CalendarView {

    class CalendarViewAutomationPeerIntegrationTests : public WEX::TestClass<CalendarViewAutomationPeerIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(CalendarViewAutomationPeerIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"5a73b065-eae3-4d93-865c-ad87e20ec0d0")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyUIATree)
            TEST_METHOD_PROPERTY(L"Description", L"Verify UIA tree of the Calendar.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyElementPatterns)
            TEST_METHOD_PROPERTY(L"Description", L"Verify Table/Grid pattern of the Calendar.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySelectionChangedEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Verify Selection Changed events of the Calendar.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDayItemRowHeaders)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that we get the correct row headers for CalendarView day items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDayItemAutomationName)
            TEST_METHOD_PROPERTY(L"Description", L"Verify strings returned by CalendarViewDayItemAutomationPeer::GetNameCore for CalendarView day items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyOutOfViewItemsDoNotSupportGridItemPattern)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that out-of-view day items do not support the grid item pattern.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAutomationNotificationEventAfterClickingNavigationButton)
            TEST_METHOD_PROPERTY(L"Description", L"Verify automation notification event is received when clicking navigation button")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCalendarViewPeerPatternsAndReadOnlyContract)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the CalendarView peer's class name, supported Grid/Table/Selection/Value patterns and their static (read-only, row-major, no row headers, SetValue not implemented) contract.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCalendarViewPeerGridAndTableInMonthView)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the CalendarView peer's GridProvider (RowCount/ColumnCount/GetItem) and TableProvider column headers in month view.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCalendarViewPeerGridAndRowHeadersInYearView)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the CalendarView peer's GridProvider and the year row headers of its items in year view.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCalendarViewPeerSelectionAndValue)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the CalendarView peer's SelectionProvider (CanSelectMultiple/GetSelection) and ValueProvider (Value) for the various selection states and display modes.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySingleSelectionReplacementRaisesRemovedFromSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that replacing the selected date in Single selection mode deselects the old date and raises the ElementRemovedFromSelection UIA event.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyBulkSelectionChangeRaisesSelectionInvalidated)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the Selection Invalidated UIA event is raised only when more than 20 dates change at once.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCalendarViewPeerAndControlGetIids)
            TEST_METHOD_PROPERTY(L"Description", L"Verify IInspectable::GetIids of CalendarView, its activation factory and its automation peer lists the interfaces they implement.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCalendarViewItemPeerInvokeAndTableItemInYearView)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the year view item peer's control type, Invoke and TableItem patterns, and that invoking a month opens that month in month view.")
        END_TEST_METHOD()

    private:
    };

} } } } } }
