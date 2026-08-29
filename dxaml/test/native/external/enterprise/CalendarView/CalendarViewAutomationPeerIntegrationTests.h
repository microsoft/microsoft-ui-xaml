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

    private:
    };

} } } } } }
