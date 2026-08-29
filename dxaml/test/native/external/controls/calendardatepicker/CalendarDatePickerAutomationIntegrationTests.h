// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>
//  Copyright (c) Microsoft Corporation.  All rights reserved.



namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace CalendarDatePicker {

    class CalendarDatePickerAutomationIntegrationTests : public WEX::TestClass<CalendarDatePickerAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(CalendarDatePickerAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyCalendarDatePickerUIAValuePattern)
            TEST_METHOD_PROPERTY(L"Description", L"Validates automation PatternInterface_Value interface on a CalendarDatePicker.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCalendarDatePickerAPLocalizedControlTypeFromPublicAPI)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Localized ControlType on a CalendarDatePicker from public API.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDefaultAutomationName)
            TEST_METHOD_PROPERTY(L"Description", L"Validates setting the automation name on a CalendarDatePicker.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAutomationControlType)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the AutomationControlType reported by CalendarDatePicker")
        END_TEST_METHOD()


    private: 
        // Verify the value by IValueProvider on AutomationPeer is the same with displayed DateText
        void ValidateUIAValueIsSameWithDateText(xaml_controls::CalendarDatePicker^ cdp);        
    };

} } } } } }
