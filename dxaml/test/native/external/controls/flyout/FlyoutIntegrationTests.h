// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Flyout {

    class FlyoutIntegrationTests : public WEX::TestClass<FlyoutIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(FlyoutIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"ab3da967-c6a7-4e5b-b3c4-c53c6c46175c;88333911-8806-45c4-840a-99c755703018;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a Flyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFlyoutOpenCloseProjectedShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the Flyout open and close, projected shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // We're on drop shadows now
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFlyoutOpenCloseDropShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the Flyout open and close, drop shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDependencyProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the Flyout dependency properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDependencyPropertyDefaultValues)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the Flyout dependency property default values.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlyoutPresenterStyle)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the Flyout presenter style.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanAttachedFlyoutShowHide)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the AttachedFlyout can show and hide.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of an open Flyout")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTargetPropertiesForwardedToPresenter)
            TEST_METHOD_PROPERTY(L"Description", L"Certain properties, including FlowDirection and RequestedTheme should get forwarded from the target to the presenter")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyThemeAppliedToPresenterWhenTargetIsInPopup)
            TEST_METHOD_PROPERTY(L"Description", L"When the target is in a Popup, verify that the theme gets propagated to the presenter")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlyoutPropertiesAtOpening)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the Flyout property change at the Opening event.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateIsLightDismissWhenFull)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that an open flyout is still light dismiss when full.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDismissOnPlacementTargetUnloaded)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that an open flyout is hidden when its placement target is unloaded")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLoadedAndUnloadedRaisedAtCorrectTimes)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the contents of a flyout have loaded and unloaded properly raised once and only once on open and close.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanPopupNestedInFlyoutOpenClose)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the nested Popups open and close.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateRTLReversesHorizontalPlacementModes)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting FlowDirection to RTL causes a flyout positioned on the left to appear on the right, and vice versa.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePlacementWithNoOpenPositions)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a Flyout with no completely open positions to any side of its target is placed where the most open space exists.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenChildFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a Flyout can be open from within another Flyout's presenter.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenChildFlyoutWithOpenSibling)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that opening a child Flyout while one of its sibling flyouts is open, closes the sibling and opens the child instead.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanReopenChildFlyoutWithNewPlacementTarget)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a child flyout can be re-opened with a new placement target.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenChildFlyoutTargetingItemInListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a Flyout targeting an item in a ListView can be open from within another Flyout's presenter.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesFlyoutChainCloseWhenOpeningNewRootFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that opening a new root flyout (not the child of an already open flyout) closes an already open flyout chain.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesChildFlyoutCloseWhenPlacementTargetUnloads)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a child flyout unloads when its placement target unloads.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenChildMenuFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a child MenuFlyout can be opened from within another flyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenChildDateTimePickerFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a child Date/TimePickerFlyout can be opened from within another flyout.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenChildCalenderViewFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a child flyout containing a CalendarView can be opened from within another flyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesChainRightClickWithChildFlyouts)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that right-clicking will dismiss an open chain of flyouts and invoke the handler on the next hit-testable element.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: 31563479 - Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCancelClosing)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that it is possible to cancel closing a flyout.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCancelChildClosing)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that it is possible to cancel closing a child flyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLightDismissOverlayMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the behavior of the LightDismissOverlayMode property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesAutoLightDismissOverlayModeCreateOverlayOnXbox)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting LightDismissOverlayMode to Auto on Xbox causes the popup overlay to get configured to On.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the brush used for the Overlay matches the 'FlyoutLightDismissOverlayBackground' resource.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePointerEventsCanBeRoutedThroughLightDismissLayerSetInXaml)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that designating an element as the light-dismiss pass-through element in XAML allows pointer events to route to it through the light-dismiss layer.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePointerEventsCanBeRoutedThroughLightDismissLayerHiDpi)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that designating an element as the light-dismiss pass-through element in XAML allows pointer events to route to it through the light-dismiss layer in HiDpi.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: 31563479 - Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // [DCPPTest] Xaml tests are failing because Xaml no longer applies the plateau scale
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePointerEventsCanBeRoutedThroughLightDismissLayerManuallySet)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that designating an element as the light-dismiss pass-through element in code-behind allows pointer events to route to it through the light-dismiss layer.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: 31563479 - Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePresenterLoadedWithoutShowAtDoesNotCrash)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that OnPresenterLoaded being raised without ShowAt having first been called does not crash the app.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFlyoutTakesThePositionItsSetToTheFirstTime)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the position of the flyout is as set in onOpening method for the first time.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateRightClickCanHideOneFlyoutWithoutHidingAllFlyouts)
            TEST_METHOD_PROPERTY(L"Description", L"Validates if a flyout is opened from another flyout, we can right-click on the original flyout to open a third flyout without that right-click hiding the original flyout.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: 31563479 - Mouse input helper doesn't work on phone/onecore
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLosingFocusClosesFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a flyout closes when it loses focus.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFlyoutPlacementMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that each value of FlyoutPlacementMode has the effect that we expect it to.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFlyoutPlacementModeAtPoint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that each value of FlyoutPlacementMode when opened at a set point has the effect that we expect it to.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFlyoutPlacementModeAtPointWithExclusionRect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that each value of FlyoutPlacementMode when opened at a set point with an exclusion rect has the effect that we expect it to.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFlyoutShowMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that FlyoutShowMode can be set from XAML.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTransientFlyoutDoesNotTakeFocus)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a flyout opened as transient does not take keyboard focus when it opens.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTransientFlyoutLightDismissLayerAllowsInputToPassThrough)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a flyout opened as transient allows input to pass through its light-dismiss layer.")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFlyoutTransientWithDismissOnPointerMoveAway)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that FlyoutShowMode=TransientWithDismissOnPointerMoveAway dismisses properly.")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateContextFlyoutPlacedAtMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that opening a ContextFlyout with the right mouse button causes it to be placed at the mouse cursor, not just on the UI element it's attached to.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyInputDevicePrefersPrimaryCommands)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that InputDevicePrefersPrimaryCommands correctly reports whether or not the flyout was opened with an input device that wants more primary commands.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateHyperlinkTakingFocusDoesNotCloseFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a hyperlink in a flyout taking focus does not close that flyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateThemeChangeWhileOpen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the flyout will respond to a theme change when open.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateHideInOpeningCancelsOpening)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that calling Hide() when opening a flyout cancels the opening.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePressAndHoldOnlyOpensOnce)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that opening a flyout using press-and-hold only opens it once.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateShowAtWindowEdge)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a flyout can be shown at the window's edge at various global scale factors.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateShowBeyondWindowEdge)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that attempts to show a flyout beyond the window's edge at various global scale factors do not throw errors.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFlyoutCanOpenLeftAndUpOutsideWindowBoundaries)
            TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // Isolated because we're changing the size and position of the test window, which could affect other tests.
            TEST_METHOD_PROPERTY(L"Description", L"Validates a flyout that wants to open with a placement of Top such that it will appear above the top of the window can do so.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Enable ValidateFlyoutCanOpenLeftAndUpOutsideWindowBoundaries
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlyoutTakesFocusWithAllowFocusOnInteractionFalseWithUIA)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that focus moves into a Flyout when opened with UIA when AllowFocusOnInteraction=false on the target.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateImmediateRetargeting)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that ShowAt can be called multiple times in a row with different targets.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyContextFlyoutOnTextBlock)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a ContextFlyout on a TextBlock can be shown.")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlyoutOpenedOffTheLeftSideOfTheScreenIsKeptInView)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a flyout opened near the left side of the screen with FlyoutPlacementMode = LeftEdgeAlignedTop is nudged back into view.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAppResourceOverrideContextFlyoutAcceleratorsPerfOptInOn)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that keyboard accelerators on a custom default TextBlock ContextFlyout work with PerfOptIn enabled.")
            TEST_METHOD_PROPERTY(L"Data:PerfOptIn", L"{true}")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAppResourceOverrideContextFlyoutAcceleratorsPerfOptInOff)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that keyboard accelerators on a custom default TextBlock ContextFlyout work with PerfOptIn disabled.")
            TEST_METHOD_PROPERTY(L"Data:PerfOptIn", L"{false}")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNonWindowedFlyoutFlipsUpAlignedToAnchorRowBottom)
            TEST_METHOD_PROPERTY(L"Description", L"When a non-windowed flyout with side placement flips upward at the window's bottom edge, its bottom must align to the anchor row's bottom.")
        END_TEST_METHOD()

    private:
        template <class DateTimePickerType>
        static void CanOpenDateTimePickerWorker();
        static void ValidateFlyoutPopupLightDismissOverlayMode(xaml_controls::LightDismissOverlayMode expectedMode);

        void ValidateShowAtWindowEdge(float scaleFactor, wf::Point offset);

        void ValidatePointerEventsCanBeRoutedThroughLightDismissLayer(
            xaml_controls::Button^ buttonPassThrough,
            xaml_controls::Button^ buttonNonPassThrough,
            xaml_controls::Button^ flyoutTarget);

        void SetupFlyoutPlacementModeTest(xaml_controls::Button^* button, xaml_controls::Flyout^* flyout);

        enum class Theme
        {
            Dark = 0,
            Light = 1,
            HighContrast = 2
        };

        void ValidateUIETreeWorker(Theme theme, Platform::String^ variation);
        void CanFlyoutOpenClose();
        void VerifyAppResourceOverrideContextFlyoutAcceleratorsHelper();
    };

} } } } } }
