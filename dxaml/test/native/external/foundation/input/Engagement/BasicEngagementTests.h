// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <RuntimeEnabledFeatureOverride.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Controls;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Engagement {
        class BasicEngagementTests : public WEX::TestClass<BasicEngagementTests>
        {
        public:
            BEGIN_TEST_CLASS(BasicEngagementTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1d91ef47-c885-45e2-a578-7aaf1a1b1296;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(IsFocusEngagementEnabledProperty)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the IsFocusEngagementEnabled property is settable, and that setting it to false disengages a control engaged with a gamepad.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(IsFocusEngagedSet)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that controls are engaged with gamepad A and disengaged with gamepad B.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UnengagedSliderInteraction)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that an unengaged slider cannot be interacted with with a gamepad.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SliderInteractionAfterEngagement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that an engaged slider can be interacted with with a gamepad.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DisengageOnlyIfCancelNotHandled)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that Gamepad B disengages a control only if it was not previously handled.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Focus engagement bugs in lifted islands
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ForceDisengage)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that RemoveFocusEngagement can force a control to disengage with a gamepad.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DisengageAfterFocusChange)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that a control engaged with a gamepad disengages if focus is changed programmatically.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EngagedAutofocus)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that autofocus is limited when a control is engaged with a gamepad.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EngagementWhenNotEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that a control engaged with a gamepad loses focus engagement when IsEnabled is set to false.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EngagementWhenNotTabStop)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that setting IsTabStop to false doesn't immediately cause a control engaged with a gamepad to disengage.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Focus engagement bugs in lifted islands
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ParentChildEngagementWhenNotEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that setting IsEnabled to false disengages a control engaged with a gamepad even when its child has focus.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ListViewEngagementInteractionAndDisengagement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that a listview can be engaged, interacted with, and disengaged with a gamepad.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FlipViewEngagementInteractionAndDisengagement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that a flipview can be engaged, interacted with, and disengaged with a gamepad.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ListBoxEngagementInteractionAndDisengagement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that a listbox can be engaged, interacted with, and disengaged with a gamepad.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ComboBoxDropdownEngagement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates engagement is not lost when focus shifts to a combobox dropdown, and that the control is still disengageable with gamepad B.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UnparentedPopupAutofocusOnEngagement)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that autofocus works in an unparented popup when engaged with a gamepad.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // WPF: Multiple XY focus failures in WPF mode
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanFocusOnFocusEngageableItem)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that focus will move to an element that is not a tab stop but can be focus engaged.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(KeyDownEventIsHandledPastEngagedElement)
                TEST_METHOD_PROPERTY(L"Description", L"When an element is engaged, we should the engaged element as handled for keydown")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NonKeyDownEventHandledCorrectlyPastEngagedElement)
                TEST_METHOD_PROPERTY(L"Description", L"When an element is engaged, normal handling behavior should still take place for non keydown events")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusingHyperlinkDoesNotDisengage)
                TEST_METHOD_PROPERTY(L"Description", L"When an element is engaged and we are focusing a hyperlink, we should not disengage")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Focus engagement bugs in lifted islands
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateIsFocusEngagedProgrammatically)
                TEST_METHOD_PROPERTY(L"Description", L"Validates pre-requisite conditions and makes sure that setting IsFocusEngaged to true on a control works as expected")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateExceptionsForIsFocusEngagedSetter)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that error conditions for IsFocusEngaged setter are represented by appropriate setters")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateEngagementIsRemovedWithUnexpectedInput)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when input comes in from devices other than Gamepad or Remote, we remove Engagement")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // [DCPP-test] WPF tests are failing with AnimationIdle timeout during test cleanup
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateIsFocusEngagedWithChildContainingFocus)
                TEST_METHOD_PROPERTY(L"Description", L"Validates pre-requisite conditions and makes sure that setting IsFocusEngaged to true on a control works as expected when one of its children has focus")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Focus engagement bugs in lifted islands
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ElementWithinEngagedControlFocusedAfterEngagement)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that when we engage a control, we attempt to set focus to an element inside of it.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NonTabbableElementWithinEngagedControlSkippedAfterEngagement)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that when we engage a control, and our likely candidate is not tabbable, we skip it as a candidate")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateFocusStaysOnEngagedControlIfNoCandidatesAfterEngagement)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that when we engage a control, we do not set focus if there are no valid candidates to focus")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EngagedElementWithinPopupShouldDisengageWhenLosingFocus)
                TEST_METHOD_PROPERTY(L"Description", L"When an element within the popup root is engaged and we focus another element within the popup root, we should disengage")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // XamlObjects events can be fired after its parent Island has been Dispose
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;
            void CheckFocusStatusForAllControls(
                _In_ std::vector<Microsoft::UI::Xaml::Controls::Control^>* pControls,
                _In_opt_ Microsoft::UI::Xaml::Controls::Control^ expectedToBeFocused,
                FocusState expectedFocusState) const;
        };

        ref class StickyEngagementSlider : public Slider
        {
        public:
            virtual void OnPreviewKeyDown(Microsoft::UI::Xaml::Input::KeyRoutedEventArgs^ args) sealed override
            {
                args->Handled = true;
            }
        };

    } } }

} } } }
