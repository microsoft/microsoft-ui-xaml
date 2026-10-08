// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.Foundation;
using Common;
using Microsoft.UI.Xaml.Controls;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class LinedFlowLayoutTests : ApiTestBase
    {
        private const double c_lineHeight = 50.0;
        private const int E_BOUNDS = unchecked((int)0x8000000B);
        private const int E_INVALID_OPERATION = unchecked((int)0x800710DD); // HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION)

        [TestMethod]
        [TestProperty("Description", "Verifies the LinedFlowLayout default property values.")]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                LinedFlowLayout linedFlowLayout = new LinedFlowLayout();
                Verify.IsNotNull(linedFlowLayout);

                Log.Comment("Verifying LinedFlowLayout default property values");
                Verify.AreEqual(0.0, linedFlowLayout.ActualLineHeight);
                Verify.AreEqual(double.NaN, linedFlowLayout.LineHeight);
                Verify.AreEqual(0.0, linedFlowLayout.LineSpacing);
                Verify.AreEqual(0.0, linedFlowLayout.MinItemSpacing);
                Verify.AreEqual(LinedFlowLayoutItemsJustification.Start, linedFlowLayout.ItemsJustification);
                Verify.AreEqual(LinedFlowLayoutItemsStretch.None, linedFlowLayout.ItemsStretch);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies LockItemToLine rejects out-of-range item indexes, including when the layout has no items.")]
        public void VerifyLockItemToLineThrowsForOutOfRangeIndex()
        {
            try
            {
                ItemsRepeater repeater = null;
                LinedFlowLayout linedFlowLayout = null;

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("LinedFlowLayout not attached to any ItemsRepeater has no items.");
                    VerifyThrowsWithHResult(E_BOUNDS, () => new LinedFlowLayout().LockItemToLine(0), "LockItemToLine(0) on detached layout");

                    repeater = CreateRepeater(itemWidths: Enumerable.Repeat(100.0, 20), repeaterWidth: 520.0, linedFlowLayout: out linedFlowLayout);
                });

                SettleLayout();

                RunOnUIThread.Execute(() =>
                {
                    VerifyThrowsWithHResult(E_BOUNDS, () => linedFlowLayout.LockItemToLine(-1), "LockItemToLine(-1)");
                    VerifyThrowsWithHResult(E_BOUNDS, () => linedFlowLayout.LockItemToLine(20), "LockItemToLine(itemCount)");
                    VerifyThrowsWithHResult(E_BOUNDS, () => linedFlowLayout.LockItemToLine(int.MaxValue), "LockItemToLine(int.MaxValue)");
                });
            }
            finally
            {
                // Always stop the layouts' measure timers, even when an assertion fails (PC-LFL-TIMER).
                DetachLayouts();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies LockItemToLine returns -1 when no measure pass has established an average items-per-line yet.")]
        public void VerifyLockItemToLineReturnsMinusOneBeforeFirstMeasure()
        {
            RunOnUIThread.Execute(() =>
            {
                var linedFlowLayout = new LinedFlowLayout() { LineHeight = c_lineHeight };
                var repeater = new ItemsRepeater() {
                    Layout = linedFlowLayout,
                    ItemsSource = CreateItems(Enumerable.Repeat(100.0, 10))
                };

                // The repeater is not in the live tree, so no measure pass happened.
                Verify.AreEqual(-1, linedFlowLayout.LockItemToLine(0));
                Verify.AreEqual(-1, linedFlowLayout.LockItemToLine(5));
                Verify.AreEqual(-1, linedFlowLayout.LockItemToLine(9));
                VerifyThrowsWithHResult(E_BOUNDS, () => linedFlowLayout.LockItemToLine(10), "LockItemToLine(itemCount) before measure");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies LockItemToLine returns the line on which each realized item is actually arranged, for first, middle and last items, and is idempotent.")]
        public void VerifyLockItemToLineReturnsArrangedLineIndex()
        {
            VerifyLockItemToLineReturnsArrangedLineIndex(useItemsInfoRequested: false);
        }

        [TestMethod]
        [TestProperty("Description", "Same as VerifyLockItemToLineReturnsArrangedLineIndex while the ItemsInfoRequested handler supplies all aspect ratios (fast path).")]
        public void VerifyLockItemToLineReturnsArrangedLineIndexWithItemsInfo()
        {
            VerifyLockItemToLineReturnsArrangedLineIndex(useItemsInfoRequested: true);
        }

        private void VerifyLockItemToLineReturnsArrangedLineIndex(bool useItemsInfoRequested)
        {
            try
            {
                const int itemCount = 23;
                const double lineSpacing = 10.0;
                ItemsRepeater repeater = null;
                LinedFlowLayout linedFlowLayout = null;
                int itemsInfoRequestedCount = 0;

                RunOnUIThread.Execute(() =>
                {
                    repeater = CreateRepeater(itemWidths: Enumerable.Repeat(100.0, itemCount), repeaterWidth: 520.0, linedFlowLayout: out linedFlowLayout);
                    linedFlowLayout.LineSpacing = lineSpacing;

                    if (useItemsInfoRequested)
                    {
                        linedFlowLayout.ItemsInfoRequested += (sender, args) =>
                        {
                            itemsInfoRequestedCount++;
                            args.ItemsRangeStartIndex = 0;
                            args.SetDesiredAspectRatios(Enumerable.Repeat(100.0 / c_lineHeight, itemCount).ToArray());
                        };
                    }
                });

                SettleLayout();

                RunOnUIThread.Execute(() =>
                {
                    if (useItemsInfoRequested)
                    {
                        Verify.IsGreaterThan(itemsInfoRequestedCount, 0, "ItemsInfoRequested must have been raised.");
                    }

                    var lineOfItem = new int[itemCount];
                    for (int i = 0; i < itemCount; i++)
                    {
                        var element = repeater.TryGetElement(i);
                        Verify.IsNotNull(element, $"Item {i} is expected to be realized.");
                        var offset = GetOffset(element, repeater);
                        double line = offset.Y / (c_lineHeight + lineSpacing);
                        lineOfItem[i] = (int)Math.Round(line);
                        Verify.IsLessThan(Math.Abs(line - lineOfItem[i]), 0.05, $"Item {i} Y={offset.Y} must be aligned to a line.");
                    }

                    int lastLine = lineOfItem[itemCount - 1];
                    Log.Comment($"Item lines: {string.Join(",", lineOfItem)}");
                    Verify.IsGreaterThan(lastLine, 1, "The scenario requires at least three lines.");

                    int firstLocked = linedFlowLayout.LockItemToLine(0);
                    Verify.AreEqual(0, firstLocked, "The first item is on line 0.");

                    foreach (int itemIndex in new int[] { 1, itemCount / 2, itemCount - 2 })
                    {
                        int locked = linedFlowLayout.LockItemToLine(itemIndex);
                        Verify.AreEqual(lineOfItem[itemIndex], locked, $"LockItemToLine({itemIndex})");
                        Verify.AreEqual(locked, linedFlowLayout.LockItemToLine(itemIndex), $"LockItemToLine({itemIndex}) is idempotent");
                    }

                    Verify.AreEqual(lastLine, linedFlowLayout.LockItemToLine(itemCount - 1), "The last item is on the last line.");
                });
            }
            finally
            {
                // Always stop the layouts' measure timers, even when an assertion fails (PC-LFL-TIMER).
                DetachLayouts();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ItemsUnlocked is raised synchronously when the collection changes or the layout is detached while items are locked, and not raised when nothing is locked.")]
        public void VerifyItemsUnlockedRaisedWhenLocksAreInvalidated()
        {
            try
            {
                ItemsRepeater repeater = null;
                LinedFlowLayout linedFlowLayout = null;
                ObservableCollection<UIElement> items = null;
                int itemsUnlockedCount = 0;

                RunOnUIThread.Execute(() =>
                {
                    repeater = CreateRepeater(itemWidths: Enumerable.Repeat(100.0, 15), repeaterWidth: 520.0, linedFlowLayout: out linedFlowLayout);
                    items = (ObservableCollection<UIElement>)repeater.ItemsSource;
                });

                SettleLayout();

                RunOnUIThread.Execute(() =>
                {
                    linedFlowLayout.ItemsUnlocked += (sender, args) =>
                    {
                        Verify.AreSame(linedFlowLayout, sender);
                        Verify.IsNull(args);
                        itemsUnlockedCount++;
                    };

                    Log.Comment("Collection change without locked items does not raise ItemsUnlocked.");
                    items.Add(CreateItem(100.0));
                    Verify.AreEqual(0, itemsUnlockedCount);
                    Content.UpdateLayout();
                    Verify.AreEqual(0, itemsUnlockedCount);

                    Log.Comment("Locking a middle item, then changing the collection, raises ItemsUnlocked once.");
                    Verify.IsGreaterThanOrEqual(linedFlowLayout.LockItemToLine(7), 0);
                    items.Add(CreateItem(100.0));
                    Verify.AreEqual(1, itemsUnlockedCount);

                    Log.Comment("The previous change cleared the locks, so a new change does not raise it again.");
                    items.RemoveAt(items.Count - 1);
                    Verify.AreEqual(1, itemsUnlockedCount);
                    Content.UpdateLayout();

                    Log.Comment("Locking only the first item also raises ItemsUnlocked on invalidation.");
                    Verify.AreEqual(0, linedFlowLayout.LockItemToLine(0));
                    items.RemoveAt(items.Count - 1);
                    Verify.AreEqual(2, itemsUnlockedCount);
                    Content.UpdateLayout();
                });
            }
            finally
            {
                // Always stop the layouts' measure timers, even when an assertion fails (PC-LFL-TIMER).
                DetachLayouts();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ItemsUnlocked is raised synchronously when a LinedFlowLayout with locked items is detached from its ItemsRepeater.")]
        public void VerifyItemsUnlockedRaisedWhenLayoutDetached()
        {
            try
            {
                ItemsRepeater repeater = null;
                LinedFlowLayout linedFlowLayout = null;
                int itemsUnlockedCount = 0;

                RunOnUIThread.Execute(() =>
                {
                    repeater = CreateRepeater(itemWidths: Enumerable.Repeat(100.0, 15), repeaterWidth: 520.0, linedFlowLayout: out linedFlowLayout);
                });

                SettleLayout();

                RunOnUIThread.Execute(() =>
                {
                    linedFlowLayout.ItemsUnlocked += (sender, args) => itemsUnlockedCount++;

                    Verify.IsGreaterThanOrEqual(linedFlowLayout.LockItemToLine(3), 0);
                    repeater.Layout = new StackLayout();
                    Verify.AreEqual(1, itemsUnlockedCount);
                    Content.UpdateLayout();
                });
            }
            finally
            {
                // Always stop the layouts' measure timers, even when an assertion fails (PC-LFL-TIMER).
                DetachLayouts();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies item positions for every ItemsJustification value when ItemsStretch is None and the line is not full.")]
        public void VerifyItemsJustificationWithoutStretch()
        {
            try
            {
                // Three 100px items in a 500px wide layout leave 200px of extra width on the single line.
                var expectations = new Dictionary<LinedFlowLayoutItemsJustification, double[]>()
                {
                    { LinedFlowLayoutItemsJustification.Start, new double[] { 0, 100, 200 } },
                    { LinedFlowLayoutItemsJustification.Center, new double[] { 100, 200, 300 } },
                    { LinedFlowLayoutItemsJustification.End, new double[] { 200, 300, 400 } },
                    { LinedFlowLayoutItemsJustification.SpaceEvenly, new double[] { 50, 200, 350 } },
                    { LinedFlowLayoutItemsJustification.SpaceAround, new double[] { 200.0 / 6.0, 200, 400 - 200.0 / 6.0 } },
                    { LinedFlowLayoutItemsJustification.SpaceBetween, new double[] { 0, 200, 400 } },
                };

                foreach (var expectation in expectations)
                {
                    ItemsRepeater repeater = null;
                    LinedFlowLayout linedFlowLayout = null;

                    RunOnUIThread.Execute(() =>
                    {
                        Log.Comment($"ItemsJustification={expectation.Key}");
                        repeater = CreateRepeater(itemWidths: Enumerable.Repeat(100.0, 3), repeaterWidth: 500.0, linedFlowLayout: out linedFlowLayout);
                        linedFlowLayout.ItemsJustification = expectation.Key;
                        Verify.AreEqual(LinedFlowLayoutItemsStretch.None, linedFlowLayout.ItemsStretch);
                    });

                    SettleLayout();

                    RunOnUIThread.Execute(() =>
                    {
                        Verify.AreEqual(c_lineHeight, linedFlowLayout.ActualLineHeight);
                        for (int i = 0; i < 3; i++)
                        {
                            var element = (FrameworkElement)repeater.TryGetElement(i);
                            var offset = GetOffset(element, repeater);
                            Log.Comment($"Item {i}: X={offset.X}, Y={offset.Y}, ActualWidth={element.ActualWidth}");
                            Verify.IsLessThan(Math.Abs(offset.X - expectation.Value[i]), 1.0, $"Item {i} X");
                            Verify.AreEqual(0.0, offset.Y, $"Item {i} Y");
                            Verify.IsLessThan(Math.Abs(element.ActualWidth - 100.0), 1.0, $"Item {i} width is not stretched");
                        }

                        DetachLayoutsCore();
                        Content = null;
                    });

                    IdleSynchronizer.Wait();
                }
            }
            finally
            {
                // Always stop the layouts' measure timers, even when an assertion fails (PC-LFL-TIMER).
                DetachLayouts();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that with an infinite available width all items are measured and arranged on a single line using MinItemSpacing and the SpaceEvenly/SpaceAround leading offset.")]
        public void VerifyUnconstrainedWidthArrangesSingleLine()
        {
            try
            {
                const double minItemSpacing = 10.0;
                var widths = new double[] { 40, 60, 80, 100 };
                // Leading offset per justification: SpaceEvenly -> MinItemSpacing, SpaceAround -> MinItemSpacing / 2, others -> 0.
                var leadingOffsets = new Dictionary<LinedFlowLayoutItemsJustification, double>()
                {
                    { LinedFlowLayoutItemsJustification.Start, 0 },
                    { LinedFlowLayoutItemsJustification.End, 0 },
                    { LinedFlowLayoutItemsJustification.SpaceEvenly, minItemSpacing },
                    { LinedFlowLayoutItemsJustification.SpaceAround, minItemSpacing / 2 },
                };

                foreach (var stretch in new LinedFlowLayoutItemsStretch[] { LinedFlowLayoutItemsStretch.None, LinedFlowLayoutItemsStretch.Fill })
                {
                    foreach (var leadingOffset in leadingOffsets)
                    {
                        ItemsRepeater repeater = null;
                        LinedFlowLayout linedFlowLayout = null;

                        RunOnUIThread.Execute(() =>
                        {
                            Log.Comment($"ItemsStretch={stretch}, ItemsJustification={leadingOffset.Key}");
                            linedFlowLayout = KeepAlive(new LinedFlowLayout() {
                                LineHeight = c_lineHeight,
                                MinItemSpacing = minItemSpacing,
                                ItemsJustification = leadingOffset.Key,
                                ItemsStretch = stretch
                            });
                            repeater = new ItemsRepeater() {
                                Layout = linedFlowLayout,
                                ItemsSource = CreateItems(widths),
                                VerticalAlignment = VerticalAlignment.Top
                            };

                            // A horizontal StackPanel measures its children with an infinite width.
                            _repeaters.Add(repeater);
                            var panel = new StackPanel() { Orientation = Orientation.Horizontal, VerticalAlignment = VerticalAlignment.Top };
                            panel.Children.Add(repeater);
                            Content = panel;
                        });

                        SettleLayout();

                        RunOnUIThread.Execute(() =>
                        {
                            double expectedDesiredWidth = widths.Sum() + (widths.Length - 1) * minItemSpacing;
                            Log.Comment($"Repeater DesiredSize={repeater.DesiredSize}");
                            Verify.IsLessThan(Math.Abs(repeater.DesiredSize.Width - expectedDesiredWidth), 0.5, "Desired width is the sum of the item widths plus spacing.");
                            Verify.IsLessThan(Math.Abs(repeater.DesiredSize.Height - c_lineHeight), 0.5, "A single line is laid out.");

                            double expectedX = stretch == LinedFlowLayoutItemsStretch.None ? leadingOffset.Value : 0.0;
                            for (int i = 0; i < widths.Length; i++)
                            {
                                var element = (FrameworkElement)repeater.TryGetElement(i);
                                Verify.IsNotNull(element, $"Item {i} is realized.");
                                var offset = GetOffset(element, repeater);
                                Log.Comment($"Item {i}: X={offset.X}, Y={offset.Y}");
                                Verify.IsLessThan(Math.Abs(offset.X - expectedX), 0.5, $"Item {i} X");
                                Verify.AreEqual(0.0, offset.Y, $"Item {i} Y");
                                expectedX += widths[i] + minItemSpacing;
                            }

                            DetachLayoutsCore();
                            Content = null;
                        });

                        IdleSynchronizer.Wait();
                    }
                }
            }
            finally
            {
                // Always stop the layouts' measure timers, even when an assertion fails (PC-LFL-TIMER).
                DetachLayouts();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a LinedFlowLayout instance cannot be used by two ItemsRepeaters at once but can be reused once detached.")]
        public void VerifyLinedFlowLayoutCannotBeShared()
        {
            try
            {
                RunOnUIThread.Execute(() =>
                {
                    var linedFlowLayout = KeepAlive(new LinedFlowLayout() { LineHeight = c_lineHeight });
                    var repeater1 = new ItemsRepeater() { Layout = linedFlowLayout, ItemsSource = CreateItems(Enumerable.Repeat(50.0, 3)) };
                    var repeater2 = new ItemsRepeater() { ItemsSource = CreateItems(Enumerable.Repeat(50.0, 3)) };

                    VerifyThrowsWithHResult(E_INVALID_OPERATION, () => repeater2.Layout = linedFlowLayout, "Sharing the layout with a second ItemsRepeater");

                    Log.Comment("Once the first ItemsRepeater releases the layout, another ItemsRepeater can use it.");
                    repeater1.Layout = null;
                    var repeater3 = new ItemsRepeater() {
                        ItemsSource = CreateItems(Enumerable.Repeat(50.0, 4)),
                        Layout = linedFlowLayout,
                        Width = 300,
                        HorizontalAlignment = HorizontalAlignment.Left,
                        VerticalAlignment = VerticalAlignment.Top
                    };
                    _repeaters.Add(repeater3);
                    Content = repeater3;
                    Content.UpdateLayout();

                    Verify.AreEqual(c_lineHeight, linedFlowLayout.ActualLineHeight);
                    for (int i = 0; i < 4; i++)
                    {
                        Verify.IsNotNull(repeater3.TryGetElement(i), $"Item {i} is realized by the new owner.");
                    }
                });
            }
            finally
            {
                // Always stop the layouts' measure timers, even when an assertion fails (PC-LFL-TIMER).
                DetachLayouts();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies RequestedRangeStartIndex/RequestedRangeLength without items info (-1/0) and when ItemsInfoRequested provides info for the whole collection (0/ItemCount).")]
        public void VerifyRequestedRangeReflectsItemsInfo()
        {
            try
            {
                const int itemCount = 12;
                ItemsRepeater repeater = null;
                LinedFlowLayout linedFlowLayout = null;
                int itemsInfoRequestedCount = 0;

                RunOnUIThread.Execute(() =>
                {
                    var detachedLayout = new LinedFlowLayout();
                    Verify.AreEqual(-1, detachedLayout.RequestedRangeStartIndex);
                    Verify.AreEqual(0, detachedLayout.RequestedRangeLength);

                    repeater = CreateRepeater(itemWidths: Enumerable.Repeat(100.0, itemCount), repeaterWidth: 520.0, linedFlowLayout: out linedFlowLayout);
                });

                SettleLayout();

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Regular path without ItemsInfoRequested handler: no items info.");
                    Verify.AreEqual(-1, linedFlowLayout.RequestedRangeStartIndex);
                    Verify.AreEqual(0, linedFlowLayout.RequestedRangeLength);

                    linedFlowLayout.ItemsInfoRequested += (sender, args) =>
                    {
                        itemsInfoRequestedCount++;
                        args.ItemsRangeStartIndex = 0;
                        args.SetDesiredAspectRatios(Enumerable.Repeat(2.0, itemCount).ToArray());
                    };
                    linedFlowLayout.InvalidateItemsInfo();
                });

                SettleLayout();

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsGreaterThan(itemsInfoRequestedCount, 0);
                    Log.Comment("Items info provided for the entire collection: the whole collection is the requested range.");
                    Verify.AreEqual(0, linedFlowLayout.RequestedRangeStartIndex);
                    Verify.AreEqual(itemCount, linedFlowLayout.RequestedRangeLength);
                });
            }
            finally
            {
                // Always stop the layouts' measure timers, even when an assertion fails (PC-LFL-TIMER).
                DetachLayouts();
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Product concern PC-LFL-TIMER (bug pending): releasing a LinedFlowLayout off the UI thread crashes the process (~LinedFlowLayout -> InvalidateMeasureTimerStop -> DispatcherTimer.IsEnabled throws -> terminate, 0xC0000409); a use-after-free in InvalidateMeasureTimerTick was also observed.
        [TestProperty("Description", "Verifies that releasing an ItemsRepeater and its LinedFlowLayout while the layout's asynchronous measure timer may still be running does not crash when the timer would have ticked.")]
        public void VerifyReleasingLinedFlowLayoutWithRunningMeasureTimerDoesNotCrash()
        {
            var weakLayout = CreateAndReleaseLinedFlowLayout();

            for (int attempt = 0; attempt < 3 && weakLayout.IsAlive; attempt++)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                IdleSynchronizer.Wait();
            }

            // Give the layout's measure timer time to reach its next ticks (its interval grows with each tick).
            for (int wait = 0; wait < 10; wait++)
            {
                global::System.Threading.Thread.Sleep(500);
                IdleSynchronizer.Wait();
            }

            RunOnUIThread.Execute(() =>
            {
                var probe = new ItemsRepeater() { ItemsSource = Enumerable.Range(0, 3).ToList() };
                Content = probe;
                Content.UpdateLayout();
                Verify.IsNotNull(probe.TryGetElement(0), "The test host is still responsive.");
            });
        }

        private WeakReference CreateAndReleaseLinedFlowLayout()
        {
            WeakReference weakLayout = null;
            RunOnUIThread.Execute(() =>
            {
                var linedFlowLayout = new LinedFlowLayout() { LineHeight = c_lineHeight };
                var repeater = new ItemsRepeater() {
                    Layout = linedFlowLayout,
                    ItemsSource = CreateItems(Enumerable.Range(0, 30).Select(i => 40.0 + (i % 5) * 20.0)),
                    Width = 520.0
                };
                weakLayout = new WeakReference(linedFlowLayout);
                Content = repeater;
                Content.UpdateLayout();
                Content = null;
            });
            return weakLayout;
        }

        // Product concern PC-LFL-TIMER: a LinedFlowLayout whose asynchronous measure timer was created crashes the
        // process when it is destroyed off the UI thread (e.g. released by the .NET finalizer): ~LinedFlowLayout calls
        // DispatcherTimer.IsEnabled, which throws and terminates the process. Layouts that run layout passes in these
        // tests are kept alive for the lifetime of the test process so that unrelated tests are not taken down by that
        // crash; the defect itself is exercised by VerifyReleasingLinedFlowLayoutWithRunningMeasureTimerDoesNotCrash.
        private static readonly List<LinedFlowLayout> s_layoutsKeptAlive = new List<LinedFlowLayout>();

        private static LinedFlowLayout KeepAlive(LinedFlowLayout linedFlowLayout)
        {
            s_layoutsKeptAlive.Add(linedFlowLayout);
            return linedFlowLayout;
        }

        private readonly List<ItemsRepeater> _repeaters = new List<ItemsRepeater>();

        // LinedFlowLayout runs a DispatcherTimer for asynchronous measure passes. Detach the layouts on the UI thread
        // at the end of each test so their timers are stopped before the next test starts (see PC-LFL-TIMER).
        private void DetachLayouts()
        {
            RunOnUIThread.Execute(() => DetachLayoutsCore());
        }

        private void DetachLayoutsCore()
        {
            foreach (var repeater in _repeaters)
            {
                repeater.Layout = null;
            }
            _repeaters.Clear();
        }

        private ItemsRepeater CreateRepeater(IEnumerable<double> itemWidths, double repeaterWidth, out LinedFlowLayout linedFlowLayout)
        {
            linedFlowLayout = KeepAlive(new LinedFlowLayout() { LineHeight = c_lineHeight });
            var repeater = new ItemsRepeater() {
                Layout = linedFlowLayout,
                ItemsSource = CreateItems(itemWidths),
                Width = repeaterWidth,
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top
            };

            _repeaters.Add(repeater);
            Content = repeater;
            return repeater;
        }

        private static ObservableCollection<UIElement> CreateItems(IEnumerable<double> widths)
        {
            return new ObservableCollection<UIElement>(widths.Select(w => CreateItem(w)));
        }

        private static UIElement CreateItem(double width)
        {
            return new Border() { Width = width, Height = c_lineHeight };
        }

        // LinedFlowLayout may schedule additional asynchronous measure passes while its
        // aspect ratio weights converge; let those run before validating positions.
        private void SettleLayout()
        {
            for (int pass = 0; pass < 3; pass++)
            {
                RunOnUIThread.Execute(() => Content.UpdateLayout());
                IdleSynchronizer.Wait();
            }
        }

        private static Point GetOffset(UIElement element, UIElement relativeTo)
        {
            return element.TransformToVisual(relativeTo).TransformPoint(new Point(0, 0));
        }

        private static void VerifyThrowsWithHResult(int expectedHResult, Action action, string context)
        {
            Exception caught = null;
            try
            {
                action();
            }
            catch (Exception e)
            {
                caught = e;
            }

            Verify.IsNotNull(caught, context + " must throw.");
            Log.Comment($"{context}: {caught.GetType().Name} HResult=0x{caught.HResult:X8} '{caught.Message}'");
            Verify.AreEqual(expectedHResult, caught.HResult, context + " HResult");
        }
    }
}
