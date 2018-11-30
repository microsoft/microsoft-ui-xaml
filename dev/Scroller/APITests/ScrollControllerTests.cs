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

#if !BUILD_WINDOWS
using ScrollerViewKind = Microsoft.UI.Xaml.Controls.ScrollerViewKind;
using ScrollerViewChangeKind = Microsoft.UI.Xaml.Controls.ScrollerViewChangeKind;
using Scroller = Microsoft.UI.Xaml.Controls.Scroller;
using ScrollerChangeOffsetsOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeOffsetsOptions;
using ScrollerChangeOffsetsWithAdditionalVelocityOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeOffsetsWithAdditionalVelocityOptions;
using ScrollerViewChangeSnapPointRespect = Microsoft.UI.Xaml.Controls.ScrollerViewChangeSnapPointRespect;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollerTests
    {
        [TestMethod]
        [TestProperty("Description", "Sets the Scroller.HorizontalScrollController and Scroller.VerticalScrollController properties.")]
        public void SettingScrollControllerProperties()
        {
            Scroller scroller = null;
            CompositionScrollController horizontalScrollController = null;
            CompositionScrollController verticalScrollController = null;

            RunOnUIThread.Execute(() =>
            {
                scroller = new Scroller();
                Verify.IsNotNull(scroller);

                horizontalScrollController = new CompositionScrollController();
                Verify.IsNotNull(horizontalScrollController);

                verticalScrollController = new CompositionScrollController();
                Verify.IsNotNull(verticalScrollController);

                Log.Comment("Setting Scroller.HorizontalScrollController and Scroller.VerticalScrollController properties.");
                scroller.HorizontalScrollController = horizontalScrollController;
                scroller.VerticalScrollController = verticalScrollController;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verifying Scroller properties.");
                Verify.AreEqual(scroller.HorizontalScrollController, horizontalScrollController);
                Verify.AreEqual(scroller.VerticalScrollController, verticalScrollController);

                Verify.IsTrue(horizontalScrollController.AreScrollerInteractionsAllowed);
                Verify.IsFalse(horizontalScrollController.IsInteracting);
                Verify.IsNull(horizontalScrollController.InteractionVisual);
                Verify.IsTrue(verticalScrollController.AreScrollerInteractionsAllowed);
                Verify.IsFalse(verticalScrollController.IsInteracting);
                Verify.IsNull(verticalScrollController.InteractionVisual);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Change Scroller view while scroll controllers are attached.")]
        public void ChangeOffsetsWhileScrollControllersAreAttached()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            CompositionScrollController horizontalScrollController = null;
            CompositionScrollController verticalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();
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

                scroller.HorizontalScrollController = horizontalScrollController;
                scroller.VerticalScrollController = verticalScrollController;

                SetupUIWithScrollControllers(
                    scroller,
                    rectangleScrollerChild,
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
            ChangeOffsets(
                scroller,
                (c_defaultUIScrollerChildWidth - c_defaultUIScrollerWidth) / 2.0,
                (c_defaultUIScrollerChildHeight - c_defaultUIScrollerHeight) / 2.0,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.DisableAnimation,
                ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
                true /*hookViewChanged*/,
                (c_defaultUIScrollerChildWidth - c_defaultUIScrollerWidth) / 2.0,
                (c_defaultUIScrollerChildHeight - c_defaultUIScrollerHeight) / 2.0);

            Log.Comment("Animate to offsets");
            ChangeOffsets(
                scroller,
                (c_defaultUIScrollerChildWidth - c_defaultUIScrollerWidth) / 4.0,
                (c_defaultUIScrollerChildHeight - c_defaultUIScrollerHeight) / 4.0,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.AllowAnimation,
                ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
                false /*hookViewChanged*/,
                (c_defaultUIScrollerChildWidth - c_defaultUIScrollerWidth) / 4.0,
                (c_defaultUIScrollerChildHeight - c_defaultUIScrollerHeight) / 4.0);

            Log.Comment("Jump to zoomFactor 2.0");
            ChangeZoomFactor(
                scroller,
                2.0f,
                0.0f,
                0.0f,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.DisableAnimation,
                false /*hookViewChanged*/);

            Log.Comment("Animate to zoomFactor 1.5");
            ChangeZoomFactor(
                scroller,
                1.5f,
                0.0f,
                0.0f,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.AllowAnimation,
                false /*hookViewChanged*/);
        }

        [TestMethod]
        [TestProperty("Description", "Change Scroller view via attached scroll controllers.")]
        public void ChangeOffsetsWithAttachedScrollControllers()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            CompositionScrollController horizontalScrollController = null;
            CompositionScrollController verticalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);
            AutoResetEvent viewChangeCompletedEvent = new AutoResetEvent(false);
            int hOffsetChangeId = -1;
            int vOffsetChangeId = -1;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();
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

                scroller.HorizontalScrollController = horizontalScrollController;
                scroller.VerticalScrollController = verticalScrollController;

                SetupUIWithScrollControllers(
                    scroller,
                    rectangleScrollerChild,
                    horizontalScrollController,
                    verticalScrollController,
                    loadedEvent);

                horizontalScrollController.OffsetChangeCompleted += (CompositionScrollController sender, CompositionScrollControllerOffsetChangeCompletedEventArgs args) =>
                {
                    Log.Comment("ChangeOffset completed (horizontal). OffsetChangeId=" + args.OffsetChangeId + ", Result=" + args.Result);

                    Log.Comment("Setting completion event");
                    viewChangeCompletedEvent.Set();
                };

                verticalScrollController.OffsetChangeCompleted += (CompositionScrollController sender, CompositionScrollControllerOffsetChangeCompletedEventArgs args) =>
                {
                    Log.Comment("ChangeOffset completed (vertical). OffsetChangeId=" + args.OffsetChangeId + ", Result=" + args.Result);
                };
            });

            WaitForEvent("Waiting for Loaded event", loadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("HorizontalScrollController size={0}, {1}", horizontalScrollController.ActualWidth, horizontalScrollController.ActualHeight);
                Log.Comment("VerticalScrollController size={0}, {1}", verticalScrollController.ActualWidth, verticalScrollController.ActualHeight);
            });

            Log.Comment("Jump to zoomFactor 0.75");
            ChangeZoomFactor(
                scroller,
                0.75f,
                0.0f,
                0.0f,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.DisableAnimation);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Jumping to horizontal offset");
                hOffsetChangeId = horizontalScrollController.ChangeOffset(
                    (c_defaultUIScrollerChildWidth * 0.75 - c_defaultUIScrollerWidth) / 4.0,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.DisableAnimation);

                Log.Comment("Jumping to vertical offset");
                vOffsetChangeId = verticalScrollController.ChangeOffset(
                    (c_defaultUIScrollerChildHeight * 0.75 - c_defaultUIScrollerHeight) / 4.0,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.DisableAnimation);

                Verify.AreEqual(hOffsetChangeId, vOffsetChangeId);
            });

            WaitForEvent("Waiting for operation completion", viewChangeCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scroller.HorizontalOffset, (c_defaultUIScrollerChildWidth * 0.75 - c_defaultUIScrollerWidth) / 4.0);
                Verify.AreEqual(scroller.VerticalOffset, (c_defaultUIScrollerChildHeight * 0.75 - c_defaultUIScrollerHeight) / 4.0);

                Log.Comment("Animating to horizontal offset");
                hOffsetChangeId = horizontalScrollController.ChangeOffset(
                    (c_defaultUIScrollerChildWidth * 0.75 - c_defaultUIScrollerWidth) / 2.0,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.AllowAnimation);

                Log.Comment("Animating to vertical offset");
                vOffsetChangeId = verticalScrollController.ChangeOffset(
                    (c_defaultUIScrollerChildHeight * 0.75 - c_defaultUIScrollerHeight) / 2.0,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.AllowAnimation);

                Verify.AreEqual(hOffsetChangeId, vOffsetChangeId);

                viewChangeCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", viewChangeCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scroller.HorizontalOffset, (c_defaultUIScrollerChildWidth * 0.75 - c_defaultUIScrollerWidth) / 2.0);
                Verify.AreEqual(scroller.VerticalOffset, (c_defaultUIScrollerChildHeight * 0.75 - c_defaultUIScrollerHeight) / 2.0);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Change Scroller view with additional velocity via attached scroll controllers.")]
        public void ChangeOffsetsWithAdditionalVelocityAndAttachedScrollControllers()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            CompositionScrollController horizontalScrollController = null;
            CompositionScrollController verticalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);
            AutoResetEvent viewChangeCompletedEvent = new AutoResetEvent(false);
            int hOffsetChangeId = -1;
            int vOffsetChangeId = -1;

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();
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

                scroller.HorizontalScrollController = horizontalScrollController;
                scroller.VerticalScrollController = verticalScrollController;

                SetupUIWithScrollControllers(
                    scroller,
                    rectangleScrollerChild,
                    horizontalScrollController,
                    verticalScrollController,
                    loadedEvent);

                horizontalScrollController.OffsetChangeCompleted += (CompositionScrollController sender, CompositionScrollControllerOffsetChangeCompletedEventArgs args) =>
                {
                    Log.Comment("ChangeOffset completed. OffsetChangeId=" + args.OffsetChangeId + ", Result=" + args.Result);

                    Log.Comment("Setting completion event");
                    viewChangeCompletedEvent.Set();
                };
            });

            WaitForEvent("Waiting for Loaded event", loadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("HorizontalScrollController size={0}, {1}", horizontalScrollController.ActualWidth, horizontalScrollController.ActualHeight);
                Log.Comment("VerticalScrollController size={0}, {1}", verticalScrollController.ActualWidth, verticalScrollController.ActualHeight);
            });

            Log.Comment("Jump to zoomFactor 0.75");
            ChangeZoomFactor(
                scroller,
                0.75f,
                0.0f,
                0.0f,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.DisableAnimation);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding velocity to horizontal offset, with default inertia decay rate");
                hOffsetChangeId = horizontalScrollController.ChangeOffset(
                    100.0f /*additionalVelocity*/, null /*inertiaDecayRate*/);

                Log.Comment("Adding velocity to vertical offset, with default inertia decay rate");
                vOffsetChangeId = verticalScrollController.ChangeOffset(
                    100.0f /*additionalVelocity*/, null /*inertiaDecayRate*/);

                Verify.AreEqual(hOffsetChangeId, vOffsetChangeId);
            });

            WaitForEvent("Waiting for operation completion", viewChangeCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scroller.HorizontalOffset={0}", scroller.HorizontalOffset);
                Log.Comment("scroller.VerticalOffset={0}", scroller.VerticalOffset);

                Verify.IsTrue(scroller.HorizontalOffset > 20.0);
                Verify.IsTrue(scroller.VerticalOffset > 20.0);

                Log.Comment("Adding negative velocity to horizontal offset, with custom inertia decay rate");
                hOffsetChangeId = horizontalScrollController.ChangeOffset(
                    -50.0f /*additionalVelocity*/, 0.9f /*inertiaDecayRate*/);

                Log.Comment("Adding negative velocity to vertical offset, with custom inertia decay rate");
                vOffsetChangeId = verticalScrollController.ChangeOffset(
                    -50.0f /*additionalVelocity*/, 0.9f /*inertiaDecayRate*/);

                Verify.AreEqual(hOffsetChangeId, vOffsetChangeId);

                viewChangeCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", viewChangeCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scroller.HorizontalOffset={0}", scroller.HorizontalOffset);
                Log.Comment("scroller.VerticalOffset={0}", scroller.VerticalOffset);

                Verify.IsTrue(scroller.HorizontalOffset < 20.0);
                Verify.IsTrue(scroller.VerticalOffset < 20.0);

                Log.Comment("Adding velocity to horizontal offset, with no inertia decay rate");
                hOffsetChangeId = horizontalScrollController.ChangeOffset(
                    200.0f /*additionalVelocity*/, 0.0f /*inertiaDecayRate*/);

                Log.Comment("Adding velocity to vertical offset, with no inertia decay rate");
                vOffsetChangeId = verticalScrollController.ChangeOffset(
                    200.0f /*additionalVelocity*/, 0.0f /*inertiaDecayRate*/);

                Verify.AreEqual(hOffsetChangeId, vOffsetChangeId);

                viewChangeCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", viewChangeCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scroller.HorizontalOffset={0}", scroller.HorizontalOffset);
                Log.Comment("scroller.VerticalOffset={0}", scroller.VerticalOffset);

                Verify.AreEqual(scroller.HorizontalOffset, 600.0);
                Verify.AreEqual(scroller.VerticalOffset, 250.0);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Change Scroller view while a bi-directional scroll controller is attached.")]
        public void ChangeOffsetsWhileBiDirectionalScrollControllerIsAttached()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            BiDirectionalScrollController biDirectionalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();
                biDirectionalScrollController = new BiDirectionalScrollController();

                biDirectionalScrollController.LogMessage += (BiDirectionalScrollController sender, string args) =>
                {
                    Log.Comment(args);
                };

                scroller.HorizontalScrollController = biDirectionalScrollController.HorizontalScrollController;
                scroller.VerticalScrollController = biDirectionalScrollController.VerticalScrollController;

                SetupUIWithBiDirectionalScrollController(
                    scroller,
                    rectangleScrollerChild,
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
            ChangeOffsets(
                scroller,
                (c_defaultUIScrollerChildWidth - c_defaultUIScrollerWidth) / 2.0,
                (c_defaultUIScrollerChildHeight - c_defaultUIScrollerHeight) / 2.0,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.DisableAnimation,
                ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
                true /*hookViewChanged*/,
                (c_defaultUIScrollerChildWidth - c_defaultUIScrollerWidth) / 2.0,
                (c_defaultUIScrollerChildHeight - c_defaultUIScrollerHeight) / 2.0);

            Log.Comment("Animate to offsets");
            ChangeOffsets(
                scroller,
                (c_defaultUIScrollerChildWidth - c_defaultUIScrollerWidth) / 4.0,
                (c_defaultUIScrollerChildHeight - c_defaultUIScrollerHeight) / 4.0,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.AllowAnimation,
                ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints,
                false /*hookViewChanged*/,
                (c_defaultUIScrollerChildWidth - c_defaultUIScrollerWidth) / 4.0,
                (c_defaultUIScrollerChildHeight - c_defaultUIScrollerHeight) / 4.0);

            Log.Comment("Jump to zoomFactor 2.0");
            ChangeZoomFactor(
                scroller,
                2.0f,
                0.0f,
                0.0f,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.DisableAnimation,
                false /*hookViewChanged*/);

            Log.Comment("Animate to zoomFactor 1.5");
            ChangeZoomFactor(
                scroller,
                1.5f,
                0.0f,
                0.0f,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.AllowAnimation,
                false /*hookViewChanged*/);
        }

        [TestMethod]
        [TestProperty("Description", "Change Scroller view via attached bi-directional scroll controller.")]
        public void ChangeOffsetsWithAttachedBiDirectionalScrollController()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            BiDirectionalScrollController biDirectionalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);
            AutoResetEvent viewChangeCompletedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();
                biDirectionalScrollController = new BiDirectionalScrollController();

                biDirectionalScrollController.LogMessage += (BiDirectionalScrollController sender, string args) =>
                {
                    Log.Comment(args);
                };

                scroller.HorizontalScrollController = biDirectionalScrollController.HorizontalScrollController;
                scroller.VerticalScrollController = biDirectionalScrollController.VerticalScrollController;

                SetupUIWithBiDirectionalScrollController(
                    scroller,
                    rectangleScrollerChild,
                    biDirectionalScrollController,
                    loadedEvent);

                biDirectionalScrollController.ViewChangeCompleted += (BiDirectionalScrollController sender, BiDirectionalScrollControllerViewChangeCompletedEventArgs args) =>
                {
                    Log.Comment("ChangeOffset completed. ViewChangeId=" + args.ViewChangeId + ", Result=" + args.Result);

                    Log.Comment("Setting completion event");
                    viewChangeCompletedEvent.Set();
                };
            });

            WaitForEvent("Waiting for Loaded event", loadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("BiDirectionalScrollController size={0}, {1}", biDirectionalScrollController.ActualWidth, biDirectionalScrollController.ActualHeight);
            });

            Log.Comment("Jump to zoomFactor 0.75");
            ChangeZoomFactor(
                scroller,
                0.75f,
                0.0f,
                0.0f,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.DisableAnimation);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Jumping to offsets");
                biDirectionalScrollController.ChangeOffsets(
                    new ScrollerChangeOffsetsOptions(
                    (c_defaultUIScrollerChildWidth * 0.75 - c_defaultUIScrollerWidth) / 4.0,
                    (c_defaultUIScrollerChildHeight * 0.75 - c_defaultUIScrollerHeight) / 4.0,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.DisableAnimation,
                    ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
            });

            WaitForEvent("Waiting for operation completion", viewChangeCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scroller.HorizontalOffset, (c_defaultUIScrollerChildWidth * 0.75 - c_defaultUIScrollerWidth) / 4.0);
                Verify.AreEqual(scroller.VerticalOffset, (c_defaultUIScrollerChildHeight * 0.75 - c_defaultUIScrollerHeight) / 4.0);

                Log.Comment("Animating to offsets");
                biDirectionalScrollController.ChangeOffsets(
                    new ScrollerChangeOffsetsOptions(
                    (c_defaultUIScrollerChildWidth * 0.75 - c_defaultUIScrollerWidth) / 2.0,
                    (c_defaultUIScrollerChildHeight * 0.75 - c_defaultUIScrollerHeight) / 2.0,
                    ScrollerViewKind.Absolute,
                    ScrollerViewChangeKind.AllowAnimation, 
                    ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));

                viewChangeCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", viewChangeCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(scroller.HorizontalOffset, (c_defaultUIScrollerChildWidth * 0.75 - c_defaultUIScrollerWidth) / 2.0);
                Verify.AreEqual(scroller.VerticalOffset, (c_defaultUIScrollerChildHeight * 0.75 - c_defaultUIScrollerHeight) / 2.0);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Change Scroller view with additional velocity via attached bi-directional scroll controller.")]
        public void ChangeOffsetsWithAdditionalVelocityAndAttachedBiDirectionalScrollController()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            Scroller scroller = null;
            Rectangle rectangleScrollerChild = null;
            BiDirectionalScrollController biDirectionalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);
            AutoResetEvent viewChangeCompletedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                rectangleScrollerChild = new Rectangle();
                scroller = new Scroller();
                biDirectionalScrollController = new BiDirectionalScrollController();

                biDirectionalScrollController.LogMessage += (BiDirectionalScrollController sender, string args) =>
                {
                    Log.Comment(args);
                };

                scroller.HorizontalScrollController = biDirectionalScrollController.HorizontalScrollController;
                scroller.VerticalScrollController = biDirectionalScrollController.VerticalScrollController;

                SetupUIWithBiDirectionalScrollController(
                    scroller,
                    rectangleScrollerChild,
                    biDirectionalScrollController,
                    loadedEvent);

                biDirectionalScrollController.ViewChangeCompleted += (BiDirectionalScrollController sender, BiDirectionalScrollControllerViewChangeCompletedEventArgs args) =>
                {
                    Log.Comment("ChangeOffsetsWithAdditionalVelocity completed. ViewChangeId=" + args.ViewChangeId + ", Result=" + args.Result);

                    Log.Comment("Setting completion event");
                    viewChangeCompletedEvent.Set();
                };
            });

            WaitForEvent("Waiting for Loaded event", loadedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("BiDirectionalScrollController size={0}, {1}", biDirectionalScrollController.ActualWidth, biDirectionalScrollController.ActualHeight);
            });

            Log.Comment("Jump to zoomFactor 0.75");
            ChangeZoomFactor(
                scroller,
                0.75f,
                0.0f,
                0.0f,
                ScrollerViewKind.Absolute,
                ScrollerViewChangeKind.DisableAnimation);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Adding velocity to offsets, with default inertia decay rates");
                biDirectionalScrollController.ChangeOffsetsWithAdditionalVelocity(
                    new ScrollerChangeOffsetsWithAdditionalVelocityOptions(new Vector2(100.0f) /*additionalVelocity*/, null /*inertiaDecayRate*/));
            });

            WaitForEvent("Waiting for operation completion", viewChangeCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scroller.HorizontalOffset={0}", scroller.HorizontalOffset);
                Log.Comment("scroller.VerticalOffset={0}", scroller.VerticalOffset);

                Verify.IsTrue(scroller.HorizontalOffset > 20.0);
                Verify.IsTrue(scroller.VerticalOffset > 20.0);

                Log.Comment("Adding negative velocity to offsets, with custom inertia decay rates");
                biDirectionalScrollController.ChangeOffsetsWithAdditionalVelocity(
                    new ScrollerChangeOffsetsWithAdditionalVelocityOptions(new Vector2(-50.0f) /*additionalVelocity*/, new Vector2(0.9f) /*inertiaDecayRate*/));

                viewChangeCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", viewChangeCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scroller.HorizontalOffset={0}", scroller.HorizontalOffset);
                Log.Comment("scroller.VerticalOffset={0}", scroller.VerticalOffset);

                Verify.IsTrue(scroller.HorizontalOffset < 20.0);
                Verify.IsTrue(scroller.VerticalOffset < 20.0);

                Log.Comment("Adding velocity to offsets, with no inertia decay rates");
                biDirectionalScrollController.ChangeOffsetsWithAdditionalVelocity(
                    new ScrollerChangeOffsetsWithAdditionalVelocityOptions(new Vector2(200.0f) /*additionalVelocity*/, new Vector2(0.0f) /*inertiaDecayRate*/));

                viewChangeCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", viewChangeCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scroller.HorizontalOffset={0}", scroller.HorizontalOffset);
                Log.Comment("scroller.VerticalOffset={0}", scroller.VerticalOffset);

                Verify.AreEqual(scroller.HorizontalOffset, 600.0);
                Verify.AreEqual(scroller.VerticalOffset, 250.0);
            });
        }

        private void SetupUIWithScrollControllers(
            Scroller scroller,
            Rectangle rectangleScrollerChild,
            CompositionScrollController horizontalScrollController,
            CompositionScrollController verticalScrollController,
            AutoResetEvent loadedEvent)
        {
            Log.Comment("Setting up UI with Scroller and scroll controlllers" + (rectangleScrollerChild == null ? "" : " and Rectangle"));

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            if (rectangleScrollerChild != null)
            {
                rectangleScrollerChild.Width = c_defaultUIScrollerChildWidth;
                rectangleScrollerChild.Height = c_defaultUIScrollerChildHeight;
                rectangleScrollerChild.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scroller);
            scroller.Width = c_defaultUIScrollerWidth;
            scroller.Height = c_defaultUIScrollerHeight;
            if (rectangleScrollerChild != null)
            {
                scroller.Child = rectangleScrollerChild;
            }

            horizontalScrollController.Width = c_defaultUIScrollerWidth;
            horizontalScrollController.HorizontalAlignment = HorizontalAlignment.Left;

            StackPanel horizontalStackPanel = new StackPanel();
            horizontalStackPanel.Orientation = Orientation.Horizontal;
            horizontalStackPanel.Children.Add(scroller);
            horizontalStackPanel.Children.Add(verticalScrollController);

            StackPanel verticalStackPanel = new StackPanel();
            verticalStackPanel.Children.Add(horizontalStackPanel);
            verticalStackPanel.Children.Add(horizontalScrollController);
            verticalStackPanel.Width = c_defaultUIScrollerWidth + c_defaultUIScrollControllerThickness;

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
            Scroller scroller,
            Rectangle rectangleScrollerChild,
            BiDirectionalScrollController biDirectionalScrollController,
            AutoResetEvent loadedEvent)
        {
            Log.Comment("Setting up UI with Scroller and bi-directional scroll controller" + (rectangleScrollerChild == null ? "" : " and Rectangle"));

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            if (rectangleScrollerChild != null)
            {
                rectangleScrollerChild.Width = c_defaultUIScrollerChildWidth;
                rectangleScrollerChild.Height = c_defaultUIScrollerChildHeight;
                rectangleScrollerChild.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scroller);
            scroller.Width = c_defaultUIScrollerWidth;
            scroller.Height = c_defaultUIScrollerHeight;
            if (rectangleScrollerChild != null)
            {
                scroller.Child = rectangleScrollerChild;
            }

            biDirectionalScrollController.Width = c_defaultUIScrollerHeight;
            biDirectionalScrollController.Height = c_defaultUIScrollerHeight;

            StackPanel horizontalStackPanel = new StackPanel();
            horizontalStackPanel.Orientation = Orientation.Horizontal;
            horizontalStackPanel.Children.Add(scroller);
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