// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System.Threading;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using ScrollSnapPointsAlignment = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointsAlignment;
using ScrollSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPoint;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using ScrollingScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollCompletedEventArgs;
using ScrollingBringingIntoViewEventArgs = Microsoft.UI.Xaml.Controls.ScrollingBringingIntoViewEventArgs;
using ScrollingPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooks;
using ScrollingPresenterViewChangeResult = Microsoft.UI.Private.Controls.ScrollingPresenterViewChangeResult;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollingPresenterTests
    {
        private const double c_defaultBringIntoViewUIScrollingPresenterNonConstrainedSize = 600.0;
        private const double c_defaultBringIntoViewUIScrollingPresenterConstrainedSize = 300.0;
        private const int c_defaultBringIntoViewUIStackPanelChildrenCount = 16;

        [TestMethod]
        [TestProperty("Description", "Brings an element within a horizontal ScrollingPresenter into view.")]
        public void BringElementIntoHorizontalScrollingPresenterViewFromNearEdge()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoView(Orientation.Horizontal, 1083.0 /*expectedHorizontalOffset*/, 0.0 /*expectedVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a horizontal ScrollingPresenter into view, with snap points.")]
        public void BringElementIntoHorizontalScrollingPresenterViewWithSnapPoints()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoView(
                orientation: Orientation.Horizontal,
                expectedHorizontalOffset: 1134.0,
                expectedVerticalOffset: 0.0,
                options: null,
                originalHorizontalOffset: 0.0,
                originalVerticalOffset: 0.0,
                originalZoomFactor: 1.0f,
                applyOptionsInBringingIntoViewHandler: false,
                applySnapPointsInBringingIntoViewHandler: true);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a vertical ScrollingPresenter into view.")]
        public void BringElementIntoVerticalScrollingPresenterViewFromNearEdge()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoView(Orientation.Vertical, 0.0 /*expectedHorizontalOffset*/, 1083.0 /*expectedVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a vertical ScrollingPresenter into view, with snap points.")]
        public void BringElementIntoVerticalScrollingPresenterViewWithSnapPoints()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoView(
                orientation: Orientation.Vertical,
                expectedHorizontalOffset: 0.0,
                expectedVerticalOffset: 1134.0,
                options: null,
                originalHorizontalOffset: 0.0,
                originalVerticalOffset: 0.0,
                originalZoomFactor: 1.0f,
                applyOptionsInBringingIntoViewHandler: false,
                applySnapPointsInBringingIntoViewHandler: true);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a horizontal ScrollingPresenter into view, starting from the maximum offset.")]
        public void BringElementIntoHorizontalScrollingPresenterViewFromFarEdge()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoView(
                Orientation.Horizontal,
                3126.0 /*expectedHorizontalOffset*/,
                130.0 /*expectedVerticalOffset*/,
                null /*options*/,
                3624.0 /*originalHorizontalOffset*/,
                0.0 /*originalVerticalOffset*/,
                2.0f /*originalZoomFactor*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a vertical ScrollingPresenter into view, starting from the maximum offset.")]
        public void BringElementIntoVerticalScrollingPresenterViewFromFarEdge()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoView(
                Orientation.Vertical,
                130.0 /*expectedHorizontalOffset*/,
                3126.0 /*expectedVerticalOffset*/,
                null /*options*/,
                0.0 /*originalHorizontalOffset*/,
                3624.0 /*originalVerticalOffset*/,
                2.0f /*originalZoomFactor*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a horizontal ScrollingPresenter into view, with left alignment.")]
        public void BringElementIntoHorizontalScrollingPresenterViewWithNearAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoViewWithAlignment(
                Orientation.Horizontal,
                0.0 /*alignmentRatio*/,
                1512.0 /*expectedHorizontalOffset*/,
                0.0 /*expectedVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a horizontal ScrollingPresenter into view, with center alignment.")]
        public void BringElementIntoHorizontalScrollingPresenterViewWithMiddleAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoViewWithAlignment(
                Orientation.Horizontal,
                0.5 /*alignmentRatio*/,
                1323.0 /*expectedHorizontalOffset*/,
                0.0 /*expectedVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a horizontal ScrollingPresenter into view, with right alignment.")]
        public void BringElementIntoHorizontalScrollingPresenterViewWithFarAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoViewWithAlignment(
                Orientation.Horizontal,
                1.0 /*alignmentRatio*/,
                1083.0 /*expectedHorizontalOffset*/,
                0.0 /*expectedVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a vertical ScrollingPresenter into view, with left alignment.")]
        public void BringElementIntoVerticalScrollingPresenterViewWithNearAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoViewWithAlignment(
                Orientation.Vertical,
                0.0 /*alignmentRatio*/,
                0.0 /*expectedHorizontalOffset*/,
                1512.0 /*expectedVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a vertical ScrollingPresenter into view, with center alignment.")]
        public void BringElementIntoVerticalScrollingPresenterViewWithMiddleAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoViewWithAlignment(
                Orientation.Vertical,
                0.5 /*alignmentRatio*/,
                0.0 /*expectedHorizontalOffset*/,
                1323.0 /*expectedVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a vertical ScrollingPresenter into view, with right alignment.")]
        public void BringElementIntoVerticalScrollingPresenterViewWithFarAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoViewWithAlignment(
                Orientation.Vertical,
                1.0 /*alignmentRatio*/,
                0.0 /*expectedHorizontalOffset*/,
                1083.0 /*expectedVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a horizontal ScrollingPresenter into view, with a shift.")]
        public void BringElementIntoHorizontalScrollingPresenterViewWithOffset()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoViewWithOffset(
                Orientation.Horizontal,
                10.0 /*offset*/,
                1073.0 /*expectedHorizontalOffset*/,
                0.0 /*expectedVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a vertical ScrollingPresenter into view, with a shift.")]
        public void BringElementIntoVerticalScrollingPresenterViewWithOffset()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoViewWithOffset(
                Orientation.Vertical,
                -10.0 /*offset*/,
                0.0 /*expectedHorizontalOffset*/,
                1093.0 /*expectedVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a horizontal ScrollingPresenter into view, with a shift, starting from the maximum offset.")]
        public void BringElementIntoHorizontalScrollingPresenterViewWithOffsetFromFarEdge()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoViewWithOffset(
                Orientation.Horizontal,
                10.0 /*offset*/,
                3116.0 /*expectedHorizontalOffset*/,
                130.0 /*expectedVerticalOffset*/,
                3624.0 /*originalHorizontalOffset*/,
                0.0 /*originalVerticalOffset*/,
                2.0f /*originalZoomFactor*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a vertical ScrollingPresenter into view, with a shift, starting from the maximum offset.")]
        public void BringElementIntoVerticalScrollingPresenterViewWithOffsetFromFarEdge()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoViewWithOffset(
                Orientation.Vertical,
                -10.0 /*offset*/,
                130.0 /*expectedHorizontalOffset*/,
                3136.0 /*expectedVerticalOffset*/,
                0.0 /*originalHorizontalOffset*/,
                3624.0 /*originalVerticalOffset*/,
                2.0f /*originalZoomFactor*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a ScrollingPresenter into view, with a TargetRect.")]
        public void BringElementIntoViewWithTargetRect()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringIntoViewOptions options = null;

            RunOnUIThread.Execute(() =>
            {
                options = new BringIntoViewOptions();
                options.TargetRect = new Rect(15, 0, 50, 100);
            });

            BringElementIntoView(
                Orientation.Horizontal,
                1028.0 /*expectedHorizontalOffset*/,
                0.0 /*expectedVerticalOffset*/,
                options);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element within a ScrollingPresenter into view, with a TargetRect and VerticalOffset.")]
        public void BringElementIntoViewWithAdjustmentInBringingIntoViewHandler()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringIntoViewOptions options = null;

            RunOnUIThread.Execute(() =>
            {
                options = new BringIntoViewOptions();
                options.TargetRect = new Rect(0, -15, 100, 50);
                options.AnimationDesired = false;
                options.VerticalOffset = -10.0;
            });

            BringElementIntoView(
                Orientation.Vertical,
                0.0 /*expectedHorizontalOffset*/,
                1008.0 /*expectedVerticalOffset*/,
                options,
                0.0 /*originalHorizontalOffset*/,
                0.0 /*originalVerticalOffset*/,
                1.0f /*originalZoomFactor*/,
                true /*applyOptionsInBringingIntoViewHandler*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside horizontal ScrollingPresenters into view.")]
        public void BringNestedElementIntoHorizontalScrollingPresenterView()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementInNestedScrollingPresentersIntoView(
                Orientation.Horizontal,
                1056.0 /*expectedOuterHorizontalOffset*/,
                0.0 /*expectedOuterVerticalOffset*/,
                1083.0 /*expectedInnerHorizontalOffset*/,
                0.0 /*expectedInnerVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside vertical ScrollingPresenters into view.")]
        public void BringNestedElementIntoVerticalScrollingPresenterView()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementInNestedScrollingPresentersIntoView(
                Orientation.Vertical,
                0.0 /*expectedOuterHorizontalOffset*/,
                1056.0 /*expectedOuterVerticalOffset*/,
                0.0 /*expectedInnerHorizontalOffset*/,
                1083.0 /*expectedInnerVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside horizontal ScrollViewers into view.")]
        public void BringNestedElementIntoHorizontalScrollViewerView()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementInNestedScrollViewersIntoView(
                Orientation.Horizontal,
                1056.0 /*expectedOuterHorizontalOffset*/,
                0.0 /*expectedOuterVerticalOffset*/,
                1083.0 /*expectedInnerHorizontalOffset*/,
                0.0 /*expectedInnerVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside vertical ScrollViewers into view.")]
        public void BringNestedElementIntoVerticalScrollViewerView()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementInNestedScrollViewersIntoView(
                Orientation.Vertical,
                0.0 /*expectedOuterHorizontalOffset*/,
                1056.0 /*expectedOuterVerticalOffset*/,
                0.0 /*expectedInnerHorizontalOffset*/,
                1083.0 /*expectedInnerVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside horizontal ScrollingPresenters into view, starting from the maximum offsets.")]
        public void BringNestedElementIntoHorizontalScrollingPresenterViewFromFarEdge()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementInNestedScrollingPresentersIntoView(
                Orientation.Horizontal,
                528.0 /*expectedOuterHorizontalOffset*/,
                0.0 /*expectedOuterVerticalOffset*/,
                2344.5 /*expectedInnerHorizontalOffset*/,
                52.5 /*expectedInnerVerticalOffset*/,
                null /*options*/,
                756.0 /*originalOuterHorizontalOffset*/,
                0.0 /*originalOuterVerticalOffset*/,
                0.5f /*originalOuterZoomFactor*/,
                2568.0 /*originalInnerHorizontalOffset*/,
                0.0 /*originalInnerVerticalOffset*/,
                1.5f /*originalInnerZoomFactor*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside vertical ScrollingPresenters into view, starting from the maximum offsets.")]
        public void BringNestedElementIntoVerticalScrollingPresenterViewFromFarEdge()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementInNestedScrollingPresentersIntoView(
                Orientation.Vertical,
                0.0 /*expectedOuterHorizontalOffset*/,
                528.0 /*expectedOuterVerticalOffset*/,
                52.5 /*expectedInnerHorizontalOffset*/,
                2344.5 /*expectedInnerVerticalOffset*/,
                null /*options*/,
                0.0 /*originalOuterHorizontalOffset*/,
                756.0 /*originalOuterVerticalOffset*/,
                0.5f /*originalOuterZoomFactor*/,
                0.0 /*originalInnerHorizontalOffset*/,
                2568.0 /*originalInnerVerticalOffset*/,
                1.5f /*originalInnerZoomFactor*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside horizontal ScrollingPresenters into view, with left alignment.")]
        public void BringNestedElementIntoHorizontalScrollingPresenterViewWithNearAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringIntoViewOptions options = null;

            RunOnUIThread.Execute(() =>
            {
                options = new BringIntoViewOptions() { HorizontalAlignmentRatio = 0.0 };
            });

            BringElementInNestedScrollingPresentersIntoView(
                Orientation.Horizontal,
                1107.0 /*expectedOuterHorizontalOffset*/,
                0.0 /*expectedOuterVerticalOffset*/,
                1512.0 /*expectedInnerHorizontalOffset*/,
                0.0 /*expectedInnerVerticalOffset*/,
                options);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside horizontal ScrollingPresenters into view, with snap points.")]
        public void BringNestedElementIntoHorizontalScrollingPresenterViewWithSnapPoints()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementInNestedScrollingPresentersIntoView(
                orientation: Orientation.Horizontal,
                expectedOuterHorizontalOffset: 1008.0,
                expectedOuterVerticalOffset: 0.0,
                expectedInnerHorizontalOffset: 1134.0,
                expectedInnerVerticalOffset: 0.0,
                options: null,
                originalOuterHorizontalOffset: 0.0,
                originalOuterVerticalOffset: 0.0,
                originalOuterZoomFactor: 1.0f,
                originalInnerHorizontalOffset: 0.0,
                originalInnerVerticalOffset: 0.0,
                originalInnerZoomFactor: 1.0f,
                applySnapPointsInBringingIntoViewHandler: true);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside vertical ScrollingPresenters into view, with center alignment.")]
        public void BringNestedElementIntoVerticalScrollingPresenterViewWithMiddleAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringIntoViewOptions options = null;

            RunOnUIThread.Execute(() =>
            {
                options = new BringIntoViewOptions() { VerticalAlignmentRatio = 0.5 };
            });

            BringElementInNestedScrollingPresentersIntoView(
                Orientation.Vertical,
                0.0 /*expectedOuterHorizontalOffset*/,
                1056.0 /*expectedOuterVerticalOffset*/,
                0.0 /*expectedInnerHorizontalOffset*/,
                1323.0 /*expectedInnerVerticalOffset*/,
                options);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside horizontal ScrollViewers into view, with left alignment.")]
        public void BringNestedElementIntoHorizontalScrollViewerViewWithNearAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringIntoViewOptions options = null;

            RunOnUIThread.Execute(() =>
            {
                options = new BringIntoViewOptions() { HorizontalAlignmentRatio = 0.0 };
            });

            BringElementInNestedScrollViewersIntoView(
                Orientation.Horizontal,
                1107.0 /*expectedOuterHorizontalOffset*/,
                0.0 /*expectedOuterVerticalOffset*/,
                1512.0 /*expectedInnerHorizontalOffset*/,
                0.0 /*expectedInnerVerticalOffset*/,
                options);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside vertical ScrollViewers into view, with center alignment.")]
        public void BringNestedElementIntoVerticalScrollViewerViewWithMiddleAlignment()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringIntoViewOptions options = null;

            RunOnUIThread.Execute(() =>
            {
                options = new BringIntoViewOptions() { VerticalAlignmentRatio = 0.5 };
            });

            BringElementInNestedScrollViewersIntoView(
                Orientation.Vertical,
                0.0 /*expectedOuterHorizontalOffset*/,
                1056.0 /*expectedOuterVerticalOffset*/,
                0.0 /*expectedInnerHorizontalOffset*/,
                1323.0 /*expectedInnerVerticalOffset*/,
                options);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside horizontal ScrollingPresenters into view, with offset.")]
        public void BringNestedElementIntoHorizontalScrollingPresenterViewWithOffset()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("ScrollingPresenter"))
            {
                BringIntoViewOptions options = null;

                RunOnUIThread.Execute(() =>
                {
                    options = new BringIntoViewOptions() { HorizontalOffset = -20.0 };
                });

                BringElementInNestedScrollingPresentersIntoView(
                    Orientation.Horizontal,
                    1056.0 /*expectedOuterHorizontalOffset*/,
                    0.0 /*expectedOuterVerticalOffset*/,
                    1103.0 /*expectedInnerHorizontalOffset*/,
                    0.0 /*expectedInnerVerticalOffset*/,
                    options);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside vertical ScrollingPresenters into view, with offset.")]
        public void BringNestedElementIntoVerticalScrollingPresenterViewWithOffset()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("ScrollingPresenter"))
            {
                BringIntoViewOptions options = null;

                RunOnUIThread.Execute(() =>
                {
                    options = new BringIntoViewOptions() { VerticalOffset = -20.0 };
                });

                BringElementInNestedScrollingPresentersIntoView(
                    Orientation.Vertical,
                    0.0 /*expectedOuterHorizontalOffset*/,
                    1056.0 /*expectedOuterVerticalOffset*/,
                    0.0 /*expectedInnerHorizontalOffset*/,
                    1103.0 /*expectedInnerVerticalOffset*/,
                    options);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside horizontal ScrollingPresenters into view, with offset.")]
        public void BringNestedElementIntoHorizontalScrollViewerViewWithOffset()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringIntoViewOptions options = null;

            RunOnUIThread.Execute(() =>
            {
                options = new BringIntoViewOptions() { HorizontalOffset = -20.0 };
            });

            BringElementInNestedScrollViewersIntoView(
                Orientation.Horizontal,
                1056.0 /*expectedOuterHorizontalOffset*/,
                0.0 /*expectedOuterVerticalOffset*/,
                1103.0 /*expectedInnerHorizontalOffset*/,
                0.0 /*expectedInnerVerticalOffset*/,
                options);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside vertical ScrollingPresenters into view, with offset.")]
        public void BringNestedElementIntoVerticalScrollViewerViewWithOffset()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringIntoViewOptions options = null;

            RunOnUIThread.Execute(() =>
            {
                options = new BringIntoViewOptions() { VerticalOffset = -20.0 };
            });

            BringElementInNestedScrollViewersIntoView(
                Orientation.Vertical,
                0.0 /*expectedOuterHorizontalOffset*/,
                1056.0 /*expectedOuterVerticalOffset*/,
                0.0 /*expectedInnerHorizontalOffset*/,
                1103.0 /*expectedInnerVerticalOffset*/,
                options);
        }

        private void BringElementIntoViewWithAlignment(
            Orientation orientation,
            double alignmentRatio,
            double expectedHorizontalOffset,
            double expectedVerticalOffset)
        {
            BringIntoViewOptions options = null;

            RunOnUIThread.Execute(() =>
            {
                options = new BringIntoViewOptions();
                if (orientation == Orientation.Horizontal)
                    options.HorizontalAlignmentRatio = alignmentRatio;
                else
                    options.VerticalAlignmentRatio = alignmentRatio;
            });

            BringElementIntoView(
                orientation,
                expectedHorizontalOffset,
                expectedVerticalOffset,
                options);
        }

        private void BringElementIntoViewWithOffset(
            Orientation orientation,
            double offset,
            double expectedHorizontalOffset,
            double expectedVerticalOffset,
            double originalHorizontalOffset = 0.0,
            double originalVerticalOffset = 0.0,
            float originalZoomFactor = 1.0f)
        {
            BringIntoViewOptions options = null;

            RunOnUIThread.Execute(() =>
            {
                options = new BringIntoViewOptions();
                if (orientation == Orientation.Horizontal)
                    options.HorizontalOffset = offset;
                else
                    options.VerticalOffset = offset;
            });

            BringElementIntoView(
                orientation,
                expectedHorizontalOffset,
                expectedVerticalOffset,
                options,
                originalHorizontalOffset,
                originalVerticalOffset,
                originalZoomFactor);
        }

        private void BringElementIntoView(
            Orientation orientation,
            double expectedHorizontalOffset,
            double expectedVerticalOffset,
            BringIntoViewOptions options = null,
            double originalHorizontalOffset = 0.0,
            double originalVerticalOffset = 0.0,
            float originalZoomFactor = 1.0f,
            bool applyOptionsInBringingIntoViewHandler = false,
            bool applySnapPointsInBringingIntoViewHandler = false)
        {
            ScrollingPresenter scrollingPresenter = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollingPresenterViewChangedEvent = new AutoResetEvent(false);
            AutoResetEvent bringIntoViewCompletedEvent = new AutoResetEvent(false);
            int bringIntoViewChangeId = -1;

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultBringIntoViewUI(orientation, scrollingPresenter, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            if (originalZoomFactor != 1.0f)
            {
                ZoomTo(scrollingPresenter, originalZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);
            }

            if (originalHorizontalOffset != 0 || originalVerticalOffset != 0)
            {
                ScrollTo(scrollingPresenter, originalHorizontalOffset, originalVerticalOffset, AnimationMode.Disabled, SnapPointsMode.Ignore, originalZoomFactor == 1.0f /*hookViewChanged*/);
            }

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenter.ViewChanged += delegate (ScrollingPresenter sender, object args)
                {
                    Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                    scrollingPresenterViewChangedEvent.Set();
                };

                scrollingPresenter.ScrollCompleted += delegate (ScrollingPresenter sender, ScrollingScrollCompletedEventArgs args)
                {
                    ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetScrollCompletedResult(args);
                    Log.Comment("ScrollingPresenter bring-into-view OffsetsChangeId={0} completed with Result={1}", args.ScrollInfo.OffsetsChangeId, result);
                    if (bringIntoViewChangeId == args.ScrollInfo.OffsetsChangeId)
                        bringIntoViewCompletedEvent.Set();
                };

                scrollingPresenter.BringingIntoView += (ScrollingPresenter sender, ScrollingBringingIntoViewEventArgs args) =>
                {
                    Log.Comment("ScrollingPresenter.BringingIntoView ScrollingPresenter={0} - TargetHorizontalOffset={1}, TargetVerticalOffset={2}, OffsetsChangeId={3}, SnapPointsMode={4}",
                        sender.Name, args.TargetHorizontalOffset, args.TargetVerticalOffset, args.ScrollInfo.OffsetsChangeId, args.SnapPointsMode);
                    bringIntoViewChangeId = args.ScrollInfo.OffsetsChangeId;

                    if (applyOptionsInBringingIntoViewHandler && options != null)
                    {
                        Log.Comment("ScrollingPresenter.BringingIntoView - Applying custom options");
                        args.RequestEventArgs.AnimationDesired = options.AnimationDesired;
                        args.RequestEventArgs.HorizontalOffset = options.HorizontalOffset;
                        args.RequestEventArgs.VerticalOffset = options.VerticalOffset;
                        if (options.TargetRect != null)
                        {
                            args.RequestEventArgs.TargetRect = (Rect)options.TargetRect;
                        }
                    }

                    if (applySnapPointsInBringingIntoViewHandler)
                    {
                        Log.Comment("ScrollingPresenter.BringingIntoView - Applying mandatory snap points");
                        AddSnapPoints(scrollingPresenter: sender, stackPanel: (sender.Content as Border).Child as StackPanel);
                        args.SnapPointsMode = SnapPointsMode.Default;
                    }
                };

                UIElement targetElement = ((scrollingPresenter.Content as Border).Child as StackPanel).Children[12];
                BringIntoViewOptions startingOptions = null;

                if (options == null)
                {
                    targetElement.StartBringIntoView();
                }
                else
                {
                    if (applyOptionsInBringingIntoViewHandler)
                    {
                        startingOptions = new BringIntoViewOptions();
                        startingOptions.HorizontalAlignmentRatio = options.HorizontalAlignmentRatio;
                        startingOptions.VerticalAlignmentRatio = options.VerticalAlignmentRatio;
                    }
                    else
                    {
                        startingOptions = options;
                    }
                    targetElement.StartBringIntoView(startingOptions);
                }
            });

            WaitForEvent("Waiting for ScrollingPresenter.ViewChanged event", scrollingPresenterViewChangedEvent);
            WaitForEvent("Waiting for bring-into-view operation completion event", bringIntoViewCompletedEvent);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final view - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(expectedHorizontalOffset, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(expectedVerticalOffset, scrollingPresenter.VerticalOffset);
                Verify.AreEqual(originalZoomFactor, scrollingPresenter.ZoomFactor);
            });
        }

        private void BringElementInNestedScrollingPresentersIntoView(
            Orientation orientation,
            double expectedOuterHorizontalOffset,
            double expectedOuterVerticalOffset,
            double expectedInnerHorizontalOffset,
            double expectedInnerVerticalOffset,
            BringIntoViewOptions options = null,
            double originalOuterHorizontalOffset = 0.0,
            double originalOuterVerticalOffset = 0.0,
            float originalOuterZoomFactor = 1.0f,
            double originalInnerHorizontalOffset = 0.0,
            double originalInnerVerticalOffset = 0.0,
            float originalInnerZoomFactor = 1.0f,
            bool applySnapPointsInBringingIntoViewHandler = false)
        {
            ScrollingPresenter outerScrollingPresenter = null;
            ScrollingPresenter innerScrollingPresenter = null;
            AutoResetEvent outerScrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent innerScrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent outerScrollingPresenterViewChangedEvent = new AutoResetEvent(false);
            AutoResetEvent innerScrollingPresenterViewChangedEvent = new AutoResetEvent(false);
            AutoResetEvent outerBringIntoViewCompletedEvent = new AutoResetEvent(false);
            AutoResetEvent innerBringIntoViewCompletedEvent = new AutoResetEvent(false);
            int outerBringIntoViewChangeId = -1;
            int innerBringIntoViewChangeId = -1;

            RunOnUIThread.Execute(() =>
            {
                outerScrollingPresenter = new ScrollingPresenter();
                innerScrollingPresenter = new ScrollingPresenter();

                SetupBringIntoViewUIWithScrollingPresenterInsideScrollingPresenter(orientation, outerScrollingPresenter, innerScrollingPresenter, outerScrollingPresenterLoadedEvent, innerScrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Inner Loaded event", innerScrollingPresenterLoadedEvent);
            WaitForEvent("Waiting for Outer Loaded event", outerScrollingPresenterLoadedEvent);

            if (originalOuterZoomFactor != 1.0f)
            {
                ZoomTo(outerScrollingPresenter, originalOuterZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);
            }

            if (originalOuterHorizontalOffset != 0 || originalOuterVerticalOffset != 0)
            {
                ScrollTo(outerScrollingPresenter, originalOuterHorizontalOffset, originalOuterVerticalOffset, AnimationMode.Disabled, SnapPointsMode.Ignore, originalOuterZoomFactor == 1.0f /*hookViewChanged*/);
            }

            if (originalInnerZoomFactor != 1.0f)
            {
                ZoomTo(innerScrollingPresenter, originalInnerZoomFactor, 0.0f, 0.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);
            }

            if (originalInnerHorizontalOffset != 0 || originalInnerVerticalOffset != 0)
            {
                ScrollTo(innerScrollingPresenter, originalInnerHorizontalOffset, originalInnerVerticalOffset, AnimationMode.Disabled, SnapPointsMode.Ignore, originalInnerZoomFactor == 1.0f /*hookViewChanged*/);
            }

            RunOnUIThread.Execute(() =>
            {
                innerScrollingPresenter.ViewChanged += delegate (ScrollingPresenter sender, object args)
                {
                    Log.Comment("Inner ViewChanged ScrollingPresenter={0} - HorizontalOffset={1}, VerticalOffset={2}, ZoomFactor={3}",
                        sender.Name, sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                    innerScrollingPresenterViewChangedEvent.Set();
                };

                innerScrollingPresenter.ScrollCompleted += delegate (ScrollingPresenter sender, ScrollingScrollCompletedEventArgs args)
                {
                    ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetScrollCompletedResult(args);

                    Log.Comment("Inner ScrollingPresenter bring-into-view OffsetsChangeId={0} completed with Result={1}", args.ScrollInfo.OffsetsChangeId, result);
                    if (innerBringIntoViewChangeId == args.ScrollInfo.OffsetsChangeId)
                        innerBringIntoViewCompletedEvent.Set();
                };

                innerScrollingPresenter.BringingIntoView += (ScrollingPresenter sender, ScrollingBringingIntoViewEventArgs args) =>
                {
                    Log.Comment("Inner ScrollingPresenter.BringingIntoView ScrollingPresenter={0} - TargetHorizontalOffset={1}, TargetVerticalOffset={2}, OffsetsChangeId={3}, SnapPointsMode={4}",
                        sender.Name, args.TargetHorizontalOffset, args.TargetVerticalOffset, args.ScrollInfo.OffsetsChangeId, args.SnapPointsMode);
                    innerBringIntoViewChangeId = args.ScrollInfo.OffsetsChangeId;

                    if (applySnapPointsInBringingIntoViewHandler)
                    {
                        Log.Comment("ScrollingPresenter.BringingIntoView - Applying mandatory snap points");
                        AddSnapPoints(scrollingPresenter: sender, stackPanel: (sender.Content as Border).Child as StackPanel);
                        args.SnapPointsMode = SnapPointsMode.Default;
                    }
                };

                outerScrollingPresenter.ViewChanged += delegate (ScrollingPresenter sender, object args)
                {
                    Log.Comment("Outer ViewChanged ScrollingPresenter={0} - HorizontalOffset={1}, VerticalOffset={2}, ZoomFactor={3}",
                        sender.Name, sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                    outerScrollingPresenterViewChangedEvent.Set();
                };

                outerScrollingPresenter.ScrollCompleted += delegate (ScrollingPresenter sender, ScrollingScrollCompletedEventArgs args)
                {
                    ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetScrollCompletedResult(args);
                    Log.Comment("Outer ScrollingPresenter bring-into-view OffsetsChangeId={0} completed with Result={1}", args.ScrollInfo.OffsetsChangeId, result);
                    if (outerBringIntoViewChangeId == args.ScrollInfo.OffsetsChangeId)
                        outerBringIntoViewCompletedEvent.Set();
                };

                outerScrollingPresenter.BringingIntoView += (ScrollingPresenter sender, ScrollingBringingIntoViewEventArgs args) =>
                {
                    Log.Comment("Outer ScrollingPresenter.BringingIntoView ScrollingPresenter={0} - TargetHorizontalOffset={1}, TargetVerticalOffset={2}, OffsetsChangeId={3}, SnapPointsMode={4}",
                        sender.Name, args.TargetHorizontalOffset, args.TargetVerticalOffset, args.ScrollInfo.OffsetsChangeId, args.SnapPointsMode);
                    outerBringIntoViewChangeId = args.ScrollInfo.OffsetsChangeId;

                    if (applySnapPointsInBringingIntoViewHandler)
                    {
                        Log.Comment("ScrollingPresenter.BringingIntoView - Applying mandatory snap points");
                        AddSnapPoints(scrollingPresenter: sender, stackPanel: (sender.Content as Border).Child as StackPanel);
                        args.SnapPointsMode = SnapPointsMode.Default;
                    }
                };

                UIElement targetElement = ((innerScrollingPresenter.Content as Border).Child as StackPanel).Children[12];

                if (options == null)
                {
                    targetElement.StartBringIntoView();
                }
                else
                {
                    targetElement.StartBringIntoView(options);
                }
            });

            WaitForEvent("Waiting for inner ScrollingPresenter.ViewChanged event", innerScrollingPresenterViewChangedEvent);
            WaitForEvent("Waiting for outer ScrollingPresenter.ViewChanged event", outerScrollingPresenterViewChangedEvent);
            WaitForEvent("Waiting for inner bring-into-view operation completion event", innerBringIntoViewCompletedEvent);
            WaitForEvent("Waiting for outer bring-into-view operation completion event", outerBringIntoViewCompletedEvent);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final inner view - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    innerScrollingPresenter.HorizontalOffset, innerScrollingPresenter.VerticalOffset, innerScrollingPresenter.ZoomFactor);
                Log.Comment("Final outer view - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    outerScrollingPresenter.HorizontalOffset, outerScrollingPresenter.VerticalOffset, outerScrollingPresenter.ZoomFactor);

                Verify.AreEqual(expectedInnerHorizontalOffset, innerScrollingPresenter.HorizontalOffset);
                Verify.AreEqual(expectedInnerVerticalOffset, innerScrollingPresenter.VerticalOffset);
                Verify.AreEqual(originalInnerZoomFactor, innerScrollingPresenter.ZoomFactor);

                Verify.AreEqual(expectedOuterHorizontalOffset, outerScrollingPresenter.HorizontalOffset);
                Verify.AreEqual(expectedOuterVerticalOffset, outerScrollingPresenter.VerticalOffset);
                Verify.AreEqual(originalOuterZoomFactor, outerScrollingPresenter.ZoomFactor);
            });
        }

        private void BringElementInNestedScrollViewersIntoView(
            Orientation orientation,
            double expectedOuterHorizontalOffset,
            double expectedOuterVerticalOffset,
            double expectedInnerHorizontalOffset,
            double expectedInnerVerticalOffset,
            BringIntoViewOptions options = null)
        {
            ScrollViewer outerScrollViewer = null;
            ScrollViewer innerScrollViewer = null;
            AutoResetEvent outerScrollViewerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent innerScrollViewerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent outerScrollViewerViewChangedEvent = new AutoResetEvent(false);
            AutoResetEvent innerScrollViewerViewChangedEvent = new AutoResetEvent(false);
            AutoResetEvent outerBringIntoViewCompletedEvent = new AutoResetEvent(false);
            AutoResetEvent innerBringIntoViewCompletedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                outerScrollViewer = new ScrollViewer();
                innerScrollViewer = new ScrollViewer();

                SetupBringIntoViewUIWithScrollViewerInsideScrollViewer(orientation, outerScrollViewer, innerScrollViewer, outerScrollViewerLoadedEvent, innerScrollViewerLoadedEvent);
            });

            WaitForEvent("Waiting for Inner Loaded event", innerScrollViewerLoadedEvent);
            WaitForEvent("Waiting for Outer Loaded event", outerScrollViewerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                innerScrollViewer.ViewChanged += delegate (object sender, ScrollViewerViewChangedEventArgs args)
                {
                    ScrollViewer sv = sender as ScrollViewer;
                    Log.Comment("Inner ViewChanged ScrollViewer={0} - HorizontalOffset={1}, VerticalOffset={2}, ZoomFactor={3}",
                        sv.Name, sv.HorizontalOffset, sv.VerticalOffset, sv.ZoomFactor);
                    if (!args.IsIntermediate)
                        innerScrollViewerViewChangedEvent.Set();
                };

                outerScrollViewer.ViewChanged += delegate (object sender, ScrollViewerViewChangedEventArgs args)
                {
                    ScrollViewer sv = sender as ScrollViewer;
                    Log.Comment("Outer ViewChanged ScrollViewer={0} - HorizontalOffset={1}, VerticalOffset={2}, ZoomFactor={3}",
                        sv.Name, sv.HorizontalOffset, sv.VerticalOffset, sv.ZoomFactor);
                    if (!args.IsIntermediate)
                        outerScrollViewerViewChangedEvent.Set();
                };

                UIElement targetElement = ((innerScrollViewer.Content as Border).Child as StackPanel).Children[12];

                if (options == null)
                {
                    targetElement.StartBringIntoView();
                }
                else
                {
                    targetElement.StartBringIntoView(options);
                }
            });

            WaitForEvent("Waiting for inner ScrollViewer.ViewChanged event", innerScrollViewerViewChangedEvent);
            WaitForEvent("Waiting for outer ScrollViewer.ViewChanged event", outerScrollViewerViewChangedEvent);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final inner view - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    innerScrollViewer.HorizontalOffset, innerScrollViewer.VerticalOffset, innerScrollViewer.ZoomFactor);
                Log.Comment("Final outer view - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    outerScrollViewer.HorizontalOffset, outerScrollViewer.VerticalOffset, outerScrollViewer.ZoomFactor);

                Verify.AreEqual(expectedInnerHorizontalOffset, innerScrollViewer.HorizontalOffset);
                Verify.AreEqual(expectedInnerVerticalOffset, innerScrollViewer.VerticalOffset);

                Verify.AreEqual(expectedOuterHorizontalOffset, outerScrollViewer.HorizontalOffset);
                Verify.AreEqual(expectedOuterVerticalOffset, outerScrollViewer.VerticalOffset);
            });
        }

        private void SetupDefaultBringIntoViewUI(
            Orientation orientation,
            ScrollingPresenter scrollingPresenter,
            AutoResetEvent scrollingPresenterLoadedEvent,
            bool setAsContentRoot = true)
        {
            Log.Comment("Setting up default bring-into-view UI with ScrollingPresenter");

            StackPanel stackPanel = new StackPanel();
            stackPanel.Name = "stackPanel";
            stackPanel.Orientation = orientation;
            stackPanel.Margin = new Thickness(30);

            Border border = new Border();
            border.Name = "border";
            border.BorderThickness = new Thickness(3);
            border.BorderBrush = new SolidColorBrush(Colors.Chartreuse);
            border.Margin = new Thickness(15);
            border.Background = new SolidColorBrush(Colors.Beige);
            border.Child = stackPanel;

            Verify.IsNotNull(scrollingPresenter);
            scrollingPresenter.Name = "scrollingPresenter";
            if (orientation == Orientation.Vertical)
            {
                scrollingPresenter.ContentOrientation = ContentOrientation.Vertical;
                scrollingPresenter.Width = c_defaultBringIntoViewUIScrollingPresenterConstrainedSize;
                scrollingPresenter.Height = c_defaultBringIntoViewUIScrollingPresenterNonConstrainedSize;
            }
            else
            {
                scrollingPresenter.ContentOrientation = ContentOrientation.Horizontal;
                scrollingPresenter.Width = c_defaultBringIntoViewUIScrollingPresenterNonConstrainedSize;
                scrollingPresenter.Height = c_defaultBringIntoViewUIScrollingPresenterConstrainedSize;
            }
            scrollingPresenter.Background = new SolidColorBrush(Colors.AliceBlue);
            scrollingPresenter.Content = border;

            InsertStackPanelChild(stackPanel, 0 /*operationCount*/, 0 /*newIndex*/, c_defaultBringIntoViewUIStackPanelChildrenCount /*newCount*/);

            if (scrollingPresenterLoadedEvent != null)
            {
                scrollingPresenter.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollingPresenter.Loaded event handler");
                    scrollingPresenterLoadedEvent.Set();
                };
            }

            scrollingPresenter.BringingIntoView += (ScrollingPresenter sender, ScrollingBringingIntoViewEventArgs args) =>
            {
                Log.Comment("ScrollingPresenter.BringingIntoView ScrollingPresenter={0} - TargetHorizontalOffset={1}, TargetVerticalOffset={2}, OffsetsChangeId={3}",
                    sender.Name, args.TargetHorizontalOffset, args.TargetVerticalOffset, args.ScrollInfo.OffsetsChangeId);
                Log.Comment("RequestEventArgs - AnimationDesired={0}, Handled={1}, HorizontalAlignmentRatio={2}, VerticalAlignmentRatio={3}",
                    args.RequestEventArgs.AnimationDesired,
                    args.RequestEventArgs.Handled,
                    args.RequestEventArgs.HorizontalAlignmentRatio,
                    args.RequestEventArgs.VerticalAlignmentRatio);
                Log.Comment("RequestEventArgs - HorizontalOffset={0}, VerticalOffset={1}, TargetElement={2}, TargetRect={3}",
                    args.RequestEventArgs.HorizontalOffset,
                    args.RequestEventArgs.VerticalOffset,
                    (args.RequestEventArgs.TargetElement as FrameworkElement).Name,
                    args.RequestEventArgs.TargetRect.ToString());
            };

            if (setAsContentRoot)
            {
                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scrollingPresenter;
            }
        }

        private void SetupDefaultBringIntoViewUIWithScrollViewer(
            Orientation orientation,
            ScrollViewer scrollViewer,
            AutoResetEvent scrollViewerLoadedEvent,
            bool setAsContentRoot = true)
        {
            Log.Comment("Setting up default bring-into-view UI with ScrollViewer");

            StackPanel stackPanel = new StackPanel();
            stackPanel.Name = "stackPanel";
            stackPanel.Orientation = orientation;
            stackPanel.Margin = new Thickness(30);

            Border border = new Border();
            border.Name = "border";
            border.BorderThickness = new Thickness(3);
            border.BorderBrush = new SolidColorBrush(Colors.Chartreuse);
            border.Margin = new Thickness(15);
            border.Background = new SolidColorBrush(Colors.Beige);
            border.Child = stackPanel;

            Verify.IsNotNull(scrollViewer);
            scrollViewer.Name = "scrollViewer";
            if (orientation == Orientation.Vertical)
            {
                scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled;
                scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
                scrollViewer.Width = c_defaultBringIntoViewUIScrollingPresenterConstrainedSize;
                scrollViewer.Height = c_defaultBringIntoViewUIScrollingPresenterNonConstrainedSize;
            }
            else
            {
                scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Disabled;
                scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                scrollViewer.Width = c_defaultBringIntoViewUIScrollingPresenterNonConstrainedSize;
                scrollViewer.Height = c_defaultBringIntoViewUIScrollingPresenterConstrainedSize;
            }
            scrollViewer.Background = new SolidColorBrush(Colors.AliceBlue);
            scrollViewer.Content = border;

            InsertStackPanelChild(stackPanel, 0 /*operationCount*/, 0 /*newIndex*/, c_defaultBringIntoViewUIStackPanelChildrenCount /*newCount*/);

            if (scrollViewerLoadedEvent != null)
            {
                scrollViewer.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollViewer.Loaded event handler");
                    scrollViewerLoadedEvent.Set();
                };
            }

            if (setAsContentRoot)
            {
                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scrollViewer;
            }
        }

        private void SetupBringIntoViewUIWithScrollingPresenterInsideScrollingPresenter(
            Orientation orientation,
            ScrollingPresenter outerScrollingPresenter,
            ScrollingPresenter innerScrollingPresenter,
            AutoResetEvent outerScrollingPresenterLoadedEvent,
            AutoResetEvent innerScrollingPresenterLoadedEvent)
        {
            Log.Comment("Setting up bring-into-view UI with ScrollingPresenter inside ScrollingPresenter");

            Log.Comment("Setting up inner ScrollingPresenter");
            SetupDefaultBringIntoViewUI(
                orientation,
                innerScrollingPresenter,
                innerScrollingPresenterLoadedEvent,
                false /*setAsContentRoot*/);

            Log.Comment("Setting up outer ScrollingPresenter");
            StackPanel stackPanel = new StackPanel();
            stackPanel.Name = "outerStackPanel";
            stackPanel.Orientation = orientation;
            stackPanel.Margin = new Thickness(30);

            Border border = new Border();
            border.Name = "outerBorder";
            border.BorderThickness = new Thickness(3);
            border.BorderBrush = new SolidColorBrush(Colors.Chartreuse);
            border.Margin = new Thickness(15);
            border.Background = new SolidColorBrush(Colors.Beige);
            border.Child = stackPanel;

            Verify.IsNotNull(outerScrollingPresenter);
            outerScrollingPresenter.Name = "outerScrollingPresenter";
            if (orientation == Orientation.Vertical)
            {
                outerScrollingPresenter.ContentOrientation = ContentOrientation.Vertical;
                outerScrollingPresenter.Width = c_defaultBringIntoViewUIScrollingPresenterConstrainedSize;
                outerScrollingPresenter.Height = c_defaultBringIntoViewUIScrollingPresenterNonConstrainedSize;
            }
            else
            {
                outerScrollingPresenter.ContentOrientation = ContentOrientation.Horizontal;
                outerScrollingPresenter.Width = c_defaultBringIntoViewUIScrollingPresenterNonConstrainedSize;
                outerScrollingPresenter.Height = c_defaultBringIntoViewUIScrollingPresenterConstrainedSize;
            }
            outerScrollingPresenter.Background = new SolidColorBrush(Colors.AliceBlue);
            outerScrollingPresenter.Content = border;

            InsertStackPanelChild(stackPanel, 0 /*operationCount*/, 0 /*newIndex*/, c_defaultBringIntoViewUIStackPanelChildrenCount / 2 /*newCount*/, "outer" /*namePrefix*/);

            stackPanel.Children.Add(innerScrollingPresenter);

            InsertStackPanelChild(stackPanel, 0 /*operationCount*/, c_defaultBringIntoViewUIStackPanelChildrenCount / 2 + 1 /*newIndex*/, c_defaultBringIntoViewUIStackPanelChildrenCount / 2 /*newCount*/, "outer" /*namePrefix*/);

            if (outerScrollingPresenterLoadedEvent != null)
            {
                outerScrollingPresenter.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Outer ScrollingPresenter.Loaded event handler");
                    outerScrollingPresenterLoadedEvent.Set();
                };
            }

            outerScrollingPresenter.BringingIntoView += (ScrollingPresenter sender, ScrollingBringingIntoViewEventArgs args) =>
            {
                Log.Comment("Outer ScrollingPresenter.BringingIntoView ScrollingPresenter={0} - TargetHorizontalOffset={1}, TargetVerticalOffset={2}, OffsetsChangeId={3}, SnapPointsMode={4}",
                    sender.Name, args.TargetHorizontalOffset, args.TargetVerticalOffset, args.ScrollInfo.OffsetsChangeId, args.SnapPointsMode);
                Log.Comment("RequestEventArgs - AnimationDesired={0}, Handled={1}, HorizontalAlignmentRatio={2}, VerticalAlignmentRatio={3}",
                    args.RequestEventArgs.AnimationDesired,
                    args.RequestEventArgs.Handled,
                    args.RequestEventArgs.HorizontalAlignmentRatio,
                    args.RequestEventArgs.VerticalAlignmentRatio);
                Log.Comment("RequestEventArgs - HorizontalOffset={0}, VerticalOffset={1}, TargetElement={2}, TargetRect={3}",
                    args.RequestEventArgs.HorizontalOffset,
                    args.RequestEventArgs.VerticalOffset,
                    (args.RequestEventArgs.TargetElement as FrameworkElement).Name,
                    args.RequestEventArgs.TargetRect.ToString());
            };

            Log.Comment("Setting window content");
            MUXControlsTestApp.App.TestContentRoot = outerScrollingPresenter;
        }

        private void SetupBringIntoViewUIWithScrollViewerInsideScrollViewer(
            Orientation orientation,
            ScrollViewer outerScrollViewer,
            ScrollViewer innerScrollViewer,
            AutoResetEvent outerScrollViewerLoadedEvent,
            AutoResetEvent innerScrollViewerLoadedEvent)
        {
            Log.Comment("Setting up bring-into-view UI with ScrollViewer inside ScrollViewer");

            Log.Comment("Setting up inner ScrollViewer");
            SetupDefaultBringIntoViewUIWithScrollViewer(
                orientation,
                innerScrollViewer,
                innerScrollViewerLoadedEvent,
                false /*setAsContentRoot*/);

            Log.Comment("Setting up outer ScrollViewer");
            StackPanel stackPanel = new StackPanel();
            stackPanel.Name = "outerStackPanel";
            stackPanel.Orientation = orientation;
            stackPanel.Margin = new Thickness(30);

            Border border = new Border();
            border.Name = "outerBorder";
            border.BorderThickness = new Thickness(3);
            border.BorderBrush = new SolidColorBrush(Colors.Chartreuse);
            border.Margin = new Thickness(15);
            border.Background = new SolidColorBrush(Colors.Beige);
            border.Child = stackPanel;

            Verify.IsNotNull(outerScrollViewer);
            outerScrollViewer.Name = "outerScrollViewer";
            if (orientation == Orientation.Vertical)
            {
                outerScrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled;
                outerScrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
                outerScrollViewer.Width = c_defaultBringIntoViewUIScrollingPresenterConstrainedSize;
                outerScrollViewer.Height = c_defaultBringIntoViewUIScrollingPresenterNonConstrainedSize;
            }
            else
            {
                outerScrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Disabled;
                outerScrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                outerScrollViewer.Width = c_defaultBringIntoViewUIScrollingPresenterNonConstrainedSize;
                outerScrollViewer.Height = c_defaultBringIntoViewUIScrollingPresenterConstrainedSize;
            }
            outerScrollViewer.Background = new SolidColorBrush(Colors.AliceBlue);
            outerScrollViewer.Content = border;

            InsertStackPanelChild(stackPanel, 0 /*operationCount*/, 0 /*newIndex*/, c_defaultBringIntoViewUIStackPanelChildrenCount / 2 /*newCount*/, "outer" /*namePrefix*/);

            stackPanel.Children.Add(innerScrollViewer);

            InsertStackPanelChild(stackPanel, 0 /*operationCount*/, c_defaultBringIntoViewUIStackPanelChildrenCount / 2 + 1 /*newIndex*/, c_defaultBringIntoViewUIStackPanelChildrenCount / 2 /*newCount*/, "outer" /*namePrefix*/);

            if (outerScrollViewerLoadedEvent != null)
            {
                outerScrollViewer.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Outer ScrollViewer.Loaded event handler");
                    outerScrollViewerLoadedEvent.Set();
                };
            }

            Log.Comment("Setting window content");
            MUXControlsTestApp.App.TestContentRoot = outerScrollViewer;
        }

        private void AddSnapPoints(ScrollingPresenter scrollingPresenter, StackPanel stackPanel)
        {
            Verify.IsNotNull(scrollingPresenter);

            if (stackPanel == null || stackPanel.Children.Count == 0)
            {
                return;
            }

            Log.Comment("Populating snap points for " + scrollingPresenter.Name + ":");

            ScrollSnapPoint scrollSnapPoint;
            GeneralTransform gt = stackPanel.TransformToVisual(scrollingPresenter.Content);
            Point stackPanelOriginPoint = new Point();
            stackPanelOriginPoint = gt.TransformPoint(stackPanelOriginPoint);

            if (stackPanel.Orientation == Orientation.Horizontal)
            {
                scrollSnapPoint = new ScrollSnapPoint(stackPanelOriginPoint.X, ScrollSnapPointsAlignment.Near);
                Log.Comment("Adding horizontal snap point to " + scrollingPresenter.Name + " at value " + stackPanelOriginPoint.X);
                scrollingPresenter.HorizontalSnapPoints.Add(scrollSnapPoint);
            }
            else
            {
                scrollSnapPoint = new ScrollSnapPoint(stackPanelOriginPoint.Y, ScrollSnapPointsAlignment.Near);
                Log.Comment("Adding vertical snap point to " + scrollingPresenter.Name + " at value " + stackPanelOriginPoint.Y);
                scrollingPresenter.VerticalSnapPoints.Add(scrollSnapPoint);
            }

            foreach (UIElement child in stackPanel.Children)
            {
                FrameworkElement childAsFE = child as FrameworkElement;

                if (childAsFE != null)
                {
                    gt = childAsFE.TransformToVisual(stackPanel);
                    Point childOriginPoint = new Point();
                    childOriginPoint = gt.TransformPoint(childOriginPoint);

                    double snapPointValue = 0.0;
                    Thickness margin = childAsFE.Margin;

                    if (stackPanel.Orientation == Orientation.Horizontal)
                    {
                        snapPointValue = margin.Right + childAsFE.ActualWidth + childOriginPoint.X;
                        if (snapPointValue <= scrollingPresenter.ScrollableWidth)
                        {
                            scrollSnapPoint = new ScrollSnapPoint(snapPointValue, ScrollSnapPointsAlignment.Near);
                            Log.Comment("Adding horizontal snap point to " + scrollingPresenter.Name + " at value " + snapPointValue);
                            scrollingPresenter.HorizontalSnapPoints.Add(scrollSnapPoint);
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        snapPointValue = margin.Bottom + childAsFE.ActualHeight + childOriginPoint.Y;
                        if (snapPointValue <= scrollingPresenter.ScrollableHeight)
                        {
                            scrollSnapPoint = new ScrollSnapPoint(snapPointValue, ScrollSnapPointsAlignment.Near);
                            Log.Comment("Adding vertical snap point to " + scrollingPresenter.Name + " at value " + snapPointValue);
                            scrollingPresenter.VerticalSnapPoints.Add(scrollSnapPoint);
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                AddSnapPoints(scrollingPresenter, child as StackPanel);
            }
        }
    }
}