// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Linq;
using Windows.Foundation;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
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

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class EffectiveViewportTests : ApiTestBase
    {
        [TestMethod]
        public void ValidateBasicScrollViewerScenario()
        {
            var realizationRects = new List<Rect>();
            var viewChangeCompletedEvent = new AutoResetEvent(false);
            ScrollViewer scrollViewer = null;
            ManualResetEvent viewChanged = new ManualResetEvent(false);
            ManualResetEvent layoutMeasured = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater() {
                    Layout = GetMonitoringLayout(new Size(500, 600), realizationRects, layoutMeasured),
                    HorizontalCacheLength = 0.0,
                    VerticalCacheLength = 0.0
                };

                scrollViewer = new ScrollViewer {
                    Content = repeater,
                    Width = 200,
                    Height = 300,
                    HorizontalScrollBarVisibility = ScrollBarVisibility.Hidden,
                    VerticalScrollBarVisibility = ScrollBarVisibility.Hidden,
                };

                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        Log.Comment("ViewChanged " + scrollViewer.HorizontalOffset + ":" + scrollViewer.VerticalOffset);
                        viewChanged.Set();
                    }
                };

                Content = scrollViewer;
            });

            Verify.IsTrue(layoutMeasured.WaitOne(), "Did not receive measure on layout");

            RunOnUIThread.Execute(() =>
            {
                // First layout pass will invalidate measure during the first arrange
                // so that we can get a viewport during the second measure/arrange pass.
                Verify.AreEqual(new Rect(0, 0, 0, 0), realizationRects[0]);
                Verify.AreEqual(new Rect(0, 0, 200, 300), realizationRects[1]);
                realizationRects.Clear();

                viewChanged.Reset();
                layoutMeasured.Reset();
                scrollViewer.ChangeView(null, 100.0, 1.0f, disableAnimation: true);
            });

            IdleSynchronizer.Wait();
            Verify.IsTrue(viewChanged.WaitOne(), "Did not receive view changed event");
            Verify.IsTrue(layoutMeasured.WaitOne(), "Did not receive measure on layout");
            viewChanged.Reset();
            layoutMeasured.Reset();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(0, 100, 200, 300), realizationRects.Last());
                realizationRects.Clear();
                viewChangeCompletedEvent.Reset();

                // Max viewport offset is (300, 400). Horizontal viewport offset
                // is expected to get coerced from 400 to 300.
                scrollViewer.ChangeView(400, 100.0, 1.0f, disableAnimation: true);
            });

            IdleSynchronizer.Wait();
            Verify.IsTrue(viewChanged.WaitOne(), "Did not receive view changed event");
            Verify.IsTrue(layoutMeasured.WaitOne(), "Did not receive measure on layout");
            viewChanged.Reset();
            layoutMeasured.Reset();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(300, 100, 200, 300), realizationRects.Last());
                realizationRects.Clear();
                viewChangeCompletedEvent.Reset();

                scrollViewer.ChangeView(null, null, 2.0f, disableAnimation: true);
            });

            IdleSynchronizer.Wait();
            Verify.IsTrue(viewChanged.WaitOne(), "Did not receive view changed event");
            Verify.IsTrue(layoutMeasured.WaitOne(), "Did not receive measure on layout");
            viewChanged.Reset();
            layoutMeasured.Reset();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(150, 50, 100, 150), realizationRects.Last());
                realizationRects.Clear();
            });
        }

        [TestMethod]
        public void ValidateOneScrollViewerScenario()
        {
            var realizationRects = new List<Rect>();
            ScrollViewer scrollViewer = null;
            var viewChangeCompletedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater()
                {
                    Layout = GetMonitoringLayout(new Size(500, 600), realizationRects),
                    HorizontalCacheLength = 0.0,
                    VerticalCacheLength = 0.0
                };

                scrollViewer = new ScrollViewer
                {
                    Content = repeater,
                    Width = 200,
                    Height = 300
                };

                Content = scrollViewer;
                Content.UpdateLayout();

                Verify.AreEqual(2, realizationRects.Count);
                Verify.AreEqual(new Rect(0, 0, 0, 0), realizationRects[0]);
                Verify.AreEqual(new Rect(0, 0, 200, 300), realizationRects[1]);
                realizationRects.Clear();

                scrollViewer.ViewChanged += (Object sender, ScrollViewerViewChangedEventArgs args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChangeCompletedEvent.Set();
                    }
                };
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                scrollViewer.ChangeView(0.0, 100.0, null, true);
            });
            Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(0, 100, 200, 300), realizationRects.Last());
                realizationRects.Clear();

                viewChangeCompletedEvent.Reset();
                scrollViewer.ChangeView(null, null, 2.0f, true);
            });
            Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(
                    new Rect(0, 50, 100, 150),
                    realizationRects.Last());
                realizationRects.Clear();
            });
        }

        [TestMethod]
        public void ValidateTwoScrollViewerScenario()
        {
            var realizationRects = new List<Rect>();
            ScrollViewer horizontalScroller = null;
            ScrollViewer verticalScroller = null;
            var horizontalViewChangeCompletedEvent = new AutoResetEvent(false);
            var verticalViewChangeCompletedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater()
                {
                    Layout = GetMonitoringLayout(new Size(500, 500), realizationRects),
                    HorizontalCacheLength = 0.0,
                    VerticalCacheLength = 0.0
                };

                horizontalScroller = new ScrollViewer
                {
                    Content = repeater,
                    HorizontalScrollBarVisibility = ScrollBarVisibility.Auto,
                    VerticalScrollBarVisibility = ScrollBarVisibility.Disabled,
                    HorizontalScrollMode = ScrollMode.Enabled,
                    VerticalScrollMode = ScrollMode.Disabled
                };

                verticalScroller = new ScrollViewer
                {
                    Content = horizontalScroller,
                    Width = 200,
                    Height = 200
                };

                Content = verticalScroller;
                Content.UpdateLayout();

                Verify.AreEqual(2, realizationRects.Count);
                Verify.AreEqual(new Rect(0, 0, 0, 0), realizationRects[0]);
                Verify.AreEqual(new Rect(0, 0, 200, 200), realizationRects[1]);
                realizationRects.Clear();

                horizontalScroller.ViewChanged += (Object sender, ScrollViewerViewChangedEventArgs args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        horizontalViewChangeCompletedEvent.Set();
                    }
                };

                verticalScroller.ViewChanged += (Object sender, ScrollViewerViewChangedEventArgs args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        verticalViewChangeCompletedEvent.Set();
                    }
                };
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                verticalScroller.ChangeView(0.0, 100.0, null, true);
            });
            Verify.IsTrue(verticalViewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(0, 100, 200, 200), realizationRects.Last());
                realizationRects.Clear();

                // Max viewport offset is (300, 300). Horizontal viewport offset
                // is expected to get coerced from 400 to 300.
                horizontalScroller.ChangeView(400.0, 100.0, null, true);
            });
            Verify.IsTrue(horizontalViewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(300, 100, 200, 200), realizationRects.Last());
                realizationRects.Clear();
            });
        }

        [TestMethod]
        public void CanGrowCacheBufferWithScrollViewer()
        {
            ScrollViewer scroller = null;
            ItemsRepeater repeater = null;
            var measureRealizationRects = new List<Rect>();
            var arrangeRealizationRects = new List<Rect>();
            var fullCacheEvent = new ManualResetEvent(initialState: false);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Preparing the visual tree...");

                scroller = new ScrollViewer
                {
                    Width = 400,
                    Height = 400
                };

                var layout = new MockVirtualizingLayout
                {
                    MeasureLayoutFunc = (availableSize, context) =>
                    {
                        var ctx = (VirtualizingLayoutContext)context;
                        Log.Comment("MeasureLayout - Rect:" +  ctx.RealizationRect);
                        if(measureRealizationRects.Count == 0 || measureRealizationRects.Last() != ctx.RealizationRect)
                        {
                            measureRealizationRects.Add(ctx.RealizationRect);
                        }

                        return new Size(1000, 2000);
                    },

                    ArrangeLayoutFunc = (finalSize, context) =>
                    {
                        var ctx = (VirtualizingLayoutContext)context;
                        Log.Comment("ArrangeLayout - Rect:" +  ctx.RealizationRect);
                        if(arrangeRealizationRects.Count == 0 || arrangeRealizationRects.Last() != ctx.RealizationRect)
                        {
                            arrangeRealizationRects.Add(ctx.RealizationRect);
                        }

                        if (ctx.RealizationRect.Height == scroller.Height * (repeater.VerticalCacheLength + 1))
                        {
                            fullCacheEvent.Set();
                        }

                        return finalSize;
                    }
                };

                repeater = new ItemsRepeater()
                {
                    Layout = layout
                };

                scroller.Content = repeater;
                Content = scroller;
            });

            if (!fullCacheEvent.WaitOne(DefaultWaitTimeInMS)) Verify.Fail("Cache full size never reached.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var cacheLength = repeater.VerticalCacheLength;
                var expectedRealizationWindow = new Rect(
                    -cacheLength / 2 * scroller.Width,
                    -cacheLength / 2 * scroller.Height,
                    (1 + cacheLength) * scroller.Width,
                    (1 + cacheLength) * scroller.Height);

                Log.Comment("Validate that the realization window reached full size.");
                Verify.AreEqual(expectedRealizationWindow, measureRealizationRects.Last());
                Verify.AreEqual(expectedRealizationWindow, arrangeRealizationRects.Last());

                Log.Comment("Validate that the realization window grew by 40 pixels each time during the process.");
                for (int i = 2; i < measureRealizationRects.Count; ++i)
                {
                    Verify.AreEqual(-40, measureRealizationRects[i].X - measureRealizationRects[i - 1].X);
                    Verify.AreEqual(-40, measureRealizationRects[i].Y - measureRealizationRects[i - 1].Y);
                    Verify.AreEqual(80, measureRealizationRects[i].Width - measureRealizationRects[i - 1].Width);
                    Verify.AreEqual(80, measureRealizationRects[i].Height - measureRealizationRects[i - 1].Height);

                    Verify.AreEqual(-40, arrangeRealizationRects[i].X - arrangeRealizationRects[i - 1].X);
                    Verify.AreEqual(-40, arrangeRealizationRects[i].Y - arrangeRealizationRects[i - 1].Y);
                    Verify.AreEqual(80, arrangeRealizationRects[i].Width - arrangeRealizationRects[i - 1].Width);
                    Verify.AreEqual(80, arrangeRealizationRects[i].Height - arrangeRealizationRects[i - 1].Height);
                }
            });
        }

        [TestMethod]
        public void CanBringIntoViewElements()
        {
            ScrollViewer scroller = null;
            ItemsRepeater itemsRepeater = null;
            var rootLoadedEvent = new AutoResetEvent(initialState: false);
            var viewChangeCompletedEvent = new AutoResetEvent(initialState: false);
            var viewChangedOffsets = new List<double>();

            RunOnUIThread.Execute(() =>
            {
                var lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus.";
                var root = (Grid)XamlReader.Load(TestUtilities.ProcessTestXamlForRepo(
                     @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' xmlns:controls='using:Microsoft.UI.Xaml.Controls'> 
                         <Grid.Resources>
                           <controls:StackLayout x:Name='VerticalStackLayout'/>
                           <controls:RecyclingElementFactory x:Key='ElementFactory'>
                             <controls:RecyclingElementFactory.RecyclePool>
                               <controls:RecyclePool/>
                             </controls:RecyclingElementFactory.RecyclePool>
                             <DataTemplate x:Key='ItemTemplate'>
                               <Border Background='LightGray' Margin='5' Height='94'>
                                 <TextBlock Text='{Binding}' TextWrapping='WrapWholeWords'/>
                               </Border>
                             </DataTemplate>
                           </controls:RecyclingElementFactory>
                         </Grid.Resources>
                         <ScrollViewer x:Name='Scroller' Width='400' Height='600' Background='Gray'>
                           <controls:ItemsRepeater
                             x:Name='ItemsRepeater'
                             ItemTemplate='{StaticResource ElementFactory}'
                             Layout='{StaticResource VerticalStackLayout}'
                             HorizontalCacheLength='0'
                             VerticalCacheLength='0'/>
                         </ScrollViewer>
                       </Grid>"));

                var elementFactory = (RecyclingElementFactory)root.Resources["ElementFactory"];
                scroller = (ScrollViewer)root.FindName("Scroller");
                itemsRepeater = (ItemsRepeater)root.FindName("ItemsRepeater");

                var items = Enumerable.Range(0, 400).Select(i => string.Format("{0}: {1}", i, lorem.Substring(0, 250)));

                itemsRepeater.ItemsSource = items;

                scroller.ViewChanged += (o, e) =>
                {
                    Log.Comment($"ScrollViewer.ViewChanged - VerticalOffset: {scroller.VerticalOffset}");
                    viewChangedOffsets.Add(scroller.VerticalOffset);
                    if (!e.IsIntermediate)
                    {
                        viewChangeCompletedEvent.Set();
                    }
                };

                Content = root;

                root.Loaded += delegate
                {
                    rootLoadedEvent.Set();
                };
            });
            Verify.IsTrue(rootLoadedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("\r\nStartBringIntoView for item index 100, w/ 0.5 vertical alignment.");
                viewChangeCompletedEvent.Reset();
                itemsRepeater.GetOrCreateElement(100).StartBringIntoView(new BringIntoViewOptions {
                    VerticalAlignmentRatio = 0.5
                });
                itemsRepeater.UpdateLayout();
            });

            Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();

            Log.Comment($"ScrollViewer.ViewChanged - Count: {viewChangedOffsets.Count}");
            Verify.AreEqual(1, viewChangedOffsets.Count);
            viewChangedOffsets.Clear();

            ValidateRealizedRange(itemsRepeater, 96, 103);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("\r\nStartBringIntoView for item index 103 (already realized) w/ animation.");
                viewChangeCompletedEvent.Reset();
                itemsRepeater.TryGetElement(103).StartBringIntoView(new BringIntoViewOptions
                {
                    VerticalAlignmentRatio = 0.5,
                    AnimationDesired = true
                });
                itemsRepeater.UpdateLayout();
            });

            Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();

            Log.Comment($"ScrollViewer.ViewChanged - Count: {viewChangedOffsets.Count}");
            Verify.IsLessThanOrEqual(1, viewChangedOffsets.Count);
            viewChangedOffsets.Clear();

            ValidateRealizedRange(itemsRepeater, 99, 107);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("\r\nStartBringIntoView for disconnected item index 0, to the top w/ unfulfilled animation and 0.5 vertical alignment.");
                viewChangeCompletedEvent.Reset();
                itemsRepeater.GetOrCreateElement(0).StartBringIntoView(new BringIntoViewOptions
                {
                    VerticalAlignmentRatio = 0.5,
                    AnimationDesired = true
                });
                itemsRepeater.UpdateLayout();
            });

            Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();

            Log.Comment($"ScrollViewer.ViewChanged - Count: {viewChangedOffsets.Count}");
            Verify.AreEqual(1, viewChangedOffsets.Count);
            viewChangedOffsets.Clear();

            ValidateRealizedRange(itemsRepeater, 0, 6);

            RunOnUIThread.Execute(() =>
            {
                // You can't align the first item in the middle obviously.
                Verify.AreEqual(0, scroller.VerticalOffset);

                Log.Comment("\r\nStartBringIntoView for item index 20.");
                viewChangeCompletedEvent.Reset();
                itemsRepeater.GetOrCreateElement(20).StartBringIntoView(new BringIntoViewOptions
                {
                    VerticalAlignmentRatio = 0.0
                });
                itemsRepeater.UpdateLayout();
            });

            Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();

            Log.Comment($"ScrollViewer.ViewChanged - Count: {viewChangedOffsets.Count}");
            Verify.AreEqual(1, viewChangedOffsets.Count);

            ValidateRealizedRange(itemsRepeater, 19, 26); 
        }

        private void ValidateRealizedRange(
            ItemsRepeater itemsRepeater,
            int expectedFirstItemIndex,
            int expectedLastItemIndex)
        {
            Log.Comment("Validating Realized Range...");
            int actualFirstItemIndex = -1;
            int actualLastItemIndex = -1;
            int itemIndex = 0;

            RunOnUIThread.Execute(() =>
            {
                var items = itemsRepeater.ItemsSource as IEnumerable<string>;

                foreach (var item in items)
                {
                    var itemElement = itemsRepeater.TryGetElement(itemIndex);

                    if (itemElement != null)
                    {
                        actualFirstItemIndex =
                            actualFirstItemIndex == -1 ?
                            itemIndex :
                            actualFirstItemIndex;
                        actualLastItemIndex = itemIndex;

                        Log.Comment($"Realized Index - {itemIndex}");
                    }

                    ++itemIndex;
                }
            });

            Log.Comment($"FirstItemIndex - {expectedFirstItemIndex}  {actualFirstItemIndex}");
            Log.Comment($"LastItemIndex  - {expectedLastItemIndex}  {actualLastItemIndex}");
            Verify.AreEqual(expectedFirstItemIndex, actualFirstItemIndex);
            Verify.AreEqual(expectedLastItemIndex, actualLastItemIndex);
        }

        private static VirtualizingLayout GetMonitoringLayout(Size desiredSize, List<Rect> realizationRects, ManualResetEvent layoutMeasured = null)
        {
            return new MockVirtualizingLayout
            {
                MeasureLayoutFunc = (availableSize, context) =>
                {
                    var ctx = (VirtualizingLayoutContext)context;
                    Log.Comment("MeasureLayout:" + ctx.RealizationRect);
                    realizationRects.Add(ctx.RealizationRect);
                    if (layoutMeasured != null)
                    {
                        layoutMeasured.Set();
                    }
                    return desiredSize;
                },

                ArrangeLayoutFunc = (finalSize, context) => finalSize
            };
        }
    }
}