// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System;
using System.Diagnostics;
using System.Numerics;
using System.Threading;
using Windows.Foundation;
using Windows.UI.Composition;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Shapes;

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
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollerTests
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

        [TestCleanup]
        public void TestCleanup()
        {
            RunOnUIThread.Execute(() =>
            {
                MUXControlsTestApp.App.TestContentRoot = null;
            });
        }

        //[TestMethod]
        //[TestProperty("Description", "Changes Scroller offsets using ScrollTo, ScrollBy, ScrollFrom and AnimationMode/SnapPointsMode enum values.")]
        // Disabled due to: Multiple unreliable Scroller tests #136
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

            // Jump to relative offsets
            ScrollBy(scroller, -4.0, 15.0, AnimationMode.Disabled, SnapPointsMode.Ignore, false /*hookViewChanged*/);

            // Animate to absolute offsets
            ScrollTo(scroller, 55.0, 25.0, AnimationMode.Enabled, SnapPointsMode.Ignore, false /*hookViewChanged*/);

            // Animate to relative offsets
            ScrollBy(scroller, 700.0, -8.0, AnimationMode.Enabled, SnapPointsMode.Ignore, false /*hookViewChanged*/);

            // Flick with additional offsets velocity
            ScrollFrom(scroller, -65.0f, 80.0f, null /*horizontalInertiaDecayRate*/, null /*verticalInertiaDecayRate*/, false /*hookViewChanged*/);

            // Flick with additional offsets velocity and custom scroll inertia decay rate
            ScrollFrom(scroller, 65.0f, -80.0f, 0.7f /*horizontalInertiaDecayRate*/, 0.8f /*verticalInertiaDecayRate*/, false /*hookViewChanged*/);

            // Do it all again while respecting snap points
            ScrollTo(scroller, 11.0, 22.0, AnimationMode.Disabled, SnapPointsMode.Default, false /*hookViewChanged*/);
            ScrollBy(scroller, -4.0, 15.0, AnimationMode.Disabled, SnapPointsMode.Default, false /*hookViewChanged*/);
            ScrollTo(scroller, 55.0, 25.0, AnimationMode.Enabled, SnapPointsMode.Default, false /*hookViewChanged*/);
            ScrollBy(scroller, 700.0, -8.0, AnimationMode.Enabled, SnapPointsMode.Default, false /*hookViewChanged*/);
        }

        //[TestMethod]
        //[TestProperty("Description", "Changes Scroller zoomFactor using ZoomTo, ZoomBy, ZoomFrom and AnimationMode enum values.")]
        // Disabled due to: Multiple unreliable Scroller tests #136
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

            // Jump to relative zoomFactor
            ZoomBy(scroller, 1.0f, 55.0f, 66.0f, AnimationMode.Disabled, SnapPointsMode.Ignore, false /*hookViewChanged*/);

            // Animate to absolute zoomFactor
            ZoomTo(scroller, 4.0f, -40.0f, -25.0f, AnimationMode.Enabled, SnapPointsMode.Ignore, false /*hookViewChanged*/);

            // Animate to relative zoomFactor
            ZoomBy(scroller, -2.0f, 100.0f, 200.0f, AnimationMode.Enabled, SnapPointsMode.Ignore, false /*hookViewChanged*/);

            // Flick with additional zoomFactor velocity
            ZoomFrom(scroller, 2.0f, null /*inertiaDecayRate*/, -50.0f, 800.0f, false /*hookViewChanged*/);

            // Flick with additional zoomFactor velocity and custom zoomFactor inertia decay rate
            ZoomFrom(scroller, -2.0f, 0.75f /*inertiaDecayRate*/, -50.0f, 800.0f, false /*hookViewChanged*/);
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
                    Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);

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
                Verify.AreEqual(scroller.ZoomFactor, 1.0f);
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Interrupted);
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
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);

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
                    Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Interrupted);
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
                int viewChangedCount = 0;

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
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                        viewChangedCount++;
                    };

                    scroller.ScrollAnimationStarting += (sender, args) =>
                    {
                        Log.Comment("ScrollAnimationStarting - OffsetsChangeId={0}", args.ScrollInfo.OffsetsChangeId);
                        Verify.IsNotNull(args.Animation);
                        Vector3KeyFrameAnimation stockKeyFrameAnimation = args.Animation as Vector3KeyFrameAnimation;
                        Verify.IsNotNull(stockKeyFrameAnimation);
                        Log.Comment("Stock duration={0} msec.", stockKeyFrameAnimation.Duration.TotalMilliseconds);
                        Verify.AreEqual(stockKeyFrameAnimation.Duration.TotalMilliseconds, c_MaxStockOffsetsChangeDuration);
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
                    Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                    Verify.AreEqual(scroller.HorizontalOffset, 600.0);
                    Verify.AreEqual(scroller.VerticalOffset, 400.0);
                    Verify.AreEqual(scroller.ZoomFactor, 1.0f);

                    Log.Comment("viewChangedCount={0}", viewChangedCount);
                    Verify.IsLessThanOrEqual(viewChangedCount, 2);

                    Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
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
                int viewChangedCount = 0;

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
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                        viewChangedCount++;
                    };

                    scroller.ZoomAnimationStarting += (sender, args) =>
                    {
                        Log.Comment("ZoomAnimationStarting - ZoomFactorChangeId={0}", args.ZoomInfo.ZoomFactorChangeId);
                        Verify.IsNotNull(args.Animation);
                        ScalarKeyFrameAnimation stockKeyFrameAnimation = args.Animation as ScalarKeyFrameAnimation;
                        Verify.IsNotNull(stockKeyFrameAnimation);
                        Log.Comment("Stock duration={0} msec.", stockKeyFrameAnimation.Duration.TotalMilliseconds);
                        Verify.AreEqual(stockKeyFrameAnimation.Duration.TotalMilliseconds, c_MaxStockZoomFactorChangeDuration);
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
                    Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                    Verify.IsLessThan(Math.Abs(scroller.HorizontalOffset - 700.0), 0.01);
                    Verify.IsLessThan(Math.Abs(scroller.VerticalOffset - 1050.0), 0.01);
                    Verify.AreEqual(scroller.ZoomFactor, 8.0f);

                    Log.Comment("viewChangedCount={0}", viewChangedCount);
                    Verify.IsLessThanOrEqual(viewChangedCount, 2);
                    Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
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
                    Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                };

                operation = StartScrollTo(
                    scroller,
                    600.0,
                    400.0,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvent);

                Log.Comment("Resetting window content to unparent Scroller");
                MUXControlsTestApp.App.TestContentRoot = null;
            });

            WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(scroller.HorizontalOffset, 0.0);
                Verify.AreEqual(scroller.VerticalOffset, 0.0);
                Verify.AreEqual(scroller.ZoomFactor, 1.0f);
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Interrupted);
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

                Verify.AreEqual(scroller.HorizontalOffset, 0.0);
                Verify.AreEqual(scroller.VerticalOffset, 0.0);
                Verify.AreEqual(scroller.ZoomFactor, 1.0f);
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Ignored);
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

                Verify.AreEqual(scroller.HorizontalOffset, 0.0);
                Verify.AreEqual(scroller.VerticalOffset, 0.0);
                Verify.AreEqual(scroller.ZoomFactor, 1.0f);
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Ignored);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Performs consecutive non-animated offsets changes.")]
        public void ConsecutiveOffsetJumps()
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
                    Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                };

                operations[0] = StartScrollTo(
                    scroller,
                    600.0,
                    400.0,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvents[0]);

                operations[1] = StartScrollTo(
                    scroller,
                    500.0,
                    300.0,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvents[1]);
            });

            WaitForEvent("Waiting for first view change completion", scrollerViewChangeOperationEvents[0]);

            WaitForEvent("Waiting for second view change completion", scrollerViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(scroller.HorizontalOffset, 500.0);
                Verify.AreEqual(scroller.VerticalOffset, 300.0);
                Verify.AreEqual(scroller.ZoomFactor, 1.0f);
                Verify.AreEqual(operations[0].Result, ScrollerViewChangeResult.Completed);
                Verify.AreEqual(operations[1].Result, ScrollerViewChangeResult.Completed);

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

                Verify.AreEqual(scroller.HorizontalOffset, 500.0);
                Verify.AreEqual(scroller.VerticalOffset, 300.0);
                Verify.AreEqual(scroller.ZoomFactor, 1.0f);
                Verify.AreEqual(operations[2].Result, ScrollerViewChangeResult.Completed);
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
                    Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                };

                operations[0] = StartZoomTo(
                    scroller,
                    8.0f,
                    150.0f,
                    120.0f,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvents[0]);

                operations[1] = StartZoomTo(
                    scroller,
                    7.0f,
                    10.0f,
                    90.0f,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvents[1]);
            });

            WaitForEvent("Waiting for first view change completion", scrollerViewChangeOperationEvents[0]);

            WaitForEvent("Waiting for second view change completion", scrollerViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(scroller.HorizontalOffset, 60.0);
                Verify.AreEqual(scroller.VerticalOffset, 540.0);
                Verify.AreEqual(scroller.ZoomFactor, 7.0f);
                Verify.AreEqual(operations[0].Result, ScrollerViewChangeResult.Completed);
                Verify.AreEqual(operations[1].Result, ScrollerViewChangeResult.Completed);

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

                Verify.AreEqual(scroller.HorizontalOffset, 60.0);
                Verify.AreEqual(scroller.VerticalOffset, 540.0);
                Verify.AreEqual(scroller.ZoomFactor, 7.0f);
                Verify.AreEqual(operations[2].Result, ScrollerViewChangeResult.Completed);
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
                    Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                };

                operation = StartScrollTo(
                    scroller,
                    600.0,
                    400.0,
                    animate ? AnimationMode.Enabled : AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollerViewChangeOperationEvent);

                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scroller;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(scroller.HorizontalOffset, 600.0);
                Verify.AreEqual(scroller.VerticalOffset, 400.0);
                Verify.AreEqual(scroller.ZoomFactor, 1.0f);
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
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
                    Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
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
                MUXControlsTestApp.App.TestContentRoot = scroller;
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.IsLessThan(Math.Abs(scroller.HorizontalOffset - 700.0), 0.01);
                Verify.IsLessThan(Math.Abs(scroller.VerticalOffset - 1050.0), 0.01);
                Verify.AreEqual(scroller.ZoomFactor, 8.0f);
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
            });
        }

        private void ScrollTo(
            Scroller scroller,
            double horizontalOffset,
            double verticalOffset,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            double? expectedFinalHorizontalOffset = null,
            double? expectedFinalVerticalOffset = null)
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
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
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
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(expectedFinalHorizontalOffset, scroller.HorizontalOffset);
                Verify.AreEqual(expectedFinalVerticalOffset, scroller.VerticalOffset);
                Verify.AreEqual(originalZoomFactor, scroller.ZoomFactor);
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
            });
        }

        private void ScrollBy(
            Scroller scroller,
            double horizontalOffsetDelta,
            double verticalOffsetDelta,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            double? expectedFinalHorizontalOffset = null,
            double? expectedFinalVerticalOffset = null)
        {
            double originalHorizontalOffset = 0.0;
            double originalVerticalOffset = 0.0;
            float originalZoomFactor = 1.0f;
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                if (hookViewChanged)
                {
                    scroller.ViewChanged += (sender, args) =>
                    {
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                    };
                }

                originalHorizontalOffset = scroller.HorizontalOffset;
                originalVerticalOffset = scroller.VerticalOffset;
                originalZoomFactor = scroller.ZoomFactor;

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
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(expectedFinalHorizontalOffset, scroller.HorizontalOffset);
                Verify.AreEqual(expectedFinalVerticalOffset, scroller.VerticalOffset);
                Verify.AreEqual(originalZoomFactor, scroller.ZoomFactor);
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
            });
        }

        private void ScrollFrom(
            Scroller scroller,
            float horizontalVelocity,
            float verticalVelocity,
            float? horizontalInertiaDecayRate,
            float? verticalInertiaDecayRate,
            bool hookViewChanged = true)
        {
            double originalHorizontalOffset = 0.0;
            double originalVerticalOffset = 0.0;
            float originalZoomFactor = 1.0f;
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                if (hookViewChanged)
                {
                    scroller.ViewChanged += (sender, args) =>
                    {
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                    };
                }

                originalHorizontalOffset = scroller.HorizontalOffset;
                originalVerticalOffset = scroller.VerticalOffset;
                originalZoomFactor = scroller.ZoomFactor;

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
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
            });
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

            ScrollerOperation operation = new ScrollerOperation();

            operation.Id = scroller.ScrollTo(
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
            bool hookViewChanged = true)
        {
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                if (hookViewChanged)
                {
                    scroller.ViewChanged += (sender, args) =>
                    {
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
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
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(zoomFactor, scroller.ZoomFactor);
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
            });
        }

        private void ZoomBy(
            Scroller scroller,
            float zoomFactorDelta,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true)
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
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                    };
                }

                originalZoomFactor = scroller.ZoomFactor;

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
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                Verify.AreEqual(zoomFactorDelta + originalZoomFactor, scroller.ZoomFactor);
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
            });
        }

        private void ZoomFrom(
            Scroller scroller,
            float zoomFactorVelocity,
            float? inertiaDecayRate,
            float centerPointX,
            float centerPointY,
            bool hookViewChanged = true)
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
                        Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                            sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                    };
                }

                originalZoomFactor = scroller.ZoomFactor;

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
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
            });
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
            Log.Comment("ChangeZoomFactor - zoomFactorVelocity={0}, inertiaDecayRate={1}, centerPoint=({2},{3})",
                zoomFactorVelocity, inertiaDecayRate, centerPointX, centerPointY);

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
            bool changeOffsetsFirst =
                viewChangeInterruptionKind == ViewChangeInterruptionKind.OffsetsChangeByOffsetsChange || viewChangeInterruptionKind == ViewChangeInterruptionKind.OffsetsChangeByZoomFactorChange;
            bool changeOffsetsSecond =
                viewChangeInterruptionKind == ViewChangeInterruptionKind.OffsetsChangeByOffsetsChange || viewChangeInterruptionKind == ViewChangeInterruptionKind.ZoomFactorChangeByOffsetsChange;
            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollerViewChangeOperationEvents = null;
            ScrollerOperation[] operations = null;
            int viewChangedCount = 0;

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
                    Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);

                    viewChangedCount++;

                    if (viewChangedCount == 3)
                    {
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
                    Verify.AreEqual(scroller.HorizontalOffset, 500.0);
                    Verify.AreEqual(scroller.VerticalOffset, 300.0);
                    Verify.AreEqual(scroller.ZoomFactor, 1.0f);
                }
                if (changeOffsetsFirst && !changeOffsetsSecond)
                {
                    Verify.IsGreaterThanOrEqual(scroller.HorizontalOffset, 600.0);
                    Verify.IsGreaterThanOrEqual(scroller.VerticalOffset, 400.0);
                    Verify.AreEqual(scroller.ZoomFactor, 7.0f);
                }
                if (!changeOffsetsFirst && changeOffsetsSecond)
                {
                    Verify.IsGreaterThanOrEqual(scroller.HorizontalOffset, 500.0);
                    Verify.IsGreaterThanOrEqual(scroller.VerticalOffset, 300.0);
                    Verify.AreEqual(scroller.ZoomFactor, 8.0f);
                }
                if (!changeOffsetsFirst && !changeOffsetsSecond)
                {
                    Verify.AreEqual(scroller.ZoomFactor, 7.0f);
                }

                Verify.AreEqual(operations[0].Result, ScrollerViewChangeResult.Interrupted);
                Verify.AreEqual(operations[1].Result, ScrollerViewChangeResult.Completed);
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
