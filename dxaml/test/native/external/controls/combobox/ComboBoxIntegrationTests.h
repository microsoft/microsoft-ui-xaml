// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CommonInputHelper.h>
#include <ComboBoxHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ComboBox {

    class ComboBoxIntegrationTests : public WEX::TestClass<ComboBoxIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ComboBoxIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"854cb07a-8bd6-43e3-a0db-4f0fe8648245;8fdf7774-ee0a-492c-9951-4371f13328a0;fb6372ee-e783-49fd-b5cc-8669c0c85f22")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a ComboBox.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a ComboBox from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanComboBoxLoadFromXaml)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the loading ComboBox from the Xaml.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDefaultProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the ComboBox default properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanExpandAndCloseProjectedShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the ComboBox expand and close, projected shadow mode.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // We're on drop shadows now
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanExpandAndCloseDropShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the ComboBox expand and close, drop shadow mode.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanInsertItemAfterExpandAndClose)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that items can be inserted into the ComboBox after an expand and close.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of ComboBox in various visual states ")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateVeryWideComboBoxItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the ComboBox behaves correctly, even when very long items are added to it.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOpenComboBoxWithAllItemsLongWidth)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the ComboBox opens correctly without a layout cycle crash in case of having all items long width.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a ComboBox can be opened with touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DropDownClosesOnComboBoxUnloaded)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the ComboBox closes the DropDown if it is unloaded. ")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateResettingSelectedIndexBringsBackPlaceholderText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting SelectedIndex to -1 causes the opacity of the placeholder text to be set back to 1.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSmoothlyScrollOpenComboBoxWithVariableWidthItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can smoothly scroll a ComboBox with items that vary in width.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EventsHandledOnGamepadB)
            TEST_METHOD_PROPERTY(L"Description", L"Checks if the handled flag is set to true when a ComboBox is closed with gamepad B.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EventHandledOnEscape)
            TEST_METHOD_PROPERTY(L"Description", L"Checks if the handled flag is set when a ComboBox is closed with Escape.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EventsContinuallyHandledOnPressAndHoldGamepadB)
            TEST_METHOD_PROPERTY(L"Description", L"Checks if the handled flag is always set to true when a ComboBox is closed with gamepad B and then held down.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateResetItemsSource)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that reset the ItemsSource property after change the items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDropdownPlacement)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the ComboBox never opens its dropdown outside the bounds of the window.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesGetFocusWhenProgrammaticallyOpened)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that focus is set on the ComboBox when it is opened programmatically.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SelectionChangedIsNotRaisedUntilClose)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that moving between items using the gamepad does not raise the SelectionChanged event.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of a ComboBox with/without a Header")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectItemWithTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can select an item by tapping")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectedValuePathProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SelectedValuePath property behaves as expected")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCustomizedPaddingWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the customized padding on the ComboBoxItem with touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateItemTemplateSettingWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ItemTemplate setting on the ComboBoxItem with touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCloseComboBoxWithF4)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the f4 key closes the combobox dropdown.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardInteraction)
            TEST_METHOD_PROPERTY(L"Description", L"Uses the keyboard to open a ComboBox, change the selection and close the ComboBox.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCycleThroughItemsWithMouseWheel)
            TEST_METHOD_PROPERTY(L"Description", L"Uses the mouse wheel to cycle through combobox items.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanLightDismissDropdown)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that you can dismiss the dropdown by tapping away from it.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOpenedComboBoxPositionByTouchInput)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the opened ComboBox's position that must be positioned within the visible bounds when it is opened by using the touch input.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Not working in WPF-hosting, scrollViewer bounds relative to WindowHelper->WindowBounds are unexpected.
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNavigateAscendingComboBoxesWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the combobox can be navigated when we realize wider items at the bottom of the list using gamepad.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNavigateAscendingComboBoxesWithKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the combobox can be navigated when we realize wider items at the bottom of the list using keyboard.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDropdownSizingForDifferentInputModes)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the size of the ComboBox dropdown and its items based on different input modes (mouse, touch, etc.).")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: 31563479 - Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCustomizedPaddingWithMouseAndKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the customized padding on the ComboBoxItem with mouse and keyboard input.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: 31563479 - Mouse input helper doesn't work on phone/onecore
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCloseComboBoxWithAltDown)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the alt down key combination closes the combobox dropdown.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOpenedComboBoxPositionWithDifferentInput)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the opened ComboBox position that must be positioned within the visible bounds when it is opened with the different input.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Not working in WPF-hosting, scrollViewer bounds relative to WindowHelper->WindowBounds are unexpected.
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLightDismissOverlayMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the behavior of the LightDismissOverlayMode property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesAutoLightDismissOverlayModeCreateOverlayOnXbox)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting LightDismissOverlayMode to Auto on Xbox causes the popup overlay to get configured to On.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the brush used for the overlay matches the 'ComboBoxLightDismissOverlayBackground' resource.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusStateForComboBoxOpenedWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the selected item has pointer focus when the ComboBox is opened via touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusStateForComboBoxOpenedWithMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the selected item has pointer focus when the ComboBox is opened via mouse.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusStateForComboBoxOpenedWithKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the selected item has keyboard focus when the ComboBox is opened via keyboard.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusStateForComboBoxOpenedWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the selected item has keyboard focus when the ComboBox is opened via gamepad.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusStateOfClosedComboBoxWhenOpenedAndClosedWithMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the closed combobox has Pointer focus after being opened and closed with mouse.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore, WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusStateOfClosedComboBoxWhenOpenedAndClosedWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the closed combobox has Pointer focus after being opened and closed with touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusStateOfClosedComboBoxWhenOpenedAndClosedWithKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the closed combobox has Keyboard focus after being opened and closed with keyboard.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusStateOfClosedComboBoxWhenOpenedAndClosedWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the closed combobox has Keyboard focus after being opened and closed with gamepad.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusStateOfClosedComboBoxWhenOpenedWithTouchAndClosedWithKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the closed combobox has Keyboard focus after being opened with touch and closed with keyboard.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusStateOfClosedComboBoxWhenOpenedWithTouchAndClosedWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the closed combobox has Keyboard focus after being opened with touch and closed with gamepad.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusStateOfClosedComboBoxWhenOpenedWithMouseAndClosedWithKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the closed combobox has Keyboard focus after being opened with mouse and closed with keyboard.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusStateOfClosedComboBoxWhenOpenedWithMouseAndClosedWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that the closed combobox has Keyboard focus after being opened with mouse and closed with gamepad.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePagingKeyInteraction)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that pressing PageUp/PageDn/End/Home correctly navigate the ComboBox.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePopupPlacementAtBottomOfScreen)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the placement of the last ComboBox item always appears within visible bounds if the Combobox is at the bottom of the screen.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOpenedDropDownHeightUsingMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the opened ComboBox drop down height by using mouse when the ComboBox is positioned on the different vertical alignments.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusTrappedWithNoSelectionPressingGamepadUp)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that pressing Gamepad Up when the ComboBox does not have a selected item does not move focus to another item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(OpenComboBoxWhileSIPIsUp)
            TEST_METHOD_PROPERTY(L"Description", L"Open a CombBox while the SIP is up.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTriggerKeyFocusNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that pressing the trigger keys navigate the ComboBox, and that the proper item is selected when the ComboBox is closed via GamepadA.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTriggerKeysDoNotChangeSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that pressing the trigger keys move focus, but don't change selection.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOpenedDropDownHeightWithMouseAfterTouchInput)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the DComp output when ComboBox is opened with the mouse after opened/closed the ComboBox with the touch input.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOpeningWithPendingClosedEventDoesNotCloseComboBox)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that opening the ComboBox when a pending Popup.Closed event is in the queue does not cause the ComboBox to immediately close itself.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetIsDropDownOpenBeforeTemplateIsAppliedAndGotFocus)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that we don't crash if IsDropDownOpen is set before template is applied and we are given focus.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesNotChangeSelectedIndexWithHorizontalArrowKeys)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Left/Right arrow keys do not change the selected index.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanHorizontallyNavigatePastAComboBox)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that XY Focus navigation can pass through a ComboBox horizontally.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesRevertSelectionOnCancel)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the selected index reverts back to what it was when opened with our various ways to cancel (Escape, GamepadB, Back-button, click/tap away).")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesRevertSelectionOnCancelWithManyItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that cancelling a ComboBox selection when there are many items correctly reverts back to the previously selected item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectionChangedTrigger)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the behavior of the SelectionChangedTrigger property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoHomeAndEndChangeSelectionWhenClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the Home & End keys change the selection when the ComboBox is closed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectedInfoPropertiesStayInSync)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the SelectedIndex, SelectedItem & SelectedValue properties stay in sync.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesNotShowMulitpleSelectionVisuals)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we don't get into a state where multiple items show as selected.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VallidateMaximumHeightIsHonored)
            TEST_METHOD_PROPERTY(L"Description", L"Validates we honor the maximum height set on the combobox.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyItemBeforeVisibleItemsInPopupIsRealized)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an item before the visible items in the open popup has been realized")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetEditableMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ComboBox's IsEditable property is settable.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ComboBox's tab behavior in editable and non-editable mode.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRaiseTextSubmittedEventComboBoxClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Validates TextSubmitted events are raised after typing a custom value, and selected value is correct.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRaiseTextSubmittedEventComboBoxOpened)
            TEST_METHOD_PROPERTY(L"Description", L"Validates TextSubmitted events are raised after typing a custom value, and selected value is correct when ComboBox is open.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTextSubmittedHandledProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ComboBox doesn't do anything after TextSubmitted event if Handled is true.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateItemAutoMatchOnTextSubmitted)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ComboBox tries to match Custom value to Item in Collection after TextSubmitted event.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateEditableModeSearchAndSelectionComboBoxClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Validate Search and Selection in Editable Mode.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateEditableModeSearchAndSelectionComboBoxOpened)
            TEST_METHOD_PROPERTY(L"Description", L"Validate Search and Selection in Editable Mode when ComboBox is open.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetComboBoxTextOnNonEditableMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validate ComboBox Text property is passed to the Editable Mode TextBox even when Editable Mode is disabled.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateEditableModeSelectedItemAndValueAreSetToNull)
            TEST_METHOD_PROPERTY(L"Description", L"Validate Editable ComboBox sets custom SelectedItem and SelectedValue to null after disabling Editable Mode.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateEditableModeGamePadInteraction)
            TEST_METHOD_PROPERTY(L"Description", L"Validate Editable ComboBox Gamepad behavior.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateEditableModePopupOpensUp)
            TEST_METHOD_PROPERTY(L"Description", L"Validate Editable ComboBox opens popup above ComboBox if needed.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"Desktop") // With windowed popups on desktop, this test is moot.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateEditableModeComboBoxCanScrollToNewItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validate Editable ComboBox can scroll to virtualized items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingSelectedIndexUpdatesTextBoxText)
            TEST_METHOD_PROPERTY(L"Description", L"Validate setting SelectedIndex updates Editable ComboBox TextBox text.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectionTriggerAlwaysCanSetCustomValueAgain)
            TEST_METHOD_PROPERTY(L"Description", L"Validate re-typing active custom value after arrowing through values on Editable ComboBox with SelectionTrigger Always commits the correct value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDropDownArrowClosesPopupOnEditableComboBox)
            TEST_METHOD_PROPERTY(L"Description", L"Validate clicking the DropDown arrow while Editable ComboBox is open closes the popup and selects the current value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDropDownOverlayVisuals)
            TEST_METHOD_PROPERTY(L"Description", L"Validates DropDown overlay visuals for Editable ComboBox.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LightDismissLayerOnIslands)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDisableShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that it's possible to disable the shadow on ComboBox.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateRestoreOnCancelIndexResetOnClose)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that opening a ComboBox, picking an item, opening it again, and then closing without selecting anything doesn't revert the selection back to the first index.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateEditableComboBoxHasLightDismissLayer)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that an open editable combo box has a light-dismiss layer that catches input before it can go elsewhere.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(OpenTwiceThenPanComboBox)
            TEST_METHOD_PROPERTY(L"Description", L"Opens the ComboBox dropdown twice and then pans its content.")
        END_TEST_METHOD()

    private:
        void CanNavigateAscendingComboBoxes(InputDevice device);

        xaml_controls::ComboBox^ SetupBasicComboBoxTest(UINT numberOfItems = 5, bool adjustMargin = true, bool isEditable = false);
        xaml_controls::ComboBox^ SetupAscendingComboBoxTest(UINT numberOfItems = 50);

        void EventsContinuallyHandledOnPressAndHold(Platform::String^ keyDownSequence, Platform::String^ keyUpSequence);

        void CloseComboBoxWithParentHandler(Platform::String^ keySequence, bool expectedKeyDownHandledValue, bool verifyKeyDown, bool expectedKeyUpHandledValue, bool verifyKeyUp);

        void DoValidateDropdownPlacement(xaml::VerticalAlignment verticalAlignment, xaml::HorizontalAlignment horizontalAlignment, bool useWideItems, int rotationAngle, xaml::FlowDirection flowDirection, bool expectOutside);

        xaml_controls::ComboBox^ SetupComboBoxCustomizedPaddingTest();

        void ValidateComboBoxItemPadding(xaml_controls::ComboBox^ comboBox, ComboBoxHelper::OpenMethod openMethod);

        void CanCloseComboBoxWithKeySequence(Platform::String^ keySequence);

        void DoValidatePosition(int itemCount, ComboBoxHelper::OpenMethod openMethod, bool addMouseOpenMethod, bool isVerticalAlignment = true);
        void ValidatePosition(xaml::HorizontalAlignment horizontalAlign, xaml::VerticalAlignment verticalAlignment, xaml_controls::ComboBox^ comboBox, ComboBoxHelper::OpenMethod openMethod);

        static void ValidateComboBoxPopupLightDismissOverlayMode(xaml_controls::LightDismissOverlayMode expectedMode);

        void ValidateFocusStateForComboBoxWorker(ComboBoxHelper::OpenMethod openMethod, xaml::FocusState expectedFocusState);

        void ValidateFocusStateOfClosedComboBoxWorker(ComboBoxHelper::OpenMethod openMethod, ComboBoxHelper::CloseMethod closeMethod, xaml::FocusState expectedFocusState);

        void CanRaiseTextSubmittedEventComboBox(bool open, bool addToItemSource = false, bool setHandled = false);

        void ValidateEditableModeSearchAndSelection(bool open);

        void InjectKeySequence(Platform::Collections::Vector<Platform::String^>^ keySequence, int timeBetweenKeyPressesInMs);

        void ValidateDropDownOverlayVisualsHelper(xaml::ElementTheme theme, bool useHighContrast, Platform::String ^ variation);

        static void VerifyFocusedElement(Platform::Object^ expectedFocusedElement);

        void EnsureEditableTextBoxHasFocus(xaml_controls::ComboBox^ comboBox);

        void CanExpandAndClose();
    };

} } } } } }
