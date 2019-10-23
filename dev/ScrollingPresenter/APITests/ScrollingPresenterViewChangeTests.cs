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

using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using AnimationMode = Microsoft.UI.Xaml.Controls.ScrollingAnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode;
using ScrollOptions = Microsoft.UI.Xaml.Controls.ScrollOptions;
using ZoomOptions = Microsoft.UI.Xaml.Controls.ZoomOptions;
using ScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollAnimationStartingEventArgs;
using ZoomAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ZoomAnimationStartingEventArgs;
using ScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollCompletedEventArgs;
using ZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ZoomCompletedEventArgs;

using ScrollingPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooks;
using ScrollingPresenterViewChangeResult = Microsoft.UI.Private.Controls.ScrollingPresenterViewChangeResult;
using Windows.UI.ViewManagement;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollingPresenterTests
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

        [TestCleanup]
        public void TestCleanup()
        {
            RunOnUIThread.Execute(() =>
            {
                MUXControlsTestApp.App.TestContentRoot = null;
            });
        }

        [TestMethod]
        [TestProperty("Description", "Changes ScrollingPresenter offsets using ScrollTo, ScrollBy, ScrollFrom and AnimationMode/SnapPointsMode enum values.")]
        public void BasicOffsetChanges()
        {
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            // Jump to absolute offsets
            ScrollTo(scrollingPresenter, 11.0, 22.0, AnimationMode.Disabled, SnapPointsMode.Ignore);
            ScrollTo(scrollingPresenter, 22.0, 11.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: false);

            // Jump to relative offsets
            ScrollBy(scrollingPresenter, -4.0, 15.0, AnimationMode.Disabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ScrollBy(scrollingPresenter, 15.0, 4.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: false);

            // Animate to absolute offsets
            ScrollTo(scrollingPresenter, 55.0, 25.0, AnimationMode.Enabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ScrollTo(scrollingPresenter, 5.0, 75.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: true);

            // Jump or animate to absolute offsets based on UISettings.AnimationsEnabled
            ScrollTo(scrollingPresenter, 55.0, 25.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false);

            // Animate to relative offsets
            ScrollBy(scrollingPresenter, 700.0, -8.0, AnimationMode.Enabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ScrollBy(scrollingPresenter, -80.0, 200.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: true);

            // Jump or animate to relative offsets based on UISettings.AnimationsEnabled
            ScrollBy(scrollingPresenter, 80.0, -200.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false);

            // Flick with additional offsets velocity
            ScrollFrom(scrollingPresenter, -65.0f, 80.0f, horizontalInertiaDecayRate: null, verticalInertiaDecayRate: null, hookViewChanged: false);

            // Flick with additional offsets velocity and custom scroll inertia decay rate
            ScrollFrom(scrollingPresenter, 65.0f, -80.0f, horizontalInertiaDecayRate: 0.7f , verticalInertiaDecayRate: 0.8f, hookViewChanged: false);

            // Do it all again while respecting snap points
            ScrollTo(scrollingPresenter, 11.0, 22.0, AnimationMode.Disabled, SnapPointsMode.Default, hookViewChanged: false);
            ScrollBy(scrollingPresenter, -4.0, 15.0, AnimationMode.Disabled, SnapPointsMode.Default, hookViewChanged: false);
            ScrollTo(scrollingPresenter, 55.0, 25.0, AnimationMode.Enabled, SnapPointsMode.Default, hookViewChanged: false);
            ScrollBy(scrollingPresenter, 700.0, -8.0, AnimationMode.Enabled, SnapPointsMode.Default, hookViewChanged: false);
        }

        [TestMethod]
        [TestProperty("Description", "Changes ScrollingPresenter zoomFactor using ZoomTo, ZoomBy, ZoomFrom and AnimationMode enum values.")]
        public void BasicZoomFactorChanges()
        {
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            // Jump to absolute zoomFactor
            ZoomTo(scrollingPresenter, 2.0f, 22.0f, 33.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);
            ZoomTo(scrollingPresenter, 5.0f, 33.0f, 22.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: false);

            // Jump to relative zoomFactor
            ZoomBy(scrollingPresenter, 1.0f, 55.0f, 66.0f, AnimationMode.Disabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ZoomBy(scrollingPresenter, 1.0f, 66.0f, 55.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: false);

            // Animate to absolute zoomFactor
            ZoomTo(scrollingPresenter, 4.0f, -40.0f, -25.0f, AnimationMode.Enabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ZoomTo(scrollingPresenter, 6.0f, 25.0f, 40.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: true);

            // Jump or animate to absolute zoomFactor based on UISettings.AnimationsEnabled
            ZoomTo(scrollingPresenter, 3.0f, 10.0f, 20.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false);

            // Animate to relative zoomFactor
            ZoomBy(scrollingPresenter, -2.0f, 100.0f, 200.0f, AnimationMode.Enabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ZoomBy(scrollingPresenter, 1.0f, 100.0f, 200.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: true);

            // Jump or animate to relative zoomFactor based on UISettings.AnimationsEnabled
            ZoomBy(scrollingPresenter, 2.0f, 200.0f, 100.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false);

            // Flick with additional zoomFactor velocity
            ZoomFrom(scrollingPresenter, 2.0f, inertiaDecayRate: null, centerPointX: -50.0f, centerPointY: 800.0f, hookViewChanged: false);

            // Flick with additional zoomFactor velocity and custom zoomFactor inertia decay rate
            ZoomFrom(scrollingPresenter, -2.0f, inertiaDecayRate: 0.75f, centerPointX: -50.0f, centerPointY: 800.0f, hookViewChanged: false);
        }

        [TestMethod]
        [TestProperty("Description", "Cancels an animated offsets change.")]
        public void BasicOffsetsChangeCancelation()
        {
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollingPresenterOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                operation = StartScrollTo(
                    scrollingPresenter,
                    600.0,
                    400.0,
                    AnimationMode.Enabled,
                    SnapPointsMode.Ignore,
                    scrollingPresenterViewChangeOperationEvent);

                bool operationCanceled = false;

                scrollingPresenter.ViewChanged += (sender, args) =>
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

            WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                Verify.IsTrue(scrollingPresenter.HorizontalOffset < 600.0);
                Verify.IsTrue(scrollingPresenter.VerticalOffset < 400.0);
                Verify.AreEqual(1.0f, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Interrupted, operation.Result);
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

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("ScrollingPresenter"))
            {
                ScrollingPresenter scrollingPresenter = null;
                Rectangle rectangleScrollingPresenterContent = null;
                AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollingPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollingPresenterContent = new Rectangle();
                    scrollingPresenter = new ScrollingPresenter();
                    scrollingPresenter.Name = "scr";

                    SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    operation = StartZoomTo(
                        scrollingPresenter,
                        8.0f,
                        100.0f,
                        150.0f,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvent);

                    bool operationCanceled = false;

                    scrollingPresenter.ViewChanged += (sender, args) =>
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

                WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                    Verify.IsTrue(scrollingPresenter.ZoomFactor < 8.0f);
                    Verify.AreEqual(ScrollingPresenterViewChangeResult.Interrupted, operation.Result);
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
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            Point newPosition1 = new Point(100, 150);
            Point newPosition2 = new Point(50, 100);
            float newZoomFactor1 = 2.0f;
            float newZoomFactor2 = 0.5f;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(
                    scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Attach to ScrollAnimationStarting");

                scrollingPresenter.ZoomAnimationStarting += (ScrollingPresenter sender, ZoomAnimationStartingEventArgs e) =>
                {
                    Log.Comment("ScrollingPresenter.ZoomAnimationStarting event handler");
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

                scrollingPresenter.ScrollAnimationStarting += (ScrollingPresenter sender, ScrollAnimationStartingEventArgs e) =>
                {
                    Log.Comment("ScrollingPresenter.ScrollAnimationStarting event handler");
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
            ScrollTo(scrollingPresenter, newPosition1.X, newPosition1.Y, AnimationMode.Enabled, SnapPointsMode.Ignore);

            Log.Comment("Animating to absolute Offset");
            ScrollTo(scrollingPresenter, newPosition2.X, newPosition2.Y, AnimationMode.Enabled, SnapPointsMode.Ignore);

            Log.Comment("Animating to absolute zoomFactor");
            ZoomTo(scrollingPresenter, newZoomFactor1, 100.0f, 200.0f, AnimationMode.Enabled, SnapPointsMode.Ignore);

            Log.Comment("Animating to absolute zoomFactor");
            ZoomTo(scrollingPresenter, newZoomFactor2, 100.0f, 200.0f, AnimationMode.Enabled, SnapPointsMode.Ignore);

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

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("ScrollingPresenter"))
            {
                ScrollingPresenter scrollingPresenter = null;
                Rectangle rectangleScrollingPresenterContent = null;
                AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollingPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollingPresenterContent = new Rectangle();
                    scrollingPresenter = new ScrollingPresenter();

                    SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    scrollingPresenter.ViewChanged += (sender, args) =>
                    {
                        Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                    };

                    scrollingPresenter.ScrollAnimationStarting += (sender, args) =>
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
                        scrollingPresenter,
                        600.0,
                        400.0,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scrollingPresenter.HorizontalOffset}, VerticalOffset={scrollingPresenter.VerticalOffset}, ZoomFactor={scrollingPresenter.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(600.0, scrollingPresenter.HorizontalOffset);
                    Verify.AreEqual(400.0, scrollingPresenter.VerticalOffset);
                    Verify.AreEqual(1.0f, scrollingPresenter.ZoomFactor);

                    Verify.IsLessThanOrEqual(viewChangedCount, 2u);

                    Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operation.Result);
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

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("ScrollingPresenter"))
            {
                ScrollingPresenter scrollingPresenter = null;
                Rectangle rectangleScrollingPresenterContent = null;
                AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollingPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollingPresenterContent = new Rectangle();
                    scrollingPresenter = new ScrollingPresenter();

                    SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    scrollingPresenter.ViewChanged += (sender, args) =>
                    {
                        Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                    };

                    scrollingPresenter.ZoomAnimationStarting += (sender, args) =>
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
                        scrollingPresenter,
                        8.0f,
                        100.0f,
                        150.0f,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scrollingPresenter.HorizontalOffset}, VerticalOffset={scrollingPresenter.VerticalOffset}, ZoomFactor={scrollingPresenter.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.IsLessThan(Math.Abs(scrollingPresenter.HorizontalOffset - 700.0), 0.01);
                    Verify.IsLessThan(Math.Abs(scrollingPresenter.VerticalOffset - 1050.0), 0.01);
                    Verify.AreEqual(8.0f, scrollingPresenter.ZoomFactor);

                    Verify.IsLessThanOrEqual(viewChangedCount, 2u);
                    Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operation.Result);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Requests a view change just before unloading scrollingPresenter.")]
        public void InterruptViewChangeWithUnloading()
        {
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollingPresenterOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent, setAsContentRoot: true);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                operation = StartScrollTo(
                    scrollingPresenter,
                    600.0,
                    400.0,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollingPresenterViewChangeOperationEvent);

                Log.Comment("Resetting window content to unparent ScrollingPresenter");
                MUXControlsTestApp.App.TestContentRoot = null;
            });

            WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                Verify.AreEqual(0.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(0.0, scrollingPresenter.VerticalOffset);
                Verify.AreEqual(1.0f, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Interrupted, operation.Result);
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
            ScrollingPresenter scrollingPresenter = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollingPresenterOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, null /*rectangleScrollingPresenterContent*/, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                operation = StartScrollTo(
                    scrollingPresenter,
                    600.0,
                    400.0,
                    AnimationMode.Enabled,
                    SnapPointsMode.Ignore,
                    scrollingPresenterViewChangeOperationEvent);
            });

            WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                Verify.AreEqual(0.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(0.0, scrollingPresenter.VerticalOffset);
                Verify.AreEqual(1.0f, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Ignored, operation.Result);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Attempts a zoomFactor change while there is no content.")]
        public void ZoomFactorChangeWithNoContent()
        {
            ScrollingPresenter scrollingPresenter = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollingPresenterOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, null /*rectangleScrollingPresenterContent*/, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                operation = StartZoomTo(
                    scrollingPresenter,
                    8.0f,
                    100.0f,
                    150.0f,
                    AnimationMode.Enabled,
                    SnapPointsMode.Ignore,
                    scrollingPresenterViewChangeOperationEvent);
            });

            WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                Verify.AreEqual(0.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(0.0, scrollingPresenter.VerticalOffset);
                Verify.AreEqual(1.0f, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Ignored, operation.Result);
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
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollingPresenterViewChangeOperationEvents = null;
            ScrollingPresenterOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenterViewChangeOperationEvents = new AutoResetEvent[3];
                scrollingPresenterViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollingPresenterViewChangeOperationEvents[1] = new AutoResetEvent(false);
                scrollingPresenterViewChangeOperationEvents[2] = new AutoResetEvent(false);

                operations = new ScrollingPresenterOperation[3];

                scrollingPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                operations[0] = StartScrollTo(
                    scrollingPresenter,
                    600.0,
                    400.0,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollingPresenterViewChangeOperationEvents[0]);

                if (!waitForFirstCompletion)
                {
                    operations[1] = StartScrollTo(
                        scrollingPresenter,
                        500.0,
                        300.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[1]);
                }
            });

            WaitForEvent("Waiting for first view change completion", scrollingPresenterViewChangeOperationEvents[0]);

            if (waitForFirstCompletion)
            {
                RunOnUIThread.Execute(() =>
                {
                    operations[1] = StartScrollTo(
                        scrollingPresenter,
                        500.0,
                        300.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[1]);
                });
            }

            WaitForEvent("Waiting for second view change completion", scrollingPresenterViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                Verify.AreEqual(500.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(300.0, scrollingPresenter.VerticalOffset);
                Verify.AreEqual(1.0f, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operations[0].Result);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operations[1].Result);

                // Jump to the same offsets.
                operations[2] = StartScrollTo(
                    scrollingPresenter,
                    500.0,
                    300.0,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollingPresenterViewChangeOperationEvents[2]);
            });

            WaitForEvent("Waiting for third view change completion", scrollingPresenterViewChangeOperationEvents[2]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                Verify.AreEqual(500.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(300.0, scrollingPresenter.VerticalOffset);
                Verify.AreEqual(1.0f, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operations[2].Result);
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
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollingPresenterViewChangeOperationEvents = null;
            ScrollingPresenterOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenterViewChangeOperationEvents = new AutoResetEvent[3];
                scrollingPresenterViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollingPresenterViewChangeOperationEvents[1] = new AutoResetEvent(false);
                scrollingPresenterViewChangeOperationEvents[2] = new AutoResetEvent(false);

                operations = new ScrollingPresenterOperation[3];

                scrollingPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                if (isFirstZoomRelative)
                {
                    operations[0] = StartZoomBy(
                        scrollingPresenter,
                        7.0f,
                        150.0f,
                        120.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartZoomTo(
                        scrollingPresenter,
                        8.0f,
                        150.0f,
                        120.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[0]);
                }
            });

            if (waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollingPresenterViewChangeOperationEvents[0]);
            }

            RunOnUIThread.Execute(() =>
            {
                if (isFirstZoomRelative)
                {
                    operations[1] = StartZoomBy(
                        scrollingPresenter,
                        -1.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[1]);
                }
                else
                {
                    operations[1] = StartZoomTo(
                        scrollingPresenter,
                        7.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[1]);
                }
            });

            if (!waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollingPresenterViewChangeOperationEvents[0]);
            }
            WaitForEvent("Waiting for second view change completion", scrollingPresenterViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                Verify.AreEqual(917.5, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(723.75, scrollingPresenter.VerticalOffset);
                Verify.AreEqual(7.0f, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operations[0].Result);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operations[1].Result);

                // Jump to the same zoomFactor
                operations[2] = StartZoomTo(
                    scrollingPresenter,
                    7.0f,
                    10.0f,
                    90.0f,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollingPresenterViewChangeOperationEvents[2]);
            });

            WaitForEvent("Waiting for third view change completion", scrollingPresenterViewChangeOperationEvents[2]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                Verify.AreEqual(917.5, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(723.75, scrollingPresenter.VerticalOffset);
                Verify.AreEqual(7.0f, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operations[2].Result);
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
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollingPresenterViewChangeOperationEvents = null;
            ScrollingPresenterOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenterViewChangeOperationEvents = new AutoResetEvent[2];
                scrollingPresenterViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollingPresenterViewChangeOperationEvents[1] = new AutoResetEvent(false);

                operations = new ScrollingPresenterOperation[2];

                scrollingPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                if (isScrollRelative)
                {
                    operations[0] = StartScrollBy(
                        scrollingPresenter,
                        80.0,
                        35.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartScrollTo(
                        scrollingPresenter,
                        80.0,
                        35.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[0]);
                }
            });

            if (waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollingPresenterViewChangeOperationEvents[0]);
            }

            RunOnUIThread.Execute(() =>
            {
                if (isZoomRelative)
                {
                    operations[1] = StartZoomBy(
                        scrollingPresenter,
                        2.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[1]);
                }
                else
                {
                    operations[1] = StartZoomTo(
                        scrollingPresenter,
                        3.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[1]);
                }
            });

            if (!waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollingPresenterViewChangeOperationEvents[0]);
            }
            WaitForEvent("Waiting for second view change completion", scrollingPresenterViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                Verify.AreEqual(260.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(285.0, scrollingPresenter.VerticalOffset);
                Verify.AreEqual(3.0f, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operations[0].Result);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operations[1].Result);
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
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollingPresenterViewChangeOperationEvents = null;
            ScrollingPresenterOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenterViewChangeOperationEvents = new AutoResetEvent[2];
                scrollingPresenterViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollingPresenterViewChangeOperationEvents[1] = new AutoResetEvent(false);

                operations = new ScrollingPresenterOperation[2];

                scrollingPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                if (isZoomRelative)
                {
                    operations[0] = StartZoomBy(
                        scrollingPresenter,
                        2.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartZoomTo(
                        scrollingPresenter,
                        3.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[0]);
                }
            });

            if (waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollingPresenterViewChangeOperationEvents[0]);
            }

            RunOnUIThread.Execute(() =>
            {
                if (isScrollRelative)
                {
                    operations[1] = StartScrollBy(
                        scrollingPresenter,
                        80.0,
                        35.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[1]);
                }
                else
                {
                    operations[1] = StartScrollTo(
                        scrollingPresenter,
                        80.0,
                        35.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[1]);
                }
            });

            if (!waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollingPresenterViewChangeOperationEvents[0]);
            }
            WaitForEvent("Waiting for second view change completion", scrollingPresenterViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                if (isScrollRelative)
                {
                    Verify.AreEqual(100.0, scrollingPresenter.HorizontalOffset);
                    Verify.AreEqual(215.0, scrollingPresenter.VerticalOffset);
                }
                else
                {
                    Verify.AreEqual(80.0, scrollingPresenter.HorizontalOffset);
                    Verify.AreEqual(35.0, scrollingPresenter.VerticalOffset);
                }
                Verify.AreEqual(3.0f, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operations[0].Result);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operations[1].Result);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Requests a non-animated offsets change before loading scrollingPresenter.")]
        public void SetOffsetsBeforeLoading()
        {
            ChangeOffsetsBeforeLoading(false /*animate*/);
        }

        [TestMethod]
        [TestProperty("Description", "Requests an animated offsets change before loading scrollingPresenter.")]
        public void AnimateOffsetsBeforeLoading()
        {
            ChangeOffsetsBeforeLoading(true /*animate*/);
        }

        [TestMethod]
        [TestProperty("Description", "Requests a non-animated zoomFactor change before loading scrollingPresenter.")]
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
        [TestProperty("Description", "Requests an animated zoomFactor change before loading scrollingPresenter.")]
        public void AnimateZoomFactorBeforeLoading()
        {
            ChangeZoomFactorBeforeLoading(true /*animate*/);
        }

        [TestMethod]
        [TestProperty("Description", "Requests a non-animated offset change immediately after increasing content size.")]
        public void OffsetJumpAfterContentResizing()
        {
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent.Width = c_defaultUIScrollingPresenterContentWidth + 200.0;
            });

            // Jump to absolute offsets
            ScrollTo(
                scrollingPresenter,
                c_defaultUIScrollingPresenterContentWidth + 200.0 - c_defaultUIScrollingPresenterWidth,
                c_defaultVerticalOffset,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore);
        }

        private void ChangeOffsetsBeforeLoading(bool animate)
        {
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollingPresenterOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent, false /*setAsContentRoot*/);

                scrollingPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                operation = StartScrollTo(
                    scrollingPresenter,
                    600.0,
                    400.0,
                    animate ? AnimationMode.Enabled : AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollingPresenterViewChangeOperationEvent);

                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scrollingPresenter;
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                Verify.AreEqual(600.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(400.0, scrollingPresenter.VerticalOffset);
                Verify.AreEqual(1.0f, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operation.Result);
            });
        }

        private void ChangeZoomFactorBeforeLoading(bool animate)
        {
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollingPresenterOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent, false /*setAsContentRoot*/);

                scrollingPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                operation = StartZoomTo(
                    scrollingPresenter,
                    8.0f,
                    100.0f,
                    150.0f,
                    animate ? AnimationMode.Enabled : AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollingPresenterViewChangeOperationEvent);

                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scrollingPresenter;
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                Verify.IsLessThan(Math.Abs(scrollingPresenter.HorizontalOffset - 700.0), 0.01);
                Verify.IsLessThan(Math.Abs(scrollingPresenter.VerticalOffset - 1050.0), 0.01);
                Verify.AreEqual(8.0f, scrollingPresenter.ZoomFactor);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operation.Result);
            });
        }

        private void ScrollTo(
            ScrollingPresenter scrollingPresenter,
            double horizontalOffset,
            double verticalOffset,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            bool? isAnimationsEnabledOverride = null,
            double? expectedFinalHorizontalOffset = null,
            double? expectedFinalVerticalOffset = null)
        {
            using (ScrollingPresenterTestHooksHelper scrollingPresenterTestHooksHelper = new ScrollingPresenterTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true,
                isAnimationsEnabledOverride: isAnimationsEnabledOverride))
            {
                Log.Comment("Waiting for any pending ExpressionAnimation start/stop notifications to occur");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollingPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    scrollingPresenterTestHooksHelper.ResetExpressionAnimationStatusChanges(scrollingPresenter);

                    if (hookViewChanged)
                    {
                        scrollingPresenter.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalZoomFactor = scrollingPresenter.ZoomFactor;

                    if (expectedFinalHorizontalOffset == null)
                    {
                        expectedFinalHorizontalOffset = horizontalOffset;
                    }

                    if (expectedFinalVerticalOffset == null)
                    {
                        expectedFinalVerticalOffset = verticalOffset;
                    }

                    operation = StartScrollTo(
                        scrollingPresenter,
                        horizontalOffset,
                        verticalOffset,
                        animationMode,
                        snapPointsMode,
                        scrollingPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scrollingPresenter.HorizontalOffset}, VerticalOffset={scrollingPresenter.VerticalOffset}, ZoomFactor={scrollingPresenter.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(expectedFinalHorizontalOffset, scrollingPresenter.HorizontalOffset);
                    Verify.AreEqual(expectedFinalVerticalOffset, scrollingPresenter.VerticalOffset);
                    Verify.AreEqual(originalZoomFactor, scrollingPresenter.ZoomFactor);
                    Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operation.Result);

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
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollingPresenterTestHooksHelper.GetExpressionAnimationStatusChanges(scrollingPresenter);
                    ScrollingPresenterTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    Verify.IsNull(expressionAnimationStatusChanges);
                });
            }
        }

        private void ScrollBy(
            ScrollingPresenter scrollingPresenter,
            double horizontalOffsetDelta,
            double verticalOffsetDelta,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            bool? isAnimationsEnabledOverride = null,
            double? expectedFinalHorizontalOffset = null,
            double? expectedFinalVerticalOffset = null)
        {
            using (ScrollingPresenterTestHooksHelper scrollingPresenterTestHooksHelper = new ScrollingPresenterTestHooksHelper(
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
                AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollingPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    scrollingPresenterTestHooksHelper.ResetExpressionAnimationStatusChanges(scrollingPresenter);

                    if (hookViewChanged)
                    {
                        scrollingPresenter.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalHorizontalOffset = scrollingPresenter.HorizontalOffset;
                    originalVerticalOffset = scrollingPresenter.VerticalOffset;
                    originalZoomFactor = scrollingPresenter.ZoomFactor;

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
                        scrollingPresenter,
                        horizontalOffsetDelta,
                        verticalOffsetDelta,
                        animationMode,
                        snapPointsMode,
                        scrollingPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scrollingPresenter.HorizontalOffset}, VerticalOffset={scrollingPresenter.VerticalOffset}, ZoomFactor={scrollingPresenter.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(expectedFinalHorizontalOffset, scrollingPresenter.HorizontalOffset);
                    Verify.AreEqual(expectedFinalVerticalOffset, scrollingPresenter.VerticalOffset);
                    Verify.AreEqual(originalZoomFactor, scrollingPresenter.ZoomFactor);
                    Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operation.Result);

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
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollingPresenterTestHooksHelper.GetExpressionAnimationStatusChanges(scrollingPresenter);
                    ScrollingPresenterTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    Verify.IsNull(expressionAnimationStatusChanges);
                });
            }
        }

        private void ScrollFrom(
            ScrollingPresenter scrollingPresenter,
            float horizontalVelocity,
            float verticalVelocity,
            float? horizontalInertiaDecayRate,
            float? verticalInertiaDecayRate,
            bool hookViewChanged = true)
        {
            using (ScrollingPresenterTestHooksHelper scrollingPresenterTestHooksHelper = new ScrollingPresenterTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true))
            {
                Log.Comment("Waiting for any pending ExpressionAnimation start/stop notifications to occur");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                double originalHorizontalOffset = 0.0;
                double originalVerticalOffset = 0.0;
                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollingPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    scrollingPresenterTestHooksHelper.ResetExpressionAnimationStatusChanges(scrollingPresenter);

                    if (hookViewChanged)
                    {
                        scrollingPresenter.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalHorizontalOffset = scrollingPresenter.HorizontalOffset;
                    originalVerticalOffset = scrollingPresenter.VerticalOffset;
                    originalZoomFactor = scrollingPresenter.ZoomFactor;

                    Log.Comment($"Original HorizontalOffset={originalHorizontalOffset}, VerticalOffset={originalVerticalOffset}, ZoomFactor={originalZoomFactor}");

                    operation = StartScrollFrom(
                        scrollingPresenter,
                        horizontalVelocity,
                        verticalVelocity,
                        horizontalInertiaDecayRate,
                        verticalInertiaDecayRate,
                        scrollingPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                    if (horizontalVelocity > 0)
                        Verify.IsTrue(originalHorizontalOffset < scrollingPresenter.HorizontalOffset);
                    else if (horizontalVelocity < 0)
                        Verify.IsTrue(originalHorizontalOffset > scrollingPresenter.HorizontalOffset);
                    else
                        Verify.IsTrue(originalHorizontalOffset == scrollingPresenter.HorizontalOffset);
                    if (verticalVelocity > 0)
                        Verify.IsTrue(originalVerticalOffset < scrollingPresenter.VerticalOffset);
                    else if (verticalVelocity < 0)
                        Verify.IsTrue(originalVerticalOffset > scrollingPresenter.VerticalOffset);
                    else
                        Verify.IsTrue(originalVerticalOffset == scrollingPresenter.VerticalOffset);
                    Verify.AreEqual(originalZoomFactor, scrollingPresenter.ZoomFactor);
                    Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operation.Result);
                });

                Log.Comment("Waiting for any ExpressionAnimation start/stop notification");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                RunOnUIThread.Execute(() =>
                {
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollingPresenterTestHooksHelper.GetExpressionAnimationStatusChanges(scrollingPresenter);
                    ScrollingPresenterTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    Verify.IsNull(expressionAnimationStatusChanges);
                });
            }
        }

        private ScrollingPresenterOperation StartScrollTo(
            ScrollingPresenter scrollingPresenter,
            double horizontalOffset,
            double verticalOffset,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            AutoResetEvent scrollingPresenterViewChangeOperationEvent)
        {
            Log.Comment("ScrollTo - horizontalOffset={0}, verticalOffset={1}, animationMode={2}, snapPointsMode={3}",
                horizontalOffset, verticalOffset, animationMode, snapPointsMode);

            viewChangedCount = 0u;
            ScrollingPresenterOperation operation = new ScrollingPresenterOperation();

            operation.Id = scrollingPresenter.ScrollTo(
                horizontalOffset,
                verticalOffset,
                new ScrollOptions(animationMode, snapPointsMode)).OffsetsChangeId;

            if (operation.Id == -1)
            {
                scrollingPresenterViewChangeOperationEvent.Set();
            }
            else
            {
                scrollingPresenter.ScrollCompleted += (ScrollingPresenter sender, ScrollCompletedEventArgs args) =>
                {
                    if (args.ScrollInfo.OffsetsChangeId == operation.Id)
                    {
                        ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetScrollCompletedResult(args);

                        Log.Comment("ScrollCompleted: ScrollTo OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollingPresenterViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollingPresenterOperation StartScrollBy(
            ScrollingPresenter scrollingPresenter,
            double horizontalOffsetDelta,
            double verticalOffsetDelta,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            AutoResetEvent scrollingPresenterViewChangeOperationEvent)
        {
            Log.Comment("ScrollBy - horizontalOffsetDelta={0}, verticalOffsetDelta={1}, animationMode={2}, snapPointsMode={3}",
                horizontalOffsetDelta, verticalOffsetDelta, animationMode, snapPointsMode);

            viewChangedCount = 0u;
            ScrollingPresenterOperation operation = new ScrollingPresenterOperation();

            operation.Id = scrollingPresenter.ScrollBy(
                horizontalOffsetDelta,
                verticalOffsetDelta,
                new ScrollOptions(animationMode, snapPointsMode)).OffsetsChangeId;

            if (operation.Id == -1)
            {
                scrollingPresenterViewChangeOperationEvent.Set();
            }
            else
            {
                scrollingPresenter.ScrollCompleted += (ScrollingPresenter sender, ScrollCompletedEventArgs args) =>
                {
                    if (args.ScrollInfo.OffsetsChangeId == operation.Id)
                    {
                        ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetScrollCompletedResult(args);

                        Log.Comment("ScrollCompleted: ScrollBy OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollingPresenterViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollingPresenterOperation StartScrollFrom(
            ScrollingPresenter scrollingPresenter,
            float horizontalVelocity,
            float verticalVelocity,
            float? horizontalInertiaDecayRate,
            float? verticalInertiaDecayRate,
            AutoResetEvent scrollingPresenterViewChangeOperationEvent)
        {
            Log.Comment("ScrollFrom - horizontalVelocity={0}, verticalVelocity={1}, horizontalInertiaDecayRate={2}, verticalInertiaDecayRate={3}",
                horizontalVelocity, verticalVelocity, horizontalInertiaDecayRate, verticalInertiaDecayRate);

            Vector2? inertiaDecayRate = null;

            if (horizontalInertiaDecayRate != null && verticalInertiaDecayRate != null)
            {
                inertiaDecayRate = new Vector2((float)horizontalInertiaDecayRate, (float)verticalInertiaDecayRate);
            }

            viewChangedCount = 0u;
            ScrollingPresenterOperation operation = new ScrollingPresenterOperation();

            operation.Id = scrollingPresenter.ScrollFrom(
                    new Vector2(horizontalVelocity, verticalVelocity),
                    inertiaDecayRate).OffsetsChangeId;

            if (operation.Id == -1)
            {
                scrollingPresenterViewChangeOperationEvent.Set();
            }
            else
            {
                scrollingPresenter.ScrollCompleted += (ScrollingPresenter sender, ScrollCompletedEventArgs args) =>
                {
                    if (args.ScrollInfo.OffsetsChangeId == operation.Id)
                    {
                        ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetScrollCompletedResult(args);

                        Log.Comment("ScrollCompleted: ScrollFrom OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollingPresenterViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private void ZoomTo(
            ScrollingPresenter scrollingPresenter,
            float zoomFactor,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            bool? isAnimationsEnabledOverride = null)
        {
            using (ScrollingPresenterTestHooksHelper scrollingPresenterTestHooksHelper = new ScrollingPresenterTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true, 
                isAnimationsEnabledOverride: isAnimationsEnabledOverride))
            {
                AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollingPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    if (hookViewChanged)
                    {
                        scrollingPresenter.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    operation = StartZoomTo(
                        scrollingPresenter,
                        zoomFactor,
                        centerPointX,
                        centerPointY,
                        animationMode,
                        snapPointsMode,
                        scrollingPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scrollingPresenter.HorizontalOffset}, VerticalOffset={scrollingPresenter.VerticalOffset}, ZoomFactor={scrollingPresenter.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(zoomFactor, scrollingPresenter.ZoomFactor);
                    Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operation.Result);

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
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollingPresenterTestHooksHelper.GetExpressionAnimationStatusChanges(scrollingPresenter);
                    ScrollingPresenterTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    VerifyExpressionAnimationStatusChangesForTranslationAndZoomFactorSuspension(expressionAnimationStatusChanges);
                });
            }
        }

        private void ZoomBy(
            ScrollingPresenter scrollingPresenter,
            float zoomFactorDelta,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            bool? isAnimationsEnabledOverride = null)
        {
            using (ScrollingPresenterTestHooksHelper scrollingPresenterTestHooksHelper = new ScrollingPresenterTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true,
                isAnimationsEnabledOverride: isAnimationsEnabledOverride))
            {
                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollingPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    if (hookViewChanged)
                    {
                        scrollingPresenter.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalZoomFactor = scrollingPresenter.ZoomFactor;

                    Log.Comment($"Original HorizontalOffset={scrollingPresenter.HorizontalOffset}, VerticalOffset={scrollingPresenter.VerticalOffset}, ZoomFactor={originalZoomFactor}");

                    operation = StartZoomBy(
                        scrollingPresenter,
                        zoomFactorDelta,
                        centerPointX,
                        centerPointY,
                        animationMode,
                        snapPointsMode,
                        scrollingPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scrollingPresenter.HorizontalOffset}, VerticalOffset={scrollingPresenter.VerticalOffset}, ZoomFactor={scrollingPresenter.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(zoomFactorDelta + originalZoomFactor, scrollingPresenter.ZoomFactor);
                    Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operation.Result);

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
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollingPresenterTestHooksHelper.GetExpressionAnimationStatusChanges(scrollingPresenter);
                    ScrollingPresenterTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    VerifyExpressionAnimationStatusChangesForTranslationAndZoomFactorSuspension(expressionAnimationStatusChanges);                    
                });
            }
        }

        private void ZoomFrom(
            ScrollingPresenter scrollingPresenter,
            float zoomFactorVelocity,
            float? inertiaDecayRate,
            float centerPointX,
            float centerPointY,
            bool hookViewChanged = true)
        {
            using (ScrollingPresenterTestHooksHelper scrollingPresenterTestHooksHelper = new ScrollingPresenterTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true))
            {
                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollingPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollingPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    if (hookViewChanged)
                    {
                        scrollingPresenter.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalZoomFactor = scrollingPresenter.ZoomFactor;

                    Log.Comment($"Original HorizontalOffset={scrollingPresenter.HorizontalOffset}, VerticalOffset={scrollingPresenter.VerticalOffset}, ZoomFactor={originalZoomFactor}");

                    operation = StartZoomFrom(
                        scrollingPresenter,
                        zoomFactorVelocity,
                        inertiaDecayRate,
                        centerPointX,
                        centerPointY,
                        scrollingPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollingPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                    if (zoomFactorVelocity > 0)
                        Verify.IsTrue(originalZoomFactor < scrollingPresenter.ZoomFactor);
                    else if (zoomFactorVelocity < 0)
                        Verify.IsTrue(originalZoomFactor > scrollingPresenter.ZoomFactor);
                    else
                        Verify.IsTrue(originalZoomFactor == scrollingPresenter.ZoomFactor);
                    Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operation.Result);
                });

                Log.Comment("Waiting for any ExpressionAnimation start/stop notification");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                RunOnUIThread.Execute(() =>
                {
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollingPresenterTestHooksHelper.GetExpressionAnimationStatusChanges(scrollingPresenter);
                    ScrollingPresenterTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    VerifyExpressionAnimationStatusChangesForTranslationAndZoomFactorSuspension(expressionAnimationStatusChanges);
                });
            }
        }

        private ScrollingPresenterOperation StartZoomTo(
            ScrollingPresenter scrollingPresenter,
            float zoomFactor,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            AutoResetEvent scrollingPresenterViewChangeOperationEvent)
        {
            Log.Comment("ZoomTo - zoomFactor={0}, centerPoint=({1},{2}), animationMode={3}, snapPointsMode={4}",
                zoomFactor, centerPointX, centerPointY, animationMode, snapPointsMode);

            viewChangedCount = 0u;
            ScrollingPresenterOperation operation = new ScrollingPresenterOperation();

            operation.Id = scrollingPresenter.ZoomTo(
                zoomFactor,
                new Vector2(centerPointX, centerPointY), 
                new ZoomOptions(animationMode, snapPointsMode)).ZoomFactorChangeId;

            if (operation.Id == -1)
            {
                scrollingPresenterViewChangeOperationEvent.Set();
            }
            else
            {
                scrollingPresenter.ZoomCompleted += (ScrollingPresenter sender, ZoomCompletedEventArgs args) =>
                {
                    if (args.ZoomInfo.ZoomFactorChangeId == operation.Id)
                    {
                        ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetZoomCompletedResult(args);

                        Log.Comment("ZoomCompleted: ZoomTo ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollingPresenterViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollingPresenterOperation StartZoomBy(
            ScrollingPresenter scrollingPresenter,
            float zoomFactorDelta,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            AutoResetEvent scrollingPresenterViewChangeOperationEvent)
        {
            Log.Comment("ZoomBy - zoomFactorDelta={0}, centerPoint=({1},{2}), animationMode={3}, snapPointsMode={4}",
                zoomFactorDelta, centerPointX, centerPointY, animationMode, snapPointsMode);

            viewChangedCount = 0u;
            ScrollingPresenterOperation operation = new ScrollingPresenterOperation();

            operation.Id = scrollingPresenter.ZoomBy(
                zoomFactorDelta,
                new Vector2(centerPointX, centerPointY),
                new ZoomOptions(animationMode, snapPointsMode)).ZoomFactorChangeId;

            if (operation.Id == -1)
            {
                scrollingPresenterViewChangeOperationEvent.Set();
            }
            else
            {
                scrollingPresenter.ZoomCompleted += (ScrollingPresenter sender, ZoomCompletedEventArgs args) =>
                {
                    if (args.ZoomInfo.ZoomFactorChangeId == operation.Id)
                    {
                        ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetZoomCompletedResult(args);

                        Log.Comment("ZoomCompleted: ZoomBy ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollingPresenterViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollingPresenterOperation StartZoomFrom(
            ScrollingPresenter scrollingPresenter,
            float zoomFactorVelocity,
            float? inertiaDecayRate,
            float centerPointX,
            float centerPointY,
            AutoResetEvent scrollingPresenterViewChangeOperationEvent)
        {
            Log.Comment("ZoomFrom - zoomFactorVelocity={0}, inertiaDecayRate={1}, centerPoint=({2},{3})",
                zoomFactorVelocity, inertiaDecayRate, centerPointX, centerPointY);

            viewChangedCount = 0u;
            ScrollingPresenterOperation operation = new ScrollingPresenterOperation();

            operation.Id = scrollingPresenter.ZoomFrom(
                zoomFactorVelocity, new Vector2(centerPointX, centerPointY), inertiaDecayRate).ZoomFactorChangeId;

            if (operation.Id == -1)
            {
                scrollingPresenterViewChangeOperationEvent.Set();
            }
            else
            {
                scrollingPresenter.ZoomCompleted += (ScrollingPresenter sender, ZoomCompletedEventArgs args) =>
                {
                    if (args.ZoomInfo.ZoomFactorChangeId == operation.Id)
                    {
                        ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetZoomCompletedResult(args);

                        Log.Comment("ZoomCompleted: ZoomFrom ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollingPresenterViewChangeOperationEvent.Set();
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
            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            AutoResetEvent scrollingPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollingPresenterViewChangeOperationEvents = null;
            ScrollingPresenterOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();

                SetupDefaultUI(scrollingPresenter, rectangleScrollingPresenterContent, scrollingPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollingPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenterViewChangeOperationEvents = new AutoResetEvent[2];
                scrollingPresenterViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollingPresenterViewChangeOperationEvents[1] = new AutoResetEvent(false);

                operations = new ScrollingPresenterOperation[2];
                operations[0] = null;
                operations[1] = null;

                scrollingPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");

                    if (viewChangedCount == 3u && !viewChangeInterruptionDone)
                    {
                        viewChangeInterruptionDone = true;
                        if (changeOffsetsSecond)
                        {
                            operations[1] = StartScrollTo(
                                scrollingPresenter,
                                500.0,
                                300.0,
                                AnimationMode.Enabled,
                                SnapPointsMode.Ignore,
                                scrollingPresenterViewChangeOperationEvents[1]);
                        }
                        else
                        {
                            operations[1] = StartZoomTo(
                                scrollingPresenter,
                                7.0f,
                                70.0f,
                                50.0f,
                                AnimationMode.Enabled,
                                SnapPointsMode.Ignore,
                                scrollingPresenterViewChangeOperationEvents[1]);
                        }
                    }
                };

                if (changeOffsetsFirst)
                {
                    operations[0] = StartScrollTo(
                        scrollingPresenter,
                        600.0,
                        400.0,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartZoomTo(
                        scrollingPresenter,
                        8.0f,
                        100.0f,
                        150.0f,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollingPresenterViewChangeOperationEvents[0]);
                }
            });

            WaitForEvent("Waiting for first view change completion", scrollingPresenterViewChangeOperationEvents[0]);

            WaitForEvent("Waiting for second view change completion", scrollingPresenterViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollingPresenter.HorizontalOffset, scrollingPresenter.VerticalOffset, scrollingPresenter.ZoomFactor);

                if (changeOffsetsFirst && changeOffsetsSecond)
                {
                    Verify.AreEqual(500.0, scrollingPresenter.HorizontalOffset);
                    Verify.AreEqual(300.0, scrollingPresenter.VerticalOffset);
                    Verify.AreEqual(1.0f, scrollingPresenter.ZoomFactor);
                }
                if (changeOffsetsFirst && !changeOffsetsSecond)
                {
                    Verify.IsGreaterThanOrEqual(scrollingPresenter.HorizontalOffset, 600.0);
                    Verify.IsGreaterThanOrEqual(scrollingPresenter.VerticalOffset, 400.0);
                    Verify.AreEqual(7.0f, scrollingPresenter.ZoomFactor);
                }
                if (!changeOffsetsFirst && changeOffsetsSecond)
                {
                    Verify.IsGreaterThanOrEqual(scrollingPresenter.HorizontalOffset, 500.0);
                    Verify.IsGreaterThanOrEqual(scrollingPresenter.VerticalOffset, 300.0);
                    Verify.AreEqual(8.0f, scrollingPresenter.ZoomFactor);
                }
                if (!changeOffsetsFirst && !changeOffsetsSecond)
                {
                    Verify.AreEqual(7.0f, scrollingPresenter.ZoomFactor);
                }

                Verify.AreEqual(ScrollingPresenterViewChangeResult.Interrupted, operations[0].Result);
                Verify.AreEqual(ScrollingPresenterViewChangeResult.Completed, operations[1].Result);
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

        private class ScrollingPresenterOperation
        {
            public ScrollingPresenterOperation()
            {
                Result = ScrollingPresenterViewChangeResult.Ignored;
                Id = -1;
            }

            public ScrollingPresenterViewChangeResult Result
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
