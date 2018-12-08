// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System.Threading;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if !BUILD_WINDOWS
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ScrollerViewKind = Microsoft.UI.Xaml.Controls.ScrollerViewKind;
using ScrollerViewChangeCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollerViewChangeCompletedEventArgs;
using ScrollerBringingIntoViewEventArgs = Microsoft.UI.Xaml.Controls.ScrollerBringingIntoViewEventArgs;
using ScrollerViewChangeKind = Microsoft.UI.Xaml.Controls.ScrollerViewChangeKind;
using ScrollerViewChangeSnapPointRespect = Microsoft.UI.Xaml.Controls.ScrollerViewChangeSnapPointRespect;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollerTests
    {
        private const double c_defaultBringIntoViewUIScrollerNonConstrainedSize = 600.0;
        private const double c_defaultBringIntoViewUIScrollerConstrainedSize = 300.0;
        private const int c_defaultBringIntoViewUIStackPanelChildrenCount = 16;

        [TestMethod]
        [TestProperty("Description", "Brings an element with a horizontal Scroller into view.")]
        public void BringElementIntoHorizontalScrollerViewFromNearEdge()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoView(Orientation.Horizontal, 1083.0 /*expectedHorizontalOffset*/, 0.0 /*expectedVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element with a vertical Scroller into view.")]
        public void BringElementIntoVerticalScrollerViewFromNearEdge()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementIntoView(Orientation.Vertical, 0.0 /*expectedHorizontalOffset*/, 1083.0 /*expectedVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings an element with a horizontal Scroller into view, starting from the maximum offset.")]
        public void BringElementIntoHorizontalScrollerViewFromFarEdge()
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
        [TestProperty("Description", "Brings an element with a vertical Scroller into view, starting from the maximum offset.")]
        public void BringElementIntoVerticalScrollerViewFromFarEdge()
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
        [TestProperty("Description", "Brings an element with a horizontal Scroller into view, with left alignment.")]
        public void BringElementIntoHorizontalScrollerViewWithNearAlignment()
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
        [TestProperty("Description", "Brings an element with a horizontal Scroller into view, with center alignment.")]
        public void BringElementIntoHorizontalScrollerViewWithMiddleAlignment()
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
        [TestProperty("Description", "Brings an element with a horizontal Scroller into view, with right alignment.")]
        public void BringElementIntoHorizontalScrollerViewWithFarAlignment()
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
        [TestProperty("Description", "Brings an element with a vertical Scroller into view, with left alignment.")]
        public void BringElementIntoVerticalScrollerViewWithNearAlignment()
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
        [TestProperty("Description", "Brings an element with a vertical Scroller into view, with center alignment.")]
        public void BringElementIntoVerticalScrollerViewWithMiddleAlignment()
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
        [TestProperty("Description", "Brings an element with a vertical Scroller into view, with right alignment.")]
        public void BringElementIntoVerticalScrollerViewWithFarAlignment()
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
        [TestProperty("Description", "Brings an element with a horizontal Scroller into view, with a shift.")]
        public void BringElementIntoHorizontalScrollerViewWithOffset()
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
        [TestProperty("Description", "Brings an element with a vertical Scroller into view, with a shift.")]
        public void BringElementIntoVerticalScrollerViewWithOffset()
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
        [TestProperty("Description", "Brings an element with a horizontal Scroller into view, with a shift, starting from the maximum offset.")]
        public void BringElementIntoHorizontalScrollerViewWithOffsetFromFarEdge()
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
        [TestProperty("Description", "Brings an element with a vertical Scroller into view, with a shift, starting from the maximum offset.")]
        public void BringElementIntoVerticalScrollerViewWithOffsetFromFarEdge()
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
        [TestProperty("Description", "Brings an element with a Scroller into view, with a TargetRect.")]
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
        [TestProperty("Description", "Brings an element with a Scroller into view, with a TargetRect and VerticalOffset.")]
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
        [TestProperty("Description", "Brings a nested element inside horizontal Scrollers into view.")]
        public void BringNestedElementIntoHorizontalScrollerView()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementInNestedScrollersIntoView(
                Orientation.Horizontal,
                1056.0 /*expectedOuterHorizontalOffset*/,
                0.0 /*expectedOuterVerticalOffset*/,
                1083.0 /*expectedInnerHorizontalOffset*/,
                0.0 /*expectedInnerVerticalOffset*/);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside vertical Scrollers into view.")]
        public void BringNestedElementIntoVerticalScrollerView()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementInNestedScrollersIntoView(
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
        [TestProperty("Description", "Brings a nested element inside horizontal Scrollers into view, starting from the maximum offsets.")]
        public void BringNestedElementIntoHorizontalScrollerViewFromFarEdge()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementInNestedScrollersIntoView(
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
        [TestProperty("Description", "Brings a nested element inside vertical Scrollers into view, starting from the maximum offsets.")]
        public void BringNestedElementIntoVerticalScrollerViewFromFarEdge()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            BringElementInNestedScrollersIntoView(
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
        [TestProperty("Description", "Brings a nested element inside horizontal Scrollers into view, with left alignment.")]
        public void BringNestedElementIntoHorizontalScrollerViewWithNearAlignment()
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

            BringElementInNestedScrollersIntoView(
                Orientation.Horizontal,
                1107.0 /*expectedOuterHorizontalOffset*/,
                0.0 /*expectedOuterVerticalOffset*/,
                1512.0 /*expectedInnerHorizontalOffset*/,
                0.0 /*expectedInnerVerticalOffset*/,
                options);
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside vertical Scrollers into view, with center alignment.")]
        public void BringNestedElementIntoVerticalScrollerViewWithMiddleAlignment()
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

            BringElementInNestedScrollersIntoView(
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
        [TestProperty("Description", "Brings a nested element inside horizontal Scrollers into view, with offset.")]
        public void BringNestedElementIntoHorizontalScrollerViewWithOffset()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("Scroller"))
            {
                BringIntoViewOptions options = null;

                RunOnUIThread.Execute(() =>
                {
                    options = new BringIntoViewOptions() { HorizontalOffset = -20.0 };
                });

                BringElementInNestedScrollersIntoView(
                    Orientation.Horizontal,
                    1056.0 /*expectedOuterHorizontalOffset*/,
                    0.0 /*expectedOuterVerticalOffset*/,
                    1103.0 /*expectedInnerHorizontalOffset*/,
                    0.0 /*expectedInnerVerticalOffset*/,
                    options);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside vertical Scrollers into view, with offset.")]
        public void BringNestedElementIntoVerticalScrollerViewWithOffset()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Skipping this test since the BringIntoViewRequested event was introduced in RS4.
                return;
            }

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("Scroller"))
            {
                BringIntoViewOptions options = null;

                RunOnUIThread.Execute(() =>
                {
                    options = new BringIntoViewOptions() { VerticalOffset = -20.0 };
                });

                BringElementInNestedScrollersIntoView(
                    Orientation.Vertical,
                    0.0 /*expectedOuterHorizontalOffset*/,
                    1056.0 /*expectedOuterVerticalOffset*/,
                    0.0 /*expectedInnerHorizontalOffset*/,
                    1103.0 /*expectedInnerVerticalOffset*/,
                    options);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Brings a nested element inside horizontal Scrollers into view, with offset.")]
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
        [TestProperty("Description", "Brings a nested element inside vertical Scrollers into view, with offset.")]
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
            bool applyOptionsInBringingIntoViewHandler = false)
        {
            Scroller scroller = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangedEvent = new AutoResetEvent(false);
            AutoResetEvent bringIntoViewCompletedEvent = new AutoResetEvent(false);
            int bringIntoViewChangeId = -1;

            RunOnUIThread.Execute(() =>
            {
                scroller = new Scroller();

                SetupDefaultBringIntoViewUI(orientation, scroller, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            if (originalZoomFactor != 1.0f)
            {
                ChangeZoomFactor(scroller, originalZoomFactor, 0.0f, 0.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation);
            }

            if (originalHorizontalOffset != 0 || originalVerticalOffset != 0)
            {
                ChangeOffsets(scroller, originalHorizontalOffset, originalVerticalOffset, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints, originalZoomFactor == 1.0f /*hookViewChanged*/);
            }

            RunOnUIThread.Execute(() =>
            {
                scroller.ViewChanged += delegate (Scroller sender, object args)
                {
                    Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                    scrollerViewChangedEvent.Set();
                };

                scroller.ViewChangeCompleted += delegate (Scroller sender, ScrollerViewChangeCompletedEventArgs args)
                {
                    Log.Comment("Scroller bring-into-view ViewChangeId={0} completed with Result={1}", args.ViewChangeId, args.Result);
                    if (bringIntoViewChangeId == args.ViewChangeId)
                        bringIntoViewCompletedEvent.Set();
                };

                scroller.BringingIntoView += (Scroller sender, ScrollerBringingIntoViewEventArgs args) =>
                {
                    Log.Comment("Scroller.BringingIntoView Scroller={0} - TargetHorizontalOffset={1}, TargetVerticalOffset={2}, ViewChangeId={3}",
                        sender.Name, args.TargetHorizontalOffset, args.TargetVerticalOffset, args.ViewChangeId);
                    bringIntoViewChangeId = args.ViewChangeId;

                    if (applyOptionsInBringingIntoViewHandler && options != null)
                    {
                        Log.Comment("Scroller.BringingIntoView - Applying custom options");
                        args.RequestEventArgs.AnimationDesired = options.AnimationDesired;
                        args.RequestEventArgs.HorizontalOffset = options.HorizontalOffset;
                        args.RequestEventArgs.VerticalOffset = options.VerticalOffset;
                        if (options.TargetRect != null)
                        {
                            args.RequestEventArgs.TargetRect = (Rect)options.TargetRect;
                        }
                    }
                };

                UIElement targetElement = ((scroller.Child as Border).Child as StackPanel).Children[12];
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

            WaitForEvent("Waiting for Scroller.ViewChanged event", scrollerViewChangedEvent);
            WaitForEvent("Waiting for bring-into-view operation completion event", bringIntoViewCompletedEvent);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final view - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);
                Verify.AreEqual(scroller.HorizontalOffset, expectedHorizontalOffset);
                Verify.AreEqual(scroller.VerticalOffset, expectedVerticalOffset);
                Verify.AreEqual(scroller.ZoomFactor, originalZoomFactor);
            });
        }

        private void BringElementInNestedScrollersIntoView(
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
            float originalInnerZoomFactor = 1.0f)
        {
            Scroller outerScroller = null;
            Scroller innerScroller = null;
            AutoResetEvent outerScrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent innerScrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent outerScrollerViewChangedEvent = new AutoResetEvent(false);
            AutoResetEvent innerScrollerViewChangedEvent = new AutoResetEvent(false);
            AutoResetEvent outerBringIntoViewCompletedEvent = new AutoResetEvent(false);
            AutoResetEvent innerBringIntoViewCompletedEvent = new AutoResetEvent(false);
            int outerBringIntoViewChangeId = -1;
            int innerBringIntoViewChangeId = -1;

            RunOnUIThread.Execute(() =>
            {
                outerScroller = new Scroller();
                innerScroller = new Scroller();

                SetupBringIntoViewUIWithScrollerInsideScroller(orientation, outerScroller, innerScroller, outerScrollerLoadedEvent, innerScrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Inner Loaded event", innerScrollerLoadedEvent);
            WaitForEvent("Waiting for Outer Loaded event", outerScrollerLoadedEvent);

            if (originalOuterZoomFactor != 1.0f)
            {
                ChangeZoomFactor(outerScroller, originalOuterZoomFactor, 0.0f, 0.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation);
            }

            if (originalOuterHorizontalOffset != 0 || originalOuterVerticalOffset != 0)
            {
                ChangeOffsets(outerScroller, originalOuterHorizontalOffset, originalOuterVerticalOffset, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints, originalOuterZoomFactor == 1.0f /*hookViewChanged*/);
            }

            if (originalInnerZoomFactor != 1.0f)
            {
                ChangeZoomFactor(innerScroller, originalInnerZoomFactor, 0.0f, 0.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation);
            }

            if (originalInnerHorizontalOffset != 0 || originalInnerVerticalOffset != 0)
            {
                ChangeOffsets(innerScroller, originalInnerHorizontalOffset, originalInnerVerticalOffset, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints, originalInnerZoomFactor == 1.0f /*hookViewChanged*/);
            }

            RunOnUIThread.Execute(() =>
            {
                innerScroller.ViewChanged += delegate (Scroller sender, object args)
                {
                    Log.Comment("Inner ViewChanged Scroller={0} - HorizontalOffset={1}, VerticalOffset={2}, ZoomFactor={3}",
                        sender.Name, sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                    innerScrollerViewChangedEvent.Set();
                };

                innerScroller.ViewChangeCompleted += delegate (Scroller sender, ScrollerViewChangeCompletedEventArgs args)
                {
                    Log.Comment("Inner Scroller bring-into-view ViewChangeId={0} completed with Result={1}", args.ViewChangeId, args.Result);
                    if (innerBringIntoViewChangeId == args.ViewChangeId)
                        innerBringIntoViewCompletedEvent.Set();
                };

                innerScroller.BringingIntoView += (Scroller sender, ScrollerBringingIntoViewEventArgs args) =>
                {
                    Log.Comment("Inner Scroller.BringingIntoView Scroller={0} - TargetHorizontalOffset={1}, TargetVerticalOffset={2}, ViewChangeId={3}",
                        sender.Name, args.TargetHorizontalOffset, args.TargetVerticalOffset, args.ViewChangeId);
                    innerBringIntoViewChangeId = args.ViewChangeId;
                };

                outerScroller.ViewChanged += delegate (Scroller sender, object args)
                {
                    Log.Comment("Outer ViewChanged Scroller={0} - HorizontalOffset={1}, VerticalOffset={2}, ZoomFactor={3}",
                        sender.Name, sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                    outerScrollerViewChangedEvent.Set();
                };

                outerScroller.ViewChangeCompleted += delegate (Scroller sender, ScrollerViewChangeCompletedEventArgs args)
                {
                    Log.Comment("Outer Scroller bring-into-view ViewChangeId={0} completed with Result={1}", args.ViewChangeId, args.Result);
                    if (outerBringIntoViewChangeId == args.ViewChangeId)
                        outerBringIntoViewCompletedEvent.Set();
                };

                outerScroller.BringingIntoView += (Scroller sender, ScrollerBringingIntoViewEventArgs args) =>
                {
                    Log.Comment("Outer Scroller.BringingIntoView Scroller={0} - TargetHorizontalOffset={1}, TargetVerticalOffset={2}, ViewChangeId={3}",
                        sender.Name, args.TargetHorizontalOffset, args.TargetVerticalOffset, args.ViewChangeId);
                    outerBringIntoViewChangeId = args.ViewChangeId;
                };

                UIElement targetElement = ((innerScroller.Child as Border).Child as StackPanel).Children[12];

                if (options == null)
                {
                    targetElement.StartBringIntoView();
                }
                else
                {
                    targetElement.StartBringIntoView(options);
                }
            });

            WaitForEvent("Waiting for inner Scroller.ViewChanged event", innerScrollerViewChangedEvent);
            WaitForEvent("Waiting for outer Scroller.ViewChanged event", outerScrollerViewChangedEvent);
            WaitForEvent("Waiting for inner bring-into-view operation completion event", innerBringIntoViewCompletedEvent);
            WaitForEvent("Waiting for outer bring-into-view operation completion event", outerBringIntoViewCompletedEvent);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final inner view - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    innerScroller.HorizontalOffset, innerScroller.VerticalOffset, innerScroller.ZoomFactor);
                Log.Comment("Final outer view - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    outerScroller.HorizontalOffset, outerScroller.VerticalOffset, outerScroller.ZoomFactor);

                Verify.AreEqual(innerScroller.HorizontalOffset, expectedInnerHorizontalOffset);
                Verify.AreEqual(innerScroller.VerticalOffset, expectedInnerVerticalOffset);
                Verify.AreEqual(innerScroller.ZoomFactor, originalInnerZoomFactor);

                Verify.AreEqual(outerScroller.HorizontalOffset, expectedOuterHorizontalOffset);
                Verify.AreEqual(outerScroller.VerticalOffset, expectedOuterVerticalOffset);
                Verify.AreEqual(outerScroller.ZoomFactor, originalOuterZoomFactor);
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

                Verify.AreEqual(innerScrollViewer.HorizontalOffset, expectedInnerHorizontalOffset);
                Verify.AreEqual(innerScrollViewer.VerticalOffset, expectedInnerVerticalOffset);
                
                Verify.AreEqual(outerScrollViewer.HorizontalOffset, expectedOuterHorizontalOffset);
                Verify.AreEqual(outerScrollViewer.VerticalOffset, expectedOuterVerticalOffset);
            });
        }

        private void SetupDefaultBringIntoViewUI(
            Orientation orientation,
            Scroller scroller,
            AutoResetEvent scrollerLoadedEvent,
            bool setAsContentRoot = true)
        {
            Log.Comment("Setting up default bring-into-view UI with Scroller");

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

            Verify.IsNotNull(scroller);
            scroller.Name = "scroller";
            if (orientation == Orientation.Vertical)
            {
                scroller.IsChildAvailableWidthConstrained = true;
                scroller.Width = c_defaultBringIntoViewUIScrollerConstrainedSize;
                scroller.Height = c_defaultBringIntoViewUIScrollerNonConstrainedSize;
            }
            else
            {
                scroller.IsChildAvailableHeightConstrained = true;
                scroller.Width = c_defaultBringIntoViewUIScrollerNonConstrainedSize;
                scroller.Height = c_defaultBringIntoViewUIScrollerConstrainedSize;
            }
            scroller.Background = new SolidColorBrush(Colors.AliceBlue);
            scroller.Child = border;

            InsertStackPanelChild(stackPanel, 0 /*operationCount*/, 0 /*newIndex*/, c_defaultBringIntoViewUIStackPanelChildrenCount /*newCount*/);

            if (scrollerLoadedEvent != null)
            {
                scroller.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Scroller.Loaded event handler");
                    scrollerLoadedEvent.Set();
                };
            }

            scroller.BringingIntoView += (Scroller sender, ScrollerBringingIntoViewEventArgs args) =>
            {
                Log.Comment("Scroller.BringingIntoView Scroller={0} - TargetHorizontalOffset={1}, TargetVerticalOffset={2}, ViewChangeId={3}",
                    sender.Name, args.TargetHorizontalOffset, args.TargetVerticalOffset, args.ViewChangeId);
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
                MUXControlsTestApp.App.TestContentRoot = scroller;
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
                scrollViewer.Width = c_defaultBringIntoViewUIScrollerConstrainedSize;
                scrollViewer.Height = c_defaultBringIntoViewUIScrollerNonConstrainedSize;
            }
            else
            {
                scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Disabled;
                scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                scrollViewer.Width = c_defaultBringIntoViewUIScrollerNonConstrainedSize;
                scrollViewer.Height = c_defaultBringIntoViewUIScrollerConstrainedSize;
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

        private void SetupBringIntoViewUIWithScrollerInsideScroller(
            Orientation orientation,
            Scroller outerScroller,
            Scroller innerScroller,
            AutoResetEvent outerScrollerLoadedEvent,
            AutoResetEvent innerScrollerLoadedEvent)
        {
            Log.Comment("Setting up bring-into-view UI with Scroller inside Scroller");

            Log.Comment("Setting up inner Scroller");
            SetupDefaultBringIntoViewUI(
                orientation,
                innerScroller,
                innerScrollerLoadedEvent,
                false /*setAsContentRoot*/);

            Log.Comment("Setting up outer Scroller");
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

            Verify.IsNotNull(outerScroller);
            outerScroller.Name = "outerScroller";
            if (orientation == Orientation.Vertical)
            {
                outerScroller.IsChildAvailableWidthConstrained = true;
                outerScroller.Width = c_defaultBringIntoViewUIScrollerConstrainedSize;
                outerScroller.Height = c_defaultBringIntoViewUIScrollerNonConstrainedSize;
            }
            else
            {
                outerScroller.IsChildAvailableHeightConstrained = true;
                outerScroller.Width = c_defaultBringIntoViewUIScrollerNonConstrainedSize;
                outerScroller.Height = c_defaultBringIntoViewUIScrollerConstrainedSize;
            }
            outerScroller.Background = new SolidColorBrush(Colors.AliceBlue);
            outerScroller.Child = border;

            InsertStackPanelChild(stackPanel, 0 /*operationCount*/, 0 /*newIndex*/, c_defaultBringIntoViewUIStackPanelChildrenCount / 2 /*newCount*/, "outer" /*namePrefix*/);

            stackPanel.Children.Add(innerScroller);

            InsertStackPanelChild(stackPanel, 0 /*operationCount*/, c_defaultBringIntoViewUIStackPanelChildrenCount / 2 + 1 /*newIndex*/, c_defaultBringIntoViewUIStackPanelChildrenCount / 2 /*newCount*/, "outer" /*namePrefix*/);

            if (outerScrollerLoadedEvent != null)
            {
                outerScroller.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Outer Scroller.Loaded event handler");
                    outerScrollerLoadedEvent.Set();
                };
            }

            outerScroller.BringingIntoView += (Scroller sender, ScrollerBringingIntoViewEventArgs args) =>
            {
                Log.Comment("Outer Scroller.BringingIntoView Scroller={0} - TargetHorizontalOffset={1}, TargetVerticalOffset={2}, ViewChangeId={3}",
                    sender.Name, args.TargetHorizontalOffset, args.TargetVerticalOffset, args.ViewChangeId);
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
            MUXControlsTestApp.App.TestContentRoot = outerScroller;
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
                outerScrollViewer.Width = c_defaultBringIntoViewUIScrollerConstrainedSize;
                outerScrollViewer.Height = c_defaultBringIntoViewUIScrollerNonConstrainedSize;
            }
            else
            {
                outerScrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Disabled;
                outerScrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
                outerScrollViewer.Width = c_defaultBringIntoViewUIScrollerNonConstrainedSize;
                outerScrollViewer.Height = c_defaultBringIntoViewUIScrollerConstrainedSize;
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
    }
}
