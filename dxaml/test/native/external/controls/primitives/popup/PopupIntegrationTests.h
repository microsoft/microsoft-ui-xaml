// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace Popup {

    class PopupIntegrationTests : public WEX::TestClass<PopupIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(PopupIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"2b4b21d5-e5c0-4fad-bed4-62030fe191a1;eadeac67-1552-4876-a67e-dc1fcb8a6e25;bdc5c68b-03c1-4217-832c-4c09f99946f4")
            TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a Popup.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanPopupOpenAndClose)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the Popup open and close.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PopupCloseDoesNotClearIsOpenBinding)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that calling Popup.Close() does not clear any bindings on the IsOpen property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesNotChainRightClickWithOpenLightDismissPopup)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that we don't chain right-clicks if we just have a regular light-dismissible popup open (not a flyout).")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Mouse injection is unreliable on OneCore
          END_TEST_METHOD()

        BEGIN_TEST_METHOD(ReplayPointerUpdate_PopupClosedDuringReplay_DoesNotCrash)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a popup synchronously closing a sibling popup during pointer-event replay does not free the XCPListNode the outer iterator is sitting on.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Mouse injection is unreliable on OneCore
          END_TEST_METHOD()

        BEGIN_TEST_METHOD(ReplayPointerUpdate_PopupClosedDuringReplay_WithManyOpenPopups_DoesNotCrash)
            TEST_METHOD_PROPERTY(L"Description", L"Same as ReplayPointerUpdate_PopupClosedDuringReplay_DoesNotCrash but with enough additional open popups (>typicalOpenPopupCount=4) to force the snapshot Jupiter::stack_vector in CPopupRoot::ReplayPointerUpdate to spill to the heap, validating the reentrant-close fix on the heap-allocated path.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Mouse injection is unreliable on OneCore
          END_TEST_METHOD()

        BEGIN_TEST_METHOD(PopupTabStop)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that tab/shift tab on a LightDismiss Popup will update the focus for its children.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoNotSetDefaultFocusOnPhoneWhenPopupIsClosed)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that on phone, we don't want to set default focus when the popup is closed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeOffsetWhenShown)
            TEST_METHOD_PROPERTY(L"Description", L"Verify changing offsets when the popup is shown")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SetSameChild)
            TEST_METHOD_PROPERTY(L"Description", L"Verifty if SetChild can handle the same object twice")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupOpenAndClose)
            TEST_METHOD_PROPERTY(L"Description", L"Verify windowed popup's open and close.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupParentCanvasCompNode)
            TEST_METHOD_PROPERTY(L"Description", L"Verify windowed popup with parent Canvas with CompNode renders correctly")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupHWNDBounds1UR)
            TEST_METHOD_PROPERTY(L"Description", L"Verify bounds of unparented windowed popup HWND with various properties on Popup.Child, Popup.Child = Rectangle")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupHWNDBounds1PR)
            TEST_METHOD_PROPERTY(L"Description", L"Verify bounds of parented windowed popup HWND with various properties on Popup.Child, Popup.Child = Rectangle")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupHWNDBounds1UC)
            TEST_METHOD_PROPERTY(L"Description", L"Verify bounds of unparented windowed popup HWND with various properties on Popup.Child, Popup.Child = Canvas")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupHWNDBounds1PC)
            TEST_METHOD_PROPERTY(L"Description", L"Verify bounds of parented windowed popup HWND with various properties on Popup.Child, Popup.Child = Canvas")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupHWNDBounds2UR)
            TEST_METHOD_PROPERTY(L"Description", L"Verify bounds of unparented windowed popup HWND with various properties on Popup, Popup.Child = Rectangle")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupHWNDBounds2PR)
            TEST_METHOD_PROPERTY(L"Description", L"Verify bounds of parented windowed popup HWND with various properties on Popup, Popup.Child = Rectangle")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupHWNDBounds2UC)
            TEST_METHOD_PROPERTY(L"Description", L"Verify bounds of unparented windowed popup HWND with various properties on Popup, Popup.Child = Canvas")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupHWNDBounds2PC)
            TEST_METHOD_PROPERTY(L"Description", L"Verify bounds of parented windowed popup HWND with various properties on Popup, Popup.Child = Canvas")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupHWNDBoundsNested)
            TEST_METHOD_PROPERTY(L"Description", L"Verify bounds of parented windowed popup HWND, nested Popups")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupHighDPI)
            // TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")   // UAP has an extra transform at the root for the zoom scale
            // Note: Tests that care about DpiAwarenessContext must also make an explicit call to InitializeHost at the start of the test.
            // This test data isn't actually applied until the test method starts, so the test host won't be able to read their values when
            // it normally initializes.
            TEST_METHOD_PROPERTY(L"data:DpiAwarenessContext", L"{Unaware,System,PerMonitor,PerMonitorV2,UnawareGdiScaled}")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PopupInHolographicModeOpenAndClose)
            TEST_METHOD_PROPERTY(L"Description", L"Verify popup's open and close in Holographic mode, windowed mode is disabled.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Visual tree is different when WPF-hosted.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupInput)
            TEST_METHOD_PROPERTY(L"Description", L"Verify Input in windowed popup's content.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupPointerInputCoords)
            TEST_METHOD_PROPERTY(L"Description", L"Verify pointer input coordinates in windowed popup's content.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupPointerInputCoordsRTL)
            TEST_METHOD_PROPERTY(L"Description", L"Verify pointer input coordinates in RTL windowed popup's content.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupUIATree)
            TEST_METHOD_PROPERTY(L"Description", L"Verify windowed popup's UIA tree.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupRTL)
            TEST_METHOD_PROPERTY(L"Description", L"Verify windowed popup renders correctly in RTL flow")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopupMoveSize)
            TEST_METHOD_PROPERTY(L"Description", L"Verify windowed popup renders correctly after moving and resizing")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WindowedPopup3DRequirements)
            TEST_METHOD_PROPERTY(L"Description", L"Verify windowed popup doesn't use a window when it has 3D that changes the direction of x/y axes")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Move windowed popups to lifted input
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PopupRTL)
            TEST_METHOD_PROPERTY(L"Description", L"Verify popup renders correctly in RTL flow")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLightDismissOverlayMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the behavior of the LightDismissOverlayMode property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesAutoLightDismissOverlayModeCreateOverlayOnXbox)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting LightDismissOverlayMode to Auto causes an overlay to get created on Xbox.")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDefaultOverlayBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the default brush used for the Overlay matches the 'PopupLightDismissOverlayBackground' resource.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayTreePlacement)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overlay element is placed behind the popup's child under the popup root.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeLightDismissOverlayModeWhileOpen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that an app can change the value of LightDismissOverlayMode while a popup is open.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesOverlaySizeWithWindow)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the overlay sizes with the window.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")  // Test uses Window.Current, which is null in wpf hosting mode.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSetChildWhileOpenWithOverlayEnabled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that setting the popup's child while it is open and the overlay is enabled correctly inserts the overlay and child under the popup root.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayDCompTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates DComp tree with an overlay-enabled popup.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverlayUIETree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates UIElement tree with an overlay-enabled popup.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Fails when WPF-hosted because Xaml has Popup IsOpen="true",
                                                            // but when I fix this the baseline file can't be compared.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(OpenPopupUnderCollapsedParent)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure a Popup opened under a parent with Visibility = Collapsed shows up correctly when uncollapsing")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(HostBackdropBrush)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure a Popup with HostBackdropBrush renders correctly")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // [DCPP-test] Delete TIE and HostBackdrop tests
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(HostBackdropBrushWindowed)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure a Windowed Popup with HostBackdropBrush renders correctly")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // [DCPP-test] Delete TIE and HostBackdrop tests
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(HostBackdropBrushLTE)
            TEST_METHOD_PROPERTY(L"Description", L"Ensure a Popup with HostBackdropBrush and targeted by LTE renders correctly")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // [DCPP-test] Delete TIE and HostBackdrop tests
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ElevationTranslate)
            TEST_METHOD_PROPERTY(L"Description", L"Apply elevation effect via Translation property")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // [DCPP-test] Delete TIE and HostBackdrop tests
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ElevationTranslateNoAnim)
            TEST_METHOD_PROPERTY(L"Description", L"Apply elevation effect via Translation property, no animation")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // [DCPP-test] Delete TIE and HostBackdrop tests
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PopupCycleDoesntInfiniteLoop)
            TEST_METHOD_PROPERTY(L"Description", L"When a popup has a cycle, make sure we don't infinite-loop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TallPopupPlacedRelativeToAnotherPopupIsBroughtInView)
            TEST_METHOD_PROPERTY(L"Description", L"When a tall popup is placed relative to another popup in the center such that it doesn't have enough space either above or below, make sure we move the second popup to give it space.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PopupSizeIncreaseDoesntChangePosition)
            TEST_METHOD_PROPERTY(L"Description", L"When a popup increases in size after being opened, we should not be swapping its calculated position.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(InitializeWithIsOpenTrueDoesNotCrash)
            TEST_METHOD_PROPERTY(L"Description", L"Verify a Popup initialized with IsOpen='True' during parsing (IsOpen set before the child) does not crash and opens once it enters the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(InitializeWithChildBeforeIsOpenDoesNotCrash)
            TEST_METHOD_PROPERTY(L"Description", L"Verify a Popup whose Child is parsed before IsOpen='True' (property-element ordering) does not crash and opens once it enters the live tree.")
        END_TEST_METHOD()

    private:
        void PopupRTLHelper(bool isWindowed);
        void WindowedPopupOpenAndCloseHelper();
        void HostBackdropBrushHelper(bool isWindowed, bool useLTE);
        void ElevationHelper(bool animate);
        void WindowedPopupHWNDBounds1Helper(bool isParented, bool popupChildIsCanvas);
        void WindowedPopupHWNDBounds2Helper(bool isParented, bool popupChildIsCanvas);
        void WindowedPopupPointerInputCoordsHelper(bool isRTL);
        void ReplayPointerUpdate_PopupClosedDuringReplay_DoesNotCrashHelper(size_t extraOpenPopupCount);
    };

} } } } } } }

