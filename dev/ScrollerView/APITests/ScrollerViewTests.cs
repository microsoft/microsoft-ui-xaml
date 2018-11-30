// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Threading;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Xaml;
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

#if !BUILD_WINDOWS
using ScrollerView = Microsoft.UI.Xaml.Controls.ScrollerView;
using ScrollerScrollMode = Microsoft.UI.Xaml.Controls.ScrollerScrollMode;
using ScrollerInputKind = Microsoft.UI.Xaml.Controls.ScrollerInputKind;
using ScrollerChainingMode = Microsoft.UI.Xaml.Controls.ScrollerChainingMode;
using ScrollerRailingMode = Microsoft.UI.Xaml.Controls.ScrollerRailingMode;
using ScrollerZoomMode = Microsoft.UI.Xaml.Controls.ScrollerZoomMode;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using ScrollerViewTestHooks = Microsoft.UI.Private.Controls.ScrollerViewTestHooks;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ScrollerViewTests
    {
        private const int c_MaxWaitDuration = 5000;
        private const double c_epsilon = 0.0000001;

        private const ScrollerScrollMode c_defaultComputedHorizontalScrollMode = ScrollerScrollMode.Disabled;
        private const ScrollerScrollMode c_defaultComputedVerticalScrollMode = ScrollerScrollMode.Disabled;
        private const ScrollerInputKind c_defaultInputKind = ScrollerInputKind.All;
        private const ScrollerChainingMode c_defaultHorizontalScrollChainingMode = ScrollerChainingMode.Auto;
        private const ScrollerChainingMode c_defaultVerticalScrollChainingMode = ScrollerChainingMode.Auto;
        private const ScrollerRailingMode c_defaultHorizontalScrollRailingMode = ScrollerRailingMode.Enabled;
        private const ScrollerRailingMode c_defaultVerticalScrollRailingMode = ScrollerRailingMode.Enabled;
        private const ScrollerScrollMode c_defaultHorizontalScrollMode = ScrollerScrollMode.Auto;
        private const ScrollerScrollMode c_defaultVerticalScrollMode = ScrollerScrollMode.Auto;
        private const ScrollerChainingMode c_defaultZoomChainingMode = ScrollerChainingMode.Auto;
        private const ScrollerZoomMode c_defaultZoomMode = ScrollerZoomMode.Disabled;
        private const bool c_defaultIsChildAvailableWidthConstrained = true;
        private const bool c_defaultIsChildAvailableHeightConstrained = false;
        private const bool c_defaultIsAnchoredAtExtent = true;
        private const double c_defaultMinZoomFactor = 0.1;
        private const double c_defaultMaxZoomFactor = 10.0;
        private const double c_defaultAnchorRatio = 0.0;

        private const double c_defaultUIScrollerViewContentWidth = 1200.0;
        private const double c_defaultUIScrollerViewContentHeight = 600.0;
        private const double c_defaultUIScrollerViewWidth = 300.0;
        private const double c_defaultUIScrollerViewHeight = 200.0;

        [TestCleanup]
        public void TestCleanup()
        {
            TestUtilities.ClearVisualTreeRoot();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollerView default properties.")]
        public void VerifyDefaultPropertyValues()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollerView not supported pre-RS2");
                return;
            }

            RunOnUIThread.Execute(() =>
            {
                ScrollerView scrollerView = new ScrollerView();
                Verify.IsNotNull(scrollerView);

                Log.Comment("Verifying ScrollerView default property values");
                Verify.IsNull(scrollerView.Content);
                Verify.IsNull(ScrollerViewTestHooks.GetScrollerPart(scrollerView));
                Verify.IsNull(scrollerView.HorizontalScrollController);
                Verify.IsNull(scrollerView.VerticalScrollController);
                Verify.AreEqual(scrollerView.ComputedHorizontalScrollMode, c_defaultComputedHorizontalScrollMode);
                Verify.AreEqual(scrollerView.ComputedVerticalScrollMode, c_defaultComputedVerticalScrollMode);
                Verify.AreEqual(scrollerView.InputKind, c_defaultInputKind);
                Verify.AreEqual(scrollerView.IsChildAvailableWidthConstrained, c_defaultIsChildAvailableWidthConstrained);
                Verify.AreEqual(scrollerView.IsChildAvailableHeightConstrained, c_defaultIsChildAvailableHeightConstrained);
                Verify.AreEqual(scrollerView.HorizontalScrollChainingMode, c_defaultHorizontalScrollChainingMode);
                Verify.AreEqual(scrollerView.VerticalScrollChainingMode, c_defaultVerticalScrollChainingMode);
                Verify.AreEqual(scrollerView.HorizontalScrollRailingMode, c_defaultHorizontalScrollRailingMode);
                Verify.AreEqual(scrollerView.VerticalScrollRailingMode, c_defaultVerticalScrollRailingMode);
                Verify.AreEqual(scrollerView.HorizontalScrollMode, c_defaultHorizontalScrollMode);
                Verify.AreEqual(scrollerView.VerticalScrollMode, c_defaultVerticalScrollMode);
                Verify.AreEqual(scrollerView.ZoomMode, c_defaultZoomMode);
                Verify.AreEqual(scrollerView.ZoomChainingMode, c_defaultZoomChainingMode);
                Verify.IsGreaterThan(scrollerView.MinZoomFactor, c_defaultMinZoomFactor - c_epsilon);
                Verify.IsLessThan(scrollerView.MinZoomFactor, c_defaultMinZoomFactor + c_epsilon);
                Verify.IsGreaterThan(scrollerView.MaxZoomFactor, c_defaultMaxZoomFactor - c_epsilon);
                Verify.IsLessThan(scrollerView.MaxZoomFactor, c_defaultMaxZoomFactor + c_epsilon);
                Verify.AreEqual(scrollerView.HorizontalAnchorRatio, c_defaultAnchorRatio);
                Verify.AreEqual(scrollerView.VerticalAnchorRatio, c_defaultAnchorRatio);
                Verify.AreEqual(scrollerView.IsAnchoredAtHorizontalExtent, c_defaultIsAnchoredAtExtent);
                Verify.AreEqual(scrollerView.IsAnchoredAtVerticalExtent, c_defaultIsAnchoredAtExtent);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the ScrollerView properties after template application.")]
        public void VerifyScrollerAttachedProperties()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollerView not supported pre-RS2");
                return;
            }

            using (PrivateLoggingHelper privateSVLoggingHelper = new PrivateLoggingHelper("ScrollerView", "Scroller"))
            {
                ScrollerView scrollerView = null;
                Rectangle rectangleScrollerViewContent = null;
                AutoResetEvent scrollerViewLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewUnloadedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollerViewContent = new Rectangle();
                    scrollerView = new ScrollerView();

                    SetupDefaultUI(scrollerView, rectangleScrollerViewContent, scrollerViewLoadedEvent, scrollerViewUnloadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollerViewLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting Scroller-cloned properties to non-default values");
                    scrollerView.InputKind = ScrollerInputKind.MouseWheel | ScrollerInputKind.Pen;
                    scrollerView.IsChildAvailableWidthConstrained = !c_defaultIsChildAvailableWidthConstrained;
                    scrollerView.IsChildAvailableHeightConstrained = !c_defaultIsChildAvailableHeightConstrained;
                    scrollerView.HorizontalScrollChainingMode = ScrollerChainingMode.Always;
                    scrollerView.VerticalScrollChainingMode = ScrollerChainingMode.Never;
                    scrollerView.HorizontalScrollRailingMode = ScrollerRailingMode.Disabled;
                    scrollerView.VerticalScrollRailingMode = ScrollerRailingMode.Disabled;
                    scrollerView.HorizontalScrollMode = ScrollerScrollMode.Enabled;
                    scrollerView.VerticalScrollMode = ScrollerScrollMode.Disabled;
                    scrollerView.ZoomMode = ScrollerZoomMode.Enabled;
                    scrollerView.ZoomChainingMode = ScrollerChainingMode.Never;
                    scrollerView.MinZoomFactor = 2.0;
                    scrollerView.MaxZoomFactor = 8.0;

                    Log.Comment("Verifying Scroller-cloned non-default properties");
                    Verify.AreEqual(scrollerView.InputKind, ScrollerInputKind.MouseWheel | ScrollerInputKind.Pen);
                    Verify.AreEqual(scrollerView.IsChildAvailableWidthConstrained, !c_defaultIsChildAvailableWidthConstrained);
                    Verify.AreEqual(scrollerView.IsChildAvailableHeightConstrained, !c_defaultIsChildAvailableHeightConstrained);
                    Verify.AreEqual(scrollerView.HorizontalScrollChainingMode, ScrollerChainingMode.Always);
                    Verify.AreEqual(scrollerView.VerticalScrollChainingMode, ScrollerChainingMode.Never);
                    Verify.AreEqual(scrollerView.HorizontalScrollRailingMode, ScrollerRailingMode.Disabled);
                    Verify.AreEqual(scrollerView.VerticalScrollRailingMode, ScrollerRailingMode.Disabled);
                    Verify.AreEqual(scrollerView.HorizontalScrollMode, ScrollerScrollMode.Enabled);
                    Verify.AreEqual(scrollerView.VerticalScrollMode, ScrollerScrollMode.Disabled);
                    Verify.AreEqual(scrollerView.ComputedHorizontalScrollMode, ScrollerScrollMode.Enabled);
                    Verify.AreEqual(scrollerView.ComputedVerticalScrollMode, ScrollerScrollMode.Disabled);
                    Verify.AreEqual(scrollerView.ZoomMode, ScrollerZoomMode.Enabled);
                    Verify.AreEqual(scrollerView.ZoomChainingMode, ScrollerChainingMode.Never);
                    Verify.IsGreaterThan(scrollerView.MinZoomFactor, 2.0 - c_epsilon);
                    Verify.IsLessThan(scrollerView.MinZoomFactor, 2.0 + c_epsilon);
                    Verify.IsGreaterThan(scrollerView.MaxZoomFactor, 8.0 - c_epsilon);
                    Verify.IsLessThan(scrollerView.MaxZoomFactor, 8.0 + c_epsilon);

                    Log.Comment("Resetting window content and ScrollerView");
                    MUXControlsTestApp.App.TestContentRoot = null;
                    scrollerView = null;
                });

                WaitForEvent("Waiting for Unloaded event", scrollerViewUnloadedEvent);

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
                Log.Warning("Test is disabled on pre-RS2 because ScrollerView not supported pre-RS2");
                return;
            }

            using (PrivateLoggingHelper privateSVLoggingHelper = new PrivateLoggingHelper("ScrollerView", "Scroller"))
            {
                ScrollerView scrollerView = null;
                Rectangle rectangleScrollerViewContent = null;
                AutoResetEvent scrollerViewLoadedEvent = new AutoResetEvent(false);
                AutoResetEvent scrollerViewUnloadedEvent = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    rectangleScrollerViewContent = new Rectangle();
                    scrollerView = new ScrollerView();

                    SetupDefaultUI(scrollerView, rectangleScrollerViewContent, scrollerViewLoadedEvent, scrollerViewUnloadedEvent);
                });

                WaitForEvent("Waiting for Loaded event", scrollerViewLoadedEvent);

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Verifying ScrollerView property values after Loaded event");
                    Verify.AreEqual(scrollerView.Content, rectangleScrollerViewContent);
                    Verify.IsNotNull(ScrollerViewTestHooks.GetScrollerPart(scrollerView));
                    Verify.AreEqual(ScrollerViewTestHooks.GetScrollerPart(scrollerView).Child, rectangleScrollerViewContent);
                    Verify.IsNotNull(scrollerView.HorizontalScrollController);
                    Verify.IsNotNull(scrollerView.VerticalScrollController);

                    Log.Comment("Resetting window content and ScrollerView");
                    MUXControlsTestApp.App.TestContentRoot = null;
                    scrollerView = null;
                });

                WaitForEvent("Waiting for Unloaded event", scrollerViewUnloadedEvent);

                IdleSynchronizer.Wait();
                Log.Comment("Garbage collecting...");
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                Log.Comment("Done");
            }
        }

        private void SetupDefaultUI(
            ScrollerView scrollerView,
            Rectangle rectangleScrollerViewContent = null,
            AutoResetEvent scrollerViewLoadedEvent = null,
            AutoResetEvent scrollerViewUnloadedEvent = null,
            bool setAsContentRoot = true)
        {
            Log.Comment("Setting up default UI with ScrollerView" + (rectangleScrollerViewContent == null ? "" : " and Rectangle"));

            LinearGradientBrush twoColorLGB = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };

            GradientStop brownGS = new GradientStop() { Color = Colors.Brown, Offset = 0.0 };
            twoColorLGB.GradientStops.Add(brownGS);

            GradientStop orangeGS = new GradientStop() { Color = Colors.Orange, Offset = 1.0 };
            twoColorLGB.GradientStops.Add(orangeGS);

            if (rectangleScrollerViewContent != null)
            {
                rectangleScrollerViewContent.Width = c_defaultUIScrollerViewContentWidth;
                rectangleScrollerViewContent.Height = c_defaultUIScrollerViewContentHeight;
                rectangleScrollerViewContent.Fill = twoColorLGB;
            }

            Verify.IsNotNull(scrollerView);
            scrollerView.Name = "scrollerView";
            scrollerView.Width = c_defaultUIScrollerViewWidth;
            scrollerView.Height = c_defaultUIScrollerViewHeight;
            if (rectangleScrollerViewContent != null)
            {
                scrollerView.Content = rectangleScrollerViewContent;
            }

            if (scrollerViewLoadedEvent != null)
            {
                scrollerView.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Scroller.Loaded event handler");
                    scrollerViewLoadedEvent.Set();
                };
            }

            if (scrollerViewUnloadedEvent != null)
            {
                scrollerView.Unloaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Scroller.Unloaded event handler");
                    scrollerViewUnloadedEvent.Set();
                };
            }

            if (setAsContentRoot)
            {
                Log.Comment("Setting window content");
                MUXControlsTestApp.App.TestContentRoot = scrollerView;
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
                Log.Comment("  Verbose: " + senderName + "m:" + msg);
            }
            else
            {
                Log.Comment("  Info:    " + senderName + "m:" + msg);
            }
        }
    }
}
