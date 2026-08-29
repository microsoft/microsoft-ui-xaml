// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace CalendarDatePicker {

    class CalendarDatePickerIntegrationTests : public WEX::TestClass<CalendarDatePickerIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(CalendarDatePickerIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"5a73b065-eae3-4d93-865c-ad87e20ec0d0;375cd7bd-e448-4315-b2a1-bc02d75b0c4f")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(ValidateDefaultPropertyValues)
            TEST_METHOD_PROPERTY(L"Description", L"Validate default values of properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a CalendarDatePicker from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenFlyoutByTapping)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can open flyout by tapping text area")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenCloseFlyoutBySettingIsCalendarOpen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can open/close flyout by set IsCalendarOpen to true/false.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenFlyoutByKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can open flyout by Enter and Space when it has focus")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCloseFlyoutBySelectingADate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can close the flyout by selecting a date")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDateIsCoerced)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that date property is coerced by min/max date")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFormatDate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the date text will be formatted correctly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SettingCalendarIdentifierChangesDateFormat)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that changing the CalendarIdentifier also updates the date format.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PressingDoesNotOpenMenuFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that pressing does not open menu flyout (if they exist).")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UIElementTree for a CDP.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateVisualStates)
            TEST_METHOD_PROPERTY(L"Description", L"Validates all Visual States.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DonotResizeCalendarView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates where there is limited space, the calendarview is not resized.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanPresetDate)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarDatePicker does not show pre-loaded value for Date property")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTwoWayBinding)
            TEST_METHOD_PROPERTY(L"Description", L"[CalendarDatePicker] DateChanged event gets fired even when the selected date hasn't changed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TestDateChangedEventWhenAssignDateToSameValue)
            TEST_METHOD_PROPERTY(L"Description", L"[CalendarDatePicker] DateChanged event gets fired even when the selected date hasn't changed.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the brush used for the Overlay matches the 'CalendarDatePickerLightDismissOverlayBackground' resource.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()
    };

} } } } } }

