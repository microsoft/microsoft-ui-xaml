// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Linq;
using Windows.Foundation;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Private.Controls;
using System.Threading;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.Mocks;
using System.Numerics;
using Common;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml.Media;
using Windows.UI;
using System.Runtime.InteropServices.WindowsRuntime;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;

using Task = System.Threading.Tasks.Task;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class ViewportTests : ApiTestBase
    {
        [TestMethod]
        public void ValidateNoScrollingSurfaceScenario()
        {
            ValidateNoScrollingSurfaceScenario(hostItemsRepeaterInPopup: false);
        }

        [TestMethod]
        public void ValidateNoScrollingSurfaceInPopupScenario()
        {
            ValidateNoScrollingSurfaceScenario(hostItemsRepeaterInPopup: true);
        }

        private void ValidateNoScrollingSurfaceScenario(bool hostItemsRepeaterInPopup)
        {
            RunOnUIThread.Execute(() =>
            {
                var realizationRects = new List<Rect>();
                var visibleRects = new List<Rect>();

                var repeater = new ItemsRepeater()
                {
                    Layout = GetMonitoringLayout(new Size(500, 500), realizationRects, visibleRects),
                    HorizontalCacheLength = 0.0,
                    VerticalCacheLength = 0.0
                };

                if (hostItemsRepeaterInPopup)
                {
                    var popup = new Popup()
                    {
                        Child = repeater
                    };

                    Content = popup;
                    Content.UpdateLayout();

                    popup.IsOpen = true;
                    Content.UpdateLayout();

                    popup.IsOpen = false;
                }
                else
                {
                    Content = repeater;
                }
                
                Content.UpdateLayout();

                foreach (Rect rect in realizationRects)
                {
                    Log.Comment("RealizationRect: {0}", rect);
                }

                foreach (Rect rect in visibleRects)
                {
                    Log.Comment("VisibleRects: {0}", rect);
                }

                Verify.AreEqual(2, realizationRects.Count);
                Verify.AreEqual(2, visibleRects.Count);

                Verify.AreEqual(new Rect(0, 0, 0, 0), realizationRects[0]);
                Verify.AreEqual(realizationRects[0], visibleRects[0]);

                // Using Effective Viewport
                Verify.AreEqual(0, realizationRects[1].X);
                // 32 pixel title bar and some tolerance for borders
                Verify.IsLessThan(Math.Abs(realizationRects[1].Y + 32), 6.0);
                // Width/Height depends on the window size, so just
                // validating something reasonable here to avoid flakiness.
                Verify.IsLessThan(500.0, realizationRects[1].Width);
                Verify.IsLessThan(500.0, realizationRects[1].Height);

                Verify.AreEqual(realizationRects[1], visibleRects[1]);

                realizationRects.Clear();
                visibleRects.Clear();
            });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125 and internal issue 18866003
        public void ValidateItemsRepeaterScrollHostScenario()
        {
            var realizationRects = new List<Rect>();
            var scrollViewer = (ScrollViewer)null;
            var viewChangedEvent = new ManualResetEvent(false);
            int waitTime = 2000; // 2 seconds 

            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater() {
                    Layout = GetMonitoringLayout(new Size(500, 600), realizationRects, null /* visibleRects */),
                    HorizontalCacheLength = 0.0,
                    VerticalCacheLength = 0.0
                };

                scrollViewer = new ScrollViewer {
                    Content = repeater,
                    HorizontalScrollBarVisibility = ScrollBarVisibility.Hidden,
                    VerticalScrollBarVisibility = ScrollBarVisibility.Hidden,
                };

                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChangedEvent.Set();
                    }
                };

                var tracker = new ItemsRepeaterScrollHost() {
                    Width = 200,
                    Height = 300,
                    ScrollViewer = scrollViewer
                };

                Content = tracker;
                Content.UpdateLayout();

                // First layout pass will invalidate measure during the first arrange
                // so that we can get a viewport during the second measure/arrange pass.
                // That's why Measure gets invoked twice.
                // After that, ScrollViewer.SizeChanged is raised and it also invalidates
                // layout (third pass).
                Verify.AreEqual(3, realizationRects.Count);
                Verify.AreEqual(new Rect(0, 0, 0, 0), realizationRects[0]);
                Verify.AreEqual(new Rect(0, 0, 200, 300), realizationRects[1]);
                Verify.AreEqual(new Rect(0, 0, 200, 300), realizationRects[2]);
                realizationRects.Clear();

                viewChangedEvent.Reset();
                scrollViewer.ChangeView(null, 100.0, 1.0f, disableAnimation: true);
            });

            Verify.IsTrue(viewChangedEvent.WaitOne(waitTime), "Waiting for view changed");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(0, 100, 200, 300), realizationRects.Last());
                realizationRects.Clear();

                viewChangedEvent.Reset();
                // Max viewport offset is (300, 400). Horizontal viewport offset
                // is expected to get coerced from 400 to 300.
                scrollViewer.ChangeView(400, 100.0, 1.0f, disableAnimation: true);
            });

            Verify.IsTrue(viewChangedEvent.WaitOne(waitTime), "Waiting for view changed");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(300, 100, 200, 300), realizationRects.Last());
                realizationRects.Clear();

                viewChangedEvent.Reset();
                scrollViewer.ChangeView(null, null, 2.0f, disableAnimation: true);
            });

            Verify.IsTrue(viewChangedEvent.WaitOne(waitTime), "Waiting for view changed");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(300, 100, 100, 150), realizationRects.Last());
                realizationRects.Clear();
            });
        }

        [TestMethod]
        public void ValidateOneScrollPresenterScenario()
        {
            var visibleRects = new List<Rect>();
            var realizationRects = new List<Rect>();
            var scrollPresenter = (ScrollPresenter)null;
            var scrollCompletedEvent = new AutoResetEvent(false);
            var zoomCompletedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater() {
                    Layout = GetMonitoringLayout(new Size(500, 600), realizationRects, visibleRects),
                    HorizontalCacheLength = 0.0,
                    VerticalCacheLength = 0.0
                };

                scrollPresenter = new ScrollPresenter {
                    Content = repeater,
                    Width = 200,
                    Height = 300
                };

                Content = scrollPresenter;
                Content.UpdateLayout();

                Verify.AreEqual(2, visibleRects.Count);
                Verify.AreEqual(new Rect(0, 0, 0, 0), visibleRects[0]);
                Verify.AreEqual(new Rect(0, 0, 200, 300), visibleRects[1]);
                visibleRects.Clear();

                Verify.AreEqual(2, realizationRects.Count);
                Verify.AreEqual(new Rect(0, 0, 0, 0), realizationRects[0]);
                Verify.AreEqual(new Rect(0, 0, 200, 300), realizationRects[1]);
                realizationRects.Clear();

                scrollPresenter.ScrollCompleted += (ScrollPresenter sender, ScrollingScrollCompletedEventArgs args) =>
                {
                    scrollCompletedEvent.Set();
                };

                scrollPresenter.ZoomCompleted += (ScrollPresenter sender, ScrollingZoomCompletedEventArgs args) =>
                {
                    zoomCompletedEvent.Set();
                };
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                scrollPresenter.ScrollTo(0.0, 100.0, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
            });
            Verify.IsTrue(scrollCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            CompositionPropertySpy.SynchronouslyTickUIThread(1);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(0, 100, 200, 300), visibleRects.Last());
                visibleRects.Clear();
                Verify.AreEqual(new Rect(0, 100, 200, 300), realizationRects.Last());
                realizationRects.Clear();

                scrollCompletedEvent.Reset();
                // Max viewport offset is (300, 400). Horizontal viewport offset
                // is expected to get coerced from 400 to 300.
                scrollPresenter.ScrollTo(400.0, 100.0, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
            });
            Verify.IsTrue(scrollCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            CompositionPropertySpy.SynchronouslyTickUIThread(1);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(300, 100, 200, 300), visibleRects.Last());
                visibleRects.Clear();

                Verify.AreEqual(new Rect(300, 100, 200, 300), realizationRects.Last());
                realizationRects.Clear();

                scrollPresenter.ZoomTo(
                    2.0f,
                    Vector2.Zero,
                    new ScrollingZoomOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
            });
            Verify.IsTrue(zoomCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            CompositionPropertySpy.SynchronouslyTickUIThread(1);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(
                    new Rect(300, 100, 100, 150),
                    realizationRects.Last());
                realizationRects.Clear();

                // There are some known differences in InteractionTracker zoom between RS1 and RS2.
                Verify.AreEqual(
                    PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2) ?
                    new Rect(300, 100, 100, 150) :
                    new Rect(150, 100, 100, 150),
                    visibleRects.Last());
                visibleRects.Clear();
            });
        }

        [TestMethod]
        public void ValidateTwoScrollPresentersScenario()
        {
            var visibleRects = new List<Rect>();
            var realizationRects = new List<Rect>();
            var horizontalScrollPresenter = (ScrollPresenter)null;
            var verticalScrollPresenter = (ScrollPresenter)null;
            var horizontalScrollCompletedEvent = new AutoResetEvent(false);
            var verticalScrollCompletedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater() {
                    Layout = GetMonitoringLayout(new Size(500, 500), realizationRects, visibleRects),
                    HorizontalCacheLength = 0.0,
                    VerticalCacheLength = 0.0
                };

                horizontalScrollPresenter = new ScrollPresenter {
                    Content = repeater,
                    ContentOrientation = ScrollingContentOrientation.Horizontal
                };

                var grid = new Grid();
                grid.Children.Add(horizontalScrollPresenter);

                verticalScrollPresenter = new ScrollPresenter {
                    Content = grid,
                    Width = 200,
                    Height = 200,
                    ContentOrientation = ScrollingContentOrientation.Vertical
                };

                Content = verticalScrollPresenter;
                Content.UpdateLayout();

                Verify.AreEqual(2, visibleRects.Count);
                Verify.AreEqual(new Rect(0, 0, 0, 0), visibleRects[0]);
                Verify.AreEqual(new Rect(0, 0, 200, 200), visibleRects[1]);
                visibleRects.Clear();

                Verify.AreEqual(2, realizationRects.Count);
                Verify.AreEqual(new Rect(0, 0, 0, 0), realizationRects[0]);
                Verify.AreEqual(new Rect(0, 0, 200, 200), realizationRects[1]);
                realizationRects.Clear();

                horizontalScrollPresenter.ScrollCompleted += (ScrollPresenter sender, ScrollingScrollCompletedEventArgs args) =>
                {
                    horizontalScrollCompletedEvent.Set();
                };

                verticalScrollPresenter.ScrollCompleted += (ScrollPresenter sender, ScrollingScrollCompletedEventArgs args) =>
                {
                    verticalScrollCompletedEvent.Set();
                };
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                verticalScrollPresenter.ScrollTo(0.0, 100.0, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
            });
            Verify.IsTrue(verticalScrollCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            CompositionPropertySpy.SynchronouslyTickUIThread(1);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(0, 100, 200, 200), visibleRects.Last());
                visibleRects.Clear();

                Verify.AreEqual(new Rect(0, 100, 200, 200), realizationRects.Last());
                realizationRects.Clear();

                // Max viewport offset is (300, 300). Horizontal viewport offset
                // is expected to get coerced from 400 to 300.
                horizontalScrollPresenter.ScrollTo(400.0, 100.0, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
            });
            Verify.IsTrue(horizontalScrollCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            CompositionPropertySpy.SynchronouslyTickUIThread(1);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(300, 100, 200, 200), visibleRects.Last());
                visibleRects.Clear();

                Verify.AreEqual(new Rect(300, 100, 200, 200), realizationRects.Last());
                realizationRects.Clear();
            });
        }

        [TestMethod]
        public void CanGrowCacheBuffer()
        {
            var scrollPresenter = (ScrollPresenter)null;
            var repeater = (ItemsRepeater)null;
            var measureVisibleRects = new List<Rect>();
            var measureRealizationRects = new List<Rect>();
            var arrangeVisibleRects = new List<Rect>();
            var arrangeRealizationRects = new List<Rect>();
            var fullCacheEvent = new ManualResetEvent(initialState: false);
            bool fullCacheReached = false;

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Preparing the visual tree...");

                scrollPresenter = new ScrollPresenter {
                    Width = 400,
                    Height = 400
                };

                var layout = new MockVirtualizingLayout {
                    MeasureLayoutFunc = (availableSize, context) =>
                    {
                        if (fullCacheReached)
                        {
                            Log.Comment("Measure pass - full cache already reached.");
                        }
                        else
                        {
                            Log.Comment("Measure pass - adding VisibleRect: {0}, RealizationRect: {1}", context.VisibleRect, context.RealizationRect);

                            measureVisibleRects.Add(context.VisibleRect);
                            measureRealizationRects.Add(context.RealizationRect);
                        }

                        return new Size(1000, 2000);
                    },

                    ArrangeLayoutFunc = (finalSize, context) =>
                    {
                        if (fullCacheReached)
                        {
                            Log.Comment("Arrange pass - full cache already reached.");
                        }
                        else
                        {
                            Log.Comment("Arrange pass - adding VisibleRect: {0}, RealizationRect: {1}", context.VisibleRect, context.RealizationRect);

                            arrangeVisibleRects.Add(context.VisibleRect);
                            arrangeRealizationRects.Add(context.RealizationRect);

                            if (context.RealizationRect.Height == scrollPresenter.Height * (repeater.VerticalCacheLength + 1))
                            {
                                fullCacheReached = true;
                                fullCacheEvent.Set();
                            }
                        }

                        return finalSize;
                    }
                };

                repeater = new ItemsRepeater() {
                    Layout = layout
                };

                scrollPresenter.Content = repeater;
                Content = scrollPresenter;
            });

            if (!fullCacheEvent.WaitOne(500000)) Verify.Fail("Cache full size never reached.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var cacheLength = repeater.VerticalCacheLength;
                var expectedVisibleWindow = new Rect(0, 0, scrollPresenter.Width, scrollPresenter.Height);
                var expectedRealizationWindow = new Rect(
                    -cacheLength / 2 * scrollPresenter.Width,
                    -cacheLength / 2 * scrollPresenter.Height,
                    (1 + cacheLength) * scrollPresenter.Width,
                    (1 + cacheLength) * scrollPresenter.Height);

                Log.Comment("Validate that the realization window reached full size. Measure passes: {0}, Arrange passes: {1}", measureRealizationRects.Count, arrangeRealizationRects.Count);
                Log.Comment("expectedRealizationWindow: {0}, measureRealizationRects.Last(): {1}", expectedRealizationWindow, measureRealizationRects.Last());
                Log.Comment("expectedRealizationWindow: {0}, arrangeRealizationRects.Last(): {1}", expectedRealizationWindow, arrangeRealizationRects.Last());
                Verify.AreEqual(expectedRealizationWindow, measureRealizationRects.Last());
                Verify.AreEqual(expectedRealizationWindow, arrangeRealizationRects.Last());

                Log.Comment("Validate that the realization window grew by 40 pixels each time during the process.");
                for (int i = 2; i < measureRealizationRects.Count; ++i)
                {
                    Log.Comment("measureRealizationRects.X delta: expected: -40, actual: {0}", measureRealizationRects[i].X - measureRealizationRects[i - 1].X);
                    Log.Comment("measureRealizationRects.Y delta: expected: -40, actual: {0}", measureRealizationRects[i].Y - measureRealizationRects[i - 1].Y);
                    Log.Comment("measureRealizationRects.Width delta: expected: 80, actual: {0}", measureRealizationRects[i].Width - measureRealizationRects[i - 1].Width);
                    Log.Comment("measureRealizationRects.Height delta: expected: 80, actual: {0}", measureRealizationRects[i].Height - measureRealizationRects[i - 1].Height);

                    Verify.IsLessThan(Math.Abs(measureRealizationRects[i].X - measureRealizationRects[i - 1].X + 40), 0.001);
                    Verify.IsLessThan(Math.Abs(measureRealizationRects[i].Y - measureRealizationRects[i - 1].Y + 40), 0.001);
                    Verify.IsLessThan(Math.Abs(measureRealizationRects[i].Width - measureRealizationRects[i - 1].Width - 80), 0.001);
                    Verify.IsLessThan(Math.Abs(measureRealizationRects[i].Height - measureRealizationRects[i - 1].Height - 80), 0.001);

                    Log.Comment("arrangeRealizationRects.X delta: expected: -40, actual: {0}", arrangeRealizationRects[i].X - arrangeRealizationRects[i - 1].X);
                    Log.Comment("arrangeRealizationRects.Y delta: expected: -40, actual: {0}", arrangeRealizationRects[i].Y - arrangeRealizationRects[i - 1].Y);
                    Log.Comment("arrangeRealizationRects.Width delta: expected: 80, actual: {0}", arrangeRealizationRects[i].Width - arrangeRealizationRects[i - 1].Width);
                    Log.Comment("arrangeRealizationRects.Height delta: expected: 80, actual: {0}", arrangeRealizationRects[i].Height - arrangeRealizationRects[i - 1].Height);

                    Verify.IsLessThan(Math.Abs(arrangeRealizationRects[i].X - arrangeRealizationRects[i - 1].X + 40), 0.001);
                    Verify.IsLessThan(Math.Abs(arrangeRealizationRects[i].Y - arrangeRealizationRects[i - 1].Y + 40), 0.001);
                    Verify.IsLessThan(Math.Abs(arrangeRealizationRects[i].Width - arrangeRealizationRects[i - 1].Width - 80), 0.001);
                    Verify.IsLessThan(Math.Abs(arrangeRealizationRects[i].Height - arrangeRealizationRects[i - 1].Height - 80), 0.001);
                }

                // Skipping first recording measureVisibleRects[0] since both the realization and visible rects are still empty at that point.
                Log.Comment("Validate expected measure visible rects");
                for (int i = 1; i < measureVisibleRects.Count; ++i)
                {
                    Log.Comment("expectedVisibleWindow: {0}, visibleRect: {1}", expectedVisibleWindow, measureVisibleRects[i]);
                    Verify.AreEqual(expectedVisibleWindow, measureVisibleRects[i]);
                }

                // Skipping first recording arrangeVisibleRects[0] since both the realization and visible rects are still empty at that point.
                Log.Comment("Validate expected arrange visible rects");
                for (int i = 1; i < arrangeVisibleRects.Count; ++i)
                {
                    Log.Comment("expectedVisibleWindow: {0}, visibleRect: {1}", expectedVisibleWindow, arrangeVisibleRects[i]);
                    Verify.AreEqual(expectedVisibleWindow, arrangeVisibleRects[i]);
                }
            });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125 and internal issue 18866003
        public void ValidateLoadUnload()
        {
            // In this test, we will repeatedly put a repeater in and out
            // of the visual tree, under the same or a different parent.
            // And we will validate that the subscriptions and unsubscriptions to
            // the IRepeaterScrollingSurface events is done in sync.

            TestScrollingSurface scroller1 = null;
            TestScrollingSurface scroller2 = null;
            ItemsRepeater repeater = null;
            WeakReference repeaterWeakRef = null;
            var renderingEvent = new ManualResetEvent(false);

            var unorderedLoadEvent = false;
            var loadCounter = 0;
            var unloadCounter = 0;

            int scroller1SubscriberCount = 0;
            int scroller2SubscriberCount = 0;

            RunOnUIThread.Execute(() =>
            {
                CompositionTarget.Rendering += (sender, args) =>
                {
                    renderingEvent.Set();
                };

                var host = new Grid();
                scroller1 = new TestScrollingSurface() { Name = "Scroller 1" };
                scroller2 = new TestScrollingSurface() { Name = "Scroller 2" };
                repeater = new ItemsRepeater();
                repeaterWeakRef = new WeakReference(repeater);

                repeater.Loaded += delegate {
                    Log.Comment("ItemsRepeater Loaded in " + ((FrameworkElement)repeater.Parent).Name);
                    unorderedLoadEvent |= (++loadCounter > unloadCounter + 1);
                };
                repeater.Unloaded += delegate {
                    Log.Comment("ItemsRepeater Unloaded");
                    unorderedLoadEvent |= (++unloadCounter > loadCounter);
                };

                // Subscribers count should never go above 1 or under 0.
                var validateSubscriberCount = new Action(() =>
                {
                    Verify.IsLessThanOrEqual(scroller1SubscriberCount, 1);
                    Verify.IsGreaterThanOrEqual(scroller1SubscriberCount, 0);

                    Verify.IsLessThanOrEqual(scroller2SubscriberCount, 1);
                    Verify.IsGreaterThanOrEqual(scroller2SubscriberCount, 0);
                });

                scroller1.ConfigurationChangedAddFunc = () => { ++scroller1SubscriberCount; validateSubscriberCount(); };
                scroller2.ConfigurationChangedAddFunc = () => { ++scroller2SubscriberCount; validateSubscriberCount(); };
                scroller1.ConfigurationChangedRemoveFunc = () => { --scroller1SubscriberCount; validateSubscriberCount(); };
                scroller2.ConfigurationChangedRemoveFunc = () => { --scroller2SubscriberCount; validateSubscriberCount(); };

                scroller1.Content = repeater;
                host.Children.Add(scroller1);
                host.Children.Add(scroller2);

                Content = host;
            });

            IdleSynchronizer.Wait();
            Verify.IsTrue(renderingEvent.WaitOne(), "Waiting for rendering event");

            renderingEvent.Reset();
            Log.Comment("Putting repeater in and out of scroller 1 until we observe two out-of-sync loaded/unloaded events.");
            for (int i = 0; i < 2; ++i)
            {
                while (!unorderedLoadEvent)
                {
                    RunOnUIThread.Execute(() =>
                    {
                        // Validate subscription count for events + reset.
                        scroller1.Content = null;
                        scroller1.Content = repeater;
                    });
                    // For this issue to repro, we need to wait in such a way
                    // that we don't tick the UI thread. We can't use IdleSynchronizer here.
                    Task.Delay(16 * 3).Wait();
                }
                unorderedLoadEvent = false;
                Log.Comment("Detected an unordered load/unload event.");
            }

            IdleSynchronizer.Wait();
            Verify.IsTrue(renderingEvent.WaitOne(), "Waiting for rendering event");

            renderingEvent.Reset();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, scroller1SubscriberCount);
                Verify.AreEqual(0, scroller2SubscriberCount);

                Log.Comment("Moving repeater from scroller 1 to scroller 2.");
                scroller1.Content = null;
                scroller2.Content = repeater;
            });

            IdleSynchronizer.Wait();
            Verify.IsTrue(renderingEvent.WaitOne(), "Waiting for rendering event");

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, scroller1SubscriberCount);
                Verify.AreEqual(1, scroller2SubscriberCount);

                Log.Comment("Moving repeater out of scroller 2.");
                scroller2.Content = null;
                repeater = null;
            });

            Log.Comment("Waiting for repeater to get GCed.");
            for (int i = 0; i < 5 && repeaterWeakRef.IsAlive; ++i)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                IdleSynchronizer.Wait();
            }
            Verify.IsFalse(repeaterWeakRef.IsAlive);

            renderingEvent.Reset();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, scroller1SubscriberCount);
                Verify.AreEqual(0, scroller2SubscriberCount);

                Log.Comment("Scroller raise IRepeaterScrollSurface.PostArrange. Make sure no one is subscribing to it.");
                scroller1.InvalidateArrange();
                scroller2.InvalidateArrange();
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();
            Verify.IsTrue(renderingEvent.WaitOne(), "Waiting for rendering event");
        }

        // Test is flaky - disabling it while debugging the issue.
        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125 and internal issue 17581054
        public void CanBringIntoViewElements()
        {
            ScrollPresenter scrollPresenter = null;
            ItemsRepeater repeater = null;
            var rootLoadedEvent = new AutoResetEvent(initialState: false);
            var scrollCompletedEvent = new AutoResetEvent(initialState: false);
            var viewChangedOffsets = new List<double>();

            RunOnUIThread.Execute(() =>
            {
                var lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus.";
                var root = (Grid)XamlReader.Load(TestUtilities.ProcessTestXamlForRepo(
                     @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' xmlns:controls='using:Microsoft.UI.Xaml.Controls'> 
                         <Grid.Resources>
                           <controls:StackLayout x:Name='VerticalStackLayout' />
                           <controls:RecyclingElementFactory x:Key='ElementFactory'>
                             <controls:RecyclingElementFactory.RecyclePool>
                               <controls:RecyclePool />
                             </controls:RecyclingElementFactory.RecyclePool>
                             <DataTemplate x:Key='ItemTemplate'>
                               <Border Background='LightGray' Margin ='5'>
                                 <TextBlock Text='{Binding}' TextWrapping='WrapWholeWords' />
                               </Border>
                             </DataTemplate>
                             <DataTemplate x:Key='GroupTemplate'>
                               <StackPanel>
                                 <TextBlock Text='{Binding Name}' FontSize='24' Foreground='White' />
                                 <controls:ItemsRepeater
                                   x:Name='InnerRepeater'
                                   ItemsSource='{Binding}'
                                   ElementFactory='{StaticResource ElementFactory}'
                                   Layout='{StaticResource VerticalStackLayout}'
                                   HorizontalCacheLength='0'
                                   VerticalCacheLength='0' />
                               </StackPanel>
                             </DataTemplate>
                           </controls:RecyclingElementFactory>
                         </Grid.Resources>
                         <controls:ScrollPresenter x:Name='ScrollPresenter' Width='400' Height='600' IsChildAvailableWidthConstrained='True' Background='Gray'>
                           <controls:ItemsRepeater
                             x:Name='ItemsRepeater'
                             ElementFactory='{StaticResource ElementFactory}'
                             Layout='{StaticResource VerticalStackLayout}'
                             HorizontalCacheLength='0'
                             VerticalCacheLength='0' />
                         </controls:ScrollPresenter>
                       </Grid>"));

                var elementFactory = (RecyclingElementFactory)root.Resources["ElementFactory"];
                scrollPresenter = (ScrollPresenter)root.FindName("ScrollPresenter");
                repeater = (ItemsRepeater)root.FindName("ItemsRepeater");

                var groups = Enumerable.Range(0, 10).Select(i => new NamedGroup<string>(
                    "Group #" + i,
                    Enumerable.Range(0, 15).Select(j => string.Format("{0}.{1}: {2}", i, j, lorem.Substring(0, 250))))).ToList();

                repeater.ItemsSource = groups;

                elementFactory.SelectTemplateKey += (s, e) =>
                {
                    e.TemplateKey =
                        (e.DataContext is NamedGroup<string>) ?
                        "GroupTemplate" :
                        "ItemTemplate";
                };

                scrollPresenter.ViewChanged += (o, e) =>
                {
                    Log.Comment("ViewChanged: " + scrollPresenter.VerticalOffset);
                    viewChangedOffsets.Add(scrollPresenter.VerticalOffset);
                };

                ((IRepeaterScrollingSurface)scrollPresenter).ViewportChanged += (o, isFinal) =>
                {
                    if (isFinal)
                    {
                        scrollCompletedEvent.Set();
                    }
                };

                Content = root;

                root.Loaded += delegate {
                    rootLoadedEvent.Set();
                };
            });
            Verify.IsTrue(rootLoadedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Scroll into view item the last item.");
                var group = (FrameworkElement)repeater.GetOrCreateElement(9);
                var innerRepeater = (ItemsRepeater)group.FindName("InnerRepeater");
                innerRepeater.GetOrCreateElement(14).StartBringIntoView();
            });
            Verify.IsTrue(scrollCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();
            Verify.AreEqual(1, viewChangedOffsets.Count);
            viewChangedOffsets.Clear();
            ValidateRealizedRange(repeater, 8, 9, 9, 8, 9, 14);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Scroll into view item 9.10 (already realized) w/ animation.");
                var group = (FrameworkElement)repeater.TryGetElement(9);
                var innerRepeater = (ItemsRepeater)group.FindName("InnerRepeater");
                innerRepeater.TryGetElement(10).StartBringIntoView(new BringIntoViewOptions {
                    VerticalAlignmentRatio = 0.5,
                    AnimationDesired = true
                });
            });
            Verify.IsTrue(scrollCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();
            Verify.IsLessThan(1, viewChangedOffsets.Count);
            viewChangedOffsets.Clear();
            ValidateRealizedRange(repeater, 8, 9, 9, 6, 9, 14);

            Log.Comment("Validate no-op StartBringIntoView operations.");
            {
                var expectedVerticalOffsetAt9_10 = double.NaN;

                RunOnUIThread.Execute(() =>
                {
                    expectedVerticalOffsetAt9_10 = scrollPresenter.VerticalOffset;
                    var group = (FrameworkElement)repeater.GetOrCreateElement(9);
                    var innerRepeater = (ItemsRepeater)group.FindName("InnerRepeater");
                    innerRepeater.GetOrCreateElement(10).StartBringIntoView(new BringIntoViewOptions {
                        VerticalAlignmentRatio = 0.5,
                        AnimationDesired = true
                    });
                });
                Verify.IsTrue(scrollCompletedEvent.WaitOne(DefaultWaitTimeInMS));
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(expectedVerticalOffsetAt9_10, scrollPresenter.VerticalOffset);
                });
            }

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Scroll to item 3.4.");
                var group = (FrameworkElement)repeater.GetOrCreateElement(3);
                var innerRepeater = (ItemsRepeater)group.FindName("InnerRepeater");
                innerRepeater.GetOrCreateElement(4).StartBringIntoView(new BringIntoViewOptions {
                    VerticalAlignmentRatio = 0.0
                });
            });

            Verify.IsTrue(scrollCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();
            ValidateRealizedRange(repeater, 2, 4, 3, 3, 3, 10);

            // Code defect bug 17711793
            //RunOnUIThread.Execute(() =>
            //{
            //    Log.Comment("Scroll 0.0 to the top");
            //    var group = (FrameworkElement)repeater.MakeAnchor(0);
            //    var innerRepeater = (ItemsRepeater)group.FindName("InnerRepeater");
            //    innerRepeater.MakeAnchor(0).StartBringIntoView(new BringIntoViewOptions
            //    {
            //        VerticalAlignmentRatio = 0.0,
            //        AnimationDesired = true
            //    });
            //});

            //Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            //IdleSynchronizer.Wait();
            //Verify.AreEqual(1, viewChangedOffsets.Count);   // Animations are always disabled for anchors outside the realized range.
            //viewChangedOffsets.Clear();
            //ValidateRealizedRange(repeater, 0, 1, 0, 0, 0, 6);
        }

        private void ValidateRealizedRange(
            ItemsRepeater repeater,
            int expectedFirstGroupIndex,
            int expectedLastGroupIndex,
            int expectedFirstItemGroupIndex,
            int expectedFirstItemIndex,
            int expectedLastItemGroupIndex,
            int expectedLastItemIndex)
        {
            int actualFirstGroupIndex = -1;
            int actualLastGroupIndex = -1;
            int actualFirstItemGroupIndex = -1;
            int actualFirstItemIndex = -1;
            int actualLastItemGroupIndex = -1;
            int actualLastItemIndex = -1;

            RunOnUIThread.Execute(() =>
            {
                int groupIndex = 0;
                int itemIndex = 0;

                var groups = (IEnumerable<NamedGroup<string>>)repeater.ItemsSource;

                foreach (var group in groups)
                {
                    var groupElement = repeater.TryGetElement(groupIndex);

                    if (groupElement != null)
                    {
                        actualFirstGroupIndex =
                            actualFirstGroupIndex == -1 ?
                            groupIndex :
                            actualFirstGroupIndex;

                        actualLastGroupIndex = groupIndex;

                        var innerRepeater = (ItemsRepeater)((FrameworkElement)groupElement).FindName("InnerRepeater");

                        foreach (var item in group)
                        {
                            var itemElement = innerRepeater.TryGetElement(itemIndex);

                            if (itemElement != null)
                            {
                                actualFirstItemGroupIndex =
                                    actualFirstItemGroupIndex == -1 ?
                                    groupIndex :
                                    actualFirstItemGroupIndex;

                                actualFirstItemIndex =
                                    actualFirstItemIndex == -1 ?
                                    itemIndex :
                                    actualFirstItemIndex;

                                actualLastItemGroupIndex = groupIndex;
                                actualLastItemIndex = itemIndex;
                            }

                            ++itemIndex;
                        }
                    }

                    itemIndex = 0;
                    ++groupIndex;
                }
            });

            Verify.AreEqual(expectedFirstGroupIndex, actualFirstGroupIndex);
            Verify.AreEqual(expectedLastGroupIndex, actualLastGroupIndex);
            Verify.AreEqual(expectedFirstItemGroupIndex, actualFirstItemGroupIndex);
            Verify.AreEqual(expectedFirstItemIndex, actualFirstItemIndex);
            Verify.AreEqual(expectedLastItemGroupIndex, actualLastItemGroupIndex);
            Verify.AreEqual(expectedLastItemIndex, actualLastItemIndex);
        }

        private void RunElementTrackingTestRoutine(Action<
            ObservableCollection<ObservableCollection<string>> /* data */,
            TestScrollingSurface[] /* scrollers */,
            ItemsRepeater /* rootRepeater */> testRoutine)
        {
            // Base setup for our element tracking tests.
            // We have 4 fake scrollers in series, initially in a non scrollable configuration.
            // Under them we have a group repeater tree (2 groups with 3 items each).
            // The group UI is a StackPanel with a TextBlock "header" and an inner ItemsRepeater.
            RunOnUIThread.Execute(() =>
            {
                int groupCount = 2;
                int itemsPerGroup = 3;

                var data = new ObservableCollection<ObservableCollection<string>>(
                    Enumerable.Range(0, groupCount).Select(i => new ObservableCollection<string>(
                        Enumerable.Range(0, itemsPerGroup).Select(j => string.Format("Item #{0}.{1}", i, j)))));

                var itemElements = Enumerable.Range(0, groupCount).Select(i =>
                    Enumerable.Range(0, itemsPerGroup).Select(j => new Border { Name = data[i][j], Width = 50, Height = 50, Background = new SolidColorBrush(Microsoft.UI.Colors.Red) }).ToList()).ToArray();

                var headerElements = Enumerable.Range(0, groupCount).Select(i => new TextBlock { Text = "Header #" + i }).ToList();
                var groupRepeaters = Enumerable.Range(0, groupCount).Select(i => new ItemsRepeater {
                    Name = "ItemsRepeater #" + i,
                    ItemsSource = data[i],
                    ItemTemplate = MockElementFactory.CreateElementFactory(itemElements[i]),
                    Layout = new TestGridLayout { Orientation = Orientation.Horizontal, MinItemWidth = 50, MinItemHeight = 50, MinRowSpacing = 10, MinColumnSpacing = 10 }
                }).ToList();
                var groupElement = Enumerable.Range(0, groupCount).Select(i =>
                {
                    var panel = new StackPanel();
                    panel.Name = "Group #" + i;
                    panel.Children.Add(headerElements[i]);
                    panel.Children.Add(groupRepeaters[i]);
                    return panel;
                }).ToList();

                var rootRepeater = new ItemsRepeater {
                    Name = "Root ItemsRepeater",
                    ItemsSource = data,
                    ItemTemplate = MockElementFactory.CreateElementFactory(groupElement),
                    Layout = new TestStackLayout { Orientation = Orientation.Vertical }
                };

                var scrollers = new TestScrollingSurface[4];
                for (int i = 0; i < scrollers.Length; ++i)
                {
                    scrollers[i] = new TestScrollingSurface() {
                        Name = "S" + i,
                        Content = i > 0 ? (UIElement)scrollers[i - 1] : rootRepeater
                    };
                }

                var resetScrollers = (Action)(() =>
                {
                    foreach (var scroller in scrollers)
                    {
                        scroller.IsHorizontallyScrollable = false;
                        scroller.IsVerticallyScrollable = false;
                        scroller.RegisterAnchorCandidateFunc = null;
                        scroller.UnregisterAnchorCandidateFunc = null;
                        scroller.GetRelativeViewportFunc = null;
                    }
                });

                var outerScroller = scrollers.Last();
                outerScroller.Width = 200.0;
                outerScroller.Height = 2000.0;

                Content = outerScroller;
                Content.UpdateLayout();

                testRoutine(data, scrollers, rootRepeater);
            });
        }

        private static VirtualizingLayout GetMonitoringLayout(Size desiredSize, List<Rect> realizationRects, List<Rect> visibleRects)
        {
            return new MockVirtualizingLayout {
                MeasureLayoutFunc = (availableSize, context) =>
                {
                    realizationRects.Add(context.RealizationRect);
                    if (visibleRects != null)
                    {
                        visibleRects.Add(context.VisibleRect);
                    }
                    return desiredSize;
                },

                ArrangeLayoutFunc = (finalSize, context) => finalSize
            };
        }

        private class TestScrollingSurface : ContentControl, IRepeaterScrollingSurface
        {
            private bool _isHorizontallyScrollable;
            private bool _isVerticallyScrollable;

            public bool InMeasure { get; set; }
            public bool InArrange { get; set; }
            public bool InPostArrange { get; private set; }

            public Action ConfigurationChangedAddFunc { get; set; }
            public Action ConfigurationChangedRemoveFunc { get; set; }

            public Action<UIElement> RegisterAnchorCandidateFunc { get; set; }
            public Action<UIElement> UnregisterAnchorCandidateFunc { get; set; }
            public Func<UIElement, Rect> GetRelativeViewportFunc { get; set; }

            public UIElement AnchorElement { get; set; }

            public bool IsHorizontallyScrollable
            {
                get { return _isHorizontallyScrollable; }
                set
                {
                    _isHorizontallyScrollable = value;
                    RaiseConfigurationChanged();
                    InvalidateMeasure();
                }
            }

            public bool IsVerticallyScrollable
            {
                get { return _isVerticallyScrollable; }
                set
                {
                    _isVerticallyScrollable = value;
                    RaiseConfigurationChanged();
                    InvalidateMeasure();
                }
            }
            
            private event ConfigurationChangedEventHandler _configurationChanged;

            public event ConfigurationChangedEventHandler ConfigurationChanged
            {
                add
                {
                    if (ConfigurationChangedAddFunc != null)
                    {
                        ConfigurationChangedAddFunc();
                    }

                    _configurationChanged += value;
                }
                remove
                {
                    if (ConfigurationChangedRemoveFunc != null)
                    {
                        ConfigurationChangedRemoveFunc();
                    }

                    _configurationChanged -= value;
                }
            }
            public event PostArrangeEventHandler PostArrange;
#pragma warning disable CS0067
            // Warning CS0067: The event 'ViewportTests.TestScrollingSurface.ViewportChanged' is never used.
            public event ViewportChangedEventHandler ViewportChanged;
#pragma warning restore CS0067

            public void RegisterAnchorCandidate(UIElement element)
            {
                RegisterAnchorCandidateFunc(element);
            }

            public void UnregisterAnchorCandidate(UIElement element)
            {
                UnregisterAnchorCandidateFunc(element);
            }

            public Rect GetRelativeViewport(UIElement child)
            {
                return GetRelativeViewportFunc(child);
            }

            protected override Size MeasureOverride(Size availableSize)
            {
                InMeasure = true;
                var result = base.MeasureOverride(availableSize);
                InMeasure = false;
                return result;
            }

            protected override Size ArrangeOverride(Size finalSize)
            {
                InArrange = true;

                var result = base.ArrangeOverride(finalSize);

                InArrange = false;
                InPostArrange = true;

                if (PostArrange != null)
                {
                    PostArrange(this);
                }

                InPostArrange = false;

                return result;
            }

            private void RaiseConfigurationChanged()
            {
                if (_configurationChanged != null)
                {
                    _configurationChanged(this);
                }
            }
        }

        private class TestStackLayout : StackLayout
        {
            public UIElement SuggestedAnchor { get; private set; }

            protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
            {
                var anchorIndex = context.RecommendedAnchorIndex;
                SuggestedAnchor = anchorIndex < 0 ? null : context.GetOrCreateElementAt(anchorIndex);
                return base.MeasureOverride(context, availableSize);
            }
        }

        private class TestGridLayout : UniformGridLayout
        {
            public UIElement SuggestedAnchor { get; private set; }

            protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
            {
                var anchorIndex = context.RecommendedAnchorIndex;
                SuggestedAnchor = anchorIndex < 0 ? null : context.GetOrCreateElementAt(anchorIndex);
                return base.MeasureOverride(context, availableSize);
            }
        }
    }
}
