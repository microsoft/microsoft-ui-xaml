// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Private.Controls;
using MUXControlsTestApp.Utilities;
using System;
using System.Threading;
using Windows.Foundation;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Xaml.Media;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using ScrollingView = Microsoft.UI.Xaml.Controls.ScrollingView;
using ScrollBarVisibility = Microsoft.UI.Xaml.Controls.ScrollBarVisibility;
using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using InputKind = Microsoft.UI.Xaml.Controls.InputKind;
using ChainingMode = Microsoft.UI.Xaml.Controls.ChainingMode;
using RailingMode = Microsoft.UI.Xaml.Controls.RailingMode;
using ZoomMode = Microsoft.UI.Xaml.Controls.ZoomMode;
using ScrollingAnchorRequestedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingAnchorRequestedEventArgs;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using ScrollingViewTestHooks = Microsoft.UI.Private.Controls.ScrollingViewTestHooks;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ScrollingViewTests : ApiTestBase
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

        private const double c_defaultUIScrollingViewContentWidth = 1200.0;
        private const double c_defaultUIScrollingViewContentHeight = 600.0;
        private const double c_defaultUIScrollingViewWidth = 300.0;
        private const double c_defaultUIScrollingViewHeight = 200.0;

        private ScrollingViewVisualStateCounts m_scrollingViewVisualStateCounts;

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollingView default properties.")]
        public void VerifyDefaultPropertyValues()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollingView not supported pre-RS2");
                return;
            }

            RunOnUIThread.Execute(() =>
            {
                ScrollingView scrollingView = new ScrollingView();
                Verify.IsNotNull(scrollingView);

                Log.Comment("Verifying ScrollingView default property values");
                Verify.IsNull(scrollingView.Content);
                Verify.IsNull(ScrollingViewTestHooks.GetScrollingPresenterPart(scrollingView));
                Verify.IsNull(scrollingView.HorizontalScrollController);
                Verify.IsNull(scrollingView.VerticalScrollController);
#if USE_SCROLLMODE_AUTO
                Verify.AreEqual(scrollingView.ComputedHorizontalScrollMode, c_defaultComputedHorizontalScrollMode);
                Verify.AreEqual(scrollingView.ComputedVerticalScrollMode, c_defaultComputedVerticalScrollMode);
#endif
                Verify.AreEqual(scrollingView.IgnoredInputKind, c_defaultIgnoredInputKind);
                Verify.AreEqual(scrollingView.ContentOrientation, c_defaultContentOrientation);
                Verify.AreEqual(scrollingView.HorizontalScrollChainingMode, c_defaultHorizontalScrollChainingMode);
                Verify.AreEqual(scrollingView.VerticalScrollChainingMode, c_defaultVerticalScrollChainingMode);
                Verify.AreEqual(scrollingView.HorizontalScrollRailingMode, c_defaultHorizontalScrollRailingMode);
                Verify.AreEqual(scrollingView.VerticalScrollRailingMode, c_defaultVerticalScrollRailingMode);
                Verify.AreEqual(scrollingView.HorizontalScrollMode, c_defaultHorizontalScrollMode);
                Verify.AreEqual(scrollingView.VerticalScrollMode, c_defaultVerticalScrollMode);
                Verify.AreEqual(scrollingView.ZoomMode, c_defaultZoomMode);
                Verify.AreEqual(scrollingView.ZoomChainingMode, c_defaultZoomChainingMode);
                Verify.IsGreaterThan(scrollingView.MinZoomFactor, c_defaultMinZoomFactor - c_epsilon);
                Verify.IsLessThan(scrollingView.MinZoomFactor, c_defaultMinZoomFactor + c_epsilon);
                Verify.IsGreaterThan(scrollingView.MaxZoomFactor, c_defaultMaxZoomFactor - c_epsilon);
                Verify.IsLessThan(scrollingView.MaxZoomFactor, c_defaultMaxZoomFactor + c_epsilon);
                Verify.AreEqual(scrollingView.HorizontalAnchorRatio, c_defaultAnchorRatio);
                Verify.AreEqual(scrollingView.VerticalAnchorRatio, c_defaultAnchorRatio);
                Verify.AreEqual(scrollingView.ExtentWidth, 0.0);
                Verify.AreEqual(scrollingView.ExtentHeight, 0.0);
                Verify.AreEqual(scrollingView.ViewportWidth, 0.0);
                Verify.AreEqual(scrollingView.ViewportHeight, 0.0);
                Verify.AreEqual(scrollingView.ScrollableWidth, 0.0);
                Verify.AreEqual(scrollingView.ScrollableHeight, 0.0);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollingView properties after template application.")]
        public void VerifyScrollingPresenterAttachedProperties()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollingView not supported pre-RS2");
                return;
            }

            using (PrivateLoggingHelper privateSVLoggingHelper = new PrivateLoggingHelper("ScrollingView", "ScrollingPresenter"))
            {
                ScrollingView scrollingView = null;
                Rectangle rectangleScrollingViewContent = null;
                AutoResetEvent scrollingViewLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollingViewUnloadedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollingViewContent = new Rectangle();
                    scrollingView = new ScrollingView();

                    SetupDefaultUI(scrollingView, rectangleScrollingViewContent, scrollingViewLoadedEvent, scrollingViewUnloadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollingViewLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting ScrollingPresenter-cloned properties to non-default values");
                    scrollingView.IgnoredInputKind = InputKind.MouseWheel | InputKind.Pen;
                    scrollingView.ContentOrientation = ContentOrientation.Horizontal;
                    scrollingView.HorizontalScrollChainingMode = ChainingMode.Always;
                    scrollingView.VerticalScrollChainingMode = ChainingMode.Never;
                    scrollingView.HorizontalScrollRailingMode = RailingMode.Disabled;
                    scrollingView.VerticalScrollRailingMode = RailingMode.Disabled;
                    scrollingView.HorizontalScrollMode = ScrollMode.Enabled;
                    scrollingView.VerticalScrollMode = ScrollMode.Disabled;
                    scrollingView.ZoomMode = ZoomMode.Enabled;
                    scrollingView.ZoomChainingMode = ChainingMode.Never;
                    scrollingView.MinZoomFactor = 2.0;
                    scrollingView.MaxZoomFactor = 8.0;

                    Log.Comment("Verifying ScrollingPresenter-cloned non-default properties");
                    Verify.AreEqual(scrollingView.IgnoredInputKind, InputKind.MouseWheel | InputKind.Pen);
                    Verify.AreEqual(scrollingView.ContentOrientation, ContentOrientation.Horizontal);
                    Verify.AreEqual(scrollingView.HorizontalScrollChainingMode, ChainingMode.Always);
                    Verify.AreEqual(scrollingView.VerticalScrollChainingMode, ChainingMode.Never);
                    Verify.AreEqual(scrollingView.HorizontalScrollRailingMode, RailingMode.Disabled);
                    Verify.AreEqual(scrollingView.VerticalScrollRailingMode, RailingMode.Disabled);
                    Verify.AreEqual(scrollingView.HorizontalScrollMode, ScrollMode.Enabled);
                    Verify.AreEqual(scrollingView.VerticalScrollMode, ScrollMode.Disabled);
#if USE_SCROLLMODE_AUTO
                    Verify.AreEqual(scrollingView.ComputedHorizontalScrollMode, ScrollMode.Enabled);
                    Verify.AreEqual(scrollingView.ComputedVerticalScrollMode, ScrollMode.Disabled);
#endif
                    Verify.AreEqual(scrollingView.ZoomMode, ZoomMode.Enabled);
                    Verify.AreEqual(scrollingView.ZoomChainingMode, ChainingMode.Never);
                    Verify.IsGreaterThan(scrollingView.MinZoomFactor, 2.0 - c_epsilon);
                    Verify.IsLessThan(scrollingView.MinZoomFactor, 2.0 + c_epsilon);
                    Verify.IsGreaterThan(scrollingView.MaxZoomFactor, 8.0 - c_epsilon);
                    Verify.IsLessThan(scrollingView.MaxZoomFactor, 8.0 + c_epsilon);

                    Log.Comment("Resetting window content and ScrollingView");
                    Content = null;
                    scrollingView = null;
                });

                WaitForEvent("Waiting for Unloaded event", scrollingViewUnloadedEvent);

                IdleSynchronizer.Wait();
                Log.Comment("Garbage collecting...");
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                Log.Comment("Done");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollingPresenter attached properties.")]
        public void VerifyPropertyValuesAfterTemplateApplication()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollingView not supported pre-RS2");
                return;
            }

            using (PrivateLoggingHelper privateSVLoggingHelper = new PrivateLoggingHelper("ScrollingView", "ScrollingPresenter"))
            {
                ScrollingView scrollingView = null;
                Rectangle rectangleScrollingViewContent = null;
                AutoResetEvent scrollingViewLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollingViewUnloadedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollingViewContent = new Rectangle();
                    scrollingView = new ScrollingView();

                    SetupDefaultUI(scrollingView, rectangleScrollingViewContent, scrollingViewLoadedEvent, scrollingViewUnloadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollingViewLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Verifying ScrollingView property values after Loaded event");
                    Verify.AreEqual(scrollingView.Content, rectangleScrollingViewContent);
                    Verify.IsNotNull(ScrollingViewTestHooks.GetScrollingPresenterPart(scrollingView));
                    Verify.AreEqual(ScrollingViewTestHooks.GetScrollingPresenterPart(scrollingView).Content, rectangleScrollingViewContent);
                    Verify.IsNotNull(scrollingView.HorizontalScrollController);
                    Verify.IsNotNull(scrollingView.VerticalScrollController);
                    Verify.AreEqual(scrollingView.ExtentWidth, c_defaultUIScrollingViewContentWidth);
                    Verify.AreEqual(scrollingView.ExtentHeight, c_defaultUIScrollingViewContentHeight);
                    Verify.AreEqual(scrollingView.ViewportWidth, c_defaultUIScrollingViewWidth);
                    Verify.AreEqual(scrollingView.ViewportHeight, c_defaultUIScrollingViewHeight);
                    Verify.AreEqual(scrollingView.ScrollableWidth, c_defaultUIScrollingViewContentWidth - c_defaultUIScrollingViewWidth);
                    Verify.AreEqual(scrollingView.ScrollableHeight, c_defaultUIScrollingViewContentHeight - c_defaultUIScrollingViewHeight);

                    Log.Comment("Resetting window content and ScrollingView");
                    Content = null;
                    scrollingView = null;
                });

                WaitForEvent("Waiting for Unloaded event", scrollingViewUnloadedEvent);

                IdleSynchronizer.Wait();
                Log.Comment("Garbage collecting...");
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                Log.Comment("Done");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollingView visual state changes based on the AutoHideScrollBars, IsEnabled and ScrollBarVisibility settings.")]
        public void VerifyVisualStates()
        {
            UISettings settings = new UISettings();
            if (!settings.AnimationsEnabled)
            {
                Log.Warning("Test is disabled when animations are turned off.");
                return;
            }

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
            using (PrivateLoggingHelper privateSVLoggingHelper = new PrivateLoggingHelper("ScrollingView"))
            {
                ScrollingView scrollingView = null;

                RunOnUIThread.Execute(() =>
                {
                    MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessageForVisualStateChange;
                    m_scrollingViewVisualStateCounts = new ScrollingViewVisualStateCounts();
                    scrollingView = new ScrollingView();
                });

                using (ScrollingViewTestHooksHelper scrollingViewTestHooksHelper = new ScrollingViewTestHooksHelper(scrollingView, autoHideScrollControllers))
                {
                    Rectangle rectangleScrollingViewContent = null;
                    AutoResetEvent scrollingViewLoadedEvent = new AutoResetEvent(false);
                    AutoResetEvent scrollingViewUnloadedEvent = new AutoResetEvent(false);

                    RunOnUIThread.Execute(() =>
                    {
                        rectangleScrollingViewContent = new Rectangle();
                        scrollingView.HorizontalScrollBarVisibility = scrollBarVisibility;
                        scrollingView.VerticalScrollBarVisibility = scrollBarVisibility;

                        SetupDefaultUI(
                            scrollingView: scrollingView,
                            rectangleScrollingViewContent: rectangleScrollingViewContent,
                            scrollingViewLoadedEvent: scrollingViewLoadedEvent,
                            scrollingViewUnloadedEvent: scrollingViewUnloadedEvent,
                            setAsContentRoot: true,
                            useParentGrid: true);
                    });

                    WaitForEvent("Waiting for Loaded event", scrollingViewLoadedEvent);

                    RunOnUIThread.Execute(() =>
                    {
                        MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessageForVisualStateChange;
                        Log.Comment($"VerifyVisualStates: isEnabled:True, scrollBarVisibility:{scrollBarVisibility}, autoHideScrollControllers:{autoHideScrollControllers}");

                        VerifyVisualStates(
                            expectedMouseIndicatorStateCount: autoHideScrollControllers ? 0u : (scrollBarVisibility == ScrollBarVisibility.Auto ? 2u : 3u),
                            expectedTouchIndicatorStateCount: 0,
                            expectedNoIndicatorStateCount: autoHideScrollControllers ? (scrollBarVisibility == ScrollBarVisibility.Auto ? 2u : 3u) : 0u,
                            expectedScrollBarsSeparatorCollapsedStateCount: autoHideScrollControllers ? (scrollBarVisibility == ScrollBarVisibility.Auto ? 1u : 3u) : 0u,
                            expectedScrollBarsSeparatorCollapsedDisabledStateCount: 0,
                            expectedScrollBarsSeparatorExpandedStateCount: autoHideScrollControllers ? 0u : (scrollBarVisibility == ScrollBarVisibility.Auto ? 1u : 3u),
                            expectedScrollBarsSeparatorDisplayedWithoutAnimationStateCount: 0,
                            expectedScrollBarsSeparatorExpandedWithoutAnimationStateCount: 0,
                            expectedScrollBarsSeparatorCollapsedWithoutAnimationStateCount: 0);

                        m_scrollingViewVisualStateCounts.ResetStateCounts();
                        MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessageForVisualStateChange;

                        Log.Comment("Disabling ScrollingView");
                        scrollingView.IsEnabled = false;
                    });

                    IdleSynchronizer.Wait();

                    RunOnUIThread.Execute(() =>
                    {
                        MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessageForVisualStateChange;
                        Log.Comment($"VerifyVisualStates: isEnabled:False, scrollBarVisibility:{scrollBarVisibility}, autoHideScrollControllers:{autoHideScrollControllers}");

                        VerifyVisualStates(
                            expectedMouseIndicatorStateCount: autoHideScrollControllers ? 0u : (scrollBarVisibility == ScrollBarVisibility.Auto ? 1u : 3u),
                            expectedTouchIndicatorStateCount: 0,
                            expectedNoIndicatorStateCount: autoHideScrollControllers ? (scrollBarVisibility == ScrollBarVisibility.Auto ? 1u : 3u) : 0u,
                            expectedScrollBarsSeparatorCollapsedStateCount: 0,
                            expectedScrollBarsSeparatorCollapsedDisabledStateCount: scrollBarVisibility == ScrollBarVisibility.Auto ? 0u : 3u,
                            expectedScrollBarsSeparatorExpandedStateCount: 0,
                            expectedScrollBarsSeparatorDisplayedWithoutAnimationStateCount: 0,
                            expectedScrollBarsSeparatorExpandedWithoutAnimationStateCount: 0,
                            expectedScrollBarsSeparatorCollapsedWithoutAnimationStateCount: 0);

                        m_scrollingViewVisualStateCounts.ResetStateCounts();
                        MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessageForVisualStateChange;

                        Log.Comment("Enabling ScrollingView");
                        scrollingView.IsEnabled = true;
                    });

                    IdleSynchronizer.Wait();

                    RunOnUIThread.Execute(() =>
                    {
                        MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessageForVisualStateChange;
                        Log.Comment($"VerifyVisualStates: isEnabled:True, scrollBarVisibility:{scrollBarVisibility}, autoHideScrollControllers:{autoHideScrollControllers}");

                        VerifyVisualStates(
                            expectedMouseIndicatorStateCount: autoHideScrollControllers ? 0u : 3u,
                            expectedTouchIndicatorStateCount: 0,
                            expectedNoIndicatorStateCount: autoHideScrollControllers ? 3u : 0u,
                            expectedScrollBarsSeparatorCollapsedStateCount: autoHideScrollControllers ? (scrollBarVisibility == ScrollBarVisibility.Auto ? 2u : 3u) : 0u,
                            expectedScrollBarsSeparatorCollapsedDisabledStateCount: 0,
                            expectedScrollBarsSeparatorExpandedStateCount: autoHideScrollControllers ? 0u : (scrollBarVisibility == ScrollBarVisibility.Auto ? 2u : 3u),
                            expectedScrollBarsSeparatorDisplayedWithoutAnimationStateCount: 0,
                            expectedScrollBarsSeparatorExpandedWithoutAnimationStateCount: 0,
                            expectedScrollBarsSeparatorCollapsedWithoutAnimationStateCount: 0);

                        Log.Comment("Resetting window content");
                        Content = null;
                        m_scrollingViewVisualStateCounts = null;
                    });

                    WaitForEvent("Waiting for Unloaded event", scrollingViewUnloadedEvent);
                }
            }
        }

        private void MUXControlsTestHooks_LoggingMessageForVisualStateChange(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            if (args.IsVerboseLevel)
            {
                if (args.Message.Contains("ScrollingView::GoToState"))
                {
                    if (args.Message.Contains("NoIndicator"))
                    {
                        m_scrollingViewVisualStateCounts.NoIndicatorStateCount++;
                    }
                    else if (args.Message.Contains("TouchIndicator"))
                    {
                        m_scrollingViewVisualStateCounts.TouchIndicatorStateCount++;
                    }
                    else if (args.Message.Contains("MouseIndicator"))
                    {
                        m_scrollingViewVisualStateCounts.MouseIndicatorStateCount++;
                    }
                    else if (args.Message.Contains("ScrollBarsSeparatorCollapsedDisabled"))
                    {
                        m_scrollingViewVisualStateCounts.ScrollBarsSeparatorCollapsedDisabledStateCount++;
                    }
                    else if (args.Message.Contains("ScrollBarsSeparatorCollapsedWithoutAnimation"))
                    {
                        m_scrollingViewVisualStateCounts.ScrollBarsSeparatorCollapsedWithoutAnimationStateCount++;
                    }
                    else if (args.Message.Contains("ScrollBarsSeparatorDisplayedWithoutAnimation"))
                    {
                        m_scrollingViewVisualStateCounts.ScrollBarsSeparatorDisplayedWithoutAnimationStateCount++;
                    }
                    else if (args.Message.Contains("ScrollBarsSeparatorExpandedWithoutAnimation"))
                    {
                        m_scrollingViewVisualStateCounts.ScrollBarsSeparatorExpandedWithoutAnimationStateCount++;
                    }
                    else if (args.Message.Contains("ScrollBarsSeparatorCollapsed"))
                    {
                        m_scrollingViewVisualStateCounts.ScrollBarsSeparatorCollapsedStateCount++;
                    }
                    else if (args.Message.Contains("ScrollBarsSeparatorExpanded"))
                    {
                        m_scrollingViewVisualStateCounts.ScrollBarsSeparatorExpandedStateCount++;
                    }
                }
            }
        }

        private void VerifyVisualStates(
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
            Log.Comment($"expectedMouseIndicatorStateCount:{expectedMouseIndicatorStateCount}, mouseIndicatorStateCount:{m_scrollingViewVisualStateCounts.MouseIndicatorStateCount}");
            Log.Comment($"expectedNoIndicatorStateCount:{expectedNoIndicatorStateCount}, noIndicatorStateCount:{m_scrollingViewVisualStateCounts.NoIndicatorStateCount}");
            Log.Comment($"expectedScrollBarsSeparatorCollapsedStateCount:{expectedScrollBarsSeparatorCollapsedStateCount}, scrollBarsSeparatorCollapsedStateCount:{m_scrollingViewVisualStateCounts.ScrollBarsSeparatorCollapsedStateCount}");
            Log.Comment($"expectedScrollBarsSeparatorCollapsedDisabledStateCount:{expectedScrollBarsSeparatorCollapsedDisabledStateCount}, scrollBarsSeparatorCollapsedDisabledStateCount:{m_scrollingViewVisualStateCounts.ScrollBarsSeparatorCollapsedDisabledStateCount}");
            Log.Comment($"expectedScrollBarsSeparatorExpandedStateCount:{expectedScrollBarsSeparatorExpandedStateCount}, scrollBarsSeparatorExpandedStateCount:{m_scrollingViewVisualStateCounts.ScrollBarsSeparatorExpandedStateCount}");

            Verify.AreEqual(expectedMouseIndicatorStateCount, m_scrollingViewVisualStateCounts.MouseIndicatorStateCount);
            Verify.AreEqual(expectedTouchIndicatorStateCount, m_scrollingViewVisualStateCounts.TouchIndicatorStateCount);
            Verify.AreEqual(expectedNoIndicatorStateCount, m_scrollingViewVisualStateCounts.NoIndicatorStateCount);
            Verify.AreEqual(expectedScrollBarsSeparatorCollapsedStateCount, m_scrollingViewVisualStateCounts.ScrollBarsSeparatorCollapsedStateCount);
            Verify.AreEqual(expectedScrollBarsSeparatorCollapsedDisabledStateCount, m_scrollingViewVisualStateCounts.ScrollBarsSeparatorCollapsedDisabledStateCount);
            Verify.AreEqual(expectedScrollBarsSeparatorExpandedStateCount, m_scrollingViewVisualStateCounts.ScrollBarsSeparatorExpandedStateCount);
            Verify.AreEqual(expectedScrollBarsSeparatorDisplayedWithoutAnimationStateCount, m_scrollingViewVisualStateCounts.ScrollBarsSeparatorDisplayedWithoutAnimationStateCount);
            Verify.AreEqual(expectedScrollBarsSeparatorExpandedWithoutAnimationStateCount, m_scrollingViewVisualStateCounts.ScrollBarsSeparatorExpandedWithoutAnimationStateCount);
            Verify.AreEqual(expectedScrollBarsSeparatorCollapsedWithoutAnimationStateCount, m_scrollingViewVisualStateCounts.ScrollBarsSeparatorCollapsedWithoutAnimationStateCount);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies anchor candidate registration and unregistration.")]
        public void VerifyAnchorCandidateRegistration()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollingView not supported pre-RS2");
                return;
            }

            using (PrivateLoggingHelper privateSVLoggingHelper = new PrivateLoggingHelper("ScrollingView", "ScrollingPresenter"))
            {
                int expectedAnchorCandidatesCount = 0;
                ScrollingPresenter scrollingPresenter = null;
                ScrollingView scrollingView = null;
                Rectangle rectangleScrollingViewContent = null;
                AutoResetEvent scrollingViewLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollingViewAnchorRequestedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollingViewContent = new Rectangle();
                    scrollingView = new ScrollingView();
                    scrollingView.HorizontalAnchorRatio = 0.1;

                    SetupDefaultUI(scrollingView, rectangleScrollingViewContent, scrollingViewLoadedEvent);

                    scrollingView.AnchorRequested += (ScrollingView sender, ScrollingAnchorRequestedEventArgs args) =>
                    {
                        Log.Comment("ScrollingView.AnchorRequested event handler. args.AnchorCandidates.Count: " + args.AnchorCandidates.Count);
                        Verify.IsNull(args.AnchorElement);
                        Verify.AreEqual(expectedAnchorCandidatesCount, args.AnchorCandidates.Count);
                        scrollingViewAnchorRequestedEvent.Set();
                    };
                });

                WaitForEvent("Waiting for Loaded event", scrollingViewLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Accessing inner ScrollingPresenter control");
                    scrollingPresenter = ScrollingViewTestHooks.GetScrollingPresenterPart(scrollingView);

                    Log.Comment("Registering Rectangle as anchor candidate");
                    scrollingView.RegisterAnchorCandidate(rectangleScrollingViewContent);
                    expectedAnchorCandidatesCount = 1;

                    Log.Comment("Forcing ScrollingPresenter layout");
                    scrollingPresenter.InvalidateArrange();
                });

                WaitForEvent("Waiting for AnchorRequested event", scrollingViewAnchorRequestedEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Unregistering Rectangle as anchor candidate");
                    scrollingView.UnregisterAnchorCandidate(rectangleScrollingViewContent);
                    expectedAnchorCandidatesCount = 0;

                    Log.Comment("Forcing ScrollingPresenter layout");
                    scrollingPresenter.InvalidateArrange();
                });

                WaitForEvent("Waiting for AnchorRequested event", scrollingViewAnchorRequestedEvent);
            }
        }

        private void SetupDefaultUI(
            ScrollingView scrollingView,
            Rectangle rectangleScrollingViewContent = null,
            AutoResetEvent scrollingViewLoadedEvent = null,
            AutoResetEvent scrollingViewUnloadedEvent = null,
            bool setAsContentRoot = true,
            bool useParentGrid = false)
        {
            Log.Comment("Setting up default UI with ScrollingView" + (rectangleScrollingViewContent == null ? "" : " and Rectangle"));

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            if (rectangleScrollingViewContent != null)
            {
                rectangleScrollingViewContent.Width = c_defaultUIScrollingViewContentWidth;
                rectangleScrollingViewContent.Height = c_defaultUIScrollingViewContentHeight;
                rectangleScrollingViewContent.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scrollingView);
            scrollingView.Name = "scrollingView";
            scrollingView.Width = c_defaultUIScrollingViewWidth;
            scrollingView.Height = c_defaultUIScrollingViewHeight;
            if (rectangleScrollingViewContent != null)
            {
                scrollingView.Content = rectangleScrollingViewContent;
            }

            if (scrollingViewLoadedEvent != null)
            {
                scrollingView.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollingView.Loaded event handler");
                    scrollingViewLoadedEvent.Set();
                };
            }

            if (scrollingViewUnloadedEvent != null)
            {
                scrollingView.Unloaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("ScrollingView.Unloaded event handler");
                    scrollingViewUnloadedEvent.Set();
                };
            }

            Grid parentGrid = null;

            if (useParentGrid)
            {
                parentGrid = new Grid();
                parentGrid.Width = c_defaultUIScrollingViewWidth * 3;
                parentGrid.Height = c_defaultUIScrollingViewHeight * 3;

                scrollingView.HorizontalAlignment = HorizontalAlignment.Left;
                scrollingView.VerticalAlignment = VerticalAlignment.Top;

                parentGrid.Children.Add(scrollingView);
            }

            if (setAsContentRoot)
            {
                Log.Comment("Setting window content");
                if (useParentGrid)
                {
                    Content = parentGrid;
                }
                else
                {
                    Content = scrollingView;
                }
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
    }

    // Custom ScrollingView that records its visual state changes.
    public class ScrollingViewVisualStateCounts
    {
        public uint NoIndicatorStateCount
        {
            get;
            set;
        }

        public uint TouchIndicatorStateCount
        {
            get;
            set;
        }

        public uint MouseIndicatorStateCount
        {
            get;
            set;
        }

        public uint ScrollBarsSeparatorExpandedStateCount
        {
            get;
            set;
        }

        public uint ScrollBarsSeparatorCollapsedStateCount
        {
            get;
            set;
        }

        public uint ScrollBarsSeparatorCollapsedDisabledStateCount
        {
            get;
            set;
        }

        public uint ScrollBarsSeparatorCollapsedWithoutAnimationStateCount
        {
            get;
            set;
        }

        public uint ScrollBarsSeparatorDisplayedWithoutAnimationStateCount
        {
            get;
            set;
        }

        public uint ScrollBarsSeparatorExpandedWithoutAnimationStateCount
        {
            get;
            set;
        }

        public void ResetStateCounts()
        {
            NoIndicatorStateCount = 0;
            TouchIndicatorStateCount = 0;
            MouseIndicatorStateCount = 0;
            ScrollBarsSeparatorExpandedStateCount = 0;
            ScrollBarsSeparatorCollapsedStateCount = 0;
            ScrollBarsSeparatorCollapsedDisabledStateCount = 0;
            ScrollBarsSeparatorCollapsedWithoutAnimationStateCount = 0;
            ScrollBarsSeparatorDisplayedWithoutAnimationStateCount = 0;
            ScrollBarsSeparatorExpandedWithoutAnimationStateCount = 0;
        }
    }
}
