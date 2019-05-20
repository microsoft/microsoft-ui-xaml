// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Threading;
using Windows.Foundation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Xaml.Media;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using ScrollViewer = Microsoft.UI.Xaml.Controls.ScrollViewer;
using ScrollBarVisibility = Microsoft.UI.Xaml.Controls.ScrollBarVisibility;
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using InputKind = Microsoft.UI.Xaml.Controls.InputKind;
using ChainingMode = Microsoft.UI.Xaml.Controls.ChainingMode;
using RailingMode = Microsoft.UI.Xaml.Controls.RailingMode;
using ZoomMode = Microsoft.UI.Xaml.Controls.ZoomMode;
using ScrollerAnchorRequestedEventArgs = Microsoft.UI.Xaml.Controls.ScrollerAnchorRequestedEventArgs;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using ScrollViewerTestHooks = Microsoft.UI.Private.Controls.ScrollViewerTestHooks;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ScrollViewerTests
    {
        private const int c_MaxWaitDuration = 5000;
        private const double c_epsilon = 0.0000001;

        private const InputKind c_defaultIgnoredInputKind = InputKind.None;
        private const ChainingMode c_defaultHorizontalScrollChainingMode = ChainingMode.Auto;
        private const ChainingMode c_defaultVerticalScrollChainingMode = ChainingMode.Auto;
        private const RailingMode c_defaultHorizontalScrollRailingMode = RailingMode.Enabled;
        private const RailingMode c_defaultVerticalScrollRailingMode = RailingMode.Enabled;
#if USE_SCROLLMODE_AUTO
        private const ScrollMode c_defaultComputedHorizontalScrollMode = ScrollMode.Disabled;
        private const ScrollMode c_defaultComputedVerticalScrollMode = ScrollMode.Disabled;
        private const ScrollMode c_defaultHorizontalScrollMode = ScrollMode.Auto;
        private const ScrollMode c_defaultVerticalScrollMode = ScrollMode.Auto;
#else
        private const ScrollMode c_defaultHorizontalScrollMode = ScrollMode.Enabled;
        private const ScrollMode c_defaultVerticalScrollMode = ScrollMode.Enabled;
#endif
        private const ChainingMode c_defaultZoomChainingMode = ChainingMode.Auto;
        private const ZoomMode c_defaultZoomMode = ZoomMode.Disabled;
        private const ContentOrientation c_defaultContentOrientation = ContentOrientation.Vertical;
        private const double c_defaultMinZoomFactor = 0.1;
        private const double c_defaultMaxZoomFactor = 10.0;
        private const double c_defaultAnchorRatio = 0.0;

        private const double c_defaultUIScrollViewerContentWidth = 1200.0;
        private const double c_defaultUIScrollViewerContentHeight = 600.0;
        private const double c_defaultUIScrollViewerWidth = 300.0;
        private const double c_defaultUIScrollViewerHeight = 200.0;

        [TestCleanup]
        public void TestCleanup()
        {
            TestUtilities.ClearVisualTreeRoot();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollViewer default properties.")]
        public void VerifyDefaultPropertyValues()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollViewer not supported pre-RS2");
                return;
            }

            RunOnUIThread.Execute(() =>
            {
                ScrollViewer scrollViewer = new ScrollViewer();
                Verify.IsNotNull(scrollViewer);

                Log.Comment("Verifying ScrollViewer default property values");
                Verify.IsNull(scrollViewer.Content);
                Verify.IsNull(ScrollViewerTestHooks.GetScrollerPart(scrollViewer));
                Verify.IsNull(scrollViewer.HorizontalScrollController);
                Verify.IsNull(scrollViewer.VerticalScrollController);
#if USE_SCROLLMODE_AUTO
                Verify.AreEqual(scrollViewer.ComputedHorizontalScrollMode, c_defaultComputedHorizontalScrollMode);
                Verify.AreEqual(scrollViewer.ComputedVerticalScrollMode, c_defaultComputedVerticalScrollMode);
#endif
                Verify.AreEqual(scrollViewer.IgnoredInputKind, c_defaultIgnoredInputKind);
                Verify.AreEqual(scrollViewer.ContentOrientation, c_defaultContentOrientation);
                Verify.AreEqual(scrollViewer.HorizontalScrollChainingMode, c_defaultHorizontalScrollChainingMode);
                Verify.AreEqual(scrollViewer.VerticalScrollChainingMode, c_defaultVerticalScrollChainingMode);
                Verify.AreEqual(scrollViewer.HorizontalScrollRailingMode, c_defaultHorizontalScrollRailingMode);
                Verify.AreEqual(scrollViewer.VerticalScrollRailingMode, c_defaultVerticalScrollRailingMode);
                Verify.AreEqual(scrollViewer.HorizontalScrollMode, c_defaultHorizontalScrollMode);
                Verify.AreEqual(scrollViewer.VerticalScrollMode, c_defaultVerticalScrollMode);
                Verify.AreEqual(scrollViewer.ZoomMode, c_defaultZoomMode);
                Verify.AreEqual(scrollViewer.ZoomChainingMode, c_defaultZoomChainingMode);
                Verify.IsGreaterThan(scrollViewer.MinZoomFactor, c_defaultMinZoomFactor - c_epsilon);
                Verify.IsLessThan(scrollViewer.MinZoomFactor, c_defaultMinZoomFactor + c_epsilon);
                Verify.IsGreaterThan(scrollViewer.MaxZoomFactor, c_defaultMaxZoomFactor - c_epsilon);
                Verify.IsLessThan(scrollViewer.MaxZoomFactor, c_defaultMaxZoomFactor + c_epsilon);
                Verify.AreEqual(scrollViewer.HorizontalAnchorRatio, c_defaultAnchorRatio);
                Verify.AreEqual(scrollViewer.VerticalAnchorRatio, c_defaultAnchorRatio);
                Verify.AreEqual(scrollViewer.ExtentWidth, 0.0);
                Verify.AreEqual(scrollViewer.ExtentHeight, 0.0);
                Verify.AreEqual(scrollViewer.ViewportWidth, 0.0);
                Verify.AreEqual(scrollViewer.ViewportHeight, 0.0);
                Verify.AreEqual(scrollViewer.ScrollableWidth, 0.0);
                Verify.AreEqual(scrollViewer.ScrollableHeight, 0.0);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollViewer properties after template application.")]
        public void VerifyScrollerAttachedProperties()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollViewer not supported pre-RS2");
                return;
            }

            using (PrivateLoggingHelper privateSVLoggingHelper = new PrivateLoggingHelper("ScrollViewer", "Scroller"))
            {
                ScrollViewer scrollViewer = null;
                Rectangle rectangleScrollViewerContent = null;
                AutoResetEvent scrollViewerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollViewerUnloadedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollViewerContent = new Rectangle();
                    scrollViewer = new ScrollViewer();

                    SetupDefaultUI(scrollViewer, rectangleScrollViewerContent, scrollViewerLoadedEvent, scrollViewerUnloadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollViewerLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting Scroller-cloned properties to non-default values");
                    scrollViewer.IgnoredInputKind = InputKind.MouseWheel | InputKind.Pen;
                    scrollViewer.ContentOrientation = ContentOrientation.Horizontal;
                    scrollViewer.HorizontalScrollChainingMode = ChainingMode.Always;
                    scrollViewer.VerticalScrollChainingMode = ChainingMode.Never;
                    scrollViewer.HorizontalScrollRailingMode = RailingMode.Disabled;
                    scrollViewer.VerticalScrollRailingMode = RailingMode.Disabled;
                    scrollViewer.HorizontalScrollMode = ScrollMode.Enabled;
                    scrollViewer.VerticalScrollMode = ScrollMode.Disabled;
                    scrollViewer.ZoomMode = ZoomMode.Enabled;
                    scrollViewer.ZoomChainingMode = ChainingMode.Never;
                    scrollViewer.MinZoomFactor = 2.0;
                    scrollViewer.MaxZoomFactor = 8.0;

                    Log.Comment("Verifying Scroller-cloned non-default properties");
                    Verify.AreEqual(scrollViewer.IgnoredInputKind, InputKind.MouseWheel | InputKind.Pen);
                    Verify.AreEqual(scrollViewer.ContentOrientation, ContentOrientation.Horizontal);
                    Verify.AreEqual(scrollViewer.HorizontalScrollChainingMode, ChainingMode.Always);
                    Verify.AreEqual(scrollViewer.VerticalScrollChainingMode, ChainingMode.Never);
                    Verify.AreEqual(scrollViewer.HorizontalScrollRailingMode, RailingMode.Disabled);
                    Verify.AreEqual(scrollViewer.VerticalScrollRailingMode, RailingMode.Disabled);
                    Verify.AreEqual(scrollViewer.HorizontalScrollMode, ScrollMode.Enabled);
                    Verify.AreEqual(scrollViewer.VerticalScrollMode, ScrollMode.Disabled);
#if USE_SCROLLMODE_AUTO
                    Verify.AreEqual(scrollViewer.ComputedHorizontalScrollMode, ScrollMode.Enabled);
                    Verify.AreEqual(scrollViewer.ComputedVerticalScrollMode, ScrollMode.Disabled);
#endif
                    Verify.AreEqual(scrollViewer.ZoomMode, ZoomMode.Enabled);
                    Verify.AreEqual(scrollViewer.ZoomChainingMode, ChainingMode.Never);
                    Verify.IsGreaterThan(scrollViewer.MinZoomFactor, 2.0 - c_epsilon);
                    Verify.IsLessThan(scrollViewer.MinZoomFactor, 2.0 + c_epsilon);
                    Verify.IsGreaterThan(scrollViewer.MaxZoomFactor, 8.0 - c_epsilon);
                    Verify.IsLessThan(scrollViewer.MaxZoomFactor, 8.0 + c_epsilon);

                    Log.Comment("Resetting window content and ScrollViewer");
                    MUXControlsTestApp.App.TestContentRoot = null;
                    scrollViewer = null;
                });

                WaitForEvent("Waiting for Unloaded event", scrollViewerUnloadedEvent);

                IdleSynchronizer.Wait();
                Log.Comment("Garbage collecting...");
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                Log.Comment("Done");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the Scroller attached properties.")]
        public void VerifyPropertyValuesAfterTemplateApplication()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollViewer not supported pre-RS2");
                return;
            }

            using (PrivateLoggingHelper privateSVLoggingHelper = new PrivateLoggingHelper("ScrollViewer", "Scroller"))
            {
                ScrollViewer scrollViewer = null;
                Rectangle rectangleScrollViewerContent = null;
                AutoResetEvent scrollViewerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollViewerUnloadedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollViewerContent = new Rectangle();
                    scrollViewer = new ScrollViewer();

                    SetupDefaultUI(scrollViewer, rectangleScrollViewerContent, scrollViewerLoadedEvent, scrollViewerUnloadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollViewerLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Verifying ScrollViewer property values after Loaded event");
                    Verify.AreEqual(scrollViewer.Content, rectangleScrollViewerContent);
                    Verify.IsNotNull(ScrollViewerTestHooks.GetScrollerPart(scrollViewer));
                    Verify.AreEqual(ScrollViewerTestHooks.GetScrollerPart(scrollViewer).Content, rectangleScrollViewerContent);
                    Verify.IsNotNull(scrollViewer.HorizontalScrollController);
                    Verify.IsNotNull(scrollViewer.VerticalScrollController);
                    Verify.AreEqual(scrollViewer.ExtentWidth, c_defaultUIScrollViewerContentWidth);
                    Verify.AreEqual(scrollViewer.ExtentHeight, c_defaultUIScrollViewerContentHeight);
                    Verify.AreEqual(scrollViewer.ViewportWidth, c_defaultUIScrollViewerWidth);
                    Verify.AreEqual(scrollViewer.ViewportHeight, c_defaultUIScrollViewerHeight);
                    Verify.AreEqual(scrollViewer.ScrollableWidth, c_defaultUIScrollViewerContentWidth - c_defaultUIScrollViewerWidth);
                    Verify.AreEqual(scrollViewer.ScrollableHeight, c_defaultUIScrollViewerContentHeight - c_defaultUIScrollViewerHeight);

                    Log.Comment("Resetting window content and ScrollViewer");
                    MUXControlsTestApp.App.TestContentRoot = null;
                    scrollViewer = null;
                });

                WaitForEvent("Waiting for Unloaded event", scrollViewerUnloadedEvent);

                IdleSynchronizer.Wait();
                Log.Comment("Garbage collecting...");
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                Log.Comment("Done");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollViewer visual state changes based on the AutoHideScrollBars, IsEnabled and ScrollBarVisibility settings.")]
        public void VerifyVisualStates()
        {
            VerifyVisualStates(ScrollBarVisibility.Auto, autoHideScrollControllers: true);
            VerifyVisualStates(ScrollBarVisibility.Visible, autoHideScrollControllers: true);

            // Non-auto-hiding ScrollControllers are only supported starting with RS4.
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
            {
                VerifyVisualStates(ScrollBarVisibility.Auto, autoHideScrollControllers: false);
                VerifyVisualStates(ScrollBarVisibility.Visible, autoHideScrollControllers: false);
            }
        }

        private void VerifyVisualStates(ScrollBarVisibility scrollBarVisibility, bool autoHideScrollControllers)
        {
            using (PrivateLoggingHelper privateSVLoggingHelper = new PrivateLoggingHelper("ScrollViewer"))
            {
                using (ScrollViewerTestHooksHelper scrollViewerTestHooksHelper = new ScrollViewerTestHooksHelper(autoHideScrollControllers))
                {
                    ScrollViewerWithCustomVisualStateManager scrollViewer = null;
                    Rectangle rectangleScrollViewerContent = null;
                    AutoResetEvent scrollViewerLoadedEvent = new AutoResetEvent(false);
                    AutoResetEvent scrollViewerUnloadedEvent = new AutoResetEvent(false);                    

                    RunOnUIThread.Execute(() =>
                    {
                        rectangleScrollViewerContent = new Rectangle();
                        scrollViewer = new ScrollViewerWithCustomVisualStateManager();
                        scrollViewer.HorizontalScrollBarVisibility = scrollBarVisibility;
                        scrollViewer.VerticalScrollBarVisibility = scrollBarVisibility;

                        SetupDefaultUI(scrollViewer, rectangleScrollViewerContent, scrollViewerLoadedEvent, scrollViewerUnloadedEvent);
                    });

                    WaitForEvent("Waiting for Loaded event", scrollViewerLoadedEvent);

                    RunOnUIThread.Execute(() =>
                    {
                        Log.Comment("Resetting window content");
                        MUXControlsTestApp.App.TestContentRoot = null;
                    });

                    WaitForEvent("Waiting for Unloaded event", scrollViewerUnloadedEvent);

                    RunOnUIThread.Execute(() =>
                    {
                        Log.Comment($"VerifyVisualStates: isEnabled:True, scrollBarVisibility:{scrollBarVisibility}, autoHideScrollControllers:{autoHideScrollControllers}");

                        VerifyVisualStates(
                            stateChanges: scrollViewer.StateChanges,
                            expectedMouseIndicatorStateCount: autoHideScrollControllers ? 0u : 2u,
                            expectedTouchIndicatorStateCount: 0,
                            expectedNoIndicatorStateCount: autoHideScrollControllers ? 2u : 0u,
                            expectedScrollBarsSeparatorCollapsedStateCount: autoHideScrollControllers ? (scrollBarVisibility == ScrollBarVisibility.Auto ? 1u : 2u) : 0u,
                            expectedScrollBarsSeparatorCollapsedDisabledStateCount: 0,
                            expectedScrollBarsSeparatorExpandedStateCount: autoHideScrollControllers ? 0u : (scrollBarVisibility == ScrollBarVisibility.Auto ? 1u : 2u),
                            expectedScrollBarsSeparatorDisplayedWithoutAnimationStateCount: 0,
                            expectedScrollBarsSeparatorExpandedWithoutAnimationStateCount: 0,
                            expectedScrollBarsSeparatorCollapsedWithoutAnimationStateCount: 0);

                        scrollViewer.StateChanges.Clear();

                        Log.Comment("Disabling ScrollViewer");
                        scrollViewer.IsEnabled = false;
                    });

                    IdleSynchronizer.Wait();

                    RunOnUIThread.Execute(() =>
                    {
                        Log.Comment($"VerifyVisualStates: isEnabled:False, scrollBarVisibility:{scrollBarVisibility}, autoHideScrollControllers:{autoHideScrollControllers}");

                        VerifyVisualStates(
                            stateChanges: scrollViewer.StateChanges,
                            expectedMouseIndicatorStateCount: autoHideScrollControllers ? 0u : (scrollBarVisibility == ScrollBarVisibility.Auto ? 1u : 2u),
                            expectedTouchIndicatorStateCount: 0,
                            expectedNoIndicatorStateCount: autoHideScrollControllers ? (scrollBarVisibility == ScrollBarVisibility.Auto ? 1u : 2u) : 0u,
                            expectedScrollBarsSeparatorCollapsedStateCount: 0,
                            expectedScrollBarsSeparatorCollapsedDisabledStateCount: scrollBarVisibility == ScrollBarVisibility.Auto ? 0u : 2u,
                            expectedScrollBarsSeparatorExpandedStateCount: 0,
                            expectedScrollBarsSeparatorDisplayedWithoutAnimationStateCount: 0,
                            expectedScrollBarsSeparatorExpandedWithoutAnimationStateCount: 0,
                            expectedScrollBarsSeparatorCollapsedWithoutAnimationStateCount: 0);

                        scrollViewer.StateChanges.Clear();

                        Log.Comment("Enabling ScrollViewer");
                        scrollViewer.IsEnabled = true;
                    });

                    IdleSynchronizer.Wait();

                    RunOnUIThread.Execute(() =>
                    {
                        Log.Comment($"VerifyVisualStates: isEnabled:True, scrollBarVisibility:{scrollBarVisibility}, autoHideScrollControllers:{autoHideScrollControllers}");

                        VerifyVisualStates(
                            stateChanges: scrollViewer.StateChanges,
                            expectedMouseIndicatorStateCount: autoHideScrollControllers ? 0u : 2u,
                            expectedTouchIndicatorStateCount: 0,
                            expectedNoIndicatorStateCount: autoHideScrollControllers ? 2u : 0u,
                            expectedScrollBarsSeparatorCollapsedStateCount: autoHideScrollControllers ? (scrollBarVisibility == ScrollBarVisibility.Auto ? 1u : 2u) : 0u,
                            expectedScrollBarsSeparatorCollapsedDisabledStateCount: 0,
                            expectedScrollBarsSeparatorExpandedStateCount: autoHideScrollControllers ? 0u : (scrollBarVisibility == ScrollBarVisibility.Auto ? 1u : 2u),
                            expectedScrollBarsSeparatorDisplayedWithoutAnimationStateCount: 0,
                            expectedScrollBarsSeparatorExpandedWithoutAnimationStateCount: 0,
                            expectedScrollBarsSeparatorCollapsedWithoutAnimationStateCount: 0);
                    });
                }
            }
        }

        private void VerifyVisualStates(
            List<StateChange> stateChanges,
            uint expectedMouseIndicatorStateCount,
            uint expectedTouchIndicatorStateCount,
            uint expectedNoIndicatorStateCount,
            uint expectedScrollBarsSeparatorCollapsedStateCount,
            uint expectedScrollBarsSeparatorCollapsedDisabledStateCount,
            uint expectedScrollBarsSeparatorExpandedStateCount,
            uint expectedScrollBarsSeparatorDisplayedWithoutAnimationStateCount,
            uint expectedScrollBarsSeparatorExpandedWithoutAnimationStateCount,
            uint expectedScrollBarsSeparatorCollapsedWithoutAnimationStateCount)
        {
            uint mouseIndicatorStateCount = 0;
            uint touchIndicatorStateCount = 0;
            uint noIndicatorStateCount = 0;

            uint scrollBarsSeparatorCollapsedStateCount = 0;
            uint scrollBarsSeparatorCollapsedDisabledStateCount = 0;
            uint scrollBarsSeparatorExpandedStateCount = 0;

            uint scrollBarsSeparatorDisplayedWithoutAnimationStateCount = 0;
            uint scrollBarsSeparatorExpandedWithoutAnimationStateCount = 0;
            uint scrollBarsSeparatorCollapsedWithoutAnimationStateCount = 0;

            foreach (StateChange stateChange in stateChanges)
            {
                Log.Comment($"StateChange: stateName:{stateChange.StateName}, group:{stateChange.Group.Name}, useTransitions:{stateChange.UseTransitions}");

                switch (stateChange.StateName)
                {
                    case "MouseIndicator":
                        mouseIndicatorStateCount++;
                        break;
                    case "TouchIndicator":
                        touchIndicatorStateCount++;
                        break;
                    case "NoIndicator":
                        noIndicatorStateCount++;
                        break;
                    case "ScrollBarsSeparatorCollapsed":
                        scrollBarsSeparatorCollapsedStateCount++;
                        break;
                    case "ScrollBarsSeparatorCollapsedDisabled":
                        scrollBarsSeparatorCollapsedDisabledStateCount++;
                        break;
                    case "ScrollBarsSeparatorExpanded":
                        scrollBarsSeparatorExpandedStateCount++;
                        break;
                    case "ScrollBarsSeparatorDisplayedWithoutAnimation":
                        scrollBarsSeparatorDisplayedWithoutAnimationStateCount++;
                        break;
                    case "ScrollBarsSeparatorExpandedWithoutAnimation":
                        scrollBarsSeparatorExpandedWithoutAnimationStateCount++;
                        break;
                    case "ScrollBarsSeparatorCollapsedWithoutAnimation":
                        scrollBarsSeparatorCollapsedWithoutAnimationStateCount++;
                        break;
                    default:
                        Verify.Fail("Unexpected ScrollViewer StateName");
                        break;
                }
            }

            Verify.AreEqual(expectedMouseIndicatorStateCount, mouseIndicatorStateCount);
            Verify.AreEqual(expectedTouchIndicatorStateCount, touchIndicatorStateCount);
            Verify.AreEqual(expectedNoIndicatorStateCount, noIndicatorStateCount);
            Verify.AreEqual(expectedScrollBarsSeparatorCollapsedStateCount, scrollBarsSeparatorCollapsedStateCount);
            Verify.AreEqual(expectedScrollBarsSeparatorCollapsedDisabledStateCount, scrollBarsSeparatorCollapsedDisabledStateCount);
            Verify.AreEqual(expectedScrollBarsSeparatorExpandedStateCount, scrollBarsSeparatorExpandedStateCount);
            Verify.AreEqual(expectedScrollBarsSeparatorDisplayedWithoutAnimationStateCount, scrollBarsSeparatorDisplayedWithoutAnimationStateCount);
            Verify.AreEqual(expectedScrollBarsSeparatorExpandedWithoutAnimationStateCount, scrollBarsSeparatorExpandedWithoutAnimationStateCount);
            Verify.AreEqual(expectedScrollBarsSeparatorCollapsedWithoutAnimationStateCount, scrollBarsSeparatorCollapsedWithoutAnimationStateCount);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies anchor candidate registration and unregistration.")]
        public void VerifyAnchorCandidateRegistration()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollViewer not supported pre-RS2");
                return;
            }

            using (PrivateLoggingHelper privateSVLoggingHelper = new PrivateLoggingHelper("ScrollViewer", "Scroller"))
            {
                int expectedAnchorCandidatesCount = 0;
                Scroller scroller = null;
                ScrollViewer scrollViewer = null;
                Rectangle rectangleScrollViewerContent = null;
                AutoResetEvent scrollViewerLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollViewerAnchorRequestedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollViewerContent = new Rectangle();
                    scrollViewer = new ScrollViewer();
                    scrollViewer.HorizontalAnchorRatio = 0.1;

                    SetupDefaultUI(scrollViewer, rectangleScrollViewerContent, scrollViewerLoadedEvent);

                    scrollViewer.AnchorRequested += (ScrollViewer sender, ScrollerAnchorRequestedEventArgs args) =>
                    {
                        Log.Comment("ScrollViewer.AnchorRequested event handler. args.AnchorCandidates.Count: " + args.AnchorCandidates.Count);
                        Verify.IsNull(args.AnchorElement);
                        Verify.AreEqual(expectedAnchorCandidatesCount, args.AnchorCandidates.Count);
                        scrollViewerAnchorRequestedEvent.Set();
                    };
                });

                WaitForEvent("Waiting for Loaded event", scrollViewerLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Accessing inner Scroller control");
                    scroller = ScrollViewerTestHooks.GetScrollerPart(scrollViewer);

                    Log.Comment("Registering Rectangle as anchor candidate");
                    scrollViewer.RegisterAnchorCandidate(rectangleScrollViewerContent);
                    expectedAnchorCandidatesCount = 1;

                    Log.Comment("Forcing Scroller layout");
                    scroller.InvalidateArrange();
                });

                WaitForEvent("Waiting for AnchorRequested event", scrollViewerAnchorRequestedEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Unregistering Rectangle as anchor candidate");
                    scrollViewer.UnregisterAnchorCandidate(rectangleScrollViewerContent);
                    expectedAnchorCandidatesCount = 0;

                    Log.Comment("Forcing Scroller layout");
                    scroller.InvalidateArrange();
                });

                WaitForEvent("Waiting for AnchorRequested event", scrollViewerAnchorRequestedEvent);
            }
        }

        private void SetupDefaultUI(
            ScrollViewer scrollViewer,
            Rectangle rectangleScrollViewerContent = null,
            AutoResetEvent scrollViewerLoadedEvent = null,
            AutoResetEvent scrollViewerUnloadedEvent = null,
            bool setAsContentRoot = true)
        {
            Log.Comment("Setting up default UI with ScrollViewer" + (rectangleScrollViewerContent == null ? "" : " and Rectangle"));

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            if (rectangleScrollViewerContent != null)
            {
                rectangleScrollViewerContent.Width = c_defaultUIScrollViewerContentWidth;
                rectangleScrollViewerContent.Height = c_defaultUIScrollViewerContentHeight;
                rectangleScrollViewerContent.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scrollViewer);
            scrollViewer.Name = "scrollViewer";
            scrollViewer.Width = c_defaultUIScrollViewerWidth;
            scrollViewer.Height = c_defaultUIScrollViewerHeight;
            if (rectangleScrollViewerContent != null)
            {
                scrollViewer.Content = rectangleScrollViewerContent;
            }

            if (scrollViewerLoadedEvent != null)
            {
                scrollViewer.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollViewer.Loaded event handler");
                    scrollViewerLoadedEvent.Set();
                };
            }

            if (scrollViewerUnloadedEvent != null)
            {
                scrollViewer.Unloaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollViewer.Unloaded event handler");
                    scrollViewerUnloadedEvent.Set();
                };
            }

            if (setAsContentRoot)
            {
                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scrollViewer;
            }
        }

        private void WaitForEvent(string logComment, EventWaitHandle eventWaitHandle)
        {
            Log.Comment(logComment);
            if (!eventWaitHandle.WaitOne(TimeSpan.FromMilliseconds(c_MaxWaitDuration)))
            {
                throw new Exception("Timeout expiration in WaitForEvent.");
            }
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string senderName = string.Empty;

            try
            {
                FrameworkElement fe = sender as FrameworkElement;

                if (fe != null)
                {
                    senderName = "s:" + fe.Name + ", ";
                }
            }
            catch
            {
            }

            if (args.IsVerboseLevel)
            {
                Log.Comment($"  Verbose: {senderName}m:{msg}");
            }
            else
            {
                Log.Comment($"  Info:    {senderName}m:{msg}");
            }
        }
    }

    public struct StateChange
    {
        public StateChange(FrameworkElement templateRoot, string stateName, VisualStateGroup group, VisualState state, bool useTransitions)
        {
            TemplateRoot = templateRoot;
            StateName = stateName;
            Group = group;
            State = state;
            UseTransitions = useTransitions;
        }

        public FrameworkElement TemplateRoot;
        public string StateName;
        public VisualStateGroup Group;
        public VisualState State;
        public bool UseTransitions;
    }

    // Custom VisualStateManager used to record all visual state changes of a ScrollViewer.
    public class CustomScrollViewerVisualStateManager : VisualStateManager
    {
        public CustomScrollViewerVisualStateManager(ScrollViewerWithCustomVisualStateManager customScrollViewer)
        {
            CustomScrollViewer = customScrollViewer;
        }

        private ScrollViewerWithCustomVisualStateManager CustomScrollViewer
        {
            get;
            set;
        }

        protected override bool GoToStateCore(Control control, FrameworkElement templateRoot, string stateName, VisualStateGroup group, VisualState state, bool useTransitions)
        {
            string controlName = control == null ? string.Empty : control.Name;

            Log.Comment($"CustomScrollViewerVisualStateManager control:{controlName}, stateName:{stateName}, group:{group.Name}, useTransitions:{useTransitions}");

            if (CustomScrollViewer != null)
            {
                CustomScrollViewer.OnGoToState(control, templateRoot, stateName, group, state, useTransitions);
            }

            return base.GoToStateCore(control, templateRoot, stateName, group, state, useTransitions);
        }
    }

    // Custom ScrollViewer that records its visual state changes.
    public class ScrollViewerWithCustomVisualStateManager : ScrollViewer
    {
        public ScrollViewerWithCustomVisualStateManager()
        {
            CustomScrollViewerVisualStateManager = new CustomScrollViewerVisualStateManager(this);
            StateChanges = new List<StateChange>();
        }

        public CustomScrollViewerVisualStateManager CustomScrollViewerVisualStateManager
        {
            get;
            private set;
        }

        public List<StateChange> StateChanges
        {
            get;
            private set;
        }

        public void OnGoToState(Control control, FrameworkElement templateRoot, string stateName, VisualStateGroup group, VisualState state, bool useTransitions)
        {
            StateChanges.Add(new StateChange(templateRoot, stateName, group, state, useTransitions));
        }

        protected override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            var layoutRoot = GetTemplateChild("PART_Root") as FrameworkElement;

            if (layoutRoot != null)
            {
                VisualStateManager.SetCustomVisualStateManager(layoutRoot, CustomScrollViewerVisualStateManager);
            }
        }
    }
}
