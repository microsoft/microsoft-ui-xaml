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
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.HResults;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class ViewManagerTests : ApiTestBase
    {
        [TestMethod]
        public void CanQueryElementFactory()
        {
            RunOnUIThread.Execute(() =>
            {
                var data = new ObservableCollection<string>();
                var dataSource = MockItemsSource.CreateDataSource(data, supportsUniqueIds: false);
                var elementFactory = MockElementFactory.CreateElementFactory(new List<UIElement> { new ContentControl() });
                var repeater = CreateRepeater(dataSource, elementFactory);

                Content = repeater;
                repeater.UpdateLayout();

                // Our layout will query the size though once for every layout pass.
                // The first layout pass is a bit special because we don't have a viewport and
                // we will invalidate measure when we get one after the first arrange pass.
                dataSource.ValidateGetSizeCalls(1); // GetSize calls are cached by ItemsSourceView.
                elementFactory.ValidateRecycleElementCalls();

                data.Add("Item #1");
                repeater.UpdateLayout();

                var item1 = (UIElement)VisualTreeHelper.GetChild(repeater, 0);

                // One GetSize invocation from the layout, another one from the view manager.
                dataSource.ValidateGetSizeCalls(2); // GetSize calls are cached by ItemsSourceView
                dataSource.ValidateGetAtCalls(new MockItemsSource.GetAtCallInfo(0));
                elementFactory.ValidateGetElementCalls(new MockElementFactory.GetElementCallInfo(0, repeater));
                elementFactory.ValidateRecycleElementCalls();
                Verify.AreEqual(item1, repeater.TryGetElement(0));

                data.RemoveAt(0);
                repeater.UpdateLayout();

                dataSource.ValidateGetAtCalls();
                dataSource.ValidateGetSizeCalls(1); // GetSize calls are cached by ItemsSourceView
                // Whenever we get an element from the view generator, we call HasKeyIndexMapping to see if we should
                // store its unique id or not.
                dataSource.ValidateGetItemIdCalls();
                elementFactory.ValidateGetElementCalls();
                elementFactory.ValidateRecycleElementCalls(new MockElementFactory.RecycleElementCallInfo(item1, repeater));
            });
        }

        [TestMethod] //Issue #1018
        [TestProperty("Ignore", "True")]
        public void CanPinFocusedElements()
        {
            // Setup a grouped repeater scenario with two groups each containing two items.
            var data = new ObservableCollection<ObservableCollection<string>>(Enumerable
                .Range(0, 2)
                .Select(i => new ObservableCollection<string>(Enumerable
                    .Range(0, 2)
                    .Select(j => string.Format("Item #{0}.{1}", i, j)))));
            List<ContentControl>[] itemElements = null;
            ItemsRepeater[] innerRepeaters = null;
            List<StackPanel> groupElements = null;
            ItemsRepeater rootRepeater = null;
            var gotFocus = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                itemElements = new[] {
                    Enumerable.Range(0, 2).Select(i => new ContentControl()).ToList(),
                    Enumerable.Range(0, 2).Select(i => new ContentControl()).ToList()
                };

                itemElements[0][0].GotFocus += delegate { gotFocus.Set(); };

                innerRepeaters = Enumerable.Range(0, 2).Select(i => CreateRepeater(
                    MockItemsSource.CreateDataSource(data[i], supportsUniqueIds: false),
                    MockElementFactory.CreateElementFactory(itemElements[i]))).ToArray();

                groupElements = Enumerable.Range(0, 2).Select(i =>
                {
                    var panel = new StackPanel();
                    panel.Children.Add(new ContentControl());
                    panel.Children.Add(innerRepeaters[i]);
                    return panel;
                }).ToList();

                rootRepeater = CreateRepeater(
                   MockItemsSource.CreateDataSource(data, supportsUniqueIds: false),
                   MockElementFactory.CreateElementFactory(groupElements));
                Content = rootRepeater;
                rootRepeater.UpdateLayout();

                itemElements[0][0].Focus(FocusState.Keyboard);
            });

            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), "Waiting for focus event on the first element of the first group.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Recycle focused element 0.0 and validate it's still realized because it is pinned.");
                {
                    var ctx = (VirtualizingLayoutContext)innerRepeaters[0].Tag;
                    ctx.RecycleElement(itemElements[0][0]);
                    Verify.AreEqual(0, innerRepeaters[0].GetElementIndex(itemElements[0][0]));
                }

                Log.Comment("Recycle element 0.1 and validate it's no longer realized because it is not pinned.");
                {
                    var ctx = (VirtualizingLayoutContext)innerRepeaters[0].Tag;
                    ctx.RecycleElement(itemElements[0][1]);
                    Verify.AreEqual(-1, innerRepeaters[0].GetElementIndex(itemElements[0][1]));
                }

                Log.Comment("Recycle group 0 and validate it's still realized because one of its items is pinned.");
                {
                    var ctx = (VirtualizingLayoutContext)rootRepeater.Tag;
                    ctx.RecycleElement(groupElements[0]);
                    Verify.AreEqual(0, rootRepeater.GetElementIndex(groupElements[0]));
                }

                itemElements[1][1].GotFocus += delegate { gotFocus.Set(); };
                itemElements[1][1].Focus(FocusState.Keyboard);
            });

            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), "Waiting for focus event on the second element of the second group.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment(@"Move focus to item 1.1 and validate item 0.0 and group 0 are recycled because 
                 the only thing keeping them around is the fact that item 0.0 was focus pinned");
                {
                    ((VirtualizingLayoutContext)rootRepeater.Tag).RecycleElement(groupElements[0]);
                    ((VirtualizingLayoutContext)innerRepeaters[0].Tag).RecycleElement(itemElements[0][0]);

                    Verify.AreEqual(-1, rootRepeater.GetElementIndex(groupElements[0]));
                    Verify.AreEqual(-1, innerRepeaters[0].GetElementIndex(itemElements[0][0]));
                    Verify.AreEqual(1, innerRepeaters[0].GetElementIndex(itemElements[1][1]));
                }

                Log.Comment(@"Delete item 1.1 from the data. This will force the element to get recycled even if it's pinned.");
                {
                    data[1].RemoveAt(1);
                    rootRepeater.UpdateLayout();

                    Verify.AreEqual(-1, innerRepeaters[1].GetElementIndex(itemElements[1][1]));
                }
            });
        }

        [TestMethod] //Issue 1018
        [TestProperty("Ignore", "True")]
        public void CanReuseElementsDuringUniqueIdReset()
        {
            var data = new WinRTCollection(Enumerable.Range(0, 2).Select(i => string.Format("Item #{0}", i)));
            List<UIElement> mapping = null;
            ItemsRepeater repeater = null;
            MockElementFactory elementFactory = null;
            ContentControl focusedElement = null;

            RunOnUIThread.Execute(() =>
            {
                mapping = new List<UIElement> { new ContentControl(), new ContentControl() };
                repeater = CreateRepeater(
                    MockItemsSource.CreateDataSource(data, supportsUniqueIds: true),
                    MockElementFactory.CreateElementFactory(mapping));
                elementFactory = (MockElementFactory)repeater.ItemTemplate;

                Content = repeater;
                repeater.UpdateLayout();

                focusedElement = (ContentControl)repeater.TryGetElement(1);
                focusedElement.Focus(FocusState.Keyboard);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                elementFactory.ValidateGetElementCalls(
                new MockElementFactory.GetElementCallInfo(0, repeater),
                new MockElementFactory.GetElementCallInfo(1, repeater));
                elementFactory.ValidateRecycleElementCalls();

                data.ResetWith(new[] { data[0], "New item" });

                Verify.AreEqual(0, repeater.GetElementIndex(mapping[0]));
                Verify.AreEqual(1, repeater.GetElementIndex(mapping[1]));
                Verify.IsNull(repeater.TryGetElement(0));
                Verify.IsNull(repeater.TryGetElement(1));

                elementFactory.ValidateGetElementCalls(/* GetElement should not be called */);
                elementFactory.ValidateRecycleElementCalls(/* RecycleElement should not be called */);

                mapping[1] = new ContentControl(); // For "New Item" 

                repeater.UpdateLayout();

                Verify.AreEqual(0, repeater.GetElementIndex(mapping[0]));
                Verify.AreEqual(1, repeater.GetElementIndex(mapping[1]));
                Verify.AreEqual(mapping[0], repeater.TryGetElement(0));
                Verify.AreEqual(mapping[1], repeater.TryGetElement(1));

                elementFactory.ValidateGetElementCalls(
                    new MockElementFactory.GetElementCallInfo(1, repeater));
                elementFactory.ValidateRecycleElementCalls(
                    new MockElementFactory.RecycleElementCallInfo(focusedElement, repeater));

                // If the focused element survived the reset, we will keep focus on it. If not, we 
                // try to find one based on the index. In this case, the focused element (index 1) 
                // got recycled, and we still have index 1 after the stable reset, so the new index 1 
                // will get focused. Note that recycling the elements to view generator in the case of
                // stable reset happens during the arrange, so by that time we will have pulled elements
                // from the stable reset pool and maybe created some new elements as well.
                int index = repeater.GetElementIndex(focusedElement);
                Log.Comment("focused index " + index);
                Verify.AreEqual(mapping[1], FocusManager.GetFocusedElement(repeater.XamlRoot));
            });
        }

        [TestMethod]
        public void CanChangeFocusAfterUniqueIdReset()
        {
            var data = new WinRTCollection(Enumerable.Range(0, 2).Select(i => string.Format("Item #{0}", i)));
            object dataSource = null;
            RunOnUIThread.Execute(() => dataSource = MockItemsSource.CreateDataSource(data, supportsUniqueIds: true));
            ItemsRepeater repeater = SetupRepeater(dataSource);
            Control focusedElement = null;

            RunOnUIThread.Execute(() =>
            {
                focusedElement = (Control)repeater.TryGetElement(0);
                focusedElement.Focus(FocusState.Keyboard);
            });

            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                data.Reset();
            });

            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                // Still focused.
                Verify.AreEqual(focusedElement, FocusManager.GetFocusedElement(repeater.XamlRoot));

                // Change focused element.
                focusedElement = (Control)repeater.TryGetElement(1);
                focusedElement.Focus(FocusState.Keyboard);
            });

            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                // Focus is on the new element.
                Verify.AreEqual(focusedElement, FocusManager.GetFocusedElement(repeater.XamlRoot));
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the correct items are generated when invoking StartBringIntoView for a disconnected item.")]
        public void ValidateBringIntoViewOperations()
        {
            ValidateBringIntoViewOperations(0.0 /*cacheLengths*/);
            ValidateBringIntoViewOperations(2.0 /*cacheLengths*/);
        }

        private void ValidateBringIntoViewOperations(double cacheLengths)
        {
            Log.Comment("\r\nValidateBringIntoViewOperations cacheLengths: " + cacheLengths);

            string strRealizedItemsIndices = null;
            List<int> realizedItemsIndices = null;
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, 200).ToList()));

            ItemsRepeater itemsRepeater = SetupRepeater(dataSource: null, itemContent: @"<TextBlock Text='{Binding}' Height='24'/>");

            RunOnUIThread.Execute(() =>
            {
                realizedItemsIndices = new List<int>();

                itemsRepeater.ElementPrepared += (sender, args) =>
                {
                    realizedItemsIndices.Add(args.Index);
                };

                itemsRepeater.ElementClearing += (sender, args) =>
                {
                    int index = sender.GetElementIndex(args.Element);
                    realizedItemsIndices.Remove(index);
                };

                itemsRepeater.HorizontalCacheLength = cacheLengths;
                itemsRepeater.VerticalCacheLength = cacheLengths;
                itemsRepeater.ItemsSource = dataSource;
            });

            // The viewport size being 200px and realization window increase being 40px per measure pass,
            // make sure there are 2.5 x cacheLengths passes to land on the final realized items.
            // cacheLengths: number of viewports constituting the pre-fetch buffer, once completely built. 
            //               Half of it is before the current viewport, and half after.
            // viewport: 200px
            // cacheIncrement: 40px (buffer increment per measure pass on each side)
            // Measure passes needed to fill the pre-fetch buffer: viewport / cacheIncrement / 2 x cacheLengths = 2.5 x cacheLengths

            IdleSynchronizer.Wait();
            strRealizedItemsIndices = LogRealizedIndices(realizedItemsIndices, (int)(2.5 * cacheLengths + 1));
            Verify.AreEqual(
                cacheLengths == 0 ? " 0 1 2 3 4 5 6 7 8 9" : " 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17",
                strRealizedItemsIndices);

            StartBringIndexIntoView(itemsRepeater, index: 99);
            strRealizedItemsIndices = LogRealizedIndices(realizedItemsIndices, (int)(2.5 * cacheLengths + 1));
            Verify.AreEqual(
                cacheLengths == 0 ? " 99 100 98 97 96 95 94 93 92 91 90" : " 99 100 101 102 103 104 105 106 107 108 109 98 97 96 95 94 93 92 91 90 89 88 87 86 85 84 83 82",
                strRealizedItemsIndices);

            StartBringIndexIntoView(itemsRepeater, index: 199);
            strRealizedItemsIndices = LogRealizedIndices(realizedItemsIndices, (int)(2.5 * cacheLengths + 1));
            Verify.AreEqual(
                cacheLengths == 0 ? " 199 198 197 196 195 194 193 192 191 190" : " 199 198 197 196 195 194 193 192 191 190 189 188 187 186 185 184 183 182",
                strRealizedItemsIndices);

            StartBringIndexIntoView(itemsRepeater, index: 49);
            strRealizedItemsIndices = LogRealizedIndices(realizedItemsIndices, (int)(2.5 * cacheLengths + 1));
            Verify.AreEqual(
                cacheLengths == 0 ? " 49 50 51 52 53 54 55 56 57 58 48" : " 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 48 47 46 45 44 43 42 41 40 39",
                strRealizedItemsIndices);

            StartBringIndexIntoView(itemsRepeater, index: 0);
            strRealizedItemsIndices = LogRealizedIndices(realizedItemsIndices, (int)(2.5 * cacheLengths + 1));
            Verify.AreEqual(
                cacheLengths == 0 ? " 0 1 2 3 4 5 6 7 8 9" : " 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17",
                strRealizedItemsIndices);
        }

        private void StartBringIndexIntoView(ItemsRepeater itemsRepeater, int index)
        {
            RunOnUIThread.Execute(() =>
            {
                Log.Comment("\r\nItemsRepeater.GetOrCreateElement(" + index + ")");
                var item = itemsRepeater.GetOrCreateElement(index);

                // Make sure the newly created item is measured so that its size is 
                // identified and can be used to bring it entirely into view.
                Log.Comment("UIElement.UpdateLayout()");
                item.UpdateLayout();

                Log.Comment("UIElement.StartBringIntoView()");
                item.StartBringIntoView();
            });

            IdleSynchronizer.Wait();
        }

        private string LogRealizedIndices(List<int> realizedItemsIndices, int ticks)
        {
            string strRealizedItemsIndices = null;

            do
            {
                RunOnUIThread.Execute(() =>
                {
                    strRealizedItemsIndices = string.Empty;

                    foreach (int i in realizedItemsIndices)
                    {
                        strRealizedItemsIndices += " " + i;
                    }

                    Log.Comment("Realized indices:" + strRealizedItemsIndices);
                });

                ticks--;
                CompositionPropertySpy.SynchronouslyTickUIThread(1);
            }
            while (ticks >= 0);

            return strRealizedItemsIndices;
        }

        [TestMethod]
        public void ValidateElementEvents()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList()));

            var repeater = SetupRepeater(dataSource);

            RunOnUIThread.Execute(() =>
            {
                List<int> preparedIndices = new List<int>();
                List<int> clearedIndices = new List<int>();
                List<KeyValuePair<int, int>> changedIndices = new List<KeyValuePair<int, int>>();

                repeater.ElementPrepared += (sender, args) =>
                {
                    preparedIndices.Add(args.Index);
                };

                repeater.ElementClearing += (sender, args) =>
                {
                    clearedIndices.Add(sender.GetElementIndex(args.Element));
                };

                repeater.ElementIndexChanged += (sender, args) =>
                {
                    changedIndices.Add(new KeyValuePair<int, int>(args.OldIndex, args.NewIndex));
                };

                Log.Comment("Insert in realized range: Inserting 1 item at index 1");
                dataSource.Insert(index: 1, count: 1, reset: false);
                repeater.UpdateLayout();

                Verify.AreEqual(1, preparedIndices.Count);
                Verify.AreEqual(1, preparedIndices[0]);
                Verify.AreEqual(2, changedIndices.Count);
                Verify.IsTrue(changedIndices.Contains(new KeyValuePair<int, int>(1, 2)));
                Verify.IsTrue(changedIndices.Contains(new KeyValuePair<int, int>(2, 3)));
                Verify.AreEqual(1, clearedIndices.Count);
                Verify.AreEqual(3, clearedIndices[0]);

                preparedIndices.Clear();
                clearedIndices.Clear();
                changedIndices.Clear();

                Log.Comment("Remove in realized range: Removing 1 item at index 0");
                dataSource.Remove(index: 0, count: 1, reset: false);
                repeater.UpdateLayout();
                Verify.AreEqual(1, clearedIndices.Count);
                Verify.AreEqual(0, clearedIndices[0]);
                Verify.AreEqual(1, preparedIndices.Count);
                Verify.AreEqual(2, preparedIndices[0]);
                Verify.AreEqual(2, changedIndices.Count);
                Verify.IsTrue(changedIndices.Contains(new KeyValuePair<int, int>(1, 0)));
                Verify.IsTrue(changedIndices.Contains(new KeyValuePair<int, int>(2, 1)));
            });
        }

        // Validate new items get generated when existing ones are removed from the top.
        [TestMethod]
        public void ValidateElementCreationUponRemovals()
        {
            CustomItemsSource dataSource = null;
            List<int> preparedIndices = null;

            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, 20).ToList()));

            var itemsRepeater = SetupRepeater(dataSource, @"<TextBlock Text='{Binding}' Height='24'/>");

            RunOnUIThread.Execute(() =>
            {
                itemsRepeater.VerticalCacheLength = 2.0;
            });

            // The viewport size being 200px and realization window increase being 40px per measure pass,
            // make sure there are 2.5 x cacheLengths passes to land on the final realized items.
            // cacheLengths: number of viewports constituting the pre-fetch buffer, once completely built. 
            //               Half of it is before the current viewport, and half after.
            // viewport: 200px
            // cacheIncrement: 40px (buffer increment per measure pass on each side)
            // Measure passes needed to fill the pre-fetch buffer: viewport / cacheIncrement / 2 x cacheLengths = 2.5 x cacheLengths
            Log.Comment("Letting the item cache grow to the VerticalCacheLength value.");
            CompositionPropertySpy.SynchronouslyTickUIThread(6);

            RunOnUIThread.Execute(() =>
            {
                preparedIndices = new List<int>();

                itemsRepeater.ElementPrepared += (sender, args) =>
                {
                    Log.Comment("ElementPrepared - Index={0}.", args.Index);
                    preparedIndices.Add(args.Index);
                };
            });

            for (int iteration = 1; iteration <= 16; iteration++)
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Removing top item at index 0.");
                    dataSource.Remove(index: 0, count: 1, reset: false);
                });

                IdleSynchronizer.Wait();
            }

            RunOnUIThread.Execute(() =>
            {
                // Removal of top items is expected to cause new item generation.
                Log.Comment("Final preparedIndices.Count={0}.", preparedIndices.Count);
                Verify.IsGreaterThan(preparedIndices.Count, 0);
            });
        }

        [TestMethod]
        public void ValidateElementIndexChangedEventOnStableReset()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSourceWithUniqueId(Enumerable.Range(0, 10).ToList()));

            var repeater = SetupRepeater(dataSource);

            RunOnUIThread.Execute(() =>
            {
                List<int> preparedIndices = new List<int>();
                List<int> clearedIndices = new List<int>();
                List<KeyValuePair<int, int>> changedIndices = new List<KeyValuePair<int, int>>();

                repeater.ElementPrepared += (sender, args) =>
                {
                    preparedIndices.Add(args.Index);
                };

                repeater.ElementClearing += (sender, args) =>
                {
                    clearedIndices.Add(sender.GetElementIndex(args.Element));
                };

                repeater.ElementIndexChanged += (sender, args) =>
                {
                    changedIndices.Add(new KeyValuePair<int, int>(args.OldIndex, args.NewIndex));
                };

                Log.Comment("(UniqueId Reset) Insert in realized range: Inserting 1 item at index 1");
                dataSource.Insert(index: 1, count: 1, reset: true, valueStart: 2000);
                repeater.UpdateLayout();

                Verify.AreEqual(1, preparedIndices.Count);
                Verify.AreEqual(1, preparedIndices[0]);
                Verify.AreEqual(1, changedIndices.Count);
                Verify.IsTrue(changedIndices.Contains(new KeyValuePair<int, int>(1, 2)));
                foreach (var ch in changedIndices)
                {
                    Log.Comment("Changed " + ch.Key + " " + ch.Value);
                }
                Verify.AreEqual(1, clearedIndices.Count);
                Verify.AreEqual(2, clearedIndices[0]);

                preparedIndices.Clear();
                clearedIndices.Clear();
                changedIndices.Clear();

                Log.Comment("(UniqueId Reset) Remove in realized range: Removing 1 item at index 0");
                dataSource.Remove(index: 0, count: 1, reset: true);
                repeater.UpdateLayout();
                Verify.AreEqual(1, clearedIndices.Count);
                Verify.AreEqual(0, clearedIndices[0]);
                foreach (var ch in changedIndices)
                {
                    Log.Comment("Changed " + ch.Key + " " + ch.Value);
                }
                Verify.AreEqual(1, preparedIndices.Count);
                Verify.AreEqual(2, preparedIndices[0]);
                Verify.AreEqual(2, changedIndices.Count);
                Verify.IsTrue(changedIndices.Contains(new KeyValuePair<int, int>(1, 0)));
                Verify.IsTrue(changedIndices.Contains(new KeyValuePair<int, int>(2, 1)));
            });
        }

        [TestMethod]
        public void ValidateGetElementAtCachingForLayout()
        {
            List<int> data = Enumerable.Range(0, 15).ToList();
            ItemsSourceView dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new ItemsSourceView(data));
            ScrollViewer scrollViewer = null;
            var repeater = SetupRepeater(dataSource, null /*layout*/, out scrollViewer);
            bool layoutRan = false;

            RunOnUIThread.Execute(() =>
            {
                var layout = new MockVirtualizingLayout();
                layout.MeasureLayoutFunc = (availableSize, context) =>
                {
                    Verify.AreEqual(15, context.ItemCount);
                    var element0 = context.GetOrCreateElementAt(0);
                    Verify.IsNotNull(element0);
                    var element1 = context.GetOrCreateElementAt(1);
                    Verify.IsNotNull(element1);
                    Verify.AreNotSame(element0, element1);

                    var element1again = context.GetOrCreateElementAt(1);
                    Verify.AreSame(element1, element1again);

                    var element10 = context.GetOrCreateElementAt(10);
                    Verify.IsNotNull(element10);
                    Verify.AreNotSame(element0, element10);
                    Verify.AreNotSame(element1, element10);

                    context.RecycleElement(element1);

                    var element0New = context.GetOrCreateElementAt(0);
                    Verify.AreSame(element0, element0New);

                    context.RecycleElement(element10);
                    context.RecycleElement(element0);

                    layoutRan = true;
                    return new Size(10, 10);
                };

                repeater.Layout = layout;
                repeater.UpdateLayout();
                Verify.IsTrue(layoutRan);
            });

            IdleSynchronizer.Wait();
        }

        // Validate that when we clear containers during panning in flow layouts, we always do it 
        // from the end of the range inwards. This allows us to better track first/last realized indices in 
        // ViewManager. Breaking this can cause a performance regression.
        [TestMethod]
        public void ValidateElementClearingOrderFromFlowLayout()
        {
            ItemsSourceView dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new ItemsSourceView(Enumerable.Range(0, 15).ToList()));
            ScrollViewer scrollViewer = null;
            var repeater = SetupRepeater(dataSource, null /*layout*/, out scrollViewer);
            List<int> clearedIndices = new List<int>();
            var viewChangedEvent = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChangedEvent.Set();
                    }
                };

                repeater.Layout = new StackLayout();
                repeater.ElementPrepared += (sender, args) =>
                {
                    ((FrameworkElement)args.Element).Height = 20;
                };
                repeater.UpdateLayout();

                repeater.ElementClearing += (sender, args) =>
                {
                    int index = repeater.GetElementIndex(args.Element);
                    Log.Comment("Clearing.." + index);
                    clearedIndices.Add(index);
                };

                scrollViewer.ChangeView(null, 100.0, null, disableAnimation: true);
            });

            Verify.IsTrue(viewChangedEvent.WaitOne(DefaultWaitTime), "Waiting for ViewChanged.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Validate order is 0, 1, 2, 3
                for (int i = 0; i < 4; i++)
                {
                    Verify.AreEqual(i, clearedIndices[i]);
                }

                clearedIndices.Clear();
                viewChangedEvent.Reset();
                scrollViewer.ChangeView(null, 0.0, null, disableAnimation: true);
            });

            Verify.IsTrue(viewChangedEvent.WaitOne(DefaultWaitTime), "Waiting for ViewChanged.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Validate order is backwards 14, 13, 12, 11
                for (int i = 0; i < 4; i++)
                {
                    Verify.AreEqual(14 - i, clearedIndices[i]);
                }
            });
        }

        [TestMethod]
        public void ValidateDataContextDoesNotGetOverwritten()
        {
            const string c_element1DataContext = "Element1_DataContext";

            RunOnUIThread.Execute(() =>
            {
                var data = new List<Button>()
                {
                    new Button()
                    {
                            Content = "Element1_Content",
                            DataContext = c_element1DataContext
                    }
                };

                var elementFactory = new DataAsElementElementFactory();

                var repeater = new ItemsRepeater() {
                    ItemsSource = data,
                    ItemTemplate = elementFactory
                };

                Content = repeater;

                Content.UpdateLayout();

                // Verify that DataContext is still the same
                var firstElement = repeater.TryGetElement(0) as Button;
                var retrievedDataContextItem1 = firstElement.DataContext as string;
                Verify.IsTrue(retrievedDataContextItem1 == c_element1DataContext);

            });
        }

        [TestMethod]
        public void ValidateDataContextGetsPropagated()
        {
            const string c_element1DataContext = "Element1_DataContext";

            RunOnUIThread.Execute(() =>
            {
                var data = new List<Button>()
                {
                    new Button()
                    {
                            Content = "Element1_Content",
                            DataContext = c_element1DataContext
                    }
                };

                var elementFactory = new ElementFromElementElementFactory();

                var repeater = new ItemsRepeater() 
                {
                    ItemsSource = data,
                    ItemTemplate = elementFactory
                };

                Content = repeater;

                Content.UpdateLayout();

                // Verify that DataContext of data has propagated to the container
                var firstElement = repeater.TryGetElement(0) as Button;
                var retrievedDataContextItem1 = firstElement.DataContext as string;
                Verify.IsTrue(data[0] == firstElement.Content);
                Verify.IsTrue(retrievedDataContextItem1 == c_element1DataContext);

            });
        }

        [TestMethod]// Issue 1018
        [TestProperty("Ignore", "True")]
        public void ValidateFocusMoveOnElementCleared()
        {
            ItemsRepeater repeater = null;
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() =>
            {
                dataSource = new CustomItemsSource(Enumerable.Range(0, 5).ToList());
            });

            repeater = SetupRepeater(dataSource, "<Button Content='{Binding}' Height='10'/>");

            // dataSource: 0 1 2 3 4 
            // Index 0 deleted, focus should be on the new element which has index 0
            SharedHelpers.RunActionsWithWait(
                new Action[]
                {
                    () => { MoveFocusToIndex(repeater, 0); },
                    () => { dataSource.Remove(0 /* index */, 1 /* count */, false /* reset*/); },
                    () => { ValidateCurrentFocus(repeater, 0 /*expectedIndex */, "1" /* expectedContent */); }
                });

            // dataSource: 1 2 3 4 
            // Last element deleted, focus should move to the previous element
            int lastIndex = dataSource.Inner.Count - 1;
            SharedHelpers.RunActionsWithWait(
                new Action[]
                {
                    () => { MoveFocusToIndex(repeater, lastIndex); },
                    () => { dataSource.Remove(lastIndex /* index */, 1 /* count */, false /* reset*/); },
                    () => { ValidateCurrentFocus(repeater, 2 /*expectedIndex */, "3" /* expectedContent */); }
                });

            // dataSource: 1 2 3 
            // Remove multiple elements
            SharedHelpers.RunActionsWithWait(
                new Action[]
                {
                    () => { MoveFocusToIndex(repeater, 0); },
                    () => { dataSource.Remove(0 /* index */, 2 /* count */, false /* reset*/); },
                    () => { ValidateCurrentFocus(repeater, 0 /*expectedIndex */, "3" /* expectedContent */); }
                });
        }

        [TestMethod] //Issue 1018
        [TestProperty("Ignore", "True")]
        public void ValidateFocusMoveOnElementClearedWithUniqueIds()
        {
            ItemsRepeater repeater = null;
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() =>
            {
                dataSource = new CustomItemsSourceWithUniqueId(Enumerable.Range(0, 5).ToList());
            });

            repeater = SetupRepeater(dataSource, "<Button Content='{Binding}' Height='10'/>");

            // dataSource: 0 1 2 3 4 
            // Index 0 deleted, focus should be on the new element which has index 0
            SharedHelpers.RunActionsWithWait(
                new Action[]
                {
                    () => { MoveFocusToIndex(repeater, 0); },
                    () => { dataSource.Remove(0 /* index */, 1 /* count */, true /* reset*/); },
                    () => { ValidateCurrentFocus(repeater, 0 /*expectedIndex */, "1" /* expectedContent */); }
                });

            // dataSource: 1 2 3 4 
            // Last element deleted, focus should move to the previous element
            int lastIndex = dataSource.Inner.Count - 1;
            SharedHelpers.RunActionsWithWait(
                new Action[]
                {
                    () => { MoveFocusToIndex(repeater, lastIndex); },
                    () => { dataSource.Remove(lastIndex /* index */, 1 /* count */, true /* reset*/); },
                    () => { ValidateCurrentFocus(repeater, 2 /*expectedIndex */, "3" /* expectedContent */); }
                });

            // dataSource: 1 2 3 
            // Reset should keep the focused element as long as the unique id matches.
            SharedHelpers.RunActionsWithWait(
                new Action[]
                {
                    () => { MoveFocusToIndex(repeater, 0); },
                    () => { dataSource.Reset(); },
                    () =>
                    {
                        int newIndex = dataSource.Inner.IndexOf(1);
                        ValidateCurrentFocus(repeater, newIndex /*expectedIndex */, "1" /* expectedContent */);
                    }
                });

            // dataSource: 1 2 3 
            // Remove multiple elements
            SharedHelpers.RunActionsWithWait(
                new Action[]
                {
                    () => { MoveFocusToIndex(repeater, 0); },
                    () => { dataSource.Remove(0 /* index */, 2 /* count */, true /* reset*/); },
                    () => { ValidateCurrentFocus(repeater, 0 /*expectedIndex */, "3" /* expectedContent */); }
                });
        }

        [TestMethod]
        // Why does this test work?
        // When the elements get created from the RecyclingElementFactory, we get already "existing" data templates.
        // However, the reason for the crash in #2384 is that those "empty" data templates actually still had their data context
        // If that data context is not null, that means it did not get cleared when the element was recycled, which is the wrong behavior.
        // To check if the clearing is working correctly, we are checking this inside the ElementFactory's RecycleElement function.
        public void ValidateElementClearingClearsDataContext()
        {
            ItemsRepeater repeater = null;
            MockElementFactory elementFactory = null;
            int elementClearingRaisedCount = 0;
            Log.Comment("Initialize ItemsRepeater");
            RunOnUIThread.Execute(() =>
            {
                elementFactory = new MockElementFactory() {
                    GetElementFunc = delegate (int index, UIElement owner) {
                        return new Button() { Content = index };
                           },

                    ClearElementFunc = delegate (UIElement element, UIElement owner) {
                        elementClearingRaisedCount++;
                        Verify.IsNull((element as FrameworkElement).DataContext);
                    }
                };

                repeater = CreateRepeater(Enumerable.Range(0, 100),
                    elementFactory);

                repeater.Layout = new StackLayout();

                Content = repeater;
                repeater.UpdateLayout();

                repeater.ItemsSource = null;

                Log.Comment("Verify ItemsRepeater cleared data contexts correctly");
                Verify.IsTrue(elementClearingRaisedCount > 0, "ItemsRepeater should have cleared some elements");
            });
        }


        private void MoveFocusToIndex(ItemsRepeater repeater, int index)
        {
            var element = repeater.TryGetElement(index) as Control;
            element.Focus(FocusState.Programmatic);
        }

        private void ValidateCurrentFocus(ItemsRepeater repeater, int expectedIndex, string expectedContent)
        {
            var currentFocus = FocusManager.GetFocusedElement(repeater.XamlRoot) as ContentControl;
            var currentFocusedIndex = repeater.GetElementIndex(currentFocus);
            Log.Comment("expectedIndex: " + expectedIndex + " actual : " + currentFocusedIndex);
            Verify.AreEqual(expectedIndex, currentFocusedIndex);
            Log.Comment("expectedContent: " + expectedContent + " actual : " + currentFocus.Content.ToString());
            Verify.AreEqual(expectedContent, currentFocus.Content.ToString());
        }

        private ItemsRepeater CreateRepeater(object dataSource, object elementFactory)
        {
            var repeater = new ItemsRepeater
            {
                ItemsSource = dataSource,
                ItemTemplate = elementFactory
            };
            repeater.Layout = CreateLayout(repeater);
            return repeater;
        }

        private VirtualizingLayout CreateLayout(ItemsRepeater repeater)
        {
            var layout = new MockVirtualizingLayout();
            var children = new List<UIElement>();

            layout.MeasureLayoutFunc = (availableSize, context) =>
            {
                repeater.Tag = repeater.Tag ?? context;
                children.Clear();
                var itemCount = context.ItemCount;

                for (int i = 0; i < itemCount; ++i)
                {
                    var element = context.GetOrCreateElementAt(i);
                    element.Measure(availableSize);
                    children.Add(element);
                }

                return new Size(10, 10);
            };

            return layout;
        }

        private ItemsRepeater SetupRepeater(object dataSource=null, string itemContent= @"<Button Content='{Binding}' Height='100'/>")
        {
            VirtualizingLayout layout = null;
            RunOnUIThread.Execute(() => layout = new StackLayout());
            ScrollViewer scrollViewer = null;
            return SetupRepeater(dataSource, layout, itemContent, out scrollViewer);
        }

        private ItemsRepeater SetupRepeater(object dataSource, VirtualizingLayout layout, out ScrollViewer scrollViewer)
        {
            return SetupRepeater( dataSource, layout, @"<Button Content='{Binding}' Height='100'/>", out scrollViewer);
        }

        private ItemsRepeater SetupRepeater(object dataSource, VirtualizingLayout layout, string itemContent, out ScrollViewer scrollViewer)
        {
            ItemsRepeater itemsRepeater = null;
            ScrollViewer sv = null;
            RunOnUIThread.Execute(() =>
            {
                var elementFactory = new RecyclingElementFactory();
                elementFactory.RecyclePool = new RecyclePool();
                elementFactory.Templates["Item"] = XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" + itemContent + @"</DataTemplate>") as DataTemplate;

                itemsRepeater = new ItemsRepeater()
                {
                    ItemsSource = dataSource,
                    ItemTemplate = elementFactory,
                    Layout = layout,
                    HorizontalCacheLength = 0.0,
                    VerticalCacheLength = 0.0
                };

                sv = new ScrollViewer
                {
                    Content = itemsRepeater
                };

                Content = new ItemsRepeaterScrollHost()
                {
                    Width = 200,
                    Height = 200,
                    ScrollViewer = sv
                };
            });

            IdleSynchronizer.Wait();
            scrollViewer = sv;
            return itemsRepeater;
        }

        // Scenario: focus the first item, scroll far away so it leaves the realized range, then scroll back.
        // Expected: the focused element keeps its index and focus while out of view and the same instance is reused
        //           when it returns.
        // A failure means: keyboard focus could be lost or the focused item re-created when the user scrolls away and
        //                  back.
        [TestMethod]
        [TestProperty("Description", "Verifies that a focused element that scrolls out of the realized range stays pinned (keeps its index and focus) and is reused, not re-prepared, when scrolled back into view.")]
        public void VerifyFocusedElementIsReusedFromPinnedPool()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 100).Select(i => "Item #" + i));
            ScrollViewer scrollViewer = null;
            ItemsRepeater repeater = null;
            Button element0 = null;
            var preparedIndexes = new List<int>();
            var gotFocus = new ManualResetEvent(false);

            StackLayout layout = null;
            RunOnUIThread.Execute(() => layout = new StackLayout());
            repeater = SetupRepeater(data, layout, out scrollViewer);
            RunOnUIThread.Execute(() =>
            {
                element0 = (Button)repeater.TryGetElement(0);
                Verify.IsNotNull(element0);
                element0.GotFocus += delegate { gotFocus.Set(); };
                Verify.IsTrue(element0.Focus(FocusState.Programmatic));
            });
            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), "Waiting for element 0 to get focus.");
            IdleSynchronizer.Wait();

            ScrollTo(scrollViewer, 5000);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(repeater.TryGetElement(50), "Item 50 is realized after scrolling.");
                Verify.AreEqual(0, repeater.GetElementIndex(element0), "The focused element is pinned and keeps its index.");
                Verify.AreSame(element0, repeater.TryGetElement(0), "The pinned element is still considered realized.");
                Verify.AreSame(element0, FocusManager.GetFocusedElement(repeater.XamlRoot), "Focus stays on the pinned element.");
                repeater.ElementPrepared += (sender, args) => preparedIndexes.Add(args.Index);
            });

            ScrollTo(scrollViewer, 0);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Prepared indexes after scrolling back: " + string.Join(",", preparedIndexes));
                Verify.AreSame(element0, repeater.TryGetElement(0), "The pinned element is reused for index 0.");
                Verify.AreEqual("Item #0", element0.Content);
                Verify.IsFalse(preparedIndexes.Contains(0), "Index 0 is served from the pinned pool, not re-prepared by the element factory.");
                Verify.AreSame(element0, FocusManager.GetFocusedElement(repeater.XamlRoot));
            });
        }

        // Scenario: focus item 90, scroll back to the top, then insert an item before 90 but after the realized items.
        // Expected: only the focused element's index changes (90 to 91, with ElementIndexChanged) and it keeps content
        //           and focus.
        // A failure means: a focused item outside the visible range could end up bound to the wrong data after an
        //                  insert.
        [TestMethod]
        [TestProperty("Description", "Verifies that inserting items after the realized range updates the index of a focused element held in the pinned pool and raises ElementIndexChanged for it.")]
        public void VerifyPinnedElementIndexUpdatesOnInsertOutsideRealizedRange()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 100).Select(i => "Item #" + i));
            ScrollViewer scrollViewer = null;
            ItemsRepeater repeater = null;
            Button element90 = null;
            var indexChanges = new List<Tuple<UIElement, int, int>>();
            var gotFocus = new ManualResetEvent(false);

            StackLayout layout = null;
            RunOnUIThread.Execute(() => layout = new StackLayout());
            repeater = SetupRepeater(data, layout, out scrollViewer);
            ScrollTo(scrollViewer, 9000);

            RunOnUIThread.Execute(() =>
            {
                element90 = (Button)repeater.TryGetElement(90);
                Verify.IsNotNull(element90);
                element90.GotFocus += delegate { gotFocus.Set(); };
                Verify.IsTrue(element90.Focus(FocusState.Programmatic));
            });
            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), "Waiting for element 90 to get focus.");
            IdleSynchronizer.Wait();

            ScrollTo(scrollViewer, 0);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(repeater.TryGetElement(0));
                Verify.AreEqual(90, repeater.GetElementIndex(element90), "The focused element is pinned outside the realized range.");

                repeater.ElementIndexChanged += (sender, args) => indexChanges.Add(Tuple.Create(args.Element, args.OldIndex, args.NewIndex));
                data.Insert(50, "Inserted");

                Log.Comment("Index changes: " + string.Join(",", indexChanges.Select(c => $"{c.Item2}->{c.Item3}")));
                Verify.AreEqual(1, indexChanges.Count, "Only the pinned element is affected by an insert after the realized range.");
                Verify.AreSame(element90, indexChanges[0].Item1);
                Verify.AreEqual(90, indexChanges[0].Item2);
                Verify.AreEqual(91, indexChanges[0].Item3);
                Verify.AreEqual(91, repeater.GetElementIndex(element90));
            });

            ScrollTo(scrollViewer, 9100);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(element90, repeater.TryGetElement(91), "The pinned element is reused at its updated index.");
                Verify.AreEqual("Item #90", element90.Content);
                Verify.AreSame(element90, FocusManager.GetFocusedElement(repeater.XamlRoot));
            });
        }

        // Scenario: focus a Button item and remove it from the source (also for the first and last items).
        // Expected: focus moves to the next realized item, or to the previous one when the last item was removed.
        // Ignored: reproduces focus jumping to the first item when the focused item is removed (PC-FOCUS-ON-REMOVE);
        //          currently fails because focus lands on item 0 instead of the adjacent item.
        [TestMethod]
        [TestProperty("Ignore", "True")] // Product concern PC-FOCUS-ON-REMOVE, same behavior as Issue 1018: after removing the focused 'Item #2', expected focus on the adjacent 'Item #3' (new index 2); actual focus is on 'Item #0' (index 0).
        [TestProperty("Description", "Verifies that removing the focused item moves focus to the next realized item, or to the previous one when the last item is removed.")]
        public void VerifyFocusMovesToAdjacentItemWhenFocusedItemRemoved()
        {
            VerifyFocusMoveOnRemoval(itemTemplate: "<Button Content='{Binding}' Height='30'/>");
        }

        // Scenario: same as the previous test with non-Control item containers whose child Button has focus.
        // Expected: focus moves to the Button inside the adjacent item.
        // Ignored: reproduces focus jumping to the first item when the focused item is removed (PC-FOCUS-ON-REMOVE);
        //          currently fails because focus lands on the first item's Button.
        [TestMethod]
        [TestProperty("Ignore", "True")] // Product concern PC-FOCUS-ON-REMOVE, same behavior as Issue 1018: after removing the item whose Button 'Item #2' is focused, expected focus on Button 'Item #3' (new index 2); actual focus is on Button 'Item #0' (index 0).
        [TestProperty("Description", "Same as VerifyFocusMovesToAdjacentItemWhenFocusedItemRemoved when item containers are not Controls: focus moves to a focusable descendant of the adjacent item.")]
        public void VerifyFocusMovesToAdjacentItemDescendantWhenFocusedItemRemoved()
        {
            VerifyFocusMoveOnRemoval(itemTemplate: "<StackPanel Height='30'><Button Content='{Binding}'/></StackPanel>");
        }

        private void VerifyFocusMoveOnRemoval(string itemTemplate)
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            ItemsRepeater repeater = null;

            RunOnUIThread.Execute(() =>
            {
                repeater = new ItemsRepeater() {
                    ItemsSource = data,
                    ItemTemplate = XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" + itemTemplate + @"</DataTemplate>"),
                    VerticalAlignment = VerticalAlignment.Top
                };
                Content = repeater;
                Content.UpdateLayout();
            });

            // Remove a middle item: the item that follows it takes its index and receives focus.
            FocusItem(repeater, 2);
            RemoveAndLogFocus(repeater, data, 2);
            IdleSynchronizer.Wait();
            VerifyFocusedItem(repeater, expectedIndex: 2, expectedContent: "Item #3");

            // Remove the first item: the next item receives focus.
            FocusItem(repeater, 0);
            RemoveAndLogFocus(repeater, data, 0);
            IdleSynchronizer.Wait();
            VerifyFocusedItem(repeater, expectedIndex: 0, expectedContent: "Item #1");

            // data: 1 3 4. Remove the last item: there is no next item, so the previous item receives focus.
            FocusItem(repeater, 2);
            RemoveAndLogFocus(repeater, data, 2);
            IdleSynchronizer.Wait();
            VerifyFocusedItem(repeater, expectedIndex: 1, expectedContent: "Item #3");
        }

        // Removes an item and logs the focused element synchronously after the removal and after the UI thread idles,
        // to distinguish the ItemsRepeater focus move from later framework focus changes.
        private void RemoveAndLogFocus(ItemsRepeater repeater, ObservableCollection<string> data, int index)
        {
            RunOnUIThread.Execute(() =>
            {
                data.RemoveAt(index);
                var focused = FocusManager.GetFocusedElement(repeater.XamlRoot);
                Log.Comment($"Focused synchronously after RemoveAt({index}): {focused?.GetType().Name} '{(focused as ContentControl)?.Content}' FocusState={(focused as Control)?.FocusState}");
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(repeater.XamlRoot);
                Log.Comment($"Focused after idle: {focused?.GetType().Name} '{(focused as ContentControl)?.Content}' FocusState={(focused as Control)?.FocusState}");
            });
        }

        private void FocusItem(ItemsRepeater repeater, int index)
        {
            var gotFocus = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                var target = GetFocusableControl(repeater.TryGetElement(index));
                if (FocusManager.GetFocusedElement(repeater.XamlRoot) == target)
                {
                    gotFocus.Set();
                }
                else
                {
                    target.GotFocus += delegate { gotFocus.Set(); };
                    Verify.IsTrue(target.Focus(FocusState.Programmatic));
                }
            });
            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), $"Waiting for item {index} to get focus.");
            IdleSynchronizer.Wait();
        }

        private void VerifyFocusedItem(ItemsRepeater repeater, int expectedIndex, string expectedContent)
        {
            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(repeater.XamlRoot) as DependencyObject;
                Log.Comment($"Focused element: {focused?.GetType().Name} '{(focused as ContentControl)?.Content}'");
                Verify.IsTrue(focused is Button, "A Button inside the repeater is expected to be focused.");

                // Walk up to the repeater's direct child to find the item index.
                UIElement container = focused as UIElement;
                while (container != null && VisualTreeHelper.GetParent(container) != repeater)
                {
                    container = VisualTreeHelper.GetParent(container) as UIElement;
                }

                Verify.IsNotNull(container, "The focused element is inside the repeater.");
                Verify.AreEqual(expectedIndex, repeater.GetElementIndex(container));
                Verify.AreEqual(expectedContent, ((Button)focused).Content);
            });
        }

        private static Control GetFocusableControl(UIElement element)
        {
            if (element is Control control)
            {
                return control;
            }
            return (Control)FocusManager.FindFirstFocusableElement(element);
        }

        private void ScrollTo(ScrollViewer scrollViewer, double verticalOffset)
        {
            var viewChanged = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                if (scrollViewer.VerticalOffset == verticalOffset)
                {
                    viewChanged.Set();
                    return;
                }

                scrollViewer.ViewChanged += OnViewChanged;
                Verify.IsTrue(scrollViewer.ChangeView(null, verticalOffset, null, disableAnimation: true));

                void OnViewChanged(object sender, ScrollViewerViewChangedEventArgs args)
                {
                    if (!args.IsIntermediate)
                    {
                        scrollViewer.ViewChanged -= OnViewChanged;
                        viewChanged.Set();
                    }
                }
            });
            Verify.IsTrue(viewChanged.WaitOne(DefaultWaitTimeInMS), $"Waiting for ViewChanged to offset {verticalOffset}.");
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.AreEqual(verticalOffset, scrollViewer.VerticalOffset));
        }

        // Scenario: raise Replace notifications with an empty old-items or new-items list from a custom data source.
        // Expected: each notification is rejected with E_FAIL and the realized elements are left untouched.
        // A failure means: malformed data source notifications could corrupt the repeater's element bookkeeping.
        [TestMethod]
        [TestProperty("Description", "Verifies that Replace notifications with an empty old or new item list are rejected with E_FAIL and leave realized elements untouched.")]
        public void VerifyInvalidReplaceNotificationsThrow()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new RawNotificationItemsSource(Enumerable.Range(0, 10).ToList());
                var repeater = new ItemsRepeater() {
                    ItemsSource = dataSource,
                    ItemTemplate = XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='{Binding}' Height='20'/></DataTemplate>"),
                    VerticalAlignment = VerticalAlignment.Top
                };
                Content = repeater;
                Content.UpdateLayout();

                var elements = Enumerable.Range(0, 10).Select(i => repeater.TryGetElement(i)).ToList();
                int notifications = 0;
                repeater.ElementIndexChanged += delegate { notifications++; };
                repeater.ElementClearing += delegate { notifications++; };

                Log.Comment("Replace with OldItems.Count == 0.");
                Verify.AreEqual(E_FAIL, CaptureHResult(() => dataSource.RaiseReplace(index: 2, oldCount: 0, newCount: 1)));

                Log.Comment("Replace with NewItems.Count == 0.");
                Verify.AreEqual(E_FAIL, CaptureHResult(() => dataSource.RaiseReplace(index: 2, oldCount: 1, newCount: 0)));

                Verify.AreEqual(0, notifications, "Rejected notifications do not touch realized elements.");
                for (int i = 0; i < 10; i++)
                {
                    Verify.AreSame(elements[i], repeater.TryGetElement(i));
                    Verify.AreEqual(i, repeater.GetElementIndex(elements[i]));
                }

                Content = null;
            });
        }

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

        // Raises collection change notifications without modifying the underlying data.
        private class RawNotificationItemsSource : CustomItemsSource
        {
            public RawNotificationItemsSource(List<int> source) : base(source) { }

            public void RaiseReplace(int index, int oldCount, int newCount)
            {
                OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(
                    global::System.Collections.Specialized.NotifyCollectionChangedAction.Replace,
                    oldStartingIndex: index,
                    oldItemsCount: oldCount,
                    newStartingIndex: index,
                    newItemsCount: newCount));
            }

            // Moves one item and raises a Move notification whose OldItems and NewItems are null,
            // as a custom INotifyCollectionChanged implementation is allowed to do.
            public void MoveWithoutItems(int oldIndex, int newIndex)
            {
                var item = Inner[oldIndex];
                Inner.RemoveAt(oldIndex);
                Inner.Insert(newIndex, item);
                OnItemsSourceChanged(new global::System.Collections.Specialized.NotifyCollectionChangedEventArgs(
                    global::System.Collections.Specialized.NotifyCollectionChangedAction.Move,
                    (global::System.Collections.IList)null,
                    newIndex,
                    oldIndex));
            }
        }

        #region Move (GitHub issue #11570)

        private const string MoveItemTemplate = @"<Button Content='{Binding}' Height='30'/>";

        // Scenario: a fully realized StackLayout repeater (no ScrollViewer) over an ObservableCollection; Move one item
        //           forward, backward, to an adjacent index, and between the first and last positions (GitHub issue #11570).
        // Expected: right after the Move every realized element reports the post-move index of the item it shows, and after
        //           layout TryGetElement(i) returns the element showing item i for every i (no null, no duplicate index).
        // A failure means: TryGetElement/GetElementIndex return null or the wrong element after ObservableCollection.Move.
        [TestMethod]
        [TestProperty("Description", "Verifies that TryGetElement and GetElementIndex follow the data after ObservableCollection.Move in a StackLayout repeater, both right after the Move and after layout.")]
        public void VerifyRealizedIndicesAfterMove()
        {
            var cases = new[]
            {
                new { Count = 2, OldIndex = 0, NewIndex = 1 }, // The reported scenario.
                new { Count = 2, OldIndex = 1, NewIndex = 0 },
                new { Count = 5, OldIndex = 0, NewIndex = 3 }, // Forward: the items in between shift down.
                new { Count = 5, OldIndex = 4, NewIndex = 1 }, // Backward: the items in between shift up.
                new { Count = 5, OldIndex = 2, NewIndex = 3 }, // Adjacent.
                new { Count = 5, OldIndex = 3, NewIndex = 2 }, // Adjacent, backward.
                new { Count = 5, OldIndex = 0, NewIndex = 4 }, // First to last.
                new { Count = 5, OldIndex = 4, NewIndex = 0 }, // Last to first.
                new { Count = 10, OldIndex = 2, NewIndex = 7 },
            };

            foreach (var c in cases)
            {
                Log.Comment($"Count={c.Count}: Move({c.OldIndex}, {c.NewIndex})");
                var data = new ObservableCollection<string>(Enumerable.Range(0, c.Count).Select(i => "Item #" + i));
                var repeater = CreateLoadedRepeater(data);

                RunOnUIThread.Execute(() =>
                {
                    VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "Before Move");

                    data.Move(c.OldIndex, c.NewIndex);
                    VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "Right after Move");

                    repeater.UpdateLayout();
                    VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "After layout");
                });
            }
        }

        // Scenario: a virtualizing repeater (50 items of 30px in a 200px ScrollViewer); Move a realized item out of the
        //           realized range and an unrealized item into it; then scroll down and Move an item from before the viewport
        //           to after it, which shifts every realized element.
        // Expected: after each Move and layout every realized element reports the post-move index of the item it shows,
        //           and the element that was first in view keeps showing its item at its post-move index and stays first
        //           in view (scroll anchoring adjusts the offset).
        // A failure means: index lookups or scroll anchoring use stale indices after a Move in a virtualized list.
        [TestMethod]
        [TestProperty("Description", "Verifies realized indices after moving items into, out of and across the realized range of a virtualizing repeater.")]
        public void VerifyRealizedIndicesAfterMoveInScrolledRepeater()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 50).Select(i => "Item #" + i));
            ScrollViewer scrollViewer = null;
            StackLayout layout = null;
            RunOnUIThread.Execute(() => layout = new StackLayout());
            var repeater = SetupRepeater(data, layout, MoveItemTemplate, out scrollViewer);

            RunOnUIThread.Execute(() =>
            {
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "Initial");
                Verify.IsNotNull(repeater.TryGetElement(1));
                Verify.IsNull(repeater.TryGetElement(30));

                Log.Comment("Move(1, 30): realized to unrealized.");
                data.Move(1, 30);
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "Right after Move(1, 30)");
                repeater.UpdateLayout();
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "After layout, Move(1, 30)");
                Verify.AreEqual("Item #2", GetElementContent(repeater.TryGetElement(1)));

                Log.Comment("Move(30, 1): unrealized to realized.");
                data.Move(30, 1);
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "Right after Move(30, 1)");
                repeater.UpdateLayout();
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "After layout, Move(30, 1)");
                Verify.AreEqual("Item #1", GetElementContent(repeater.TryGetElement(1)));
            });

            ScrollTo(scrollViewer, 300);

            UIElement firstInView = null;
            RunOnUIThread.Execute(() =>
            {
                firstInView = repeater.TryGetElement(10);
                Verify.IsNotNull(firstInView);
                Verify.AreEqual("Item #10", GetElementContent(firstInView));
                Log.Comment($"VerticalOffset before Move(1, 30): {scrollViewer.VerticalOffset}");

                Log.Comment("Move(1, 30): the index of every realized element decreases by one.");
                data.Move(1, 30);
                Verify.AreEqual(9, repeater.GetElementIndex(firstInView), "Right after Move: the first element in view reports its post-move index.");
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "Right after Move(1, 30), scrolled");
                repeater.UpdateLayout();
                Log.Comment($"VerticalOffset after Move(1, 30) and layout: {scrollViewer.VerticalOffset}");
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "After layout, Move(1, 30), scrolled");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"VerticalOffset after idle: {scrollViewer.VerticalOffset}, first element in view at {firstInView.ActualOffset.Y}");
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "After idle, scrolled");
                Verify.IsGreaterThan(CountRealizedChildren(repeater), 0);
                Verify.AreSame(firstInView, repeater.TryGetElement(9), "The element that was first in view is realized at its post-move index.");
                Verify.AreEqual("Item #10", GetElementContent(firstInView));
                Verify.IsLessThan(Math.Abs(firstInView.ActualOffset.Y - scrollViewer.VerticalOffset), 1.0, "Scroll anchoring keeps the same element first in view (no viewport jump).");
            });
        }

        // Scenario: a fully realized repeater; two Moves in the same tick that cancel out; Move, layout, Move, layout; and
        //           a Move followed by an Insert and a RemoveAt in the same tick.
        // Expected: the realized elements match the data after every operation and after every layout pass.
        // A failure means: a later collection change shifts indices from a stale post-Move state.
        [TestMethod]
        [TestProperty("Description", "Verifies realized indices for operation sequences that include Move: Move+Move in one tick, Move/layout/Move/layout, and Move followed by Insert and RemoveAt.")]
        public void VerifyRealizedIndicesAfterMoveSequences()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => "Item #" + i));
            var repeater = CreateLoadedRepeater(data);

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Move(0, 2) then Move(2, 0) in the same tick.");
                data.Move(0, 2);
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "After Move(0, 2)");
                data.Move(2, 0);
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "After Move(2, 0)");
                repeater.UpdateLayout();
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "After layout");
            });

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Move(0, 1), layout, Move(1, 2), layout.");
                data.Move(0, 1);
                repeater.UpdateLayout();
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "After Move(0, 1) and layout");
                data.Move(1, 2);
                repeater.UpdateLayout();
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "After Move(1, 2) and layout");
            });

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Move(2, 0), Insert(1), RemoveAt(3) in the same tick.");
                data.Move(2, 0);
                data.Insert(1, "Inserted");
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "After Move and Insert");
                data.RemoveAt(3);
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "After Move, Insert and RemoveAt");
                repeater.UpdateLayout();
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "After layout");
            });
        }

        // Scenario: Move(0, 3) in a fully realized 5-item repeater with an ElementIndexChanged handler that calls
        //           TryGetElement, GetElementIndex and GetOrCreateElement.
        // Expected: ElementIndexChanged is raised once for each realized element whose index changed (0->3, 1->0, 2->1,
        //           3->2), and inside every handler the repeater already reports the complete post-move mapping:
        //           TryGetElement(NewIndex) and GetOrCreateElement(NewIndex) return args.Element and no child is added.
        // A failure means: handlers see a half-updated mapping (for example two elements with the same index) or the
        //                  repeater does not tell listeners that indices changed.
        [TestMethod]
        [TestProperty("Description", "Verifies ElementIndexChanged is raised for every element whose index a Move changes, after all indices are updated, so handlers can call TryGetElement/GetOrCreateElement safely.")]
        public void VerifyElementIndexChangedOnMove()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var repeater = CreateLoadedRepeater(data);

            RunOnUIThread.Execute(() =>
            {
                var elements = Enumerable.Range(0, 5).Select(i => repeater.TryGetElement(i)).ToList();
                int childCount = VisualTreeHelper.GetChildrenCount(repeater);
                var changes = new List<Tuple<UIElement, int, int>>();

                TypedEventHandler<ItemsRepeater, ItemsRepeaterElementIndexChangedEventArgs> handler = (sender, args) =>
                {
                    Log.Comment($"ElementIndexChanged {args.OldIndex} -> {args.NewIndex} '{GetElementContent(args.Element)}'");
                    changes.Add(Tuple.Create(args.Element, args.OldIndex, args.NewIndex));
                    Verify.AreEqual(args.NewIndex, repeater.GetElementIndex(args.Element), "GetElementIndex(args.Element) inside the handler.");
                    Verify.AreSame(args.Element, repeater.TryGetElement(args.NewIndex), "TryGetElement(args.NewIndex) inside the handler.");
                    Verify.AreEqual(data[args.NewIndex], GetElementContent(args.Element));
                    Verify.AreSame(args.Element, repeater.GetOrCreateElement(args.NewIndex), "GetOrCreateElement(args.NewIndex) inside the handler.");
                    // Every item is realized, so already in the first handler each index must map to the element that
                    // shows the item at that index (no index held by two elements, none missing).
                    VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: $"Inside the handler for {args.OldIndex} -> {args.NewIndex}");
                };

                repeater.ElementIndexChanged += handler;
                data.Move(0, 3);
                repeater.ElementIndexChanged -= handler;

                Verify.AreEqual(childCount, VisualTreeHelper.GetChildrenCount(repeater), "The handlers did not add children.");
                var expected = new[]
                {
                    Tuple.Create(elements[0], 0, 3),
                    Tuple.Create(elements[1], 1, 0),
                    Tuple.Create(elements[2], 2, 1),
                    Tuple.Create(elements[3], 3, 2),
                };
                Verify.AreEqual(expected.Length, changes.Count, "One ElementIndexChanged per realized element whose index changed.");
                foreach (var e in expected)
                {
                    Verify.IsTrue(changes.Any(c => c.Item1 == e.Item1 && c.Item2 == e.Item2 && c.Item3 == e.Item3),
                        $"ElementIndexChanged {e.Item2} -> {e.Item3} for '{GetElementContent(e.Item1)}'.");
                }

                repeater.UpdateLayout();
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "After layout");
            });
        }

        // Scenario: an app CollectionChanged handler, subscribed after the repeater, reads TryGetElement for every index
        //           when Move(0, 3) is raised.
        // Expected: every element it gets shows the item at that index; the elements of the items that shifted are found.
        // A failure means: apps that react to CollectionChanged get the wrong elements after a Move.
        [TestMethod]
        [TestProperty("Description", "Verifies that a CollectionChanged handler running after the repeater sees post-move elements from TryGetElement.")]
        public void VerifyTryGetElementFromCollectionChangedHandlerAfterMove()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var repeater = CreateLoadedRepeater(data);

            RunOnUIThread.Execute(() =>
            {
                bool handlerRan = false;
                data.CollectionChanged += (sender, args) =>
                {
                    handlerRan = true;
                    Verify.AreEqual(global::System.Collections.Specialized.NotifyCollectionChangedAction.Move, args.Action);
                    VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "Inside CollectionChanged");
                    for (int i = 0; i <= 2; i++)
                    {
                        Verify.IsNotNull(repeater.TryGetElement(i), $"The element of shifted item '{data[i]}' is found at index {i}.");
                    }
                };

                data.Move(0, 3);
                Verify.IsTrue(handlerRan);
            });
        }

        // Scenario: the focused (and therefore pinned) element is the item that moves: Move(0, 3) in a 5-item repeater,
        //           then layout (Move variant of GitHub issue #12659).
        // Expected: the same element instance is reused at index 3, still shows its item and keeps focus; no element is
        //           left behind at index 0 with the moved item.
        // A failure means: a focused item that is moved leaves a stale pinned element at its old index.
        [TestMethod]
        [TestProperty("Description", "Verifies that a focused element that is moved keeps focus and is reused at its new index.")]
        public void VerifyFocusedElementIsReusedAtNewIndexAfterMove()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var repeater = CreateLoadedRepeater(data);
            FocusItem(repeater, 0);

            RunOnUIThread.Execute(() =>
            {
                var focused = repeater.TryGetElement(0);
                Verify.AreSame(focused, FocusManager.GetFocusedElement(repeater.XamlRoot));

                data.Move(0, 3);
                Verify.AreEqual(3, repeater.GetElementIndex(focused), "Right after Move the focused element reports its new index.");
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "Right after Move");

                repeater.UpdateLayout();
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "After layout");
                Verify.AreSame(focused, repeater.TryGetElement(3), "The focused element is reused at its new index.");
                Verify.AreEqual("Item #0", GetElementContent(focused));
                Verify.AreSame(focused, FocusManager.GetFocusedElement(repeater.XamlRoot), "Focus stays on the moved element.");
            });
        }

        // Scenario: the focused (pinned) element is between the old and new positions of a moved item: focus 'Item #2',
        //           Move(0, 4), layout, then ask for the next (Tab) focus candidate inside the repeater.
        // Expected: the focused element is re-indexed once (2 -> 1), keeps focus, and the next focus candidate is the
        //           element at index 2 ('Item #3').
        // A failure means: a pinned element is re-indexed twice or not at all, or focus navigation uses stale indices.
        [TestMethod]
        [TestProperty("Description", "Verifies that a focused element in the range shifted by a Move is re-indexed once, keeps focus, and that focus navigation follows the post-move order.")]
        public void VerifyFocusedElementInShiftedRangeAfterMove()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var repeater = CreateLoadedRepeater(data);
            RunOnUIThread.Execute(() => repeater.TabFocusNavigation = KeyboardNavigationMode.Local);
            FocusItem(repeater, 2);

            RunOnUIThread.Execute(() =>
            {
                var focused = repeater.TryGetElement(2);
                var changes = new List<Tuple<int, int>>();
                repeater.ElementIndexChanged += (sender, args) =>
                {
                    if (args.Element == focused)
                    {
                        changes.Add(Tuple.Create(args.OldIndex, args.NewIndex));
                    }
                };

                data.Move(0, 4);
                repeater.UpdateLayout();

                Verify.AreEqual(1, changes.Count, "The focused element is re-indexed exactly once.");
                Verify.AreEqual(2, changes[0].Item1);
                Verify.AreEqual(1, changes[0].Item2);
                Verify.AreEqual(1, repeater.GetElementIndex(focused));
                Verify.AreSame(focused, repeater.TryGetElement(1));
                Verify.AreSame(focused, FocusManager.GetFocusedElement(repeater.XamlRoot), "Focus stays on the element.");
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "After layout");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(repeater.XamlRoot) as UIElement;
                Log.Comment($"Focused after idle: '{GetElementContent(focused)}'");
                Verify.AreSame(repeater.TryGetElement(1), focused, "Focus is still on the element at index 1 after idle.");

                var next = FocusManager.FindNextElement(FocusNavigationDirection.Next, new FindNextElementOptions() { SearchRoot = repeater.XamlRoot.Content }) as UIElement;
                Log.Comment($"Next focus candidate: '{GetElementContent(next)}'");
                Verify.AreSame(repeater.TryGetElement(2), next, "The next focus candidate is the element at the next index.");
                Verify.AreEqual("Item #3", GetElementContent(next));
            });
        }

        // Scenario: the items are UIElements (Buttons, no ItemTemplate); Move(0, 2) in a fully realized 4-item repeater.
        // Expected: for every index, TryGetElement(i) is data[i] itself and GetElementIndex(data[i]) is i.
        // A failure means: repeaters that host UIElement items report stale indices after a Move.
        [TestMethod]
        [TestProperty("Description", "Verifies TryGetElement/GetElementIndex after a Move when the items are UIElements.")]
        public void VerifyUIElementItemsAfterMove()
        {
            ObservableCollection<UIElement> data = null;
            ItemsRepeater repeater = null;
            RunOnUIThread.Execute(() =>
            {
                data = new ObservableCollection<UIElement>(Enumerable.Range(0, 4).Select(i => new Button() { Content = "Item #" + i, Height = 30 }));
                repeater = new ItemsRepeater() { ItemsSource = data, VerticalAlignment = VerticalAlignment.Top };
                Content = repeater;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                data.Move(0, 2);
                repeater.UpdateLayout();
                for (int i = 0; i < data.Count; i++)
                {
                    Log.Comment($"Index {i}: data '{GetElementContent(data[i])}', TryGetElement '{GetElementContent(repeater.TryGetElement(i))}', GetElementIndex(data[i]) {repeater.GetElementIndex(data[i])}");
                }
                for (int i = 0; i < data.Count; i++)
                {
                    Verify.AreSame(data[i], repeater.TryGetElement(i), $"TryGetElement({i}) is the item at index {i}.");
                    Verify.AreEqual(i, repeater.GetElementIndex(data[i]));
                }
            });
        }

        // Scenario: the items are UIElements (Buttons, no ItemTemplate) in a fully realized 4-item repeater, with the default
        //           StackLayout and with a NonVirtualizingLayout; RemoveAt(0), then RemoveAt(1). Without an ItemTemplate, clearing
        //           a removed element also removes it from Children (with the NonVirtualizingLayout the repeater clears it during
        //           the Remove; StackLayout clears its own elements afterwards).
        // Expected: right after each Remove and after layout, TryGetElement(i) is data[i] and GetElementIndex(data[i]) is i,
        //           and the removed item is no longer a realized child.
        // A failure means: the Remove skipped the child that followed the removed one in Children (Children changed while it
        //                  was walked), so that child kept its old index and TryGetElement returns null or the wrong item.
        [TestMethod]
        [TestProperty("Description", "Verifies TryGetElement/GetElementIndex after a Remove when the items are UIElements.")]
        public void VerifyUIElementItemsAfterRemove()
        {
            foreach (bool nonVirtualizing in new[] { false, true })
            {
                Log.Comment(nonVirtualizing ? "NonVirtualizingLayout" : "StackLayout");
                ObservableCollection<UIElement> data = null;
                ItemsRepeater repeater = null;
                RunOnUIThread.Execute(() =>
                {
                    data = new ObservableCollection<UIElement>(Enumerable.Range(0, 4).Select(i => new Button() { Content = "Item #" + i, Height = 30 }));
                    repeater = new ItemsRepeater() { ItemsSource = data, VerticalAlignment = VerticalAlignment.Top };
                    if (nonVirtualizing)
                    {
                        repeater.Layout = new NonVirtualStackLayout();
                    }
                    Content = repeater;
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    foreach (int index in new[] { 0, 1 })
                    {
                        var removed = data[index];
                        string operation = $"RemoveAt({index}) of '{GetElementContent(removed)}'";
                        data.RemoveAt(index);
                        VerifyUIElementItemsMatchData(repeater, data, removed, $"Right after {operation}");
                        repeater.UpdateLayout();
                        VerifyUIElementItemsMatchData(repeater, data, removed, $"After {operation} and layout");
                    }
                });
            }
        }

        private static void VerifyUIElementItemsMatchData(ItemsRepeater repeater, IList<UIElement> data, UIElement removed, string context)
        {
            for (int i = 0; i < data.Count; i++)
            {
                    Log.Comment($"{context}: index {i}: data '{GetElementContent(data[i])}', TryGetElement '{GetElementContent(repeater.TryGetElement(i))}', GetElementIndex(data[i]) {repeater.GetElementIndex(data[i])}");
            }
            for (int i = 0; i < data.Count; i++)
            {
                    Verify.AreSame(data[i], repeater.TryGetElement(i), $"{context}: TryGetElement({i}) is the item at index {i}.");
                    Verify.AreEqual(i, repeater.GetElementIndex(data[i]), $"{context}: GetElementIndex of the item at index {i}.");
            }
            Verify.AreEqual(-1, repeater.GetElementIndex(removed), $"{context}: the removed item is not a realized child.");
        }

        // Scenario: a keyed data source (IKeyIndexMapping) raises a Reset (stable reset pending, elements in the unique id
        //           reset pool) and then a Move before the next layout pass.
        // Expected: after layout every realized element shows the item at its index.
        // A failure means: the Move remap disturbs elements waiting in the unique id reset pool.
        [TestMethod]
        [TestProperty("Description", "Verifies that a Move raised while a stable (keyed) Reset is pending leaves the repeater consistent after layout.")]
        public void VerifyMoveDuringPendingStableReset()
        {
            CustomItemsSourceWithUniqueId dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSourceWithUniqueId(Enumerable.Range(0, 10).ToList()));
            var repeater = CreateLoadedRepeater(dataSource);

            RunOnUIThread.Execute(() =>
            {
                VerifyRealizedElementsMatchData(repeater, dataSource.Inner.Select(i => i.ToString()).ToList(), expectAllRealized: true, context: "Initial");
                dataSource.Reset();
                dataSource.Move(oldIndex: 0, newIndex: 4, count: 1, reset: false);
                repeater.UpdateLayout();
                VerifyRealizedElementsMatchData(repeater, dataSource.Inner.Select(i => i.ToString()).ToList(), expectAllRealized: true, context: "After Reset, Move and layout");
            });
        }

        // Scenario: a NonVirtualizingLayout (no ElementManager) shows 5 items; Move(0, 3), then layout.
        // Expected: the layout gets the element of item i for index i, so the visual order and the index lookups both
        //           follow the data.
        // A failure means: non-virtualizing layouts show pre-move content at the old positions after a Move.
        [TestMethod]
        [TestProperty("Description", "Verifies that a NonVirtualizingLayout shows and reports post-move items after a Move.")]
        public void VerifyNonVirtualizingLayoutAfterMove()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var repeater = CreateLoadedRepeater(data, () => new NonVirtualStackLayout());

            RunOnUIThread.Execute(() =>
            {
                data.Move(0, 3);
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "Right after Move");
                repeater.UpdateLayout();
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "After layout");
                VerifyVisualOrderMatchesData(repeater, data);
            });
        }

        // Scenario: a custom VirtualizingLayout that does not override OnItemsChangedCore (so nothing but ItemsRepeater
        //           tracks indices) realizes every index with GetOrCreateElementAt; Move(4, 1), then layout.
        // Expected: GetOrCreateElementAt(i) returns the element of item i, and the index lookups follow the data.
        // A failure means: custom virtualizing layouts get pre-move elements for an index after a Move.
        [TestMethod]
        [TestProperty("Description", "Verifies that a custom VirtualizingLayout on the default OnItemsChangedCore gets post-move elements after a Move.")]
        public void VerifyCustomVirtualizingLayoutAfterMove()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var elementsFromLayout = new List<UIElement>();
            Func<Layout> createLayout = () => new MockVirtualizingLayout()
            {
                MeasureLayoutFunc = (availableSize, context) =>
                {
                    elementsFromLayout.Clear();
                    double height = 0;
                    for (int i = 0; i < context.ItemCount; i++)
                    {
                        var element = context.GetOrCreateElementAt(i);
                        element.Measure(availableSize);
                        elementsFromLayout.Add(element);
                        height += element.DesiredSize.Height;
                    }
                    return new Size(100, height);
                },
                ArrangeLayoutFunc = (finalSize, context) =>
                {
                    double offset = 0;
                    foreach (var element in elementsFromLayout)
                    {
                        element.Arrange(new Rect(0, offset, element.DesiredSize.Width, element.DesiredSize.Height));
                        offset += element.DesiredSize.Height;
                    }
                    return finalSize;
                }
            };
            var repeater = CreateLoadedRepeater(data, createLayout);

            RunOnUIThread.Execute(() =>
            {
                data.Move(4, 1);
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "Right after Move");
                repeater.UpdateLayout();
                Verify.AreEqual(data.Count, elementsFromLayout.Count);
                for (int i = 0; i < data.Count; i++)
                {
                    Verify.AreEqual(data[i], GetElementContent(elementsFromLayout[i]), $"GetOrCreateElementAt({i}) returns the element of item {i}.");
                }
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "After layout");
                VerifyVisualOrderMatchesData(repeater, data);
            });
        }

        // Scenario: a custom VirtualizingLayout realizes a window [first, last] of 20 items with GetOrCreateElementAt (no
        //           ForceCreate). A Move from outside the window shifts a realized element just past an edge of the window,
        //           and the window grows by one at that edge before the next layout:
        //           (a) window [0, 4], Move(10, 1): items 1..4 shift to 2..5 and the window becomes [0, 5];
        //           (b) window [5, 9], Move(2, 15): items 5..9 shift to 4..8 and the window becomes [4, 9].
        // Expected: GetOrCreateElementAt(i) returns the element of item i for every i in the window; the element that was
        //           shifted past the edge is reused (same instance) instead of a second element being created for its index;
        //           ElementPrepared fires only for the one index whose item was not realized before the Move.
        // A failure means: the realized-range cache of the ViewManager is stale after a Move, so a realized element whose
        //                  index moved outside the cached range is not found and a duplicate element is created.
        [TestMethod]
        [TestProperty("Description", "Verifies that after a Move a custom VirtualizingLayout reuses a realized element whose index moved outside the previously realized range.")]
        public void VerifyCustomVirtualizingLayoutReusesShiftedElementAfterMove()
        {
            var cases = new[]
            {
                new { First = 0, Last = 4, OldIndex = 10, NewIndex = 1, NewFirst = 0, NewLast = 5, ShiftedIndex = 5, PreparedIndex = 1 },
                new { First = 5, Last = 9, OldIndex = 2, NewIndex = 15, NewFirst = 4, NewLast = 9, ShiftedIndex = 4, PreparedIndex = 9 },
            };

            foreach (var c in cases)
            {
                Log.Comment($"Window [{c.First}, {c.Last}], Move({c.OldIndex}, {c.NewIndex}), new window [{c.NewFirst}, {c.NewLast}]");
                var data = new ObservableCollection<string>(Enumerable.Range(0, 20).Select(i => "Item #" + i));
                int first = c.First;
                int last = c.Last;
                var elementsFromLayout = new List<UIElement>();
                Func<Layout> createLayout = () => new MockVirtualizingLayout()
                {
                    MeasureLayoutFunc = (availableSize, context) =>
                    {
                        elementsFromLayout.Clear();
                        for (int i = first; i <= last; i++)
                        {
                            var element = context.GetOrCreateElementAt(i);
                            element.Measure(availableSize);
                            elementsFromLayout.Add(element);
                        }
                        return new Size(100, 30 * (last - first + 1));
                    },
                    ArrangeLayoutFunc = (finalSize, context) =>
                    {
                        double offset = 0;
                        foreach (var element in elementsFromLayout)
                        {
                            element.Arrange(new Rect(0, offset, element.DesiredSize.Width, element.DesiredSize.Height));
                            offset += element.DesiredSize.Height;
                        }
                        return finalSize;
                    }
                };
                var repeater = CreateLoadedRepeater(data, createLayout);

                RunOnUIThread.Execute(() =>
                {
                    VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "Before Move");
                    // The item that the Move shifts past the edge of the window.
                    string shiftedItem = data[c.ShiftedIndex + (c.OldIndex > c.NewIndex ? -1 : 1)];
                    var shiftedElement = Enumerable.Range(c.First, c.Last - c.First + 1)
                        .Select(i => repeater.TryGetElement(i))
                        .Single(element => GetElementContent(element) == shiftedItem);

                    var preparedIndices = new List<int>();
                    repeater.ElementPrepared += (sender, args) => preparedIndices.Add(args.Index);

                    data.Move(c.OldIndex, c.NewIndex);
                    Verify.AreEqual(c.ShiftedIndex, repeater.GetElementIndex(shiftedElement), $"Right after Move: '{shiftedItem}' reports index {c.ShiftedIndex}.");

                    first = c.NewFirst;
                    last = c.NewLast;
                    repeater.InvalidateMeasure();
                    repeater.UpdateLayout();

                    Log.Comment($"ElementPrepared indices: [{string.Join(", ", preparedIndices)}]");
                    Verify.AreEqual(last - first + 1, elementsFromLayout.Count);
                    for (int i = first; i <= last; i++)
                    {
                        Verify.AreEqual(data[i], GetElementContent(elementsFromLayout[i - first]), $"GetOrCreateElementAt({i}) returns the element of item {i}.");
                    }
                    Verify.AreSame(shiftedElement, elementsFromLayout[c.ShiftedIndex - first], $"The realized element of '{shiftedItem}' is reused at index {c.ShiftedIndex}.");
                    Verify.AreEqual(1, preparedIndices.Count, "Only the item that was not realized before the Move gets a new element.");
                    Verify.AreEqual(c.PreparedIndex, preparedIndices[0]);
                    VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "After layout");
                });
            }
        }

        // Scenario: a custom VirtualizingLayout realizes items 0..4 of 20 with GetOrCreateElementAt (no ForceCreate). During
        //           RemoveAt(0), the ElementClearing handler of the removed element calls GetOrCreateElement(10), an index that
        //           is not realized; then layout runs for the same window [0, 4].
        // Expected: the element of 'Item #1' (now at index 0) is reused for index 0 (same instance as before the Remove), and
        //           ElementPrepared fires for no index the layout had realized before the Remove (0..3 after the shift).
        // A failure means: the realized-range cache of the ViewManager was computed by the handler call while the removed
        //                  element still counted at index 0, the clear of that element then moved the start of the cached range
        //                  past index 0, and the layout got a second element for index 0 instead of the realized one.
        [TestMethod]
        [TestProperty("Description", "Verifies that a custom VirtualizingLayout reuses the shifted element after a Remove whose ElementClearing handler calls GetOrCreateElement.")]
        public void VerifyCustomVirtualizingLayoutReusesShiftedElementAfterGetOrCreateElementInElementClearingDuringRemove()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 20).Select(i => "Item #" + i));
            const int first = 0;
            const int last = 4;
            var elementsFromLayout = new List<UIElement>();
            Func<Layout> createLayout = () => new MockVirtualizingLayout()
            {
                MeasureLayoutFunc = (availableSize, context) =>
                {
                    elementsFromLayout.Clear();
                    for (int i = first; i <= last; i++)
                    {
                        var element = context.GetOrCreateElementAt(i);
                        element.Measure(availableSize);
                        elementsFromLayout.Add(element);
                    }
                    return new Size(100, 30 * (last - first + 1));
                },
                ArrangeLayoutFunc = (finalSize, context) =>
                {
                    double offset = 0;
                    foreach (var element in elementsFromLayout)
                    {
                        element.Arrange(new Rect(0, offset, element.DesiredSize.Width, element.DesiredSize.Height));
                        offset += element.DesiredSize.Height;
                    }
                    return finalSize;
                }
            };
            var repeater = CreateLoadedRepeater(data, createLayout);

            RunOnUIThread.Execute(() =>
            {
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "Before Remove");
                var shiftedElement = repeater.TryGetElement(1);
                Verify.AreEqual("Item #1", GetElementContent(shiftedElement));

                int clearing = 0;
                UIElement createdInHandler = null;
                repeater.ElementClearing += (sender, args) =>
                {
                    clearing++;
                    Log.Comment($"ElementClearing #{clearing} '{GetElementContent(args.Element)}'");
                    if (clearing == 1)
                    {
                        createdInHandler = sender.GetOrCreateElement(10);
                        Log.Comment($"GetOrCreateElement(10) in the handler returned '{GetElementContent(createdInHandler)}'");
                    }
                };

                data.RemoveAt(0);
                Verify.IsTrue(clearing >= 1, "The removed element was cleared.");
                Verify.AreEqual(data[10], GetElementContent(createdInHandler), "GetOrCreateElement(10) in the handler returns the element of item 10.");
                Verify.AreSame(shiftedElement, repeater.TryGetElement(0), "Right after Remove: TryGetElement(0) is the element of 'Item #1'.");

                var preparedIndices = new List<int>();
                repeater.ElementPrepared += (sender, args) => preparedIndices.Add(args.Index);
                repeater.InvalidateMeasure();
                repeater.UpdateLayout();

                Log.Comment($"ElementPrepared indices during layout: [{string.Join(", ", preparedIndices)}]");
                Verify.AreEqual(last - first + 1, elementsFromLayout.Count);
                for (int i = first; i <= last; i++)
                {
                    Verify.AreEqual(data[i], GetElementContent(elementsFromLayout[i - first]), $"GetOrCreateElementAt({i}) returns the element of item {i}.");
                }
                Verify.AreSame(shiftedElement, elementsFromLayout[0], "The realized element of 'Item #1' is reused at index 0.");
                Verify.AreSame(shiftedElement, repeater.TryGetElement(0), "After layout: TryGetElement(0) is the element of 'Item #1'.");
                Verify.IsFalse(preparedIndices.Any(index => index < last), "No new element for the indices that were realized before the Remove.");
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "After layout");
            });
        }

        // Scenario: a custom INotifyCollectionChanged source raises Move with null OldItems/NewItems.
        // Expected: the repeater treats it as a move of one item (like the layouts do), so it stays consistent.
        // A failure means: a Move without item lists crashes or corrupts the index mapping.
        [TestMethod]
        [TestProperty("Description", "Verifies that a Move notification without OldItems is handled as a single-item move.")]
        public void VerifyMoveWithoutOldItems()
        {
            RawNotificationItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new RawNotificationItemsSource(Enumerable.Range(0, 5).ToList()));
            var repeater = CreateLoadedRepeater(dataSource);

            RunOnUIThread.Execute(() =>
            {
                dataSource.MoveWithoutItems(oldIndex: 0, newIndex: 3);
                VerifyRealizedElementsMatchData(repeater, dataSource.Inner.Select(i => i.ToString()).ToList(), expectAllRealized: false, context: "Right after Move");
                repeater.UpdateLayout();
                VerifyRealizedElementsMatchData(repeater, dataSource.Inner.Select(i => i.ToString()).ToList(), expectAllRealized: true, context: "After layout");
            });
        }

        // Scenario: a fully realized 5-item repeater over an ObservableCollection. The first ElementIndexChanged handler call
        //           replaces ItemsSource with a new collection while the repeater is still handling the collection change:
        //           Move(0, 4), and the Insert(0) / RemoveAt(0) precedents.
        // Expected: no crash. ElementIndexChanged is raised only for elements that are still realized at args.NewIndex, so it
        //           is not raised again after the new ItemsSource cleared the old elements. After layout the repeater shows the
        //           new collection.
        // A failure means: the old ItemsSourceView is destroyed while it is still raising CollectionChanged (crash of the test
        //                  process), or the repeater reports index changes for elements it has already cleared.
        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource from an ElementIndexChanged handler during a Move does not crash and raises no further ElementIndexChanged for cleared elements.")]
        public void VerifyItemsSourceReplacedInElementIndexChangedDuringMove()
        {
            VerifyItemsSourceReplacedInElementIndexChanged("Move(0, 4)", data => data.Move(0, 4));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource from an ElementIndexChanged handler during an Insert does not crash.")]
        public void VerifyItemsSourceReplacedInElementIndexChangedDuringInsert()
        {
            VerifyItemsSourceReplacedInElementIndexChanged("Insert(0)", data => data.Insert(0, "Inserted"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource from an ElementIndexChanged handler during a Remove does not crash.")]
        public void VerifyItemsSourceReplacedInElementIndexChangedDuringRemove()
        {
            VerifyItemsSourceReplacedInElementIndexChanged("RemoveAt(0)", data => data.RemoveAt(0));
        }

        private void VerifyItemsSourceReplacedInElementIndexChanged(string operation, Action<ObservableCollection<string>> change)
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var newData = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "New #" + i));
            var repeater = CreateLoadedRepeater(data);

            RunOnUIThread.Execute(() =>
            {
                var events = new List<Tuple<int, int, int>>(); // OldIndex, NewIndex, GetElementIndex(args.Element) in the handler.
                bool replaced = false;
                repeater.ElementIndexChanged += (sender, args) =>
                {
                    int reportedIndex = sender.GetElementIndex(args.Element);
                    Log.Comment($"ElementIndexChanged {args.OldIndex} -> {args.NewIndex} '{GetElementContent(args.Element)}', GetElementIndex = {reportedIndex}");
                    events.Add(Tuple.Create(args.OldIndex, args.NewIndex, reportedIndex));
                    if (!replaced)
                    {
                        replaced = true;
                        Log.Comment("Handler: ItemsSource = new collection");
                        sender.ItemsSource = newData;
                    }
                };

                Log.Comment(operation);
                change(data);
                Log.Comment($"{operation} returned");

                Verify.IsTrue(replaced, $"{operation} raised ElementIndexChanged, so the handler replaced ItemsSource.");
                foreach (var e in events)
                {
                    Verify.AreEqual(e.Item2, e.Item3, $"ElementIndexChanged {e.Item1} -> {e.Item2} is raised only for an element that is realized at its new index.");
                }
                Verify.AreEqual(1, events.Count, "No ElementIndexChanged after the new ItemsSource cleared the old elements.");

                repeater.UpdateLayout();
                Verify.AreSame(newData, repeater.ItemsSource);
                VerifyRealizedElementsMatchData(repeater, newData, expectAllRealized: true, context: "After layout");
            });
        }

        // Scenario: a custom VirtualizingLayout that does not override OnItemsChangedCore (so it recycles nothing when
        //           ItemsSource is replaced) realizes every index with GetOrCreateElementAt. A collection change re-indexes a
        //           realized element to an index outside the realized range that the ViewManager has cached, and the
        //           ElementIndexChanged handler for that element replaces ItemsSource with a 6-item collection, which ends the
        //           change. Then layout.
        //           (a) RemoveAt(0) of 5 items, children created in index order: item 0 is cleared (the cached range shrinks
        //               to [1, 4]) and item 1 moves to index 0.
        //           (b) Replace(0, 1 -> 2) of 5 items, children created in reverse index order: item 4 is re-indexed first
        //               and moves to index 5, past the cached range [0, 4].
        // Expected: exactly one ElementIndexChanged; the element that the handler saw keeps its new index; no two realized
        //           children report the same index; ElementPrepared fires only for the indices that had no realized element.
        // A failure means: when a handler ends the change, the repeater keeps a realized-range cache that misses a realized
        //                  element (the cache update at the end of the Remove/Replace loop is skipped), so the layout gets a
        //                  second element for that element's index.
        [TestMethod]
        [TestProperty("Description", "Verifies that a custom VirtualizingLayout reuses the element re-indexed by a Remove after an ElementIndexChanged handler replaced ItemsSource.")]
        public void VerifyCustomVirtualizingLayoutReusesElementAfterItemsSourceReplacedDuringRemove()
        {
            VerifyCustomVirtualizingLayoutReusesElementAfterItemsSourceReplaced(
                "RemoveAt(0)",
                () => new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i)),
                source => ((ObservableCollection<string>)source).RemoveAt(0),
                realizeInReverseOrder: false,
                expectedNewIndex: 0);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that a custom VirtualizingLayout reuses the element re-indexed by a count-changing Replace after an ElementIndexChanged handler replaced ItemsSource.")]
        public void VerifyCustomVirtualizingLayoutReusesElementAfterItemsSourceReplacedDuringReplace()
        {
            VerifyCustomVirtualizingLayoutReusesElementAfterItemsSourceReplaced(
                "Replace(0, 1 -> 2)",
                () => new CustomItemsSource(Enumerable.Range(0, 5).ToList()),
                source => ((CustomItemsSource)source).Replace(index: 0, oldCount: 1, newCount: 2, reset: false),
                realizeInReverseOrder: true,
                expectedNewIndex: 5);
        }

        private void VerifyCustomVirtualizingLayoutReusesElementAfterItemsSourceReplaced(
            string operation,
            Func<object> createSource,
            Action<object> change,
            bool realizeInReverseOrder,
            int expectedNewIndex)
        {
            object source = null;
            RunOnUIThread.Execute(() => source = createSource());
            var newData = new ObservableCollection<string>(Enumerable.Range(0, 6).Select(i => "New #" + i));
            var elementsFromLayout = new Dictionary<int, UIElement>();
            Func<Layout> createLayout = () => new MockVirtualizingLayout()
            {
                MeasureLayoutFunc = (availableSize, context) =>
                {
                    elementsFromLayout.Clear();
                    int count = context.ItemCount;
                    for (int k = 0; k < count; k++)
                    {
                        int i = realizeInReverseOrder ? count - 1 - k : k;
                        var element = context.GetOrCreateElementAt(i);
                        element.Measure(availableSize);
                        elementsFromLayout[i] = element;
                    }
                    return new Size(100, 30 * count);
                },
                ArrangeLayoutFunc = (finalSize, context) =>
                {
                    foreach (var entry in elementsFromLayout)
                    {
                        entry.Value.Arrange(new Rect(0, 30 * entry.Key, entry.Value.DesiredSize.Width, entry.Value.DesiredSize.Height));
                    }
                    return finalSize;
                }
            };
            var repeater = CreateLoadedRepeater(source, createLayout);

            RunOnUIThread.Execute(() =>
            {
                var events = new List<string>();
                UIElement reindexedElement = null;
                repeater.ElementIndexChanged += (sender, args) =>
                {
                    string description = $"ElementIndexChanged {args.OldIndex} -> {args.NewIndex} '{GetElementContent(args.Element)}'";
                    Log.Comment(description);
                    events.Add(description);
                    if (reindexedElement == null)
                    {
                        reindexedElement = args.Element;
                        Verify.AreEqual(expectedNewIndex, args.NewIndex, $"{operation}: the first re-indexed element moves to index {expectedNewIndex}.");
                        Log.Comment("Handler: ItemsSource = new collection");
                        sender.ItemsSource = newData;
                    }
                };
                var preparedIndices = new List<int>();
                repeater.ElementPrepared += (sender, args) => preparedIndices.Add(args.Index);

                Log.Comment(operation);
                change(source);
                Log.Comment($"{operation} returned");

                Verify.IsNotNull(reindexedElement, $"{operation} raised ElementIndexChanged, so the handler replaced ItemsSource.");
                Verify.AreEqual(1, events.Count, "No ElementIndexChanged after the handler replaced ItemsSource: " + string.Join(" | ", events));
                Verify.AreEqual(expectedNewIndex, repeater.GetElementIndex(reindexedElement), "The re-indexed element keeps its new index.");

                var realizedBeforeLayout = new HashSet<int>(
                    Enumerable.Range(0, VisualTreeHelper.GetChildrenCount(repeater))
                        .Select(c => repeater.GetElementIndex((UIElement)VisualTreeHelper.GetChild(repeater, c)))
                        .Where(index => index >= 0));
                Log.Comment($"Before layout: realized indices [{string.Join(", ", realizedBeforeLayout.OrderBy(i => i))}]");

                repeater.UpdateLayout();

                var children = Enumerable.Range(0, VisualTreeHelper.GetChildrenCount(repeater)).Select(c => (UIElement)VisualTreeHelper.GetChild(repeater, c)).ToList();
                Log.Comment($"After layout: children [{string.Join(", ", children.Select(child => GetElementContent(child) + "@" + repeater.GetElementIndex(child)))}]");
                Log.Comment($"After layout: ElementPrepared indices [{string.Join(", ", preparedIndices)}]");

                Verify.AreEqual(newData.Count, elementsFromLayout.Count);
                var seen = new HashSet<int>();
                foreach (var child in children)
                {
                    int index = repeater.GetElementIndex(child);
                    if (index >= 0)
                    {
                        Verify.IsTrue(seen.Add(index), $"Only one realized child reports index {index}.");
                    }
                }
                foreach (int index in preparedIndices)
                {
                    Verify.IsFalse(realizedBeforeLayout.Contains(index), $"ElementPrepared({index}) fires only for an index that had no realized element.");
                }
            });
        }

        // Scenario: a custom VirtualizingLayout that does not override OnItemsChangedCore (so it keeps every element when the
        //           repeater is reset) realizes every index of a 5-item ObservableCollection. The first ElementIndexChanged
        //           handler call sets a new ItemTemplate (no layout), which resets the repeater and ends its collection change.
        //           Done for Insert(0) and RemoveAt(0).
        // Expected: right after the change and after the next layout, every realized element reports the index of its item
        //           (TryGetElement(i) shows item i, no two elements report the same index), and the layout gets the element
        //           of item i for index i.
        // A failure means: the repeater stopped re-indexing the realized elements when the handler ended the change, so
        //                  elements that the reset did not clear keep their pre-change index (wrong element for an index).
        [TestMethod]
        [TestProperty("Description", "Verifies that a custom VirtualizingLayout gets post-insert indices after an ElementIndexChanged handler set ItemTemplate during an Insert.")]
        public void VerifyCustomVirtualizingLayoutAfterItemTemplateReplacedInElementIndexChangedDuringInsert()
        {
            VerifyCustomVirtualizingLayoutAfterItemTemplateReplacedInElementIndexChanged("Insert(0)", data => data.Insert(0, "Inserted"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that a custom VirtualizingLayout gets post-remove indices after an ElementIndexChanged handler set ItemTemplate during a Remove.")]
        public void VerifyCustomVirtualizingLayoutAfterItemTemplateReplacedInElementIndexChangedDuringRemove()
        {
            VerifyCustomVirtualizingLayoutAfterItemTemplateReplacedInElementIndexChanged("RemoveAt(0)", data => data.RemoveAt(0));
        }

        private void VerifyCustomVirtualizingLayoutAfterItemTemplateReplacedInElementIndexChanged(string operation, Action<ObservableCollection<string>> change)
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var elementsFromLayout = new List<UIElement>();
            Func<Layout> createLayout = () => new MockVirtualizingLayout()
            {
                MeasureLayoutFunc = (availableSize, context) =>
                {
                    elementsFromLayout.Clear();
                    double height = 0;
                    for (int i = 0; i < context.ItemCount; i++)
                    {
                        var element = context.GetOrCreateElementAt(i);
                        element.Measure(availableSize);
                        elementsFromLayout.Add(element);
                        height += element.DesiredSize.Height;
                    }
                    return new Size(100, height);
                },
                ArrangeLayoutFunc = (finalSize, context) =>
                {
                    double offset = 0;
                    foreach (var element in elementsFromLayout)
                    {
                        element.Arrange(new Rect(0, offset, element.DesiredSize.Width, element.DesiredSize.Height));
                        offset += element.DesiredSize.Height;
                    }
                    return finalSize;
                }
            };
            var repeater = CreateLoadedRepeater(data, createLayout);

            RunOnUIThread.Execute(() =>
            {
                bool replaced = false;
                repeater.ElementIndexChanged += (sender, args) =>
                {
                    Log.Comment($"ElementIndexChanged {args.OldIndex} -> {args.NewIndex} '{GetElementContent(args.Element)}', GetElementIndex = {sender.GetElementIndex(args.Element)}");
                    if (!replaced)
                    {
                        replaced = true;
                        Log.Comment("Handler: ItemTemplate = new template");
                        sender.ItemTemplate = XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><Button Content='{Binding}' Height='30' Tag='NewTemplate'/></DataTemplate>");
                    }
                };

                Log.Comment(operation);
                change(data);
                Log.Comment($"{operation} returned");

                Verify.IsTrue(replaced, $"{operation} raised ElementIndexChanged, so the handler set ItemTemplate.");
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: false, context: "Right after " + operation);

                repeater.UpdateLayout();
                Verify.AreEqual(data.Count, elementsFromLayout.Count);
                for (int i = 0; i < data.Count; i++)
                {
                    Verify.AreEqual(data[i], GetElementContent(elementsFromLayout[i]), $"GetOrCreateElementAt({i}) returns the element of item {i}.");
                }
                VerifyRealizedElementsMatchData(repeater, data, expectAllRealized: true, context: "After layout");
                VerifyVisualOrderMatchesData(repeater, data);
            });
        }

        // Scenario: a 100-item StackLayout repeater in a ScrollViewer; item 90 is focused and the view scrolled back to the
        //           top, so item 90 is held in the pinned pool (not by the layout). The first ElementIndexChanged handler call
        //           sets a new ItemTemplate (no layout), which resets the layout and ends the collection change. Done for
        //           Insert(0) and RemoveAt(0).
        // Expected: the pinned element reports the index of its item after the change, and after scrolling to it the
        //           repeater uses it for that index (no second element for item 90, no two elements with the same index).
        // A failure means: the repeater stopped re-indexing when the handler ended the change, so the pinned element (which
        //                  the layout reset does not clear) keeps its pre-change index.
        [TestMethod]
        [TestProperty("Description", "Verifies that a pinned element gets its post-insert index after an ElementIndexChanged handler set ItemTemplate during an Insert.")]
        public void VerifyPinnedElementIndexAfterItemTemplateReplacedInElementIndexChangedDuringInsert()
        {
            VerifyPinnedElementIndexAfterItemTemplateReplacedInElementIndexChanged("Insert(0)", data => data.Insert(0, "Inserted"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that a pinned element gets its post-remove index after an ElementIndexChanged handler set ItemTemplate during a Remove.")]
        public void VerifyPinnedElementIndexAfterItemTemplateReplacedInElementIndexChangedDuringRemove()
        {
            VerifyPinnedElementIndexAfterItemTemplateReplacedInElementIndexChanged("RemoveAt(0)", data => data.RemoveAt(0));
        }

        private void VerifyPinnedElementIndexAfterItemTemplateReplacedInElementIndexChanged(string operation, Action<ObservableCollection<string>> change)
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 100).Select(i => "Item #" + i));
            ScrollViewer scrollViewer = null;
            ItemsRepeater repeater = null;
            Button element90 = null;
            var gotFocus = new ManualResetEvent(false);

            StackLayout layout = null;
            RunOnUIThread.Execute(() => layout = new StackLayout());
            repeater = SetupRepeater(data, layout, out scrollViewer);
            ScrollTo(scrollViewer, 9000);

            RunOnUIThread.Execute(() =>
            {
                element90 = (Button)repeater.TryGetElement(90);
                Verify.IsNotNull(element90);
                element90.GotFocus += delegate { gotFocus.Set(); };
                Verify.IsTrue(element90.Focus(FocusState.Programmatic));
            });
            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), "Waiting for element 90 to get focus.");
            IdleSynchronizer.Wait();

            ScrollTo(scrollViewer, 0);

            int expectedIndex = -1;
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(repeater.TryGetElement(0));
                Verify.AreEqual(90, repeater.GetElementIndex(element90), "The focused element is pinned outside the realized range.");
                var children = Enumerable.Range(0, VisualTreeHelper.GetChildrenCount(repeater)).Select(c => (UIElement)VisualTreeHelper.GetChild(repeater, c)).ToList();
                Log.Comment($"Children before {operation}: [{string.Join(", ", children.Select(child => GetElementContent(child) + "@" + repeater.GetElementIndex(child)))}]");

                bool replaced = false;
                repeater.ElementIndexChanged += (sender, args) =>
                {
                    Log.Comment($"ElementIndexChanged {args.OldIndex} -> {args.NewIndex} '{GetElementContent(args.Element)}'");
                    if (!replaced)
                    {
                        replaced = true;
                        Log.Comment("Handler: ItemTemplate = new element factory");
                        // Same kind of factory as SetupRepeater, so that the old elements are recycled into a factory of their kind.
                        var newFactory = new RecyclingElementFactory() { RecyclePool = new RecyclePool() };
                        newFactory.Templates["Item"] = XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><Button Content='{Binding}' Height='100' Tag='NewTemplate'/></DataTemplate>") as DataTemplate;
                        sender.ItemTemplate = newFactory;
                    }
                };

                Log.Comment(operation);
                change(data);
                Log.Comment($"{operation} returned");

                Verify.IsTrue(replaced, $"{operation} raised ElementIndexChanged, so the handler set ItemTemplate.");
                expectedIndex = data.IndexOf("Item #90");
                Verify.AreEqual(expectedIndex, repeater.GetElementIndex(element90), "The pinned element reports the index of its item.");
            });

            ScrollTo(scrollViewer, expectedIndex * 100);

            RunOnUIThread.Execute(() =>
            {
                var children = Enumerable.Range(0, VisualTreeHelper.GetChildrenCount(repeater)).Select(c => (UIElement)VisualTreeHelper.GetChild(repeater, c)).ToList();
                Log.Comment($"Children after scrolling to item 90: [{string.Join(", ", children.Select(child => GetElementContent(child) + "@" + repeater.GetElementIndex(child)))}]");
                Verify.AreEqual(expectedIndex, repeater.GetElementIndex(element90), "The pinned element keeps the index of its item.");
                Verify.AreSame(element90, repeater.TryGetElement(expectedIndex), "The repeater uses the pinned element for the index of its item.");
                var seenIndices = new HashSet<int>();
                var seenItems = new HashSet<string>();
                foreach (var child in children)
                {
                    int index = repeater.GetElementIndex(child);
                    if (index >= 0)
                    {
                        Verify.IsTrue(seenIndices.Add(index), $"Only one realized child reports index {index}.");
                        Verify.IsTrue(seenItems.Add(GetElementContent(child)), $"Item '{GetElementContent(child)}' is realized once.");
                        Verify.AreEqual(data[index], GetElementContent(child), $"The child that reports index {index} shows item {index}.");
                    }
                }

                // Remove the pinned item, so that the repeater clears the pinned element (created by the old ItemTemplate)
                // before the test tears the tree down.
                Log.Comment("Remove the pinned item");
                data.RemoveAt(expectedIndex);
                Log.Comment($"After the removal the old pinned element reports index {repeater.GetElementIndex(element90)}");
            });
        }

        // Scenario: a fully realized 10-item StackLayout repeater over a CustomItemsSource. A Replace that changes the item count
        //           leaves exactly one realized element after the replaced range, the last child (Replace(7, 2 -> 1): 9 -> 8,
        //           Replace(8, 1 -> 2): 9 -> 10). Its ElementIndexChanged handler replaces ItemsSource with a new 10-item
        //           collection and either calls UpdateLayout() (then an item is inserted at 9 and layout runs), or does not
        //           (then the cached realized range is invalid before the Replace because of two Moves that cancel out, and an
        //           item is inserted at 0 before layout).
        // Expected: no crash or assertion, one ElementIndexChanged, and after layout every realized element matches the new
        //           collection (TryGetElement(i) shows item i, one element per index, visual order = data order).
        // A failure means: the Replace updated the repeater's cached realized range after the handler replaced ItemsSource
        //                  (the loop ended on its own condition after the last child), so the cache no longer covers the
        //                  realized elements (two elements for one index) or is invalid (assertion in a checked build).
        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource and updating layout from the ElementIndexChanged handler of the last re-indexed child during a Replace with fewer items leaves the repeater consistent.")]
        public void VerifyItemsSourceReplacedAndLayoutUpdatedInLastElementIndexChangedDuringReplaceWithFewerItems()
        {
            VerifyItemsSourceReplacedInLastElementIndexChangedDuringReplace(oldCount: 2, newCount: 1, updateLayout: true);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource and updating layout from the ElementIndexChanged handler of the last re-indexed child during a Replace with more items leaves the repeater consistent.")]
        public void VerifyItemsSourceReplacedAndLayoutUpdatedInLastElementIndexChangedDuringReplaceWithMoreItems()
        {
            VerifyItemsSourceReplacedInLastElementIndexChangedDuringReplace(oldCount: 1, newCount: 2, updateLayout: true);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource from the ElementIndexChanged handler of the last re-indexed child during a Replace with fewer items, with an invalid realized-range cache, leaves the repeater consistent.")]
        public void VerifyItemsSourceReplacedInLastElementIndexChangedDuringReplaceAfterMoveWithFewerItems()
        {
            VerifyItemsSourceReplacedInLastElementIndexChangedDuringReplace(oldCount: 2, newCount: 1, updateLayout: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource from the ElementIndexChanged handler of the last re-indexed child during a Replace with more items, with an invalid realized-range cache, leaves the repeater consistent.")]
        public void VerifyItemsSourceReplacedInLastElementIndexChangedDuringReplaceAfterMoveWithMoreItems()
        {
            VerifyItemsSourceReplacedInLastElementIndexChangedDuringReplace(oldCount: 1, newCount: 2, updateLayout: false);
        }

        private void VerifyItemsSourceReplacedInLastElementIndexChangedDuringReplace(int oldCount, int newCount, bool updateLayout)
        {
            // Only the item at 9 is after the replaced range.
            int replaceIndex = 9 - oldCount;
            string operation = $"Replace({replaceIndex}, {oldCount} -> {newCount})";
            CustomItemsSource dataSource = null;
            ObservableCollection<string> newData = null;
            RunOnUIThread.Execute(() =>
            {
                dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList());
                newData = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "New #" + i));
            });
            var repeater = CreateLoadedRepeater(dataSource);

            RunOnUIThread.Execute(() =>
            {
                VerifyRealizedElementsMatchData(repeater, dataSource.Inner.Select(i => i.ToString()).ToList(), expectAllRealized: true, context: "Before " + operation);

                var events = new List<string>();
                bool armed = false;
                bool replaced = false;
                repeater.ElementIndexChanged += (sender, args) =>
                {
                    string description = $"ElementIndexChanged {args.OldIndex} -> {args.NewIndex} '{GetElementContent(args.Element)}', GetElementIndex = {sender.GetElementIndex(args.Element)}";
                    Log.Comment(description);
                    if (!armed)
                    {
                        return;
                    }
                    events.Add(description);
                    if (!replaced)
                    {
                        replaced = true;
                        Log.Comment("Handler: ItemsSource = new collection");
                        sender.ItemsSource = newData;
                        if (updateLayout)
                        {
                            Log.Comment("Handler: UpdateLayout()");
                            sender.UpdateLayout();
                            Log.Comment($"Handler: UpdateLayout() returned, {CountRealizedChildren(sender)} realized children");
                        }
                    }
                };

                if (!updateLayout)
                {
                    // Two Moves that cancel out: every element stays realized at its index, but the cached realized range
                    // is invalid when the Replace starts.
                    Log.Comment("Move(0, 1); Move(1, 0)");
                    dataSource.Move(oldIndex: 0, newIndex: 1, count: 1, reset: false);
                    dataSource.Move(oldIndex: 1, newIndex: 0, count: 1, reset: false);
                }

                armed = true;
                Log.Comment(operation);
                dataSource.Replace(index: replaceIndex, oldCount: oldCount, newCount: newCount, reset: false);
                Log.Comment($"{operation} returned");

                Verify.IsTrue(replaced, $"{operation} raised ElementIndexChanged, so the handler replaced ItemsSource.");
                Verify.AreEqual(1, events.Count, "One ElementIndexChanged (only the last child is re-indexed): " + string.Join(" | ", events));

                int insertIndex = updateLayout ? 9 : 0;
                Log.Comment($"Insert({insertIndex}) into the new collection");
                newData.Insert(insertIndex, "Inserted");
                Log.Comment($"Insert({insertIndex}) returned");

                repeater.UpdateLayout();
                Verify.AreSame(newData, repeater.ItemsSource);
                VerifyRealizedElementsMatchData(repeater, newData, expectAllRealized: true, context: "After layout");
                VerifyVisualOrderMatchesData(repeater, newData);
            });
        }

        // Scenario: a fully realized 5-item StackLayout repeater over an ObservableCollection. The first ElementIndexChanged
        //           handler call replaces ItemsSource with a new collection (or sets a new ItemTemplate) and then calls
        //           UpdateLayout(), which runs because the replacement ended the repeater's collection change. Done for
        //           Move(0, 4), Insert(0) and RemoveAt(0).
        // Expected: no crash and no further ElementIndexChanged (the layout in the handler realized the elements for the
        //           current state, so the rest of the old change does not apply to them). After the change returns and one
        //           more layout, every realized element matches the current collection (TryGetElement(i) shows item i,
        //           GetElementIndex round-trips, visual order = data order) and uses the current ItemTemplate.
        // A failure means: the repeater applies the stale Move/Insert/Remove to the elements it realized for the new state
        //                  (wrong element for an index, wrong content or order on screen), the same class of bug as
        //                  GitHub #11570.
        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource and updating layout from an ElementIndexChanged handler during a Move leaves the repeater consistent with the new collection.")]
        public void VerifyItemsSourceReplacedAndLayoutUpdatedInElementIndexChangedDuringMove()
        {
            VerifyStateReplacedAndLayoutUpdatedInElementIndexChanged("Move(0, 4)", data => data.Move(0, 4), replaceItemTemplate: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource and updating layout from an ElementIndexChanged handler during an Insert leaves the repeater consistent with the new collection.")]
        public void VerifyItemsSourceReplacedAndLayoutUpdatedInElementIndexChangedDuringInsert()
        {
            VerifyStateReplacedAndLayoutUpdatedInElementIndexChanged("Insert(0)", data => data.Insert(0, "Inserted"), replaceItemTemplate: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource and updating layout from an ElementIndexChanged handler during a Remove leaves the repeater consistent with the new collection.")]
        public void VerifyItemsSourceReplacedAndLayoutUpdatedInElementIndexChangedDuringRemove()
        {
            VerifyStateReplacedAndLayoutUpdatedInElementIndexChanged("RemoveAt(0)", data => data.RemoveAt(0), replaceItemTemplate: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that setting ItemTemplate and updating layout from an ElementIndexChanged handler during a Move leaves the repeater consistent with the collection.")]
        public void VerifyItemTemplateReplacedAndLayoutUpdatedInElementIndexChangedDuringMove()
        {
            VerifyStateReplacedAndLayoutUpdatedInElementIndexChanged("Move(0, 4)", data => data.Move(0, 4), replaceItemTemplate: true);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that setting ItemTemplate and updating layout from an ElementIndexChanged handler during an Insert leaves the repeater consistent with the collection.")]
        public void VerifyItemTemplateReplacedAndLayoutUpdatedInElementIndexChangedDuringInsert()
        {
            VerifyStateReplacedAndLayoutUpdatedInElementIndexChanged("Insert(0)", data => data.Insert(0, "Inserted"), replaceItemTemplate: true);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that setting ItemTemplate and updating layout from an ElementIndexChanged handler during a Remove leaves the repeater consistent with the collection.")]
        public void VerifyItemTemplateReplacedAndLayoutUpdatedInElementIndexChangedDuringRemove()
        {
            VerifyStateReplacedAndLayoutUpdatedInElementIndexChanged("RemoveAt(0)", data => data.RemoveAt(0), replaceItemTemplate: true);
        }

        private void VerifyStateReplacedAndLayoutUpdatedInElementIndexChanged(string operation, Action<ObservableCollection<string>> change, bool replaceItemTemplate)
        {
            const string newTemplateTag = "NewTemplate";
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var newData = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "New #" + i));
            var repeater = CreateLoadedRepeater(data);

            RunOnUIThread.Execute(() =>
            {
                var events = new List<string>();
                bool replaced = false;
                repeater.ElementIndexChanged += (sender, args) =>
                {
                    string description = $"ElementIndexChanged {args.OldIndex} -> {args.NewIndex} '{GetElementContent(args.Element)}', GetElementIndex = {sender.GetElementIndex(args.Element)}";
                    Log.Comment(description);
                    events.Add(description);
                    if (!replaced)
                    {
                        replaced = true;
                        if (replaceItemTemplate)
                        {
                            Log.Comment("Handler: ItemTemplate = new template");
                            sender.ItemTemplate = XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><Button Content='{Binding}' Height='30' Tag='" + newTemplateTag + @"'/></DataTemplate>");
                        }
                        else
                        {
                            Log.Comment("Handler: ItemsSource = new collection");
                            sender.ItemsSource = newData;
                        }

                        Log.Comment("Handler: UpdateLayout()");
                        sender.UpdateLayout();
                        Log.Comment($"Handler: UpdateLayout() returned, {CountRealizedChildren(sender)} realized children");
                    }
                };

                Log.Comment(operation);
                change(data);
                Log.Comment($"{operation} returned");

                Verify.IsTrue(replaced, $"{operation} raised ElementIndexChanged, so the handler replaced the state.");
                Verify.AreEqual(1, events.Count, "No ElementIndexChanged after the handler replaced the state and updated layout: " + string.Join(" | ", events));

                repeater.UpdateLayout();
                var currentData = replaceItemTemplate ? data : newData;
                Verify.AreSame(currentData, repeater.ItemsSource);
                VerifyRealizedElementsMatchData(repeater, currentData, expectAllRealized: true, context: "After layout");
                VerifyVisualOrderMatchesData(repeater, currentData);
                if (replaceItemTemplate)
                {
                    for (int i = 0; i < currentData.Count; i++)
                    {
                        Verify.AreEqual(newTemplateTag, (repeater.TryGetElement(i) as FrameworkElement)?.Tag as string, $"TryGetElement({i}) uses the new ItemTemplate.");
                    }
                }
            });
        }

        // Scenario: a fully realized 10-item StackLayout repeater over a CustomItemsSource. A Replace notification changes the
        //           item count (Replace(0, 1 -> 2) and Replace(0, 2 -> 1)), so the repeater re-indexes the realized elements after
        //           the replaced range and raises ElementIndexChanged for them. The first handler call replaces ItemsSource with a
        //           new collection (or sets a new ItemTemplate) and calls UpdateLayout(), or only replaces ItemsSource (then the test
        //           inserts an item into the new collection before the next layout).
        // Expected: no crash or assertion, exactly one ElementIndexChanged (the rest of the old Replace does not apply to the
        //           elements realized for the current state). After the change returns and one more layout, every realized
        //           element matches the current source (TryGetElement(i) shows item i, GetElementIndex round-trips, visual
        //           order = data order).
        // A failure means: the repeater keeps shifting the indices of elements it realized for the new state (wrong element for
        //                  an index, more ElementIndexChanged) or leaves its realized-range cache invalid (assertion in a
        //                  checked build), the same class of bug as GitHub #11570.
        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource and updating layout from an ElementIndexChanged handler during a Replace that adds items leaves the repeater consistent with the new collection.")]
        public void VerifyItemsSourceReplacedAndLayoutUpdatedInElementIndexChangedDuringReplaceWithMoreItems()
        {
            VerifyStateReplacedInElementIndexChangedDuringReplace(oldCount: 1, newCount: 2, replaceItemTemplate: false, updateLayout: true, removeLastItemFirst: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource and updating layout from an ElementIndexChanged handler during a Replace that removes items leaves the repeater consistent with the new collection.")]
        public void VerifyItemsSourceReplacedAndLayoutUpdatedInElementIndexChangedDuringReplaceWithFewerItems()
        {
            VerifyStateReplacedInElementIndexChangedDuringReplace(oldCount: 2, newCount: 1, replaceItemTemplate: false, updateLayout: true, removeLastItemFirst: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that setting ItemTemplate and updating layout from an ElementIndexChanged handler during a Replace that adds items leaves the repeater consistent with the source.")]
        public void VerifyItemTemplateReplacedAndLayoutUpdatedInElementIndexChangedDuringReplaceWithMoreItems()
        {
            VerifyStateReplacedInElementIndexChangedDuringReplace(oldCount: 1, newCount: 2, replaceItemTemplate: true, updateLayout: true, removeLastItemFirst: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that setting ItemTemplate and updating layout from an ElementIndexChanged handler during a Replace that removes items leaves the repeater consistent with the source.")]
        public void VerifyItemTemplateReplacedAndLayoutUpdatedInElementIndexChangedDuringReplaceWithFewerItems()
        {
            VerifyStateReplacedInElementIndexChangedDuringReplace(oldCount: 2, newCount: 1, replaceItemTemplate: true, updateLayout: true, removeLastItemFirst: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource (without updating layout) from an ElementIndexChanged handler during a Replace that adds items leaves the repeater consistent with the new collection.")]
        public void VerifyItemsSourceReplacedInElementIndexChangedDuringReplaceWithMoreItems()
        {
            VerifyStateReplacedInElementIndexChangedDuringReplace(oldCount: 1, newCount: 2, replaceItemTemplate: false, updateLayout: false, removeLastItemFirst: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource (without updating layout) from an ElementIndexChanged handler during a Replace that removes items leaves the repeater consistent with the new collection.")]
        public void VerifyItemsSourceReplacedInElementIndexChangedDuringReplaceWithFewerItems()
        {
            VerifyStateReplacedInElementIndexChangedDuringReplace(oldCount: 2, newCount: 1, replaceItemTemplate: false, updateLayout: false, removeLastItemFirst: false);
        }

        // Same as above, but the Replace directly follows a Remove of the last item (no layout in between), so the repeater's
        // cached realized range is not computed when the Replace starts. The handler only replaces ItemsSource; then an item
        // is inserted into the new collection before layout.
        // A failure means: the Replace adds its count change to a realized range computed while no element is held by the
        // layout, which leaves an invalid range that the Insert reads (assertion in a checked build).
        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource from an ElementIndexChanged handler during a Replace (adding items) that follows a Remove leaves the repeater consistent with the new collection.")]
        public void VerifyItemsSourceReplacedInElementIndexChangedDuringReplaceAfterRemoveWithMoreItems()
        {
            VerifyStateReplacedInElementIndexChangedDuringReplace(oldCount: 1, newCount: 2, replaceItemTemplate: false, updateLayout: false, removeLastItemFirst: true);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource from an ElementIndexChanged handler during a Replace (removing items) that follows a Remove leaves the repeater consistent with the new collection.")]
        public void VerifyItemsSourceReplacedInElementIndexChangedDuringReplaceAfterRemoveWithFewerItems()
        {
            VerifyStateReplacedInElementIndexChangedDuringReplace(oldCount: 2, newCount: 1, replaceItemTemplate: false, updateLayout: false, removeLastItemFirst: true);
        }

        private void VerifyStateReplacedInElementIndexChangedDuringReplace(int oldCount, int newCount, bool replaceItemTemplate, bool updateLayout, bool removeLastItemFirst)
        {
            const string newTemplateTag = "NewTemplate";
            string operation = $"Replace(0, {oldCount} -> {newCount})";
            CustomItemsSource dataSource = null;
            ObservableCollection<string> newData = null;
            RunOnUIThread.Execute(() =>
            {
                dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList());
                newData = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "New #" + i));
            });
            var repeater = CreateLoadedRepeater(dataSource);

            RunOnUIThread.Execute(() =>
            {
                VerifyRealizedElementsMatchData(repeater, dataSource.Inner.Select(i => i.ToString()).ToList(), expectAllRealized: true, context: "Before " + operation);

                var events = new List<string>();
                bool replaced = false;
                repeater.ElementIndexChanged += (sender, args) =>
                {
                    string description = $"ElementIndexChanged {args.OldIndex} -> {args.NewIndex} '{GetElementContent(args.Element)}', GetElementIndex = {sender.GetElementIndex(args.Element)}";
                    Log.Comment(description);
                    events.Add(description);
                    if (!replaced)
                    {
                        replaced = true;
                        if (replaceItemTemplate)
                        {
                            Log.Comment("Handler: ItemTemplate = new template");
                            sender.ItemTemplate = XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><Button Content='{Binding}' Height='30' Tag='" + newTemplateTag + @"'/></DataTemplate>");
                        }
                        else
                        {
                            Log.Comment("Handler: ItemsSource = new collection");
                            sender.ItemsSource = newData;
                        }

                        if (updateLayout)
                        {
                            Log.Comment("Handler: UpdateLayout()");
                            sender.UpdateLayout();
                            Log.Comment($"Handler: UpdateLayout() returned, {CountRealizedChildren(sender)} realized children");
                        }
                    }
                };

                if (removeLastItemFirst)
                {
                    Log.Comment("Remove(9)");
                    dataSource.Remove(index: 9, count: 1, reset: false);
                }

                Log.Comment(operation);
                dataSource.Replace(index: 0, oldCount: oldCount, newCount: newCount, reset: false);
                Log.Comment($"{operation} returned");

                Verify.IsTrue(replaced, $"{operation} raised ElementIndexChanged, so the handler replaced the state.");
                Verify.AreEqual(1, events.Count, "No ElementIndexChanged after the handler replaced the state: " + string.Join(" | ", events));

                if (!updateLayout && !replaceItemTemplate)
                {
                    // Another change before the next layout reads the repeater's cached realized range.
                    Log.Comment("Insert(0) into the new collection before layout");
                    newData.Insert(0, "Inserted");
                    Log.Comment("Insert(0) returned");
                }

                repeater.UpdateLayout();
                IList<string> currentData = replaceItemTemplate ? dataSource.Inner.Select(i => i.ToString()).ToList() : newData;
                Verify.AreSame(replaceItemTemplate ? (object)dataSource : newData, repeater.ItemsSource);
                VerifyRealizedElementsMatchData(repeater, currentData, expectAllRealized: true, context: "After layout");
                VerifyVisualOrderMatchesData(repeater, currentData);
                if (replaceItemTemplate)
                {
                    for (int i = 0; i < currentData.Count; i++)
                    {
                        Verify.AreEqual(newTemplateTag, (repeater.TryGetElement(i) as FrameworkElement)?.Tag as string, $"TryGetElement({i}) uses the new ItemTemplate.");
                    }
                }
            });
        }

        // Scenario: a fully realized 10-item StackLayout repeater over a CustomItemsSource; Replace(0, 1 -> 2) and
        //           Replace(0, 2 -> 1) with an ElementIndexChanged handler that only records the events.
        // Expected: one ElementIndexChanged per realized element after the replaced range, each to its index shifted by the
        //           count change, and the repeater is consistent with the source after layout.
        // A failure means: the Replace re-indexing stopped early or skipped elements although no handler replaced the state
        //                  (guard against stopping an uninterrupted change).
        [TestMethod]
        [TestProperty("Description", "Verifies that a Replace that changes the item count re-indexes every realized element after the replaced range when no handler replaces the state.")]
        public void VerifyElementIndexChangedDuringReplaceWithCountChange()
        {
            foreach (var counts in new[] { Tuple.Create(1, 2), Tuple.Create(2, 1) })
            {
                int oldCount = counts.Item1;
                int newCount = counts.Item2;
                string operation = $"Replace(0, {oldCount} -> {newCount})";
                CustomItemsSource dataSource = null;
                RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList()));
                var repeater = CreateLoadedRepeater(dataSource);

                RunOnUIThread.Execute(() =>
                {
                    var events = new List<Tuple<int, int>>();
                    repeater.ElementIndexChanged += (sender, args) =>
                    {
                        Log.Comment($"ElementIndexChanged {args.OldIndex} -> {args.NewIndex} '{GetElementContent(args.Element)}'");
                        events.Add(Tuple.Create(args.OldIndex, args.NewIndex));
                    };

                    Log.Comment(operation);
                    dataSource.Replace(index: 0, oldCount: oldCount, newCount: newCount, reset: false);

                    var expected = Enumerable.Range(oldCount, 10 - oldCount).Select(i => Tuple.Create(i, i + newCount - oldCount)).ToList();
                    Verify.AreEqual(
                        string.Join(", ", expected.Select(e => $"{e.Item1}->{e.Item2}")),
                        string.Join(", ", events.OrderBy(e => e.Item1).Select(e => $"{e.Item1}->{e.Item2}")),
                        $"{operation}: one ElementIndexChanged per realized element after the replaced range.");

                    repeater.UpdateLayout();
                    var currentData = dataSource.Inner.Select(i => i.ToString()).ToList();
                    VerifyRealizedElementsMatchData(repeater, currentData, expectAllRealized: true, context: $"After {operation} and layout");
                    VerifyVisualOrderMatchesData(repeater, currentData);
                });
            }
        }

        // Scenario: a fully realized 10-item repeater over a CustomItemsSource. With a StackLayout and with a NonVirtualizingLayout:
        //           Insert(1), Remove(1), and Remove(5) after Move(0, 9) plus layout (so a shifted element precedes the removed one
        //           in Children); with a StackLayout also Replace(1, 1 -> 2) and Replace(1, 2 -> 1). ElementIndexChanged and
        //           ElementClearing handlers record the events, and each ElementIndexChanged handler call asks the repeater about
        //           the element.
        // Expected: in every ElementIndexChanged, GetElementIndex(args.Element) is args.NewIndex, and TryGetElement(args.NewIndex)
        //           is args.Element or a removed/replaced element that is not cleared yet (StackLayout creates its elements with
        //           SuppressAutoRecycle and clears removed and replaced elements itself, after the repeater raised
        //           ElementIndexChanged); one ElementIndexChanged per shifted element; with the NonVirtualizingLayout
        //           (auto-recycled elements, cleared by the repeater) a Remove raises ElementClearing for the removed element
        //           before the first ElementIndexChanged; consistent after layout.
        // A failure means: ElementIndexChanged handlers see a partly re-indexed repeater, or the order of the events of a
        //                  change differs from the documented one.
        [TestMethod]
        [TestProperty("Description", "Verifies that ElementIndexChanged handlers see the post-change index mapping on Insert/Remove/Replace and that a Remove clears auto-recycled elements before ElementIndexChanged.")]
        public void VerifyElementIndexChangedHandlersSeePostChangeMapping()
        {
            foreach (bool nonVirtualizing in new[] { false, true })
            {
                VerifyElementIndexChangedHandlerSeesPostChangeMapping("Insert(1, 1)", s => s.Insert(index: 1, count: 1, reset: false), firstShiftedOldIndex: 1, shift: 1, affectedStart: -1, affectedCount: 0, isRemove: false, moveFirst: false, nonVirtualizing: nonVirtualizing);
                VerifyElementIndexChangedHandlerSeesPostChangeMapping("Remove(1, 1)", s => s.Remove(index: 1, count: 1, reset: false), firstShiftedOldIndex: 2, shift: -1, affectedStart: 1, affectedCount: 1, isRemove: true, moveFirst: false, nonVirtualizing: nonVirtualizing);
                VerifyElementIndexChangedHandlerSeesPostChangeMapping("Remove(5, 1) after Move(0, 9)", s => s.Remove(index: 5, count: 1, reset: false), firstShiftedOldIndex: 6, shift: -1, affectedStart: 5, affectedCount: 1, isRemove: true, moveFirst: true, nonVirtualizing: nonVirtualizing);
            }

            // StackLayout only: with a NonVirtualizingLayout the next layout pass reuses a replaced element that is still realized
            // at its index, which is a separate issue that this change does not address.
            VerifyElementIndexChangedHandlerSeesPostChangeMapping("Replace(1, 1 -> 2)", s => s.Replace(index: 1, oldCount: 1, newCount: 2, reset: false), firstShiftedOldIndex: 2, shift: 1, affectedStart: 1, affectedCount: 1, isRemove: false, moveFirst: false, nonVirtualizing: false);
            VerifyElementIndexChangedHandlerSeesPostChangeMapping("Replace(1, 2 -> 1)", s => s.Replace(index: 1, oldCount: 2, newCount: 1, reset: false), firstShiftedOldIndex: 3, shift: -1, affectedStart: 1, affectedCount: 2, isRemove: false, moveFirst: false, nonVirtualizing: false);
        }

        // affectedStart/affectedCount: the removed (isRemove) or replaced items, by their index before the change.
        private void VerifyElementIndexChangedHandlerSeesPostChangeMapping(string operation, Action<CustomItemsSource> change, int firstShiftedOldIndex, int shift, int affectedStart, int affectedCount, bool isRemove, bool moveFirst, bool nonVirtualizing)
        {
            operation += nonVirtualizing ? " (NonVirtualizingLayout)" : " (StackLayout)";
            Log.Comment(operation);
            const int itemCount = 10;
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, itemCount).ToList()));
            var repeater = CreateLoadedRepeater(dataSource, nonVirtualizing ? () => new NonVirtualStackLayout() : (Func<Layout>)null);

            RunOnUIThread.Execute(() =>
            {
                if (moveFirst)
                {
                    dataSource.Move(oldIndex: 0, newIndex: 9, count: 1, reset: false);
                    repeater.UpdateLayout();
                    VerifyRealizedElementsMatchData(repeater, dataSource.Inner.Select(i => i.ToString()).ToList(), expectAllRealized: true, context: "After Move(0, 9) and layout");
                }

                var affectedElements = Enumerable.Range(affectedStart, affectedCount).Select(i => repeater.TryGetElement(i)).ToList();
                Log.Comment($"Removed or replaced elements: [{string.Join(", ", affectedElements.Select(GetElementContent))}]");
                Verify.IsFalse(affectedElements.Contains(null), "The removed or replaced items are realized before the change.");

                var events = new List<string>();
                var pairs = new List<Tuple<int, int>>();
                var mismatches = new List<string>();
                var clearingPositions = new Dictionary<UIElement, int>();
                repeater.ElementClearing += (sender, args) =>
                {
                    if (!clearingPositions.ContainsKey(args.Element))
                    {
                        clearingPositions[args.Element] = events.Count;
                    }
                    events.Add($"ElementClearing '{GetElementContent(args.Element)}'");
                };
                repeater.ElementIndexChanged += (sender, args) =>
                {
                    var elementAtNewIndex = sender.TryGetElement(args.NewIndex);
                    int reportedIndex = sender.GetElementIndex(args.Element);
                    events.Add($"ElementIndexChanged {args.OldIndex} -> {args.NewIndex} '{GetElementContent(args.Element)}'");
                    pairs.Add(Tuple.Create(args.OldIndex, args.NewIndex));
                    if (reportedIndex != args.NewIndex)
                    {
                        mismatches.Add($"GetElementIndex('{GetElementContent(args.Element)}') is {reportedIndex}, not {args.NewIndex}");
                    }
                    // A removed or replaced element that is not cleared yet (see above) keeps its old index, so it can share the new index.
                    bool isUnclearedAffectedElement = affectedElements.Contains(elementAtNewIndex) && !(isRemove && nonVirtualizing);
                    if (elementAtNewIndex != args.Element && !isUnclearedAffectedElement)
                    {
                        mismatches.Add($"TryGetElement({args.NewIndex}) is '{GetElementContent(elementAtNewIndex)}', not '{GetElementContent(args.Element)}'");
                    }
                };

                change(dataSource);
                foreach (var e in events)
                {
                    Log.Comment(e);
                }

                Verify.AreEqual(0, mismatches.Count, $"{operation}: the ElementIndexChanged handler sees the post-change mapping: " + string.Join(" | ", mismatches));

                var expected = Enumerable.Range(firstShiftedOldIndex, itemCount - firstShiftedOldIndex).Select(i => Tuple.Create(i, i + shift)).ToList();
                Verify.AreEqual(
                    string.Join(", ", expected.Select(e => $"{e.Item1}->{e.Item2}")),
                    string.Join(", ", pairs.OrderBy(e => e.Item1).Select(e => $"{e.Item1}->{e.Item2}")),
                    $"{operation}: one ElementIndexChanged per realized element after the changed range.");

                if (isRemove)
                {
                    int firstIndexChanged = events.FindIndex(e => e.StartsWith("ElementIndexChanged"));
                    foreach (var removed in affectedElements)
                    {
                        Verify.IsTrue(clearingPositions.ContainsKey(removed), $"{operation}: ElementClearing is raised for the removed element.");
                        if (nonVirtualizing)
                        {
                            // Auto-recycled removed elements are cleared by the repeater before ElementIndexChanged. StackLayout
                            // clears its own (SuppressAutoRecycle) elements afterwards, in OnItemsChangedCore.
                            Verify.IsTrue(firstIndexChanged < 0 || clearingPositions[removed] < firstIndexChanged,
                                $"{operation}: the removed element is cleared before the first ElementIndexChanged.");
                        }
                    }
                }

                repeater.UpdateLayout();
                var currentData = dataSource.Inner.Select(i => i.ToString()).ToList();
                VerifyRealizedElementsMatchData(repeater, currentData, expectAllRealized: true, context: $"After {operation} and layout");
                VerifyVisualOrderMatchesData(repeater, currentData);
            });
        }

        // Scenario: a fully realized 5-item repeater with a NonVirtualizingLayout over an ObservableCollection. Clear() (Reset) and
        //           RemoveAt(0) make the repeater clear its realized elements, which raises ElementClearing. The first handler
        //           call replaces ItemsSource with a new collection and calls UpdateLayout(), which realizes the new elements.
        // Expected: no ElementClearing after the handler returned (the rest of the old change does not apply to the elements
        //           realized for the new collection) and, after one more layout, every realized element matches the new
        //           collection (TryGetElement(i) shows item i, GetElementIndex round-trips, visual order = data order).
        // A failure means: the repeater keeps clearing or re-indexing the elements it realized for the new collection, the
        //                  same class of bug as GitHub #11570.
        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource and updating layout from an ElementClearing handler during a Reset does not clear the elements realized for the new collection.")]
        public void VerifyItemsSourceReplacedAndLayoutUpdatedInElementClearingDuringReset()
        {
            VerifyItemsSourceReplacedAndLayoutUpdatedInElementClearing("Clear()", data => data.Clear());
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing ItemsSource and updating layout from an ElementClearing handler during a Remove leaves the repeater consistent with the new collection.")]
        public void VerifyItemsSourceReplacedAndLayoutUpdatedInElementClearingDuringRemove()
        {
            VerifyItemsSourceReplacedAndLayoutUpdatedInElementClearing("RemoveAt(0)", data => data.RemoveAt(0));
        }

        private void VerifyItemsSourceReplacedAndLayoutUpdatedInElementClearing(string operation, Action<ObservableCollection<string>> change)
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var newData = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "New #" + i));
            var repeater = CreateLoadedRepeater(data, () => new NonVirtualStackLayout());

            RunOnUIThread.Execute(() =>
            {
                var afterHandler = new List<string>();
                int clearing = 0;
                bool replaced = false;
                bool handlerReturned = false;
                repeater.ElementClearing += (sender, args) =>
                {
                    clearing++;
                    string description = $"ElementClearing #{clearing} '{GetElementContent(args.Element)}' index {sender.GetElementIndex(args.Element)}";
                    Log.Comment(description);
                    if (handlerReturned)
                    {
                        afterHandler.Add(description);
                    }
                    if (!replaced)
                    {
                        replaced = true;
                        Log.Comment("Handler: ItemsSource = new collection");
                        sender.ItemsSource = newData;
                        Log.Comment("Handler: UpdateLayout()");
                        sender.UpdateLayout();
                        Log.Comment($"Handler: UpdateLayout() returned, {CountRealizedChildren(sender)} realized children");
                        handlerReturned = true;
                    }
                };

                Log.Comment(operation);
                change(data);
                Log.Comment($"{operation} returned, {clearing} ElementClearing");
                Verify.AreEqual(0, afterHandler.Count, "No ElementClearing after the handler replaced the state: " + string.Join(" | ", afterHandler));
                repeater.UpdateLayout();
                VerifyRealizedElementsMatchData(repeater, newData, expectAllRealized: true, context: "After layout");
                VerifyVisualOrderMatchesData(repeater, newData);
            });
        }

        private ItemsRepeater CreateLoadedRepeater(object itemsSource, Func<Layout> createLayout = null, string itemContent = MoveItemTemplate)
        {
            ItemsRepeater repeater = null;
            RunOnUIThread.Execute(() =>
            {
                repeater = new ItemsRepeater()
                {
                    ItemsSource = itemsSource,
                    ItemTemplate = XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" + itemContent + @"</DataTemplate>"),
                    VerticalAlignment = VerticalAlignment.Top
                };
                if (createLayout != null)
                {
                    repeater.Layout = createLayout();
                }
                Content = repeater;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            return repeater;
        }

        private static string GetElementContent(UIElement element)
        {
            if (element is ContentControl contentControl)
            {
                return contentControl.Content?.ToString();
            }
            if (element is TextBlock textBlock)
            {
                return textBlock.Text;
            }
            return element == null ? "{null}" : element.GetType().Name;
        }

        private static int CountRealizedChildren(ItemsRepeater repeater)
        {
            int count = 0;
            for (int c = 0; c < VisualTreeHelper.GetChildrenCount(repeater); c++)
            {
                if (repeater.GetElementIndex((UIElement)VisualTreeHelper.GetChild(repeater, c)) >= 0)
                {
                    count++;
                }
            }
            return count;
        }

        // Verifies that for every realized child, GetElementIndex(child) = k implies that the child shows data[k] and that
        // TryGetElement(k) is the child, and that for every index i, TryGetElement(i) (when not null) shows data[i].
        private static void VerifyRealizedElementsMatchData(ItemsRepeater repeater, IList<string> data, bool expectAllRealized, string context)
        {
            var children = Enumerable.Range(0, VisualTreeHelper.GetChildrenCount(repeater)).Select(c => (UIElement)VisualTreeHelper.GetChild(repeater, c)).ToList();
            Log.Comment($"{context}: data [{string.Join(", ", data)}]");
            Log.Comment($"{context}: children [{string.Join(", ", children.Select(child => GetElementContent(child) + "@" + repeater.GetElementIndex(child)))}]");

            var seen = new HashSet<int>();
            foreach (var child in children)
            {
                int index = repeater.GetElementIndex(child);
                if (index < 0)
                {
                    continue;
                }

                Verify.IsLessThan(index, data.Count, $"{context}: child '{GetElementContent(child)}' reports an index inside the data.");
                Verify.IsTrue(seen.Add(index), $"{context}: only one realized child reports index {index}.");
                Verify.AreEqual(data[index], GetElementContent(child), $"{context}: the child that reports index {index} shows item {index}.");
                Verify.AreSame(child, repeater.TryGetElement(index), $"{context}: TryGetElement({index}) is the child that reports index {index}.");
            }

            for (int i = 0; i < data.Count; i++)
            {
                var element = repeater.TryGetElement(i);
                if (element == null)
                {
                    if (expectAllRealized)
                    {
                        Verify.Fail($"{context}: TryGetElement({i}) is null although every item is realized.");
                    }
                    continue;
                }

                Verify.AreEqual(data[i], GetElementContent(element), $"{context}: TryGetElement({i}) shows item {i}.");
                Verify.AreEqual(i, repeater.GetElementIndex(element), $"{context}: GetElementIndex(TryGetElement({i})).");
            }
        }

        private static void VerifyVisualOrderMatchesData(ItemsRepeater repeater, IList<string> data)
        {
            var realized = Enumerable.Range(0, VisualTreeHelper.GetChildrenCount(repeater))
                .Select(c => (UIElement)VisualTreeHelper.GetChild(repeater, c))
                .Where(child => repeater.GetElementIndex(child) >= 0)
                .OrderBy(child => child.ActualOffset.Y)
                .Select(child => GetElementContent(child))
                .ToList();
            Log.Comment($"Visual order: [{string.Join(", ", realized)}]");
            Verify.AreEqual(string.Join(", ", data), string.Join(", ", realized), "The items are shown in data order.");
        }

        #endregion

        private int DefaultWaitTime = 2000;
    }
}
