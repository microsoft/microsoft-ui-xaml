// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <CustomMetadataRegistrar.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace CommandBar {

    class CommandBarIntegrationTests : public WEX::TestClass<CommandBarIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(CommandBarIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e9192bce-1f8a-48ea-8327-14058db070f2;e6e3a886-04a7-4146-a8a7-26a3e81d503b;cc5953d4-6553-42e5-8c02-80720aa9d842")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a CommandBar.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a CommandBar from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanReapplyTemplate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully reapply a template after the first time we apply it.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndCloseUsingAPI)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CommandBar opens and closes, with appropriate events firing, using IsOpen property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndCloseUsingMoreButtonProjectedShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CommandBar opens and closes, with appropriate events firing, using taps on More Button, projected shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // We're on drop shadows now
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndCloseUsingMoreButtonDropShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CommandBar opens and closes, with appropriate events firing, using taps on More Button, drop shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesCloseOnPrimaryCommandSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CommandBar can close when a primary command is selected from the overflow.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesCloseOnSecondaryCommandSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CommandBar can close when a secondary command is selected from the overflow.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanAddToAndRemoveFromCommandCollections)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that items can be added to the CommandBar's collection properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverflowPlacement)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overflow's open direction and alignment.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanResizeCommandBarAfterOpeningAndClosing)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that resizing the AppBar after opening and closing causes its width to properly get updated.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseLargeAppBarButton)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a CommandBar can use an AppBarButton taller than the app window.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Mouse input helper doesn't work on phone/onecore
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateMoreButtonVisualInDisabledState)
            TEST_METHOD_PROPERTY(L"Description", L"When the CommandBar is Disabled, the more button should be greyed out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateAppBarButtonsHaveInvisibleLabelsWhenClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBarButtons have invisible labels when IsOpen is false.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateAppBarButtonsAreOffsetWithAppBarToggleButtons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBarButtons' text labels are offset to the right when there also are AppBarToggleButtons in the same secondary commands list.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateInlineCommandBarLightDismissBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CommandBars can be placed inline are light-dismissible.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTreeBoth)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the CommandBar UIElement Tree with both Primary and Secondary items.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTreePrimaryOnly)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the CommandBar UIElement Tree with Primary items only.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTreeSecondaryOnly)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the CommandBar UIElement Tree with secondary items only.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PrimaryCommandItemsDoNotDisappear)
            TEST_METHOD_PROPERTY(L"Description", L"Validates a fix for a bug where primary command items would disappear unexpectedly.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverflowSnapsToWindowWidth)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overflow snaps to the window width when it's less than 480.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverflowMaxHeight)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overflow's max height is 50% of the window height.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverflowScrollViewerDoesNotScrollWithArrowKeys)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overflow menu's scrollviewer does not scroll with arrow keys.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesFocusReturnToMoreButtonFromOverflowMenuWhenClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Validates focus returns to the more button when it was previously in the overflow menu when closing.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFirstElementIsNotFocusedWhenClosingCommandBar)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that closing a CommandBar does not result in focus being transferred to the first focusable element in the page.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanMaintainFocusAfterCollectionOrSizeChange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that focused command bar elements stay focused after a collection or size change.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanReopenInClosedHandler)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting CommandBar.IsOpen = true in Closed does not permanently hide labels and the overflow popup as though the CommandBar were still closed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesNotShowMenuIfSecondaryElementsAreCollapsed)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overflow menu is not shown when all the items are Collapsed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesCloseBeforeButtonClickHandlersExecute)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that after clicking on an AppBarButton, the CommandBar closes before the button's click handlers execute.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateClosedMinimalCommandBarWithSecondaryCommandsOnlyIsVisible)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a minimal closed command bar with only secondary commands is visible.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesFocusExpandButtonWithOverflowEscKey)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that once you enter a bottom commandbar overflow, you can exit it by pressing Escape and focus is restored to the Expand Button.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateVisualStateForFullWidthMenu)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the sizing and the border for a full width menu.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MoveItemsBetweenPrimaryAndSecondaryCommands)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that items moved between Primary and Secondary commands go to the correct VisualStates.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth & Height of AppBar in various configurations.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateArrowKeys)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the CommandBar behavior for arrow key presses.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateRightClickBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that non-hidden CommandBars do not open on right-tap, while hidden ones do.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"True")  // Need to disable since this test uses Core Window pointer events that are not supported
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNotTabIntoWhenClosedAndHidden)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that tabbing will not focus the CommandBar when it's ClosedDisplayMode=Hidden and it's closed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesCycleFocusWhenOpen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates CommandBar cycles focus when open.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanTabIntoOverflowMenuWhenTopOrBottom)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a user can tab into the CommandBar's overflow menu when set as a Top/Bottom AppBar.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCommandBarOpensInsideLayoutBounds)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the CommandBar opens down/up based on the available space inside the layout bounds.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDefaultLayoutPositionPropagates)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting DefaultLayoutPosition on the CommandBar propagates down to AppBarButtons and AppBarToggleButtons.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverflowButtonHidesWhenAppropriateWithNoAppBarButtons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the overflow button is hidden when told to be hidden, or when there's nothing to be shown by clicking it, with no app bar buttons.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverflowButtonHidesWhenAppropriateWithPrimaryAppBarButtons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the overflow button is hidden when told to be hidden, or when there's nothing to be shown by clicking it, with primary app bar buttons.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverflowButtonHidesWhenAppropriateWithSecondaryAppBarButtons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the overflow button is hidden when told to be hidden, or when there's nothing to be shown by clicking it, with secondary app bar buttons.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverflowButtonHidesWhenAppropriateWithPrimaryAndSecondaryAppBarButtons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the overflow button is hidden when told to be hidden, or when there's nothing to be shown by clicking it, with both primary and secondary app bar buttons.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDynamicOverflowOnOff)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the dynamic overflow On/Off.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDynamicOverflowByChangingWindowsSizeOverride)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the dynamic overflow with windows size change.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDynamicOverflowAddRemovePrimaryItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the dynamic overflow for adding and removing the primary items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDynamicOverflowAppBarSeparator)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the AppBarSeparator with dynamic overflow operation.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFireDynamicOverflowItemsChangingEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the firing dynamic overflow items changing event.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDynamicOverflowOrderBasic)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the dynamic overflow moving order.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDynamicOverflowOrderTestCases)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the dynamic overflow moving order multiple test cases.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDynamicOverflowWithContentControl)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the layout of CommandBar.Content when IsDynamicOverflowEnabled is true or false.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateVisualStateUpdatesWhenDynamicOverflowCausesItemsToMove)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the CommandBar's visual state is properly updated when items move from the primary commands collection to the secondary commands collection, or vice versa.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDynamicOverflowWithCustomAppBarButton)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the dynamic overflow moving behavior with the CustomAppBarButton that implement ICommandbarElement.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePrimaryButtonWidthAtRightDefaultLabelPosition)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the primary buttons width after applying the overflow style at the Right DefaultLabelPosition.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateMenuSizingForDifferentInputModes)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the size of the CommandBar menu and its items based on different input modes (mouse, touch, etc.).")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: 31563479 - Mouse input helper doesn't work on phone/onecore
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDynamicOverflowWithChangingOrientation)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the dynamic overflow with changing the orientation.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesResetOverflowButtonStylingWhenRemovedAndAddedBack)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a button removed while in the overflow and then re-inserted back into the primary area is not styled with the overflow style.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanArrowIntoTheContentArea)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that arrow keys will navigate you into the content area.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesOpenOverflowWithArrowInputOnMoreButton)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overflow menu is opened when an arrow key is entered with focus on the more button.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSymbolMenuIcons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overflow menu shows Symbol Icons.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePathMenuIcons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overflow menu shows Path Icons.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFontMenuIcons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overflow menu shows Font Icons.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateBitmapMenuIcons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overflow menu shows Bitmap Icons.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateBitmapMenuIconsNoMonochrome)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overflow menu shows Bitmap Icons without the monochrome masking.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCollapsedItemsDoNotPreventReturnFromOverflow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the CommandBar's visual state is properly updated when items move from the primary commands collection to the secondary commands collection, or vice versa.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateMoreButtonCanShowWithoutSizeChanging)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that adding elements to the secondary collection or changing the value of ClosedDisplayMode changes the visibility of the more button.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateButtonsMoveToAndFromOverflowWithoutSizeChange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBarButtons move to and from the overflow as expected when sizing the CommandBar less than the size it minimally needs to display all its content, and then sizing it to be larger than that.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLightDismissWithScaling)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the light dismiss layer covers the entire window when a scale factor is applied")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // TODO:  ChangeDPI has stability issues
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyVisibilityChangeUpdatesCommandBarVisualState)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that changing the visibility of an AppBarButton in a CommandBar correctly updates the visual state")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateResetingTheStateOfAppBarButton)
            TEST_METHOD_PROPERTY(L"Description", L"Validates PointerOver on an appbarbutton doesn't persist after the command bar collapses.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: 31563479 - Mouse input helper doesn't work on phone/onecore
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCanMakeSubMenuBySettingFlyoutProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that setting the Flyout property on an AppBarButton in the overflow functions normally as a sub-menu.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: 31563479 - Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Not working in WPF-hosting, this validation fails intermitently: 
                                                          //    "Moving mouse over MenuFlyoutAppBarButton2, which should close the first menu flyout and open the second."
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySubMenuDoesNotEatPointerInput)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that setting the Flyout property on an AppBarButton and then showing that flyout does not display a light-dismiss layer that prevents interaction with the rest of the CommandBar.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySubMenuHasLightDismissOnPrimaryAppBarButton)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that setting the Flyout property on a primary AppBarButton does not cause that flyout to cease to have a light-dismiss layer.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateForegroundForXamlUICommand)
            TEST_METHOD_PROPERTY(L"Description", L"Verify Foreground is correct for AppBarButton/AppBarToggleButton when XamlUICommand is used")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAppBarButtonFlyoutPosition)
            TEST_METHOD_PROPERTY(L"Description", L"Verify AppBarButton.Flyout, a MenuFlyout, opens at an identical position before and after it is shown in the overflow section.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAppBarButtonFlyoutLightDismiss)
            TEST_METHOD_PROPERTY(L"Description", L"Verify AppBarButton.Flyout, a MenuFlyout, supports light dismiss after it is shown in the overflow section.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAccessKeyBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that AccessKey works correctly with CommandBar.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyClosingSubMenuDoesNotClearFocus)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that closing a sub-menu does not entirely clear keyboard focus in XAML.")
            TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // Adds a handler to a static event, so we'll cordon it off from other tests so it doesn't affect anything else.
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySubMenuOpensLeftInRTL)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that sub-menus open to the left when flow direction is RTL.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySubMenuStaysInViewWhenBothLeftAndRightFailToFit)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that sub-menus still open fully in view even when neither opening fully left nor opening fully right fit in the available space.")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

    private:
        enum class Location
        {
            Inline,
            Top,
            Bottom
        };

        enum class OverflowOpenDirection
        {
            Up = 0,
            Down
        };

        enum class OverflowAlignment
        {
            Left = 0,
            Right
        };

        enum class OpenMethod
        {
            Mouse,
            Touch,
            Keyboard,
            Gamepad,
            Programmatic
        };

        enum class DynamicOverflowOrderTest
        {
            OrderTestForAlternativeValue,           // Test for alternative order - {1,2,1,2,1,2,1,2,1,2}
            OrderTestForAllSameValue,               // Test for all same order - {1,1,1,1,1,1,1,1,1,1}
            OrderTestForTwoPairedValue,             // Test for paired order group - {1,2,3,4,5,1,2,3,4,5}
            OrderTestForFallbackDefault,            // Test for order set and default rightmost dynamic overflow - {1,1,2,2,0,0,0,0,0,0}
            OrderTestForMovingPriorSeparator,       // Test for separator moving that is in the prior index of moving primary command
            OrderTestForMovingPosteriorSeparator    // Test for separator moving that is in the posterior index of moving primary command
        };

        void EmptyPageContent(xaml_controls::Page^ page);

        static xaml_controls::Panel^ ValidateUIElementTestSetup(bool addPrimary, bool addSecondary);
        void ValidateOverflowPlacementWorker(OverflowOpenDirection openDirection, OverflowAlignment alignment, bool isRTL);
        void ValidateOpenAndCloseWorker(std::function<void(xaml_controls::CommandBar^)> openFunc, std::function<void(xaml_controls::CommandBar^)> closeFunc, bool validateDCompTree = false);
        void ValidateRightClickBehaviorWorker(xaml_controls::AppBarClosedDisplayMode closedDisplayMode);
        void ValidateInlineCommandBarOpenDirection(xaml::VerticalAlignment alignment);

        void ValidateOverflowButtonHidesWhenAppropriate(
            bool addPrimary,
            bool addSecondary);

        void ValidateOverflowButtonState(
            bool addPrimary,
            bool addSecondary,
            xaml_controls::AppBarClosedDisplayMode closedDisplayMode,
            xaml_controls::CommandBarDefaultLabelPosition defaultLabelPosition,
            xaml_controls::CommandBarOverflowButtonVisibility overflowButtonVisibility,
            xaml::Visibility expectedOverflowButtonVisibility);

        xaml_controls::Control^ GetMoreButton(xaml_controls::CommandBar^ cmdBar);
        void OpenCommandBar(xaml_controls::CommandBar^ cmdBar, OpenMethod openMethod);
        void CloseCommandBar(xaml_controls::CommandBar^ cmdBar);

        xaml_controls::Page^ SetupDynamicOverflowTest(xaml_controls::CommandBar^* commandBar, unsigned int numButtonsToAddExtraToPrimary, unsigned int numButtonsToAddToSecondary, bool isSetOrder = false);
        xaml_controls::CommandBar^ CreateCommandBarWithPrimaryCommandOrderSet(DynamicOverflowOrderTest orderTestCase);
        void ValidateDynamicOverflowWorker(xaml_controls::CommandBar^ cmdBar, bool isExpectedPrimaryCommandInOverflow, unsigned int numButtonsToAddExtraToPrimary, unsigned int numButtonsToAddToSecondary, bool isSetOrder = false);
        void ValidateDynamicOverflowItemsChangingEventWorker(xaml_controls::CommandBar^ cmdBar, bool isAdding);
        void ValidateDynamicOverflowOrderWorker(xaml_controls::CommandBar^ cmdBar, DynamicOverflowOrderTest orderTestCase);

        void DoesCycleFocusWhenOpenWorker(Location location, size_t numTabs, const wchar_t* expectedFocusSequence);

        Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureDisableTransitionsForTest;

        void RunSecondaryIconTests(Platform::String^ xamlString);

        // This member variable stores the MetadataProvider we'll use to inform Juptier about
        // the custom types in this test.

        Platform::String^ GetUIElementTreeValidationRules()
        {
            return ref new Platform::String(
                LR"(<?xml version='1.0' encoding='UTF-8'?>
                <Rules>
                    <Rule Applicability='//Element' Inclusion='Blacklist'>
                        <Property Name='FocusState'/>
                        <Property Name='IndicatorMode'/>
                        <Property Name='IsPressed'/>
                    </Rule>
                </Rules>)");
        }

        void CanOpenAndCloseUsingMoreButton();
    };

} } } } } }
