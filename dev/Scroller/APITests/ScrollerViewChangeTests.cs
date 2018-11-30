// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System;
using System.Numerics;
using System.Threading;
using Windows.Foundation;
using Windows.UI.Composition;
using Windows.UI.Xaml.Controls;
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
using Scroller = Microsoft.UI.Xaml.Controls.Scroller;
using ScrollerChangeOffsetsOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeOffsetsOptions;
using ScrollerChangeOffsetsWithAdditionalVelocityOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeOffsetsWithAdditionalVelocityOptions;
using ScrollerChangeZoomFactorOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeZoomFactorOptions;
using ScrollerChangeZoomFactorWithAdditionalVelocityOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeZoomFactorWithAdditionalVelocityOptions;
using ScrollerChangingOffsetsEventArgs = Microsoft.UI.Xaml.Controls.ScrollerChangingOffsetsEventArgs;
using ScrollerChangingZoomFactorEventArgs = Microsoft.UI.Xaml.Controls.ScrollerChangingZoomFactorEventArgs;
using ScrollerViewChangeCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollerViewChangeCompletedEventArgs;
using ScrollerViewChangeKind = Microsoft.UI.Xaml.Controls.ScrollerViewChangeKind;
using ScrollerViewChangeResult = Microsoft.UI.Xaml.Controls.ScrollerViewChangeResult;
using ScrollerViewChangeSnapPointRespect = Microsoft.UI.Xaml.Controls.ScrollerViewChangeSnapPointRespect;
using ScrollerViewKind = Microsoft.UI.Xaml.Controls.ScrollerViewKind;
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

        [TestMethod]
        [TestProperty("Description", "Changes Scroller offsets using various ScrollerViewKind and ScrollerViewChangeKind enum values.")]
        public void BasicOffsetChanges()
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Jump to absolute offsets
            ChangeOffsets(scroller, 11.0, 22.0, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints);

            // Jump to relative offsets
            ChangeOffsets(scroller, -4.0, 15.0, ScrollerViewKind.RelativeToCurrentView, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints, false /*hookViewChanged*/);

            // Animate to absolute offsets
            ChangeOffsets(scroller, 55.0, 25.0, ScrollerViewKind.Absolute, ScrollerViewChangeKind.AllowAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints, false /*hookViewChanged*/);

            // Animate to relative offsets
            ChangeOffsets(scroller, 700.0, -8.0, ScrollerViewKind.RelativeToCurrentView, ScrollerViewChangeKind.AllowAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints, false /*hookViewChanged*/);

            // Flick with additional offsets velocity
            ChangeOffsets(scroller, -65.0f, 80.0f, null /*horizontalInertiaDecayRate*/, null /*verticalInertiaDecayRate*/, false /*hookViewChanged*/);

            // Flick with additional offsets velocity and custom scroll inertia decay rate
            ChangeOffsets(scroller, 65.0f, -80.0f, 0.7f /*horizontalInertiaDecayRate*/, 0.8f /*verticalInertiaDecayRate*/, false /*hookViewChanged*/);

            // Do it all again while respecting snap points
            ChangeOffsets(scroller, 11.0, 22.0, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.RespectSnapPoints, false /*hookViewChanged*/);
            ChangeOffsets(scroller, -4.0, 15.0, ScrollerViewKind.RelativeToCurrentView, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.RespectSnapPoints, false /*hookViewChanged*/);
            ChangeOffsets(scroller, 55.0, 25.0, ScrollerViewKind.Absolute, ScrollerViewChangeKind.AllowAnimation, ScrollerViewChangeSnapPointRespect.RespectSnapPoints, false /*hookViewChanged*/);
            ChangeOffsets(scroller, 700.0, -8.0, ScrollerViewKind.RelativeToCurrentView, ScrollerViewChangeKind.AllowAnimation, ScrollerViewChangeSnapPointRespect.RespectSnapPoints, false /*hookViewChanged*/);
        }

        [TestMethod]
        [TestProperty("Description", "Changes Scroller zoomFactor using various ScrollerViewKind and ScrollerViewChangeKind enum values.")]
        public void BasicZoomFactorChanges()
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            // Jump to absolute zoomFactor
            ChangeZoomFactor(scroller, 2.0f, 22.0f, 33.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation);

            // Jump to relative zoomFactor
            ChangeZoomFactor(scroller, 1.0f, 55.0f, 66.0f, ScrollerViewKind.RelativeToCurrentView, ScrollerViewChangeKind.DisableAnimation, false /*hookViewChanged*/);

            // Animate to absolute zoomFactor
            ChangeZoomFactor(scroller, 4.0f, -40.0f, -25.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.AllowAnimation, false /*hookViewChanged*/);

            // Animate to relative zoomFactor
            ChangeZoomFactor(scroller, -2.0f, 100.0f, 200.0f, ScrollerViewKind.RelativeToCurrentView, ScrollerViewChangeKind.AllowAnimation, false /*hookViewChanged*/);

            // Flick with additional zoomFactor velocity
            ChangeZoomFactor(scroller, 2.0f, null /*inertiaDecayRate*/, -50.0f, 800.0f, false /*hookViewChanged*/);

            // Flick with additional zoomFactor velocity and custom zoomFactor inertia decay rate
            ChangeZoomFactor(scroller, -2.0f, 0.75f /*inertiaDecayRate*/, -50.0f, 800.0f, false /*hookViewChanged*/);
        }

        [TestMethod]
        [TestProperty("Description", "Cancels an animated offsets change.")]
        public void BasicOffsetsChangeCancelation()
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                operation = StartChangeOffsets(
                    scroller,
                    600.0,
                    400.0,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.AllowAnimation,
                    ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
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
                        sender.ChangeOffsets(new ScrollerChangeOffsetsOptions(0, 0, ScrollerViewKind.RelativeToCurrentView, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
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
                Rectangle rectangleScrollerChild = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollerOperation operation = null;

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollerChild = new Rectangle();
                    scroller = new Scroller();
                    scroller.Name = "scr";

                    SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    operation = StartChangeZoomFactor(
                        scroller,
                        8.0f,
                        100.0f,
                        150.0f,
                        ScrollerViewKind.Absolute,
                        ScrollerViewChangeKind.AllowAnimation,
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
                            sender.ChangeZoomFactor(new ScrollerChangeZoomFactorOptions(0, ScrollerViewKind.RelativeToCurrentView, Vector2.Zero, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));                            
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
            "on ScrollerChangingOffsetsEventArgs and ScrollerChangingZoomFactorEventArgs respectively are accurate.")]
        public void ValidateChangingOffsetsAndZoomFactorEventArgsHaveValidStartAndEndPositions()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping test on RS1.");
                return;
            }
            int numOffsetChanges = 0;
            int numZoomFactorChanges = 0;
            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            Point newPosition1 = new Point(100, 150);
            Point newPosition2 = new Point(50, 100);
            float newZoomFactor1 = 2.0f;
            float newZoomFactor2 = 0.5f;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(
                    scroller, rectangleScrollerChild, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Attach to ChangingOffsets");

                scroller.ChangingZoomFactor += (Scroller sender, ScrollerChangingZoomFactorEventArgs e) =>
                {
                    Log.Comment("Scroller.ChangingZoomFactor event handler");
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

                scroller.ChangingOffsets += (Scroller sender, ScrollerChangingOffsetsEventArgs e) =>
                {
                    Log.Comment("Scroller.ChangingOffsets event handler");
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
            ChangeOffsets(scroller, newPosition1.X, newPosition1.Y, ScrollerViewKind.Absolute, ScrollerViewChangeKind.AllowAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints);

            Log.Comment("Animating to absolute Offset");
            ChangeOffsets(scroller, newPosition2.X, newPosition2.Y, ScrollerViewKind.Absolute, ScrollerViewChangeKind.AllowAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints);

            Log.Comment("Animating to absolute zoomFactor");
            ChangeZoomFactor(scroller, newZoomFactor1, 100.0f, 200.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.AllowAnimation);

            Log.Comment("Animating to absolute zoomFactor");
            ChangeZoomFactor(scroller, newZoomFactor2, 100.0f, 200.0f, ScrollerViewKind.Absolute, ScrollerViewChangeKind.AllowAnimation);

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
                Rectangle rectangleScrollerChild = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollerOperation operation = null;
                int viewChangedCount = 0;

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollerChild = new Rectangle();
                    scroller = new Scroller();

                    SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);
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

                    scroller.ChangingOffsets += (sender, args) =>
                    {
                        Log.Comment("ChangingOffsets - ViewChangeId={0}", args.ViewChangeId);
                        Verify.IsNotNull(args.Animation);
                        Vector3KeyFrameAnimation stockKeyFrameAnimation = args.Animation as Vector3KeyFrameAnimation;
                        Verify.IsNotNull(stockKeyFrameAnimation);
                        Log.Comment("Stock duration={0} msec.", stockKeyFrameAnimation.Duration.TotalMilliseconds);
                        Verify.AreEqual(stockKeyFrameAnimation.Duration.TotalMilliseconds, c_MaxStockOffsetsChangeDuration);
                        stockKeyFrameAnimation.Duration = TimeSpan.FromMilliseconds(10);
                    };

                    operation = StartChangeOffsets(
                        scroller,
                        600.0,
                        400.0,
                        ScrollerViewKind.Absolute,
                        ScrollerViewChangeKind.AllowAnimation,
                        ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
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
                Rectangle rectangleScrollerChild = null;
                AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
                ScrollerOperation operation = null;
                int viewChangedCount = 0;

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollerChild = new Rectangle();
                    scroller = new Scroller();

                    SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);
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

                    scroller.ChangingZoomFactor += (sender, args) =>
                    {
                        Log.Comment("ChangingZoomFactor - ViewChangeId={0}", args.ViewChangeId);
                        Verify.IsNotNull(args.Animation);
                        ScalarKeyFrameAnimation stockKeyFrameAnimation = args.Animation as ScalarKeyFrameAnimation;
                        Verify.IsNotNull(stockKeyFrameAnimation);
                        Log.Comment("Stock duration={0} msec.", stockKeyFrameAnimation.Duration.TotalMilliseconds);
                        Verify.AreEqual(stockKeyFrameAnimation.Duration.TotalMilliseconds, c_MaxStockZoomFactorChangeDuration);
                        stockKeyFrameAnimation.Duration = TimeSpan.FromMilliseconds(10);
                    };

                    operation = StartChangeZoomFactor(
                        scroller,
                        8.0f,
                        100.0f,
                        150.0f,
                        ScrollerViewKind.Absolute,
                        ScrollerViewChangeKind.AllowAnimation,
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
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent, setAsContentRoot: true);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                scroller.ViewChanged += (sender, args) =>
                {
                    Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                };

                operation = StartChangeOffsets(
                    scroller,
                    600.0,
                    400.0,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.DisableAnimation,
                    ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
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
        [TestProperty("Description", "Attempts an offsets change while there is no child.")]
        public void OffsetsChangeWithNoChild()
        {
            Scroller scroller = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                scroller = new Scroller();

                SetupDefaultUI(scroller, null /*rectangleScrollerChild*/, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                operation = StartChangeOffsets(
                    scroller,
                    600.0,
                    400.0,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.AllowAnimation,
                    ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
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
        [TestProperty("Description", "Attempts a zoomFactor change while there is no child.")]
        public void ZoomFactorChangeWithNoChild()
        {
            Scroller scroller = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                scroller = new Scroller();

                SetupDefaultUI(scroller, null /*rectangleScrollerChild*/, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                operation = StartChangeZoomFactor(
                    scroller,
                    8.0f,
                    100.0f,
                    150.0f,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.AllowAnimation,
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
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollerViewChangeOperationEvents = null;
            ScrollerOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);
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

                operations[0] = StartChangeOffsets(
                    scroller,
                    600.0,
                    400.0,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.DisableAnimation,
                    ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
                    scrollerViewChangeOperationEvents[0]);

                operations[1] = StartChangeOffsets(
                    scroller,
                    500.0,
                    300.0,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.DisableAnimation,
                    ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
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
                operations[2] = StartChangeOffsets(
                    scroller,
                    500.0,
                    300.0,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.DisableAnimation,
                    ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
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
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollerViewChangeOperationEvents = null;
            ScrollerOperation[] operations = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);
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

                operations[0] = StartChangeZoomFactor(
                    scroller,
                    8.0f,
                    150.0f,
                    120.0f,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.DisableAnimation,
                    scrollerViewChangeOperationEvents[0]);

                operations[1] = StartChangeZoomFactor(
                    scroller,
                    7.0f,
                    10.0f,
                    90.0f,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.DisableAnimation,
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
                operations[2] = StartChangeZoomFactor(
                    scroller,
                    7.0f,
                    10.0f,
                    90.0f,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.DisableAnimation,
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
        [TestProperty("Description", "Requests a non-animated offset change immediately after increasing child size.")]
        public void OffsetJumpAfterChildResizing()
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", scrollerLoadedEvent);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild.Width = c_defaultUIScrollerChildWidth + 200.0;
            });

            // Jump to absolute offsets
            ChangeOffsets(
                scroller, 
                c_defaultUIScrollerChildWidth + 200.0 - c_defaultUIScrollerWidth, 
                c_defaultVerticalOffset, 
                ScrollerViewKind.Absolute, 
                ScrollerViewChangeKind.DisableAnimation,
                ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints);
        }

        private void ChangeOffsetsBeforeLoading(bool animate)
        {
            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent, false /*setAsContentRoot*/);

                scroller.ViewChanged += (sender, args) =>
                {
                    Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                };

                operation = StartChangeOffsets(
                    scroller,
                    600.0,
                    400.0,
                    ScrollerViewKind.Absolute,
                    animate ? ScrollerViewChangeKind.AllowAnimation : ScrollerViewChangeKind.DisableAnimation,
                    ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
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
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollerViewChangeOperationEvent = new AutoResetEvent(false);
            ScrollerOperation operation = null;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent, false /*setAsContentRoot*/);

                scroller.ViewChanged += (sender, args) =>
                {
                    Log.Comment("ViewChanged - HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                        sender.HorizontalOffset, sender.VerticalOffset, sender.ZoomFactor);
                };

                operation = StartChangeZoomFactor(
                    scroller,
                    8.0f,
                    100.0f,
                    150.0f,
                    ScrollerViewKind.Absolute,
                    animate ? ScrollerViewChangeKind.AllowAnimation : ScrollerViewChangeKind.DisableAnimation,
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

        private void ChangeOffsets(
            Scroller scroller,
            double horizontalOffset,
            double verticalOffset,
            ScrollerViewKind viewKind,
            ScrollerViewChangeKind viewChangeKind,
            ScrollerViewChangeSnapPointRespect snapPointRespect,
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
                    switch (viewKind)
                    {
                        case ScrollerViewKind.Absolute:
                            expectedFinalHorizontalOffset = horizontalOffset;
                            break;
                        case ScrollerViewKind.RelativeToCurrentView:
                            expectedFinalHorizontalOffset = horizontalOffset + originalHorizontalOffset;
                            break;
                    }
                }

                if (expectedFinalVerticalOffset == null)
                {
                    switch (viewKind)
                    {
                        case ScrollerViewKind.Absolute:
                            expectedFinalVerticalOffset = verticalOffset;
                            break;
                        case ScrollerViewKind.RelativeToCurrentView:
                            expectedFinalVerticalOffset = verticalOffset + originalVerticalOffset;
                            break;
                    }
                }

                operation = StartChangeOffsets(
                    scroller,
                    horizontalOffset,
                    verticalOffset,
                    viewKind,
                    viewChangeKind,
                    snapPointRespect,
                    scrollerViewChangeOperationEvent);
            });

            WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                switch (viewKind)
                {
                    case ScrollerViewKind.Absolute:
                    case ScrollerViewKind.RelativeToCurrentView:
                        Verify.AreEqual(expectedFinalHorizontalOffset, scroller.HorizontalOffset);
                        Verify.AreEqual(expectedFinalVerticalOffset, scroller.VerticalOffset);
                        break;
                    case ScrollerViewKind.RelativeToEndOfInertiaView:
                        Verify.Fail("ScrollerViewKind.RelativeToEndOfInertiaView is not supported by ScrollerTests::ChangeOffsets");
                        break;
                }
                Verify.AreEqual(originalZoomFactor, scroller.ZoomFactor);
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
            });
        }

        private void ChangeOffsets(
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

                operation = StartChangeOffsets(
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

        private ScrollerOperation StartChangeOffsets(
            Scroller scroller,
            double horizontalOffset,
            double verticalOffset,
            ScrollerViewKind viewKind,
            ScrollerViewChangeKind viewChangeKind,
            ScrollerViewChangeSnapPointRespect snapPointRespect,
            AutoResetEvent scrollerViewChangeOperationEvent)
        {
            Log.Comment("ChangeOffsets - horizontalOffset={0}, verticalOffset={1}, viewKind={2}, viewChangeKind={3}",
                horizontalOffset, verticalOffset, viewKind, viewChangeKind);

            ScrollerOperation operation = new ScrollerOperation();

            operation.Id = scroller.ChangeOffsets(
                new ScrollerChangeOffsetsOptions(horizontalOffset, verticalOffset, viewKind, viewChangeKind, snapPointRespect));

            if (operation.Id == -1)
            {
                scrollerViewChangeOperationEvent.Set();
            }
            else
            {
                scroller.ViewChangeCompleted += (Scroller sender, ScrollerViewChangeCompletedEventArgs args) =>
                {
                    if (args.ViewChangeId == operation.Id)
                    {
                        Log.Comment("ViewChangeCompleted: ChangeOffsets ViewChangeId=" + args.ViewChangeId + ", Result=" + args.Result);
                        operation.Result = args.Result;

                        Log.Comment("Setting completion event");
                        scrollerViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollerOperation StartChangeOffsets(
            Scroller scroller,
            float horizontalVelocity,
            float verticalVelocity,
            float? horizontalInertiaDecayRate,
            float? verticalInertiaDecayRate,
            AutoResetEvent scrollerViewChangeOperationEvent)
        {
            Log.Comment("ChangeOffsets - horizontalVelocity={0}, verticalVelocity={1}, horizontalInertiaDecayRate={2}, verticalInertiaDecayRate={3}",
                horizontalVelocity, verticalVelocity, horizontalInertiaDecayRate, verticalInertiaDecayRate);

            Vector2? inertiaDecayRate = null;

            if (horizontalInertiaDecayRate != null && verticalInertiaDecayRate != null)
            {
                inertiaDecayRate = new Vector2((float)horizontalInertiaDecayRate, (float)verticalInertiaDecayRate);
            }

            ScrollerOperation operation = new ScrollerOperation();

            operation.Id = scroller.ChangeOffsetsWithAdditionalVelocity(
                new ScrollerChangeOffsetsWithAdditionalVelocityOptions(
                    new Vector2(horizontalVelocity, verticalVelocity),
                    inertiaDecayRate));

            if (operation.Id == -1)
            {
                scrollerViewChangeOperationEvent.Set();
            }
            else
            {
                scroller.ViewChangeCompleted += (Scroller sender, ScrollerViewChangeCompletedEventArgs args) =>
                {
                    if (args.ViewChangeId == operation.Id)
                    {
                        Log.Comment("ViewChangeCompleted: ChangeOffsets ViewChangeId=" + args.ViewChangeId + ", Result=" + args.Result);
                        operation.Result = args.Result;

                        Log.Comment("Setting completion event");
                        scrollerViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private void ChangeZoomFactor(
            Scroller scroller,
            float zoomFactor,
            float centerPointX,
            float centerPointY,
            ScrollerViewKind viewKind,
            ScrollerViewChangeKind viewChangeKind,
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

                operation = StartChangeZoomFactor(
                    scroller,
                    zoomFactor,
                    centerPointX,
                    centerPointY,
                    viewKind,
                    viewChangeKind,
                    scrollerViewChangeOperationEvent);
            });

            WaitForEvent("Waiting for view change completion", scrollerViewChangeOperationEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Final HorizontalOffset={0}, VerticalOffset={1}, ZoomFactor={2}",
                    scroller.HorizontalOffset, scroller.VerticalOffset, scroller.ZoomFactor);

                switch (viewKind)
                {
                    case ScrollerViewKind.Absolute:
                        Verify.AreEqual(zoomFactor, scroller.ZoomFactor);
                        break;
                    case ScrollerViewKind.RelativeToCurrentView:
                        Verify.AreEqual(zoomFactor + originalZoomFactor, scroller.ZoomFactor);
                        break;
                    case ScrollerViewKind.RelativeToEndOfInertiaView:
                        Verify.Fail("ScrollerViewKind.RelativeToEndOfInertiaView is not supported by ScrollerTests::ChangeZoomFactor");
                        break;
                }
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
            });
        }

        private void ChangeZoomFactor(
            Scroller scroller,
            float additionalVelocity,
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

                operation = StartChangeZoomFactor(
                    scroller,
                    additionalVelocity,
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

                if (additionalVelocity > 0)
                    Verify.IsTrue(originalZoomFactor < scroller.ZoomFactor);
                else if (additionalVelocity < 0)
                    Verify.IsTrue(originalZoomFactor > scroller.ZoomFactor);
                else
                    Verify.IsTrue(originalZoomFactor == scroller.ZoomFactor);
                Verify.AreEqual(operation.Result, ScrollerViewChangeResult.Completed);
            });
        }

        private ScrollerOperation StartChangeZoomFactor(
            Scroller scroller,
            float zoomFactor,
            float centerPointX,
            float centerPointY,
            ScrollerViewKind viewKind,
            ScrollerViewChangeKind viewChangeKind,
            AutoResetEvent scrollerViewChangeOperationEvent)
        {
            Log.Comment("ChangeZoomFactor - zoomFactor={0}, centerPoint=({1},{2}), viewKind={3}, viewChangeKind={4}",
                zoomFactor, centerPointX, centerPointY, viewKind, viewChangeKind);

            ScrollerOperation operation = new ScrollerOperation();

            operation.Id = scroller.ChangeZoomFactor(
                new ScrollerChangeZoomFactorOptions(zoomFactor, viewKind, new Vector2(centerPointX, centerPointY), viewChangeKind, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));

            if (operation.Id == -1)
            {
                scrollerViewChangeOperationEvent.Set();
            }
            else
            {
                scroller.ViewChangeCompleted += (Scroller sender, ScrollerViewChangeCompletedEventArgs args) =>
                {
                    if (args.ViewChangeId == operation.Id)
                    {
                        Log.Comment("ViewChangeCompleted: ChangeZoomFactor ViewChangeId=" + args.ViewChangeId + ", Result=" + args.Result);
                        operation.Result = args.Result;

                        Log.Comment("Setting completion event");
                        scrollerViewChangeOperationEvent.Set();
                    }
                };
            }

            return operation;
        }

        private ScrollerOperation StartChangeZoomFactor(
            Scroller scroller,
            float additionalVelocity,
            float? inertiaDecayRate,
            float centerPointX,
            float centerPointY,
            AutoResetEvent scrollerViewChangeOperationEvent)
        {
            Log.Comment("ChangeZoomFactor - additionalVelocity={0}, inertiaDecayRate={1}, centerPoint=({2},{3})",
                additionalVelocity, inertiaDecayRate, centerPointX, centerPointY);

            ScrollerOperation operation = new ScrollerOperation();

            operation.Id = scroller.ChangeZoomFactorWithAdditionalVelocity(
                new ScrollerChangeZoomFactorWithAdditionalVelocityOptions(additionalVelocity, inertiaDecayRate, new Vector2(centerPointX, centerPointY)));

            if (operation.Id == -1)
            {
                scrollerViewChangeOperationEvent.Set();
            }
            else
            {
                scroller.ViewChangeCompleted += (Scroller sender, ScrollerViewChangeCompletedEventArgs args) =>
                {
                    if (args.ViewChangeId == operation.Id)
                    {
                        Log.Comment("ViewChangeCompleted: ChangeZoomFactor ViewChangeId=" + args.ViewChangeId + ", Result=" + args.Result);
                        operation.Result = args.Result;

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
            Rectangle rectangleScrollerChild = null;
            AutoResetEvent scrollerLoadedEvent = new AutoResetEvent(false);
            AutoResetEvent[] scrollerViewChangeOperationEvents = null;
            ScrollerOperation[] operations = null;
            int viewChangedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();

                SetupDefaultUI(scroller, rectangleScrollerChild, scrollerLoadedEvent);
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
                            operations[1] = StartChangeOffsets(
                                scroller,
                                500.0,
                                300.0,
                                ScrollerViewKind.Absolute,
                                ScrollerViewChangeKind.AllowAnimation,
                                ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
                                scrollerViewChangeOperationEvents[1]);
                        }
                        else
                        {
                            operations[1] = StartChangeZoomFactor(
                                scroller,
                                7.0f,
                                70.0f,
                                50.0f,
                                ScrollerViewKind.Absolute,
                                ScrollerViewChangeKind.AllowAnimation,
                                scrollerViewChangeOperationEvents[1]);
                        }
                    }
                };

                if (changeOffsetsFirst)
                {
                    operations[0] = StartChangeOffsets(
                        scroller,
                        600.0,
                        400.0,
                        ScrollerViewKind.Absolute,
                        ScrollerViewChangeKind.AllowAnimation,
                        ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
                        scrollerViewChangeOperationEvents[0]);
                }
                else
                {
                    operations[0] = StartChangeZoomFactor(
                        scroller,
                        8.0f,
                        100.0f,
                        150.0f,
                        ScrollerViewKind.Absolute,
                        ScrollerViewChangeKind.AllowAnimation,
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
            if (!eventWaitHandle.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration)))
            {
                throw new Exception("Timeout expiration in WaitForEvent.");
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
