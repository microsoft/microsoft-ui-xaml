// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ToolTip {

    class ToolTipIntegrationTests : public WEX::TestClass<ToolTipIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ToolTipIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286;57e0de30-efb3-4001-9ccc-b38032fd1974;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a ToolTip.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a ToolTip from the live tree.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetToolTipBeforeAndAfterManagedPeerCreation)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that no crash occurs when attempting to set a tool tip both before and after this element's managed peer is created.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD();

        BEGIN_TEST_METHOD(CanSetAndGetOffsetProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully set/get the ToolTip Offset properties.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD();

        BEGIN_TEST_METHOD(ValidateUIETreeForKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of ToolTip in various visual states when opened by a keyboard.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Theme changes not propagating to open tooltip when using RequestedTheme
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIETreeForTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of ToolTip in various visual states when opened by touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Theme changes not propagating to open tooltip when using RequestedTheme
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CloseToolTipBeforeItIsFullyOpen)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a tooltip does not remain on the screen if it is closed before it has finished opening.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnableAndDisableOpenedToolTip)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that enabling and disabling of an opened ToolTip makes it appear and disappear respectively.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndCloseDisabledToolTip)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that opening and closing of a disabled ToolTip does not make it appear.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPlacementTargetIsHonored)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that setting ToolTipService.PlacementTarget on a ToolTip's owner causes it to appear at that placement target when shown.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenToolTipWithKeyboardFocusWithMouse)
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore, OneCore")
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a ToolTip is shown when its owner receives keyboard focus.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenToolTipWithKeyboardFocusWithoutMouse)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"OneCore") //The onecore version of the above test, since Mouse input helpers don't work
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a ToolTip is shown when its owner receives keyboard focus.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyToolTipPlacedCorrectlyWhenRTL)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that an RTL ToolTip is placed at the same position as an LTR ToolTip.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyToolTipAlwaysInViewWithMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a ToolTip positioned such that it would normally appear outside of the window will have its positioning modified so that it appears within the window instead.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore, OneCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyToolTipAlwaysInViewWithoutMouse)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"OneCore") //The onecore version of the above test, since Mouse input helpers don't work
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a ToolTip positioned such that it would normally appear outside of the window will have its positioning modified so that it appears within the window instead.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyToolTipClippedIfLargerThanWindowWithMouse)
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore, OneCore")// TODO: Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a ToolTip that is larger than the window is clipped before being displayed.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyToolTipClippedIfLargerThanWindowWithoutMouse)
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"OneCore") //The onecore version of the above test, since Mouse input helpers don't work
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a ToolTip that is larger than the window is clipped before being displayed.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Clipping issue
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyToolTipIsRespositionedOnRootVisualSizeChange)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a ToolTip is repositioned if the size of the root visual changes.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Disabled. In the new implementation, ToolTip is closed and change is covered by VerifyControlPositionChangeDismissToolTip
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIETreeForMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of ToolTip in various visual states when opened by a mouse.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Popup's VerticalOffset is off by 1 pixel
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Theme changes not propagating to open tooltip when using RequestedTheme
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNestedToolTips)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ToolTips applied to nested UIElements behave correctly.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanToolTipOpenCloseProjectedShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the ToolTip open and close, in projected shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // We're on drop shadows now
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanToolTipOpenCloseDropShadow)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the ToolTip open and close, in drop shadow mode.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore,Santorini")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Re-enable after fixing unreliability.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyToolTipPlacement)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the ToolTip placement target.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPlacementModeMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the ToolTip placement mode mouse.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore, WindowsCore")// TODO: Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOutOfWindowToolTipPlacement)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the out of Xaml window ToolTip placement.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")   // Tool tip bounds mismatch
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOutOfWindowRTLToolTipPlacementWithMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the out of Xaml window RTL ToolTip placement with mouse.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOutOfWindowToolTipPlacementWithMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the out of Xaml window ToolTip placement with mouse.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScaledToolTipPlacement)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the scaled ToolTip placement.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateRTLToolTipPlacement)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the RightToLeft ToolTip placement.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateToolTipPositionOnSliderWithMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ToolTip position on the Slider control with mouse.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
         END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePlacementWithoutValidHWnd)
            TEST_METHOD_PROPERTY(L"Description", L"Move the mouse over an open ToolTip and let it close. Opening the next ToolTip should succeed.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF") // TODO: Converge windowed Popups
                                                          // Input on an island's windowed popup doesn't pass through to the island content.
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // ToolTip: ValidatePlacementWithoutValidHWnd and ValidatePointerOverToolTip, uap only tests, are flaky
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetToolTipOnNonStatefulObjects)
            TEST_METHOD_PROPERTY(L"Description", L"Verify we can set ToolTips for objects that normally don't have any peer state.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ShowToolTipFromWindowedPopup)
            TEST_METHOD_PROPERTY(L"Description", L"Validate the position of ToolTip that is shown from a windowed popup (e.g. a MenuFlyout).")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyToolTipOnlyShowsProgramaticallyOnXbox)
            TEST_METHOD_PROPERTY(L"Description", L"On XBox, ToolTipService should never open a ToolTip, they must be opened programatically.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCanPlaceToolTipOnHyperlink)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a ToolTip placed on a Hyperlink text element works properly.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // TODO: Mouse input helper doesn't work on phone/onecore
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTooltipIsNotTopmost)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that for a ToolTip in regular Xaml app, the popup window is not topmost in z-order.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Windowed Popup isn't supported on OneCore.
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateTooltipRepositionsAfterWindowPositionChange)
            TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // Isolated because it messes with the test app window, which we don't want to affect other tests.
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Disabled pending investigation.
            TEST_METHOD_PROPERTY(L"Description", L"Validates that after window position changes, an open tooltip correctly updates its position.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePlacementRectWithKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting PlacementRect on a ToolTip overrides the bounds of its target.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePlacementRectWithMouse)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting PlacementRect on a ToolTip causes a mouse-positioned tooltip to be pushed outside of the rect.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePlacementRectChangeInSizeChanged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting PlacementRect on a ToolTip in its ToolTip.SizeChanged handler takes effect immediately.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenToolTipWithUIASetFocus)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that setting focus to the tooltip target with UIA causes the ToolTip to show")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(OpenCloseOpen)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")   // ToolTip bounds mismatch
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateOnlyCtrlDimissToolTip)
            TEST_METHOD_PROPERTY(L"Description", L"Verify only Ctrl Down+Ctrl Up dismiss ToolTip.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore, OneCore, Santorini")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePointerOverToolTip)
            TEST_METHOD_PROPERTY(L"Description", L"Verify Pointer can move over ToolTip.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore, OneCore, Santorini")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // ToolTip: ValidatePointerOverToolTip test doesn't work with WPF and latest changes
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // ToolTip: ValidatePlacementWithoutValidHWnd and ValidatePointerOverToolTip, uap only tests, are flaky
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyControlPositionChangeDismissToolTip)
            TEST_METHOD_PROPERTY(L"Description", L"Verify control position change will dimiss ToolTip.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore, OneCore, Santorini")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPointerMoveInsideControlNotDismissToolTip)
            TEST_METHOD_PROPERTY(L"Description", L"Verify Pointer move inside control doesn't dimiss ToolTip.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore, OneCore, Santorini")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

    private:
        enum class InputMode
        {
            None,
            Touch,
            Mouse,
            Keyboard,
            UIA
        };

        static xaml_controls::Button^ SetupToolTipTest();
        xaml_controls::Button^ SetupOutOfWindowToolTipTest();
        xaml_controls::Button^ SetupScaledToolTipTest();

        static xaml_controls::ToolTip^ CreateToolTip();
        static xaml_controls::ToolTip^ CreateLongToolTip();
        static xaml_controls::ToolTip^ CreateTallToolTip();

        static void OpenToolTip(
            xaml_controls::Button^ button,
            xaml_controls::ToolTip^ toolTip,
            InputMode inputMode,
            bool isTargetPosition = false,
            wf::Point point = wf::Point(0,0));

        static void CloseToolTip(xaml_controls::ToolTip^ toolTip, InputMode inputmode);

        static wf::Rect GetToolTipBounds(xaml_controls::ToolTip^ toolTip);

        void ToolTipIntegrationTests::PerformToolTipPlacement(
            xaml_controls::Button^ button,
            xaml_controls::ToolTip^ toolTip,
            xaml_primitives::PlacementMode mode,
            xaml_primitives::PlacementMode modeExpected,
            InputMode inputMode,
            bool validateDCompTree = false);

        void VerifyToolTipPosition(
            FrameworkElement^ target,
            FrameworkElement^ toolTip,
            xaml_primitives::PlacementMode modeExpected);

        void ValidateUIETree(InputMode mode);

        void ValidateTooltipIsTopmostHelper(bool componentHosted);

        void CanOpenToolTipWithKeyboardFocus(bool withMouse);
        void VerifyToolTipAlwaysInView(bool withMouse);
        void VerifyToolTipClippedIfLargerThanWindow(bool withMouse);

        void CanToolTipOpenClose();

        static const int ToolTipDefaultMouseOffset = 20;
    };

} } } } } }
