// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;
using MUXControlsTestApp.Utilities;
using System;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using System.Threading;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class FlowLayoutCollectionChangeTests : ApiTestBase
    {
        [TestMethod]
        public void ValidateInserts()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList());
                var repeater = SetupRepeater(dataSource);

                Log.Comment("Insert in realized range: Inserting 3 items at index 4");
                dataSource.Insert(index: 4, count: 3, reset: false);
                repeater.UpdateLayout();

                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.IsGreaterThan(realized, 0);

                Log.Comment("Insert before realized range: Inserting 3 items at index 0");
                dataSource.Insert(index: 0, count: 3, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.IsGreaterThan(realized, 0);

                Log.Comment("Insert after realized range: Inserting 3 items at index 10");
                dataSource.Insert(index: 10, count: 3, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.IsGreaterThan(realized, 0);
            });
        }

        [TestMethod]
        public void ValidateSentinelsDuringInserts()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList());
                int getElementCallCount = 0;
                int recycleElementCallCount = 0;

                var elementFactory = new RecyclingElementFactoryDerived()
                {
                    Templates = { { "key", SharedHelpers.GetDataTemplate("<TextBlock Text='{Binding}' Height='100' />") } },
                    RecyclePool = new RecyclePool(),
                    GetElementFunc = (int index, UIElement owner, UIElement elementFromBase) =>
                    {
                        getElementCallCount++;
                        return elementFromBase;
                    },
                    ClearElementFunc = (UIElement element, UIElement owner) => recycleElementCallCount++,
                    ValidateElementIndices = false
                };

                var repeater = SetupRepeater(dataSource, elementFactory);

                getElementCallCount = 0;
                recycleElementCallCount = 0;
                Log.Comment("Insert in realized range: Inserting 100 items at index 1");
                dataSource.Insert(index: 1, count: 100, reset: false);
                repeater.UpdateLayout();

                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.IsGreaterThan(realized, 0);

                Verify.IsLessThan(getElementCallCount, 6);
                Verify.IsLessThan(recycleElementCallCount, 6);
            });
        }

        [TestMethod]
        public void CanRemoveItemsStartingBeforeRealizedRange()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, 20).ToList()));
            ScrollViewer scrollViewer = null;
            ItemsRepeater repeater = null;
            var viewChangedEvent = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                repeater = SetupRepeater(dataSource, ref scrollViewer);
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChangedEvent.Set();
                    }
                };

                scrollViewer.ChangeView(null, 600, null, true);
            });

            Verify.IsTrue(viewChangedEvent.WaitOne(DefaultWaitTime), "Waiting for ViewChanged.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Remove before realized range: start:(0)beforeView end:(1)beforeView.");
                dataSource.Remove(index: 0, count: 2, reset: false);
                repeater.UpdateLayout();

                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(4, realized);

                Log.Comment("Remove before realized range: start:(0)beforeView end:(4)inview.");
                dataSource.Remove(index: 0, count: 5, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);

                Log.Comment("Insert before realized range: Inserting 10 items at index 0");
                dataSource.Insert(index: 0, count: 10, reset: false);
                repeater.UpdateLayout();
                VerifyRealizedRange(repeater, dataSource);

                Log.Comment("Insert after realized range: Inserting 10 items at index 19");
                dataSource.Insert(index: 19, count: 10, reset: false);
                repeater.UpdateLayout();
                VerifyRealizedRange(repeater, dataSource);

                Log.Comment("Remove before realized range: start:(5)beforeView end:(25)afterView.");
                dataSource.Remove(index: 5, count: 20, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.IsLessThanOrEqual(2, realized);
            });
        }

        [TestMethod]
        public void CanRemoveItemsStartingInRealizedRange()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList());
                var repeater = SetupRepeater(dataSource);

                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);

                Log.Comment("Remove in realized range: start:(1)InView end:(3)InView.");
                dataSource.Remove(index: 1, count: 3, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);

                Log.Comment("Remove in realized range: start:(1)InView end:(6)InView.");
                dataSource.Remove(index: 1, count: 6, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(1, realized);
            });
        }

        [TestMethod]
        [TestProperty("Bug", "19259478")]
        public void CanRemoveAndInsertItemsInRealizedRange()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new CustomItemsSource(Enumerable.Range(0, 3).ToList());
                var repeater = SetupRepeater(dataSource);

                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);

                Log.Comment("Remove index 1 in realized range");
                dataSource.Remove(index: 1, count: 1, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(2, realized);

                Log.Comment("Add 5 items at index 1");
                dataSource.Insert(index: 1, count: 5, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);
            });
        }

        [TestMethod]
        public void CanRemoveItemsAfterRealizedRange()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList());
                var repeater = SetupRepeater(dataSource);

                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);

                Log.Comment("Remove after realized range: start:(8)afterView end:(9)afterView.");
                dataSource.Remove(index: 8, count: 2, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);
            });
        }

        [TestMethod]
        public void CanReplaceSingleItem()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList()));
            ScrollViewer scrollViewer = null;
            ItemsRepeater repeater = null;
            var viewChangedEvent = new ManualResetEvent(false);
            int elementsCleared = 0;
            int elementsPrepared = 0;

            RunOnUIThread.Execute(() =>
            {
                repeater = SetupRepeater(dataSource, ref scrollViewer);
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if(!args.IsIntermediate)
                    {
                        viewChangedEvent.Set();
                    }
                };

                repeater.ElementPrepared += (sender, args) => { elementsPrepared++; };
                repeater.ElementClearing += (sender, args) => { elementsCleared++; };

                scrollViewer.ChangeView(null, 200, null, true);
            });

            Verify.IsTrue(viewChangedEvent.WaitOne(DefaultWaitTime), "Waiting for ViewChanged.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(4, realized);

                Log.Comment("Replace before realized range.");
                dataSource.Replace(index: 0, oldCount: 1, newCount: 1, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(4, realized);

                Log.Comment("Replace in realized range.");
                elementsPrepared = 0;
                elementsCleared = 0;
                dataSource.Replace(index: 2, oldCount: 1, newCount: 1, reset: false);
                repeater.UpdateLayout();
                Verify.AreEqual(1, elementsPrepared);
                Verify.AreEqual(1, elementsCleared);

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(4, realized);

                Log.Comment("Replace after realized range");
                dataSource.Replace(index: 8, oldCount: 1, newCount: 1, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(4, realized);
            });
        }

        [TestMethod]
        public void CanMoveItem()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList()));
            ScrollViewer scrollViewer = null;
            ItemsRepeater repeater = null;
            var viewChangedEvent = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                repeater = SetupRepeater(dataSource, ref scrollViewer);
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChangedEvent.Set();
                    }
                };
                scrollViewer.ChangeView(null, 400, null, true);
            });

            Verify.IsTrue(viewChangedEvent.WaitOne(DefaultWaitTime), "Waiting for ViewChanged.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(4, realized);

                Log.Comment("Move before realized range.");
                dataSource.Move(oldIndex: 0, newIndex: 1, count: 2, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(4, realized);

                Log.Comment("Move in realized range.");
                dataSource.Move(oldIndex: 3, newIndex: 5, count: 2, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(4, realized);

                Log.Comment("Move after realized range");
                dataSource.Move(oldIndex: 7, newIndex: 8, count: 2, reset: false);
                repeater.UpdateLayout();

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(4, realized);
            });
        }

        [TestMethod]
        public void VerifyElement0OwnershipInUniformGridLayout()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(new List<int>()));
            ItemsRepeater repeater = null;
            int elementsCleared = 0;
            int elementsPrepared = 0;

            RunOnUIThread.Execute(() =>
            {
                repeater = SetupRepeater(dataSource, new UniformGridLayout());
                repeater.ElementPrepared += (sender, args) => { elementsPrepared++; };
                repeater.ElementClearing += (sender, args) => { elementsCleared++; };
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Add two item");
                dataSource.Insert(index: 0, count: 1, reset: false);
                dataSource.Insert(index: 1, count: 1, reset: false);
                repeater.UpdateLayout();
                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(2, realized);

                Log.Comment("replace the first item");
                dataSource.Replace(index: 0, oldCount: 1, newCount: 1, reset: false);
                repeater.UpdateLayout();
                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(2, realized);

                Log.Comment("Remove two items");
                dataSource.Remove(index: 1, count: 1, reset: false);
                dataSource.Remove(index: 0, count: 1, reset:false);
                repeater.UpdateLayout();
                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(0, realized);

                Verify.AreEqual(elementsPrepared, elementsCleared);
            });
        }

        [TestMethod]
        public void EnsureReplaceOfAnchorDoesNotResetAllContainers()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList()));
            ScrollViewer scrollViewer = null;
            ItemsRepeater repeater = null;
            var viewChangedEvent = new ManualResetEvent(false);
            int elementsCleared = 0;
            int elementsPrepared = 0;

            RunOnUIThread.Execute(() =>
            {
                repeater = SetupRepeater(dataSource, ref scrollViewer);
                repeater.ElementPrepared += (sender, args) => { elementsPrepared++; };
                repeater.ElementClearing += (sender, args) => { elementsCleared++; };
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);

                Log.Comment("Replace anchor element 0");
                elementsPrepared = 0;
                elementsCleared = 0;
                dataSource.Replace(index: 0, oldCount: 1, newCount: 1, reset: false);
                repeater.UpdateLayout();
                Verify.AreEqual(1, elementsPrepared);
                Verify.AreEqual(1, elementsCleared);

                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Replace notifications whose old and new item counts differ shift the indexes of the following realized elements and keep the element-to-data mapping correct.")]
        public void ReplaceMultipleItems()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList());
                var repeater = SetupRepeater(dataSource);
                var indexChanges = new List<Tuple<int, int>>();
                repeater.ElementIndexChanged += (sender, args) => indexChanges.Add(Tuple.Create(args.OldIndex, args.NewIndex));

                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);

                Log.Comment("Replace 1 item with 3 items at index 0, in the realized range: realized items 1 and 2 move to 3 and 4.");
                dataSource.Replace(index: 0, oldCount: 1, newCount: 3, reset: false);
                LogIndexChanges(indexChanges);
                Verify.IsTrue(indexChanges.Contains(Tuple.Create(1, 3)));
                Verify.IsTrue(indexChanges.Contains(Tuple.Create(2, 4)));
                Verify.AreEqual(2, indexChanges.Count);
                repeater.UpdateLayout();
                Verify.AreEqual(12, dataSource.Inner.Count);
                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);

                Log.Comment("Replace 2 items with 1 item at index 0, in the realized range: realized item 2 moves to 1.");
                indexChanges.Clear();
                dataSource.Replace(index: 0, oldCount: 2, newCount: 1, reset: false);
                LogIndexChanges(indexChanges);
                Verify.AreEqual(1, indexChanges.Count);
                Verify.AreEqual(Tuple.Create(2, 1), indexChanges[0]);
                repeater.UpdateLayout();
                Verify.AreEqual(11, dataSource.Inner.Count);
                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);

                Log.Comment("Replace 1 item with 4 items after the realized range: realized elements keep their indexes.");
                indexChanges.Clear();
                var before = Enumerable.Range(0, 3).Select(i => repeater.TryGetElement(i)).ToList();
                dataSource.Replace(index: 8, oldCount: 1, newCount: 4, reset: false);
                Verify.AreEqual(0, indexChanges.Count);
                repeater.UpdateLayout();
                Verify.AreEqual(14, dataSource.Inner.Count);
                for (int i = 0; i < 3; i++)
                {
                    Verify.AreSame(before[i], repeater.TryGetElement(i));
                }
                realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);
            });
        }

        private static void LogIndexChanges(List<Tuple<int, int>> indexChanges)
        {
            Log.Comment("ElementIndexChanged: " + string.Join(",", indexChanges.Select(c => $"{c.Item1}->{c.Item2}")));
        }

        [TestMethod]

        [TestProperty("Ignore", "True")] // Product concern PC-GRID-ANCHOR-INSERT (bug pending): after an insert before a scrolled anchor, UniformGridLayout places items outside their grid cells.
        [TestProperty("Description", "Verifies a scrolled UniformGridLayout keeps every realized item in its grid cell after an insert at index 0 shifts the scroll anchor away from the start of its row.")]
        public void ValidateGridPlacementAfterInsertBeforeScrolledAnchor()
        {
            const int columns = 3;
            const double cellSize = 100.0;
            var dataSource = (CustomItemsSource)null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, 60).ToList()));
            ScrollViewer scrollViewer = null;
            ItemsRepeater repeater = null;
            var viewChangedEvent = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                var elementFactory = new RecyclingElementFactory() { RecyclePool = new RecyclePool() };
                elementFactory.Templates["Item"] = SharedHelpers.GetDataTemplate(@"<TextBlock Text='{Binding}' Width='100' Height='100' />");
                repeater = new ItemsRepeater() {
                    ItemsSource = dataSource,
                    ItemTemplate = elementFactory,
                    Layout = new UniformGridLayout() { MinItemWidth = cellSize, MinItemHeight = cellSize, MaximumRowsOrColumns = columns },
                    VerticalCacheLength = 0,
                    HorizontalCacheLength = 0
                };
                scrollViewer = new ScrollViewer() {
                    Content = repeater,
                    Width = columns * cellSize,
                    Height = 300,
                    VerticalAnchorRatio = 0.0
                };
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChangedEvent.Set();
                    }
                };
                Content = scrollViewer;
                Content.UpdateLayout();
                scrollViewer.ChangeView(null, 1050, null, disableAnimation: true);
            });

            Verify.IsTrue(viewChangedEvent.WaitOne(DefaultWaitTime), "Waiting for ViewChanged.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(repeater.TryGetElement(0), "Item 0 is scrolled out of the realized range.");
                VerifyGridPlacement(repeater, dataSource, columns, cellSize);

                Log.Comment("Insert one item at index 0.");
                dataSource.Insert(index: 0, count: 1, reset: false);
                repeater.UpdateLayout();
            });

            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => repeater.UpdateLayout());
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"VerticalOffset after insert: {scrollViewer.VerticalOffset}");
                int realized = VerifyGridPlacement(repeater, dataSource, columns, cellSize);
                Verify.IsGreaterThanOrEqual(realized, columns * 3, "At least the visible rows are realized.");
            });
        }

        private int VerifyGridPlacement(ItemsRepeater repeater, CustomItemsSource dataSource, int columns, double cellSize)
        {
            int realized = 0;
            int first = -1;
            int last = -1;
            for (int i = 0; i < dataSource.Inner.Count; i++)
            {
                var element = repeater.TryGetElement(i) as TextBlock;
                if (element == null)
                {
                    continue;
                }

                realized++;
                first = first == -1 ? i : first;
                last = i;
                Verify.AreEqual(dataSource.GetAt(i).ToString(), element.Text, $"Item {i} data");
                var offset = element.TransformToVisual(repeater).TransformPoint(new global::Windows.Foundation.Point(0, 0));
                Log.Comment($"Item {i} '{element.Text}' at ({offset.X}, {offset.Y})");
                Verify.AreEqual((i % columns) * cellSize, offset.X, $"Item {i} X");
                Verify.AreEqual((i / columns) * cellSize, offset.Y, $"Item {i} Y");
            }

            Log.Comment($"Realized range: [{first}, {last}], count {realized}");
            Verify.AreEqual(last - first + 1, realized, "The realized range is contiguous.");
            return realized;
        }

        [TestMethod]
        public void ValidateStableResets()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new CustomItemsSourceWithUniqueId(Enumerable.Range(0, 3).ToList());
                var repeater = SetupRepeater(dataSource);

                Log.Comment("Reset collection");
                dataSource.GetAtCallCount = 0;
                dataSource.Reset();
                repeater.UpdateLayout();

                // Make sure data was not requested because during a stable reset
                // the elements and associated bound data are reused
                Verify.AreEqual(0, dataSource.GetAtCallCount);
                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);
            });
        }

        [TestMethod]
        public void ValidateRegularResets()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new CustomItemsSource(Enumerable.Range(0, 3).ToList());
                var repeater = SetupRepeater(dataSource);

                Log.Comment("Reset collection");
                dataSource.GetAtCallCount = 0;
                dataSource.Reset();
                repeater.UpdateLayout();

                // Make sure data was requested because during a normal reset elements (already bound with data) are not reused.
                Verify.AreEqual(4, dataSource.GetAtCallCount);
                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(3, realized);
            });
        }

        [TestMethod]
        public void ValidateClear()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new CustomItemsSource(Enumerable.Range(0, 3).ToList());
                var repeater = SetupRepeater(dataSource, new FlowLayout());

                Log.Comment("Clear the collection");
                dataSource.Clear();
                repeater.UpdateLayout();

                var realized = VerifyRealizedRange(repeater, dataSource);
                Verify.AreEqual(0, realized);
            });
        }

        private ItemsRepeater SetupRepeater(CustomItemsSource dataSource)
        {
            ScrollViewer sv = null;
            return SetupRepeater(dataSource, GetElementFactory(), ref sv, new StackLayout());
        }

        private ItemsRepeater SetupRepeater(CustomItemsSource dataSource, VirtualizingLayout layout)
        {
            ScrollViewer sv = null;
            return SetupRepeater(dataSource, GetElementFactory(), ref sv, layout);
        }

        private ItemsRepeater SetupRepeater(CustomItemsSource dataSource, ElementFactory elementFactory = null)
        {
            ScrollViewer sv = null;

            return SetupRepeater(dataSource, elementFactory, ref sv, new StackLayout());
        }

        private ItemsRepeater SetupRepeater(CustomItemsSource dataSource, ref ScrollViewer scrollViewer)
        {
            return SetupRepeater(dataSource, GetElementFactory(), ref scrollViewer, new StackLayout());
        }

        private RecyclingElementFactory GetElementFactory()
        {
            var elementFactory = new RecyclingElementFactory();
            elementFactory.RecyclePool = new RecyclePool();
            elementFactory.Templates["Item"] = SharedHelpers.GetDataTemplate(@"<TextBlock Text='{Binding}' Height='100' />");
            return elementFactory;
        }

        private ItemsRepeater SetupRepeater(CustomItemsSource dataSource, ElementFactory elementFactory, ref ScrollViewer scrollViewer, VirtualizingLayout layout)
        {
            var repeater = new ItemsRepeater()
            {
                ItemsSource = dataSource,
                ItemTemplate = elementFactory,
                Layout = layout,
                VerticalCacheLength = 0,
                HorizontalCacheLength = 0
            };

            scrollViewer = new ScrollViewer
            {
                Content = repeater
            };

            Content = new ItemsRepeaterScrollHost()
            {
                Width = 200,
                Height = 200,
                ScrollViewer = scrollViewer
            };

            Content.UpdateLayout();
            if (dataSource.Count > 0)
            {
                int realized = VerifyRealizedRange(repeater, dataSource);
                Verify.IsGreaterThan(realized, 0);
            }

            return repeater;
        }
        private int VerifyRealizedRange(ItemsRepeater repeater, CustomItemsSource dataSource)
        {
            int numRealized = 0;
            // trace
            Log.Comment("index:ItemsSourceView:Item");
            for (int i = 0; i < dataSource.Inner.Count; i++)
            {
                var element = repeater.TryGetElement(i);
                if (element != null)
                {
                    var data = ((TextBlock)element).Text;
                    Log.Comment("{0} {1}:[{2}]", i, dataSource.GetAt(i), data);
                    numRealized++;
                }
                else
                {
                    Log.Comment(i + ":[null]");
                }
            }

            // verify
            for (int i = 0; i < dataSource.Inner.Count; i++)
            {
                var element = repeater.TryGetElement(i);
                if (element != null)
                {
                    var data = ((TextBlock)element).Text;
                    Verify.AreEqual(dataSource.GetAt(i).ToString(), data);
                }
            }

            return numRealized;
        }

        private int DefaultWaitTime = 2000; 
    }
}
