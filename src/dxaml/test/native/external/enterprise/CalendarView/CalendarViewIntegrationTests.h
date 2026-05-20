// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <collection.h>
#include <CommonInputHelper.h>
#include <SafeEventRegistration.h>
#include <Versioning.h>

#include "CalendarHelper.h"

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace CalendarView {

    class CalendarViewIntegrationTests : public WEX::TestClass<CalendarViewIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(CalendarViewIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"4f7c3f9b-7644-4513-afe1-f0bb414ff9cc;262268fd-57a5-48fa-9410-bd31d3fa5217")
            TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            TEST_CLASS_PROPERTY(L"TestSuite", L"A") // This test class is quite large, so we break it down into suites.
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //

        BEGIN_TEST_METHOD(DisplayCalendarViewWithRoundedCornersStyle)
            TEST_METHOD_PROPERTY(L"Description", L"Displays and exercises a CalendarView control with rounded corner styles.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DisplayCalendarViewWithRoundedCornersStyleAndCustomNumberOfWeeksInView)
            TEST_METHOD_PROPERTY(L"Description", L"Displays and exercises a CalendarView control with rounded corner styles and non-default NumberOfWeeksInView values.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DisplayCalendarViewWithSquareCornersStyles)
            TEST_METHOD_PROPERTY(L"Description", L"Displays and exercises a CalendarView control with square corner styles.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DisplayCalendarViewWithSquareCornersStylesAndCustomNumberOfWeeksInView)
            TEST_METHOD_PROPERTY(L"Description", L"Displays and exercises a CalendarView control with square corner styles and non-default NumberOfWeeksInView values.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyMultipleEraCalendar)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView: crashes in Japanese decade view when a decade scope cross eras")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDefaultPropertyValues)
            TEST_METHOD_PROPERTY(L"Description", L"Validate default values of properties.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TestSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validate selections, selected dates changed event.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectOutOfRangeDate)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that we can select a date that doesn't exist in min/max range")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectDuplicatedDates)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that we can mark a date a selected more than one time.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNotSelectBlackoutDate)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that we can not select blackout date.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNotSelectMoreDates)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that we can not select more dates than what the selection mode supports.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeSelectedDatesInsideSelectedDatesChangedEvent)
            TEST_METHOD_PROPERTY(L"Description", L"It's not allowed to change the selection inside a SelectedDatesChanged event")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TestViewMode)
            TEST_METHOD_PROPERTY(L"Description", L"Change view mode.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TestCICEvents)
            TEST_METHOD_PROPERTY(L"Description", L"Test calendar item changing events, set blackout and density bars.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyBlackoutProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Verify blackout property will be cleared when container is being recycled and reused.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCalendarItemCount)
            TEST_METHOD_PROPERTY(L"Description", L"validate the number of day, month and year items.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNumberOfWeeksInViewRange)
            TEST_METHOD_PROPERTY(L"Description", L"Validate boundary checks for the NumberOfWeeksInView property.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyButtonState)
            TEST_METHOD_PROPERTY(L"Description", L"validate the state (disabled/enabled) of navigation buttons.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyNavigationButtonsBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the behavior that clicking button will navigate one scope or one page based on if we can show a full scope in viewport or not.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySelfAdaptivePanel)
            TEST_METHOD_PROPERTY(L"Description", L"verify that when Month name is too long to fit in viewport, we automatically adjust the dimensions")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a CalendarView from the live tree.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CalendarPanelLayoutTestFirstItemPositonTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validates layouts of CalendarPanel: the position of first item")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CalendarPanelLayoutTestRowsAndColsTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validates layouts of CalendarPanel: to verify how many rows and cols we arranged in the viewport")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CalendarPanelLayoutTestStretchTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validates layouts of CalendarPanel: to verify stretch/non stretch mode")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeDisplayModeBeforeLoaded)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView doesn't respect displaymode before loaded.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTransitionAnimation)
            TEST_METHOD_PROPERTY(L"Description", L"Validate transition animation when switching display mode.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHeaderTransitionAnimation)
            TEST_METHOD_PROPERTY(L"Description", L"Validate transition animation when changing headertext.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeLanguage)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that we can change language.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDCompTreeWithPointerOverNavigationButton)
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the DComp tree when Pointer Over NavigationButton")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDCompTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the DComp tree and surfaces using MockDComp.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // WPF_HOSTING_MODE_FAILURE - DComp baseline doesn't match
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDCompTreeWithCompositionBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the DComp tree and surfaces using MockDComp when using a composition brush")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDCompTreeWithLinearGradientBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the DComp tree and surfaces using MockDComp when using a linear gradient brush")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UIElement.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeStylePropsDynamically)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can change Font Properties/Brushes/BorderThickness dynamically")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScopeChange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we always make the scope that has most items as current scope.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetDayItemStyle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can set dayitem style")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeMonthCalendarIdentifier)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that all calendar types work in month mode.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeYearCalendarIdentifier)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that all calendar types work in year mode.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeDecadeCalendarIdentifier)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that all calendar types work in decade mode.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPersianCalendarMonthBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPersianCalendarYearBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Fix and re-enable.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPersianCalendarDecadeBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGregorianCalendarMonthBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGregorianCalendarYearBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGregorianCalendarDecadeBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHebrewCalendarMonthBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHebrewCalendarYearBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHebrewCalendarDecadeBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Fix and re-enable. Crash in Windows.Globalization.dll.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHijriCalendarMonthBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHijriCalendarYearBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Fix and re-enable.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHijriCalendarDecadeBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"A")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyJapaneseCalendarMonthBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyJapaneseCalendarYearBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyJapaneseCalendarDecadeBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyJulianCalendarMonthBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyJulianCalendarYearBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyJulianCalendarDecadeBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyKoreanCalendarMonthBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyKoreanCalendarYearBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyKoreanCalendarDecadeBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTaiwanCalendarMonthBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTaiwanCalendarYearBoundaries)
                TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTaiwanCalendarDecadeBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThaiCalendarMonthBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThaiCalendarYearBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThaiCalendarDecadeBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyUmAlQuraCalendarMonthBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyUmAlQuraCalendarYearBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Fix and re-enable.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyUmAlQuraCalendarDecadeBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"verify operations on the calendar boundaries.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()


        BEGIN_TEST_METHOD(VerifyPersianCalendarBoundariesByTapping)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView crashes in DecadeView when clicking the previous button when date is half page away from the MinDate")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGregorianCalendarBoundariesByTapping)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView crashes in DecadeView when clicking the previous button when date is half page away from the MinDate")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHebrewCalendarBoundariesByTapping)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView crashes in DecadeView when clicking the previous button when date is half page away from the MinDate")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHijriCalendarBoundariesByTapping)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView crashes in DecadeView when clicking the previous button when date is half page away from the MinDate")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyJapaneseCalendarBoundariesByTapping)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView crashes in DecadeView when clicking the previous button when date is half page away from the MinDate")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyJulianCalendarBoundariesByTapping)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView crashes in DecadeView when clicking the previous button when date is half page away from the MinDate")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyKoreanCalendarBoundariesByTapping)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView crashes in DecadeView when clicking the previous button when date is half page away from the MinDate")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTaiwanCalendarBoundariesByTapping)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView crashes in DecadeView when clicking the previous button when date is half page away from the MinDate")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThaiCalendarBoundariesByTapping)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView crashes in DecadeView when clicking the previous button when date is half page away from the MinDate")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyUmAlQuraCalendarBoundariesByTapping)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView crashes in DecadeView when clicking the previous button when date is half page away from the MinDate")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScopeStateAfterLoaded)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView: Scope states are not correct when CalendarView is loaded.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScopeInManipulation)
            TEST_METHOD_PROPERTY(L"Description", L"Calendar scrolling animation is not smooth because whole screen turns gray before turning white")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScopeWithDST)
            TEST_METHOD_PROPERTY(L"Description", L"Calendar control shows the 31st as part of next month. (// DayLightSaving dates are shown with wrong scope state.)")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDisplayDateOnCorrectPositionWhenCallSetDisplayDate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that when we call display date we show the display date in correct position.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDisplayDateOnCorrectPositionWhenSwitchView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that when we switch view we show the display date in correct position.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDayItemFontPropertiesAffectMeasure)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that when dayitem font properties changed, we remeasure calendarview.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHeaderTextChangesWhenCalendarIdentifierChanged)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView: the headertext doesn't respect calendaridentifier")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySetDisplayDateBeforeLoaded)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView doesn't respect SetDisplayDate before loaded.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NavigationButtonShouldBeDisabledWhenThereIsNoMoreDates)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView navigation button is not disabled when there is no more dates, clicking it app will crash")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFocusOnCorrectItemWithTap)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView: wrong item is focused when a view gets focus by mouse")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyRenderLayers)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView: Density bars are drawn under the content.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDisplayDate)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView: System Clock takes me to the wrong year.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeStyle)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView: Access violation when editing copy of template of CalendarView in XAML designer")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Crash on style change. Fix and re-enable.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSwitchDisplayModeByCtrlUpAfterLoaded)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView: hit assertion when using 'ctrl + up' to switch from MonthView to YearView")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(KeyboardNavigationTestNavigationKeyTest)
            TEST_METHOD_PROPERTY(L"Description", L"Vaidate keyboard navigation by navigation key.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Vaidate navigation by gamepad.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(KeyboardNavigationTestCanTryToNavigateOutOfBoundary)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView keyboard navigation : app crashes when pressing Up / Left on the first item, or down / right on the last item")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(KeyboardNavigationTestTabTest)
            TEST_METHOD_PROPERTY(L"Description", L"Vaidate focus by Tab and shift+Tab.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(KeyboardNavigationTestSpaceEnterTest)
            TEST_METHOD_PROPERTY(L"Description", L"Vaidate focus by Space and Enter.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateChromeFocus)
            TEST_METHOD_PROPERTY(L"Description", L"Vaidate chrome focus state in Month mode.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateChromeFocusYear)
            TEST_METHOD_PROPERTY(L"Description", L"Vaidate chrome focus state in Year mode.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateChromeFocusDecade)
            TEST_METHOD_PROPERTY(L"Description", L"Vaidate chrome focus state in Decade mode.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // WPF_HOSTING_MODE_FAILURE - DComp baseline doesn't match
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(IgnoreBringIntoViewOnFocusChange)
            TEST_METHOD_PROPERTY(L"Description", L"Ignore BringIntoViewOnFocusChange, when we use keyboard to focus the item on last visible row, make sure we don't add 20 pixel on the bottom")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SuspendBuildTreeWhileCollapsed)
            TEST_METHOD_PROPERTY(L"Description", L"Suspend buildtree work when BuildTree controls are collapsed")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySkippedDaysInSamoa)
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView : 1 / 1 / 2012 is skipped in Samoa timezone due to the TimeZone changed from UTC - 11 : 00 to UTC + 13 : 00 at the end of 12/31/2011")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Re-enable CalendarView timezone tests on non-Desktop skus
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TodayVisualTest)
            TEST_METHOD_PROPERTY(L"Description", L"verify all Today-related visual states")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // MOCK10_REMOVAL
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangingViewHeaderTest)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure correct header is displayed after switching views.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // MOCK10_REMOVAL
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangingViewHeaderWithCustomDimensionsTest)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure correct header is displayed after switching views and with custom row/column dimensions for year and decade.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // MOCK10_REMOVAL
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyChangingHeightDoesNotScrollAwayFromItems)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that when the height of a CalendarView changes the current items are not scrolled out of view.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // [WinUI3]CalendarViewIntegrationTests::VerifyChangingHeightDoesNotScrollAwayFromItems fails after WinUI 2.8 port to WinUI 3
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyForegroundColorChangesPropagateWhenInPopup)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that changes to the CalendarItemForeground property properly propagate when CalendarView is in a closed popup.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlowDirectionForCalendar)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the FlowDirection used for each CalendarIdentifier.")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollAcrossJapaneseEraOnMonthView)
            TEST_METHOD_PROPERTY(L"Description", L"Clock/Date Flyout: Month View: cannot go to Showa63 12 from Heisei01 1 by clicking the 'Back' button")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollAcrossJapaneseEraOnYearView)
            TEST_METHOD_PROPERTY(L"Description", L"'^' button stops working after navigating pass Heisei 31")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollAcrossJapaneseEraOnDecadeView)
            TEST_METHOD_PROPERTY(L"Description", L"Clicking navigation button doesn't switch to next Era if it's the end of an era")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #37592131
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSwitchDisplayMode)
            TEST_METHOD_PROPERTY(L"Description", L"Verify can switch display modes")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
        END_TEST_METHOD()

