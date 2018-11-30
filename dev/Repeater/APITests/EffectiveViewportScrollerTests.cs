// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Linq;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using System.Threading;
using System.Collections.Generic;
using Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.Mocks;
using System.Numerics;
using Common;
using System.Collections.ObjectModel;
using Windows.UI.Xaml.Media;
using Windows.UI;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if !BUILD_WINDOWS
using VirtualizingLayout = Microsoft.UI.Xaml.Controls.VirtualizingLayout;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using VirtualizingLayoutContext = Microsoft.UI.Xaml.Controls.VirtualizingLayoutContext;
using RecyclingElementFactory = Microsoft.UI.Xaml.Controls.RecyclingElementFactory;
using StackLayout = Microsoft.UI.Xaml.Controls.StackLayout;
using UniformGridLayout = Microsoft.UI.Xaml.Controls.UniformGridLayout;
using ScrollAnchorProvider = Microsoft.UI.Xaml.Controls.ScrollAnchorProvider;
using Scroller = Microsoft.UI.Xaml.Controls.Scroller;
using ScrollerViewChangeCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollerViewChangeCompletedEventArgs;
using ScrollerChangeOffsetsOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeOffsetsOptions;
using ScrollerViewChangeKind = Microsoft.UI.Xaml.Controls.ScrollerViewChangeKind;
using ScrollerViewKind = Microsoft.UI.Xaml.Controls.ScrollerViewKind;
using ScrollerChangeZoomFactorOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeZoomFactorOptions;
using IRepeaterScrollingSurface = Microsoft.UI.Private.Controls.IRepeaterScrollingSurface;
using ConfigurationChangedEventHandler = Microsoft.UI.Private.Controls.ConfigurationChangedEventHandler;
using PostArrangeEventHandler = Microsoft.UI.Private.Controls.PostArrangeEventHandler;
using ViewportChangedEventHandler = Microsoft.UI.Private.Controls.ViewportChangedEventHandler;
using ScrollerViewChangeSnapPointRespect = Microsoft.UI.Xaml.Controls.ScrollerViewChangeSnapPointRespect;
#endif

