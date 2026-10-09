// Copyright (c) Microsoft Corporation. All rights reserved.
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
    public partial class LayoutTests : ApiTestBase
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

        // Scenario: use a StackLayout whose items keep changing size so layout does not settle for many passes.
        // Expected: the repeater stops re-measuring after its fixed iteration limit and keeps the last extent (within
        //           1px).
        // A failure means: an unsettled StackLayout could hit a layout cycle and crash the app.
        [TestMethod]
        [TestProperty("Description", "Verifies that ItemsRepeater short-circuits an unsettled StackLayout measure loop after 60 consecutive measures, returning the last layout extent instead of re-measuring the layout.")]
        public void VerifyStackLayoutCycleShortcut()
        {
            // Must match ItemsRepeater::s_maxStackLayoutIterations.
            const int maxStackLayoutIterations = 60;
            const int arrangeInvalidationLimit = maxStackLayoutIterations + 10;

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
                    // Keep the layout unsettled for a bounded number of passes so that the
                    // ItemsRepeater shortcut engages without hitting the framework's layout cycle limit.
                    if (arrangeCount < arrangeInvalidationLimit)
                    {
                        mockStackLayout.InvalidateMeasure();
                    }
                    return new Size(100, 200 + arrangeCount);
                };
                
                repeater.Layout = mockStackLayout;
                repeater.ItemsSource = Enumerable.Range(0, 10);
                repeater.ItemTemplate = GetDataTemplate("<Button Content='{Binding}' Height='200'/>");
                repeater.VerticalAlignment = VerticalAlignment.Top;
                repeater.HorizontalAlignment = HorizontalAlignment.Left;

                Content = repeater;
                Content.UpdateLayout();

                Log.Comment($"measureCount={measureCount}, arrangeCount={arrangeCount}, DesiredSize={repeater.DesiredSize}");
                Verify.IsGreaterThanOrEqual(arrangeCount, arrangeInvalidationLimit, "The layout was kept unsettled for the expected number of passes.");
                Verify.AreEqual(maxStackLayoutIterations - 1, measureCount, "Layout measures stop once the shortcut engages.");
                // Layout rounding at the display scale (e.g. 150%) can snap the desired size to the nearest physical pixel.
                Verify.IsLessThan(Math.Abs(repeater.DesiredSize.Width - 100.0), 1.0);
                Verify.IsLessThan(Math.Abs(repeater.DesiredSize.Height - (200.0 + maxStackLayoutIterations - 1)), 1.0, "The shortcut returns the last measured extent.");
            });
        }

        // Scenario: call InvalidateArrange on a custom layout used by a repeater.
        // Expected: ArrangeInvalidated is raised and the repeater re-arranges the layout without re-measuring it.
        // A failure means: custom layouts could not request a cheap re-arrange, or would trigger unnecessary measures.
        [TestMethod]
        [TestProperty("Description", "Verifies Layout.InvalidateArrange raises ArrangeInvalidated and makes the ItemsRepeater re-arrange its layout without re-measuring it.")]
        public void VerifyLayoutInvalidateArrangeRearrangesRepeater()
        {
            RunOnUIThread.Execute(() =>
            {
                var layout = new CountingStackingLayout();
                var repeater = new ItemsRepeater() {
                    ItemsSource = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = GetDataTemplate("<TextBlock Text='{Binding}' Height='20'/>"),
                    Layout = layout,
                    VerticalAlignment = VerticalAlignment.Top
                };
                Content = repeater;
                Content.UpdateLayout();

                int measureCount = layout.MeasureCount;
                int arrangeCount = layout.ArrangeCount;
                Verify.IsGreaterThan(arrangeCount, 0);

                int arrangeInvalidatedCount = 0;
                int measureInvalidatedCount = 0;
                layout.ArrangeInvalidated += (sender, args) => { Verify.AreSame(layout, sender); arrangeInvalidatedCount++; };
                layout.MeasureInvalidated += (sender, args) => measureInvalidatedCount++;

                layout.CallInvalidateArrange();
                Verify.AreEqual(1, arrangeInvalidatedCount);
                Verify.AreEqual(0, measureInvalidatedCount);

                Content.UpdateLayout();
                Verify.AreEqual(arrangeCount + 1, layout.ArrangeCount, "The repeater re-arranged its layout.");
                Verify.AreEqual(measureCount, layout.MeasureCount, "InvalidateArrange does not trigger a measure pass.");
            });
        }

        // Scenario: add and remove items in the source of a repeater that uses a non-virtualizing layout.
        // Expected: the repeater re-measures and the realized elements match the updated items.
        // A failure means: non-virtualizing layouts would show stale items after the data changes.
        [TestMethod]
        [TestProperty("Description", "Verifies a NonVirtualizingLayout ItemsRepeater re-measures and realizes elements when items are added to or removed from its source.")]
        public void VerifyNonVirtualLayoutRemeasuresOnCollectionChange()
        {
            RunOnUIThread.Execute(() =>
            {
                var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => "Item #" + i));
                var repeater = new ItemsRepeater() {
                    ItemsSource = data,
                    ItemTemplate = GetDataTemplate("<TextBlock Text='{Binding}' Height='20'/>"),
                    Layout = new NonVirtualStackLayout(),
                    VerticalAlignment = VerticalAlignment.Top
                };
                Content = repeater;
                Content.UpdateLayout();
                Verify.AreEqual(60.0, repeater.ActualHeight);

                data.Add("Item #3");
                Content.UpdateLayout();
                Verify.AreEqual(80.0, repeater.ActualHeight);
                var added = repeater.TryGetElement(3) as TextBlock;
                Verify.IsNotNull(added);
                Verify.AreEqual("Item #3", added.Text);

                data.RemoveAt(0);
                Content.UpdateLayout();
                Verify.AreEqual(60.0, repeater.ActualHeight);
                for (int i = 0; i < 3; i++)
                {
                    Verify.AreEqual("Item #" + (i + 1), ((TextBlock)repeater.TryGetElement(i)).Text);
                }
            });
        }

        // Scenario: run a virtualizing layout through the context of a non-virtualizing layout.
        // Expected: the items are the children, the realization window is infinite, there is no anchor, recycling does
        //           nothing and the layout origin must stay at (0,0).
        // A failure means: layouts that wrap other layouts would get a wrong view of the items and break.
        [TestMethod]
        [TestProperty("Description", "Verifies the context a VirtualizingLayout receives when hosted by a NonVirtualizingLayout: items are the children, the realization window is infinite, there is no anchor, recycling is a no-op and LayoutOrigin must stay at (0,0).")]
        public void VerifyVirtualizingLayoutHostedByNonVirtualizingLayoutContext()
        {
            RunOnUIThread.Execute(() =>
            {
                var probe = new ProbeVirtualizingLayout();
                var repeater = new ItemsRepeater() {
                    ItemsSource = Enumerable.Range(0, 4).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = GetDataTemplate("<TextBlock Text='{Binding}' Height='20'/>"),
                    Layout = new WrappingNonVirtualizingLayout(probe),
                    VerticalAlignment = VerticalAlignment.Top
                };

                bool probed = false;
                probe.MeasureProbe = (context) =>
                {
                    if (probed)
                    {
                        return;
                    }
                    probed = true;

                    Verify.AreEqual(4, context.ItemCount);
                    var infinite = new Rect(0, 0, double.PositiveInfinity, double.PositiveInfinity);
                    Verify.AreEqual(infinite, context.RealizationRect);
                    Verify.AreEqual(infinite, context.VisibleRect);
                    Verify.AreEqual(-1, context.RecommendedAnchorIndex);
                    Verify.AreEqual(new Point(0, 0), context.LayoutOrigin);

                    for (int i = 0; i < 4; i++)
                    {
                        var element = context.GetOrCreateElementAt(i);
                        Verify.IsNotNull(element);
                        Verify.AreSame(element, context.GetItemAt(i), "Items of a non-virtualizing context are its child elements.");
                        Verify.AreSame(element, context.GetOrCreateElementAt(i, ElementRealizationOptions.None));
                        Verify.AreEqual("Item #" + i, ((TextBlock)element).Text);
                    }

                    var first = context.GetOrCreateElementAt(0);
                    context.RecycleElement(first);
                    Verify.AreSame(first, context.GetOrCreateElementAt(0), "RecycleElement is a no-op for non-virtualizing contexts.");

                    context.LayoutOrigin = new Point(0, 0);
                    probe.LayoutOriginHResult = CaptureHResult(() => context.LayoutOrigin = new Point(5, 0));
                    Verify.AreEqual(new Point(0, 0), context.LayoutOrigin);
                };

                Content = repeater;
                Content.UpdateLayout();

                Verify.IsTrue(probed, "The hosted VirtualizingLayout was measured.");
                Verify.AreEqual(E_INVALIDARG, probe.LayoutOriginHResult, "A non-zero LayoutOrigin is rejected.");
                Verify.AreEqual(80.0, repeater.ActualHeight, "The hosted layout stacked the four 20px children.");
            });
        }

        // Scenario: store an object in the context's LayoutState during initialization of a non-virtualizing layout.
        // Expected: the same object is read back in later measure/arrange passes and when the layout is uninitialized.
        // A failure means: custom layouts would lose their per-repeater state between layout passes.
        [TestMethod]
        [TestProperty("Description", "Verifies a NonVirtualizingLayout hosted by ItemsRepeater can store state in its context's LayoutState and read it back in later passes and on uninitialization.")]
        public void VerifyNonVirtualizingLayoutStateRoundTrip()
        {
            RunOnUIThread.Execute(() =>
            {
                var layout = new StatefulNonVirtualizingLayout();
                var repeater = new ItemsRepeater() {
                    ItemsSource = Enumerable.Range(0, 2).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = GetDataTemplate("<TextBlock Text='{Binding}' Height='20'/>"),
                    Layout = layout
                };

                Verify.IsNotNull(layout.InitialState, "InitializeForContextCore ran.");
                Content = repeater;
                Content.UpdateLayout();
                repeater.InvalidateMeasure();
                Content.UpdateLayout();

                Verify.IsGreaterThanOrEqual(layout.MeasuredStates.Count, 2);
                foreach (var state in layout.MeasuredStates)
                {
                    Verify.AreSame(layout.InitialState, state, "LayoutState persists across measure passes.");
                }

                repeater.Layout = new StackLayout();
                Verify.AreSame(layout.InitialState, layout.UninitializedState, "LayoutState is still available on uninitialization.");
            });
        }

        // Scenario: call the default implementations of the overridable layout, context, element factory and transition
        //           members.
        // Expected: they fail with E_NOTIMPL, except the virtualizing layout's default arrange, which returns the final
        //           size (within layout rounding).
        // A failure means: derived classes that skip an override would behave differently than documented.
        [TestMethod]
        [TestProperty("Description", "Verifies the default implementations of the overridable layout, context, element factory and transition provider members: E_NOTIMPL except VirtualizingLayout.ArrangeOverride which returns the final size.")]
        public void VerifyDefaultOverridableImplementations()
        {
            RunOnUIThread.Execute(() =>
            {
                Log.Comment("VirtualizingLayoutContext default Core members.");
                var context = new EmptyVirtualizingLayoutContext();
                VerifyNotImplemented(() => { var x = context.ItemCount; }, "ItemCount");
                VerifyNotImplemented(() => context.GetItemAt(0), "GetItemAt");
                VerifyNotImplemented(() => context.GetOrCreateElementAt(0), "GetOrCreateElementAt");
                VerifyNotImplemented(() => context.GetOrCreateElementAt(0, ElementRealizationOptions.ForceCreate), "GetOrCreateElementAt(options)");
                VerifyNotImplemented(() => context.RecycleElement(new Border()), "RecycleElement");
                VerifyNotImplemented(() => { var x = context.RealizationRect; }, "RealizationRect");
                VerifyNotImplemented(() => { var x = context.VisibleRect; }, "VisibleRect");
                VerifyNotImplemented(() => { var x = context.RecommendedAnchorIndex; }, "RecommendedAnchorIndex");
                VerifyNotImplemented(() => { var x = context.LayoutOrigin; }, "LayoutOrigin get");
                VerifyNotImplemented(() => context.LayoutOrigin = new Point(1, 1), "LayoutOrigin set");
                VerifyNotImplemented(() => { var x = context.LayoutState; }, "LayoutState get");
                VerifyNotImplemented(() => context.LayoutState = new object(), "LayoutState set");

                Log.Comment("NonVirtualizingLayoutContext default ChildrenCore.");
                var nonVirtualizingContext = new EmptyNonVirtualizingLayoutContext();
                VerifyNotImplemented(() => { var x = nonVirtualizingContext.Children; }, "Children");

                Log.Comment("ElementFactory default Core members.");
                var factory = new EmptyElementFactory();
                VerifyNotImplemented(() => factory.GetElement(new ElementFactoryGetArgs() { Data = 1 }), "GetElement");
                VerifyNotImplemented(() => factory.RecycleElement(new ElementFactoryRecycleArgs() { Element = new Border() }), "RecycleElement");

                Log.Comment("ItemCollectionTransitionProvider default Core members.");
                var transitionProvider = new EmptyItemCollectionTransitionProvider();
                VerifyNotImplemented(() => transitionProvider.ShouldAnimate(null), "ShouldAnimate");
                VerifyNotImplemented(() => transitionProvider.CallBaseStartTransitions(), "StartTransitions");

                Log.Comment("NonVirtualizingLayout default overrides.");
                var nonVirtualizingLayout = new BaseCallingNonVirtualizingLayout();
                var nonVirtualizingRepeater = new ItemsRepeater() { ItemsSource = Enumerable.Range(0, 2).ToList(), Layout = nonVirtualizingLayout };
                Content = nonVirtualizingRepeater;
                Content.UpdateLayout();
                Verify.AreEqual(E_NOTIMPL, nonVirtualizingLayout.BaseMeasureHResult, "NonVirtualizingLayout.MeasureOverride");
                Verify.AreEqual(E_NOTIMPL, nonVirtualizingLayout.BaseArrangeHResult, "NonVirtualizingLayout.ArrangeOverride");

                Log.Comment("VirtualizingLayout default overrides.");
                var virtualizingLayout = new BaseCallingVirtualizingLayout();
                var virtualizingRepeater = new ItemsRepeater() {
                    ItemsSource = Enumerable.Range(0, 2).ToList(),
                    Layout = virtualizingLayout,
                    Width = 120,
                    Height = 48
                };
                Content = virtualizingRepeater;
                Content.UpdateLayout();
                Verify.AreEqual(E_NOTIMPL, virtualizingLayout.BaseMeasureHResult, "VirtualizingLayout.MeasureOverride");
                Log.Comment($"ArrangeOverride finalSize={virtualizingLayout.ArrangeFinalSize}, base result={virtualizingLayout.BaseArrangeResult}");
                // The final size can be snapped to physical pixels at non-100% display scales.
                Verify.IsLessThan(Math.Abs(virtualizingLayout.ArrangeFinalSize.Width - 120), 1.0);
                Verify.IsLessThan(Math.Abs(virtualizingLayout.ArrangeFinalSize.Height - 48), 1.0);
                Verify.AreEqual(virtualizingLayout.ArrangeFinalSize, virtualizingLayout.BaseArrangeResult, "VirtualizingLayout.ArrangeOverride returns the final size.");
            });
        }

        // Scenario: wrap StackLayout, UniformGridLayout and FlowLayout in a layout that stores a layout state in the context
        //           before initializing the built-in layout: first a state of the matching type, then an unrelated object.
        // Expected: a matching state object is adopted as is and the items are laid out; an unrelated object is rejected
        //           (E_NOINTERFACE); switching the repeater to another layout uninitializes the wrapped layout cleanly.
        // A failure means: layouts composed of built-in layouts could lose their state or corrupt another layout's state.
        [TestMethod]
        [TestProperty("Description", "Verifies built-in layouts adopt an existing LayoutState of their own type and reject a foreign LayoutState.")]
        public void VerifyBuiltInLayoutsAdoptOrRejectExistingLayoutState()
        {
            var cases = new List<Tuple<string, Func<VirtualizingLayout>, Func<object>, Point[]>>()
            {
                Tuple.Create<string, Func<VirtualizingLayout>, Func<object>, Point[]>("StackLayout",
                    () => new StackLayout(), () => new StackLayoutState(),
                    // StackLayout gives each item the full 200px width; the 50px Border is centered in it.
                    new Point[] { new Point(75, 0), new Point(75, 20), new Point(75, 40) }),
                Tuple.Create<string, Func<VirtualizingLayout>, Func<object>, Point[]>("UniformGridLayout",
                    () => new UniformGridLayout() { MinItemWidth = 50, MinItemHeight = 20 }, () => new UniformGridLayoutState(),
                    new Point[] { new Point(0, 0), new Point(50, 0), new Point(100, 0) }),
                Tuple.Create<string, Func<VirtualizingLayout>, Func<object>, Point[]>("FlowLayout",
                    () => new FlowLayout(), () => new FlowLayoutState(),
                    new Point[] { new Point(0, 0), new Point(50, 0), new Point(100, 0) }),
            };

            foreach (var testCase in cases)
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment(testCase.Item1 + " with a matching state.");
                    var matching = new StateSeedingLayout(testCase.Item2(), testCase.Item3);
                    var repeater = CreateFixedItemsRepeater(matching);
                    Content = repeater;
                    Content.UpdateLayout();
                    Verify.AreEqual(0, matching.InitializeHResult, "A state of the layout's own type is accepted.");
                    Verify.AreSame(matching.SeededState, matching.StateAfterInitialize, "The existing state object is adopted.");
                    for (int i = 0; i < 3; i++)
                    {
                        var offset = repeater.TryGetElement(i).TransformToVisual(repeater).TransformPoint(new Point());
                        Log.Comment($"Item {i}: {offset}");
                        Verify.IsLessThan(Math.Abs(offset.X - testCase.Item4[i].X), 1.0, $"Item {i} X");
                        Verify.IsLessThan(Math.Abs(offset.Y - testCase.Item4[i].Y), 1.0, $"Item {i} Y");
                    }

                    // Switching away uninitializes the wrapped layout through the wrapper.
                    repeater.Layout = new StackLayout();
                    Content.UpdateLayout();
                    Verify.AreEqual(1, matching.UninitializeCount);
                    Verify.IsNotNull(repeater.TryGetElement(2), "The new layout lays the items out.");

                    Log.Comment(testCase.Item1 + " with a foreign state.");
                    var foreign = new StateSeedingLayout(testCase.Item2(), () => new object());
                    repeater.Layout = foreign;
                    Content.UpdateLayout();
                    // The layouts cast the state with as<>(), so the cast fails with E_NOINTERFACE before their own
                    // "LayoutState must derive from ..." E_FAIL check can run.
                    Verify.AreEqual(E_NOINTERFACE, foreign.InitializeHResult, "A foreign state object is rejected.");
                    Verify.IsNotNull(repeater.TryGetElement(2), "After resetting the state, the layout works.");
                    Content = null;
                });
            }
        }

        // Scenario: lay out three 100px items on a 500px wide FlowLayout line for every FlowLayoutLineAlignment value.
        // Expected: the 200px of free space is distributed as each alignment defines (start, end, center, around, between,
        //           evenly), within 1px.
        // A failure means: FlowLayout lines would be aligned differently than the LineAlignment the app selected.
        [TestMethod]
        [TestProperty("Description", "Verifies item positions on a partially filled FlowLayout line for every FlowLayoutLineAlignment value.")]
        public void VerifyFlowLayoutLineAlignmentPositions()
        {
            var expectations = new Dictionary<FlowLayoutLineAlignment, double[]>()
            {
                { FlowLayoutLineAlignment.Start, new double[] { 0, 100, 200 } },
                { FlowLayoutLineAlignment.End, new double[] { 200, 300, 400 } },
                { FlowLayoutLineAlignment.Center, new double[] { 100, 200, 300 } },
                { FlowLayoutLineAlignment.SpaceAround, new double[] { 200.0 / 6.0, 200, 400 - 200.0 / 6.0 } },
                { FlowLayoutLineAlignment.SpaceBetween, new double[] { 0, 200, 400 } },
                { FlowLayoutLineAlignment.SpaceEvenly, new double[] { 50, 200, 350 } },
            };

            foreach (var expectation in expectations)
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("LineAlignment=" + expectation.Key);
                    var repeater = new ItemsRepeater() {
                        ItemsSource = Enumerable.Range(0, 3).ToList(),
                        ItemTemplate = GetDataTemplate("<Border Width='100' Height='20'/>"),
                        Layout = new FlowLayout() { LineAlignment = expectation.Key },
                        Width = 500,
                        HorizontalAlignment = HorizontalAlignment.Left,
                        VerticalAlignment = VerticalAlignment.Top
                    };
                    Content = repeater;
                    Content.UpdateLayout();
                    for (int i = 0; i < 3; i++)
                    {
                        var offset = repeater.TryGetElement(i).TransformToVisual(repeater).TransformPoint(new Point());
                        Log.Comment($"Item {i}: X={offset.X}, Y={offset.Y}");
                        Verify.IsLessThan(Math.Abs(offset.X - expectation.Value[i]), 1.0, $"Item {i} X");
                        Verify.IsLessThan(Math.Abs(offset.Y), 1.0, $"Item {i} Y");
                    }
                    Content = null;
                });
            }
        }

        // Scenario: implement the overridable methods of VirtualizingLayoutContext in an app-defined context and call the
        //           public context members, then lay out a custom VirtualizingLayout directly with that context.
        // Expected: every public member backed by an overridable method returns what the override returns (ItemCount,
        //           GetItemAt, both GetOrCreateElementAt overloads, RecycleElement, RealizationRect, VisibleRect), and the
        //           layout measures the context's elements.
        // A failure means: app-provided layout contexts (for example to host a layout elsewhere) would be ignored.
        [TestMethod]
        [TestProperty("Description", "Verifies an app-defined VirtualizingLayoutContext's overrides back every public context member and drive a layout.")]
        public void VerifyCustomVirtualizingLayoutContextOverridesAreUsed()
        {
            RunOnUIThread.Execute(() =>
            {
                var context = new CustomVirtualizingLayoutContext(3);
                Verify.AreEqual(3, context.ItemCount);
                Verify.AreEqual("Data 1", context.GetItemAt(1));
                Verify.AreSame(context.Elements[2], context.GetOrCreateElementAt(2));
                Verify.AreEqual(ElementRealizationOptions.None, context.LastOptions);
                Verify.AreSame(context.Elements[0], context.GetOrCreateElementAt(0, ElementRealizationOptions.ForceCreate | ElementRealizationOptions.SuppressAutoRecycle));
                Verify.AreEqual(ElementRealizationOptions.ForceCreate | ElementRealizationOptions.SuppressAutoRecycle, context.LastOptions);
                context.RecycleElement(context.Elements[1]);
                Verify.AreSame(context.Elements[1], context.LastRecycled);
                Verify.AreEqual(new Rect(0, 10, 300, 400), context.RealizationRect);
                Verify.AreEqual(new Rect(0, 20, 300, 200), context.VisibleRect);

                var layout = new ProbeVirtualizingLayout();
                var desired = layout.Measure(context, new Size(300, double.PositiveInfinity));
                Verify.AreEqual(new Size(100, 30), desired, "The layout measured the three 10px elements supplied by the context.");
                Verify.AreEqual(new Size(300, 30), layout.Arrange(context, new Size(300, 30)));
            });
        }

        // Scenario: implement the property-style overridables RecommendedAnchorIndexCore and LayoutOriginCore in an
        //           app-defined VirtualizingLayoutContext and read/write the public properties.
        // Expected: RecommendedAnchorIndex returns 2 and LayoutOrigin reads back what was written, through the overrides.
        // Ignored: reproduces app overrides of property-style context members not being used (PC-CONTEXT-PROPERTY-OVERRIDES);
        //          currently reading RecommendedAnchorIndex terminates the process (0xC000027B).
        [TestMethod]
        [TestProperty("Ignore", "True")] // Product concern PC-CONTEXT-PROPERTY-OVERRIDES: expected RecommendedAnchorIndex/LayoutOrigin to come from the C# overrides; actual: reading RecommendedAnchorIndex crashes the test host (stowed exception 0xC000027B).
        [TestProperty("Description", "Verifies RecommendedAnchorIndex and LayoutOrigin of an app-defined VirtualizingLayoutContext are backed by its property overrides.")]
        public void VerifyCustomVirtualizingLayoutContextPropertyOverridesAreUsed()
        {
            RunOnUIThread.Execute(() =>
            {
                var context = new CustomVirtualizingLayoutContext(1);
                Verify.AreEqual(2, context.RecommendedAnchorIndex);
                Verify.AreEqual(new Point(0, 0), context.LayoutOrigin);
                context.LayoutOrigin = new Point(4, 6);
                Verify.AreEqual(new Point(4, 6), context.LayoutOrigin);
            });
        }

        // Scenario: implement NonVirtualizingLayoutContext.ChildrenCore in an app-defined context and lay out a custom
        //           non-virtualizing layout with it.
        // Expected: Children returns the override's elements and the layout measures exactly those elements.
        // Ignored: reproduces app overrides of property-style context members not being used (PC-CONTEXT-PROPERTY-OVERRIDES);
        //          currently reading Children terminates the process (0xC000027B).
        [TestMethod]
        [TestProperty("Ignore", "True")] // Product concern PC-CONTEXT-PROPERTY-OVERRIDES: expected Children to come from the C# ChildrenCore override; actual: reading Children crashes the test host (stowed exception 0xC000027B).
        [TestProperty("Description", "Verifies an app-defined NonVirtualizingLayoutContext's ChildrenCore backs Children and drives a layout.")]
        public void VerifyCustomNonVirtualizingLayoutContextChildrenAreUsed()
        {
            RunOnUIThread.Execute(() =>
            {
                var children = Enumerable.Range(0, 4).Select(i => (UIElement)new Border() { Width = 25, Height = 10 }).ToList();
                var context = new CustomNonVirtualizingLayoutContext(children);
                Verify.AreEqual(4, context.Children.Count);
                Verify.AreSame(children[3], context.Children[3]);

                var layout = new WrappingNonVirtualizingLayout(new ProbeVirtualizingLayout());
                var desired = layout.Measure(context, new Size(200, double.PositiveInfinity));
                Verify.AreEqual(new Size(100, 40), desired, "The layout measured the four 10px children from ChildrenCore.");
            });
        }

        // Scenario: implement LayoutContext.LayoutStateCore (get and set) in an app-defined VirtualizingLayoutContext and use
        //           the public LayoutState property.
        // Expected: reading and writing LayoutState go through the override.
        // Ignored: reproduces app overrides of property-style context members not being used (PC-CONTEXT-PROPERTY-OVERRIDES);
        //          currently setting LayoutState fails with E_NOTIMPL from the base class and the override is never called.
        [TestMethod]
        [TestProperty("Ignore", "True")] // Product concern PC-CONTEXT-PROPERTY-OVERRIDES: expected LayoutState to use the C# LayoutStateCore override; actual: E_NOTIMPL from LayoutContext::LayoutStateCore, override never called.
        [TestProperty("Description", "Verifies LayoutContext.LayoutState is backed by an app-defined LayoutStateCore override.")]
        public void VerifyLayoutStateCoreOverrideIsUsed()
        {
            RunOnUIThread.Execute(() =>
            {
                var context = new CustomVirtualizingLayoutContext(1);
                var state = new object();
                int setHResult = CaptureHResult(() => context.LayoutState = state);
                Log.Comment($"LayoutState set: 0x{setHResult:X8}, override set calls={context.LayoutStateSetCount}");
                Verify.AreEqual(0, setHResult, "Setting LayoutState uses the override.");
                Verify.AreEqual(1, context.LayoutStateSetCount);
                Verify.AreSame(state, context.LayoutState);
            });
        }

        // Scenario: use a custom layout that declares its item order with the protected SetIndexBasedLayoutOrientation and
        //           overrides CreateDefaultItemTransitionProvider, then assign it to an ItemsRepeater.
        // Expected: IndexBasedLayoutOrientation reports the declared value (default None), and the repeater asks the layout
        //           for its default transition provider when the layout is assigned.
        // A failure means: accessibility and keyboard navigation would not know a custom layout's item order, or custom
        //                  layouts could not supply default item transitions.
        [TestMethod]
        [TestProperty("Description", "Verifies Layout.SetIndexBasedLayoutOrientation/IndexBasedLayoutOrientation and that ItemsRepeater calls CreateDefaultItemTransitionProvider.")]
        public void VerifyCustomLayoutOrientationAndDefaultTransitionProvider()
        {
            RunOnUIThread.Execute(() =>
            {
                var layout = new OrientedProbeLayout();
                Verify.AreEqual(IndexBasedLayoutOrientation.None, layout.IndexBasedLayoutOrientation);
                layout.DeclareOrientation(IndexBasedLayoutOrientation.LeftToRight);
                Verify.AreEqual(IndexBasedLayoutOrientation.LeftToRight, layout.IndexBasedLayoutOrientation);
                layout.DeclareOrientation(IndexBasedLayoutOrientation.TopToBottom);
                Verify.AreEqual(IndexBasedLayoutOrientation.TopToBottom, layout.IndexBasedLayoutOrientation);

                var repeater = new ItemsRepeater() { ItemsSource = Enumerable.Range(0, 2).ToList(), Layout = layout };
                Content = repeater;
                Content.UpdateLayout();
                Verify.AreEqual(1, layout.CreateDefaultItemTransitionProviderCount, "The repeater asks the new layout for its default provider.");
                Verify.IsNotNull(repeater.TryGetElement(1));
                Content = null;
            });
        }

        private partial class CustomVirtualizingLayoutContext : VirtualizingLayoutContext
        {
            private Point _origin;
            private object _state;

            public CustomVirtualizingLayoutContext(int count)
            {
                Elements = Enumerable.Range(0, count).Select(i => (UIElement)new Border() { Width = 100, Height = 10 }).ToList();
            }

            public List<UIElement> Elements { get; }
            public ElementRealizationOptions LastOptions { get; private set; } = (ElementRealizationOptions)0xFF;
            public UIElement LastRecycled { get; private set; }
            public int LayoutStateSetCount { get; private set; }

            protected override int ItemCountCore() { return Elements.Count; }
            protected override object GetItemAtCore(int index) { return "Data " + index; }
            protected override UIElement GetOrCreateElementAtCore(int index, ElementRealizationOptions options)
            {
                LastOptions = options;
                return Elements[index];
            }
            protected override void RecycleElementCore(UIElement element) { LastRecycled = element; }
            protected override Rect RealizationRectCore() { return new Rect(0, 10, 300, 400); }
            protected override Rect VisibleRectCore() { return new Rect(0, 20, 300, 200); }
            protected override int RecommendedAnchorIndexCore { get { return 2; } }
            protected override Point LayoutOriginCore { get { return _origin; } set { _origin = value; } }
            protected override object LayoutStateCore
            {
                get { return _state; }
                set { LayoutStateSetCount++; _state = value; }
            }
        }

        private partial class CustomNonVirtualizingLayoutContext : NonVirtualizingLayoutContext
        {
            private readonly IReadOnlyList<UIElement> _children;

            public CustomNonVirtualizingLayoutContext(IReadOnlyList<UIElement> children)
            {
                _children = children;
            }

            protected override IReadOnlyList<UIElement> ChildrenCore { get { return _children; } }
        }

        private partial class OrientedProbeLayout : ProbeVirtualizingLayout
        {
            public int CreateDefaultItemTransitionProviderCount { get; private set; }

            public void DeclareOrientation(IndexBasedLayoutOrientation orientation)
            {
                SetIndexBasedLayoutOrientation(orientation);
            }

            protected override ItemCollectionTransitionProvider CreateDefaultItemTransitionProvider()
            {
                CreateDefaultItemTransitionProviderCount++;
                return null;
            }
        }

        private ItemsRepeater CreateFixedItemsRepeater(VirtualizingLayout layout)
        {
            return new ItemsRepeater() {
                ItemsSource = Enumerable.Range(0, 3).ToList(),
                ItemTemplate = GetDataTemplate("<Border Width='50' Height='20'/>"),
                Layout = layout,
                Width = 200,
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top
            };
        }

        private partial class StateSeedingLayout : VirtualizingLayout
        {
            private readonly VirtualizingLayout _inner;
            private readonly Func<object> _seed;

            public StateSeedingLayout(VirtualizingLayout inner, Func<object> seed)
            {
                _inner = inner;
                _seed = seed;
            }

            public int InitializeHResult { get; private set; } = -1;
            public object SeededState { get; private set; }
            public object StateAfterInitialize { get; private set; }
            public int UninitializeCount { get; private set; }

            protected override void InitializeForContextCore(VirtualizingLayoutContext context)
            {
                SeededState = _seed();
                context.LayoutState = SeededState;
                InitializeHResult = CaptureHResult(() => _inner.InitializeForContext(context));
                if (InitializeHResult != 0)
                {
                    context.LayoutState = null;
                    _inner.InitializeForContext(context);
                }
                StateAfterInitialize = context.LayoutState;
            }

            protected override void UninitializeForContextCore(VirtualizingLayoutContext context)
            {
                UninitializeCount++;
                _inner.UninitializeForContext(context);
            }

            protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
            {
                return _inner.Measure(context, availableSize);
            }

            protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
            {
                return _inner.Arrange(context, finalSize);
            }
        }

        private const int E_FAIL = unchecked((int)0x80004005);
        private const int E_NOINTERFACE = unchecked((int)0x80004002);
        private const int E_NOTIMPL = unchecked((int)0x80004001);
        private const int E_INVALIDARG = unchecked((int)0x80070057);

        private static int CaptureHResult(Action action)
        {
            try
            {
                action();
            }
            catch (Exception e)
            {
                Log.Comment($"Caught {e.GetType().Name}: HResult=0x{e.HResult:X8} '{e.Message}'");
                return e.HResult;
            }
            return 0;
        }

        private static void VerifyNotImplemented(Action action, string context)
        {
            Verify.AreEqual(E_NOTIMPL, CaptureHResult(action), context);
        }

        private partial class CountingStackingLayout : VirtualizingLayout
        {
            public int MeasureCount { get; private set; }
            public int ArrangeCount { get; private set; }

            public void CallInvalidateArrange()
            {
                InvalidateArrange();
            }

            protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
            {
                MeasureCount++;
                double height = 0;
                for (int i = 0; i < context.ItemCount; i++)
                {
                    var element = context.GetOrCreateElementAt(i);
                    element.Measure(availableSize);
                    height += element.DesiredSize.Height;
                }
                return new Size(100, height);
            }

            protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
            {
                ArrangeCount++;
                double offset = 0;
                for (int i = 0; i < context.ItemCount; i++)
                {
                    var element = context.GetOrCreateElementAt(i);
                    element.Arrange(new Rect(0, offset, finalSize.Width, element.DesiredSize.Height));
                    offset += element.DesiredSize.Height;
                }
                return finalSize;
            }
        }

        private partial class ProbeVirtualizingLayout : VirtualizingLayout
        {
            public Action<VirtualizingLayoutContext> MeasureProbe { get; set; }
            public int LayoutOriginHResult { get; set; }

            protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
            {
                MeasureProbe?.Invoke(context);
                double height = 0;
                for (int i = 0; i < context.ItemCount; i++)
                {
                    var element = context.GetOrCreateElementAt(i);
                    element.Measure(availableSize);
                    height += element.DesiredSize.Height;
                }
                return new Size(100, height);
            }

            protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
            {
                double offset = 0;
                for (int i = 0; i < context.ItemCount; i++)
                {
                    var element = context.GetOrCreateElementAt(i);
                    element.Arrange(new Rect(0, offset, finalSize.Width, element.DesiredSize.Height));
                    offset += element.DesiredSize.Height;
                }
                return finalSize;
            }
        }

        private partial class WrappingNonVirtualizingLayout : NonVirtualizingLayout
        {
            private readonly Layout _inner;

            public WrappingNonVirtualizingLayout(Layout inner)
            {
                _inner = inner;
            }

            protected override void InitializeForContextCore(NonVirtualizingLayoutContext context)
            {
                _inner.InitializeForContext(context);
            }

            protected override void UninitializeForContextCore(NonVirtualizingLayoutContext context)
            {
                _inner.UninitializeForContext(context);
            }

            protected override Size MeasureOverride(NonVirtualizingLayoutContext context, Size availableSize)
            {
                return _inner.Measure(context, availableSize);
            }

            protected override Size ArrangeOverride(NonVirtualizingLayoutContext context, Size finalSize)
            {
                return _inner.Arrange(context, finalSize);
            }
        }

        private partial class StatefulNonVirtualizingLayout : NonVirtualizingLayout
        {
            public object InitialState { get; private set; }
            public object UninitializedState { get; private set; }
            public List<object> MeasuredStates { get; } = new List<object>();

            protected override void InitializeForContextCore(NonVirtualizingLayoutContext context)
            {
                InitialState = new object();
                context.LayoutState = InitialState;
            }

            protected override void UninitializeForContextCore(NonVirtualizingLayoutContext context)
            {
                UninitializedState = context.LayoutState;
                context.LayoutState = null;
            }

            protected override Size MeasureOverride(NonVirtualizingLayoutContext context, Size availableSize)
            {
                MeasuredStates.Add(context.LayoutState);
                foreach (var child in context.Children)
                {
                    child.Measure(availableSize);
                }
                return new Size(100, 20 * context.Children.Count);
            }

            protected override Size ArrangeOverride(NonVirtualizingLayoutContext context, Size finalSize)
            {
                double offset = 0;
                foreach (var child in context.Children)
                {
                    child.Arrange(new Rect(0, offset, finalSize.Width, 20));
                    offset += 20;
                }
                return finalSize;
            }
        }

        private partial class EmptyVirtualizingLayoutContext : VirtualizingLayoutContext
        {
        }

        private partial class EmptyNonVirtualizingLayoutContext : NonVirtualizingLayoutContext
        {
        }

        private partial class EmptyElementFactory : ElementFactory
        {
        }

        private partial class EmptyItemCollectionTransitionProvider : ItemCollectionTransitionProvider
        {
            public void CallBaseStartTransitions()
            {
                base.StartTransitions(new List<ItemCollectionTransition>());
            }
        }

        private partial class BaseCallingNonVirtualizingLayout : NonVirtualizingLayout
        {
            public int BaseMeasureHResult { get; private set; } = -1;
            public int BaseArrangeHResult { get; private set; } = -1;

            protected override Size MeasureOverride(NonVirtualizingLayoutContext context, Size availableSize)
            {
                BaseMeasureHResult = CaptureHResult(() => base.MeasureOverride(context, availableSize));
                return new Size(10, 10);
            }

            protected override Size ArrangeOverride(NonVirtualizingLayoutContext context, Size finalSize)
            {
                BaseArrangeHResult = CaptureHResult(() => base.ArrangeOverride(context, finalSize));
                return finalSize;
            }
        }

        private partial class BaseCallingVirtualizingLayout : VirtualizingLayout
        {
            public int BaseMeasureHResult { get; private set; } = -1;
            public Size ArrangeFinalSize { get; private set; }
            public Size BaseArrangeResult { get; private set; }

            protected override Size MeasureOverride(VirtualizingLayoutContext context, Size availableSize)
            {
                BaseMeasureHResult = CaptureHResult(() => base.MeasureOverride(context, availableSize));
                return new Size(10, 10);
            }

            protected override Size ArrangeOverride(VirtualizingLayoutContext context, Size finalSize)
            {
                ArrangeFinalSize = finalSize;
                BaseArrangeResult = base.ArrangeOverride(context, finalSize);
                return BaseArrangeResult;
            }
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
