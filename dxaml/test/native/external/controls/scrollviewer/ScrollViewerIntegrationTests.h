// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <functional>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ScrollViewer {

    class ScrollViewerIntegrationTests : public WEX::TestClass<ScrollViewerIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ScrollViewerIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a ScrollViewer.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a ScrollViewer from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollToHorizontalOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollToHorizontalOffset properly changes the view.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollToVerticalOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollToVerticalOffset properly changes the view.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CannotScrollToHorizontalOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollToHorizontalOffset does not change the view because horizontal scrollbar visibility is disabled.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CannotScrollToVerticalOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollToVerticalOffset does not change the view because vertical scrollbar visibility is disabled.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanZoomToFactor)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ZoomToFactor properly changes the view after scrolling to the bottom.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSnapToZoomSnapPointWUC)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the ZoomFactor property snaps to a mandatory zoom snap point during inertia.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeViewHorizontally)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ChangeView in the horizontal direction properly changes the view.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeViewVertically)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ChangeView in the vertical direction properly changes the view.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanChangeViewByZooming)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ChangeView can change the zoom factor.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyResponseToOcclusions)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a ScrollViewer can respond to occlusions to help bring text elements into view.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollWithMultipleMouseWheelClicks)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to scroll with multiple successive mouse wheel clicks.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SlightlyChangeZoomFactor)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ChangeView can slightly change the zoom factor during a manipulation.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithHeaders)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer headers stay in sync with the primary content when ChangeView is called during a manipulation.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP Test: Some tests show input differences in RS4 and 19H1
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithMandatorySingleSnapPoints1)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method while using regular, Near-aligned, MandatorySingle scroll snap points.")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36501455
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithMandatorySingleSnapPoints2)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method while using irregular, Near-aligned, MandatorySingle scroll snap points.")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36501455
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithMandatorySingleSnapPoints3)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method while using regular, Center-aligned, MandatorySingle scroll snap points.")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36501455
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithMandatorySingleSnapPoints4)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method while using irregular, Center-aligned, MandatorySingle scroll snap points.")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36501455
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithMandatorySingleSnapPoints5)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method while using regular, Far-aligned, MandatorySingle scroll snap points.")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36501455
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithMandatorySingleSnapPoints6)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method while using irregular, Far-aligned, MandatorySingle scroll snap points.")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36501455
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithMandatorySnapPoints1)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method while using regular, Near-aligned, Mandatory scroll snap points.")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36501455
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithMandatorySnapPoints2)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method while using irregular, Near-aligned, Mandatory scroll snap points.")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36501455
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithMandatorySnapPoints3)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method while using regular, Center-aligned, Mandatory scroll snap points.")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36501455
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithMandatorySnapPoints4)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method while using irregular, Center-aligned, Mandatory scroll snap points.")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36501455
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithMandatorySnapPoints5)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method while using regular, Far-aligned, Mandatory scroll snap points.")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36501455
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithMandatorySnapPoints6)
            TEST_METHOD_PROPERTY(L"Description", L"Exercises the ChangeView method while using irregular, Far-aligned, Mandatory scroll snap points.")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36501455
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PanGestureManipulatableElementWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to interact with a manipulatable element within a ScrollViewer via touch input.")
            // This test was disabled on WindowsCore when we were using TIE.  It is unclear whether this continues to be true now that have eliminated the use
            // of TIE.  Since we cannot be sure it will run on WindowsCore (lifted doesn't run at all there yet), per code review it is being left disabled.
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PanGestureManipulatableElementWithPen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to interact with a manipulatable element within a ScrollViewer via pen input.")
            // This test was disabled on WindowsCore when we were using TIE.  It is unclear whether this continues to be true now that have eliminated the use
            // of TIE.  Since we cannot be sure it will run on WindowsCore (lifted doesn't run at all there yet), per code review it is being left disabled.
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CancelDirectManipulationsWithoutManip)
            TEST_METHOD_PROPERTY(L"Description", L"Invokes CancelDirectManipulations without manipulation.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CancelDirectManipulationsInViewChangingWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Invokes CancelDirectManipulations in a ViewChanging handler for pan with touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CancelDirectManipulationsInViewChangingWithPen)
            TEST_METHOD_PROPERTY(L"Description", L"Invokes CancelDirectManipulations in a ViewChanging handler for pan with pen.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CancelDirectManipulationsForPointerEventsWUC)
            TEST_METHOD_PROPERTY(L"Description", L"Invokes CancelDirectManipulations in a ViewChanging handler and then handles Pointer events.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // [DCPPTest] Some input tests still timing out waiting for events
                                                        // "Waiting for ScrollViewer's vertical pan to complete." time out in Helix
                                                        // "Waiting for ScrollViewer's PointerReleased event." times out in Nebula VM
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PanResizingContentWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer can temporarily activate pan configuration after content resizing with Touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PanResizingContentWithPen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer can temporarily activate pan configuration after content resizing with Pen.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeScrollViewerHeightToZero)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer's Height can be set to 0 after being loaded.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ResetContent)
            TEST_METHOD_PROPERTY(L"Description", L"Resets the ScrollViewer.Content property and ensures the ScrollableHeight properties is then 0.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ReenterContent)
            TEST_METHOD_PROPERTY(L"Description", L"Momentarily sets the ScrollViewer.Content property to null within a single tick and makes sure the rendering does not change.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewTwice)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer.ChangeView can be called twice in a row with different target zoom factors.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewDuringViewChanging1)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer.ChangeView can be called during inertia inside a ViewChanging handler triggered by a ChangeView call.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewDuringViewChanging2)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer.ChangeView can be called inside a ViewChanging handler triggered by a ZoomToFactor call.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #43273714
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewDuringInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer.ChangeView can be called multiple times during inertia.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithAnimationAfterDoubleTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer.ChangeView with animation can be called right after processing a double tap.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // ScrollViewerIntegrationTests::ChangeViewWithAnimationAfterDoubleTap/ChangeViewWithoutAnimationAfterDoubleTap are unreliable.
            TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // DCPP: RS4 Test failure: 4 tests disabled due to Scrollviewer issues
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithoutAnimationAfterDoubleTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer.ChangeView without animation can be called right after processing a double tap.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // ScrollViewerIntegrationTests::ChangeViewWithAnimationAfterDoubleTap/ChangeViewWithoutAnimationAfterDoubleTap are unreliable.
            TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // DCPP: RS4 Test failure: 4 tests disabled due to Scrollviewer issues
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CollapseScrollViewerDuringChangeViewInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that inertia completes properly after collapsing ScrollViewer during a ChangeView inertia.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CollapseContentDuringChangeViewInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that manipulation completes properly after collapsing ScrollViewer.Content during a ChangeView inertia.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CollapseScrollViewerDuringPanWithTouchInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that inertia completes properly after collapsing ScrollViewer during a pan inertia with touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeScrollViewerZIndexDuringPanWithTouchInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that inertia completes properly after changing the Canvas.ZIndex of the ScrollViewer during a pan inertia with touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeScrollViewerZIndexDuringChangeViewInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that inertia completes properly after changing the Canvas.ZIndex of the ScrollViewer during a ChangeView inertia with touch.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeContentZIndexDuringPanWithTouchInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that inertia completes properly after changing the Canvas.ZIndex of the ScrollViewer during a pan inertia with touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CollapseScrollViewerDuringPanWithPenInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that inertia completes properly after collapsing ScrollViewer during a pan inertia with pen.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeScrollViewerZIndexDuringPanWithPenInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that inertia completes properly after changing the Canvas.ZIndex of the ScrollViewer during a pan inertia with pen.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeContentZIndexDuringPanWithPenInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that inertia completes properly after changing the Canvas.ZIndex of the ScrollViewer during a pan inertia with pen.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeContentZIndexDuringChangeViewInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that inertia completes properly after changing the Canvas.ZIndex of the ScrollViewer during a ChangeView inertia.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CollapseContentDuringPanWithTouchInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that manipulation completes properly after collapsing ScrollViewer.Content during a touch pan inertia.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CollapseContentDuringPanWithPenInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that manipulation completes properly after collapsing ScrollViewer.Content during a pen pan inertia.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MakeScrollViewerTransparentDuringChangeViewInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that inertia completes properly after making ScrollViewer transparent during a ChangeView inertia.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MakeContentTransparentDuringChangeViewInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that manipulation completes properly after making ScrollViewer.Content transparent during a ChangeView inertia.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MakeScrollViewerTransparentDuringPanWithTouchInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that inertia completes properly after making ScrollViewer transparent during a touch pan inertia.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MakeScrollViewerTransparentDuringPanWithPenInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that inertia completes properly after making ScrollViewer transparent during a pen pan inertia.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MakeContentTransparentDuringPanWithTouchInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that manipulation completes properly after making ScrollViewer.Content transparent during a touch pan inertia.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MakeContentTransparentDuringPanWithPenInertia)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that manipulation completes properly after making ScrollViewer.Content transparent during a pen pan inertia.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PanWithTouchAndTransparentScrollViewer)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to pan with touch a ScrollViewer with Opacity==0.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PanWithTouchAndTransparentScrollViewerContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to pan with touch a ScrollViewer.Content with Opacity==0.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PanWithPenAndTransparentScrollViewer)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to pan with pen a ScrollViewer with Opacity==0.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PanWithPenAndTransparentScrollViewerContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to pan with pen a ScrollViewer.Content with Opacity==0.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithTransparentScrollViewer)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to invoke ChangeView for a ScrollViewer with Opacity==0.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ChangeViewWithTransparentScrollViewerContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to invoke ChangeView for a ScrollViewer.Content with Opacity==0.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ZoomToFactorWithTransparentScrollViewer)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to invoke ZoomToFactor for a ScrollViewer.Content with Opacity==0.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ZoomToFactorWithTransparentScrollViewerContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to invoke ZoomToFactor for a ScrollViewer.Content with Opacity==0.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ScrollToVerticalOffsetWithTransparentScrollViewer)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to invoke ScrollToVerticalOffset for a ScrollViewer.Content with Opacity==0.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ScrollToVerticalOffsetWithTransparentScrollViewerContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to invoke ScrollToVerticalOffset for a ScrollViewer.Content with Opacity==0.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ViewChangeEventsAreCorrect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the view changing and view changed events contain expected values.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DefaultValuesAreCorrect)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the default values for ScrollViewer properties are correctly set on both platforms.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOverpanSuppression)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that invoking IScrollViewerPrivate::DisableOverpan turns off overpan and IScrollViewerPrivate::EnableOverpan turns it back on.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDMStartedAndCompletedEventsForTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that manipulating a ScrollViewer content with touch triggers the DirectManipulationStarted/Completed events, and programmatic view changes do not.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // [DCPP-test] ScrollViewerIntegrationTests::ValidateDMStartedAndCompletedEventsFor[Touch/Pen]/ZoomInWithChaining still failing
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDMStartedAndCompletedEventsForPen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that manipulating a ScrollViewer content with pen triggers the DirectManipulationStarted/Completed events, and programmatic view changes do not.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // [DCPP-test] ScrollViewerIntegrationTests::ValidateDMStartedAndCompletedEventsFor[Touch/Pen]/ZoomInWithChaining still failing
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDMStartedAndCompletedEventsForOverpan)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that overpanning a ScrollViewer content with touch triggers the DirectManipulationStarted/Completed events.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SizedTextBlock)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a short text in a TextBlock with a large MinWidth pushes the large extent to the owning ScrollViewer.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SizedImage)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a small image with stretch alignment and a large MinWidth pushes the large extent to the owning ScrollViewer.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree_OptimizedStyles)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of ScrollViewer in various visual states.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree_OldStyles)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of ScrollViewer in various visual states.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            TEST_METHOD_PROPERTY(L"Data:XamlOptionalChanges", L"{DefaultStyleOptimizations:false}")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ResizeUnconstrainedScrollViewer)
            TEST_METHOD_PROPERTY(L"Description", L"Validates an unconstrained ScrollViewer inside a GridView with Stretch content alignment can be resized.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(InterruptChangeViewLowVelocityInertiaWithPan)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the events raised when interrupting a ChangeView-triggered low velocity inertia with a pan.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(InterruptPanWithTouchLowVelocityInertiaWithPan)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the events raised when interrupting a touch pan-triggered low velocity inertia with a pan created with touch input.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(InterruptPanWithPenLowVelocityInertiaWithPan)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the events raised when interrupting a pen pan-triggered low velocity inertia with a pan created with pen input.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(InterruptChangeViewLowVelocityInertiaWithTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the events raised when interrupting a ChangeView-triggered low velocity inertia with a tap.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(InterruptPanWithTouchLowVelocityInertiaWithTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the events raised when interrupting a touch pan-triggered low velocity inertia with a tap.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(InterruptPanWithPenLowVelocityInertiaWithTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the events raised when interrupting a pen pan-triggered low velocity inertia with a tap.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AttemptPanWithTouchWithWidthScaledToZero)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a ScrollViewer with a width scaled to 0 cannot be panned with touch.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AttemptPanWithTouchWithHeightScaledToZero)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a ScrollViewer with a height scaled to 0 cannot be panned with touch.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AttemptPanWithPenWithWidthScaledToZero)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a ScrollViewer with a width scaled to 0 cannot be panned with Pen.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AttemptPanWithPenWithHeightScaledToZero)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a ScrollViewer with a height scaled to 0 cannot be panned with Pen.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PanRotatedScrollViewerWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a rotated ScrollViewer can be panned with touch.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Data:Angle", L"{45, 90, 135, 180, 225, 270, 315}")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PanRotatedScrollViewerWithPen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a rotated ScrollViewer can be panned with pen.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Data:Angle", L"{45, 90, 135, 180, 225, 270, 315}")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNoLayoutCycleByChangeContentSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a ScrollViewer hasn't a layout cycle crash by changing the content size.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNoLayoutCycleByChangeAlignment)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a ScrollViewer hasn't a lyaout cycle crash by changing the alignment.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNoLayoutCycleWithScaledMargins)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a ScrollViewer hasn't a lyaout cycle crash when its Content has a Margin at various scale factors.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNoLayoutCycleWithMaxOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Validate ability to pan to bottom of Content without ScrollBar layout cycle, at various scale factors.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScrollBarTrackLengthWithContentChangedH)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a horizontal ScrollBar has the proper track length size with scrolling the content.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScrollBarTrackLengthWithContentChangedV)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a vertical ScrollBar has the proper track length size with scrolling the content.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScrollIndicatorsShowWhenFocusingScrollViewer)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer's scroll indicators show when it initially gets focus.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of ScrollViewer/ScrollBar.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SCPArrangeOverride)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ZoomInWithChaining)
            TEST_METHOD_PROPERTY(L"Description", L"Zoom-in inner ScrollViewer and chain to outer ScrollViewer.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PanWithChainingWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Use touch to pan inner ScrollViewer and chain to outer ScrollViewer.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PanWithChainingWithPen)
            TEST_METHOD_PROPERTY(L"Description", L"Use pen to pan inner ScrollViewer and chain to outer ScrollViewer.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScrollViewerDoesntLeakWhenAddedToItemCollection)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that the ScrollViewer and its content doesn't leak when the ScrollViewer enters both an ItemCollection and the visual tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LeaveReenterTreeOnManipulationStartingWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that the ScrollViewer can leave and re-renter the visual tree when a touch manipulation is starting.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LeaveReenterTreeOnManipulationStartingWithPen)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that the ScrollViewer can leave and re-renter the visual tree when a pen manipulation is starting.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FastScrollingWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that the ScrollViewer can scroll fast among focusable children with the gamepad.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ScrollingWithGamepadWithCanvasAsContentRoot)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that the ScrollViewer can scroll among focusable children with the gamepad while the content root is a Canvas.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateProperInputHandlingWhenNoOpDueToXYFocusProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validate that the ScrollViewer does not mark game pad navigation as handled when it no ops due to XYFocus properties.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LargeChangesWithHeadersHorizontalScroll)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer headers are taken into account when applying large increments and decrements with ScrollBars.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LargeChangesWithHeadersVerticalScroll)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollViewer headers are taken into account when applying large increments and decrements with ScrollBars.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NoDMStartedAndCompletedEventsWithKeyboardOrMouseWheelInput)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that scrolling a ScrollViewer content with the keyboard or the mouse wheel does not trigger the DirectManipulationStarted/Completed events.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // DCPP: RS4 Test failure: 4 tests disabled due to Scrollviewer issues
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MouseWheelInputAfterTap)
            TEST_METHOD_PROPERTY(L"Description", L"Processes PointerMoved events while tapping and mouse-wheeling a ScrollViewer.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGamepadDPadNavigateToPartiallyVisibleItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that auto-focus correctly moves focus to a partially visible candidate instead of scrolling.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NoDisplayedScrollBarWhenContentHeightCrossesViewportHeight)
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // Disabled due to #36625772
            TEST_METHOD_PROPERTY(L"Description", L"Validates that no vertical ScrollBar is momentarily displayed when the ScrollViewer.Content.Height gets larger than ScrollViewer.Height.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DragScrollBarThumbWithPen)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to drag the ScrollBar's thumb with the pen to scroll the ScrollViewer's content.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // DCPP: RS4 Test failure: 4 tests disabled due to Scrollviewer issues
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DisablingScrollViewerStopsThumbDrag)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that disabling the ScrollViewer stops the ongoing ScrollBar Thumb drag.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScrollViewerVisualStates)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ScrollViewer enters the MouseIndicator, TouchIndicator and NoIndicator visual states when scrolling and panning.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ConstrainImageAvailableSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validate basic effect of ScrollContentPresenter's SizesContentToTemplatedParent property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ConstrainVerticalStackPanelAvailableSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validate effect of ScrollContentPresenter's SizesContentToTemplatedParent on vertical StackPanel.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ConstrainHorizontalStackPanelAvailableSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validate effect of ScrollContentPresenter's SizesContentToTemplatedParent on horizontal StackPanel.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCanContentRenderOutsideBounds)
            TEST_METHOD_PROPERTY(L"Description", L"Validate ScrollViewer.CanContentRenderOutsideBounds boolean property.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyScrollviewerCleanDestroy)
            TEST_METHOD_PROPERTY(L"Description", L"Validate ScrollViewer's dtor does not throw.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // WPF_HOSTING_MODE_FAILURE - Creates new UWP view
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScrollviewerKeyboardInteraction)
            TEST_METHOD_PROPERTY(L"Description", L"Validate ScrollViewer scrolls with arrow keys.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScrollViewerTakingFocus)
            TEST_METHOD_PROPERTY(L"Description", L"Validate ScrollViewer does not take focus when clicked, while IsTabStop is False and focused element is within it.")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

    private:
        enum ChangeViewDirection
        {
            ChangeViewDirectionHorizontal,
            ChangeViewDirectionVertical,
            ChangeViewDirectionZoom,
        };

        inline Platform::String^ GetResourcesPath() const;
        xaml_controls::ScrollViewer^ AddScrollViewer(xaml_controls::Orientation scrollViewerOrientation = xaml_controls::Orientation::Vertical, bool disableOverpan = false);

        void CanSnapToZoomSnapPointInternal();
        void CancelDirectManipulationsForPointerEventsInternal();

        void DoScrollToOffset(ChangeViewDirection direction, bool canScroll);
        void DoChangeView(ChangeViewDirection direction);
        void DoChangeViewWithSnapPoints(bool useRegularScrollSnapPoints, xaml_controls::SnapPointsType verticalSnapPointsType, xaml_primitives::SnapPointsAlignment verticalSnapPointsAlignment);
        void ChangeViewDuringInertia(bool targetSameViewTwice);
        void ChangeViewAfterDoubleTap(bool animate);
        void HideElementDuringPanInertia(bool hideScrollViewer, bool changeVisibility, bool panWithTouch);
        void HideElementDuringChangeViewInertia(bool hideScrollViewer, bool changeVisibility);
        void ViewChangeWithTransparentElement(bool transparentScrollViewer, bool pan, bool changeView, bool zoomToFactor, bool scrollToVerticalOffset, bool panWithTouch);
        void InterruptLowVelocityInertia(bool isInertiaFromPan, bool interruptWithPan, bool panWithTouch);
        void ChangeElementZIndexDuringInertia(bool changeScrollViewer, bool isInertiaFromPan, bool panWithTouch);
        void AttemptPanWithScaledScrollViewer(bool forScaledWidth, bool panWithTouch);
        void LargeChangesWithHeadersCommon(
            Platform::String^ scrollBarName,
            Platform::String^ largeDecreaseRepeatButtonName,
            Platform::String^ largeIncreaseRepeatButtonName,
            Platform::String^ smallDecreaseRepeatButtonName,
            Platform::String^ smallIncreaseRepeatButtonName,
            double largeOffset,
            double smallOffset,
            std::function<double(xaml_controls::ScrollViewer^)> offsetGetter);

        void PanGestureManipulatableElement(bool panWithTouch);
        void CancelDirectManipulationsInViewChanging(bool panWithTouch);
        void PanResizingContent(bool panWithTouch);
        void ValidateDMStartedAndCompletedEvents(bool panWithTouch);
        void PanRotatedScrollViewer(bool panWithTouch, int degrees);
        void PanWithChaining(bool panWithTouch);
        void LeaveReenterTreeOnManipulationStarting(bool panWithTouch);
        void ConstrainStackPanelAvailableSize(xaml_controls::Orientation orientation);
        void ValidateNoLayoutCycleWithScaledMargins(float scaleFactor);
        void ValidateNoLayoutCycleWithMaxOffset(float scaleFactor);
        void ValidateScrollBarTrackLengthWithContentChanged(bool isVerticalScenario);
        void ValidateUIElementTreeHelper();
    };

} } } } } }



