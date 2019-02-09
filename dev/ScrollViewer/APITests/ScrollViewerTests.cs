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

using ScrollViewer = Microsoft.UI.Xaml.Controls.ScrollViewer;
using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using InputKind = Microsoft.UI.Xaml.Controls.InputKind;
using ChainingMode = Microsoft.UI.Xaml.Controls.ChainingMode;
using RailingMode = Microsoft.UI.Xaml.Controls.RailingMode;
using ZoomMode = Microsoft.UI.Xaml.Controls.ZoomMode;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using ScrollViewerTestHooks = Microsoft.UI.Private.Controls.ScrollViewerTestHooks;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ScrollViewerTests
    {
        private const int c_MaxWaitDuration = 5000;
        private const double c_epsilon = 0.0000001;

        private const InputKind c_defaultInputKind = InputKind.All;
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
        private const bool c_defaultIsAnchoredAtExtent = true;
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
                Verify.AreEqual(scrollViewer.InputKind, c_defaultInputKind);
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
                Verify.AreEqual(scrollViewer.IsAnchoredAtHorizontalExtent, c_defaultIsAnchoredAtExtent);
                Verify.AreEqual(scrollViewer.IsAnchoredAtVerticalExtent, c_defaultIsAnchoredAtExtent);
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
                    scrollViewer.InputKind = InputKind.MouseWheel | InputKind.Pen;
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
                    Verify.AreEqual(scrollViewer.InputKind, InputKind.MouseWheel | InputKind.Pen);
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
                    Log.Comment("Scroller.Loaded event handler");
                    scrollViewerLoadedEvent.Set();
                };
            }

            if (scrollViewerUnloadedEvent != null)
            {
                scrollViewer.Unloaded += (object sender, RoutedEventArgs e) =>
                {
                    Log.Comment("Scroller.Unloaded event handler");
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
                Log.Comment("  Verbose: " + senderName + "m:" + msg);
            }
            else
            {
                Log.Comment("  Info:    " + senderName + "m:" + msg);
            }
        }
    }
}