#if BUILD_WINDOWS

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class EffectiveViewportScrollerTests : TestsBase
    {
        [TestMethod]
        public void ValidateOneScrollerScenario()
        {
            var realizationRects = new List<Rect>();
            Scroller scroller = null;
            var viewChangeCompletedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater()
                {
                    Layout = GetMonitoringLayout(new Size(500, 600), realizationRects),
                    HorizontalCacheLength = 0.0,
                    VerticalCacheLength = 0.0
                };

                scroller = new Scroller
                {
                    Child = repeater,
                    Width = 200,
                    Height = 300
                };

                Content = scroller;
                Content.UpdateLayout();

                Verify.AreEqual(2, realizationRects.Count);
                Verify.AreEqual(new Rect(0, 0, 0, 0), realizationRects[0]);
                Verify.AreEqual(new Rect(0, 0, 200, 300), realizationRects[1]);
                realizationRects.Clear();

                scroller.ViewChangeCompleted += (Scroller sender, ScrollerViewChangeCompletedEventArgs args) =>
                {
                    viewChangeCompletedEvent.Set();
                };
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                scroller.ChangeOffsets(new ScrollerChangeOffsetsOptions(0.0, 100.0, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
            });
            Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(0, 100, 200, 300), realizationRects.Last());
                realizationRects.Clear();

                viewChangeCompletedEvent.Reset();
                scroller.ChangeZoomFactor(new ScrollerChangeZoomFactorOptions(2.0f, ScrollerViewKind.Absolute, Vector2.Zero, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
            });
            Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(
                    new Rect(0, 100, 100, 150),
                    realizationRects.Last());
                realizationRects.Clear();
            });
        }

        [TestMethod]
        public void ValidateTwoScrollersScenario()
        {
            var realizationRects = new List<Rect>();
            Scroller horizontalScroller = null;
            Scroller verticalScroller = null;
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

                horizontalScroller = new Scroller
                {
                    Child = repeater,
                    IsChildAvailableHeightConstrained = true
                };

                // Placing a Grid in between two Scroller controls to avoid
                // unsupported combined use of facades and ElementCompositionPreview.
                var grid = new Grid();
                grid.Children.Add(horizontalScroller);

                verticalScroller = new Scroller
                {
                    Child = grid,
                    Width = 200,
                    Height = 200,
                    IsChildAvailableWidthConstrained = true
                };

                Content = verticalScroller;
                Content.UpdateLayout();

                Verify.AreEqual(2, realizationRects.Count);
                Verify.AreEqual(new Rect(0, 0, 0, 0), realizationRects[0]);
                Verify.AreEqual(new Rect(0, 0, 200, 200), realizationRects[1]);
                realizationRects.Clear();

                horizontalScroller.ViewChangeCompleted += (Scroller sender, ScrollerViewChangeCompletedEventArgs args) =>
                {
                    horizontalViewChangeCompletedEvent.Set();
                };

                verticalScroller.ViewChangeCompleted += (Scroller sender, ScrollerViewChangeCompletedEventArgs args) =>
                {
                    verticalViewChangeCompletedEvent.Set();
                };
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                verticalScroller.ChangeOffsets(new ScrollerChangeOffsetsOptions(0.0, 100.0, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
            });
            Verify.IsTrue(verticalViewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(0, 100, 200, 200), realizationRects.Last());
                realizationRects.Clear();

                // Max viewport offset is (300, 300). Horizontal viewport offset
                // is expected to get coerced from 400 to 300.
                horizontalScroller.ChangeOffsets(new ScrollerChangeOffsetsOptions(400.0, 100.0, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
            });
            Verify.IsTrue(horizontalViewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new Rect(300, 100, 200, 200), realizationRects.Last());
                realizationRects.Clear();
            });
        }

        [TestMethod]
        public void CanGrowCacheBuffer()
        {
            Scroller scroller = null;
            ItemsRepeater repeater = null;
            var measureRealizationRects = new List<Rect>();
            var arrangeRealizationRects = new List<Rect>();
            var fullCacheEvent = new ManualResetEvent(initialState: false);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Preparing the visual tree...");

                scroller = new Scroller
                {
                    Width = 400,
                    Height = 400
                };

                var layout = new MockVirtualizingLayout
                {
                    MeasureLayoutFunc = (availableSize, context) =>
                    {
                        var ctx = (VirtualizingLayoutContext)context;
                        measureRealizationRects.Add(ctx.RealizationRect);
                        return new Size(1000, 2000);
                    },

                    ArrangeLayoutFunc = (finalSize, context) =>
                    {
                        var ctx = (VirtualizingLayoutContext)context;
                        arrangeRealizationRects.Add(ctx.RealizationRect);

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

                scroller.Child = repeater;
                Content = scroller;
            });

            if (!fullCacheEvent.WaitOne(500000)) Verify.Fail("Cache full size never reached.");
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
#if BUILD_WINDOWS
        [TestProperty("Ignore", "True")] // TODO 19581880: Re-enable after investigating and fixing the test failures.
#endif
        public void CanBringIntoViewElements()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                Log.Warning("Skipping CanBringIntoViewElements because UIElement.BringIntoViewRequested was added in RS4.");
                return;
            }

            Scroller scroller = null;
            ItemsRepeater repeater = null;
            var rootLoadedEvent = new AutoResetEvent(initialState: false);
            var effectiveViewChangeCompletedEvent = new AutoResetEvent(initialState: false);
            var viewChangeCompletedEvent = new AutoResetEvent(initialState: false);

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
                           </controls:RecyclingElementFactory>
                         </Grid.Resources>
                         <controls:Scroller x:Name='Scroller' Width='400' Height='600' IsChildAvailableWidthConstrained='True' Background='Gray'>
                           <controls:ItemsRepeater
                             x:Name='ItemsRepeater'
                             ElementFactory='{StaticResource ElementFactory}'
                             Layout='{StaticResource VerticalStackLayout}'
                             HorizontalCacheLength='0'
                             VerticalCacheLength='0' />
                         </controls:Scroller>
                       </Grid>"));

                var elementFactory = (RecyclingElementFactory)root.Resources["ElementFactory"];
                scroller = (Scroller)root.FindName("Scroller");
                repeater = (ItemsRepeater)root.FindName("ItemsRepeater");

                var items = Enumerable.Range(0, 400).Select(i => string.Format("{0}: {1}", i, lorem.Substring(0, 250)));

                repeater.ItemsSource = items;

                scroller.ViewChanged += (o, e) =>
                {
                    Log.Comment("Scroller.ViewChanged: VerticalOffset=" + scroller.VerticalOffset);
                    viewChangedOffsets.Add(scroller.VerticalOffset);
                    viewChangeCompletedEvent.Set();
                };

                scroller.BringingIntoView += (o, e) =>
                {
                    Log.Comment("Scroller.BringingIntoView:");
                    Log.Comment("TargetVerticalOffset=" + e.TargetVerticalOffset);
                    Log.Comment("RequestEventArgs.AnimationDesired=" + e.RequestEventArgs.AnimationDesired);
                    Log.Comment("RequestEventArgs.Handled=" + e.RequestEventArgs.Handled);
                    Log.Comment("RequestEventArgs.VerticalAlignmentRatio=" + e.RequestEventArgs.VerticalAlignmentRatio);
                    Log.Comment("RequestEventArgs.VerticalOffset=" + e.RequestEventArgs.VerticalOffset);
                    Log.Comment("RequestEventArgs.TargetRect=" + e.RequestEventArgs.TargetRect);
                };

                scroller.EffectiveViewportChanged += (o, args) =>
                {
                    Log.Comment("Scroller.EffectiveViewportChanged: VerticalOffset=" + scroller.VerticalOffset);
                    effectiveViewChangeCompletedEvent.Set();
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
                repeater.GetOrCreateElement(100).StartBringIntoView();
                repeater.UpdateLayout();
            });

            Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();
            Verify.AreEqual(1, viewChangedOffsets.Count);
            viewChangedOffsets.Clear();

            ValidateRealizedRange(repeater, 99, 106);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Scroll into view item 105 (already realized) w/ animation.");
                repeater.TryGetElement(105).StartBringIntoView(new BringIntoViewOptions
                {
                    VerticalAlignmentRatio = 0.5,
                    AnimationDesired = true
                });
                repeater.UpdateLayout();
            });

            Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();
            Verify.IsLessThanOrEqual(1, viewChangedOffsets.Count);
            viewChangedOffsets.Clear();
            ValidateRealizedRange(repeater, 99, 106);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Scroll item 0 to the top w/ animation and 0.5 vertical alignment.");
                repeater.GetOrCreateElement(0).StartBringIntoView(new BringIntoViewOptions
                {
                    VerticalAlignmentRatio = 0.5,
                    AnimationDesired = true
                });
            });

            Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();
            viewChangedOffsets.Clear();
            ValidateRealizedRange(repeater, 0, 6);

            RunOnUIThread.Execute(() =>
            {
                // You can't align the first group in the middle obviously.
                Verify.AreEqual(0, scroller.VerticalOffset);

                Log.Comment("Scroll to item 20.");
                repeater.GetOrCreateElement(20).StartBringIntoView(new BringIntoViewOptions
                {
                    VerticalAlignmentRatio = 0.0
                });
                repeater.UpdateLayout();
            });

            Verify.IsTrue(viewChangeCompletedEvent.WaitOne(DefaultWaitTimeInMS));
            IdleSynchronizer.Wait();
            ValidateRealizedRange(repeater, 19, 26);
        }

        private void ValidateRealizedRange(
            ItemsRepeater repeater,
            int expectedFirstItemIndex,
            int expectedLastItemIndex)
        {
            Log.Comment("Validating Realized Range...");
            int actualFirstItemIndex = -1;
            int actualLastItemIndex = -1;
            int itemIndex = 0;
            RunOnUIThread.Execute(() =>
            {
                var items = repeater.ItemsSource as IEnumerable<string>;
                foreach (var item in items)
                {
                    var itemElement = repeater.TryGetElement(itemIndex);

                    if (itemElement != null)
                    {
                        actualFirstItemIndex =
                            actualFirstItemIndex == -1 ?
                            itemIndex :
                            actualFirstItemIndex;
                        actualLastItemIndex = itemIndex;
                    }

                    ++itemIndex;
                }
            });

            Log.Comment(string.Format("FirstItemIndex      - {0}   {1}", expectedFirstItemIndex, actualFirstItemIndex));
            Log.Comment(string.Format("LastItemIndex       - {0}   {1}", expectedLastItemIndex, actualLastItemIndex));
            Verify.AreEqual(expectedFirstItemIndex, actualFirstItemIndex);
            Verify.AreEqual(expectedLastItemIndex, actualLastItemIndex);
        }

        private static VirtualizingLayout GetMonitoringLayout(Size desiredSize, List<Rect> realizationRects)
        {
            return new MockVirtualizingLayout
            {
                MeasureLayoutFunc = (availableSize, context) =>
                {
                    var ctx = (VirtualizingLayoutContext)context;
                    realizationRects.Add(ctx.RealizationRect);
                    return desiredSize;
                },

                ArrangeLayoutFunc = (finalSize, context) => finalSize
            };
        }
    }
}

#endif
