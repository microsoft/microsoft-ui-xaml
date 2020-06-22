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

using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using MUXControlsTestApp;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class ScrollerTests : ApiTestBase
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
                // We need the styles of the CompositionScrollController, so lets load it
                App.AppendResourceDictionaryToMergedDictionaries(App.AdditionStylesXaml);

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
        [TestProperty("Description", "Change Scroller view while scroll controllers are attached.")]
        public void ChangeOffsetsWhileScrollControllersAreAttached()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Comment("Skipping failing test on RS1.");
                return;
            }

            Scroller scroller = null;
            Rectangle rectangleScrollerContent = null;
            CompositionScrollController horizontalScrollController = null;
            CompositionScrollController verticalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                // We need the styles of the CompositionScrollController, so lets load it
                App.AppendResourceDictionaryToMergedDictionaries(App.AdditionStylesXaml);

                rectangleScrollerContent = new Rectangle();
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
                    rectangleScrollerContent,
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
                scroller,
                (c_defaultUIScrollerContentWidth - c_defaultUIScrollerWidth) / 2.0,
                (c_defaultUIScrollerContentHeight - c_defaultUIScrollerHeight) / 2.0,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: true,
                isAnimationsEnabledOverride: null, 
                expectedFinalHorizontalOffset: (c_defaultUIScrollerContentWidth - c_defaultUIScrollerWidth) / 2.0, 
                expectedFinalVerticalOffset: (c_defaultUIScrollerContentHeight - c_defaultUIScrollerHeight) / 2.0);

            Log.Comment("Animate to offsets");
            ScrollTo(
                scroller,
                (c_defaultUIScrollerContentWidth - c_defaultUIScrollerWidth) / 4.0,
                (c_defaultUIScrollerContentHeight - c_defaultUIScrollerHeight) / 4.0,
                AnimationMode.Enabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false,
                isAnimationsEnabledOverride: null,
                expectedFinalHorizontalOffset: (c_defaultUIScrollerContentWidth - c_defaultUIScrollerWidth) / 4.0,
                expectedFinalVerticalOffset: (c_defaultUIScrollerContentHeight - c_defaultUIScrollerHeight) / 4.0);

            Log.Comment("Jump to zoomFactor 2.0");
            ZoomTo(
                scroller,
                2.0f,
                0.0f,
                0.0f,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false);

            Log.Comment("Animate to zoomFactor 1.5");
            ZoomTo(
                scroller,
                1.5f,
                0.0f,
                0.0f,
                AnimationMode.Enabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false);
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
            Rectangle rectangleScrollerContent = null;
            CompositionScrollController horizontalScrollController = null;
            CompositionScrollController verticalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollCompletedEvent = new AutoResetEvent(false);
            int hOffsetChangeId = -1;
            int vOffsetChangeId = -1;

            RunOnUIThread.Execute(() =>
            {
                // We need the styles of the CompositionScrollController, so lets load it
                App.AppendResourceDictionaryToMergedDictionaries(App.AdditionStylesXaml);

                rectangleScrollerContent = new Rectangle();
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
                    rectangleScrollerContent,
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
                scroller,
                0.75f,
                0.0f,
                0.0f,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Jumping to horizontal offset");
                hOffsetChangeId = horizontalScrollController.ScrollTo(
                    (c_defaultUIScrollerContentWidth * 0.75 - c_defaultUIScrollerWidth) / 4.0,
                    AnimationMode.Disabled);

                Log.Comment("Jumping to vertical offset");
                vOffsetChangeId = verticalScrollController.ScrollTo(
                    (c_defaultUIScrollerContentHeight * 0.75 - c_defaultUIScrollerHeight) / 4.0,
                    AnimationMode.Disabled);

                Verify.AreEqual(hOffsetChangeId, vOffsetChangeId);
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual((c_defaultUIScrollerContentWidth * 0.75 - c_defaultUIScrollerWidth) / 4.0, scroller.HorizontalOffset);
                Verify.AreEqual((c_defaultUIScrollerContentHeight * 0.75 - c_defaultUIScrollerHeight) / 4.0, scroller.VerticalOffset);

                Log.Comment("Animating to horizontal offset");
                hOffsetChangeId = horizontalScrollController.ScrollTo(
                    (c_defaultUIScrollerContentWidth * 0.75 - c_defaultUIScrollerWidth) / 2.0,
                    AnimationMode.Enabled);

                Log.Comment("Animating to vertical offset");
                vOffsetChangeId = verticalScrollController.ScrollTo(
                    (c_defaultUIScrollerContentHeight * 0.75 - c_defaultUIScrollerHeight) / 2.0,
                    AnimationMode.Enabled);

                Verify.AreEqual(hOffsetChangeId, vOffsetChangeId);

                scrollCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual((c_defaultUIScrollerContentWidth * 0.75 - c_defaultUIScrollerWidth) / 2.0, scroller.HorizontalOffset);
                Verify.AreEqual((c_defaultUIScrollerContentHeight * 0.75 - c_defaultUIScrollerHeight) / 2.0, scroller.VerticalOffset);
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
            Rectangle rectangleScrollerContent = null;
            CompositionScrollController horizontalScrollController = null;
            CompositionScrollController verticalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollCompletedEvent = new AutoResetEvent(false);
            int hOffsetChangeId = -1;
            int vOffsetChangeId = -1;

            RunOnUIThread.Execute(() =>
            {
                // We need the styles of the CompositionScrollController, so lets load it
                App.AppendResourceDictionaryToMergedDictionaries(App.AdditionStylesXaml);

                rectangleScrollerContent = new Rectangle();
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
                    rectangleScrollerContent,
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
                scroller,
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
                Log.Comment("scroller.HorizontalOffset={0}", scroller.HorizontalOffset);
                Log.Comment("scroller.VerticalOffset={0}", scroller.VerticalOffset);

                Verify.IsTrue(scroller.HorizontalOffset > 20.0);
                Verify.IsTrue(scroller.VerticalOffset > 20.0);

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
                Log.Comment("scroller.HorizontalOffset={0}", scroller.HorizontalOffset);
                Log.Comment("scroller.VerticalOffset={0}", scroller.VerticalOffset);

                Verify.IsTrue(scroller.HorizontalOffset < 20.0);
                Verify.IsTrue(scroller.VerticalOffset < 20.0);

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
                Log.Comment("scroller.HorizontalOffset={0}", scroller.HorizontalOffset);
                Log.Comment("scroller.VerticalOffset={0}", scroller.VerticalOffset);

                Verify.AreEqual(600.0, scroller.HorizontalOffset);
                Verify.AreEqual(250.0, scroller.VerticalOffset);
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
            Rectangle rectangleScrollerContent = null;
            BiDirectionalScrollController biDirectionalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                // We need the styles of the CompositionScrollController, so lets load it
                App.AppendResourceDictionaryToMergedDictionaries(App.AdditionStylesXaml);

                rectangleScrollerContent = new Rectangle();
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
                    rectangleScrollerContent,
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
                scroller,
                (c_defaultUIScrollerContentWidth - c_defaultUIScrollerWidth) / 2.0,
                (c_defaultUIScrollerContentHeight - c_defaultUIScrollerHeight) / 2.0,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: true,
                isAnimationsEnabledOverride: null,
                expectedFinalHorizontalOffset: (c_defaultUIScrollerContentWidth - c_defaultUIScrollerWidth) / 2.0,
                expectedFinalVerticalOffset: (c_defaultUIScrollerContentHeight - c_defaultUIScrollerHeight) / 2.0);

            Log.Comment("Animate to offsets");
            ScrollTo(
                scroller,
                (c_defaultUIScrollerContentWidth - c_defaultUIScrollerWidth) / 4.0,
                (c_defaultUIScrollerContentHeight - c_defaultUIScrollerHeight) / 4.0,
                AnimationMode.Enabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false,
                isAnimationsEnabledOverride: null,
                expectedFinalHorizontalOffset: (c_defaultUIScrollerContentWidth - c_defaultUIScrollerWidth) / 4.0,
                expectedFinalVerticalOffset: (c_defaultUIScrollerContentHeight - c_defaultUIScrollerHeight) / 4.0);

            Log.Comment("Jump to zoomFactor 2.0");
            ZoomTo(
                scroller,
                2.0f,
                0.0f,
                0.0f,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false);

            Log.Comment("Animate to zoomFactor 1.5");
            ZoomTo(
                scroller,
                1.5f,
                0.0f,
                0.0f,
                AnimationMode.Enabled,
                SnapPointsMode.Ignore,
                hookViewChanged: false);
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
            Rectangle rectangleScrollerContent = null;
            BiDirectionalScrollController biDirectionalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollCompletedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                // We need the styles of the CompositionScrollController, so lets load it
                App.AppendResourceDictionaryToMergedDictionaries(App.AdditionStylesXaml);

                rectangleScrollerContent = new Rectangle();
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
                    rectangleScrollerContent,
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
                scroller,
                0.75f,
                0.0f,
                0.0f,
                AnimationMode.Disabled,
                SnapPointsMode.Ignore);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Jumping to offsets");
                biDirectionalScrollController.ScrollTo(
                    (c_defaultUIScrollerContentWidth * 0.75 - c_defaultUIScrollerWidth) / 4.0,
                    (c_defaultUIScrollerContentHeight * 0.75 - c_defaultUIScrollerHeight) / 4.0,
                    AnimationMode.Disabled);
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual((c_defaultUIScrollerContentWidth * 0.75 - c_defaultUIScrollerWidth) / 4.0, scroller.HorizontalOffset);
                Verify.AreEqual((c_defaultUIScrollerContentHeight * 0.75 - c_defaultUIScrollerHeight) / 4.0, scroller.VerticalOffset);

                Log.Comment("Animating to offsets");
                biDirectionalScrollController.ScrollTo(
                    (c_defaultUIScrollerContentWidth * 0.75 - c_defaultUIScrollerWidth) / 2.0,
                    (c_defaultUIScrollerContentHeight * 0.75 - c_defaultUIScrollerHeight) / 2.0,
                    AnimationMode.Enabled);

                scrollCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual((c_defaultUIScrollerContentWidth * 0.75 - c_defaultUIScrollerWidth) / 2.0, scroller.HorizontalOffset);
                Verify.AreEqual((c_defaultUIScrollerContentHeight * 0.75 - c_defaultUIScrollerHeight) / 2.0, scroller.VerticalOffset);
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
            Rectangle rectangleScrollerContent = null;
            BiDirectionalScrollController biDirectionalScrollController = null;
            AutoResetEvent loadedEvent = new AutoResetEvent(false);
            AutoResetEvent scrollCompletedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                // We need the styles of the CompositionScrollController, so lets load it
                App.AppendResourceDictionaryToMergedDictionaries(App.AdditionStylesXaml);

                rectangleScrollerContent = new Rectangle();
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
                    rectangleScrollerContent,
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
                scroller,
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
                Log.Comment("scroller.HorizontalOffset={0}", scroller.HorizontalOffset);
                Log.Comment("scroller.VerticalOffset={0}", scroller.VerticalOffset);

                Verify.IsTrue(scroller.HorizontalOffset > 20.0);
                Verify.IsTrue(scroller.VerticalOffset > 20.0);

                Log.Comment("Adding negative velocity to offsets, with custom inertia decay rates");
                biDirectionalScrollController.ScrollFrom(
                    new Vector2(-50.0f) /*offsetsVelocity*/, new Vector2(0.9f) /*inertiaDecayRate*/);

                scrollCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scroller.HorizontalOffset={0}", scroller.HorizontalOffset);
                Log.Comment("scroller.VerticalOffset={0}", scroller.VerticalOffset);

                Verify.IsTrue(scroller.HorizontalOffset < 20.0);
                Verify.IsTrue(scroller.VerticalOffset < 20.0);

                Log.Comment("Adding velocity to offsets, with no inertia decay rates");
                biDirectionalScrollController.ScrollFrom(
                    new Vector2(200.0f) /*offsetsVelocity*/, new Vector2(0.0f) /*inertiaDecayRate*/);

                scrollCompletedEvent.Reset();
            });

            WaitForEvent("Waiting for operation completion", scrollCompletedEvent);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("scroller.HorizontalOffset={0}", scroller.HorizontalOffset);
                Log.Comment("scroller.VerticalOffset={0}", scroller.VerticalOffset);

                Verify.AreEqual(600.0, scroller.HorizontalOffset);
                Verify.AreEqual(250.0, scroller.VerticalOffset);
            });
        }

        private void SetupUIWithScrollControllers(
            Scroller scroller,
            Rectangle rectangleScrollerContent,
            CompositionScrollController horizontalScrollController,
            CompositionScrollController verticalScrollController,
            AutoResetEvent loadedEvent)
        {
            Log.Comment("Setting up UI with Scroller and scroll controlllers" + (rectangleScrollerContent == null ? "" : " and Rectangle"));

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            if (rectangleScrollerContent != null)
            {
                rectangleScrollerContent.Width = c_defaultUIScrollerContentWidth;
                rectangleScrollerContent.Height = c_defaultUIScrollerContentHeight;
                rectangleScrollerContent.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scroller);
            scroller.Width = c_defaultUIScrollerWidth;
            scroller.Height = c_defaultUIScrollerHeight;
            if (rectangleScrollerContent != null)
            {
                scroller.Content = rectangleScrollerContent;
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
            Content = verticalStackPanel;
        }

        private void SetupUIWithBiDirectionalScrollController(
            Scroller scroller,
            Rectangle rectangleScrollerContent,
            BiDirectionalScrollController biDirectionalScrollController,
            AutoResetEvent loadedEvent)
        {
            Log.Comment("Setting up UI with Scroller and bi-directional scroll controller" + (rectangleScrollerContent == null ? "" : " and Rectangle"));

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            if (rectangleScrollerContent != null)
            {
                rectangleScrollerContent.Width = c_defaultUIScrollerContentWidth;
                rectangleScrollerContent.Height = c_defaultUIScrollerContentHeight;
                rectangleScrollerContent.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scroller);
            scroller.Width = c_defaultUIScrollerWidth;
            scroller.Height = c_defaultUIScrollerHeight;
            if (rectangleScrollerContent != null)
            {
                scroller.Content = rectangleScrollerContent;
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
            Content = horizontalStackPanel;
        }
    }
}