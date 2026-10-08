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

        private const int E_FAIL = unchecked((int)0x80004005);

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
        }

        private int DefaultWaitTime = 2000;
    }
}
