// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Collection.h>
#include <Versioning.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AutoSuggestBox {

    class AutoSuggestBoxIntegrationTests : public WEX::TestClass<AutoSuggestBoxIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AutoSuggestBoxIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"c84f8180-05b4-44ec-9575-917de852b45f;e6e3a886-04a7-4146-a8a7-26a3e81d503b")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create an AutoSuggestBox.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove an AutoSuggestBox from the live tree.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetAndSetProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set/get public properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRaiseTextChangedEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a TextChanged event raises upon changing the text.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRaiseTextChangedEventInFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a TextChanged event raises upon changing the text in a Flyout.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTextBoxStyle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set the TextBoxStyle property.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetQueryButtonIcon)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the value of QueryIcon is properly propagated down into QueryButton.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateQueryButtonIsCollapsedWithNoQueryIcon)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that QueryButton is collapsed with QueryIcon = null.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDataContextDoesNotPropagateIntoHeader)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a DataConext external to AutoSuggestBox does not propagate into the AutoSuggestBox's header.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCollectionUpdates)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that dynamic updates to the ItemsSource are propagated correctly.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of AutoSuggestBox and its SuggestionsList.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSuggestionListNavigationUsingGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates navigation to the suggestion list using gamepad.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSuggestionListNavigationUsingKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates navigation to the suggestion list using keyboard.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCloseSuggestionListUsingGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the suggestions list can be closed with gamepad input.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCloseSuggestionListUsingKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the suggestions list can be closed with keyboard input.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRaiseQuerySubmittedUsingGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates QuerySubmitted event can be raised when using gamepad.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRaiseQuerySubmittedUsingKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates QuerySubmitted event can be raised when using keyboard.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRaiseQuerySubmittedUsingMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates QuerySubmitted event can be raised when using mouse.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTraverseSuggestionListUsingGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates traversing through the suggestion list, using gamepad, causes SuggestionChanged to be raised for each element.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")

        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTraverseSuggestionListUsingKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates traversing through the suggestion list, using keyboard, causes SuggestionChanged to be raised for each element.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanMoveAwayUsingGamepadWhenSuggestionListIsClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can move away from an AutoSuggestBox when the SuggestionList is not open, using gamepad.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanMoveAwayUsingKeyboardWhenSuggestionListIsClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can move away from an AutoSuggestBox when the SuggestionList is not open, using keyboard.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNotMoveAwayUsingGamepadWhenSuggestionListIsOpen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can not move away from an AutoSuggestBox when the SuggestionList is open, using gamepad.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRaiseSuggestionChosenEventProjectedShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a SuggestionChosen event raises upon changing the text, projected shadow mode.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // We're on drop shadows now
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRaiseSuggestionChosenEventDropShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a SuggestionChosen event raises upon changing the text, drop shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanTraceTelemetryData)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that certain telemetry is logged during typical user scenario.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateQuerySubmittedContainsCurrentText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the QuerySubmitted event is raised with the current text in the AutoSuggestBox.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateAutoSuggestBoxWorksWithoutQueryButton)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that an AutoSuggestBox without a QueryButton in its TextBoxStyle still works properly.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanTapOnItemAfterSelectingItWithKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that navigating to an item with the keyboard and then tapping on it causes QuerySubmitted to be correctly raised.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateAutoSuggestBoxPosition)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the AutoSuggestionBox position.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of AutoSuggestBox in various visual states.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ScrollWheelScrollsSuggestions)
            TEST_METHOD_PROPERTY(L"Description", L"The scroll wheel should scroll the suggestion list in the correct direction.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSuggestionListChangeInTextChangedHandler)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that making updates during the callout to the textchanged event handler works correctly without crashing.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDisplayMemberPathPropagatesToSuggestionsPopup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting DisplayMemberPath properly propagates its value to the ListView in the suggestions popup.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUpdateItemsSource)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that updates to ItemsSource get reflected in an open suggestion list")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePopupOpensAsSoonAsItemsSourceChanges)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that changing the value of ItemsSource in TextChanged immediately opens the popup.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateIsSuggestionListOpenChangesWhenPopupOpens)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that opening the suggestions list popup causes the value of IsSuggestionListOpen to change.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLightDismissOverlayMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the behavior of the LightDismissOverlayMode property.")
                                                           // which would happen on the phone.
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(IsAutoLightDismissOverlayModeVisibleOnXbox)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting LightDismissOverlayMode to Auto on Xbox causes the overlay to be visible.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the brush used for the overlay matches the 'AutoSuggestBoxLightDismissOverlayBackground' resource.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayUIETree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates UIElement tree with an overlay-enabled AutoSuggestBox.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSipClosedOnLostFocus)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the SIP closes when the autosuggestbox loses focus.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone") // OneCore VM does not support SIP
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanTabOutWhileSuggestionListIsOpen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that tabbing out of the control while the suggestion list is open is supported.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesNotClearTextWhenTabbedPast)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that tabbing past the control does not clear any text in the ASB.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUnloadAndReloadReregistersQuerySubmittedEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that unloading and loading the ASB reregisters the QuerySubmitted event properly.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSuggestionListFitsInWindow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SuggestionList is properly arranged inside the available space when using IsSuggestionListOpen property to open it.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"Desktop") // Windowed popups make this test moot.
        END_TEST_METHOD()

    private:
        enum class InputMethod
        {
            Gamepad,
            Keyboard,
            Mouse,
            Remote
        };

        void StartTracingTelemetry();
        void StopTracingTelemetry();

        xaml_controls::StackPanel^ SetupAutoSuggestBoxTest(xaml::VerticalAlignment verticalAlign, Platform::Object^ itemList);
        xaml_controls::StackPanel^ SetupAutoSuggestBoxTestWithEvents(xaml::VerticalAlignment verticalAlign, Platform::Object^ itemList,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> gotFocusEvent,
            SafeEventRegistrationType(xaml_controls::AutoSuggestBox, GotFocus)& gotFocusRegistration,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> textChangedEvent,
            SafeEventRegistrationType(xaml_controls::AutoSuggestBox, TextChanged)& textChangedRegistration,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> suggestionChosenEvent,
            SafeEventRegistrationType(xaml_controls::AutoSuggestBox, SuggestionChosen)& suggestionChosenRegistration,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> popupOpenedEvent,
            SafeEventRegistrationType(xaml_primitives::Popup, Opened)& popupOpenedRegistration,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> popupClosedEvent,
            SafeEventRegistrationType(xaml_primitives::Popup, Closed)& popupClosedRegistration);
        xaml_controls::AutoSuggestBox^ SetupAutoSuggestBoxWithQueryIcon();
        static xaml_controls::StackPanel^ SetupAutoSuggestBoxForUIValidation(xaml::VerticalAlignment verticalAlign, Platform::Collections::Vector<Platform::String^>^ itemList);
        static Platform::Collections::Vector<Platform::String^>^ GetStandardSuggestionList();

        void ScrollWheelScrollsSuggestionsWorker(xaml::VerticalAlignment alignment);

        void PerformValidateAutoSuggestBoxPosition(xaml::Thickness margin);
        void PerformValidateSuggestionListNavigation(xaml::VerticalAlignment verticalAlign, InputDevice inputDevice, bool goDownFirst);
        void PerformCanCloseSuggestionList(InputDevice inputDevice);
        void PerformCanRaiseQuerySubmitted(InputMethod inputMethod, bool goToFirstSuggestion);
        void PerformValidateTraverseSuggestionList(xaml::VerticalAlignment verticalAlign, InputDevice inputDevice, bool goDownFirst);
        void PerformMoveAwayFromAutoSuggestBox(InputDevice inputDevice, bool moveHorizontally, bool suggestionListOpen, bool goToFirstSuggestion);

        void NextInput(InputDevice inputDevice, bool goDown);

        static xaml_controls::Panel^ SetupOverlayTreeValidationTest();
        static xaml::FrameworkElement^ GetAutoSuggestBoxOverlayElement(xaml_controls::AutoSuggestBox^ autoSuggestBox);
        static void ValidateVisibilityOfOverlayElement(xaml_controls::AutoSuggestBox^ autoSuggestBox, bool expectedIsVisible);

        void CanRaiseSuggestionChosenEvent();

    private:
        bool m_bIsTracing;
    };

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::AutoSuggestBox

