// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBar {

    class AppBarIntegrationTests : public WEX::TestClass<AppBarIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AppBarIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"3d58ff19-7f87-4023-8cdf-dcff2b313f3f;88333911-8806-45c4-840a-99c755703018;cc5953d4-6553-42e5-8c02-80720aa9d842")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create an AppBar.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove an AppBar from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndCloseUsingAPI)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBar opens/closes in response to calls to AppBar.IsOpen.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetAndSetClosedDisplayMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the AppBar.ClosedDisplayMode property is accessible and has the correct default value in Threshold.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanHideAppBarWithHiddenClosedDisplayMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting ClosedDisplayMode to Hidden removes the AppBar from the visual tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeAppBarHeightWithClosedDisplayMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting ClosedDisplayMode to Minimal or Compact changes the AppBar's height.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusShiftWhenClosedAppBarIsAdded)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the focus stays on the current focused element and does not shift to the AppBar when a closed AppBar is added dynamically.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusShiftWhenOpenedAppBarIsAdded)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the focus shift between last focused element and an opened AppBar when it is added dynamically.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusShiftWhenClosedUnfocusedAppBarIsOpenedAndClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the focus shift between last focused element and the appBarButton of a closed AppBar when it is opened/closed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFocusShiftWhenClosedFocusedAppBarIsOpenedAndClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the focus stays on the AppBar if it was was already there before the AppBar was opened/closed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateExpandButtonVisualInDisabledState)
            TEST_METHOD_PROPERTY(L"Description", L"When the AppBar is Disabled, the expand button should be greyed out")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanResizeAppBarAfterOpeningAndClosing)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that resizing the AppBar after opening and closing causes its width to properly get updated.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateInlineAppBars)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBars can be placed with other items in the visual tree.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateAppBarWithParentedLTE)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBars can be placed inside a root (example: Popup, or MediaElement) which requires the LTE to be parented explicitly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCloseAppBarUsingGamepadB)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBars can be closed by pressing the B button when using a gamepad.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCloseAppBarUsingEsc)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBars can be closed by pressing the Escape keyboard key.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth & Height of AppBar in various configurations.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanClosedDisplayModesControlLayout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting AppBar.ClosedDisplayMode causes the control to be sized appropriately and to be able to be expanded by tapping on the ellipsis button.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndCloseUsingKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Top and Bottom (and not Inline) AppBars open/close in response to ContextMenu key.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Not working in WPF-hosting: ContextMenu key doesn't trigger this.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCloseNonStickyAppBarUsingEscapeKey)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that only non-sticky AppBars can be closed by using the Escape key.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndCloseUsingMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBar opens/closes in response to mouse input.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndCloseUsingPen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBar opens/closes in response to pen + barrel button.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenMinimalAppBarUsingMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that an AppBar with AppBar.ClosedDisplayMode=Minimal can be opened by clicking the bar itself.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndCloseUsingExpandButton)
            TEST_METHOD_PROPERTY(L"Description", L"Validates tapping on the '...' button opens both AppBars if at least one is closed, and closes them if they're both open.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanTabThroughChildItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Tab navigation works on AppBar child items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanClickAButtonInAnAppBar)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that items in an app bar are clickable.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanClosedDisplayModesAffectTabbingWhenClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting AppBar.ClosedDisplayMode causes the tab experience to be different when closed depending on the visible items that exist.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanClosedDisplayModesAffectTabbingWhenOpen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting AppBar.ClosedDisplayMode causes the tab experience to be different when open depending on the visible items that exist.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateWinBlueTabbingIsPreserved)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting AppBar.ClosedDisplayMode to Hidden and IsSticky to false on all AppBars causes the WinBlue tabbing experience to occur.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNotTabIntoWhenClosedAndHidden)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that tabbing will not focus the AppBar when it's ClosedDisplayMode=Hidden and it's closed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLightDismissOverlayMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the behavior of the LightDismissOverlayMode property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLightDismissOverlayModeForTopBottomAppBars)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the behavior of the LightDismissOverlayMode property when set on Top/Bottom app bars.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(IsAutoLightDismissOverlayModeVisibleOnXbox)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting LightDismissOverlayMode to Auto on Xbox causes the overlay to be visible.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(IsAutoLightDismissOverlayModeVisibleForTopBottomAppBarsOnXbox)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting LightDismissOverlayMode to Auto on Xbox causes the overlay to be visible for Top/Bottom app bars.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayDCompTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates DComp tree with an overlay-enabled app bar.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Not stable between runs; there is a phantom visual that keeps showing up.
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayUIETree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates UIElement tree with an overlay-enabled app bar.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the brush used for the overlay matches the 'AppBarLightDismissOverlayBackground' resource.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayBrushForTopBottomAppBars)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the brush used for the overlay for top/bottom app bars matches the 'AppBarLightDismissOverlayBackground' resource.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAutomationPeerIsOffscreenIsFalseForElementInOpenAppBar)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that AutomationPeer.IsOffScreen returns false for items in an open appbar")
        END_TEST_METHOD()

    private:
        xaml_controls::Page^ SetupClosedDisplayModeTestEnvironment(bool setClosedDisplayModeValues);
        xaml_controls::Page^ SetupFocusShiftTestPage();
        xaml_controls::Page^ SetupTopBottomInlineAppBarsPage();
        xaml_controls::AppBar^ CreateFocusShiftTestAppBar(bool isOpen);
        void CanOpenMinimalAppBarUsingMouseHelper(xaml_controls::AppBar^ appBar);
        void CanOpenAndCloseUsingRightTappedEvent(bool usePen);

        void CanCloseAppBarUsingDevice(InputDevice device);
        void CanCloseAppBarHelper(std::function<void(bool expectedHandledValue, xaml_controls::AppBar^ appbar)> closeFunction, xaml_controls::Page^ page);

        void AttachOpenedAndClosedHandlers(
            xaml_controls::AppBar^& appbar,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& openedEvent,
            SafeEventRegistrationType(xaml_controls::AppBar, Opened)& openedRegistration,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& closedEvent,
            SafeEventRegistrationType(xaml_controls::AppBar, Closed)& closedRegistration);

        static xaml_controls::Panel^ SetupOverlayTreeValidationTest();
        static xaml::FrameworkElement^ GetAppBarOverlayElement(xaml_controls::AppBar^ appBar);
        static void ValidateVisibilityOfOverlayElement(xaml_controls::AppBar^ appBar, bool expectedIsVisible);

        Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureDisableTransitionsForTest;
    };

} } } } } }

