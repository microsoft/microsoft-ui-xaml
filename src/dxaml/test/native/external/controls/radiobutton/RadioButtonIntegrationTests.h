// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace RadioButton {

    class RadioButtonIntegrationTests : public WEX::TestClass<RadioButtonIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(RadioButtonIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911;eadeac67-1552-4876-a67e-dc1fcb8a6e25;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a RadioButton.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a RadioButton from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanClickUsingTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can we can click a RadioButton using Tap.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesGroupingWork)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that RadioButton can change states.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CreateMultipleChecked)
            TEST_METHOD_PROPERTY(L"Description", L"Validates when multiple check radio buttons are added to the tree in the same group, only one will end up checked.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(CanChangeGroupName)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that RadioButton group name can be changed.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetIntermediateState)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that RadioButton can be set to indeterminate state.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of RadioButton in various visual states ")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of RadioButton.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusDoesNotShiftWhenFocusedRadioButtonGroupNameIsChanged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the focus stays when a focused RadioButton's group name is changed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusShiftsWhenFocusedRadioButtonIsRemoved)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the focus moves to the first focusable element on the page when a focused RadioButton is removed from the page.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTraverseDefaultRadioButtonGroupByGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates traversing through the 'default' group of RadioButtons with gamepad input.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTraverseDefaultRadioButtonGroupByKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates traversing through the 'default' group of RadioButtons with keyboard input.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTraverseRadioButtonGroupByGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates traversing through a named group of RadioButtons with gamepad input.")            
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore") 
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTraverseRadioButtonGroupByKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates traversing through a named group of RadioButtons with keyboard input.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // WPF_HOSTING_MODE_FAILURE - up/down arrow keys don't seem to have an effect
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanInteractWithRadioButtonsOnAPage)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can key through and check RadioButtons on a page, which we can navigate to and from.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanTapRadioButtonInAPopup)
            TEST_METHOD_PROPERTY(L"Description", L"Regression test: Edge crashes when RadioButton Group is inside a Popup.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SelectionPatternShouldInvokeClick)
            TEST_METHOD_PROPERTY(L"Description", L"RadioButtonAutomationPeer.Selection should invoke the Click event")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RadioButtonInCycleTabFocusDoesNotCauseInfiniteLoop)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that pressing the up or down arrow with focus on a radio button when there is no other radio button to give focus to does not cause an infinite loop with TabFocusNavigation='Cycle'.")
        END_TEST_METHOD()

    private:
        xaml_controls::StackPanel^ AddRadioButtonsToPanel(
            bool isNamedGroup,
            std::vector<xaml_controls::RadioButton^> & radioButtons,
            xaml_controls::StackPanel^ panel = nullptr);
        xaml_controls::StackPanel^ AddRadioButtonsToPanelWithCheckedEvents(
            bool isNamedGroup,
            std::vector<xaml_controls::RadioButton^> & radioButtons,
            std::vector<SafeEventRegistrationType(xaml_controls::RadioButton, Checked)>& checkedRegistrations,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> radioButtonCheckedEvent,
            xaml_controls::StackPanel^ panel = nullptr);
        xaml_controls::StackPanel^ AddRadioButtonsToPanelWithFocusedHandler(
            bool isNamedGroup,
            std::vector<xaml_controls::RadioButton^> & radioButtons,
            std::vector<SafeEventRegistrationType(xaml_controls::RadioButton, GotFocus)>& focusedRegistrations,
            xaml::RoutedEventHandler^ gotFocusHandler,
            xaml_controls::StackPanel^ panel = nullptr);

        void PerformValidateFocusShift(bool shouldRenameGroup, bool shouldRemove, bool focusShifts);
        void PerformValidateTraverseRadioButtonGroup(bool isNamedGroup, InputDevice inputDevice, bool moveForwardFirst, bool useLeftRightkeys);
    };

} } } } } }
