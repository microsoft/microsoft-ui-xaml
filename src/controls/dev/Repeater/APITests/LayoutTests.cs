﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;
using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.Mocks;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading;
using Windows.Foundation;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class LayoutTests : ApiTestBase
    {
        [TestMethod]
        public void ValidateMappingAndAutoRecycling()
        {
            ItemsRepeater repeater = null;
            ScrollViewer scrollViewer = null;
            RunOnUIThread.Execute(() =>
            {
                var layout = new MockVirtualizingLayout()
                {
                    MeasureLayoutFunc = (availableSize, context) =>
                    {
                        var element0 = context.GetOrCreateElementAt(index: 0);
                        // lookup - repeater will give back the same element and note that this element will not
                        // be pinned - i.e it will be auto recycled after a measure pass where GetElementAt(0) is not called.
                        var element0lookup = context.GetOrCreateElementAt(index: 0, options: ElementRealizationOptions.None);

                        var element1 = context.GetOrCreateElementAt(index: 1, options: ElementRealizationOptions.ForceCreate | ElementRealizationOptions.SuppressAutoRecycle);
                        // forcing a new element for index 1 that will be pinned (not auto recycled). This will be 
                        // a completely new element. Repeater does not do the mapping/lookup when forceCreate is true.
                        var element1Clone = context.GetOrCreateElementAt(index: 1, options: ElementRealizationOptions.ForceCreate | ElementRealizationOptions.SuppressAutoRecycle);

                        Verify.AreSame(element0, element0lookup);
                        Verify.AreNotSame(element1, element1Clone);

                        element0.Measure(availableSize);
                        element1.Measure(availableSize);
                        element1Clone.Measure(availableSize);
                        return new Size(100, 100);
                    },
                };

                Content = CreateAndInitializeRepeater(
                    itemsSource: Enumerable.Range(0, 5),
                    layout: layout,
                    elementFactory: GetDataTemplate("<Button>Hello</Button>"),
                    repeater: ref repeater,
                    scrollViewer: ref scrollViewer);

                Content.UpdateLayout();

                Verify.IsNotNull(repeater.TryGetElement(0));
                Verify.IsNotNull(repeater.TryGetElement(1));

                layout.MeasureLayoutFunc = null;

                repeater.InvalidateMeasure();
                Content.UpdateLayout();

                Verify.IsNull(repeater.TryGetElement(0)); // not pinned, should be auto recycled.
                Verify.IsNotNull(repeater.TryGetElement(1)); // pinned, should stay alive
            });
        }

        [TestMethod]
        public void ValidateNonVirtualLayoutWithItemsRepeater()
        {
            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater();
                repeater.Layout = new NonVirtualStackLayout();
                repeater.ItemsSource = Enumerable.Range(0, 10);
                repeater.ItemTemplate = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                         <Button Content='{Binding}' Height='100' />
                    </DataTemplate>");

                Content = repeater;
                Content.UpdateLayout();

                double expectedYOffset = 0;
                for (int i = 0; i < repeater.ItemsSourceView.Count; i++)
                {
                    var child = repeater.TryGetElement(i) as Button;
                    Verify.IsNotNull(child);
                    var layoutBounds = LayoutInformation.GetLayoutSlot(child);
                    Verify.AreEqual(expectedYOffset, layoutBounds.Y);
                    Verify.AreEqual(i, child.Content);
                    expectedYOffset += 100;
                }
            });
        }

        [TestMethod]
        public void VerifyItemsRepeaterNullLayoutIsTreatedAsStackLayout()
        {
            // ItemsRepeater.Layout defaults to 'null'.
            // The null Layout is internally treated as a StackLayout.
            // Verify that it behaves correctly and that if we switch Layout away from null and back again it 
            // continues to work correctly.
            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater();
                repeater.ItemsSource = Enumerable.Range(0, 10);
                repeater.ItemTemplate = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                         <Button Content='{Binding}' Height='10' Width='100' />
                    </DataTemplate>");

                Action validateStackLayout = () =>
                {
                    double expectedYOffset = 0;
                    for (int i = 0; i < repeater.ItemsSourceView.Count; i++)
                    {
                        var child = repeater.TryGetElement(i) as Button;
                        Verify.IsNotNull(child);
                        var layoutBounds = LayoutInformation.GetLayoutSlot(child);
                        Verify.AreEqual(expectedYOffset, layoutBounds.Y);
                        Verify.AreEqual(i, child.Content);
                        expectedYOffset += 10;
                    }
                };

                Action validateGridLayout = () =>
                {
                    int totalColumns = 5;
                    int currentCol = 0;
                    int currentRow = 0;
                    for (int i = 0; i < repeater.ItemsSourceView.Count; i++)
                    {
                        double expectedYOffset = currentRow * 10;
                        double expectedXOffset = currentCol * 100;
                        var child = repeater.TryGetElement(i) as Button;
                        Verify.IsNotNull(child);
                        var layoutBounds = LayoutInformation.GetLayoutSlot(child);
                        Verify.AreEqual(expectedYOffset, layoutBounds.Y);
                        Verify.AreEqual(expectedXOffset, layoutBounds.X);
                        Verify.AreEqual(i, child.Content);
                        currentCol++;
                        if(currentCol == totalColumns)
                        {
                            currentCol = 0;
                            currentRow++;
                        }
                    }
                };

                Grid grid = new Grid 
                {
                    Width = 500,
                    Height = 500,
                };
                grid.Children.Add(repeater);
                Content = grid;

                Log.Comment("Validate repeater with default 'null' Layout");
                Verify.IsNull(repeater.Layout);
                Content.UpdateLayout();
                validateStackLayout();

                Log.Comment("Switch Layout to UniformGridLayout ");
                var uniformGridLayout = new UniformGridLayout 
                {
                    MaximumRowsOrColumns = 5,
                    MinItemHeight = 10,
                    MinItemWidth = 100,
                };
                repeater.Layout = uniformGridLayout;
                Content.UpdateLayout();
                validateGridLayout();

                Log.Comment("Switch Layout back to 'null' Layout");
                repeater.Layout = null;
                Content.UpdateLayout();
                validateStackLayout();
            });
        }

        [TestMethod]
        public void ValidateNonVirtualLayoutDoesNotGetMeasuredForViewportChanges()
        {
            RunOnUIThread.Execute(() =>
            {
                int measureCount = 0;
                int arrangeCount = 0;
                var repeater = new ItemsRepeater();

                // with a non virtualizing layout, repeater will just
                // run layout once. 
                repeater.Layout = new MockNonVirtualizingLayout() 
                {
                    MeasureLayoutFunc = (size, context) =>
                    {
                        measureCount++;
                        return new Size(100, 800);
                    },
                    ArrangeLayoutFunc = (size, context) =>
                    {
                        arrangeCount++;
                        return new Size(100, 800);
                    }
                };

                repeater.ItemsSource = Enumerable.Range(0, 10);
                repeater.ItemTemplate = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                         <Button Content='{Binding}' Height='100' />
                    </DataTemplate>");

                Content = new ScrollViewer() 
                {
                    Content = repeater
                };
                Content.UpdateLayout();

                Verify.AreEqual(1, measureCount);
                Verify.AreEqual(1, arrangeCount);

                measureCount = 0;
                arrangeCount = 0;

                // Once we switch to a virtualizing layout we should 
                // get at least two passes to update the viewport.
                repeater.Layout = new MockVirtualizingLayout() {
                    MeasureLayoutFunc = (size, context) =>
                    {
                        measureCount++;
                        return new Size(100, 800);
                    },
                    ArrangeLayoutFunc = (size, context) =>
                    {
                        arrangeCount++;
                        return new Size(100, 800);
                    }
                };

                Content.UpdateLayout();

                Verify.IsGreaterThan(measureCount, 1);
                Verify.IsGreaterThan(arrangeCount, 1);
            });
        }

        [TestMethod]
        public void ValidateStackLayoutDisabledVirtualizationWithItemsRepeater()
        {
            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater();
                var stackLayout = new StackLayout();
                stackLayout.IsVirtualizationEnabled = false;
                repeater.Layout = stackLayout;
                repeater.ItemsSource = Enumerable.Range(0, 10);
                repeater.ItemTemplate = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                         <Button Content='{Binding}' Height='100' />
                    </DataTemplate>");

                var scrollViewer = new ScrollViewer() {
                    Content = repeater
                };
                scrollViewer.Height = 100;
                Content = scrollViewer;
                Content.UpdateLayout();

                for (int i = 0; i < repeater.ItemsSourceView.Count; i++)
                {
                    var child = repeater.TryGetElement(i) as Button;
                    Verify.IsNotNull(child);
                }
            });
        }

        [TestMethod]
        public void VerifyStackLayoutCycleShortcut()
        {
            RunOnUIThread.Execute(() =>
            {
                int measureCount = 0;
                int arrangeCount = 0;
                var repeater = new ItemsRepeater();
                var mockStackLayout = new MockStackLayout();

                mockStackLayout.MeasureLayoutFunc = (size, context) =>
                {
                    // Simulating variable sized children that cause
                    // the ItemsRepeater's layout to not settle.
                    // This would normally cause a layout cycle but the use
                    // of ItemsRepeater::m_stackLayoutMeasureCounter avoids it.
                    mockStackLayout.InvalidateMeasure();
                    measureCount++;
                    return new Size(100, 200 + measureCount);
                };
                mockStackLayout.ArrangeLayoutFunc = (size, context) =>
                {
                    arrangeCount++;
                    return new Size(100, 200 + arrangeCount);
                };
                
                repeater.Layout = mockStackLayout;
                repeater.ItemsSource = Enumerable.Range(0, 10);
                repeater.ItemTemplate = GetDataTemplate("<Button Content='{Binding}' Height='200'/>");

                Content = repeater;
                Content.UpdateLayout();
            });
        }

        [TestMethod]
        public void VerifyStackLayoutAlignment()
        {
            for (int horizontalAlignment = 0; horizontalAlignment <= 3; horizontalAlignment++)
            {
                Border border = null;
                ItemsRepeater itemsRepeater = null;

                RunOnUIThread.Execute(() =>
                {
                    itemsRepeater = new ItemsRepeater()
                    {
                        HorizontalAlignment = (HorizontalAlignment)horizontalAlignment,
                        ItemsSource = Enumerable.Range(0, 2)
                    };

                    border = new Border()
                    {
                        Background = new SolidColorBrush(Colors.Azure),
                        Width = 400,
                        Height = 400,
                        Child = itemsRepeater
                    };

                    Content = border;
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"ItemsRepeater.HorizontalAlignment set to {itemsRepeater.HorizontalAlignment}");
                    Log.Comment($"ItemsRepeater actual size: {itemsRepeater.ActualWidth}, {itemsRepeater.ActualHeight}");

                    Verify.AreEqual((HorizontalAlignment)horizontalAlignment, itemsRepeater.HorizontalAlignment);

                    GeneralTransform gt = itemsRepeater.TransformToVisual(border);
                    Point itemsRepeaterOriginPoint = new Point();
                    itemsRepeaterOriginPoint = gt.TransformPoint(itemsRepeaterOriginPoint);
                    Log.Comment($"ItemsRepeater position: {itemsRepeaterOriginPoint}");

                    for (int itemIndex = 0; itemIndex <= 1; itemIndex++)
                    {
                        FrameworkElement itemElement = itemsRepeater.TryGetElement(itemIndex) as FrameworkElement;
                        Verify.IsNotNull(itemElement);
                        var layoutBounds = LayoutInformation.GetLayoutSlot(itemElement);
                        Log.Comment($"ItemsRepeater child #{itemIndex} layout bounds: {layoutBounds}");
                    }

                    switch (itemsRepeater.HorizontalAlignment)
                    {
                        case HorizontalAlignment.Left:
                        case HorizontalAlignment.Stretch:
                            Verify.AreEqual(0, itemsRepeaterOriginPoint.X);
                            break;
                        case HorizontalAlignment.Center:
                            Verify.IsTrue(itemsRepeaterOriginPoint.X > 0);
                            Verify.AreEqual((border.ActualWidth - itemsRepeater.ActualWidth) / 2, itemsRepeaterOriginPoint.X);
                            break;
                        case HorizontalAlignment.Right:
                            Verify.IsTrue(itemsRepeaterOriginPoint.X > 0);
                            Verify.AreEqual(border.ActualWidth - itemsRepeater.ActualWidth, itemsRepeaterOriginPoint.X);
                            break;
                    }
                });
            }
        }

        [TestMethod]
        public void VerifyUniformGridLayoutDoesntCrashWhenTryingToScrollToEnd()
        {
            ItemsRepeater repeater = null;
            ScrollViewer scrollViewer = null;
            RunOnUIThread.Execute(() =>
            {
                repeater = new ItemsRepeater {
                    ItemsSource = Enumerable.Range(0, 1000).Select(i => new Border {
                        Background = new SolidColorBrush(Colors.Blue),
                        Child = new TextBlock { Text = "#" + i }
                    }).ToArray(),
                    Layout = new UniformGridLayout {
                        MinItemWidth = 100,
                        MinItemHeight = 40,
                        MinRowSpacing = 10,
                        MinColumnSpacing = 10
                    }
                };
                scrollViewer = new ScrollViewer { Content = repeater };
                Content = scrollViewer;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                scrollViewer.ChangeView(0, repeater.ActualHeight, null);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                scrollViewer.ChangeView(0, 0, null);
            });

            IdleSynchronizer.Wait();

            // The test guards against an app crash, so this is enough to verify
            Verify.IsTrue(true);
        }

        [TestMethod]
        public void VerifyUniformGridLayoutDoesntHangWhenTryingToScrollToStart()
        {
            ItemsRepeater itemsRepeater = null;
            ScrollViewer scrollViewer = null;
            ManualResetEvent viewChanged = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Setting up UI.");

                itemsRepeater = new ItemsRepeater {
                    ItemsSource = Enumerable.Range(0, 10).Select(_ => Enumerable.Range(1, 6).ToList()).ToList()
                };
                itemsRepeater.ItemTemplate = XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                          <ItemsRepeater ItemsSource='{Binding}'>
                              <ItemsRepeater.Layout>
                                  <UniformGridLayout
                                      Orientation='Horizontal'
                                      MinItemWidth='150.0'
                                      MaximumRowsOrColumns='3'
                                      MinRowSpacing='4'
                                      MinColumnSpacing='4'
                                      MinItemHeight='150.0'
                                      ItemsStretch='Fill'/>
                              </ItemsRepeater.Layout>
                              <ItemsRepeater.ItemTemplate>
                                  <DataTemplate>
                                      <TextBlock Text='{Binding}'/>
                                  </DataTemplate>
                              </ItemsRepeater.ItemTemplate>
                           </ItemsRepeater>
                      </DataTemplate>") as DataTemplate;

                scrollViewer = new ScrollViewer {
                    Width = 500.0, 
                    Height = 400.0,
                    Content = itemsRepeater
                };
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ScrollViewer.ViewChanged raised for VerticalOffset '{scrollViewer.VerticalOffset}'.");

                    if (!args.IsIntermediate)
                    {
                        Log.Comment("ScrollViewer.ChangeView completed.");

                        viewChanged.Set();
                    }
                };

                Content = scrollViewer;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            Log.Comment("Scrolling to the bottom in 16 jumps...");

            for (int i = 1; i <= 16; i++)
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Jumping to VerticalOffset '{scrollViewer.ScrollableHeight * i / 16}'.");

                    scrollViewer.ChangeView(null, scrollViewer.ScrollableHeight * i / 16, null, true);
                });

                Verify.IsTrue(viewChanged.WaitOne());
                viewChanged.Reset();
                IdleSynchronizer.Wait();
            }

            Log.Comment("Scrolling to the top in 16 jumps...");

            for (int i = 15; i >= 0; i--)
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Jumping to VerticalOffset '{scrollViewer.ScrollableHeight * i / 16}'.");

                    scrollViewer.ChangeView(null, scrollViewer.ScrollableHeight * i / 16, null, true);
                });

                Verify.IsTrue(viewChanged.WaitOne());
                viewChanged.Reset();
                IdleSynchronizer.Wait();
            }
        }

        private ItemsRepeaterScrollHost CreateAndInitializeRepeater(
           object itemsSource,
           VirtualizingLayout layout,
           object elementFactory,
           ref ItemsRepeater repeater,
           ref ScrollViewer scrollViewer)
        {
            repeater = new ItemsRepeater()
            {
                ItemsSource = itemsSource,
                Layout = layout,
                ItemTemplate = elementFactory,
                HorizontalCacheLength = 0,
                VerticalCacheLength = 0,
            };

            scrollViewer = new ScrollViewer() 
            {
                Content = repeater
            };

            return new ItemsRepeaterScrollHost()
            {
                Width = 400,
                Height = 400,
                ScrollViewer = scrollViewer
            };
        }

        private DataTemplate GetDataTemplate(string content)
        {
            return (DataTemplate)XamlReader.Load(
                       string.Format(@"<DataTemplate  
                            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                           {0}
                        </DataTemplate>", content));
        }
    }
}
