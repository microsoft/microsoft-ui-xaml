// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Pivot {

    class PivotIntegrationTests : public WEX::TestClass<PivotIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(PivotIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"2b4b21d5-e5c0-4fad-bed4-62030fe191a1;eadeac67-1552-4876-a67e-dc1fcb8a6e25;bdc5c68b-03c1-4217-832c-4c09f99946f4")
            TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP") // Pivot dtor calling on wrong thread during GC causes crash on shutdown in WPF host mode
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a Pivot.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanInstantiatePivotItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a PivotItem.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanInstantiateDerivedPivotItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a DerivedPivotItem.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a Pivot from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanApplyIncompleteTemplate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can apply a template without the necessary components without crashing.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanReapplyTemplate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully reapply a template after the first time we apply it.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ReleasePivotWhileUnloaded)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully do the final release of the Pivot control while it is unloaded.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectNextItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully navigate to the next PivotItem in Pivot.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectItemDuringUnloading)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully navigate to a PivotItem while the Pivot is being unloaded.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDynamicallyAddItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add PivotItems to the Pivot on the fly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDynamicallyChangeItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully change PivotItems in the Pivot on the fly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDynamicallyDeleteItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully delete PivotItems from the Pivot on the fly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDynamicallyTransitionToAndFromHavingOneItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully go down to and then up from one item in Pivot on the fly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetAndGetProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set and get Pivot specific properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateStaticHeadersStayInPlace)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that static headers do not move when the selected index changes.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDynamicHeadersMove)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that dynamic headers move when the selected index changes.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSwitchBetweenStaticAndDynamicHeaders)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully switch between static and dynamic headers by changing whether or not the headers fit within the Pivot.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateStaticHeaderPanelIsNotRequired)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the Pivot still loads and functions fine without the static header panel in the template.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree on pivot in its various visual states.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCreateSubClassedPivot)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that you can create a sub-classed pivot.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetSelectedItemBeforeTemplateApplied)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that you can set SelectedItem before OnApplyTemplate is called without a crash.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRetemplatePivot)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the windows 10 template is flexible and easy to retemplate.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanPresentLeftAndRightContentAreas)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the behavior of LeftHeaderContent(Template) and RightHeaderContent(Template) properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRestylePivot)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the Pivot style.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFlickDiagonallyOnChildListViewWithoutMovingToNewItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that flicking diagonally on a child list view does not cause the Pivot to shift to a new PivotItem.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFlickDiagonallyOnChildGridViewWithoutMovingToNewItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that flicking diagonally on a child list view does not cause the Pivot to shift to a new PivotItem.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanApplyPivotHeaderItemVerticalAlignment)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that PivotHeaderPanel arrange its children correctly in respect to their vertical alignment.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePivotItemsAreImmediatelyVisibleAfterSelectionChanged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that changing the selected item in a Pivot causes the newly selected item to be immediately made visible.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusIsMovedAfterSelectionChangedWhenPivotItemElementHasFocus)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that if a PivotItem's element has keyboard focus when changing the selected item, focus is moved to the content of the new element.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NavigatingToPivotItemWithFocusableNonControlDoesNotCrash)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that when focus is within a pivot item and the user navigates to a new pivot item whose first focusable element is not a control we don't crash.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateContentTemplateRootExistsInPivotItemLoading)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that PivotItem.ContentTemplateRoot is non-null within the context of the PivotItemLoading event, such that content can be placed into it.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanProgrammaticallyAddPivotItemToEmptyCollection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that adding a PivotItem to Pivot.Items when Pivot.Items was previously empty does not incur a null-ref exception.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanPivotHeaderItemUpdateVisualsOnPointerCaptureLost)
            TEST_METHOD_PROPERTY(L"Description", L"Tests that when you touch a PivotHeaderItem it doesn't stay selected when dragging your finger outside the control.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNavigatePivotWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the contents of a Pivot can be navigated between using a gamepad.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeHeadersWithGamepadShoulderButtons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that you can change Pivot headers with the gamepad shoulder buttons.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeHeadersWithGamepadShoulderButtonsRightToLeft)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that you can change Pivot headers with the gamepad shoulder buttons with FlowDirection=RightToLeft.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePivotHeadersDoNotWrapWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a Pivot does not wrap when navigating it with a gamepad.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies the ActualHeight of Pivot's container doesn't change and take space away from the Pivot's content.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateChangingItemsDoesNotChangeFocusState)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that changing items in a Pivot does not change the focus state of the header panel.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateEmptyPivotItemDoesNotCauseCrash)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that switching to an empty PivotItem does not cause a crash.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeHeaderTemplateAtRuntime)
            TEST_METHOD_PROPERTY(L"Description", L"Validates changing the HeaderTemplate property on a Pivot at runtime is properly applied.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeTitleAtRuntime)
            TEST_METHOD_PROPERTY(L"Description", L"Validates changing the Title property on a Pivot at runtime is properly applied.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateInfiniteSpaceAvailableSnapsPivotToWindowWidth)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that placing a Pivot into a context where it has infinite available size causes its width to be snapped to the window width.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // This test uses WindowHelper.WindowBounds, which currently returns an incorrect value when running in WPF mode
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanApplySlideInAnimationGroupToElements)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can apply the attached property Pivot.SlideInAnimationGroup to elements beneath a PivotItem.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCanAddItemsOnSelectionChanged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that adding items on selection changed does not land us on the wrong selected index.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateHeaderFocusVisualPlacement)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that HeaderFocusVisualPlacement works correctly in the Pivot header.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePivotCanWrapWithTouchWhenCarouselIsEnabled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can wrap pivot through touch input when IsHeaderItemsCarouselEnabled is set to true..")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStaticHeadersScroll)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the secondary relationships applied to the static header panel.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanTogglePivotIsHeaderItemsCarouselEnabled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can toggle between the carousel and non-carousel modes after the pivot is loaded.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanPivotWithPaddleButtons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully navigate through pivot items using the paddle buttons.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: Mouse input helper doesn't work on phone/onecore
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFallbackToNavigationButtonsVisibleIfStatesAreMissing)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we fallback to the NavigationButtonsVisible visual state if the PreviousButtonVisible/NextButtonVisible visual states are not available (which is the case for upgraded TH2 apps with retemplated pivots).")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: Mouse input helper doesn't work on phone/onecore
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTabOnlyStopsAtPivotAndContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that tabbing through a pivot only stops at the pivot itself and at its current content.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
         END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseKeyboardKeysToChangeSelectedIndex)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that you can change the selected index with the keyboard when the pivot is focused. Validate we can't wrap in static mode.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseKeyboardKeysToChangeSelectedIndexRightToLeft)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that you can change the selected index with the keyboard with FlowDirection=RightToLeft.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateThatScrollViewerDoNotHandleInput)
            TEST_METHOD_PROPERTY(L"Description", L"Most of Pivot is inside a ScrollViewer. We need to validate that the latter isn't handling input on behalf of Pivot.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NavigationButtonsShouldNotShowUpForLockedOrStaticHeaderPivot)
            TEST_METHOD_PROPERTY(L"Description", L"Next/Previous navigation buttons should not be shown in a Pivot that's locked or with a static header.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanActivePivotHeaderItemChangeFromPressedToPointerOver)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the active PivotHeaderItem returns to PointerOver after being Pressed.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanPivotShowPaddleButtonsAfterAddingItem)
            TEST_METHOD_PROPERTY(L"Description", L"Tests that Paddle buttons are visible after adding a new PivotHeaderItem in a Pivot that previously didn't need Paddle Buttons.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanPanAfterItemsAddedAtRuntime)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can pan if we create a Pivot and then later add items to make it pannable.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeSelectedItemUsingTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully navigate between PivotItems in Pivot by tapping headers or content.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollOuterScrollViewerWithMouseWheel)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that, if a pivot is inside a parent ScrollViewer, that we can scroll the latter with the mouse wheel even if the pointer is on top of the pivot.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollOuterScrollViewerWithFocusChange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that, if a pivot is inside a parent ScrollViewer, that we can scroll the latter with a bring into view request due to focus change.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PivotInSplitViewWithContentControlDoesNotCrash)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that putting a Pivot in a SplitView pane where the pane root is a ContentControl does not crash.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeGhostPivotHeaderItemTemplateWithoutCrash)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that changing the template of the ghost PivotHeaderItem and then move to a new item without crashing.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesNotAnimateWhenAnimationsAreDisabled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates turning off global animations will turn off all animations in pivot.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // MOCK10_REMOVAL Disable until we can disable animations without mock10
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFocusFollowerPosition)
            // Pivot focus rect is not following the selection
            TEST_METHOD_PROPERTY(L"Description", L"Validates focusFollower is at the right position when selection changes")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TransitionReentrancyOnDirtyRelayout)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure that relayout during transitions is handled correctly.")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

    private:
        void CanUseKeyboardKeysToChangeSelectedIndexImpl(bool shouldWrap, bool isRTL);
        void ValidatePivotCanWrapWithTouchWhenCarouselIsEnabledImpl(const std::vector<double>& itemsWidth, double viewportSize, bool isHeaderItemsCarouselEnabled);
        void CanStaticHeadersScrollImpl(const std::vector<double>& itemsWidth, double viewportSize);
        void CanChangeHeadersWithGamepadShoulderButtonsImpl(bool isRTL);

        Microsoft::UI::Xaml::Controls::Pivot^ SetupPivotWithItemWidthsAndViewportSize(const std::vector<double>& itemsWidth, double viewportSize, bool isHeaderItemsCarouselEnabled);

        void PressKeyboardLogicalRight(bool isRTL);
        void PressKeyboardLogicalLeft(bool isRTL);

        enum class PivotContent
        {
            NoContent,
            TextBlockContent,
            TextBlockItemTemplateContent,
            TextBoxContent,
            TextBoxExtraContent,
            ListViewContent,
            GridViewContent,
        };

        static xaml_controls::Pivot^ CreatePivot(PivotContent content);
        static xaml_controls::Pivot^ SetupPivotTest(PivotContent content);

        static xaml_primitives::PivotHeaderItem^ FindPivotHeaderItemWithContent(xaml_controls::Pivot^ parentPivot, Platform::String^ content);
    };

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Pivot