#define VerifyAllTimeZonesPartDecl(i) \
        BEGIN_TEST_METHOD(VerifyAllTimeZonesPart##i)\
            TEST_METHOD_PROPERTY(L"Description", L"CalendarView days and selected dates are in incorrect offset when timezone is on half hour.")\
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")\
            TEST_METHOD_PROPERTY(L"TestSuite", L"B") \
        END_TEST_METHOD()
        // TODO: Re-enable CalendarView timezone tests on non-Desktop skus

VerifyAllTimeZonesPartDecl(0)
VerifyAllTimeZonesPartDecl(1)
VerifyAllTimeZonesPartDecl(2)
VerifyAllTimeZonesPartDecl(3)
VerifyAllTimeZonesPartDecl(4)
VerifyAllTimeZonesPartDecl(5)
VerifyAllTimeZonesPartDecl(6)
VerifyAllTimeZonesPartDecl(7)
VerifyAllTimeZonesPartDecl(8)
VerifyAllTimeZonesPartDecl(9)
VerifyAllTimeZonesPartDecl(10)
VerifyAllTimeZonesPartDecl(11)
VerifyAllTimeZonesPartDecl(12)
VerifyAllTimeZonesPartDecl(13)
VerifyAllTimeZonesPartDecl(14)
VerifyAllTimeZonesPartDecl(15)
VerifyAllTimeZonesPartDecl(16)
VerifyAllTimeZonesPartDecl(17)
VerifyAllTimeZonesPartDecl(18)
VerifyAllTimeZonesPartDecl(19)

#undef VerifyAllTimeZonesPartDecl


        //
        // Platform:Desktop
        //

        BEGIN_TEST_METHOD(SnapPointTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validate snap points behavior. Calendar has different snap behaviors based on the panel's dimension.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestSuite", L"B")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Values are slightly different after visual update. Fix and re-enable.
        END_TEST_METHOD()

private:
        Microsoft::UI::Xaml::Media::LinearGradientBrush^ MakeLinearGradientBrush(float startX, float endX);
        void VerifyJapaneseEraBoundary(CalendarHelper::CalendarViewHelper& helper, xaml_controls::CalendarViewDisplayMode const& displayMode, ::Windows::Foundation::DateTime const& date, Platform::String^ oldEraHeaderText=nullptr, Platform::String^ newEraHeaderText=nullptr);
        void DisplayCalendarView(int numberOfWeeksInView, bool isInteractive, bool enableRoundedCalendarViewBaseItemChrome);
        void SetBlackouts(xaml_controls::CalendarView^ calendarView, bool isTodayBlackedOut, bool isSundayBlackedOut);
        void SetBlackout(xaml_controls::CalendarViewDayItem^ dayItem, bool isTodayBlackedOut, bool isSundayBlackedOut);
        void SetDensityColors(xaml_controls::CalendarView^ calendarView, bool hasDensityBars);
        void SetDensityColors(xaml_controls::CalendarViewDayItem^ dayItem, bool hasDensityBars);
        Platform::String^ GetResourcesPath() const;
    };

} } } } } }

