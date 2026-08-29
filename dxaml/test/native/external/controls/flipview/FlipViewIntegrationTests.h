// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace FlipView {

    class FlipViewIntegrationTests : public WEX::TestClass<FlipViewIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(FlipViewIntegrationTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"ab3da967-c6a7-4e5b-b3c4-c53c6c46175c;a69ddfa4-5142-4bed-887d-6d0ca14a3473;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a FlipView.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a FlipView from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanVirtualizeAndRealize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully realize and virtualize items in FlipView.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFlipWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully change the selection using touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFlipWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully change the selection using gamepad.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFlipWithKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully change the selection using keyboard.")
        END_TEST_METHOD()

       BEGIN_TEST_METHOD(CanAddManiplateAndRemoveItemsInSuccession)
            TEST_METHOD_PROPERTY(L"Description", L"Crash in FlipView when removing an item from ObservableCollection.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRestoreSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the FlipView SelectedIndex property is restored when the FlipView is put back into the tree and DManip is aware of the offset.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of FlipView in various visual states.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOneItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the FlipView has only one item for show and hide.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateHidingFlipViewHidesNavigationButtons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting Visibility to Collapsed on FlipView also sets its buttons to that value immediately.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDisablingFlipViewHidesNavigationButtons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting IsEnabled to false on FlipView also sets its buttons to that value immediately.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CannotScrollPastEdges)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that when we're at the first item or the last item, we can't scroll past it.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCollapsedButtonState)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the collapsed button state with the single item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateResizeFlipView)
            TEST_METHOD_PROPERTY(L"Description", L"Resizing the FlipView should not result in the selected item changing.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateResizeFlipViewWithStackPanelItemsPanel)
            TEST_METHOD_PROPERTY(L"Description", L"Resizing the FlipView (using a StackPanel for ItemsPanel) should not result in the selected item changing.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeSelectionWhileSuspended)
            TEST_METHOD_PROPERTY(L"Description", L"A FlipView that changes its selection while the app is suspended is not expected to animate its view.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateResizeFlipViewWhileChangingItem)
            TEST_METHOD_PROPERTY(L"Description", L"A FlipView that changes size while it is changing item should result in the correct item being shown.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyInitialSelectedIndex)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that setting SelectedIndex on the FlipView before it has loaded works correctly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySelectionChangeViaAutomationPeer)
            TEST_METHOD_PROPERTY(L"Description", L"Verify changing selection by using the items automation peer ScrollIntoView function")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFlipWithMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully change the selection using mouse and template buttons.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScrollableWithMouseWheel)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully use the mouse wheel to scroll through a FlipView.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScrollableWithMouseWheelWithoutAnimation)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully use the mouse wheel to scroll through a FlipView after turning off OS animations.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScrollingWithMouseWheelQuicklyIsIgnored)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that if we scroll twice quickly with the mouse wheel, it ignores the second scroll until enough time has passed.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateEmptyFlipViewDoesNotCrashWithMouseWheel)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that scrolling on a FlipView with no FlipViewItems does not crash.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePagingKeyInteractionVertical)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that PageUp/PageDn and Gamepad triggers move selection.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePagingKeyInteractionHorizontal)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that PageUp/PageDn and Gamepad bumpers move selection.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGamepadKeyDownRoutedOrHandledVertical)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that Vertical FlipView does not handle Right/Left/Shoulders, but does handle Up/Down/Triggers.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGamepadKeyDownRoutedOrHandledHorizontal)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that Horizontal FlipView does handle Right/Left/Shoulders, but does not handle Up/Down/Triggers.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFillAutoSuggestBox)
            TEST_METHOD_PROPERTY(L"Description", L"Verify ability to enter text into an AutoSuggestBox without changing FlipView's selected item.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(IncreaseSelectedIndexDuringAnimation)
            TEST_METHOD_PROPERTY(L"Description", L"Verify ability to increase the SelectedIndex while the FlipView animates to another item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DecreaseSelectedIndexDuringAnimation)
            TEST_METHOD_PROPERTY(L"Description", L"Verify ability to decrease the SelectedIndex while the FlipView animates to another item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanBringFlipViewIntoView)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that FlipView in a ScrollViewer is brought into view when one of its items gets focused.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MouseWheelInputsFlipOnce)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that FlipView only flips a single item given a series of mouse wheel messages.")
        END_TEST_METHOD()

    private:
        void VerifyGamepadKeyDownRoutedOrHandledRunner(xaml_controls::Orientation orientation);
        void ChangeSelectedIndexDuringAnimation(int selectedIndexDelta, bool whileIncrementingSelectedIndex);
        void ValidateScrollableWithMouseWheelWithOptionalAnimation(bool animate);
        void DoValidateResizeFlipView(bool useNonVirtualizingStackPanel);

        static xaml_controls::FlipView^ CreateFlipView(xaml_controls::Orientation orientation, unsigned int itemsCount, bool useNonVirtualizingStackPanel = false);
        static xaml_controls::FlipView^ SetupBasicFlipView(xaml_controls::Orientation orientation, unsigned int itemsCount);

        static bool AreClose(double x, double y)
        {
            return std::abs(x - y) < 0.001;
        }

    };

} } } } } }

