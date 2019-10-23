// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System.Numerics;
using System.Threading;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using AnimationMode = Microsoft.UI.Xaml.Controls.ScrollingAnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode;
using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollingPresenterTests
    {
        [TestMethod]
        [TestProperty("Description", "Sets the ScrollingPresenter.HorizontalScrollController and ScrollingPresenter.VerticalScrollController properties.")]
        public void SettingScrollControllerProperties()
        {
            ScrollingPresenter scrollingPresenter = null;
            CompositionScrollController horizontalScrollController = null;
            CompositionScrollController verticalScrollController = null;

            RunOnUIThread.Execute(() =>
            {
                scrollingPresenter = new ScrollingPresenter();
                Verify.IsNotNull(scrollingPresenter);

                horizontalScrollController = new CompositionScrollController();
                Verify.IsNotNull(horizontalScrollController);

                verticalScrollController = new CompositionScrollController();
                Verify.IsNotNull(verticalScrollController);

                Log.Comment("Setting ScrollingPresenter.HorizontalScrollController and ScrollingPresenter.VerticalScrollController properties.");
                scrollingPresenter.HorizontalScrollController = horizontalScrollController;
                scrollingPresenter.VerticalScrollController = verticalScrollController;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verifying ScrollingPresenter properties.");
                Verify.AreEqual(scrollingPresenter.HorizontalScrollController, horizontalScrollController);
                Verify.AreEqual(scrollingPresenter.VerticalScrollController, verticalScrollController);

                Verify.IsTrue(horizontalScrollController.AreInteractionsAllowed);
                Verify.IsTrue(horizontalScrollController.AreScrollerInteractionsAllowed);
                Verify.IsFalse(horizontalScrollController.IsInteracting);
                Verify.IsNull(horizontalScrollController.InteractionVisual);
                Verify.IsTrue(verticalScrollController.AreInteractionsAllowed);
                Verify.IsTrue(verticalScrollController.AreScrollerInteractionsAllowed);
                Verify.IsFalse(verticalScrollController.IsInteracting);
                Verify.IsNull(verticalScrollController.InteractionVisual);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Change ScrollingPresenter view while scroll controllers are attached.")]
        public void ChangeOffsetsWhileScrollControllersAreAttached()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            CompositionScrollController horizontalScrollController = null;
            CompositionScrollController verticalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();
                horizontalScrollController = new CompositionScrollController();
                verticalScrollController = new CompositionScrollController();

                horizontalScrollController.Orientation = Orientation.Horizontal;

                horizontalScrollController.LogMessage += (CompositionScrollController sender, string args) =>
                {
                    Log.Comment(args);
                };

                verticalScrollController.LogMessage += (CompositionScrollController sender, string args) =>
                {
                    Log.Comment(args);
                };

                scrollingPresenter.HorizontalScrollController = horizontalScrollController;
                scrollingPresenter.VerticalScrollController = verticalScrollController;

                SetupUIWithScrollControllers(
                    scrollingPresenter,
                    rectangleScrollingPresenterContent,
                    horizontalScrollController,
                    verticalScrollController,
                    loadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", loadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("HorizontalScrollController size={0}, {1}", horizontalScrollController.ActualWidth, horizontalScrollController.ActualHeight);
                Log.Comment("VerticalScrollController size={0}, {1}", verticalScrollController.ActualWidth, verticalScrollController.ActualHeight);
            });

            IdleSynchronizer.Wait();

            Log.Comment("Jump to offsets");
            ScrollTo(
                scrollingPresenter,
                (c_defaultUIScrollingPresenterContentWidth - c_defaultUIScrollingPresenterWidth) / 2.0,
                (c_defaultUIScrollingPresenterContentHeight - c_defaultUIScrollingPresenterHeight) / 2.0,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: true,
                isAnimationsEnabledOverride: null, 
                expectedFinalHorizontalOffset: (c_defaultUIScrollingPresenterContentWidth - c_defaultUIScrollingPresenterWidth) / 2.0, 
                expectedFinalVerticalOffset: (c_defaultUIScrollingPresenterContentHeight - c_defaultUIScrollingPresenterHeight) / 2.0);

            Log.Comment("Animate to offsets");
            ScrollTo(
                scrollingPresenter,
                (c_defaultUIScrollingPresenterContentWidth - c_defaultUIScrollingPresenterWidth) / 4.0,
                (c_defaultUIScrollingPresenterContentHeight - c_defaultUIScrollingPresenterHeight) / 4.0,
                AnimationMode.Enabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false,
                isAnimationsEnabledOverride: null,
                expectedFinalHorizontalOffset: (c_defaultUIScrollingPresenterContentWidth - c_defaultUIScrollingPresenterWidth) / 4.0,
                expectedFinalVerticalOffset: (c_defaultUIScrollingPresenterContentHeight - c_defaultUIScrollingPresenterHeight) / 4.0);

            Log.Comment("Jump to zoomFactor 2.0");
            ZoomTo(
                scrollingPresenter,
                2.0f,
                0.0f,
                0.0f,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false);

            Log.Comment("Animate to zoomFactor 1.5");
            ZoomTo(
                scrollingPresenter,
                1.5f,
                0.0f,
                0.0f,
                AnimationMode.Enabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false);
        }

        [TestMethod]
        [TestProperty("Description", "Change ScrollingPresenter view via attached scroll controllers.")]
        public void ChangeOffsetsWithAttachedScrollControllers()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            CompositionScrollController horizontalScrollController = null;
            CompositionScrollController verticalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollCompletedEvent = new AutoResetEvent(false);
            int hOffsetChangeId = -1;
            int vOffsetChangeId = -1;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();
                horizontalScrollController = new CompositionScrollController();
                verticalScrollController = new CompositionScrollController();

                horizontalScrollController.Orientation = Orientation.Horizontal;

                horizontalScrollController.LogMessage += (CompositionScrollController sender, string args) =>
                {
                    Log.Comment(args);
                };

                verticalScrollController.LogMessage += (CompositionScrollController sender, string args) =>
                {
                    Log.Comment(args);
                };

                scrollingPresenter.HorizontalScrollController = horizontalScrollController;
                scrollingPresenter.VerticalScrollController = verticalScrollController;

                SetupUIWithScrollControllers(
                    scrollingPresenter,
                    rectangleScrollingPresenterContent,
                    horizontalScrollController,
                    verticalScrollController,
                    loadedEvent);

                horizontalScrollController.OffsetChangeCompleted += (CompositionScrollController sender, CompositionScrollControllerOffsetChangeCompletedEventArgs args) =>
                {
                    Log.Comment("ChangeOffset completed (horizontal). OffsetChangeId=" + args.OffsetChangeId);

                    Log.Comment("Setting completion event");
                    scrollCompletedEvent.Set();
                };

                verticalScrollController.OffsetChangeCompleted += (CompositionScrollController sender, CompositionScrollControllerOffsetChangeCompletedEventArgs args) =>
                {
                    Log.Comment("ChangeOffset completed (vertical). OffsetChangeId=" + args.OffsetChangeId);
                };
            });

            WaitForEvent("Waiting for Loaded event", loadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("HorizontalScrollController size={0}, {1}", horizontalScrollController.ActualWidth, horizontalScrollController.ActualHeight);
                Log.Comment("VerticalScrollController size={0}, {1}", verticalScrollController.ActualWidth, verticalScrollController.ActualHeight);
            });

            Log.Comment("Jump to zoomFactor 0.75");
            ZoomTo(
                scrollingPresenter,
                0.75f,
                0.0f,
                0.0f,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Jumping to horizontal offset");
                hOffsetChangeId = horizontalScrollController.ScrollTo(
                    (c_defaultUIScrollingPresenterContentWidth * 0.75 - c_defaultUIScrollingPresenterWidth) / 4.0,
                    AnimationMode.Disabled);

                Log.Comment("Jumping to vertical offset");
                vOffsetChangeId = verticalScrollController.ScrollTo(
                    (c_defaultUIScrollingPresenterContentHeight * 0.75 - c_defaultUIScrollingPresenterHeight) / 4.0,
                    AnimationMode.Disabled);

                Verify.AreEqual(hOffsetChangeId, vOffsetChangeId);
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual((c_defaultUIScrollingPresenterContentWidth * 0.75 - c_defaultUIScrollingPresenterWidth) / 4.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual((c_defaultUIScrollingPresenterContentHeight * 0.75 - c_defaultUIScrollingPresenterHeight) / 4.0, scrollingPresenter.VerticalOffset);

                Log.Comment("Animating to horizontal offset");
                hOffsetChangeId = horizontalScrollController.ScrollTo(
                    (c_defaultUIScrollingPresenterContentWidth * 0.75 - c_defaultUIScrollingPresenterWidth) / 2.0,
                    AnimationMode.Enabled);

                Log.Comment("Animating to vertical offset");
                vOffsetChangeId = verticalScrollController.ScrollTo(
                    (c_defaultUIScrollingPresenterContentHeight * 0.75 - c_defaultUIScrollingPresenterHeight) / 2.0,
                    AnimationMode.Enabled);

                Verify.AreEqual(hOffsetChangeId, vOffsetChangeId);

                scrollCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual((c_defaultUIScrollingPresenterContentWidth * 0.75 - c_defaultUIScrollingPresenterWidth) / 2.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual((c_defaultUIScrollingPresenterContentHeight * 0.75 - c_defaultUIScrollingPresenterHeight) / 2.0, scrollingPresenter.VerticalOffset);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Change ScrollingPresenter view with additional velocity via attached scroll controllers.")]
        public void ChangeOffsetsWithAdditionalVelocityAndAttachedScrollControllers()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            CompositionScrollController horizontalScrollController = null;
            CompositionScrollController verticalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollCompletedEvent = new AutoResetEvent(false);
            int hOffsetChangeId = -1;
            int vOffsetChangeId = -1;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();
                horizontalScrollController = new CompositionScrollController();
                verticalScrollController = new CompositionScrollController();

                horizontalScrollController.Orientation = Orientation.Horizontal;

                horizontalScrollController.LogMessage += (CompositionScrollController sender, string args) =>
                {
                    Log.Comment(args);
                };

                verticalScrollController.LogMessage += (CompositionScrollController sender, string args) =>
                {
                    Log.Comment(args);
                };

                scrollingPresenter.HorizontalScrollController = horizontalScrollController;
                scrollingPresenter.VerticalScrollController = verticalScrollController;

                SetupUIWithScrollControllers(
                    scrollingPresenter,
                    rectangleScrollingPresenterContent,
                    horizontalScrollController,
                    verticalScrollController,
                    loadedEvent);

                horizontalScrollController.OffsetChangeCompleted += (CompositionScrollController sender, CompositionScrollControllerOffsetChangeCompletedEventArgs args) =>
                {
                    Log.Comment("ChangeOffset completed. OffsetChangeId=" + args.OffsetChangeId);

                    Log.Comment("Setting completion event");
                    scrollCompletedEvent.Set();
                };
            });

            WaitForEvent("Waiting for Loaded event", loadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("HorizontalScrollController size={0}, {1}", horizontalScrollController.ActualWidth, horizontalScrollController.ActualHeight);
                Log.Comment("VerticalScrollController size={0}, {1}", verticalScrollController.ActualWidth, verticalScrollController.ActualHeight);
            });

            Log.Comment("Jump to zoomFactor 0.75");
            ZoomTo(
                scrollingPresenter,
                0.75f,
                0.0f,
                0.0f,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding velocity to horizontal offset, with default inertia decay rate");
                hOffsetChangeId = horizontalScrollController.ScrollFrom(
                    100.0f /*offsetVelocity*/, null /*inertiaDecayRate*/);

                Log.Comment("Adding velocity to vertical offset, with default inertia decay rate");
                vOffsetChangeId = verticalScrollController.ScrollFrom(
                    100.0f /*offsetVelocity*/, null /*inertiaDecayRate*/);

                Verify.AreEqual(hOffsetChangeId, vOffsetChangeId);
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scrollingPresenter.HorizontalOffset={0}", scrollingPresenter.HorizontalOffset);
                Log.Comment("scrollingPresenter.VerticalOffset={0}", scrollingPresenter.VerticalOffset);

                Verify.IsTrue(scrollingPresenter.HorizontalOffset > 20.0);
                Verify.IsTrue(scrollingPresenter.VerticalOffset > 20.0);

                Log.Comment("Adding negative velocity to horizontal offset, with custom inertia decay rate");
                hOffsetChangeId = horizontalScrollController.ScrollFrom(
                    -50.0f /*offsetVelocity*/, 0.9f /*inertiaDecayRate*/);

                Log.Comment("Adding negative velocity to vertical offset, with custom inertia decay rate");
                vOffsetChangeId = verticalScrollController.ScrollFrom(
                    -50.0f /*offsetVelocity*/, 0.9f /*inertiaDecayRate*/);

                Verify.AreEqual(hOffsetChangeId, vOffsetChangeId);

                scrollCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scrollingPresenter.HorizontalOffset={0}", scrollingPresenter.HorizontalOffset);
                Log.Comment("scrollingPresenter.VerticalOffset={0}", scrollingPresenter.VerticalOffset);

                Verify.IsTrue(scrollingPresenter.HorizontalOffset < 20.0);
                Verify.IsTrue(scrollingPresenter.VerticalOffset < 20.0);

                Log.Comment("Adding velocity to horizontal offset, with no inertia decay rate");
                hOffsetChangeId = horizontalScrollController.ScrollFrom(
                    200.0f /*offsetVelocity*/, 0.0f /*inertiaDecayRate*/);

                Log.Comment("Adding velocity to vertical offset, with no inertia decay rate");
                vOffsetChangeId = verticalScrollController.ScrollFrom(
                    200.0f /*offsetVelocity*/, 0.0f /*inertiaDecayRate*/);

                Verify.AreEqual(hOffsetChangeId, vOffsetChangeId);

                scrollCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scrollingPresenter.HorizontalOffset={0}", scrollingPresenter.HorizontalOffset);
                Log.Comment("scrollingPresenter.VerticalOffset={0}", scrollingPresenter.VerticalOffset);

                Verify.AreEqual(600.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(250.0, scrollingPresenter.VerticalOffset);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Change ScrollingPresenter view while a bi-directional scroll controller is attached.")]
        public void ChangeOffsetsWhileBiDirectionalScrollControllerIsAttached()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            BiDirectionalScrollController biDirectionalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();
                biDirectionalScrollController = new BiDirectionalScrollController();

                biDirectionalScrollController.LogMessage += (BiDirectionalScrollController sender, string args) =>
                {
                    Log.Comment(args);
                };

                scrollingPresenter.HorizontalScrollController = biDirectionalScrollController.HorizontalScrollController;
                scrollingPresenter.VerticalScrollController = biDirectionalScrollController.VerticalScrollController;

                SetupUIWithBiDirectionalScrollController(
                    scrollingPresenter,
                    rectangleScrollingPresenterContent,
                    biDirectionalScrollController,
                    loadedEvent);
            });

            WaitForEvent("Waiting for Loaded event", loadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("BiDirectionalScrollController size={0}, {1}", biDirectionalScrollController.ActualWidth, biDirectionalScrollController.ActualHeight);
            });

            IdleSynchronizer.Wait();

            Log.Comment("Jump to offsets");
            ScrollTo(
                scrollingPresenter,
                (c_defaultUIScrollingPresenterContentWidth - c_defaultUIScrollingPresenterWidth) / 2.0,
                (c_defaultUIScrollingPresenterContentHeight - c_defaultUIScrollingPresenterHeight) / 2.0,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: true,
                isAnimationsEnabledOverride: null,
                expectedFinalHorizontalOffset: (c_defaultUIScrollingPresenterContentWidth - c_defaultUIScrollingPresenterWidth) / 2.0,
                expectedFinalVerticalOffset: (c_defaultUIScrollingPresenterContentHeight - c_defaultUIScrollingPresenterHeight) / 2.0);

            Log.Comment("Animate to offsets");
            ScrollTo(
                scrollingPresenter,
                (c_defaultUIScrollingPresenterContentWidth - c_defaultUIScrollingPresenterWidth) / 4.0,
                (c_defaultUIScrollingPresenterContentHeight - c_defaultUIScrollingPresenterHeight) / 4.0,
                AnimationMode.Enabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false,
                isAnimationsEnabledOverride: null,
                expectedFinalHorizontalOffset: (c_defaultUIScrollingPresenterContentWidth - c_defaultUIScrollingPresenterWidth) / 4.0,
                expectedFinalVerticalOffset: (c_defaultUIScrollingPresenterContentHeight - c_defaultUIScrollingPresenterHeight) / 4.0);

            Log.Comment("Jump to zoomFactor 2.0");
            ZoomTo(
                scrollingPresenter,
                2.0f,
                0.0f,
                0.0f,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false);

            Log.Comment("Animate to zoomFactor 1.5");
            ZoomTo(
                scrollingPresenter,
                1.5f,
                0.0f,
                0.0f,
                AnimationMode.Enabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false);
        }

        [TestMethod]
        [TestProperty("Description", "Change ScrollingPresenter view via attached bi-directional scroll controller.")]
        public void ChangeOffsetsWithAttachedBiDirectionalScrollController()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            BiDirectionalScrollController biDirectionalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollCompletedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();
                biDirectionalScrollController = new BiDirectionalScrollController();

                biDirectionalScrollController.LogMessage += (BiDirectionalScrollController sender, string args) =>
                {
                    Log.Comment(args);
                };

                scrollingPresenter.HorizontalScrollController = biDirectionalScrollController.HorizontalScrollController;
                scrollingPresenter.VerticalScrollController = biDirectionalScrollController.VerticalScrollController;

                SetupUIWithBiDirectionalScrollController(
                    scrollingPresenter,
                    rectangleScrollingPresenterContent,
                    biDirectionalScrollController,
                    loadedEvent);

                biDirectionalScrollController.ScrollCompleted += (BiDirectionalScrollController sender, BiDirectionalScrollControllerScrollCompletedEventArgs args) =>
                {
                    Log.Comment("ChangeOffset completed. OffsetsChangeId=" + args.OffsetsChangeId);

                    Log.Comment("Setting completion event");
                    scrollCompletedEvent.Set();
                };
            });

            WaitForEvent("Waiting for Loaded event", loadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("BiDirectionalScrollController size={0}, {1}", biDirectionalScrollController.ActualWidth, biDirectionalScrollController.ActualHeight);
            });

            Log.Comment("Jump to zoomFactor 0.75");
            ZoomTo(
                scrollingPresenter,
                0.75f,
                0.0f,
                0.0f,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Jumping to offsets");
                biDirectionalScrollController.ScrollTo(
                    (c_defaultUIScrollingPresenterContentWidth * 0.75 - c_defaultUIScrollingPresenterWidth) / 4.0,
                    (c_defaultUIScrollingPresenterContentHeight * 0.75 - c_defaultUIScrollingPresenterHeight) / 4.0,
                    AnimationMode.Disabled);
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual((c_defaultUIScrollingPresenterContentWidth * 0.75 - c_defaultUIScrollingPresenterWidth) / 4.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual((c_defaultUIScrollingPresenterContentHeight * 0.75 - c_defaultUIScrollingPresenterHeight) / 4.0, scrollingPresenter.VerticalOffset);

                Log.Comment("Animating to offsets");
                biDirectionalScrollController.ScrollTo(
                    (c_defaultUIScrollingPresenterContentWidth * 0.75 - c_defaultUIScrollingPresenterWidth) / 2.0,
                    (c_defaultUIScrollingPresenterContentHeight * 0.75 - c_defaultUIScrollingPresenterHeight) / 2.0,
                    AnimationMode.Enabled);

                scrollCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual((c_defaultUIScrollingPresenterContentWidth * 0.75 - c_defaultUIScrollingPresenterWidth) / 2.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual((c_defaultUIScrollingPresenterContentHeight * 0.75 - c_defaultUIScrollingPresenterHeight) / 2.0, scrollingPresenter.VerticalOffset);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Change ScrollingPresenter view with additional velocity via attached bi-directional scroll controller.")]
        public void ChangeOffsetsWithAdditionalVelocityAndAttachedBiDirectionalScrollController()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            ScrollingPresenter scrollingPresenter = null;
            Rectangle rectangleScrollingPresenterContent = null;
            BiDirectionalScrollController biDirectionalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollCompletedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollingPresenterContent = new Rectangle();
                scrollingPresenter = new ScrollingPresenter();
                biDirectionalScrollController = new BiDirectionalScrollController();

                biDirectionalScrollController.LogMessage += (BiDirectionalScrollController sender, string args) =>
                {
                    Log.Comment(args);
                };

                scrollingPresenter.HorizontalScrollController = biDirectionalScrollController.HorizontalScrollController;
                scrollingPresenter.VerticalScrollController = biDirectionalScrollController.VerticalScrollController;

                SetupUIWithBiDirectionalScrollController(
                    scrollingPresenter,
                    rectangleScrollingPresenterContent,
                    biDirectionalScrollController,
                    loadedEvent);

                biDirectionalScrollController.ScrollCompleted += (BiDirectionalScrollController sender, BiDirectionalScrollControllerScrollCompletedEventArgs args) =>
                {
                    Log.Comment("ScrollFrom completed. OffsetsChangeId=" + args.OffsetsChangeId);

                    Log.Comment("Setting completion event");
                    scrollCompletedEvent.Set();
                };
            });

            WaitForEvent("Waiting for Loaded event", loadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("BiDirectionalScrollController size={0}, {1}", biDirectionalScrollController.ActualWidth, biDirectionalScrollController.ActualHeight);
            });

            Log.Comment("Jump to zoomFactor 0.75");
            ZoomTo(
                scrollingPresenter,
                0.75f,
                0.0f,
                0.0f,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding velocity to offsets, with default inertia decay rates");
                biDirectionalScrollController.ScrollFrom(
                    new Vector2(100.0f) /*offsetsVelocity*/, null /*inertiaDecayRate*/);
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scrollingPresenter.HorizontalOffset={0}", scrollingPresenter.HorizontalOffset);
                Log.Comment("scrollingPresenter.VerticalOffset={0}", scrollingPresenter.VerticalOffset);

                Verify.IsTrue(scrollingPresenter.HorizontalOffset > 20.0);
                Verify.IsTrue(scrollingPresenter.VerticalOffset > 20.0);

                Log.Comment("Adding negative velocity to offsets, with custom inertia decay rates");
                biDirectionalScrollController.ScrollFrom(
                    new Vector2(-50.0f) /*offsetsVelocity*/, new Vector2(0.9f) /*inertiaDecayRate*/);

                scrollCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scrollingPresenter.HorizontalOffset={0}", scrollingPresenter.HorizontalOffset);
                Log.Comment("scrollingPresenter.VerticalOffset={0}", scrollingPresenter.VerticalOffset);

                Verify.IsTrue(scrollingPresenter.HorizontalOffset < 20.0);
                Verify.IsTrue(scrollingPresenter.VerticalOffset < 20.0);

                Log.Comment("Adding velocity to offsets, with no inertia decay rates");
                biDirectionalScrollController.ScrollFrom(
                    new Vector2(200.0f) /*offsetsVelocity*/, new Vector2(0.0f) /*inertiaDecayRate*/);

                scrollCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scrollingPresenter.HorizontalOffset={0}", scrollingPresenter.HorizontalOffset);
                Log.Comment("scrollingPresenter.VerticalOffset={0}", scrollingPresenter.VerticalOffset);

                Verify.AreEqual(600.0, scrollingPresenter.HorizontalOffset);
                Verify.AreEqual(250.0, scrollingPresenter.VerticalOffset);
            });
        }

        private void SetupUIWithScrollControllers(
            ScrollingPresenter scrollingPresenter,
            Rectangle rectangleScrollingPresenterContent,
            CompositionScrollController horizontalScrollController,
            CompositionScrollController verticalScrollController,
            AutoResetEvent loadedEvent)
        {
            Log.Comment("Setting up UI with ScrollingPresenter and scroll controlllers" + (rectangleScrollingPresenterContent == null ? "" : " and Rectangle"));

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            if (rectangleScrollingPresenterContent != null)
            {
                rectangleScrollingPresenterContent.Width = c_defaultUIScrollingPresenterContentWidth;
                rectangleScrollingPresenterContent.Height = c_defaultUIScrollingPresenterContentHeight;
                rectangleScrollingPresenterContent.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scrollingPresenter);
            scrollingPresenter.Width = c_defaultUIScrollingPresenterWidth;
            scrollingPresenter.Height = c_defaultUIScrollingPresenterHeight;
            if (rectangleScrollingPresenterContent != null)
            {
                scrollingPresenter.Content = rectangleScrollingPresenterContent;
            }

            horizontalScrollController.Width = c_defaultUIScrollingPresenterWidth;
            horizontalScrollController.HorizontalAlignment = HorizontalAlignment.Left;

            StackPanel horizontalStackPanel = new StackPanel();
            horizontalStackPanel.Orientation = Orientation.Horizontal;
            horizontalStackPanel.Children.Add(scrollingPresenter);
            horizontalStackPanel.Children.Add(verticalScrollController);

            StackPanel verticalStackPanel = new StackPanel();
            verticalStackPanel.Children.Add(horizontalStackPanel);
            verticalStackPanel.Children.Add(horizontalScrollController);
            verticalStackPanel.Width = c_defaultUIScrollingPresenterWidth + c_defaultUIScrollControllerThickness;

            if (loadedEvent != null)
            {
                verticalStackPanel.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Loaded event handler");
                    loadedEvent.Set();
                };
            }

            Log.Comment("Setting window content");
            MUXControlsTestApp.App.TestContentRoot = verticalStackPanel;
        }

        private void SetupUIWithBiDirectionalScrollController(
            ScrollingPresenter scrollingPresenter,
            Rectangle rectangleScrollingPresenterContent,
            BiDirectionalScrollController biDirectionalScrollController,
            AutoResetEvent loadedEvent)
        {
            Log.Comment("Setting up UI with ScrollingPresenter and bi-directional scroll controller" + (rectangleScrollingPresenterContent == null ? "" : " and Rectangle"));

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            if (rectangleScrollingPresenterContent != null)
            {
                rectangleScrollingPresenterContent.Width = c_defaultUIScrollingPresenterContentWidth;
                rectangleScrollingPresenterContent.Height = c_defaultUIScrollingPresenterContentHeight;
                rectangleScrollingPresenterContent.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scrollingPresenter);
            scrollingPresenter.Width = c_defaultUIScrollingPresenterWidth;
            scrollingPresenter.Height = c_defaultUIScrollingPresenterHeight;
            if (rectangleScrollingPresenterContent != null)
            {
                scrollingPresenter.Content = rectangleScrollingPresenterContent;
            }

            biDirectionalScrollController.Width = c_defaultUIScrollingPresenterHeight;
            biDirectionalScrollController.Height = c_defaultUIScrollingPresenterHeight;

            StackPanel horizontalStackPanel = new StackPanel();
            horizontalStackPanel.Orientation = Orientation.Horizontal;
            horizontalStackPanel.Children.Add(scrollingPresenter);
            horizontalStackPanel.Children.Add(biDirectionalScrollController);

            if (loadedEvent != null)
            {
                horizontalStackPanel.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Loaded event handler");
                    loadedEvent.Set();
                };
            }

            Log.Comment("Setting window content");
            MUXControlsTestApp.App.TestContentRoot = horizontalStackPanel;
        }
    }
}