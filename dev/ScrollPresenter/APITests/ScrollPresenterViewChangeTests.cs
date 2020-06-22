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

using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using ScrollingScrollOptions = Microsoft.UI.Xaml.Controls.ScrollingScrollOptions;
using ScrollingZoomOptions = Microsoft.UI.Xaml.Controls.ScrollingZoomOptions;
using ScrollingScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollAnimationStartingEventArgs;
using ScrollingZoomAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollingZoomAnimationStartingEventArgs;
using ScrollingScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollCompletedEventArgs;
using ScrollingZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingZoomCompletedEventArgs;

using ScrollPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollPresenterTestHooks;
using ScrollPresenterViewChangeResult = Microsoft.UI.Private.Controls.ScrollPresenterViewChangeResult;
using Windows.UI.ViewManagement;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollPresenterTests : ApiTestBase
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
        [TestProperty("Description", "Changes ScrollPresenter offsets using ScrollTo, ScrollBy, ScrollFrom and AnimationMode/SnapPointsMode enum values.")]
        public void BasicOffsetChanges()
        {
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            // Jump to absolute offsets
            ScrollTo(scrollPresenter, 11.0, 22.0, AnimationMode.Disabled, SnapPointsMode.Ignore);
            ScrollTo(scrollPresenter, 22.0, 11.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: false);

            // Jump to relative offsets
            ScrollBy(scrollPresenter, -4.0, 15.0, AnimationMode.Disabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ScrollBy(scrollPresenter, 15.0, 4.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: false);

            // Animate to absolute offsets
            ScrollTo(scrollPresenter, 55.0, 25.0, AnimationMode.Enabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ScrollTo(scrollPresenter, 5.0, 75.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: true);

            // Jump or animate to absolute offsets based on UISettings.AnimationsEnabled
            ScrollTo(scrollPresenter, 55.0, 25.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false);

            // Animate to relative offsets
            ScrollBy(scrollPresenter, 700.0, -8.0, AnimationMode.Enabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ScrollBy(scrollPresenter, -80.0, 200.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: true);

            // Jump or animate to relative offsets based on UISettings.AnimationsEnabled
            ScrollBy(scrollPresenter, 80.0, -200.0, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false);

            // Flick with additional offsets velocity
            ScrollFrom(scrollPresenter, -65.0f, 80.0f, horizontalInertiaDecayRate: null, verticalInertiaDecayRate: null, hookViewChanged: false);

            // Flick with additional offsets velocity and custom scroll inertia decay rate
            ScrollFrom(scrollPresenter, 65.0f, -80.0f, horizontalInertiaDecayRate: 0.7f , verticalInertiaDecayRate: 0.8f, hookViewChanged: false);

            // Do it all again while respecting snap points
            ScrollTo(scrollPresenter, 11.0, 22.0, AnimationMode.Disabled, SnapPointsMode.Default, hookViewChanged: false);
            ScrollBy(scrollPresenter, -4.0, 15.0, AnimationMode.Disabled, SnapPointsMode.Default, hookViewChanged: false);
            ScrollTo(scrollPresenter, 55.0, 25.0, AnimationMode.Enabled, SnapPointsMode.Default, hookViewChanged: false);
            ScrollBy(scrollPresenter, 700.0, -8.0, AnimationMode.Enabled, SnapPointsMode.Default, hookViewChanged: false);
        }

        [TestMethod]
        [TestProperty("Description", "Changes ScrollPresenter zoomFactor using ZoomTo, ZoomBy, ZoomFrom and AnimationMode enum values.")]
        public void BasicZoomFactorChanges()
        {
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            // Jump to absolute zoomFactor
            ZoomTo(scrollPresenter, 2.0f, 22.0f, 33.0f, AnimationMode.Disabled, SnapPointsMode.Ignore);
            ZoomTo(scrollPresenter, 5.0f, 33.0f, 22.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: false);

            // Jump to relative zoomFactor
            ZoomBy(scrollPresenter, 1.0f, 55.0f, 66.0f, AnimationMode.Disabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ZoomBy(scrollPresenter, 1.0f, 66.0f, 55.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: false);

            // Animate to absolute zoomFactor
            ZoomTo(scrollPresenter, 4.0f, -40.0f, -25.0f, AnimationMode.Enabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ZoomTo(scrollPresenter, 6.0f, 25.0f, 40.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: true);

            // Jump or animate to absolute zoomFactor based on UISettings.AnimationsEnabled
            ZoomTo(scrollPresenter, 3.0f, 10.0f, 20.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false);

            // Animate to relative zoomFactor
            ZoomBy(scrollPresenter, -2.0f, 100.0f, 200.0f, AnimationMode.Enabled, SnapPointsMode.Ignore, hookViewChanged: false);
            ZoomBy(scrollPresenter, 1.0f, 100.0f, 200.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false, isAnimationsEnabledOverride: true);

            // Jump or animate to relative zoomFactor based on UISettings.AnimationsEnabled
            ZoomBy(scrollPresenter, 2.0f, 200.0f, 100.0f, AnimationMode.Auto, SnapPointsMode.Ignore, hookViewChanged: false);

            // Flick with additional zoomFactor velocity
            ZoomFrom(scrollPresenter, 2.0f, inertiaDecayRate: null, centerPointX: -50.0f, centerPointY: 800.0f, hookViewChanged: false);

            // Flick with additional zoomFactor velocity and custom zoomFactor inertia decay rate
            ZoomFrom(scrollPresenter, -2.0f, inertiaDecayRate: 0.75f, centerPointX: -50.0f, centerPointY: 800.0f, hookViewChanged: false);
        }

        [TestMethod]
        [TestProperty("Description", "Cancels an animated offsets change.")]
        public void BasicOffsetsChangeCancelation()
        {
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollPresenterOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                operation = StartScrollTo(
                    scrollPresenter,
                    600.0,
                    400.0,
                    AnimationMode.Enabled,
                    SnapPointsMode.Ignore,
                    scrollPresenterViewChangeOperationEvent);

                bool operationCanceled = false;

                scrollPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");

                    if ((sender.HorizontalOffset >= 150.0 || sender.VerticalOffset >= 100.0) && !operationCanceled)
                    {
                        Log.Comment("Canceling view change");
                        operationCanceled = true;
                        sender.ScrollBy(0, 0, new ScrollingScrollOptions(AnimationMode.Disabled, SnapPointsMode.Ignore));
                    }
                };
            });

            WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                Verify.IsTrue(scrollPresenter.HorizontalOffset < 600.0);
                Verify.IsTrue(scrollPresenter.VerticalOffset < 400.0);
                Verify.AreEqual(1.0f, scrollPresenter.ZoomFactor);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Interrupted, operation.Result);
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

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("ScrollPresenter"))
            {
                ScrollPresenter scrollPresenter = null;
                Rectangle rectangleScrollPresenterContent = null;
                AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollPresenterContent = new Rectangle();
                    scrollPresenter = new ScrollPresenter();
                    scrollPresenter.Name = "scr";

                    SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    operation = StartZoomTo(
                        scrollPresenter,
                        8.0f,
                        100.0f,
                        150.0f,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvent);

                    bool operationCanceled = false;

                    scrollPresenter.ViewChanged += (sender, args) =>
                    {
                        Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");

                        if (sender.ZoomFactor >= 2.0 && !operationCanceled)
                        {
                            Log.Comment("Canceling view change");
                            operationCanceled = true;
                            sender.ZoomBy(0, Vector2.Zero, new ScrollingZoomOptions(AnimationMode.Disabled, SnapPointsMode.Ignore));
                        }
                    };
                });

                WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                    Verify.IsTrue(scrollPresenter.ZoomFactor < 8.0f);
                    Verify.AreEqual(ScrollPresenterViewChangeResult.Interrupted, operation.Result);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Checks to make sure the exposed startPosition, endPosition, StartZoomFactor, and EndZoomFactor " +
            "on ScrollingScrollAnimationStartingEventArgs and ScrollingZoomAnimationStartingEventArgs respectively are accurate.")]
        public void ValidateScrollAnimationStartingAndZoomFactorEventArgsHaveValidStartAndEndPositions()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1.");
                return;
            }
            int numOffsetChanges = 0;
            int numZoomFactorChanges = 0;
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            Point newPosition1 = new Point(100, 150);
            Point newPosition2 = new Point(50, 100);
            float newZoomFactor1 = 2.0f;
            float newZoomFactor2 = 0.5f;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(
                    scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Attach to ScrollAnimationStarting");

                scrollPresenter.ZoomAnimationStarting += (ScrollPresenter sender, ScrollingZoomAnimationStartingEventArgs e) =>
                {
                    Log.Comment("ScrollPresenter.ZoomAnimationStarting event handler");
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

                scrollPresenter.ScrollAnimationStarting += (ScrollPresenter sender, ScrollingScrollAnimationStartingEventArgs e) =>
                {
                    Log.Comment("ScrollPresenter.ScrollAnimationStarting event handler");
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
            ScrollTo(scrollPresenter, newPosition1.X, newPosition1.Y, AnimationMode.Enabled, SnapPointsMode.Ignore);

            Log.Comment("Animating to absolute Offset");
            ScrollTo(scrollPresenter, newPosition2.X, newPosition2.Y, AnimationMode.Enabled, SnapPointsMode.Ignore);

            Log.Comment("Animating to absolute zoomFactor");
            ZoomTo(scrollPresenter, newZoomFactor1, 100.0f, 200.0f, AnimationMode.Enabled, SnapPointsMode.Ignore);

            Log.Comment("Animating to absolute zoomFactor");
            ZoomTo(scrollPresenter, newZoomFactor2, 100.0f, 200.0f, AnimationMode.Enabled, SnapPointsMode.Ignore);

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

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("ScrollPresenter"))
            {
                ScrollPresenter scrollPresenter = null;
                Rectangle rectangleScrollPresenterContent = null;
                AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollPresenterContent = new Rectangle();
                    scrollPresenter = new ScrollPresenter();

                    SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    scrollPresenter.ViewChanged += (sender, args) =>
                    {
                        Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                    };

                    scrollPresenter.ScrollAnimationStarting += (sender, args) =>
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
                        scrollPresenter,
                        600.0,
                        400.0,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scrollPresenter.HorizontalOffset}, VerticalOffset={scrollPresenter.VerticalOffset}, ZoomFactor={scrollPresenter.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(600.0, scrollPresenter.HorizontalOffset);
                    Verify.AreEqual(400.0, scrollPresenter.VerticalOffset);
                    Verify.AreEqual(1.0f, scrollPresenter.ZoomFactor);

                    Verify.IsLessThanOrEqual(viewChangedCount, 2u);

                    Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operation.Result);
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

            using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("ScrollPresenter"))
            {
                ScrollPresenter scrollPresenter = null;
                Rectangle rectangleScrollPresenterContent = null;
                AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollPresenterContent = new Rectangle();
                    scrollPresenter = new ScrollPresenter();

                    SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    scrollPresenter.ViewChanged += (sender, args) =>
                    {
                        Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                    };

                    scrollPresenter.ZoomAnimationStarting += (sender, args) =>
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
                        scrollPresenter,
                        8.0f,
                        100.0f,
                        150.0f,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scrollPresenter.HorizontalOffset}, VerticalOffset={scrollPresenter.VerticalOffset}, ZoomFactor={scrollPresenter.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.IsLessThan(Math.Abs(scrollPresenter.HorizontalOffset - 700.0), 0.01);
                    Verify.IsLessThan(Math.Abs(scrollPresenter.VerticalOffset - 1050.0), 0.01);
                    Verify.AreEqual(8.0f, scrollPresenter.ZoomFactor);

                    Verify.IsLessThanOrEqual(viewChangedCount, 2u);
                    Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operation.Result);
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Requests a view change just before unloading scrollPresenter.")]
        public void InterruptViewChangeWithUnloading()
        {
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollPresenterOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent, setAsContentRoot: true);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                operation = StartScrollTo(
                    scrollPresenter,
                    600.0,
                    400.0,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollPresenterViewChangeOperationEvent);

                Log.Comment("Resetting window content to unparent ScrollPresenter");
                Content = null;
            });

            WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                Verify.AreEqual(0.0, scrollPresenter.HorizontalOffset);
                Verify.AreEqual(0.0, scrollPresenter.VerticalOffset);
                Verify.AreEqual(1.0f, scrollPresenter.ZoomFactor);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Interrupted, operation.Result);
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
            ScrollPresenter scrollPresenter = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollPresenterOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, null /*rectangleScrollPresenterContent*/, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                operation = StartScrollTo(
                    scrollPresenter,
                    600.0,
                    400.0,
                    AnimationMode.Enabled,
                    SnapPointsMode.Ignore,
                    scrollPresenterViewChangeOperationEvent);
            });

            WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                Verify.AreEqual(0.0, scrollPresenter.HorizontalOffset);
                Verify.AreEqual(0.0, scrollPresenter.VerticalOffset);
                Verify.AreEqual(1.0f, scrollPresenter.ZoomFactor);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Ignored, operation.Result);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Attempts a zoomFactor change while there is no content.")]
        public void ZoomFactorChangeWithNoContent()
        {
            ScrollPresenter scrollPresenter = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollPresenterOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, null /*rectangleScrollPresenterContent*/, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                operation = StartZoomTo(
                    scrollPresenter,
                    8.0f,
                    100.0f,
                    150.0f,
                    AnimationMode.Enabled,
                    SnapPointsMode.Ignore,
                    scrollPresenterViewChangeOperationEvent);
            });

            WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                Verify.AreEqual(0.0, scrollPresenter.HorizontalOffset);
                Verify.AreEqual(0.0, scrollPresenter.VerticalOffset);
                Verify.AreEqual(1.0f, scrollPresenter.ZoomFactor);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Ignored, operation.Result);
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
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollPresenterViewChangeOperationEvents = null;
            ScrollPresenterOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollPresenterViewChangeOperationEvents = new AutoResetEvent[3];
                scrollPresenterViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollPresenterViewChangeOperationEvents[1] = new AutoResetEvent(false);
                scrollPresenterViewChangeOperationEvents[2] = new AutoResetEvent(false);

                operations = new ScrollPresenterOperation[3];

                scrollPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                operations[0] = StartScrollTo(
                    scrollPresenter,
                    600.0,
                    400.0,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollPresenterViewChangeOperationEvents[0]);

                if (!waitForFirstCompletion)
                {
                    operations[1] = StartScrollTo(
                        scrollPresenter,
                        500.0,
                        300.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[1]);
                }
            });

            WaitForEvent("Waiting for first view change completion", scrollPresenterViewChangeOperationEvents[0]);

            if (waitForFirstCompletion)
            {
                RunOnUIThread.Execute(() =>
                {
                    operations[1] = StartScrollTo(
                        scrollPresenter,
                        500.0,
                        300.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[1]);
                });
            }

            WaitForEvent("Waiting for second view change completion", scrollPresenterViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                Verify.AreEqual(500.0, scrollPresenter.HorizontalOffset);
                Verify.AreEqual(300.0, scrollPresenter.VerticalOffset);
                Verify.AreEqual(1.0f, scrollPresenter.ZoomFactor);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operations[0].Result);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operations[1].Result);

                // Jump to the same offsets.
                operations[2] = StartScrollTo(
                    scrollPresenter,
                    500.0,
                    300.0,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollPresenterViewChangeOperationEvents[2]);
            });

            WaitForEvent("Waiting for third view change completion", scrollPresenterViewChangeOperationEvents[2]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                Verify.AreEqual(500.0, scrollPresenter.HorizontalOffset);
                Verify.AreEqual(300.0, scrollPresenter.VerticalOffset);
                Verify.AreEqual(1.0f, scrollPresenter.ZoomFactor);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operations[2].Result);
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
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollPresenterViewChangeOperationEvents = null;
            ScrollPresenterOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollPresenterViewChangeOperationEvents = new AutoResetEvent[3];
                scrollPresenterViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollPresenterViewChangeOperationEvents[1] = new AutoResetEvent(false);
                scrollPresenterViewChangeOperationEvents[2] = new AutoResetEvent(false);

                operations = new ScrollPresenterOperation[3];

                scrollPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                if (isFirstZoomRelative)
                {
                    operations[0] = StartZoomBy(
                        scrollPresenter,
                        7.0f,
                        150.0f,
                        120.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartZoomTo(
                        scrollPresenter,
                        8.0f,
                        150.0f,
                        120.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[0]);
                }
            });

            if (waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollPresenterViewChangeOperationEvents[0]);
            }

            RunOnUIThread.Execute(() =>
            {
                if (isFirstZoomRelative)
                {
                    operations[1] = StartZoomBy(
                        scrollPresenter,
                        -1.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[1]);
                }
                else
                {
                    operations[1] = StartZoomTo(
                        scrollPresenter,
                        7.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[1]);
                }
            });

            if (!waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollPresenterViewChangeOperationEvents[0]);
            }
            WaitForEvent("Waiting for second view change completion", scrollPresenterViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                Verify.AreEqual(917.5, scrollPresenter.HorizontalOffset);
                Verify.AreEqual(723.75, scrollPresenter.VerticalOffset);
                Verify.AreEqual(7.0f, scrollPresenter.ZoomFactor);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operations[0].Result);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operations[1].Result);

                // Jump to the same zoomFactor
                operations[2] = StartZoomTo(
                    scrollPresenter,
                    7.0f,
                    10.0f,
                    90.0f,
                    AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollPresenterViewChangeOperationEvents[2]);
            });

            WaitForEvent("Waiting for third view change completion", scrollPresenterViewChangeOperationEvents[2]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                Verify.AreEqual(917.5, scrollPresenter.HorizontalOffset);
                Verify.AreEqual(723.75, scrollPresenter.VerticalOffset);
                Verify.AreEqual(7.0f, scrollPresenter.ZoomFactor);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operations[2].Result);
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
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollPresenterViewChangeOperationEvents = null;
            ScrollPresenterOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollPresenterViewChangeOperationEvents = new AutoResetEvent[2];
                scrollPresenterViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollPresenterViewChangeOperationEvents[1] = new AutoResetEvent(false);

                operations = new ScrollPresenterOperation[2];

                scrollPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                if (isScrollRelative)
                {
                    operations[0] = StartScrollBy(
                        scrollPresenter,
                        80.0,
                        35.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartScrollTo(
                        scrollPresenter,
                        80.0,
                        35.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[0]);
                }
            });

            if (waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollPresenterViewChangeOperationEvents[0]);
            }

            RunOnUIThread.Execute(() =>
            {
                if (isZoomRelative)
                {
                    operations[1] = StartZoomBy(
                        scrollPresenter,
                        2.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[1]);
                }
                else
                {
                    operations[1] = StartZoomTo(
                        scrollPresenter,
                        3.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[1]);
                }
            });

            if (!waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollPresenterViewChangeOperationEvents[0]);
            }
            WaitForEvent("Waiting for second view change completion", scrollPresenterViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                Verify.AreEqual(260.0, scrollPresenter.HorizontalOffset);
                Verify.AreEqual(285.0, scrollPresenter.VerticalOffset);
                Verify.AreEqual(3.0f, scrollPresenter.ZoomFactor);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operations[0].Result);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operations[1].Result);
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
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollPresenterViewChangeOperationEvents = null;
            ScrollPresenterOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollPresenterViewChangeOperationEvents = new AutoResetEvent[2];
                scrollPresenterViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollPresenterViewChangeOperationEvents[1] = new AutoResetEvent(false);

                operations = new ScrollPresenterOperation[2];

                scrollPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                if (isZoomRelative)
                {
                    operations[0] = StartZoomBy(
                        scrollPresenter,
                        2.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartZoomTo(
                        scrollPresenter,
                        3.0f,
                        10.0f,
                        90.0f,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[0]);
                }
            });

            if (waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollPresenterViewChangeOperationEvents[0]);
            }

            RunOnUIThread.Execute(() =>
            {
                if (isScrollRelative)
                {
                    operations[1] = StartScrollBy(
                        scrollPresenter,
                        80.0,
                        35.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[1]);
                }
                else
                {
                    operations[1] = StartScrollTo(
                        scrollPresenter,
                        80.0,
                        35.0,
                        AnimationMode.Disabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[1]);
                }
            });

            if (!waitForFirstCompletion)
            {
                WaitForEvent("Waiting for first view change completion", scrollPresenterViewChangeOperationEvents[0]);
            }
            WaitForEvent("Waiting for second view change completion", scrollPresenterViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                if (isScrollRelative)
                {
                    Verify.AreEqual(100.0, scrollPresenter.HorizontalOffset);
                    Verify.AreEqual(215.0, scrollPresenter.VerticalOffset);
                }
                else
                {
                    Verify.AreEqual(80.0, scrollPresenter.HorizontalOffset);
                    Verify.AreEqual(35.0, scrollPresenter.VerticalOffset);
                }
                Verify.AreEqual(3.0f, scrollPresenter.ZoomFactor);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operations[0].Result);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operations[1].Result);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Requests a non-animated offsets change before loading scrollPresenter.")]
        public void SetOffsetsBeforeLoading()
        {
            ChangeOffsetsBeforeLoading(false /*animate*/);
        }

        [TestMethod]
        [TestProperty("Description", "Requests an animated offsets change before loading scrollPresenter.")]
        public void AnimateOffsetsBeforeLoading()
        {
            ChangeOffsetsBeforeLoading(true /*animate*/);
        }

        [TestMethod]
        [TestProperty("Description", "Requests a non-animated zoomFactor change before loading scrollPresenter.")]
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
        [TestProperty("Description", "Requests an animated zoomFactor change before loading scrollPresenter.")]
        public void AnimateZoomFactorBeforeLoading()
        {
            ChangeZoomFactorBeforeLoading(true /*animate*/);
        }

        [TestMethod]
        [TestProperty("Description", "Requests a non-animated offset change immediately after increasing content size.")]
        public void OffsetJumpAfterContentResizing()
        {
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent.Width = c_defaultUIScrollPresenterContentWidth + 200.0;
            });

            // Jump to absolute offsets
            ScrollTo(
                scrollPresenter,
                c_defaultUIScrollPresenterContentWidth + 200.0 - c_defaultUIScrollPresenterWidth,
                c_defaultVerticalOffset,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore);
        }

        private void ChangeOffsetsBeforeLoading(bool animate)
        {
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollPresenterOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent, false /*setAsContentRoot*/);

                scrollPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                operation = StartScrollTo(
                    scrollPresenter,
                    600.0,
                    400.0,
                    animate ? AnimationMode.Enabled : AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollPresenterViewChangeOperationEvent);

                Log.Comment("Setting window content");
                Content = scrollPresenter;
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                Verify.AreEqual(600.0, scrollPresenter.HorizontalOffset);
                Verify.AreEqual(400.0, scrollPresenter.VerticalOffset);
                Verify.AreEqual(1.0f, scrollPresenter.ZoomFactor);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operation.Result);
            });
        }

        private void ChangeZoomFactorBeforeLoading(bool animate)
        {
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollPresenterOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent, false /*setAsContentRoot*/);

                scrollPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                };

                operation = StartZoomTo(
                    scrollPresenter,
                    8.0f,
                    100.0f,
                    150.0f,
                    animate ? AnimationMode.Enabled : AnimationMode.Disabled,
                    SnapPointsMode.Ignore,
                    scrollPresenterViewChangeOperationEvent);

                Log.Comment("Setting window content");
                Content = scrollPresenter;
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                Verify.IsLessThan(Math.Abs(scrollPresenter.HorizontalOffset - 700.0), 0.01);
                Verify.IsLessThan(Math.Abs(scrollPresenter.VerticalOffset - 1050.0), 0.01);
                Verify.AreEqual(8.0f, scrollPresenter.ZoomFactor);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operation.Result);
            });
        }

        private void ScrollTo(
            ScrollPresenter scrollPresenter,
            double horizontalOffset,
            double verticalOffset,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            bool? isAnimationsEnabledOverride = null,
            double? expectedFinalHorizontalOffset = null,
            double? expectedFinalVerticalOffset = null)
        {
            using (ScrollPresenterTestHooksHelper scrollPresenterTestHooksHelper = new ScrollPresenterTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true,
                isAnimationsEnabledOverride: isAnimationsEnabledOverride))
            {
                Log.Comment("Waiting for any pending ExpressionAnimation start/stop notifications to occur");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    scrollPresenterTestHooksHelper.ResetExpressionAnimationStatusChanges(scrollPresenter);

                    if (hookViewChanged)
                    {
                        scrollPresenter.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalZoomFactor = scrollPresenter.ZoomFactor;

                    if (expectedFinalHorizontalOffset == null)
                    {
                        expectedFinalHorizontalOffset = horizontalOffset;
                    }

                    if (expectedFinalVerticalOffset == null)
                    {
                        expectedFinalVerticalOffset = verticalOffset;
                    }

                    operation = StartScrollTo(
                        scrollPresenter,
                        horizontalOffset,
                        verticalOffset,
                        animationMode,
                        snapPointsMode,
                        scrollPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scrollPresenter.HorizontalOffset}, VerticalOffset={scrollPresenter.VerticalOffset}, ZoomFactor={scrollPresenter.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(expectedFinalHorizontalOffset, scrollPresenter.HorizontalOffset);
                    Verify.AreEqual(expectedFinalVerticalOffset, scrollPresenter.VerticalOffset);
                    Verify.AreEqual(originalZoomFactor, scrollPresenter.ZoomFactor);
                    Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operation.Result);

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
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollPresenterTestHooksHelper.GetExpressionAnimationStatusChanges(scrollPresenter);
                    ScrollPresenterTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    Verify.IsNull(expressionAnimationStatusChanges);
                });
            }
        }

        private void ScrollBy(
            ScrollPresenter scrollPresenter,
            double horizontalOffsetDelta,
            double verticalOffsetDelta,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            bool? isAnimationsEnabledOverride = null,
            double? expectedFinalHorizontalOffset = null,
            double? expectedFinalVerticalOffset = null)
        {
            using (ScrollPresenterTestHooksHelper scrollPresenterTestHooksHelper = new ScrollPresenterTestHooksHelper(
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
                AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    scrollPresenterTestHooksHelper.ResetExpressionAnimationStatusChanges(scrollPresenter);

                    if (hookViewChanged)
                    {
                        scrollPresenter.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalHorizontalOffset = scrollPresenter.HorizontalOffset;
                    originalVerticalOffset = scrollPresenter.VerticalOffset;
                    originalZoomFactor = scrollPresenter.ZoomFactor;

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
                        scrollPresenter,
                        horizontalOffsetDelta,
                        verticalOffsetDelta,
                        animationMode,
                        snapPointsMode,
                        scrollPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scrollPresenter.HorizontalOffset}, VerticalOffset={scrollPresenter.VerticalOffset}, ZoomFactor={scrollPresenter.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(expectedFinalHorizontalOffset, scrollPresenter.HorizontalOffset);
                    Verify.AreEqual(expectedFinalVerticalOffset, scrollPresenter.VerticalOffset);
                    Verify.AreEqual(originalZoomFactor, scrollPresenter.ZoomFactor);
                    Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operation.Result);

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
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollPresenterTestHooksHelper.GetExpressionAnimationStatusChanges(scrollPresenter);
                    ScrollPresenterTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    Verify.IsNull(expressionAnimationStatusChanges);
                });
            }
        }

        private void ScrollFrom(
            ScrollPresenter scrollPresenter,
            float horizontalVelocity,
            float verticalVelocity,
            float? horizontalInertiaDecayRate,
            float? verticalInertiaDecayRate,
            bool hookViewChanged = true)
        {
            using (ScrollPresenterTestHooksHelper scrollPresenterTestHooksHelper = new ScrollPresenterTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true))
            {
                Log.Comment("Waiting for any pending ExpressionAnimation start/stop notifications to occur");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                double originalHorizontalOffset = 0.0;
                double originalVerticalOffset = 0.0;
                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    scrollPresenterTestHooksHelper.ResetExpressionAnimationStatusChanges(scrollPresenter);

                    if (hookViewChanged)
                    {
                        scrollPresenter.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalHorizontalOffset = scrollPresenter.HorizontalOffset;
                    originalVerticalOffset = scrollPresenter.VerticalOffset;
                    originalZoomFactor = scrollPresenter.ZoomFactor;

                    Log.Comment($"Original HorizontalOffset={originalHorizontalOffset}, VerticalOffset={originalVerticalOffset}, ZoomFactor={originalZoomFactor}");

                    operation = StartScrollFrom(
                        scrollPresenter,
                        horizontalVelocity,
                        verticalVelocity,
                        horizontalInertiaDecayRate,
                        verticalInertiaDecayRate,
                        scrollPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                    if (horizontalVelocity > 0)
                        Verify.IsTrue(originalHorizontalOffset < scrollPresenter.HorizontalOffset);
                    else if (horizontalVelocity < 0)
                        Verify.IsTrue(originalHorizontalOffset > scrollPresenter.HorizontalOffset);
                    else
                        Verify.IsTrue(originalHorizontalOffset == scrollPresenter.HorizontalOffset);
                    if (verticalVelocity > 0)
                        Verify.IsTrue(originalVerticalOffset < scrollPresenter.VerticalOffset);
                    else if (verticalVelocity < 0)
                        Verify.IsTrue(originalVerticalOffset > scrollPresenter.VerticalOffset);
                    else
                        Verify.IsTrue(originalVerticalOffset == scrollPresenter.VerticalOffset);
                    Verify.AreEqual(originalZoomFactor, scrollPresenter.ZoomFactor);
                    Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operation.Result);
                });

                Log.Comment("Waiting for any ExpressionAnimation start/stop notification");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                RunOnUIThread.Execute(() =>
                {
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollPresenterTestHooksHelper.GetExpressionAnimationStatusChanges(scrollPresenter);
                    ScrollPresenterTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    Verify.IsNull(expressionAnimationStatusChanges);
                });
            }
        }

        private ScrollPresenterOperation StartScrollTo(
            ScrollPresenter scrollPresenter,
            double horizontalOffset,
            double verticalOffset,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            AutoResetEvent scrollPresenterViewChangeOperationEvent)
        {
            Log.Comment("ScrollTo - horizontalOffset={0}, verticalOffset={1}, animationMode={2}, snapPointsMode={3}",
                horizontalOffset, verticalOffset, animationMode, snapPointsMode);

            viewChangedCount = 0u;
            ScrollPresenterOperation operation = new ScrollPresenterOperation();

            operation.Id = scrollPresenter.ScrollTo(
                horizontalOffset,
                verticalOffset,
                new ScrollingScrollOptions(animationMode, snapPointsMode)).OffsetsChangeId;

            if (operation.Id == -1)
            {
                scrollPresenterViewChangeOperationEvent.Set();
            }
            else
            {
                scrollPresenter.ScrollCompleted += (ScrollPresenter sender, ScrollingScrollCompletedEventArgs args) =>
                {
                    if (args.ScrollInfo.OffsetsChangeId == operation.Id)
                    {
                        ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetScrollCompletedResult(args);

                        Log.Comment("ScrollCompleted: ScrollTo OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollPresenterViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollPresenterOperation StartScrollBy(
            ScrollPresenter scrollPresenter,
            double horizontalOffsetDelta,
            double verticalOffsetDelta,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            AutoResetEvent scrollPresenterViewChangeOperationEvent)
        {
            Log.Comment("ScrollBy - horizontalOffsetDelta={0}, verticalOffsetDelta={1}, animationMode={2}, snapPointsMode={3}",
                horizontalOffsetDelta, verticalOffsetDelta, animationMode, snapPointsMode);

            viewChangedCount = 0u;
            ScrollPresenterOperation operation = new ScrollPresenterOperation();

            operation.Id = scrollPresenter.ScrollBy(
                horizontalOffsetDelta,
                verticalOffsetDelta,
                new ScrollingScrollOptions(animationMode, snapPointsMode)).OffsetsChangeId;

            if (operation.Id == -1)
            {
                scrollPresenterViewChangeOperationEvent.Set();
            }
            else
            {
                scrollPresenter.ScrollCompleted += (ScrollPresenter sender, ScrollingScrollCompletedEventArgs args) =>
                {
                    if (args.ScrollInfo.OffsetsChangeId == operation.Id)
                    {
                        ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetScrollCompletedResult(args);

                        Log.Comment("ScrollCompleted: ScrollBy OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollPresenterViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollPresenterOperation StartScrollFrom(
            ScrollPresenter scrollPresenter,
            float horizontalVelocity,
            float verticalVelocity,
            float? horizontalInertiaDecayRate,
            float? verticalInertiaDecayRate,
            AutoResetEvent scrollPresenterViewChangeOperationEvent)
        {
            Log.Comment("ScrollFrom - horizontalVelocity={0}, verticalVelocity={1}, horizontalInertiaDecayRate={2}, verticalInertiaDecayRate={3}",
                horizontalVelocity, verticalVelocity, horizontalInertiaDecayRate, verticalInertiaDecayRate);

            Vector2? inertiaDecayRate = null;

            if (horizontalInertiaDecayRate != null && verticalInertiaDecayRate != null)
            {
                inertiaDecayRate = new Vector2((float)horizontalInertiaDecayRate, (float)verticalInertiaDecayRate);
            }

            viewChangedCount = 0u;
            ScrollPresenterOperation operation = new ScrollPresenterOperation();

            operation.Id = scrollPresenter.ScrollFrom(
                    new Vector2(horizontalVelocity, verticalVelocity),
                    inertiaDecayRate).OffsetsChangeId;

            if (operation.Id == -1)
            {
                scrollPresenterViewChangeOperationEvent.Set();
            }
            else
            {
                scrollPresenter.ScrollCompleted += (ScrollPresenter sender, ScrollingScrollCompletedEventArgs args) =>
                {
                    if (args.ScrollInfo.OffsetsChangeId == operation.Id)
                    {
                        ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetScrollCompletedResult(args);

                        Log.Comment("ScrollCompleted: ScrollFrom OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollPresenterViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private void ZoomTo(
            ScrollPresenter scrollPresenter,
            float zoomFactor,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            bool? isAnimationsEnabledOverride = null)
        {
            using (ScrollPresenterTestHooksHelper scrollPresenterTestHooksHelper = new ScrollPresenterTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true, 
                isAnimationsEnabledOverride: isAnimationsEnabledOverride))
            {
                AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    if (hookViewChanged)
                    {
                        scrollPresenter.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    operation = StartZoomTo(
                        scrollPresenter,
                        zoomFactor,
                        centerPointX,
                        centerPointY,
                        animationMode,
                        snapPointsMode,
                        scrollPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scrollPresenter.HorizontalOffset}, VerticalOffset={scrollPresenter.VerticalOffset}, ZoomFactor={scrollPresenter.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(zoomFactor, scrollPresenter.ZoomFactor);
                    Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operation.Result);

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
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollPresenterTestHooksHelper.GetExpressionAnimationStatusChanges(scrollPresenter);
                    ScrollPresenterTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    VerifyExpressionAnimationStatusChangesForTranslationAndZoomFactorSuspension(expressionAnimationStatusChanges);
                });
            }
        }

        private void ZoomBy(
            ScrollPresenter scrollPresenter,
            float zoomFactorDelta,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            bool hookViewChanged = true,
            bool? isAnimationsEnabledOverride = null)
        {
            using (ScrollPresenterTestHooksHelper scrollPresenterTestHooksHelper = new ScrollPresenterTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true,
                isAnimationsEnabledOverride: isAnimationsEnabledOverride))
            {
                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    if (hookViewChanged)
                    {
                        scrollPresenter.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalZoomFactor = scrollPresenter.ZoomFactor;

                    Log.Comment($"Original HorizontalOffset={scrollPresenter.HorizontalOffset}, VerticalOffset={scrollPresenter.VerticalOffset}, ZoomFactor={originalZoomFactor}");

                    operation = StartZoomBy(
                        scrollPresenter,
                        zoomFactorDelta,
                        centerPointX,
                        centerPointY,
                        animationMode,
                        snapPointsMode,
                        scrollPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Final HorizontalOffset={scrollPresenter.HorizontalOffset}, VerticalOffset={scrollPresenter.VerticalOffset}, ZoomFactor={scrollPresenter.ZoomFactor}");
                    Log.Comment($"Final viewChangedCount={viewChangedCount}");

                    Verify.AreEqual(zoomFactorDelta + originalZoomFactor, scrollPresenter.ZoomFactor);
                    Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operation.Result);

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
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollPresenterTestHooksHelper.GetExpressionAnimationStatusChanges(scrollPresenter);
                    ScrollPresenterTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    VerifyExpressionAnimationStatusChangesForTranslationAndZoomFactorSuspension(expressionAnimationStatusChanges);                    
                });
            }
        }

        private void ZoomFrom(
            ScrollPresenter scrollPresenter,
            float zoomFactorVelocity,
            float? inertiaDecayRate,
            float centerPointX,
            float centerPointY,
            bool hookViewChanged = true)
        {
            using (ScrollPresenterTestHooksHelper scrollPresenterTestHooksHelper = new ScrollPresenterTestHooksHelper(
                enableAnchorNotifications: false,
                enableInteractionSourcesNotifications: false,
                enableExpressionAnimationStatusNotifications: true))
            {
                float originalZoomFactor = 1.0f;
                AutoResetEvent scrollPresenterViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollPresenterOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    if (hookViewChanged)
                    {
                        scrollPresenter.ViewChanged += (sender, args) =>
                        {
                            Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
                        };
                    }

                    originalZoomFactor = scrollPresenter.ZoomFactor;

                    Log.Comment($"Original HorizontalOffset={scrollPresenter.HorizontalOffset}, VerticalOffset={scrollPresenter.VerticalOffset}, ZoomFactor={originalZoomFactor}");

                    operation = StartZoomFrom(
                        scrollPresenter,
                        zoomFactorVelocity,
                        inertiaDecayRate,
                        centerPointX,
                        centerPointY,
                        scrollPresenterViewChangeOperationEvent);
                });

                WaitForEvent("Waiting for view change completion", scrollPresenterViewChangeOperationEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                    if (zoomFactorVelocity > 0)
                        Verify.IsTrue(originalZoomFactor < scrollPresenter.ZoomFactor);
                    else if (zoomFactorVelocity < 0)
                        Verify.IsTrue(originalZoomFactor > scrollPresenter.ZoomFactor);
                    else
                        Verify.IsTrue(originalZoomFactor == scrollPresenter.ZoomFactor);
                    Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operation.Result);
                });

                Log.Comment("Waiting for any ExpressionAnimation start/stop notification");
                CompositionPropertySpy.SynchronouslyTickUIThread(6);

                RunOnUIThread.Execute(() =>
                {
                    List<ExpressionAnimationStatusChange> expressionAnimationStatusChanges = scrollPresenterTestHooksHelper.GetExpressionAnimationStatusChanges(scrollPresenter);
                    ScrollPresenterTestHooksHelper.LogExpressionAnimationStatusChanges(expressionAnimationStatusChanges);
                    VerifyExpressionAnimationStatusChangesForTranslationAndZoomFactorSuspension(expressionAnimationStatusChanges);
                });
            }
        }

        private ScrollPresenterOperation StartZoomTo(
            ScrollPresenter scrollPresenter,
            float zoomFactor,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            AutoResetEvent scrollPresenterViewChangeOperationEvent)
        {
            Log.Comment("ZoomTo - zoomFactor={0}, centerPoint=({1},{2}), animationMode={3}, snapPointsMode={4}",
                zoomFactor, centerPointX, centerPointY, animationMode, snapPointsMode);

            viewChangedCount = 0u;
            ScrollPresenterOperation operation = new ScrollPresenterOperation();

            operation.Id = scrollPresenter.ZoomTo(
                zoomFactor,
                new Vector2(centerPointX, centerPointY), 
                new ScrollingZoomOptions(animationMode, snapPointsMode)).ZoomFactorChangeId;

            if (operation.Id == -1)
            {
                scrollPresenterViewChangeOperationEvent.Set();
            }
            else
            {
                scrollPresenter.ZoomCompleted += (ScrollPresenter sender, ScrollingZoomCompletedEventArgs args) =>
                {
                    if (args.ZoomInfo.ZoomFactorChangeId == operation.Id)
                    {
                        ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetZoomCompletedResult(args);

                        Log.Comment("ZoomCompleted: ZoomTo ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollPresenterViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollPresenterOperation StartZoomBy(
            ScrollPresenter scrollPresenter,
            float zoomFactorDelta,
            float centerPointX,
            float centerPointY,
            AnimationMode animationMode,
            SnapPointsMode snapPointsMode,
            AutoResetEvent scrollPresenterViewChangeOperationEvent)
        {
            Log.Comment("ZoomBy - zoomFactorDelta={0}, centerPoint=({1},{2}), animationMode={3}, snapPointsMode={4}",
                zoomFactorDelta, centerPointX, centerPointY, animationMode, snapPointsMode);

            viewChangedCount = 0u;
            ScrollPresenterOperation operation = new ScrollPresenterOperation();

            operation.Id = scrollPresenter.ZoomBy(
                zoomFactorDelta,
                new Vector2(centerPointX, centerPointY),
                new ScrollingZoomOptions(animationMode, snapPointsMode)).ZoomFactorChangeId;

            if (operation.Id == -1)
            {
                scrollPresenterViewChangeOperationEvent.Set();
            }
            else
            {
                scrollPresenter.ZoomCompleted += (ScrollPresenter sender, ScrollingZoomCompletedEventArgs args) =>
                {
                    if (args.ZoomInfo.ZoomFactorChangeId == operation.Id)
                    {
                        ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetZoomCompletedResult(args);

                        Log.Comment("ZoomCompleted: ZoomBy ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollPresenterViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollPresenterOperation StartZoomFrom(
            ScrollPresenter scrollPresenter,
            float zoomFactorVelocity,
            float? inertiaDecayRate,
            float centerPointX,
            float centerPointY,
            AutoResetEvent scrollPresenterViewChangeOperationEvent)
        {
            Log.Comment("ZoomFrom - zoomFactorVelocity={0}, inertiaDecayRate={1}, centerPoint=({2},{3})",
                zoomFactorVelocity, inertiaDecayRate, centerPointX, centerPointY);

            viewChangedCount = 0u;
            ScrollPresenterOperation operation = new ScrollPresenterOperation();

            operation.Id = scrollPresenter.ZoomFrom(
                zoomFactorVelocity, new Vector2(centerPointX, centerPointY), inertiaDecayRate).ZoomFactorChangeId;

            if (operation.Id == -1)
            {
                scrollPresenterViewChangeOperationEvent.Set();
            }
            else
            {
                scrollPresenter.ZoomCompleted += (ScrollPresenter sender, ScrollingZoomCompletedEventArgs args) =>
                {
                    if (args.ZoomInfo.ZoomFactorChangeId == operation.Id)
                    {
                        ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetZoomCompletedResult(args);

                        Log.Comment("ZoomCompleted: ZoomFrom ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
                        operation.Result = result;

                        Log.Comment("Setting completion event");
                        scrollPresenterViewChangeOperationEvent.Set();
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
            ScrollPresenter scrollPresenter = null;
            Rectangle rectangleScrollPresenterContent = null;
            AutoResetEvent scrollPresenterLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollPresenterViewChangeOperationEvents = null;
            ScrollPresenterOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollPresenterContent = new Rectangle();
                scrollPresenter = new ScrollPresenter();

                SetupDefaultUI(scrollPresenter, rectangleScrollPresenterContent, scrollPresenterLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollPresenterLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scrollPresenterViewChangeOperationEvents = new AutoResetEvent[2];
                scrollPresenterViewChangeOperationEvents[0] = new AutoResetEvent(false);
                scrollPresenterViewChangeOperationEvents[1] = new AutoResetEvent(false);

                operations = new ScrollPresenterOperation[2];
                operations[0] = null;
                operations[1] = null;

                scrollPresenter.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged viewChangedCount={++viewChangedCount} - HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");

                    if (viewChangedCount == 3u && !viewChangeInterruptionDone)
                    {
                        viewChangeInterruptionDone = true;
                        if (changeOffsetsSecond)
                        {
                            operations[1] = StartScrollTo(
                                scrollPresenter,
                                500.0,
                                300.0,
                                AnimationMode.Enabled,
                                SnapPointsMode.Ignore,
                                scrollPresenterViewChangeOperationEvents[1]);
                        }
                        else
                        {
                            operations[1] = StartZoomTo(
                                scrollPresenter,
                                7.0f,
                                70.0f,
                                50.0f,
                                AnimationMode.Enabled,
                                SnapPointsMode.Ignore,
                                scrollPresenterViewChangeOperationEvents[1]);
                        }
                    }
                };

                if (changeOffsetsFirst)
                {
                    operations[0] = StartScrollTo(
                        scrollPresenter,
                        600.0,
                        400.0,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartZoomTo(
                        scrollPresenter,
                        8.0f,
                        100.0f,
                        150.0f,
                        AnimationMode.Enabled,
                        SnapPointsMode.Ignore,
                        scrollPresenterViewChangeOperationEvents[0]);
                }
            });

            WaitForEvent("Waiting for first view change completion", scrollPresenterViewChangeOperationEvents[0]);

            WaitForEvent("Waiting for second view change completion", scrollPresenterViewChangeOperationEvents[1]);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scrollPresenter.HorizontalOffset, scrollPresenter.VerticalOffset, scrollPresenter.ZoomFactor);

                if (changeOffsetsFirst && changeOffsetsSecond)
                {
                    Verify.AreEqual(500.0, scrollPresenter.HorizontalOffset);
                    Verify.AreEqual(300.0, scrollPresenter.VerticalOffset);
                    Verify.AreEqual(1.0f, scrollPresenter.ZoomFactor);
                }
                if (changeOffsetsFirst && !changeOffsetsSecond)
                {
                    Verify.IsGreaterThanOrEqual(scrollPresenter.HorizontalOffset, 600.0);
                    Verify.IsGreaterThanOrEqual(scrollPresenter.VerticalOffset, 400.0);
                    Verify.AreEqual(7.0f, scrollPresenter.ZoomFactor);
                }
                if (!changeOffsetsFirst && changeOffsetsSecond)
                {
                    Verify.IsGreaterThanOrEqual(scrollPresenter.HorizontalOffset, 500.0);
                    Verify.IsGreaterThanOrEqual(scrollPresenter.VerticalOffset, 300.0);
                    Verify.AreEqual(8.0f, scrollPresenter.ZoomFactor);
                }
                if (!changeOffsetsFirst && !changeOffsetsSecond)
                {
                    Verify.AreEqual(7.0f, scrollPresenter.ZoomFactor);
                }

                Verify.AreEqual(ScrollPresenterViewChangeResult.Interrupted, operations[0].Result);
                Verify.AreEqual(ScrollPresenterViewChangeResult.Completed, operations[1].Result);
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

        private class ScrollPresenterOperation
        {
            public ScrollPresenterOperation()
            {
                Result = ScrollPresenterViewChangeResult.Ignored;
                Id = -1;
            }

            public ScrollPresenterViewChangeResult Result
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
