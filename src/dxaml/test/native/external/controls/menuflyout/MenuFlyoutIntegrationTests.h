// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace MenuFlyout {

    class MenuFlyoutIntegrationTests : public WEX::TestClass<MenuFlyoutIntegrationTests>
    {
    public:
        MenuFlyoutIntegrationTests()
            : m_menuCommandParam1("MenuItem1Command")
            , m_menuCommandParam2("ToggleMenuItemCommand")
            , m_menuCommandParam3("SplitMenuItemCommand")
        { };

        BEGIN_TEST_CLASS(MenuFlyoutIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"b914a3e3-0d78-4274-88b9-46a4773bb21b;65d77a94-832e-4d74-af02-4e3b3f5180cc;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a MenuFlyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanMenuFlyoutOpenCloseProjectedShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the MenuFlyout open and close, projected shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // We're on drop shadows now
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanMenuFlyoutOpenCloseDropShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the MenuFlyout open and close, drop shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyMenuFlyoutPresenterStyle)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the MenuFlyout presenter style.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeMenuFlyoutPresenterStyleAtRuntime)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the MenuFlyout presenter style can be changed at runtime.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanAttachedMenuFlyoutShowHide)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the attached MenuFlyout show and hide.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanClickMenuFlyoutItem)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the Click event on MenuFlyoutItem.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanClickToggleMenuFlyoutItem)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the Click event on ToggleMenuFlyoutItem.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanClickSplitMenuFlyoutItem)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the Click event on SplitMenuFlyoutItem.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateShowAtTargetPosition)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the positioning of the MenuFlyout under a range of cases (value of point passed to ShowAt, FlowDirection, input type).")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateShowAtTargetPositionForPen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the positioning of the MenuFlyout opened with Pen under a range of cases (value of point passed to ShowAt, FlowDirection, input type).")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore, OneCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // For menuflyout placement with pen, the placement logic assumes left-handedness
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateShowAtTargetPositionRelativeToElement)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the positioning of the MenuFlyout relative to a UIElement.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TallMenuFlyoutShouldAlignToTopOfScreen)
            TEST_METHOD_PROPERTY(L"Description", L"If a MenuFlyout is too tall to vertically align to the point passed to ShowAt, it will open aligned to the top of the screen.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WideMenuFlyoutShouldAlignToLeftOfScreen)
            TEST_METHOD_PROPERTY(L"Description", L"If a MenuFlyout is too wide to horizontally align to the point passed to ShowAt, it will open aligned to the left of the screen.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // x-coordinate of flyout is 7.5 pixels off
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOnlyOneSubMenuItemIsOpenAtATimeByTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that any currently open sub menus are closed prior to opening a new one.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOnlyOneSplitMenuItemIsOpenAtATimeByTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that any currently open split menu item submenus are closed prior to opening a new one.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSubMenuItemPosition)
            TEST_METHOD_PROPERTY(L"Description", L"Validate a showing sub menu item position.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemPosition)
            TEST_METHOD_PROPERTY(L"Description", L"Validate a showing split menu item position.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNestedSubMenuItemPosition)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the position of a tall nested sub menu.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // WindowHelper->WindowBounds is (-7.5,-7.5), 1040.5 x 736.5
                                                            // Test expects width to be 1024 and height 768
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNestedSplitMenuItemPosition)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the position of a tall nested split menu.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // WindowHelper->WindowBounds is (-7.5,-7.5), 1040.5 x 736.5
                                                            // Test expects width to be 1024 and height 768
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSubMenuItemUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UIElement tree of the MenuFlyoutSubItem.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of MenuFlyout in various visual states ")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateRTLSubMenuItemPosition)
            TEST_METHOD_PROPERTY(L"Description", L"Validate a showing RTL sub menu item position.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateRTLSplitMenuItemPosition)
            TEST_METHOD_PROPERTY(L"Description", L"Validate a showing RTL split menu item position.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSubMenuItemProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validate a MenuFlyoutSubItem properties.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validate a SplitMenuFlyoutItem properties.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateRequestedThemeOnPresenterTakesEffect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting RequestedTheme on the MenuFlyoutPresenter takes effect and is not overridden by RequestedTheme set on the MenuFlyout's owner.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCanChangeRequestedThemeOnPresenterOwner)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that you can change RequestedTheme on the MenuFlyoutPresenter's owner and have it take effect.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateShowAtFlowDirection)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that MenuFlyout gets the correct FlowDirection based on the target element.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateThatLayoutTransitionsDoRun)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that when we open/close a MenuFlyout, theme transitions run.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSubMenuItemByGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a sub menu item with gamepad input.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSubMenuItemByKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a sub menu item with keyboard input.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSubMenuItemByMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a sub menu item with mouse input.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSubMenuItemByRemote)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a sub menu item with remote input.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSubMenuItemByTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a sub menu item with touch input.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemByGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a split menu flyout item with gamepad input.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemByKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a split menu flyout item with keyboard input.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemByMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a split menu flyout item with mouse input.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemByTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a split menu flyout item with touch input.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTraverseMenuFlyoutItemsByGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates traversing through a list of MenuFlyoutItems with gamepad input.")
                                                            // Unstable in Helix
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTraverseMenuFlyoutItemsByKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates traversing through a list of MenuFlyoutItems with keyboard input.")
                                                            // Unstable in Helix
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTraverseSplitMenuFlyoutItemsByGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates traversing through a list of SplitMenuFlyoutItems with gamepad input, including focus on primary and secondary buttons.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTraverseSplitMenuFlyoutItemsByKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates traversing through a list of SplitMenuFlyoutItems with keyboard input, including focus on primary and secondary buttons.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScrollableItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a scrollable MenuFlyout with the long menu items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSubMenuItemInPage)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a sub menu item with the root visual element being a Page.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemInPage)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a split menu item with the root visual element being a Page.")
        END_TEST_METHOD()        

        BEGIN_TEST_METHOD(ValidateCollapsingResetsVisualState)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting Visibility = Collapsed on a menu item resets its visual state.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePopupWindowedPositionInSimulatedHolographic)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the Popup windowed position (with simulated Holographic shell).")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"Desktop") // This test tests popup positions within a window, which doesn't apply with windowed popups.
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSubMenuPositionWithinWindow)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the Popup sub menu position that must be within the window.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"Desktop,WindowsCore") // Windowed popups make this test moot.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSubMenuItemWithLongItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a sub menu item with the long menu items.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemWithLongItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing a split menu item with the long menu items.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOpenMultiSubMenuItemByMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates showing the different multiple sub menu item with mouse input.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateRightClickChaining)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that right clicking away from a MenuFlyout not only dismisses the current flyout, but if a handler is set up, opens a new MenuFlyout.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the brush used for the Overlay matches the 'FlyoutLightDismissOverlayBackground' resource.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePopupWindowedPosition)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the Popup windowed position.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Windowed Popup isn't supported on Phone and OneCore.
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateWindowedPositionNearMonitorEdge)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Windowed Popup is only on desktop
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateShowAtMonitorEdge)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a windowed MenuFlyout can be shown at the monitor's edge at various global scale factors.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Windowed Popup is only on desktop
            TEST_METHOD_PROPERTY(L"RegressionBug", L"19H1:18596040")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFlyoutSizingForDifferentInputModes)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the size of the MenuFlyout and its items based on different input modes (mouse, touch, etc.).")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")  // Disabled on desktop due to reliability issue
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateMenuFlyoutSizeByTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the MenuFlyoutPresenter and MenuFlyoutItem size by the touch input.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateMenuFlyoutSizeByMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the MenuFlyoutPresenter and MenuFlyoutItem size by the mouse input.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: 31563479 - Mouse input helper doesn't work on phone/onecore
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateMenuFlyoutInVisibleBounds)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the MenuFlyout position in the visible bounds area.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCanUseBothShowAtMethods)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that no crash occurs when we use both versions of ShowAt() on the same flyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePopupWindowedScrollingWithMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates scrolling content of windowed MenuFlyout using the mouse.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Windowed Popup isn't supported on Phone and OneCore.
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardNavigationAfterClosingSubMenu)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to navigate through menu items with keyboard after closing sub menu.")
            TEST_METHOD_PROPERTY(L"RegressionBug", L"19H1:20543301")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOpenedSubMenuFocusItemByKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the focused menu item is always the first menu item when the sub menu item is opened.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemInternalKeyboardTraversal)
            TEST_METHOD_PROPERTY(L"Description", L"Validates keyboard traversal and focus management for split menu flyout item using Left and Right Keys.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusedItemDownAndUpAfterOverrideFocusItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the focused menu item down and up navigation after override the focus item at the opened MenuFlyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSingleItemGetsInitialKeyboardFocus)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that having a single MenuFlyoutItem in a MenuFlyout still gets focus as expected when the MenuFlyout opens via keyboard.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateMenuItemsShowIcons)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a menu shows icons correctly.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FocusDoesNotJumpWhenUsingGamepadTriggersFollowedByDPad)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that focus does not unexpectedly jump when using Gamepad triggers followed by DPad input.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDetectChangesToCanExecuteWithoutBeingInVisualTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we properly pick up changes to a command's CanExecute property that occur when we're out of the visual tree once we're added back to the visual tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MenuFlyoutRemainsInBoundsWhenShownTwice)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a MenuFlyout remains within the screen's boundaries when its ShowAt method is called twice in a row.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // y coordinate of flyout is about 8 pixels off.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorCreatesDefaultItemKeyboardAcceleratorText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on a MenuFlyoutItem causes us to generate a default value for KeyboardAcceleratorTextOverride.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorDoesNotOverrideItemCustomKeyboardAcceleratorText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on a MenuFlyoutItem with a value of KeyboardAcceleratorTextOverride already defined does not overwrite that value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorCreatesDefaultToggleItemKeyboardAcceleratorText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on a ToggleMenuFlyoutItem causes us to generate a default value for KeyboardAcceleratorTextOverride.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorDoesNotOverrideToggleItemCustomKeyboardAcceleratorText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on a ToggleMenuFlyoutItem with a value of KeyboardAcceleratorTextOverride already defined does not overwrite that value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorCreatesDefaultSplitItemKeyboardAcceleratorText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on a SplitMenuFlyoutItem causes us to generate a default value for KeyboardAcceleratorTextOverride.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingKeyboardAcceleratorDoesNotOverrideSplitItemCustomKeyboardAcceleratorText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting a KeyboardAccelerator on a SplitMenuFlyoutItem with a value of KeyboardAcceleratorTextOverride already defined does not overwrite that value.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardAcceleratorTextHiddenInSplitMenuItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that keyboard accelerator text is not shown in default SplitMenuFlyoutItem.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDefaultKeyboardAcceleratorTextPopulatesFully)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that having icons, toggle checkmarks, and keyboard accelerators all appearing in a MenuFlyout does not stop later keyboard accelerator text from being populated.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyIsEnabledPropagatesTreeFromCommand)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that IsEnabled=false gets propagated down the visual tree when Command.CanExecute returns false.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingUICommandSetsProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the Command property to a UICommand causes the MenuFlyoutItem to properly pick up the properties from the object.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingUICommandDoesNotOverwriteProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the Command property to a UICommand does not cause a MenuFlyoutItem to overwrite an already-set properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingUICommandSetsToggleProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the Command property to a UICommand causes the ToggleMenuFlyoutItem to properly pick up the properties from the object.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingUICommandDoesNotOverwriteToggleProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the Command property to a UICommand does not cause a ToggleMenuFlyoutItem to overwrite an already-set properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingUICommandSetsSplitProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the Command property to a UICommand causes the SplitMenuFlyoutItem to properly pick up the properties from the object.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSettingUICommandDoesNotOverwriteSplitProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the Command property to a UICommand does not cause a SplitMenuFlyoutItem to overwrite an already-set properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateMenuFlyoutTouchPositionForExclusionRect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that MenuFlyout does not open over exclusion rect in touch mode.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDependencyPropertyDefaultValues)
            TEST_METHOD_PROPERTY(L"Description", L"Verify dependency property default values.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLargeNonWindowedMenuIsPositionedCorrectly)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a non-windowed menu is positioned correctly when it doesn't fit beneath its flyout target.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyMenuFlyoutResizesWhenItemsChangeWhileOpen)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that changing the contents of the Items collection of a MenuFlyout causes the MenuFlyout to resize itself appropriately.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemSubMenuPresenterStyleFromXaml)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a SplitMenuFlyoutItem's SubMenuPresenterStyle set from XAML is applied correctly.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCanChangeSplitMenuItemSubMenuPresenterStyleAtRuntime)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a SplitMenuFlyoutItem's SubMenuPresenterStyle can be changed at runtime and cleared.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemSubMenuPresenterStyleWithItemsWrapGrid)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ItemsWrapGrid is supported as an ItemsPanel in SplitMenuFlyoutItem's SubMenuPresenterStyle.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemSubMenuItemStyleFromXaml)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SubMenuItemStyle is applied to sub menu items when set in XAML.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemSubMenuItemStyleWithLocalStyles)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SubMenuItemStyle is applied correctly when some sub menu items have local styles.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCanChangeSplitMenuItemSubMenuItemStyle)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SubMenuItemStyle can be applied, changed, and cleared at runtime.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemSubMenuItemStyleWithDynamicItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SubMenuItemStyle is applied to dynamically added items with proper handling of local styles.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSplitMenuItemSubMenuItemStyleTypeCompatibility)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SubMenuItemStyle handles type compatibility correctly with different menu item types.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

    private:
        enum class InputMethod
        {
            Gamepad,
            Keyboard,
            Mouse,
            Remote,
            Touch,
            Pen,
        };

        void ValidatePopupWindowedPosition(bool expectWindowedPopup);

        xaml_controls::MenuFlyout^ CreateMenuFlyout(xaml_controls::Primitives::FlyoutPlacementMode placement);

        xaml_controls::MenuFlyout^ CreateMenuFlyout();
        xaml_controls::MenuFlyout^ CreateMenuFlyoutWithSubItem();
        xaml_controls::MenuFlyout^ CreateMenuFlyoutWithSplitItems();

        xaml_controls::MenuFlyout^ CreateMenuFlyoutSubItemsFromXaml();
        xaml_controls::MenuFlyout^ CreateSplitMenuFlyoutItemsFromXaml();
        xaml_controls::MenuFlyout^ CreateMenuFlyoutLongItemsFromXaml();

        xaml_controls::MenuFlyoutSubItem^ CreateMenuFlyoutSubItem();

        void PerformFlowDirectionTest(bool hasElement, bool hasPoint, bool isRTL, bool expectSuccess);

        xaml_controls::MenuFlyoutPresenter^ GetMenuFlyoutPresenter(xaml_controls::MenuFlyout^ menuFlyout);

        void ShowMenuFlyout(xaml_controls::MenuFlyout^ menuFlyout, xaml::UIElement^ relativeTo, float horizontalOffset, float verticalOffset, bool forceTapAsPreviousInputMessage = true);

        void TapSubMenuItem(xaml_controls::MenuFlyoutSubItem^ subItem);
        void MoveToSubMenuItem(xaml_controls::MenuFlyoutSubItem^ subItem);
        void NavigateSubMenu(xaml_controls::MenuFlyout^ menuFlyout, xaml_controls::Button^ button, InputDevice device);

        void TapSplitMenuItemPrimary(xaml_controls::SplitMenuFlyoutItem^ splitItem);
        void TapSplitMenuItemSecondary(xaml_controls::SplitMenuFlyoutItem^ splitItem);
        void MoveToSplitMenuItemPrimary(xaml_controls::SplitMenuFlyoutItem^ splitItem);
        void MoveToSplitMenuItemSecondary(xaml_controls::SplitMenuFlyoutItem^ splitItem);
        void NavigateSplitMenu(xaml_controls::MenuFlyout^ menuFlyout, xaml_controls::Button^ button, InputDevice device);

        void PerformValidateSubMenuItem(InputMethod inputMethod);
        void PerformValidateSplitMenuItem(InputMethod inputMethod);
        void PerformValidateTraverseMenuFlyoutItems(InputMethod inputMethod, boolean goDownFirst);
        void PerformValidateTraverseSplitMenuFlyoutItems(InputMethod inputMethod, boolean goDownFirst);

        xaml_controls::MenuFlyoutSubItem^ GetSubItem(wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items);
        xaml_controls::MenuFlyoutSubItem^ GetSubItem(wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items, int index);
        xaml_controls::MenuFlyoutPresenter^ GetCurrentPresenter();

        xaml_controls::Grid^ SetupRootPanelForSubMenuItemStyleTests();
        xaml_controls::Canvas^ SetupRootPanelForSubMenuTest();

        void GetMenuFlyoutItemsHorizontalPadding(wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items, double &leftPadding, double &rightPadding);
        void VerifyMenuFlyoutItemsPadding(wfc::IVector<xaml_controls::MenuFlyoutItemBase^>^ items, xaml::Thickness expectedPadding);

        void OpenSubItemWithMouse(xaml_controls::MenuFlyoutSubItem^ menuFlyoutSubItem);

        void ValidateMenuFlyoutPosition(::Windows::UI::ViewManagement::ApplicationViewBoundsMode expectedBoundsMode);
        void ValidateShowAtMonitorEdge(float scaleFactor);

        void InjectInput(InputMethod inputMethod);

        enum class HorizontalOpenDirection
        {
            OpenRight,
            OpenLeft,
        };

        enum class VerticalOpenDirection
        {
            OpenUp,
            OpenDown,
        };

        void DoValidateShowAtTargetPosition(
            wf::Point showAtPosition,
            InputMethod inputMethod,
            xaml::FlowDirection flowDirection,
            HorizontalOpenDirection expectedHorizontalOpenDirection,
            VerticalOpenDirection expectedVerticalDirection,
            bool mockLeftHandedness,
            bool disableFullHwndSupport);

        xaml_input::KeyboardAccelerator^ CreateKeyboardAccelerator(::Windows::System::VirtualKey key, ::Windows::System::VirtualKeyModifiers modifiers);

        void CanMenuFlyoutOpenClose();

    private:
        Platform::String^     m_menuCommandParam1;
        Platform::String^     m_menuCommandParam2;
        Platform::String^     m_menuCommandParam3;

        static const UINT m_subMenuOverlapPixels;
    };

} } } } } }
