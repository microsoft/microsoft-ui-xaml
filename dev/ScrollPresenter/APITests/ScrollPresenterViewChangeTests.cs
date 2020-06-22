// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Numerics;
using System.Threading;
using Windows.Foundation;
using Windows.UI.Composition;
using Windows.UI.Xaml.Shapes;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using ScrollOptions = Microsoft.UI.Xaml.Controls.ScrollOptions;
using ZoomOptions = Microsoft.UI.Xaml.Controls.ZoomOptions;
using ScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollAnimationStartingEventArgs;
using ZoomAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ZoomAnimationStartingEventArgs;
using ScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollCompletedEventArgs;
using ZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ZoomCompletedEventArgs;

using ScrollerTestHooks = Microsoft.UI.Private.Controls.ScrollerTestHooks;
using ScrollerViewChangeResult = Microsoft.UI.Private.Controls.ScrollerViewChangeResult;
using Windows.UI.ViewManagement;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollerTests : ApiTestBase
    {
        // Enum used in tests that use InterruptViewChange
        private enum ViewChangeInterruptionKind
        {
            OffsetsChangeByOffsetsChange,
            OffsetsChangeByZoomFactorChange,
            ZoomFactorChangeByOffsetsChange,
            ZoomFactorChangeByZoomFactorChange,
        }

        private const int c_MaxWaitDuration = 5000;
        private const int c_MaxStockOffsetsChangeDuration = 1000;
        private const int c_MaxStockZoomFactorChangeDuration = 1000;

        private uint viewChangedCount = 0u;

        [TestMethod]
        [TestProperty("Description", "Changes Scroller offsets using ScrollTo, ScrollBy, ScrollFrom and AnimationMode/SnapPointsMode enum values.")]
        public void BasicOffsetChanges()
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Jump to absolute offsets
            ScrollTo(scroller, 11.0, 22.0, AnimationMode.Disabled, SnapPointsMode.Ignore);
            ScrollTo(scroller, 22.0, 11.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: false);

            // Jump to relative offsets
            ScrollBy(scroller, -4.0, 15.0, AnimationMode.Disabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ScrollBy(scroller, 15.0, 4.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: false);

            // Animate to absolute offsets
            ScrollTo(scroller, 55.0, 25.0, AnimationMode.Enabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ScrollTo(scroller, 5.0, 75.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: true);

            // Jump or animate to absolute offsets based on UISettings.AnimationsEnabled
            ScrollTo(scroller, 55.0, 25.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false);

            // Animate to relative offsets
            ScrollBy(scroller, 700.0, -8.0, AnimationMode.Enabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ScrollBy(scroller, -80.0, 200.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: true);

            // Jump or animate to relative offsets based on UISettings.AnimationsEnabled
            ScrollBy(scroller, 80.0, -200.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false);

            // Flick with additional offsets velocity
            ScrollFrom(scroller, -65.0f, 80.0f, horizontalInertiaDecayRate: null, verticalInertiaDecayRate: null, hookViewChanged: false);

            // Flick with additional offsets velocity and custom scroll inertia decay rate
            ScrollFrom(scroller, 65.0f, -80.0f, horizontalInertiaDecayRate: 0.7f , verticalInertiaDecayRate: 0.8f, hookViewChanged: false);

            // Do it all again while respecting snap points
            ScrollTo(scroller, 11.0, 22.0, AnimationMode.Disabled, SnapPointsMode.Default, hookViewChanged: false);
            ScrollBy(scroller, -4.0, 15.0, AnimationMode.Disabled, SnapPointsMode.Default, hookViewChanged: false);
            ScrollTo(scroller, 55.0, 25.0, AnimationMode.Enabled, SnapPointsMode.Default, hookViewChanged: false);
            ScrollBy(scroller, 700.0, -8.0, AnimationMode.Enabled, SnapPointsMode.Default, hookViewChanged: false);
        }

        [TestMethod]
        [TestProperty("Description", "Changes Scroller zoomFactor using ZoomTo, ZoomBy, ZoomFrom and AnimationMode enum values.")]
        public void BasicZoomFactorChanges()
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Jump to absolute zoomFactor
            ZoomTo(scroller, 2.0f, 22.0f, 33.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);
            ZoomTo(scroller, 5.0f, 33.0f, 22.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: false);

            // Jump to relative zoomFactor
            ZoomBy(scroller, 1.0f, 55.0f, 66.0f, AnimationMode.Disabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ZoomBy(scroller, 1.0f, 66.0f, 55.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: false);

            // Animate to absolute zoomFactor
            ZoomTo(scroller, 4.0f, -40.0f, -25.0f, AnimationMode.Enabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ZoomTo(scroller, 6.0f, 25.0f, 40.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: true);

            // Jump or animate to absolute zoomFactor based on UISettings.AnimationsEnabled
            ZoomTo(scroller, 3.0f, 10.0f, 20.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false);

            // Animate to relative zoomFactor
            ZoomBy(scroller, -2.0f, 100.0f, 200.0f, AnimationMode.Enabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ZoomBy(scroller, 1.0f, 100.0f, 200.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: true);

            // Jump or animate to relative zoomFactor based on UISettings.AnimationsEnabled
            ZoomBy(scroller, 2.0f, 200.0f, 100.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false);

            // Flick with additional zoomFactor velocity
            ZoomFrom(scroller, 2.0f, inertiaDecayRate: null, centerPointX: -50.0f, centerPointY: 800.0f, hookViewChanged: false);

            // Flick with additional zoomFactor velocity and custom zoomFactor inertia decay rate
            ZoomFrom(scroller, -2.0f, inertiaDecayRate: 0.75f, centerPointX: -50.0f, centerPointY: 800.0f, hookViewChanged: false);
        }

        [TestMethod]
        [TestProperty("Description", "Cancels an animated offsets change.")]
        public void BasicOffsetsChangeCancelation()
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                operation = StartScrollTo(
                    scroller,
                    600.0,
                    400.0,
                    AnimationMode.Enabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvent);

                bool operationCanceled = false;

                scroller.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");

                    if ((sender.HorizontalOffset >= 150.0 || sender.VerticalOffset >= 100.0) && !operationCanceled)
                    {
                        Log.Comment("Canceling view change");
                        operationCanceled = true;
                        sender.ScrollBy(0, 0, new ScrollOptions(AnimationMode.Disabled, SnapPointsMode.Ignore));
                    }
                };
            });

            WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.IsTrue(scroller.HorizontalOffset < 600.0);
                Verify.IsTrue(scroller.VerticalOffset < 400.0);
                Verify.AreEqual(1.0f, scroller.ZoomFactor);
                Verify.AreEqual(ScrollerViewChangeResult.Interrupted, operation.Result);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Cancels an animated zoomFactor change.")]
        public void BasicZoomFactorChangeCancelation()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone) && !PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2))
            {
                Log.Warning("Skipping test on RS2 Phone where it randomly causes the next test to crash (bug #13418413).");
                return;
            }

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("Scroller"))
            {
                Scroller scroller = null;
                Rectangle rectangleScrollerContent = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollerOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollerContent = new Rectangle();
                    scroller = new Scroller();
                    scroller.Name = "scr";

                    SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    operation = StartZoomTo(
                        scroller,
                        8.0f,
                        100.0f,
                        150.0f,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvent);

                    bool operationCanceled = false;

                    scroller.ViewChanged += (sender, args) =>
                    {
                        Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");

                        if (sender.ZoomFactor >= 2.0 && !operationCanceled)
                        {
                            Log.Comment("Canceling view change");
                            operationCanceled = true;
                            sender.ZoomBy(0, Vector2.Zero, new ZoomOptions(AnimationMode.Disabled, SnapPointsMode.Ignore));
                        }
                    };
                });

                WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                    Verify.IsTrue(scroller.ZoomFactor < 8.0f);
                    Verify.AreEqual(ScrollerViewChangeResult.Interrupted, operation.Result);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Checks to make sure the exposed startPosition, endPosition, StartZoomFactor, and EndZoomFactor " +
            "on ScrollAnimationStartingEventArgs and ZoomAnimationStartingEventArgs respectively are accurate.")]
        public void ValidateScrollAnimationStartingAndZoomFactorEventArgsHaveValidStartAndEndPositions()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1.");
                return;
            }
            int numOffsetChanges = 0;
            int numZoomFactorChanges = 0;
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            Point newPosition1 = new Point(100, 150);
            Point newPosition2 = new Point(50, 100);
            float newZoomFactor1 = 2.0f;
            float newZoomFactor2 = 0.5f;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(
                    scroller, rectangleScrollerContent, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Attach to ScrollAnimationStarting");

                scroller.ZoomAnimationStarting += (Scroller sender, ZoomAnimationStartingEventArgs e) =>
                {
                    Log.Comment("Scroller.ZoomAnimationStarting event handler");
                    if (numZoomFactorChanges == 0)
                    {
                        Verify.AreEqual(2.0f, e.EndZoomFactor);
                        Verify.AreEqual(1.0f, e.StartZoomFactor);
                        numZoomFactorChanges++;
                    }
                    else
                    {
                        Verify.AreEqual(0.5f, e.EndZoomFactor);
                        Verify.AreEqual(2.0f, e.StartZoomFactor);
                    }
                };

                scroller.ScrollAnimationStarting += (Scroller sender, ScrollAnimationStartingEventArgs e) =>
                {
                    Log.Comment("Scroller.ScrollAnimationStarting event handler");
                    if (numOffsetChanges == 0)
                    {
                        Verify.AreEqual(100.0f, e.EndPosition.X);
                        Verify.AreEqual(150.0f, e.EndPosition.Y);
                        Verify.AreEqual(0.0f, e.StartPosition.X);
                        Verify.AreEqual(0.0f, e.StartPosition.Y);

                        numOffsetChanges++;
                    }
                    else
                    {
                        Verify.AreEqual(50.0f, e.EndPosition.X);
                        Verify.AreEqual(100.0f, e.EndPosition.Y);
                        Verify.AreEqual(100.0f, e.StartPosition.X);
                        Verify.AreEqual(150.0f, e.StartPosition.Y);

                        numOffsetChanges++;
                    }
                };
            });

            Log.Comment("Animating to absolute Offset");
            ScrollTo(scroller, newPosition1.X, newPosition1.Y, AnimationMode.Enabled, SnapPointsMode.Ignore);

            Log.Comment("Animating to absolute Offset");
            ScrollTo(scroller, newPosition2.X, newPosition2.Y, AnimationMode.Enabled, SnapPointsMode.Ignore);

            Log.Comment("Animating to absolute zoomFactor");
            ZoomTo(scroller, newZoomFactor1, 100.0f, 200.0f, AnimationMode.Enabled, SnapPointsMode.Ignore);

            Log.Comment("Animating to absolute zoomFactor");
            ZoomTo(scroller, newZoomFactor2, 100.0f, 200.0f, AnimationMode.Enabled, SnapPointsMode.Ignore);

        }

        [TestMethod]
        [TestProperty("Description", "Performs an animated offsets change with an overridden duration.")]
        public void OffsetsChangeWithCustomDuration()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1 because of bug 13649219.");
                return;
            }

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("Scroller"))
            {
                Scroller scroller = null;
                Rectangle rectangleScrollerContent = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollerOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollerContent = new Rectangle();
                    scroller = new Scroller();

                    SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    scroller.ViewChanged += (sender, args) =>
                    {
                        Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                    };

                    scroller.ScrollAnimationStarting += (sender, args) =>
                    {
                        Log.Comment("ScrollAnimationStarting - OffsetsChangeId={0}", args.ScrollInfo.OffsetsChangeId);
                        Verify.IsNotNull(args.Animation);
                        Vector3KeyFrameAnimation stockKeyFrameAnimation = args.Animation as Vector3KeyFrameAnimation;
                        Verify.IsNotNull(stockKeyFrameAnimation);
                        Log.Comment("Stock duration={0} msec.", stockKeyFrameAnimation.Duration.TotalMilliseconds);
                        Verify.AreEqual(c_MaxStockOffsetsChangeDuration, stockKeyFrameAnimation.Duration.TotalMilliseconds);
                        stockKeyFrameAnimation.Duration = TimeSpan.FromMilliseconds(10);
                    };

                    operation = StartScrollTo(
                        scroller,
                        600.0,
                        400.0,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scroller.HorizontalOffset}, VerticalOffset={scroller.VerticalOffset}, ZoomFactor={scroller.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(600.0, scroller.HorizontalOffset);
                    Verify.AreEqual(400.0, scroller.VerticalOffset);
                    Verify.AreEqual(1.0f, scroller.ZoomFactor);

                    Verify.IsLessThanOrEqual(viewChangedCount, 2u);

                    Verify.AreEqual(ScrollerViewChangeResult.Completed, operation.Result);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Performs an animated zoomFactor change with an overridden duration.")]
        public void ZoomFactorChangeWithCustomDuration()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1 because of bug 13649219.");
                return;
            }

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("Scroller"))
            {
                Scroller scroller = null;
                Rectangle rectangleScrollerContent = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollerOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollerContent = new Rectangle();
                    scroller = new Scroller();

                    SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    scroller.ViewChanged += (sender, args) =>
                    {
                        Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                    };

                    scroller.ZoomAnimationStarting += (sender, args) =>
                    {
                        Log.Comment("ZoomAnimationStarting - ZoomFactorChangeId={0}", args.ZoomInfo.ZoomFactorChangeId);
                        Verify.IsNotNull(args.Animation);
                        ScalarKeyFrameAnimation stockKeyFrameAnimation = args.Animation as ScalarKeyFrameAnimation;
                        Verify.IsNotNull(stockKeyFrameAnimation);
                        Log.Comment("Stock duration={0} msec.", stockKeyFrameAnimation.Duration.TotalMilliseconds);
                        Verify.AreEqual(c_MaxStockZoomFactorChangeDuration, stockKeyFrameAnimation.Duration.TotalMilliseconds);
                        stockKeyFrameAnimation.Duration = TimeSpan.FromMilliseconds(10);
                    };

                    operation = StartZoomTo(
                        scroller,
                        8.0f,
                        100.0f,
                        150.0f,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scroller.HorizontalOffset}, VerticalOffset={scroller.VerticalOffset}, ZoomFactor={scroller.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.IsLessThan(Math.Abs(scroller.HorizontalOffset - 700.0), 0.01);
                    Verify.IsLessThan(Math.Abs(scroller.VerticalOffset - 1050.0), 0.01);
                    Verify.AreEqual(8.0f, scroller.ZoomFactor);

                    Verify.IsLessThanOrEqual(viewChangedCount, 2u);
                    Verify.AreEqual(ScrollerViewChangeResult.Completed, operation.Result);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Requests a view change just before unloading scroller.")]
        public void InterruptViewChangeWithUnloading()
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent, setAsContentRoot: true);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scroller.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                operation = StartScrollTo(
                    scroller,
                    600.0,
                    400.0,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvent);

                Log.Comment("Resetting window content to unparent Scroller");
                Content = null;
            });

            WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(0.0, scroller.HorizontalOffset);
                Verify.AreEqual(0.0, scroller.VerticalOffset);
                Verify.AreEqual(1.0f, scroller.ZoomFactor);
                Verify.AreEqual(ScrollerViewChangeResult.Interrupted, operation.Result);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Interrupts an animated offsets change with another one.")]
        public void InterruptOffsetsChangeWithOffsetsChange()
        {
            InterruptViewChange(ViewChangeInterruptionKind.OffsetsChangeByOffsetsChange);
        }

        [TestMethod]
        [TestProperty("Description", "Interrupts an animated offsets change with a zoomFactor change.")]
        public void InterruptOffsetsChangeWithZoomFactorChange()
        {
            InterruptViewChange(ViewChangeInterruptionKind.OffsetsChangeByZoomFactorChange);
        }

        [TestMethod]
        [TestProperty("Description", "Interrupts an animated zoomFactor change with an offsets change.")]
        public void InterruptZoomFactorChangeWithOffsetsChange()
        {
            InterruptViewChange(ViewChangeInterruptionKind.ZoomFactorChangeByOffsetsChange);
        }

        [TestMethod]
        [TestProperty("Description", "Interrupts an animated zoomFactor change with another one.")]
        public void InterruptZoomFactorChangeWithZoomFactorChange()
        {
            InterruptViewChange(ViewChangeInterruptionKind.ZoomFactorChangeByZoomFactorChange);
        }

        [TestMethod]
        [TestProperty("Description", "Attempts an offsets change while there is no content.")]
        public void OffsetsChangeWithNoContent()
        {
            Scroller scroller = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                scroller = new Scroller();

                SetupDefaultUI(scroller, null /*rectangleScrollerContent*/, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                operation = StartScrollTo(
                    scroller,
                    600.0,
                    400.0,
                    AnimationMode.Enabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvent);
            });

            WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(0.0, scroller.HorizontalOffset);
                Verify.AreEqual(0.0, scroller.VerticalOffset);
                Verify.AreEqual(1.0f, scroller.ZoomFactor);
                Verify.AreEqual(ScrollerViewChangeResult.Ignored, operation.Result);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Attempts a zoomFactor change while there is no content.")]
        public void ZoomFactorChangeWithNoContent()
        {
            Scroller scroller = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                scroller = new Scroller();

                SetupDefaultUI(scroller, null /*rectangleScrollerContent*/, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                operation = StartZoomTo(
                    scroller,
                    8.0f,
                    100.0f,
                    150.0f,
                    AnimationMode.Enabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvent);
            });

            WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(0.0, scroller.HorizontalOffset);
                Verify.AreEqual(0.0, scroller.VerticalOffset);
                Verify.AreEqual(1.0f, scroller.ZoomFactor);
                Verify.AreEqual(ScrollerViewChangeResult.Ignored, operation.Result);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Performs consecutive non-animated offsets changes.")]
        public void ConsecutiveOffsetJumps()
        {
            ConsecutiveOffsetJumps(waitForFirstCompletion: true);
            ConsecutiveOffsetJumps(waitForFirstCompletion: false);
        }

        private void ConsecutiveOffsetJumps(bool waitForFirstCompletion)
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollerViewChangeOperationEvents = null;
            ScrollerOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollerViewChangeOperationEvents = new AutoResetEvent[3];
                scrollerViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollerViewChangeOperationEvents[1] = new AutoResetEvent(false);
                scrollerViewChangeOperationEvents[2] = new AutoResetEvent(false);

                operations = new ScrollerOperation[3];

                scroller.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                operations[0] = StartScrollTo(
                    scroller,
                    600.0,
                    400.0,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvents[0]);

                if (!waitForFirstCompletion)
                {
                    operations[1] = StartScrollTo(
                        scroller,
                        500.0,
                        300.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[1]);
                }
            });

            WaitForEvent("Waiting for first view change completion", scrollerViewChangeOperationEvents[0]);

            if (waitForFirstCompletion)
            {
                RunOnUIThread.Execute(() =>
                {
                    operations[1] = StartScrollTo(
                        scroller,
                        500.0,
                        300.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[1]);
                });
            }

            WaitForEvent("Waiting for second view change completion", scrollerViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(500.0, scroller.HorizontalOffset);
                Verify.AreEqual(300.0, scroller.VerticalOffset);
                Verify.AreEqual(1.0f, scroller.ZoomFactor);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operations[0].Result);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operations[1].Result);

                // Jump to the same offsets.
                operations[2] = StartScrollTo(
                    scroller,
                    500.0,
                    300.0,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvents[2]);
            });

            WaitForEvent("Waiting for third view change completion", scrollerViewChangeOperationEvents[2]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(500.0, scroller.HorizontalOffset);
                Verify.AreEqual(300.0, scroller.VerticalOffset);
                Verify.AreEqual(1.0f, scroller.ZoomFactor);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operations[2].Result);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Performs consecutive non-animated zoomFactor changes.")]
        public void ConsecutiveZoomFactorJumps()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1 because InteractionTracker jumps to different VerticalOffset.");
                return;
            }

            ConsecutiveZoomFactorJumps(isFirstZoomRelative: false, isSecondZoomRelative: false, waitForFirstCompletion:  true);
            ConsecutiveZoomFactorJumps(isFirstZoomRelative: false, isSecondZoomRelative:  true, waitForFirstCompletion:  true);
            ConsecutiveZoomFactorJumps(isFirstZoomRelative:  true, isSecondZoomRelative: false, waitForFirstCompletion:  true);
            ConsecutiveZoomFactorJumps(isFirstZoomRelative:  true, isSecondZoomRelative:  true, waitForFirstCompletion:  true);
            ConsecutiveZoomFactorJumps(isFirstZoomRelative: false, isSecondZoomRelative: false, waitForFirstCompletion: false);
            ConsecutiveZoomFactorJumps(isFirstZoomRelative: false, isSecondZoomRelative:  true, waitForFirstCompletion: false);
            ConsecutiveZoomFactorJumps(isFirstZoomRelative:  true, isSecondZoomRelative: false, waitForFirstCompletion: false);
            ConsecutiveZoomFactorJumps(isFirstZoomRelative:  true, isSecondZoomRelative:  true, waitForFirstCompletion: false);
        }

        private void ConsecutiveZoomFactorJumps(bool isFirstZoomRelative, bool isSecondZoomRelative, bool waitForFirstCompletion)
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollerViewChangeOperationEvents = null;
            ScrollerOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollerViewChangeOperationEvents = new AutoResetEvent[3];
                scrollerViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollerViewChangeOperationEvents[1] = new AutoResetEvent(false);
                scrollerViewChangeOperationEvents[2] = new AutoResetEvent(false);

                operations = new ScrollerOperation[3];

                scroller.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                if (isFirstZoomRelative)
                {
                    operations[0] = StartZoomBy(
                        scroller,
                        7.0f,
                        150.0f,
                        120.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartZoomTo(
                        scroller,
                        8.0f,
                        150.0f,
                        120.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[0]);
                }
            });

            if (waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollerViewChangeOperationEvents[0]);
            }

            RunOnUIThread.Execute(() =>
            {
                if (isFirstZoomRelative)
                {
                    operations[1] = StartZoomBy(
                        scroller,
                        -1.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[1]);
                }
                else
                {
                    operations[1] = StartZoomTo(
                        scroller,
                        7.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[1]);
                }
            });

            if (!waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollerViewChangeOperationEvents[0]);
            }
            WaitForEvent("Waiting for second view change completion", scrollerViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(917.5, scroller.HorizontalOffset);
                Verify.AreEqual(723.75, scroller.VerticalOffset);
                Verify.AreEqual(7.0f, scroller.ZoomFactor);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operations[0].Result);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operations[1].Result);

                // Jump to the same zoomFactor
                operations[2] = StartZoomTo(
                    scroller,
                    7.0f,
                    10.0f,
                    90.0f,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvents[2]);
            });

            WaitForEvent("Waiting for third view change completion", scrollerViewChangeOperationEvents[2]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(917.5, scroller.HorizontalOffset);
                Verify.AreEqual(723.75, scroller.VerticalOffset);
                Verify.AreEqual(7.0f, scroller.ZoomFactor);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operations[2].Result);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Performs consecutive non-animated offsets and zoom factor changes.")]
        public void ConsecutiveScrollAndZoomJumps()
        {
            ConsecutiveScrollAndZoomJumps(isScrollRelative: false, isZoomRelative: false, waitForFirstCompletion:  true);
            ConsecutiveScrollAndZoomJumps(isScrollRelative: false, isZoomRelative:  true, waitForFirstCompletion:  true);
            ConsecutiveScrollAndZoomJumps(isScrollRelative:  true, isZoomRelative: false, waitForFirstCompletion:  true);
            ConsecutiveScrollAndZoomJumps(isScrollRelative:  true, isZoomRelative:  true, waitForFirstCompletion:  true);
            ConsecutiveScrollAndZoomJumps(isScrollRelative: false, isZoomRelative: false, waitForFirstCompletion: false);
            ConsecutiveScrollAndZoomJumps(isScrollRelative: false, isZoomRelative:  true, waitForFirstCompletion: false);
            ConsecutiveScrollAndZoomJumps(isScrollRelative:  true, isZoomRelative: false, waitForFirstCompletion: false);
            ConsecutiveScrollAndZoomJumps(isScrollRelative:  true, isZoomRelative:  true, waitForFirstCompletion: false);
        }

        private void ConsecutiveScrollAndZoomJumps(bool isScrollRelative, bool isZoomRelative, bool waitForFirstCompletion)
        { 
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollerViewChangeOperationEvents = null;
            ScrollerOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollerViewChangeOperationEvents = new AutoResetEvent[2];
                scrollerViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollerViewChangeOperationEvents[1] = new AutoResetEvent(false);

                operations = new ScrollerOperation[2];

                scroller.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                if (isScrollRelative)
                {
                    operations[0] = StartScrollBy(
                        scroller,
                        80.0,
                        35.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartScrollTo(
                        scroller,
                        80.0,
                        35.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[0]);
                }
            });

            if (waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollerViewChangeOperationEvents[0]);
            }

            RunOnUIThread.Execute(() =>
            {
                if (isZoomRelative)
                {
                    operations[1] = StartZoomBy(
                        scroller,
                        2.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[1]);
                }
                else
                {
                    operations[1] = StartZoomTo(
                        scroller,
                        3.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[1]);
                }
            });

            if (!waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollerViewChangeOperationEvents[0]);
            }
            WaitForEvent("Waiting for second view change completion", scrollerViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(260.0, scroller.HorizontalOffset);
                Verify.AreEqual(285.0, scroller.VerticalOffset);
                Verify.AreEqual(3.0f, scroller.ZoomFactor);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operations[0].Result);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operations[1].Result);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Performs consecutive non-animated zoom factor and offsets changes.")]
        public void ConsecutiveZoomAndScrollJumps()
        {
            ConsecutiveZoomAndScrollJumps(isZoomRelative: false, isScrollRelative: false, waitForFirstCompletion:  true);
            ConsecutiveZoomAndScrollJumps(isZoomRelative: false, isScrollRelative:  true, waitForFirstCompletion:  true);
            ConsecutiveZoomAndScrollJumps(isZoomRelative:  true, isScrollRelative: false, waitForFirstCompletion:  true);
            ConsecutiveZoomAndScrollJumps(isZoomRelative:  true, isScrollRelative:  true, waitForFirstCompletion:  true);
            ConsecutiveZoomAndScrollJumps(isZoomRelative: false, isScrollRelative: false, waitForFirstCompletion: false);
            ConsecutiveZoomAndScrollJumps(isZoomRelative: false, isScrollRelative:  true, waitForFirstCompletion: false);
            ConsecutiveZoomAndScrollJumps(isZoomRelative:  true, isScrollRelative: false, waitForFirstCompletion: false);
            ConsecutiveZoomAndScrollJumps(isZoomRelative:  true, isScrollRelative:  true, waitForFirstCompletion: false);
        }

        private void ConsecutiveZoomAndScrollJumps(bool isZoomRelative, bool isScrollRelative, bool waitForFirstCompletion)
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollerViewChangeOperationEvents = null;
            ScrollerOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollerViewChangeOperationEvents = new AutoResetEvent[2];
                scrollerViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollerViewChangeOperationEvents[1] = new AutoResetEvent(false);

                operations = new ScrollerOperation[2];

                scroller.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                if (isZoomRelative)
                {
                    operations[0] = StartZoomBy(
                        scroller,
                        2.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartZoomTo(
                        scroller,
                        3.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[0]);
                }
            });

            if (waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollerViewChangeOperationEvents[0]);
            }

            RunOnUIThread.Execute(() =>
            {
                if (isScrollRelative)
                {
                    operations[1] = StartScrollBy(
                        scroller,
                        80.0,
                        35.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[1]);
                }
                else
                {
                    operations[1] = StartScrollTo(
                        scroller,
                        80.0,
                        35.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[1]);
                }
            });

            if (!waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollerViewChangeOperationEvents[0]);
            }
            WaitForEvent("Waiting for second view change completion", scrollerViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                if (isScrollRelative)
                {
                    Verify.AreEqual(100.0, scroller.HorizontalOffset);
                    Verify.AreEqual(215.0, scroller.VerticalOffset);
                }
                else
                {
                    Verify.AreEqual(80.0, scroller.HorizontalOffset);
                    Verify.AreEqual(35.0, scroller.VerticalOffset);
                }
                Verify.AreEqual(3.0f, scroller.ZoomFactor);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operations[0].Result);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operations[1].Result);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Requests a non-animated offsets change before loading scroller.")]
        public void SetOffsetsBeforeLoading()
        {
            ChangeOffsetsBeforeLoading(false /*animate*/);
        }

        [TestMethod]
        [TestProperty("Description", "Requests an animated offsets change before loading scroller.")]
        public void AnimateOffsetsBeforeLoading()
        {
            ChangeOffsetsBeforeLoading(true /*animate*/);
        }

        [TestMethod]
        [TestProperty("Description", "Requests a non-animated zoomFactor change before loading scroller.")]
        public void SetZoomFactorBeforeLoading()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1 because InteractionTracker jumps to different VerticalOffset.");
                return;
            }

            ChangeZoomFactorBeforeLoading(false /*animate*/);
        }

        [TestMethod]
        [TestProperty("Description", "Requests an animated zoomFactor change before loading scroller.")]
        public void AnimateZoomFactorBeforeLoading()
        {
            ChangeZoomFactorBeforeLoading(true /*animate*/);
        }

        [TestMethod]
        [TestProperty("Description", "Requests a non-animated offset change immediately after increasing content size.")]
        public void OffsetJumpAfterContentResizing()
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent.Width = c_defaultUIScrollerContentWidth + 200.0;
            });

            // Jump to absolute offsets
            ScrollTo(
                scroller,
                c_defaultUIScrollerContentWidth + 200.0 - c_defaultUIScrollerWidth,
                c_defaultVerticalOffset,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore);
        }

        private void ChangeOffsetsBeforeLoading(bool animate)
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent, false /*setAsContentRoot*/);

                scroller.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                operation = StartScrollTo(
                    scroller,
                    600.0,
                    400.0,
                    animate ? AnimationMode.Enabled : AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvent);

                Log.Comment("Setting window content");
                Content = scroller;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(600.0, scroller.HorizontalOffset);
                Verify.AreEqual(400.0, scroller.VerticalOffset);
                Verify.AreEqual(1.0f, scroller.ZoomFactor);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operation.Result);
            });
        }

        private void ChangeZoomFactorBeforeLoading(bool animate)
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent, false /*setAsContentRoot*/);

                scroller.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                operation = StartZoomTo(
                    scroller,
                    8.0f,
                    100.0f,
                    150.0f,
                    animate ? AnimationMode.Enabled : AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvent);

                Log.Comment("Setting window content");
                Content = scroller;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.IsLessThan(Math.Abs(scroller.HorizontalOffset - 700.0), 0.01);
                Verify.IsLessThan(Math.Abs(scroller.VerticalOffset - 1050.0), 0.01);
                Verify.AreEqual(8.0f, scroller.ZoomFactor);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operation.Result);
            });
        }

        private void ScrollTo(
            Scroller scroller,
            double horizontalOffset,
            double verticalOffset,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            bool? isAnimationsEnabledOverride = null,
            double? expectedFinalHorizontalOffset = null,
            double? expectedFinalVerticalOffset = null)
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true,
                isAnimationsEnabledOverride: isAnimationsEnabledOverride))
            {
                Log.Comment("Waiting for any pending ExpressionAnimation start/stop notifications to occur");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollerOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    scrollerTestHooksHelper.ResetExpressionAnimationStatusChanges(scroller);

                    if (hookViewChanged)
                    {
                        scroller.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalZoomFactor = scroller.ZoomFactor;

                    if (expectedFinalHorizontalOffset == null)
                    {
                        expectedFinalHorizontalOffset = horizontalOffset;
                    }

                    if (expectedFinalVerticalOffset == null)
                    {
                        expectedFinalVerticalOffset = verticalOffset;
                    }

                    operation = StartScrollTo(
                        scroller,
                        horizontalOffset,
                        verticalOffset,
                        animationMode,
                        snapPointsMode,
                        scrollerViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scroller.HorizontalOffset}, VerticalOffset={scroller.VerticalOffset}, ZoomFactor={scroller.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(expectedFinalHorizontalOffset, scroller.HorizontalOffset);
                    Verify.AreEqual(expectedFinalVerticalOffset, scroller.VerticalOffset);
                    Verify.AreEqual(originalZoomFactor, scroller.ZoomFactor);
                    Verify.AreEqual(ScrollerViewChangeResult.Completed, operation.Result);

                    if (GetEffectiveIsAnimationEnabled(animationMode, isAnimationsEnabledOverride))
                    {
                        Verify.IsFalse(viewChangedCount == 1u);
                    }
                    else
                    {
                        Verify.IsLessThanOrEqual(viewChangedCount, 1u);
                    }
                });

                Log.Comment("Waiting for any ExpressionAnimation start/stop notification");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                RunOnUIThread.Execute(() =>
                {
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollerTestHooksHelper.GetExpressionAnimationStatusChanges(scroller);
                    ScrollerTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    Verify.IsNull(expressionAnimationStatusChanges);
                });
            }
        }

        private void ScrollBy(
            Scroller scroller,
            double horizontalOffsetDelta,
            double verticalOffsetDelta,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            bool? isAnimationsEnabledOverride = null,
            double? expectedFinalHorizontalOffset = null,
            double? expectedFinalVerticalOffset = null)
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true,
                isAnimationsEnabledOverride: isAnimationsEnabledOverride))
            {
                Log.Comment("Waiting for any pending ExpressionAnimation start/stop notifications to occur");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                double originalHorizontalOffset = 0.0;
                double originalVerticalOffset = 0.0;
                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollerOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    scrollerTestHooksHelper.ResetExpressionAnimationStatusChanges(scroller);

                    if (hookViewChanged)
                    {
                        scroller.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalHorizontalOffset = scroller.HorizontalOffset;
                    originalVerticalOffset = scroller.VerticalOffset;
                    originalZoomFactor = scroller.ZoomFactor;

                    Log.Comment($"Original HorizontalOffset={originalHorizontalOffset}, VerticalOffset={originalVerticalOffset}, ZoomFactor={originalZoomFactor}");

                    if (expectedFinalHorizontalOffset == null)
                    {
                        expectedFinalHorizontalOffset = horizontalOffsetDelta + originalHorizontalOffset;
                    }

                    if (expectedFinalVerticalOffset == null)
                    {
                        expectedFinalVerticalOffset = verticalOffsetDelta + originalVerticalOffset;
                    }

                    operation = StartScrollBy(
                        scroller,
                        horizontalOffsetDelta,
                        verticalOffsetDelta,
                        animationMode,
                        snapPointsMode,
                        scrollerViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scroller.HorizontalOffset}, VerticalOffset={scroller.VerticalOffset}, ZoomFactor={scroller.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(expectedFinalHorizontalOffset, scroller.HorizontalOffset);
                    Verify.AreEqual(expectedFinalVerticalOffset, scroller.VerticalOffset);
                    Verify.AreEqual(originalZoomFactor, scroller.ZoomFactor);
                    Verify.AreEqual(ScrollerViewChangeResult.Completed, operation.Result);

                    if (GetEffectiveIsAnimationEnabled(animationMode, isAnimationsEnabledOverride))
                    {
                        Verify.IsGreaterThan(viewChangedCount, 1u);
                    }
                    else
                    {
                        Verify.AreEqual(1u, viewChangedCount);
                    }
                });

                Log.Comment("Waiting for any ExpressionAnimation start/stop notification");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                RunOnUIThread.Execute(() =>
                {
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollerTestHooksHelper.GetExpressionAnimationStatusChanges(scroller);
                    ScrollerTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    Verify.IsNull(expressionAnimationStatusChanges);
                });
            }
        }

        private void ScrollFrom(
            Scroller scroller,
            float horizontalVelocity,
            float verticalVelocity,
            float? horizontalInertiaDecayRate,
            float? verticalInertiaDecayRate,
            bool hookViewChanged = true)
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true))
            {
                Log.Comment("Waiting for any pending ExpressionAnimation start/stop notifications to occur");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                double originalHorizontalOffset = 0.0;
                double originalVerticalOffset = 0.0;
                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollerOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    scrollerTestHooksHelper.ResetExpressionAnimationStatusChanges(scroller);

                    if (hookViewChanged)
                    {
                        scroller.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalHorizontalOffset = scroller.HorizontalOffset;
                    originalVerticalOffset = scroller.VerticalOffset;
                    originalZoomFactor = scroller.ZoomFactor;

                    Log.Comment($"Original HorizontalOffset={originalHorizontalOffset}, VerticalOffset={originalVerticalOffset}, ZoomFactor={originalZoomFactor}");

                    operation = StartScrollFrom(
                        scroller,
                        horizontalVelocity,
                        verticalVelocity,
                        horizontalInertiaDecayRate,
                        verticalInertiaDecayRate,
                        scrollerViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                    if (horizontalVelocity > 0)
                        Verify.IsTrue(originalHorizontalOffset < scroller.HorizontalOffset);
                    else if (horizontalVelocity < 0)
                        Verify.IsTrue(originalHorizontalOffset > scroller.HorizontalOffset);
                    else
                        Verify.IsTrue(originalHorizontalOffset == scroller.HorizontalOffset);
                    if (verticalVelocity > 0)
                        Verify.IsTrue(originalVerticalOffset < scroller.VerticalOffset);
                    else if (verticalVelocity < 0)
                        Verify.IsTrue(originalVerticalOffset > scroller.VerticalOffset);
                    else
                        Verify.IsTrue(originalVerticalOffset == scroller.VerticalOffset);
                    Verify.AreEqual(originalZoomFactor, scroller.ZoomFactor);
                    Verify.AreEqual(ScrollerViewChangeResult.Completed, operation.Result);
                });

                Log.Comment("Waiting for any ExpressionAnimation start/stop notification");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                RunOnUIThread.Execute(() =>
                {
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollerTestHooksHelper.GetExpressionAnimationStatusChanges(scroller);
                    ScrollerTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    Verify.IsNull(expressionAnimationStatusChanges);
                });
            }
        }

        private ScrollerOperation StartScrollTo(
            Scroller scroller,
            double horizontalOffset,
            double verticalOffset,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            AutoResetEvent scrollerViewChangeOperationEvent)
        {
            Log.Comment("ScrollTo - horizontalOffset={0}, verticalOffset={1}, animationMode={2}, snapPointsMode={3}",
                horizontalOffset, verticalOffset, animationMode, snapPointsMode);

            viewChangedCount = 0u;
            ScrollerOperation operation = new ScrollerOperation();

            operation.Id = scroller.ScrollTo(
                horizontalOffset,
                verticalOffset,
                new ScrollOptions(animationMode, snapPointsMode)).OffsetsChangeId;

            if (operation.Id == -1)
            {
                scrollerViewChangeOperationEvent.Set();
            }
            else
            {
                scroller.ScrollCompleted += (Scroller sender, ScrollCompletedEventArgs args) =>
                {
                    if (args.ScrollInfo.OffsetsChangeId == operation.Id)
                    {
                        ScrollerViewChangeResult result = ScrollerTestHooks.GetScrollCompletedResult(args);

                        Log.Comment("ScrollCompleted: ScrollTo OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollerViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollerOperation StartScrollBy(
            Scroller scroller,
            double horizontalOffsetDelta,
            double verticalOffsetDelta,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            AutoResetEvent scrollerViewChangeOperationEvent)
        {
            Log.Comment("ScrollBy - horizontalOffsetDelta={0}, verticalOffsetDelta={1}, animationMode={2}, snapPointsMode={3}",
                horizontalOffsetDelta, verticalOffsetDelta, animationMode, snapPointsMode);

            viewChangedCount = 0u;
            ScrollerOperation operation = new ScrollerOperation();

            operation.Id = scroller.ScrollBy(
                horizontalOffsetDelta,
                verticalOffsetDelta,
                new ScrollOptions(animationMode, snapPointsMode)).OffsetsChangeId;

            if (operation.Id == -1)
            {
                scrollerViewChangeOperationEvent.Set();
            }
            else
            {
                scroller.ScrollCompleted += (Scroller sender, ScrollCompletedEventArgs args) =>
                {
                    if (args.ScrollInfo.OffsetsChangeId == operation.Id)
                    {
                        ScrollerViewChangeResult result = ScrollerTestHooks.GetScrollCompletedResult(args);

                        Log.Comment("ScrollCompleted: ScrollBy OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollerViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollerOperation StartScrollFrom(
            Scroller scroller,
            float horizontalVelocity,
            float verticalVelocity,
            float? horizontalInertiaDecayRate,
            float? verticalInertiaDecayRate,
            AutoResetEvent scrollerViewChangeOperationEvent)
        {
            Log.Comment("ScrollFrom - horizontalVelocity={0}, verticalVelocity={1}, horizontalInertiaDecayRate={2}, verticalInertiaDecayRate={3}",
                horizontalVelocity, verticalVelocity, horizontalInertiaDecayRate, verticalInertiaDecayRate);

            Vector2? inertiaDecayRate = null;

            if (horizontalInertiaDecayRate != null && verticalInertiaDecayRate != null)
            {
                inertiaDecayRate = new Vector2((float)horizontalInertiaDecayRate, (float)verticalInertiaDecayRate);
            }

            viewChangedCount = 0u;
            ScrollerOperation operation = new ScrollerOperation();

            operation.Id = scroller.ScrollFrom(
                    new Vector2(horizontalVelocity, verticalVelocity),
                    inertiaDecayRate).OffsetsChangeId;

            if (operation.Id == -1)
            {
                scrollerViewChangeOperationEvent.Set();
            }
            else
            {
                scroller.ScrollCompleted += (Scroller sender, ScrollCompletedEventArgs args) =>
                {
                    if (args.ScrollInfo.OffsetsChangeId == operation.Id)
                    {
                        ScrollerViewChangeResult result = ScrollerTestHooks.GetScrollCompletedResult(args);

                        Log.Comment("ScrollCompleted: ScrollFrom OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollerViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private void ZoomTo(
            Scroller scroller,
            float zoomFactor,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            bool? isAnimationsEnabledOverride = null)
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true, 
                isAnimationsEnabledOverride: isAnimationsEnabledOverride))
            {
                AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollerOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    if (hookViewChanged)
                    {
                        scroller.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    operation = StartZoomTo(
                        scroller,
                        zoomFactor,
                        centerPointX,
                        centerPointY,
                        animationMode,
                        snapPointsMode,
                        scrollerViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scroller.HorizontalOffset}, VerticalOffset={scroller.VerticalOffset}, ZoomFactor={scroller.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(zoomFactor, scroller.ZoomFactor);
                    Verify.AreEqual(ScrollerViewChangeResult.Completed, operation.Result);

                    if (GetEffectiveIsAnimationEnabled(animationMode, isAnimationsEnabledOverride))
                    {
                        Verify.IsGreaterThan(viewChangedCount, 1u);
                    }
                    else
                    {
                        Verify.AreEqual(1u, viewChangedCount);
                    }
                });

                Log.Comment("Waiting for any ExpressionAnimation start/stop notification");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                RunOnUIThread.Execute(() =>
                {
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollerTestHooksHelper.GetExpressionAnimationStatusChanges(scroller);
                    ScrollerTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    VerifyExpressionAnimationStatusChangesForTranslationAndZoomFactorSuspension(expressionAnimationStatusChanges);
                });
            }
        }

        private void ZoomBy(
            Scroller scroller,
            float zoomFactorDelta,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            bool? isAnimationsEnabledOverride = null)
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true,
                isAnimationsEnabledOverride: isAnimationsEnabledOverride))
            {
                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollerOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    if (hookViewChanged)
                    {
                        scroller.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalZoomFactor = scroller.ZoomFactor;

                    Log.Comment($"Original HorizontalOffset={scroller.HorizontalOffset}, VerticalOffset={scroller.VerticalOffset}, ZoomFactor={originalZoomFactor}");

                    operation = StartZoomBy(
                        scroller,
                        zoomFactorDelta,
                        centerPointX,
                        centerPointY,
                        animationMode,
                        snapPointsMode,
                        scrollerViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scroller.HorizontalOffset}, VerticalOffset={scroller.VerticalOffset}, ZoomFactor={scroller.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(zoomFactorDelta + originalZoomFactor, scroller.ZoomFactor);
                    Verify.AreEqual(ScrollerViewChangeResult.Completed, operation.Result);

                    if (GetEffectiveIsAnimationEnabled(animationMode, isAnimationsEnabledOverride))
                    {
                        Verify.IsGreaterThan(viewChangedCount, 1u);
                    }
                    else
                    {
                        Verify.AreEqual(1u, viewChangedCount);
                    }
                });

                Log.Comment("Waiting for any ExpressionAnimation start/stop notification");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                RunOnUIThread.Execute(() =>
                {
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollerTestHooksHelper.GetExpressionAnimationStatusChanges(scroller);
                    ScrollerTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    VerifyExpressionAnimationStatusChangesForTranslationAndZoomFactorSuspension(expressionAnimationStatusChanges);                    
                });
            }
        }

        private void ZoomFrom(
            Scroller scroller,
            float zoomFactorVelocity,
            float? inertiaDecayRate,
            float centerPointX,
            float centerPointY,
            bool hookViewChanged = true)
        {
            using (ScrollerTestHooksHelper scrollerTestHooksHelper = new ScrollerTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true))
            {
                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollerOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    if (hookViewChanged)
                    {
                        scroller.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalZoomFactor = scroller.ZoomFactor;

                    Log.Comment($"Original HorizontalOffset={scroller.HorizontalOffset}, VerticalOffset={scroller.VerticalOffset}, ZoomFactor={originalZoomFactor}");

                    operation = StartZoomFrom(
                        scroller,
                        zoomFactorVelocity,
                        inertiaDecayRate,
                        centerPointX,
                        centerPointY,
                        scrollerViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                    if (zoomFactorVelocity > 0)
                        Verify.IsTrue(originalZoomFactor < scroller.ZoomFactor);
                    else if (zoomFactorVelocity < 0)
                        Verify.IsTrue(originalZoomFactor > scroller.ZoomFactor);
                    else
                        Verify.IsTrue(originalZoomFactor == scroller.ZoomFactor);
                    Verify.AreEqual(ScrollerViewChangeResult.Completed, operation.Result);
                });

                Log.Comment("Waiting for any ExpressionAnimation start/stop notification");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                RunOnUIThread.Execute(() =>
                {
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollerTestHooksHelper.GetExpressionAnimationStatusChanges(scroller);
                    ScrollerTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    VerifyExpressionAnimationStatusChangesForTranslationAndZoomFactorSuspension(expressionAnimationStatusChanges);
                });
            }
        }

        private ScrollerOperation StartZoomTo(
            Scroller scroller,
            float zoomFactor,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            AutoResetEvent scrollerViewChangeOperationEvent)
        {
            Log.Comment("ZoomTo - zoomFactor={0}, centerPoint=({1},{2}), animationMode={3}, snapPointsMode={4}",
                zoomFactor, centerPointX, centerPointY, animationMode, snapPointsMode);

            viewChangedCount = 0u;
            ScrollerOperation operation = new ScrollerOperation();

            operation.Id = scroller.ZoomTo(
                zoomFactor,
                new Vector2(centerPointX, centerPointY), 
                new ZoomOptions(animationMode, snapPointsMode)).ZoomFactorChangeId;

            if (operation.Id == -1)
            {
                scrollerViewChangeOperationEvent.Set();
            }
            else
            {
                scroller.ZoomCompleted += (Scroller sender, ZoomCompletedEventArgs args) =>
                {
                    if (args.ZoomInfo.ZoomFactorChangeId == operation.Id)
                    {
                        ScrollerViewChangeResult result = ScrollerTestHooks.GetZoomCompletedResult(args);

                        Log.Comment("ZoomCompleted: ZoomTo ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollerViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollerOperation StartZoomBy(
            Scroller scroller,
            float zoomFactorDelta,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            AutoResetEvent scrollerViewChangeOperationEvent)
        {
            Log.Comment("ZoomBy - zoomFactorDelta={0}, centerPoint=({1},{2}), animationMode={3}, snapPointsMode={4}",
                zoomFactorDelta, centerPointX, centerPointY, animationMode, snapPointsMode);

            viewChangedCount = 0u;
            ScrollerOperation operation = new ScrollerOperation();

            operation.Id = scroller.ZoomBy(
                zoomFactorDelta,
                new Vector2(centerPointX, centerPointY),
                new ZoomOptions(animationMode, snapPointsMode)).ZoomFactorChangeId;

            if (operation.Id == -1)
            {
                scrollerViewChangeOperationEvent.Set();
            }
            else
            {
                scroller.ZoomCompleted += (Scroller sender, ZoomCompletedEventArgs args) =>
                {
                    if (args.ZoomInfo.ZoomFactorChangeId == operation.Id)
                    {
                        ScrollerViewChangeResult result = ScrollerTestHooks.GetZoomCompletedResult(args);

                        Log.Comment("ZoomCompleted: ZoomBy ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollerViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollerOperation StartZoomFrom(
            Scroller scroller,
            float zoomFactorVelocity,
            float? inertiaDecayRate,
            float centerPointX,
            float centerPointY,
            AutoResetEvent scrollerViewChangeOperationEvent)
        {
            Log.Comment("ZoomFrom - zoomFactorVelocity={0}, inertiaDecayRate={1}, centerPoint=({2},{3})",
                zoomFactorVelocity, inertiaDecayRate, centerPointX, centerPointY);

            viewChangedCount = 0u;
            ScrollerOperation operation = new ScrollerOperation();

            operation.Id = scroller.ZoomFrom(
                zoomFactorVelocity, new Vector2(centerPointX, centerPointY), inertiaDecayRate).ZoomFactorChangeId;

            if (operation.Id == -1)
            {
                scrollerViewChangeOperationEvent.Set();
            }
            else
            {
                scroller.ZoomCompleted += (Scroller sender, ZoomCompletedEventArgs args) =>
                {
                    if (args.ZoomInfo.ZoomFactorChangeId == operation.Id)
                    {
                        ScrollerViewChangeResult result = ScrollerTestHooks.GetZoomCompletedResult(args);

                        Log.Comment("ZoomCompleted: ZoomFrom ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollerViewChangeOperationEvent.Set();
                    }
                };
            }
            return operation;
        }

        private void InterruptViewChange(
            ViewChangeInterruptionKind viewChangeInterruptionKind)
        {
            bool viewChangeInterruptionDone = false;
            bool changeOffsetsFirst =
                viewChangeInterruptionKind == ViewChangeInterruptionKind.OffsetsChangeByOffsetsChange || viewChangeInterruptionKind == ViewChangeInterruptionKind.OffsetsChangeByZoomFactorChange;
            bool changeOffsetsSecond =
                viewChangeInterruptionKind == ViewChangeInterruptionKind.OffsetsChangeByOffsetsChange || viewChangeInterruptionKind == ViewChangeInterruptionKind.ZoomFactorChangeByOffsetsChange;
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollerViewChangeOperationEvents = null;
            ScrollerOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerContent = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerContent, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollerViewChangeOperationEvents = new AutoResetEvent[2];
                scrollerViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollerViewChangeOperationEvents[1] = new AutoResetEvent(false);

                operations = new ScrollerOperation[2];
                operations[0] = null;
                operations[1] = null;

                scroller.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");

                    if (viewChangedCount == 3u && !viewChangeInterruptionDone)
                    {
                        viewChangeInterruptionDone = true;
                        if (changeOffsetsSecond)
                        {
                            operations[1] = StartScrollTo(
                                scroller,
                                500.0,
                                300.0,
                                AnimationMode.Enabled,
                                SnapPointsMode.Ignore,
                                scrollerViewChangeOperationEvents[1]);
                        }
                        else
                        {
                            operations[1] = StartZoomTo(
                                scroller,
                                7.0f,
                                70.0f,
                                50.0f,
                                AnimationMode.Enabled,
                                SnapPointsMode.Ignore,
                                scrollerViewChangeOperationEvents[1]);
                        }
                    }
                };

                if (changeOffsetsFirst)
                {
                    operations[0] = StartScrollTo(
                        scroller,
                        600.0,
                        400.0,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartZoomTo(
                        scroller,
                        8.0f,
                        100.0f,
                        150.0f,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollerViewChangeOperationEvents[0]);
                }
            });

            WaitForEvent("Waiting for first view change completion", scrollerViewChangeOperationEvents[0]);

            WaitForEvent("Waiting for second view change completion", scrollerViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                if (changeOffsetsFirst && changeOffsetsSecond)
                {
                    Verify.AreEqual(500.0, scroller.HorizontalOffset);
                    Verify.AreEqual(300.0, scroller.VerticalOffset);
                    Verify.AreEqual(1.0f, scroller.ZoomFactor);
                }
                if (changeOffsetsFirst && !changeOffsetsSecond)
                {
                    Verify.IsGreaterThanOrEqual(scroller.HorizontalOffset, 600.0);
                    Verify.IsGreaterThanOrEqual(scroller.VerticalOffset, 400.0);
                    Verify.AreEqual(7.0f, scroller.ZoomFactor);
                }
                if (!changeOffsetsFirst && changeOffsetsSecond)
                {
                    Verify.IsGreaterThanOrEqual(scroller.HorizontalOffset, 500.0);
                    Verify.IsGreaterThanOrEqual(scroller.VerticalOffset, 300.0);
                    Verify.AreEqual(8.0f, scroller.ZoomFactor);
                }
                if (!changeOffsetsFirst && !changeOffsetsSecond)
                {
                    Verify.AreEqual(7.0f, scroller.ZoomFactor);
                }

                Verify.AreEqual(ScrollerViewChangeResult.Interrupted, operations[0].Result);
                Verify.AreEqual(ScrollerViewChangeResult.Completed, operations[1].Result);
            });
        }

        private void WaitForEvent(string logComment, EventWaitHandle eventWaitHandle)
        {
            Log.Comment(logComment);
            if (Debugger.IsAttached)
            {
                eventWaitHandle.WaitOne();
            }
            else
            {
                if (!eventWaitHandle.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration)))
                {
                    throw new Exception("Timeout expiration in WaitForEvent.");
                }
            }
        }

        private void VerifyExpressionAnimationStatusChangesForTranslationAndZoomFactorSuspension(List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges)
        {
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                // Facades are enabled. The translation and zoom factor animations are expected to be interrupted momentarily.
                Verify.IsNotNull(expressionAnimationStatusChanges);
                Verify.AreEqual(4, expressionAnimationStatusChanges.Count);

                Verify.IsFalse(expressionAnimationStatusChanges[0].IsExpressionAnimationStarted);
                Verify.IsFalse(expressionAnimationStatusChanges[1].IsExpressionAnimationStarted);
                Verify.IsTrue(expressionAnimationStatusChanges[2].IsExpressionAnimationStarted);
                Verify.IsTrue(expressionAnimationStatusChanges[3].IsExpressionAnimationStarted);

                Verify.AreEqual("Translation", expressionAnimationStatusChanges[0].PropertyName);
                Verify.AreEqual("Scale", expressionAnimationStatusChanges[1].PropertyName);
                Verify.AreEqual("Translation", expressionAnimationStatusChanges[2].PropertyName);
                Verify.AreEqual("Scale", expressionAnimationStatusChanges[3].PropertyName);
            }
            else
            {
                Verify.IsNull(expressionAnimationStatusChanges);
            }
        }

        private bool GetEffectiveIsAnimationEnabled(AnimationMode animationMode, bool? isAnimationsEnabledOverride)
        {
            switch (animationMode)
            {
                case AnimationMode.Auto:
                    switch (isAnimationsEnabledOverride)
                    {
                        case null:
                            return new UISettings().AnimationsEnabled;
                        default :
                            return isAnimationsEnabledOverride.Value;
                    }
                case AnimationMode.Enabled:
                    return true;
            }

            return false;
        }

        private class ScrollerOperation
        {
            public ScrollerOperation()
            {
                Result = ScrollerViewChangeResult.Ignored;
                Id = -1;
            }

            public ScrollerViewChangeResult Result
            {
                get;
                set;
            }

            public int Id
            {
                get;
                set;
            }
        }
    }
}
