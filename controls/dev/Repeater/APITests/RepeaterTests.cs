// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Linq;
using Windows.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Common;
using Microsoft.UI.Xaml.Media;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using System.Collections.ObjectModel;
using System.Threading;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.Mocks;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Windows.Foundation;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.HResults;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public partial class RepeaterTests : ApiTestBase
    {
        [TestMethod]
        public void ValidateElementToIndexMapping()
        {
            ItemsRepeater repeater = null;
            RunOnUIThread.Execute(() =>
            {
                var elementFactory = new RecyclingElementFactory();
                elementFactory.RecyclePool = new RecyclePool();
                elementFactory.Templates["Item"] = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                          <TextBlock Text='{Binding}' Height='50' />
                      </DataTemplate>");

                repeater = new ItemsRepeater() {
                    ItemsSource = Enumerable.Range(0, 10).Select(i => string.Format("Item #{0}", i)),
                    ItemTemplate = elementFactory,
                    // Default is StackLayout, so do not have to explicitly set.
                    // Layout = new StackLayout(),
                };

                Content = new ItemsRepeaterScrollHost() {
                    Width = 400,
                    Height = 800,
                    ScrollViewer = new ScrollViewer {
                        Content = repeater
                    }
                };

                Content.UpdateLayout();

                for (int i = 0; i < 10; i++)
                {
                    var element = repeater.TryGetElement(i);
                    Verify.IsNotNull(element);
                    Verify.AreEqual(string.Format("Item #{0}", i), ((TextBlock)element).Text);
                    Verify.AreEqual(i, repeater.GetElementIndex(element));
                }

                Verify.IsNull(repeater.TryGetElement(20));
            });
        }

        [TestMethod]
        public void ValidateRepeaterDefaults()
        {
            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater() {
                    ItemsSource = Enumerable.Range(0, 10).Select(i => string.Format("Item #{0}", i)),
                };

                Content = new ItemsRepeaterScrollHost() {
                    Width = 400,
                    Height = 800,
                    ScrollViewer = new ScrollViewer {
                        Content = repeater
                    }
                };

                Content.UpdateLayout();

                for (int i = 0; i < 10; i++)
                {
                    var element = repeater.TryGetElement(i);
                    Verify.IsNotNull(element);
                    Verify.AreEqual(string.Format("Item #{0}", i), ((TextBlock)element).Text);
                    Verify.AreEqual(i, repeater.GetElementIndex(element));
                }

                Verify.IsNull(repeater.TryGetElement(20));
            });
        }

        // Scenario: set ItemsSource to null, a list, another list, back to null and to a new list.
        // Expected: ItemsSourceView is null when there is no source and otherwise exposes the current items in order.
        // A failure means: switching or clearing the items source could crash or leave the repeater showing stale data.
        [TestMethod]
        public void CanSetItemsSource()
        {
            // In bug 12042052, we crash when we set ItemsSource to null because we try to subscribe to
            // the DataSourceChanged event on a null instance.
            RunOnUIThread.Execute(() =>
            {
                {
                    var repeater = new ItemsRepeater();
                    repeater.ItemsSource = null;
                    Verify.IsNull(repeater.ItemsSourceView);
                    repeater.ItemsSource = Enumerable.Range(0, 5).Select(i => string.Format("Item #{0}", i));
                    Verify.IsNotNull(repeater.ItemsSourceView);
                    Verify.AreEqual(5, repeater.ItemsSourceView.Count);
                    Verify.AreEqual("Item #0", repeater.ItemsSourceView.GetAt(0));
                }

                {
                    var repeater = new ItemsRepeater();
                    repeater.ItemsSource = Enumerable.Range(0, 5).Select(i => string.Format("Item #{0}", i));
                    Verify.AreEqual("Item #4", repeater.ItemsSourceView.GetAt(4));
                    repeater.ItemsSource = Enumerable.Range(5, 5).Select(i => string.Format("Item #{0}", i));
                    Verify.AreEqual(5, repeater.ItemsSourceView.Count);
                    Verify.AreEqual("Item #5", repeater.ItemsSourceView.GetAt(0));
                    repeater.ItemsSource = null;
                    Verify.IsNull(repeater.ItemsSourceView);
                    repeater.ItemsSource = Enumerable.Range(10, 5).Select(i => string.Format("Item #{0}", i));
                    Verify.AreEqual("Item #10", repeater.ItemsSourceView.GetAt(0));
                    repeater.ItemsSource = null;
                    Verify.IsNull(repeater.ItemsSourceView);
                }
            });
        }

        [TestMethod]
        public void ValidateGetSetItemsSource()
        {
            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater repeater = new ItemsRepeater();
                var dataSource = new ItemsSourceView(Enumerable.Range(0, 10).Select(i => string.Format("Item #{0}", i)));
                repeater.SetValue(ItemsRepeater.ItemsSourceProperty, dataSource);
                Verify.AreSame(dataSource, repeater.GetValue(ItemsRepeater.ItemsSourceProperty) as ItemsSourceView);
                Verify.AreSame(dataSource, repeater.ItemsSourceView);
            });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled until we have a fix for https://github.com/microsoft/CsWinRT/issues/430
        public void ValidateNullItemsSource()
        {
            RunOnUIThread.Execute(() =>
            {
                string errorMessage = string.Empty;
                ItemsRepeater repeater = new ItemsRepeater();
                try
                {
                    repeater.GetOrCreateElement(0);
                }
                catch (COMException e)
                {
                    errorMessage = e.Message;
                }
                //Make sure that we threw E_FAIL
                string expectedErrorMessage = "ItemSource doesn't have a value";
                Log.Comment($"Expected error message: '{expectedErrorMessage}'");
                Log.Comment($"Actual error message: '{errorMessage}'");
                Verify.IsTrue(errorMessage.Contains(expectedErrorMessage));
            });
        }


        [TestMethod]
        public void VerifyClearingItemsSourceClearsElements()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 4).Select(i => "Item #" + i));
            var mapping = (List<ContentControl>)null;

            RunOnUIThread.Execute(() =>
            {
                mapping = Enumerable.Range(0, data.Count).Select(i => new ContentControl { Width = 40, Height = 40 }).ToList();

                var dataSource = MockItemsSource.CreateDataSource(data, supportsUniqueIds: false);
                var elementFactory = MockElementFactory.CreateElementFactory(mapping);
                ItemsRepeater repeater = new ItemsRepeater();
                repeater.ItemsSource = dataSource;
                repeater.ItemTemplate = elementFactory;
                // This was an issue only for NonVirtualizing layouts
                repeater.Layout = new MyCustomNonVirtualizingStackLayout();
                Content = repeater;
                Content.UpdateLayout();
                elementFactory.ValidateRecycleElementCalls();

                repeater.ItemsSource = null;

                // Every realized element is returned to the element factory with the repeater as its Parent.
                elementFactory.ValidateRecycleElementCalls(
                    mapping.Select(element => new MockElementFactory.RecycleElementCallInfo(element, repeater)).ToArray());
            });

            foreach (var item in mapping)
            {
                Verify.IsNull(item.Parent);
            }
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout and a custom IElementFactory has its ItemsSource
        //           property replaced with a new list, an empty list, or null.
        // Expected: each realized element gets exactly one RecycleElement call whose args.Parent is the ItemsRepeater, the
        //           old elements leave the repeater, and only the new items are realized afterwards.
        // A failure means: element factories that pool elements per parent get a null Parent and cannot reuse them
        //           (task.ms/62261645).
        [TestMethod]
        [TestProperty("Description", "Verifies RecycleElement gets the ItemsRepeater as Parent when ItemsSource is replaced on a repeater with a NonVirtualizingLayout.")]
        public void VerifyRecycleElementHasParentWhenItemsSourceIsReplacedWithNonVirtualizingLayout()
        {
            foreach (var replacement in new[] { "NewList", "EmptyList", "Null" })
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("ItemsSource replacement: " + replacement);
                    var created = new List<UIElement>();
                    var elementFactory = new MockElementFactory()
                    {
                        GetElementFunc = (index, owner) =>
                        {
                            var element = new ContentControl { Width = 40, Height = 40 };
                            created.Add(element);
                            return element;
                        }
                    };

                    var repeater = new ItemsRepeater()
                    {
                        ItemsSource = Enumerable.Range(0, 4).Select(i => "Item #" + i).ToList(),
                        ItemTemplate = elementFactory,
                        Layout = new MyCustomNonVirtualizingStackLayout(),
                    };
                    Content = repeater;
                    Content.UpdateLayout();

                    Verify.AreEqual(4, created.Count);
                    var oldElements = created.ToList();
                    created.Clear();
                    elementFactory.ValidateRecycleElementCalls();

                    repeater.ItemsSource =
                        replacement == "NewList" ? Enumerable.Range(0, 3).Select(i => "New item #" + i).ToList() :
                        replacement == "EmptyList" ? new List<string>() :
                        null;

                    elementFactory.ValidateRecycleElementCalls(
                        oldElements.Select(element => new MockElementFactory.RecycleElementCallInfo(element, repeater)).ToArray());
                    foreach (var element in oldElements)
                    {
                        Verify.IsNull(VisualTreeHelper.GetParent(element), "Recycled elements are removed from the repeater.");
                    }

                    Content.UpdateLayout();

                    int expectedCount = replacement == "NewList" ? 3 : 0;
                    Verify.AreEqual(expectedCount, created.Count);
                    Verify.AreEqual(expectedCount, VisualTreeHelper.GetChildrenCount(repeater));
                    elementFactory.ValidateRecycleElementCalls();
                });
            }
        }

        // Scenario: same as VerifyRecycleElementHasParentWhenItemsSourceIsReplacedWithNonVirtualizingLayout, but ItemsSource is
        //           replaced, the repeater is laid out, and ItemsSource is replaced again.
        // Expected: each replacement recycles only the elements realized for the previous source, once each, with the
        //           ItemsRepeater as args.Parent.
        // A failure means: a later replacement recycles stale elements twice or passes a null Parent (task.ms/62261645).
        [TestMethod]
        [TestProperty("Description", "Verifies repeated ItemsSource replacement on a NonVirtualizingLayout repeater recycles each element once with the repeater as Parent.")]
        public void VerifyRecycleElementHasParentWhenItemsSourceIsReplacedTwiceWithNonVirtualizingLayout()
        {
            RunOnUIThread.Execute(() =>
            {
                var created = new List<UIElement>();
                var elementFactory = new MockElementFactory()
                {
                    GetElementFunc = (index, owner) =>
                    {
                        var element = new ContentControl { Width = 40, Height = 40 };
                        created.Add(element);
                        return element;
                    }
                };

                var repeater = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = elementFactory,
                    Layout = new MyCustomNonVirtualizingStackLayout(),
                };
                Content = repeater;
                Content.UpdateLayout();

                for (int round = 1; round <= 2; round++)
                {
                    Log.Comment("Replacement round " + round);
                    var realized = created.ToList();
                    created.Clear();
                    Verify.AreEqual(3, realized.Count);

                    repeater.ItemsSource = Enumerable.Range(0, 3).Select(i => "Round " + round + " item #" + i).ToList();
                    elementFactory.ValidateRecycleElementCalls(
                        realized.Select(element => new MockElementFactory.RecycleElementCallInfo(element, repeater)).ToArray());

                    Content.UpdateLayout();
                    Verify.AreEqual(3, VisualTreeHelper.GetChildrenCount(repeater));
                }
            });
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout has its ItemsSource replaced; while the first element is
        //           being recycled, the element factory's RecycleElement sets ItemsSource again (a nested replacement).
        // Expected: no exception; every realized element is recycled exactly once, all with the ItemsRepeater as
        //           args.Parent; the repeater then shows the nested source.
        // A failure means: re-entrant ItemsSource changes recycle elements without a Parent, twice, or crash
        //           (task.ms/62261645).
        [TestMethod]
        [TestProperty("Description", "Verifies a nested ItemsSource replacement from RecycleElement on a NonVirtualizingLayout repeater recycles each element once with the repeater as Parent.")]
        public void VerifyNestedItemsSourceReplacementFromRecycleElementWithNonVirtualizingLayout()
        {
            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater repeater = null;
                bool nestedReplacementDone = false;
                var nestedSource = new List<string> { "Nested item #0", "Nested item #1" };
                var created = new List<UIElement>();
                var elementFactory = new MockElementFactory()
                {
                    GetElementFunc = (index, owner) =>
                    {
                        var element = new ContentControl { Width = 40, Height = 40 };
                        created.Add(element);
                        return element;
                    },
                    ClearElementFunc = (element, parent) =>
                    {
                        if (!nestedReplacementDone)
                        {
                            nestedReplacementDone = true;
                            Log.Comment("Replacing ItemsSource from RecycleElement");
                            repeater.ItemsSource = nestedSource;
                        }
                    }
                };

                repeater = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, 4).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = elementFactory,
                    Layout = new MyCustomNonVirtualizingStackLayout(),
                };
                Content = repeater;
                Content.UpdateLayout();

                var oldElements = created.ToList();
                created.Clear();
                Verify.AreEqual(4, oldElements.Count);

                repeater.ItemsSource = Enumerable.Range(0, 3).Select(i => "New item #" + i).ToList();

                Verify.IsTrue(nestedReplacementDone);
                elementFactory.ValidateRecycleElementCalls(
                    oldElements.Select(element => new MockElementFactory.RecycleElementCallInfo(element, repeater)).ToArray());

                Content.UpdateLayout();
                Verify.AreEqual(nestedSource.Count, repeater.ItemsSourceView.Count);
                Verify.AreEqual(nestedSource.Count, created.Count);
                Verify.AreEqual(nestedSource.Count, VisualTreeHelper.GetChildrenCount(repeater));
            });
        }

        // Scenario: same as VerifyNestedItemsSourceReplacementFromRecycleElementWithNonVirtualizingLayout, but the element
        //           factory reads args.Element and args.Parent again after the nested replacement returns (for example to
        //           pool the element with args.Parent as owner).
        // Expected: args.Element and args.Parent stay the element being recycled and the ItemsRepeater for the whole
        //           RecycleElement call, in the outer call and in the nested calls.
        // A failure means: a nested recycle reset the args the outer RecycleElement call is still using, so the factory
        //           sees a null Element and a null Parent (task.ms/62261645).
        [TestMethod]
        [TestProperty("Description", "Verifies RecycleElement args keep Element and Parent after a nested ItemsSource replacement from RecycleElement on a NonVirtualizingLayout repeater.")]
        public void VerifyRecycleElementArgsAreUnchangedAfterNestedItemsSourceReplacementWithNonVirtualizingLayout()
        {
            RunOnUIThread.Execute(() =>
            {
                var nestedSource = new List<string> { "Nested item #0", "Nested item #1" };
                var elementFactory = new ArgsRereadingElementFactory();
                var repeater = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = elementFactory,
                    Layout = new MyCustomNonVirtualizingStackLayout(),
                };
                elementFactory.ReentrantAction = () =>
                {
                    Log.Comment("Replacing ItemsSource from RecycleElement");
                    repeater.ItemsSource = nestedSource;
                };
                Content = repeater;
                Content.UpdateLayout();

                var oldElements = elementFactory.Created.ToList();
                elementFactory.Created.Clear();
                Verify.AreEqual(3, oldElements.Count);

                repeater.ItemsSource = Enumerable.Range(0, 3).Select(i => "New item #" + i).ToList();

                Verify.IsNull(elementFactory.ReentrantAction, "The nested replacement should have run.");
                Verify.AreEqual(oldElements.Count, elementFactory.Calls.Count, "Each old element is recycled once.");
                foreach (var call in elementFactory.Calls)
                {
                    Log.Comment(string.Format("RecycleElement for element #{0}: before (Element set: {1}, Parent is repeater: {2}), after (same Element: {3}, Parent is repeater: {4})",
                        oldElements.IndexOf(call.ElementBefore), call.ElementBefore != null, call.ParentBefore == repeater,
                        call.ElementAfter == call.ElementBefore, call.ParentAfter == repeater));
                    Verify.IsTrue(oldElements.Contains(call.ElementBefore), "args.Element is one of the old elements.");
                    Verify.AreEqual(repeater, call.ParentBefore, "args.Parent is the repeater when RecycleElement starts.");
                    Verify.AreEqual(call.ElementBefore, call.ElementAfter, "args.Element is unchanged when RecycleElement ends.");
                    Verify.AreEqual(repeater, call.ParentAfter, "args.Parent is unchanged when RecycleElement ends.");
                }
                Verify.AreEqual(oldElements.Count, elementFactory.Calls.Select(call => call.ElementBefore).Distinct().Count());

                Content.UpdateLayout();
                Verify.AreEqual(nestedSource.Count, elementFactory.Created.Count);
                Verify.AreEqual(nestedSource.Count, VisualTreeHelper.GetChildrenCount(repeater));
            });
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout handles ElementClearing. On the first call the handler
        //           replaces ItemsSource (a nested replacement that raises ElementClearing again) and then reads args.Element.
        //           A later, non-nested replacement raises ElementClearing again.
        // Expected: args.Element stays the element being cleared for the whole outer handler call; the nested calls get
        //           their own args with their own element; the later calls use the repeater's cached args again.
        // A failure means: the nested ElementClearing calls overwrote the args the outer handler is still using, so the
        //           outer handler acts on another element (task.ms/62261645).
        [TestMethod]
        [TestProperty("Description", "Verifies ElementClearing args keep Element after a nested ItemsSource replacement from ElementClearing on a NonVirtualizingLayout repeater.")]
        public void VerifyElementClearingArgsAreUnchangedAfterNestedItemsSourceReplacementWithNonVirtualizingLayout()
        {
            RunOnUIThread.Execute(() =>
            {
                var factory = new MockElementFactory()
                {
                    GetElementFunc = (index, owner) => new ContentControl { Width = 40, Height = 40 },
                };
                var nestedSource = new List<string> { "Nested item #0", "Nested item #1" };
                var repeater = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = factory,
                    Layout = new MyCustomNonVirtualizingStackLayout(),
                };
                Content = repeater;
                Content.UpdateLayout();
                var oldElements = Enumerable.Range(0, 3).Select(i => repeater.TryGetElement(i)).ToList();
                Verify.IsTrue(oldElements.All(e => e != null), "3 elements realized.");

                bool replaceFromHandler = true;
                int depth = 0;
                var calls = new List<(ItemsRepeaterElementClearingEventArgs Args, UIElement ElementBefore, UIElement ElementAfter, int Depth)>();
                repeater.ElementClearing += (s, args) =>
                {
                    var before = args.Element;
                    var index = calls.Count;
                    calls.Add((args, before, null, depth));
                    if (replaceFromHandler)
                    {
                        replaceFromHandler = false;
                        Log.Comment("Replacing ItemsSource from ElementClearing");
                        depth++;
                        repeater.ItemsSource = nestedSource;
                        depth--;
                    }
                    calls[index] = (args, before, args.Element, calls[index].Depth);
                };

                repeater.ItemsSource = Enumerable.Range(0, 3).Select(i => "New item #" + i).ToList();

                Verify.IsFalse(replaceFromHandler, "The nested replacement should have run.");
                foreach (var call in calls)
                {
                    Log.Comment(string.Format("ElementClearing depth {0}: element before #{1}, after #{2}, same args as the outer call: {3}",
                        call.Depth, oldElements.IndexOf(call.ElementBefore), oldElements.IndexOf(call.ElementAfter), ReferenceEquals(call.Args, calls[0].Args)));
                    Verify.IsTrue(oldElements.Contains(call.ElementBefore), "args.Element is one of the old elements.");
                    Verify.AreSame(call.ElementBefore, call.ElementAfter, "args.Element is unchanged when the handler returns.");
                }
                var outerCall = calls[0];
                Verify.AreEqual(0, outerCall.Depth);
                var nestedCalls = calls.Where(call => call.Depth == 1).ToList();
                Verify.IsTrue(nestedCalls.Count > 0, "The nested replacement raised ElementClearing.");
                Verify.IsTrue(nestedCalls.All(call => !ReferenceEquals(call.Args, outerCall.Args)), "Nested calls get their own args.");
                Verify.IsTrue(oldElements.All(e => calls.Count(call => call.ElementBefore == e) == 1), "Every old element is cleared once.");

                Content.UpdateLayout();
                Verify.AreEqual(nestedSource.Count, VisualTreeHelper.GetChildrenCount(repeater));

                var nestedElements = Enumerable.Range(0, nestedSource.Count).Select(i => repeater.TryGetElement(i)).ToList();
                calls.Clear();
                repeater.ItemsSource = new List<string> { "Last item #0" };

                Verify.AreEqual(nestedElements.Count, calls.Count, "The later replacement clears each element once.");
                foreach (var call in calls)
                {
                    Verify.IsTrue(nestedElements.Contains(call.ElementBefore));
                    Verify.AreSame(call.ElementBefore, call.ElementAfter);
                    Verify.IsTrue(ReferenceEquals(call.Args, outerCall.Args), "Later, non-nested calls use the repeater's cached args again.");
                }
            });
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout is taken out of the live tree (as for a cached Page, a
        //           closed Popup or Flyout, or an unselected tab) and ItemsSource is replaced; it is then put back.
        //           Variants: a custom element factory, and a DataTemplate (which pools its elements).
        // Expected: as in the live tree, ElementClearing is raised and RecycleElement is called (with the repeater as
        //           Parent) for each realized element; with a DataTemplate the pooled elements are reused.
        // A failure means: the repeater dropped its elements without clearing or recycling them because it checked the
        //           visual parent, which is null out of the live tree (task.ms/62261645).
        [TestMethod]
        [TestProperty("Description", "Verifies elements are cleared and recycled when ItemsSource is replaced on a NonVirtualizingLayout repeater that is not in the live tree.")]
        public void VerifyItemsSourceReplacementRecyclesElementsWhenRepeaterIsNotInLiveTreeWithNonVirtualizingLayout()
        {
            foreach (bool useDataTemplate in new[] { false, true })
            {
                ItemsRepeater repeater = null;
                var recycled = new List<(UIElement Element, UIElement Parent)>();
                var clearing = new List<UIElement>();
                List<UIElement> oldElements = null;

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Variant: " + (useDataTemplate ? "DataTemplate" : "custom element factory"));
                    var factory = new MockElementFactory()
                    {
                        GetElementFunc = (index, owner) => new ContentControl { Width = 40, Height = 40 },
                        ClearElementFunc = (element, parent) => recycled.Add((element, parent))
                    };
                    repeater = new ItemsRepeater()
                    {
                        ItemsSource = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList(),
                        ItemTemplate = useDataTemplate ? (object)CreateDataTemplateWithContent(@"<TextBlock Text='{Binding}' Height='40' />") : factory,
                        Layout = new MyCustomNonVirtualizingStackLayout(),
                    };
                    repeater.ElementClearing += (s, a) => clearing.Add(a.Element);
                    Content = repeater;
                    Content.UpdateLayout();
                    oldElements = Enumerable.Range(0, 3).Select(i => repeater.TryGetElement(i)).ToList();
                    Verify.IsTrue(oldElements.All(e => e != null), "3 elements realized.");
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Content = null;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Visual parent of the first element out of the live tree is null: " + (VisualTreeHelper.GetParent(oldElements[0]) == null));
                    repeater.ItemsSource = Enumerable.Range(0, 2).Select(i => "New item #" + i).ToList();
                    Log.Comment(string.Format("ElementClearing: {0}, RecycleElement: {1} (Parent is repeater: {2})",
                        clearing.Count, recycled.Count, recycled.Count(r => r.Parent == repeater)));

                    Verify.AreEqual(oldElements.Count, clearing.Count, "ElementClearing is raised for each old element.");
                    Verify.IsTrue(oldElements.All(e => clearing.Contains(e)));
                    if (!useDataTemplate)
                    {
                        Verify.AreEqual(oldElements.Count, recycled.Count, "Each old element is returned to the element factory.");
                        Verify.IsTrue(recycled.All(r => r.Parent == repeater), "RecycleElement gets the repeater as Parent.");
                    }

                    Content = repeater;
                    Content.UpdateLayout();
                    Verify.AreEqual(2, VisualTreeHelper.GetChildrenCount(repeater));
                    if (useDataTemplate)
                    {
                        int reused = Enumerable.Range(0, 2).Select(i => repeater.TryGetElement(i)).Count(e => oldElements.Contains(e));
                        Log.Comment("New elements reused from the DataTemplate pool: " + reused);
                        Verify.AreEqual(2, reused, "The DataTemplate's pool supplies the new elements.");
                    }
                    Content = null;
                });
                IdleSynchronizer.Wait();
            }
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout uses an element factory whose RecycleElement throws for the
        //           first element recycled by an ItemsSource replacement (the test catches the exception from the setter).
        //           ItemsSource is then replaced again, which recycles the remaining elements normally.
        // Expected: after the second replacement the args object passed to the throwing call no longer holds the element or
        //           the repeater, and the later RecycleElement calls get their element and the repeater, through the
        //           repeater's cached args as before the exception.
        // A failure means: the exception left Element and Parent set on the repeater's cached RecycleElement args, so they
        //           keep the element and the repeater for the repeater's lifetime and every later recycle uses new args
        //           (task.ms/62261645).
        [TestMethod]
        [TestProperty("Description", "Verifies RecycleElement args are reset after RecycleElement throws during an ItemsSource replacement on a NonVirtualizingLayout repeater.")]
        public void VerifyRecycleElementArgsAreResetAfterRecycleElementThrowsWithNonVirtualizingLayout()
        {
            RunOnUIThread.Execute(() =>
            {
                var elementFactory = new ThrowOnceElementFactory();
                var repeater = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = elementFactory,
                    Layout = new MyCustomNonVirtualizingStackLayout(),
                };
                Content = repeater;
                Content.UpdateLayout();

                var oldElements = elementFactory.Created.ToList();
                elementFactory.Created.Clear();
                Verify.AreEqual(3, oldElements.Count);

                elementFactory.ThrowOnNextRecycle = true;
                int hresult = CaptureHResult(() => repeater.ItemsSource = Enumerable.Range(0, 3).Select(i => "New item #" + i).ToList());
                Log.Comment($"First ItemsSource replacement (RecycleElement throws) returned HResult=0x{hresult:X8}");
                Verify.AreEqual(1, elementFactory.Calls.Count, "Only the throwing RecycleElement call ran during the first replacement.");
                var thrownCall = elementFactory.Calls[0];
                Verify.IsTrue(thrownCall.Threw, "The first RecycleElement call threw.");
                Verify.IsTrue(oldElements.Contains(thrownCall.ElementBefore), "args.Element of the throwing call is one of the old elements.");
                Verify.AreEqual(repeater, thrownCall.ParentBefore, "args.Parent of the throwing call is the repeater.");

                repeater.ItemsSource = Enumerable.Range(0, 2).Select(i => "Last item #" + i).ToList();

                var laterCalls = elementFactory.Calls.Skip(1).ToList();
                Verify.AreEqual(oldElements.Count - 1, laterCalls.Count, "The second replacement recycles the elements the first one did not reach.");
                foreach (var call in laterCalls)
                {
                    Log.Comment(string.Format("Later RecycleElement for element #{0}: Parent is repeater: {1}, same args object as the throwing call: {2}",
                        oldElements.IndexOf(call.ElementBefore), call.ParentBefore == repeater, ReferenceEquals(call.Args, thrownCall.Args)));
                    Verify.IsFalse(call.Threw);
                    Verify.IsTrue(oldElements.Contains(call.ElementBefore), "args.Element is one of the old elements.");
                    Verify.AreNotEqual(thrownCall.ElementBefore, call.ElementBefore, "The element whose recycle threw is not recycled again.");
                    Verify.AreEqual(repeater, call.ParentBefore, "args.Parent is the repeater.");
                }

                Log.Comment(string.Format("Args of the throwing call after the second replacement: Element set: {0}, Parent set: {1}",
                    thrownCall.Args.Element != null, thrownCall.Args.Parent != null));
                Verify.IsNull(thrownCall.Args.Element, "The args of the throwing call no longer hold its element.");
                Verify.IsNull(thrownCall.Args.Parent, "The args of the throwing call no longer hold the repeater.");
                foreach (var call in laterCalls)
                {
                    Verify.IsTrue(ReferenceEquals(call.Args, thrownCall.Args), "Later recycles use the repeater's cached args again.");
                    Verify.IsNull(call.Args.Element);
                    Verify.IsNull(call.Args.Parent);
                }

                Content.UpdateLayout();
                Verify.AreEqual(2, elementFactory.Created.Count);
                Verify.AreEqual(2, VisualTreeHelper.GetChildrenCount(repeater));
            });
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout uses an element factory that, in RecycleElement, puts the
        //           element in a RecyclePool with args.Parent as owner and hands it to another owner right away (so the
        //           pool removes it from the repeater's children); ItemsSource is replaced.
        // Expected: every realized element is recycled exactly once and the repeater has no children afterwards.
        // A failure means: the repeater skips elements, or crashes, when its children change while it recycles them.
        [TestMethod]
        [TestProperty("Description", "Verifies every element is recycled once when elements leave the repeater's children during an ItemsSource replacement on a NonVirtualizingLayout repeater.")]
        public void VerifyItemsSourceReplacementRecyclesAllElementsWhenFactoryRemovesThemFromParent()
        {
            RunOnUIThread.Execute(() =>
            {
                var created = new List<UIElement>();
                var recycled = new List<UIElement>();
                var pool = new RecyclePool();
                var otherOwner = new StackPanel();
                int movedToOtherOwner = 0;
                var elementFactory = new MockElementFactory()
                {
                    GetElementFunc = (index, owner) =>
                    {
                        var element = new ContentControl { Width = 40, Height = 40 };
                        created.Add(element);
                        return element;
                    },
                    ClearElementFunc = (element, parent) =>
                    {
                        recycled.Add(element);
                        // Pool the element with args.Parent as its owner, then hand it to another owner right away.
                        // RecyclePool removes an element from its owner's children when another owner takes it,
                        // so the element leaves the repeater's children while the repeater is recycling.
                        pool.PutElement(element, "key", parent);
                        if (pool.TryGetElement("key", otherOwner) == element && VisualTreeHelper.GetParent(element) == null)
                        {
                            movedToOtherOwner++;
                        }
                    }
                };

                var repeater = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, 5).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = elementFactory,
                    Layout = new MyCustomNonVirtualizingStackLayout(),
                };
                Content = repeater;
                Content.UpdateLayout();

                var oldElements = created.ToList();
                Verify.AreEqual(5, oldElements.Count);

                repeater.ItemsSource = new List<string>();

                Log.Comment("Elements taken out of the repeater by the factory: " + movedToOtherOwner);
                Verify.AreEqual(oldElements.Count, recycled.Count);
                for (int i = 0; i < oldElements.Count; i++)
                {
                    Verify.AreEqual(1, recycled.Count(element => element == oldElements[i]), "Element " + i + " should be recycled once.");
                }

                Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(repeater));
            });
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout and no ItemTemplate shows UIElement items (each item is its
        //           own element); its ItemsSource is replaced with a new list of UIElements, or with null. Runs with 1 and 4
        //           items, with the first element focused.
        // Expected: no exception; ElementClearing is raised once per old element, every old element leaves the repeater,
        //           and only the new elements are children after the next layout.
        // A failure means: the repeater skips children or reads past the end of Children while it removes UIElement
        //           items (each removal shrinks Children during the walk); this crashed for two or more items.
        [TestMethod]
        [TestProperty("Description", "Verifies replacing ItemsSource of UIElement items without an ItemTemplate on a NonVirtualizingLayout repeater removes every old element.")]
        public void VerifyItemsSourceReplacementRemovesUIElementItemsWithoutItemTemplateWithNonVirtualizingLayout()
        {
            foreach (var count in new[] { 1, 4 })
            {
                foreach (var replaceWithNull in new[] { false, true })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        Log.Comment(string.Format("Items: {0}, replaced with {1}", count, replaceWithNull ? "null" : "a new list"));
                        var oldItems = Enumerable.Range(0, count).Select(i => new Button { Content = "Old #" + i, Width = 80, Height = 30 }).ToList();
                        var newItems = Enumerable.Range(0, 3).Select(i => new Button { Content = "New #" + i, Width = 80, Height = 30 }).ToList();
                        var cleared = new List<UIElement>();
                        var repeater = new ItemsRepeater()
                        {
                            ItemsSource = oldItems,
                            Layout = new MyCustomNonVirtualizingStackLayout(),
                        };
                        repeater.ElementClearing += (sender, args) => cleared.Add(args.Element);
                        Content = repeater;
                        Content.UpdateLayout();

                        Verify.AreEqual(count, VisualTreeHelper.GetChildrenCount(repeater));
                        foreach (var item in oldItems)
                        {
                            Verify.AreSame(repeater, VisualTreeHelper.GetParent(item));
                        }
                        Log.Comment("First element focused: " + oldItems[0].Focus(FocusState.Programmatic));

                        repeater.ItemsSource = replaceWithNull ? null : newItems;

                        Verify.AreEqual(count, cleared.Count);
                        for (int i = 0; i < count; i++)
                        {
                            Verify.AreEqual(1, cleared.Count(element => element == oldItems[i]), "ElementClearing should be raised once for element " + i + ".");
                            Verify.IsNull(VisualTreeHelper.GetParent(oldItems[i]), "Element " + i + " should be removed from the repeater.");
                        }
                        Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(repeater));

                        Content.UpdateLayout();

                        Verify.AreEqual(replaceWithNull ? 0 : newItems.Count, VisualTreeHelper.GetChildrenCount(repeater));
                        if (!replaceWithNull)
                        {
                            foreach (var item in newItems)
                            {
                                Verify.AreSame(repeater, VisualTreeHelper.GetParent(item));
                            }
                        }
                    });
                }
            }
        }

        // Scenario: two ItemsRepeaters A and B with NonVirtualizingLayouts use element factories that share one RecyclePool.
        //           A's ItemsSource is replaced. When A's factory recycles the first element, it replaces A's ItemsSource
        //           again (a nested replacement). For each element the nested replacement recycles, A's factory asks B to
        //           realize its next item (B.GetOrCreateElement), so B takes that element from the pool right away.
        // Expected: A recycles each of its old elements exactly once and does not raise ElementClearing for an element
        //           that B owns; B keeps the elements it realized (B's children at the expected indices); the pool holds
        //           no element that is still a child of B.
        // A failure means: after a nested ItemsSource change, the outer replacement clears and recycles elements that
        //           another repeater sharing the pool has already realized.
        [TestMethod]
        [TestProperty("Description", "Verifies an ItemsSource replacement on a NonVirtualizingLayout repeater does not clear elements that another repeater sharing the RecyclePool realized during a nested replacement.")]
        public void VerifyItemsSourceReplacementDoesNotClearElementsRealizedByAnotherRepeaterSharingThePool()
        {
            RunOnUIThread.Execute(() =>
            {
                const string key = "key";
                var pool = new RecyclePool();
                ItemsRepeater repeaterA = null;
                ItemsRepeater repeaterB = null;
                bool nestedReplacementStarted = false;
                bool inNestedReplacement = false;
                int nextIndexB = 0;
                var createdByA = new List<UIElement>();
                var recycledByA = new List<UIElement>();
                var takenByB = new List<UIElement>();
                var clearedWhileOwnedByB = new List<UIElement>();
                var nestedSource = new List<string> { "Nested #0", "Nested #1" };

                var factoryA = new MockElementFactory()
                {
                    GetElementFunc = (index, owner) =>
                    {
                        var element = new ContentControl { Width = 40, Height = 40 };
                        createdByA.Add(element);
                        return element;
                    },
                    ClearElementFunc = (element, parent) =>
                    {
                        recycledByA.Add(element);
                        pool.PutElement(element, key, parent);
                        if (!nestedReplacementStarted)
                        {
                            nestedReplacementStarted = true;
                            inNestedReplacement = true;
                            Log.Comment("Replacing ItemsSource of A from RecycleElement");
                            repeaterA.ItemsSource = nestedSource;
                            inNestedReplacement = false;
                        }
                        else if (inNestedReplacement)
                        {
                            // B takes the element that was just pooled (the pool returns the last entry when no entry
                            // belongs to B).
                            takenByB.Add(repeaterB.GetOrCreateElement(nextIndexB++));
                        }
                    }
                };
                var factoryB = new MockElementFactory()
                {
                    GetElementFunc = (index, owner) => pool.TryGetElement(key, owner) ?? new ContentControl { Width = 40, Height = 40 },
                };

                repeaterA = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, 5).Select(i => "A" + i).ToList(),
                    ItemTemplate = factoryA,
                    Layout = new MyCustomNonVirtualizingStackLayout(),
                };
                repeaterB = new ItemsRepeater()
                {
                    ItemTemplate = factoryB,
                    Layout = new MyCustomNonVirtualizingStackLayout(),
                };
                var root = new StackPanel();
                root.Children.Add(repeaterA);
                root.Children.Add(repeaterB);
                Content = root;
                Content.UpdateLayout();

                var oldElementsA = createdByA.ToList();
                createdByA.Clear();
                Verify.AreEqual(5, oldElementsA.Count);

                // B gets items but is not laid out yet, so GetOrCreateElement realizes them on demand.
                repeaterB.ItemsSource = Enumerable.Range(0, 6).Select(i => "B" + i).ToList();
                repeaterA.ElementClearing += (sender, args) =>
                {
                    if (VisualTreeHelper.GetParent(args.Element) == repeaterB)
                    {
                        clearedWhileOwnedByB.Add(args.Element);
                    }
                };

                repeaterA.ItemsSource = Enumerable.Range(0, 3).Select(i => "New A" + i).ToList();

                Verify.IsTrue(nestedReplacementStarted);
                Log.Comment("Elements B took from the pool during the nested replacement: " + takenByB.Count);
                Verify.AreEqual(oldElementsA.Count - 1, takenByB.Count);
                Verify.AreEqual(0, clearedWhileOwnedByB.Count, "A should not raise ElementClearing for elements that B owns.");
                for (int i = 0; i < oldElementsA.Count; i++)
                {
                    Verify.AreEqual(1, recycledByA.Count(element => element == oldElementsA[i]), "A's element " + i + " should be recycled once.");
                }

                for (int i = 0; i < takenByB.Count; i++)
                {
                    Verify.AreSame(oldElementsA[i + 1], takenByB[i], "B should reuse the element A just recycled.");
                    Verify.AreSame(repeaterB, VisualTreeHelper.GetParent(takenByB[i]));
                    Verify.AreSame(takenByB[i], repeaterB.TryGetElement(i), "B's element " + i + " should stay realized.");
                    Verify.AreEqual(i, repeaterB.GetElementIndex(takenByB[i]));
                }

                Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(repeaterA));

                int pooledCount = 0;
                UIElement pooled;
                while ((pooled = pool.TryGetElement(key)) != null)
                {
                    pooledCount++;
                    Verify.IsFalse(VisualTreeHelper.GetParent(pooled) == repeaterB, "The pool should not hold an element that is a child of B.");
                }
                Log.Comment("Elements left in the pool: " + pooledCount);

                Content.UpdateLayout();

                Verify.AreEqual(nestedSource.Count, VisualTreeHelper.GetChildrenCount(repeaterA));
                Verify.AreEqual(6, VisualTreeHelper.GetChildrenCount(repeaterB));
                for (int i = 0; i < takenByB.Count; i++)
                {
                    Verify.AreSame(takenByB[i], repeaterB.TryGetElement(i));
                }
            });
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout and a custom element factory shows 3 items; its
        //           ItemsSource is replaced, and the ElementClearing handler for the element at position 0, 1 or 2 replaces
        //           ItemsSource again (a nested replacement that clears the same element while its ElementClearing is
        //           still running).
        // Expected: no exception or assert; ElementClearing is raised once for each old element and each old element is
        //           returned to the element factory exactly once; the repeater shows the nested source after layout.
        // A failure means: the element whose ElementClearing started the nested replacement is recycled twice (for
        //           example, a duplicate RecyclePool entry), gets a second ElementClearing from the nested replacement,
        //           or the repeater crashes.
        [TestMethod]
        [TestProperty("Description", "Verifies a nested ItemsSource replacement from ElementClearing on a NonVirtualizingLayout repeater recycles each element once.")]
        public void VerifyNestedItemsSourceReplacementFromElementClearingWithNonVirtualizingLayout()
        {
            foreach (var position in new[] { 0, 1, 2 })
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Nested replacement from ElementClearing of the element at position " + position);
                    var created = new List<UIElement>();
                    var recycled = new List<UIElement>();
                    var elementFactory = new MockElementFactory()
                    {
                        GetElementFunc = (index, owner) =>
                        {
                            var element = new ContentControl { Width = 40, Height = 40 };
                            created.Add(element);
                            return element;
                        },
                        ClearElementFunc = (element, parent) => recycled.Add(element)
                    };

                    var repeater = new ItemsRepeater()
                    {
                        ItemsSource = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList(),
                        ItemTemplate = elementFactory,
                        Layout = new MyCustomNonVirtualizingStackLayout(),
                    };
                    Content = repeater;
                    Content.UpdateLayout();

                    var oldElements = created.ToList();
                    created.Clear();
                    Verify.AreEqual(3, oldElements.Count);

                    bool nestedReplacementDone = false;
                    var nestedSource = new List<string> { "Nested #0", "Nested #1" };
                    var clearing = new List<UIElement>();
                    repeater.ElementClearing += (sender, args) =>
                    {
                        clearing.Add(args.Element);
                        if (!nestedReplacementDone && args.Element == oldElements[position])
                        {
                            nestedReplacementDone = true;
                            Log.Comment("Replacing ItemsSource from ElementClearing");
                            repeater.ItemsSource = nestedSource;
                        }
                    };

                    repeater.ItemsSource = Enumerable.Range(0, 3).Select(i => "New item #" + i).ToList();

                    Verify.IsTrue(nestedReplacementDone);
                    Verify.AreEqual(oldElements.Count, recycled.Count, "Each old element should be recycled once.");
                    for (int i = 0; i < oldElements.Count; i++)
                    {
                        Log.Comment(string.Format("Element {0}: ElementClearing {1}, RecycleElement {2}", i,
                            clearing.Count(element => element == oldElements[i]), recycled.Count(element => element == oldElements[i])));
                        Verify.AreEqual(1, recycled.Count(element => element == oldElements[i]), "Element " + i + " should be recycled once.");
                        Verify.AreEqual(1, clearing.Count(element => element == oldElements[i]), "ElementClearing should be raised once for element " + i + ".");
                        Verify.IsNull(VisualTreeHelper.GetParent(oldElements[i]));
                    }
                    Verify.AreEqual(oldElements.Count, clearing.Count, "ElementClearing should be raised once per old element.");
                    Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(repeater));

                    Content.UpdateLayout();

                    Verify.AreEqual(nestedSource.Count, repeater.ItemsSourceView.Count);
                    Verify.AreEqual(nestedSource.Count, created.Count);
                    Verify.AreEqual(nestedSource.Count, VisualTreeHelper.GetChildrenCount(repeater));
                });
            }
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout and a custom element factory shows 3 items; its
        //           ItemsSource is replaced, and the ElementClearing handler for the element X at position 0, 1 or 2
        //           replaces ItemsSource again (a nested replacement that clears and recycles X while its ElementClearing is
        //           still running), then realizes X again with GetOrCreateElement(0) (the factory hands X back) and
        //           replaces ItemsSource a third time, which clears X again - all before the handler returns.
        // Expected: ElementClearing is raised once for each lifetime of X (twice in total, the second time while X is
        //           realized at index 0) and once for each other old element; X is recycled once per lifetime.
        // A failure means: the repeater still treats X as being in the ElementClearing raise of its first lifetime and does
        //           not raise ElementClearing when X is cleared in its second lifetime (or raises it twice).
        [TestMethod]
        [TestProperty("Description", "Verifies ElementClearing is raised again for an element that an ElementClearing handler re-realizes and clears again on a NonVirtualizingLayout repeater.")]
        public void VerifyElementClearingIsRaisedAgainForElementRealizedAgainFromElementClearingWithNonVirtualizingLayout()
        {
            foreach (var position in new[] { 0, 1, 2 })
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Element at position " + position + " is cleared, realized again and cleared again from its ElementClearing handler");
                    UIElement reusedElement = null;
                    bool reuse = false;
                    var recycled = new List<UIElement>();
                    var elementFactory = new MockElementFactory()
                    {
                        GetElementFunc = (index, owner) =>
                        {
                            if (reuse && reusedElement != null && recycled.Contains(reusedElement))
                            {
                                return reusedElement;
                            }
                            return new ContentControl { Width = 40, Height = 40 };
                        },
                        ClearElementFunc = (element, parent) => recycled.Add(element)
                    };

                    var repeater = new ItemsRepeater()
                    {
                        ItemsSource = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList(),
                        ItemTemplate = elementFactory,
                        Layout = new MyCustomNonVirtualizingStackLayout(),
                    };
                    Content = repeater;
                    Content.UpdateLayout();

                    var oldElements = Enumerable.Range(0, 3).Select(i => repeater.TryGetElement(i)).ToList();
                    Verify.IsTrue(oldElements.All(e => e != null), "3 elements realized.");
                    reusedElement = oldElements[position];

                    bool nestedChangesDone = false;
                    UIElement realizedAgain = null;
                    int recycledBeforeSecondClear = -1;
                    var thirdSource = new List<string> { "Third #0", "Third #1" };
                    var clearing = new List<(UIElement Element, int Index)>();
                    repeater.ElementClearing += (sender, args) =>
                    {
                        var element = args.Element;
                        clearing.Add((element, repeater.GetElementIndex(element)));
                        if (!nestedChangesDone && element == reusedElement)
                        {
                            nestedChangesDone = true;
                            Log.Comment("Replacing ItemsSource from ElementClearing");
                            repeater.ItemsSource = new List<string> { "Nested #0", "Nested #1" };
                            Log.Comment("Realizing item 0 again from ElementClearing");
                            reuse = true;
                            try
                            {
                                realizedAgain = repeater.GetOrCreateElement(0);
                            }
                            finally
                            {
                                reuse = false;
                            }
                            recycledBeforeSecondClear = recycled.Count(e => e == reusedElement);
                            Log.Comment("Replacing ItemsSource a third time from ElementClearing");
                            repeater.ItemsSource = thirdSource;
                        }
                    };

                    repeater.ItemsSource = Enumerable.Range(0, 3).Select(i => "New item #" + i).ToList();

                    Verify.IsTrue(nestedChangesDone);
                    Verify.AreSame(reusedElement, realizedAgain, "GetOrCreateElement(0) realized the element that started the nested changes again.");
                    Verify.AreEqual(1, recycledBeforeSecondClear, "The nested replacement recycled the element once before it was realized again.");
                    for (int i = 0; i < oldElements.Count; i++)
                    {
                        var calls = clearing.Where(call => call.Element == oldElements[i]).ToList();
                        Log.Comment(string.Format("Element {0}: ElementClearing {1} (indices {2}), RecycleElement {3}", i, calls.Count,
                            string.Join(",", calls.Select(call => call.Index)), recycled.Count(element => element == oldElements[i])));
                        int lifetimes = i == position ? 2 : 1;
                        Verify.AreEqual(lifetimes, calls.Count, "ElementClearing is raised once per lifetime of element " + i + ".");
                        Verify.AreEqual(lifetimes, recycled.Count(element => element == oldElements[i]), "Element " + i + " is recycled once per lifetime.");
                    }
                    var secondClearIndex = clearing.Where(call => call.Element == reusedElement).Select(call => call.Index).Skip(1).DefaultIfEmpty(int.MinValue).First();
                    Verify.AreEqual(0, secondClearIndex, "The second ElementClearing of the element comes while it is realized at index 0.");
                    Verify.AreEqual(4, clearing.Count);
                    Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(repeater));

                    Content.UpdateLayout();
                    Verify.AreEqual(thirdSource.Count, repeater.ItemsSourceView.Count);
                    Verify.AreEqual(thirdSource.Count, VisualTreeHelper.GetChildrenCount(repeater));
                });
            }
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout and a custom element factory shows 3 items of an
        //           ObservableCollection. The item at position 0, 1 or 2 is removed; the ElementClearing handler for its
        //           element X replaces ItemsSource (a nested replacement that clears and recycles X while its ElementClearing
        //           is still running), then realizes X again with GetOrCreateElement(0) (the factory hands X back) and returns.
        // Expected: when the handler returns, the clear of X that raised the event does not continue on X's new lifetime:
        //           X stays realized at index 0 with its DataContext, is recycled once, ElementClearing is raised once for
        //           X, and X becomes a scroll anchor candidate like the other realized element after layout.
        // A failure means: the outer clear recycled X again (or cleared its DataContext, or dropped it from the anchor
        //           candidates) although X was realized again, so the repeater lost an element it shows.
        [TestMethod]
        [TestProperty("Description", "Verifies the clear that raised ElementClearing does not continue on an element that the handler cleared through a nested ItemsSource replacement and realized again, on a NonVirtualizingLayout repeater.")]
        public void VerifyElementRealizedAgainFromElementClearingIsNotRecycledByOuterClearWithNonVirtualizingLayout()
        {
            foreach (var position in new[] { 0, 1, 2 })
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Element at position " + position + " is cleared, and realized again by its ElementClearing handler after a nested ItemsSource replacement");
                    UIElement reusedElement = null;
                    bool reuse = false;
                    var recycled = new List<UIElement>();
                    var elementFactory = new MockElementFactory()
                    {
                        GetElementFunc = (index, owner) =>
                        {
                            if (reuse && reusedElement != null && recycled.Contains(reusedElement))
                            {
                                return reusedElement;
                            }
                            return new ContentControl { Width = 40, Height = 40 };
                        },
                        ClearElementFunc = (element, parent) => recycled.Add(element)
                    };

                    var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => "Item #" + i));
                    var repeater = new ItemsRepeater()
                    {
                        ItemsSource = data,
                        ItemTemplate = elementFactory,
                        Layout = new MyCustomNonVirtualizingStackLayout(),
                    };
                    Content = repeater;
                    Content.UpdateLayout();

                    reusedElement = repeater.TryGetElement(position);
                    Verify.IsNotNull(reusedElement);

                    int clearingCount = 0;
                    bool nestedChangesDone = false;
                    UIElement realizedAgain = null;
                    int indexInHandler = int.MinValue;
                    var nestedSource = new List<string> { "Nested #0", "Nested #1" };
                    repeater.ElementClearing += (sender, args) =>
                    {
                        if (args.Element != reusedElement)
                        {
                            return;
                        }
                        clearingCount++;
                        if (!nestedChangesDone)
                        {
                            nestedChangesDone = true;
                            Log.Comment("Replacing ItemsSource from ElementClearing");
                            repeater.ItemsSource = nestedSource;
                            Log.Comment("Realizing item 0 again from ElementClearing");
                            reuse = true;
                            try
                            {
                                realizedAgain = repeater.GetOrCreateElement(0);
                            }
                            finally
                            {
                                reuse = false;
                            }
                            indexInHandler = repeater.GetElementIndex(reusedElement);
                        }
                    };

                    data.RemoveAt(position);

                    var dataContext = ((FrameworkElement)reusedElement).DataContext;
                    Log.Comment(string.Format("After the clear: index in handler {0}, GetElementIndex {1}, DataContext {2}, ElementClearing {3}, RecycleElement {4}",
                        indexInHandler, repeater.GetElementIndex(reusedElement), dataContext ?? "<null>", clearingCount, recycled.Count(e => e == reusedElement)));
                    Verify.IsTrue(nestedChangesDone);
                    Verify.AreSame(reusedElement, realizedAgain, "GetOrCreateElement(0) realized the element that started the nested change again.");
                    Verify.AreEqual(0, indexInHandler, "The element is realized at index 0 in the handler.");
                    Verify.AreEqual(0, repeater.GetElementIndex(reusedElement), "The element is still realized at index 0 after the clear that raised ElementClearing returns.");
                    Verify.AreEqual(nestedSource[0], dataContext, "The element keeps the DataContext of its new item.");
                    Verify.AreEqual(1, clearingCount, "ElementClearing is raised once for the element.");
                    Verify.AreEqual(1, recycled.Count(e => e == reusedElement), "The element is recycled once.");

                    Content.UpdateLayout();
                    repeater.InvalidateMeasure();
                    Content.UpdateLayout();

                    var other = repeater.TryGetElement(1);
                    Log.Comment(string.Format("After layout: children {0}, GetElementIndex {1}, RecycleElement {2}, CanBeScrollAnchor {3} (item 1: {4})",
                        VisualTreeHelper.GetChildrenCount(repeater), repeater.GetElementIndex(reusedElement), recycled.Count(e => e == reusedElement),
                        reusedElement.CanBeScrollAnchor, other == null ? "<none>" : other.CanBeScrollAnchor.ToString()));
                    Verify.AreEqual(nestedSource.Count, VisualTreeHelper.GetChildrenCount(repeater), "The repeater holds one element per item.");
                    Verify.AreSame(reusedElement, repeater.TryGetElement(0), "Layout keeps the element realized again at index 0.");
                    Verify.AreEqual(1, recycled.Count(e => e == reusedElement), "Layout does not recycle the element.");
                    Verify.IsNotNull(other);
                    Verify.IsTrue(other.CanBeScrollAnchor, "The element created by layout is a scroll anchor candidate.");
                    Verify.IsTrue(reusedElement.CanBeScrollAnchor, "The element realized again is a scroll anchor candidate too.");
                });
            }
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout and a custom element factory shows 3 items. Its Layout (or
        //           ItemTemplate) is replaced, which clears every realized element in a walk that does not reset the realized
        //           range afterwards. The ElementClearing handler for the last element X (old index 2) replaces ItemsSource with
        //           3 new items (a nested replacement that clears X and the children), then realizes items 0, 1 and 2 with
        //           GetOrCreateElement and makes item 0 the anchor again, and returns.
        // Expected: when the handler returns, the clear of X that raised the event does not shrink the realized range with X's
        //           old index 2, so the next layout finds the element the handler realized at index 2: it creates and recycles
        //           no element and the repeater holds one element per item.
        // A failure means: the outer clear updated the realized range with X's old index after the nested change had already
        //           done it, the range no longer includes index 2 although an element is realized there, and layout created a
        //           second element for item 2 (and recycled the extra one on the next layout).
        [TestMethod]
        [TestProperty("Description", "Verifies the clear that raised ElementClearing does not update the realized range when the handler cleared the element through a nested ItemsSource replacement during a Layout or ItemTemplate change, on a NonVirtualizingLayout repeater.")]
        public void VerifyLayoutDoesNotDuplicateElementRealizedFromElementClearingDuringLayoutOrItemTemplateChangeWithNonVirtualizingLayout()
        {
            foreach (var outerChange in new[] { "Layout", "ItemTemplate" })
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Outer change: " + outerChange + "; the ElementClearing handler of the last element replaces ItemsSource and realizes items 0, 1 and 2");
                    var created = new List<UIElement>();
                    var recycled = new List<UIElement>();
                    Func<MockElementFactory> createFactory = () => new MockElementFactory()
                    {
                        GetElementFunc = (index, owner) =>
                        {
                            var element = new ContentControl { Width = 40, Height = 40 };
                            created.Add(element);
                            return element;
                        },
                        ClearElementFunc = (element, parent) => recycled.Add(element)
                    };

                    var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => "Item #" + i));
                    var repeater = new ItemsRepeater()
                    {
                        ItemsSource = data,
                        ItemTemplate = createFactory(),
                        Layout = new MyCustomNonVirtualizingStackLayout(),
                    };
                    Content = repeater;
                    Content.UpdateLayout();

                    var lastElement = repeater.TryGetElement(2);
                    Verify.IsNotNull(lastElement);

                    bool nestedChangesDone = false;
                    var realizedInHandler = new UIElement[3];
                    var nestedSource = new List<string> { "Nested #0", "Nested #1", "Nested #2" };
                    repeater.ElementClearing += (sender, args) =>
                    {
                        if (args.Element != lastElement || nestedChangesDone)
                        {
                            return;
                        }
                        nestedChangesDone = true;
                        Log.Comment("Replacing ItemsSource from ElementClearing of the element at index 2");
                        repeater.ItemsSource = nestedSource;
                        Log.Comment("Realizing items 0, 1 and 2 from ElementClearing, then making item 0 the anchor again");
                        for (int i = 0; i < nestedSource.Count; i++)
                        {
                            realizedInHandler[i] = repeater.GetOrCreateElement(i);
                        }
                        repeater.GetOrCreateElement(0);
                    };

                    if (outerChange == "Layout")
                    {
                        repeater.Layout = new MyCustomNonVirtualizingStackLayout();
                    }
                    else
                    {
                        repeater.ItemTemplate = createFactory();
                    }

                    Verify.IsTrue(nestedChangesDone);
                    for (int i = 0; i < nestedSource.Count; i++)
                    {
                        Verify.IsNotNull(realizedInHandler[i]);
                        Verify.AreEqual(i, repeater.GetElementIndex(realizedInHandler[i]), "The element realized in the handler for item " + i + " is still realized after the change.");
                    }

                    int createdBeforeLayout = created.Count;
                    int recycledBeforeLayout = recycled.Count;
                    Content.UpdateLayout();
                    Log.Comment(string.Format("After the first layout: children {0}, created {1}, recycled {2}, element at 2 is the one realized in the handler: {3}",
                        VisualTreeHelper.GetChildrenCount(repeater), created.Count - createdBeforeLayout, recycled.Count - recycledBeforeLayout,
                        repeater.TryGetElement(2) == realizedInHandler[2]));
                    Verify.AreEqual(0, created.Count - createdBeforeLayout, "Layout does not create an element for an item that is already realized.");
                    Verify.AreEqual(nestedSource.Count, VisualTreeHelper.GetChildrenCount(repeater), "The repeater holds one element per item.");

                    repeater.InvalidateMeasure();
                    Content.UpdateLayout();
                    Log.Comment(string.Format("After the second layout: children {0}, created {1}, recycled {2}",
                        VisualTreeHelper.GetChildrenCount(repeater), created.Count - createdBeforeLayout, recycled.Count - recycledBeforeLayout));
                    Verify.AreEqual(0, created.Count - createdBeforeLayout, "Layout does not create an element.");
                    Verify.AreEqual(0, recycled.Count - recycledBeforeLayout, "Layout does not recycle an element.");
                    for (int i = 0; i < nestedSource.Count; i++)
                    {
                        Verify.AreSame(realizedInHandler[i], repeater.TryGetElement(i), "Layout keeps the element realized in the handler for item " + i + ".");
                    }
                });
            }
        }

        // Scenario: two ItemsRepeaters with NonVirtualizingLayouts share a RecyclePool through their element factories.
        //           Repeater A shows 3 items; its ItemsSource is replaced, and the ElementClearing handler for A's element X
        //           at position 0, 1 or 2 replaces A's ItemsSource again (a nested replacement that clears X and puts it in
        //           the pool while its ElementClearing is still running), then realizes 3 items of repeater B with
        //           GetOrCreateElement, so B takes X from the pool, and returns.
        // Expected: when the handler returns, A's clear of X that raised the event does not continue on X: X stays realized
        //           by B with B's DataContext, A recycles X once, and the next layout does not throw.
        // A failure means: A recycled X again although B realized it (B loses X, X is put back in the pool while it is
        //           B's child, and the next layout fails with "Element is already the child of another element").
        [TestMethod]
        [TestProperty("Description", "Verifies the clear that raised ElementClearing does not continue on an element that the handler cleared through a nested ItemsSource replacement and another repeater sharing the RecyclePool realized.")]
        public void VerifyElementTakenByOtherRepeaterFromElementClearingIsNotRecycledByOuterClearWithNonVirtualizingLayout()
        {
            foreach (var position in new[] { 0, 1, 2 })
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Element at position " + position + " of repeater A is cleared, and taken by repeater B from the shared pool in its ElementClearing handler after a nested ItemsSource replacement");
                    const string key = "key";
                    var pool = new RecyclePool();
                    var recycledByA = new List<UIElement>();
                    var factoryA = new MockElementFactory()
                    {
                        GetElementFunc = (index, owner) => pool.TryGetElement(key, owner) ?? new ContentControl { Width = 40, Height = 40 },
                        ClearElementFunc = (element, parent) => { recycledByA.Add(element); pool.PutElement(element, key, parent); }
                    };
                    var factoryB = new MockElementFactory()
                    {
                        GetElementFunc = (index, owner) => pool.TryGetElement(key, owner) ?? new ContentControl { Width = 40, Height = 40 },
                        ClearElementFunc = (element, parent) => pool.PutElement(element, key, parent)
                    };
                    var repeaterA = new ItemsRepeater()
                    {
                        ItemsSource = Enumerable.Range(0, 3).Select(i => "A" + i).ToList(),
                        ItemTemplate = factoryA,
                        Layout = new MyCustomNonVirtualizingStackLayout(),
                    };
                    var repeaterB = new ItemsRepeater()
                    {
                        ItemTemplate = factoryB,
                        Layout = new MyCustomNonVirtualizingStackLayout(),
                    };
                    var root = new StackPanel();
                    root.Children.Add(repeaterA);
                    root.Children.Add(repeaterB);
                    Content = root;
                    Content.UpdateLayout();

                    var element = repeaterA.TryGetElement(position);
                    Verify.IsNotNull(element);
                    repeaterB.ItemsSource = Enumerable.Range(0, 3).Select(i => "B" + i).ToList();

                    int clearingCount = 0;
                    bool nestedChangesDone = false;
                    var takenByB = new List<UIElement>();
                    repeaterA.ElementClearing += (sender, args) =>
                    {
                        if (args.Element != element)
                        {
                            return;
                        }
                        clearingCount++;
                        if (!nestedChangesDone)
                        {
                            nestedChangesDone = true;
                            Log.Comment("Replacing ItemsSource of repeater A from ElementClearing");
                            repeaterA.ItemsSource = new List<string> { "Nested #0", "Nested #1" };
                            Log.Comment("Realizing the items of repeater B from ElementClearing");
                            for (int i = 0; i < 3; i++)
                            {
                                takenByB.Add(repeaterB.GetOrCreateElement(i));
                            }
                        }
                    };

                    repeaterA.ItemsSource = Enumerable.Range(0, 3).Select(i => "New A" + i).ToList();

                    int indexInB = takenByB.IndexOf(element);
                    var dataContext = ((FrameworkElement)element).DataContext;
                    Log.Comment(string.Format("After the clear: taken by B at {0}, parent is B {1}, B.GetElementIndex {2}, DataContext {3}, ElementClearing on A {4}, RecycleElement by A {5}",
                        indexInB, VisualTreeHelper.GetParent(element) == repeaterB, repeaterB.GetElementIndex(element), dataContext ?? "<null>",
                        clearingCount, recycledByA.Count(e => e == element)));
                    Verify.IsTrue(nestedChangesDone);
                    Verify.IsTrue(indexInB >= 0, "Repeater B took the element from the shared pool.");
                    Verify.AreEqual(indexInB, repeaterB.GetElementIndex(element), "The element is still realized by repeater B.");
                    Verify.AreEqual("B" + indexInB, dataContext, "The element keeps repeater B's DataContext.");
                    Verify.AreEqual(1, clearingCount, "ElementClearing is raised once for the element on repeater A.");
                    Verify.AreEqual(1, recycledByA.Count(e => e == element), "Repeater A recycles the element once.");

                    string layoutError = "none";
                    try
                    {
                        Content.UpdateLayout();
                    }
                    catch (Exception e)
                    {
                        layoutError = e.GetType().Name + " 0x" + e.HResult.ToString("X8") + " " + e.Message;
                    }
                    Log.Comment(string.Format("After layout: error {0}, A children {1}, B children {2}, parent is B {3}",
                        layoutError, VisualTreeHelper.GetChildrenCount(repeaterA), VisualTreeHelper.GetChildrenCount(repeaterB),
                        VisualTreeHelper.GetParent(element) == repeaterB));
                    Verify.AreEqual("none", layoutError, "Layout does not throw.");
                    Verify.AreEqual(2, VisualTreeHelper.GetChildrenCount(repeaterA));
                    Verify.AreEqual(3, VisualTreeHelper.GetChildrenCount(repeaterB));
                    Verify.AreEqual(indexInB, repeaterB.GetElementIndex(element), "The element is still realized by repeater B after layout.");
                });
            }
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout shows 3 items and has two C# ElementClearing handlers. Its
        //           ItemsSource is replaced; the first handler replaces ItemsSource again from the ElementClearing of the
        //           element at position 0, 1 or 2 (a nested replacement that clears the same element while its
        //           ElementClearing is still running). The second handler records what it sees for each element.
        //           Variants: a custom element factory, and a DataTemplate.
        // Expected: each handler gets ElementClearing once for each old element, and each old element is recycled once.
        //           The second handler gets the other elements while they are still realized. C# handlers of one event
        //           share one native delegate, so the repeater cannot call the second handler before the nested
        //           replacement recycles the element that started it: the second handler gets that element after it was
        //           recycled (logged, not verified; see the RadioButtons test for handlers registered separately).
        // A failure means: a handler got ElementClearing twice or not at all for an old element, an element was recycled
        //           twice, or the second handler got an element that was not involved in the nested replacement after it
        //           was recycled.
        [TestMethod]
        [TestProperty("Description", "Verifies two ElementClearing handlers each get every element once when the first one replaces ItemsSource on a NonVirtualizingLayout repeater.")]
        public void VerifyTwoElementClearingHandlersGetEachElementOnceAfterNestedItemsSourceReplacementWithNonVirtualizingLayout()
        {
            foreach (bool useDataTemplate in new[] { false, true })
            {
                foreach (var position in new[] { 0, 1, 2 })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        Log.Comment(string.Format("Variant: {0}, nested replacement from the ElementClearing of the element at position {1}",
                            useDataTemplate ? "DataTemplate" : "custom element factory", position));
                        var recycled = new List<UIElement>();
                        var elementFactory = new MockElementFactory()
                        {
                            GetElementFunc = (index, owner) => new ContentControl { Width = 40, Height = 40 },
                            ClearElementFunc = (element, parent) => recycled.Add(element)
                        };

                        var oldItems = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList();
                        var repeater = new ItemsRepeater()
                        {
                            ItemsSource = oldItems,
                            ItemTemplate = useDataTemplate ? (object)CreateDataTemplateWithContent(@"<TextBlock Text='{Binding}' Height='40' />") : elementFactory,
                            Layout = new MyCustomNonVirtualizingStackLayout(),
                        };
                        Content = repeater;
                        Content.UpdateLayout();

                        var oldElements = Enumerable.Range(0, 3).Select(i => repeater.TryGetElement(i)).ToList();
                        Verify.IsTrue(oldElements.All(e => e != null), "3 elements realized.");

                        bool nestedReplacementDone = false;
                        var nestedSource = new List<string> { "Nested #0", "Nested #1" };
                        var firstHandlerCalls = new List<UIElement>();
                        var secondHandlerCalls = new List<(UIElement Element, object DataContext, int Index, DependencyObject Parent, bool Recycled)>();
                        repeater.ElementClearing += (sender, args) =>
                        {
                            firstHandlerCalls.Add(args.Element);
                            if (!nestedReplacementDone && args.Element == oldElements[position])
                            {
                                nestedReplacementDone = true;
                                Log.Comment("First handler: replacing ItemsSource from ElementClearing");
                                repeater.ItemsSource = nestedSource;
                            }
                        };
                        repeater.ElementClearing += (sender, args) =>
                        {
                            var element = args.Element;
                            secondHandlerCalls.Add((
                                element,
                                (element as FrameworkElement)?.DataContext,
                                repeater.GetElementIndex(element),
                                VisualTreeHelper.GetParent(element),
                                recycled.Contains(element)));
                        };

                        repeater.ItemsSource = Enumerable.Range(0, 3).Select(i => "New item #" + i).ToList();

                        Verify.IsTrue(nestedReplacementDone);
                        foreach (var call in secondHandlerCalls)
                        {
                            Log.Comment(string.Format("Second handler: element #{0}, DataContext '{1}', GetElementIndex {2}, parent is repeater {3}, already recycled {4}",
                                oldElements.IndexOf(call.Element), call.DataContext, call.Index, call.Parent == repeater, call.Recycled));
                        }

                        for (int i = 0; i < oldElements.Count; i++)
                        {
                            Verify.AreEqual(1, firstHandlerCalls.Count(element => element == oldElements[i]), "The first handler gets ElementClearing once for element " + i + ".");
                            var calls = secondHandlerCalls.Where(call => call.Element == oldElements[i]).ToList();
                            Verify.AreEqual(1, calls.Count, "The second handler gets ElementClearing once for element " + i + ".");
                            if (i != position)
                            {
                                Verify.AreEqual(oldItems[i], calls[0].DataContext, "Element " + i + " still has its DataContext when the second handler runs.");
                                Verify.AreEqual(i, calls[0].Index, "GetElementIndex of element " + i + " when the second handler runs.");
                                Verify.AreEqual(repeater, calls[0].Parent, "Element " + i + " is still a child of the repeater when the second handler runs.");
                                Verify.IsFalse(calls[0].Recycled, "Element " + i + " was not yet recycled when the second handler runs.");
                            }
                            if (!useDataTemplate)
                            {
                                Verify.AreEqual(1, recycled.Count(element => element == oldElements[i]), "Element " + i + " should be recycled once.");
                            }
                        }
                        Verify.AreEqual(oldElements.Count, firstHandlerCalls.Count);
                        Verify.AreEqual(oldElements.Count, secondHandlerCalls.Count);
                        Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(repeater));

                        Content.UpdateLayout();
                        Verify.AreEqual(nestedSource.Count, VisualTreeHelper.GetChildrenCount(repeater));
                    });
                }
            }
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout and a custom element factory shows 3 items from an
        //           ObservableCollection; the first, middle or last item is removed, or the collection is cleared, and the
        //           ElementClearing handler for the element that the collection change clears replaces ItemsSource (a
        //           nested replacement that clears the same element while the collection change is still clearing it).
        // Expected: no exception or assert; ElementClearing is raised once for each old element and each old element is
        //           returned to the element factory exactly once, with the repeater as Parent; after layout the repeater
        //           shows the new source with the right element indices.
        // A failure means: the collection change recycles the element a second time after the nested replacement already
        //           recycled it (for example, a duplicate RecyclePool entry), the nested replacement raises a second
        //           ElementClearing for it, or the repeater crashes.
        [TestMethod]
        [TestProperty("Description", "Verifies an ItemsSource replacement from ElementClearing during a collection Remove or Reset on a NonVirtualizingLayout repeater recycles each element once.")]
        public void VerifyItemsSourceReplacementFromElementClearingDuringCollectionChangeWithNonVirtualizingLayout()
        {
            var changes = new (string Name, Action<ObservableCollection<string>> Apply)[]
            {
                ("RemoveAt(0)", data => data.RemoveAt(0)),
                ("RemoveAt(1)", data => data.RemoveAt(1)),
                ("RemoveAt(2)", data => data.RemoveAt(2)),
                ("Clear", data => data.Clear()),
            };

            foreach (var change in changes)
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Nested ItemsSource replacement from ElementClearing during " + change.Name);
                    var created = new List<UIElement>();
                    var recycled = new List<(UIElement Element, UIElement Parent)>();
                    var elementFactory = new MockElementFactory()
                    {
                        GetElementFunc = (index, owner) =>
                        {
                            var element = new ContentControl { Width = 40, Height = 40 };
                            created.Add(element);
                            return element;
                        },
                        ClearElementFunc = (element, parent) => recycled.Add((element, parent))
                    };

                    var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => "Item #" + i));
                    var repeater = new ItemsRepeater()
                    {
                        ItemsSource = data,
                        ItemTemplate = elementFactory,
                        Layout = new MyCustomNonVirtualizingStackLayout(),
                    };
                    Content = repeater;
                    Content.UpdateLayout();

                    var oldElements = created.ToList();
                    created.Clear();
                    Verify.AreEqual(3, oldElements.Count);

                    bool nestedReplacementDone = false;
                    var nestedSource = new List<string> { "Nested #0", "Nested #1" };
                    var clearing = new List<UIElement>();
                    repeater.ElementClearing += (sender, args) =>
                    {
                        clearing.Add(args.Element);
                        if (!nestedReplacementDone)
                        {
                            nestedReplacementDone = true;
                            Log.Comment("Replacing ItemsSource from ElementClearing of element " + oldElements.IndexOf(args.Element));
                            repeater.ItemsSource = nestedSource;
                        }
                    };

                    change.Apply(data);

                    Verify.IsTrue(nestedReplacementDone);
                    Verify.AreEqual(oldElements.Count, recycled.Count, "Each old element should be recycled once.");
                    for (int i = 0; i < oldElements.Count; i++)
                    {
                        Log.Comment(string.Format("Element {0}: ElementClearing {1}, RecycleElement {2}", i,
                            clearing.Count(element => element == oldElements[i]), recycled.Count(r => r.Element == oldElements[i])));
                        Verify.AreEqual(1, recycled.Count(r => r.Element == oldElements[i]), "Element " + i + " should be recycled once.");
                        Verify.AreEqual(1, clearing.Count(element => element == oldElements[i]), "ElementClearing should be raised once for element " + i + ".");
                        Verify.IsNull(VisualTreeHelper.GetParent(oldElements[i]));
                    }
                    Verify.AreEqual(oldElements.Count, clearing.Count, "ElementClearing should be raised once per old element.");
                    foreach (var r in recycled)
                    {
                        Verify.AreSame(repeater, r.Parent, "RecycleElement should get the repeater as Parent.");
                    }
                    Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(repeater));

                    Content.UpdateLayout();

                    Verify.AreEqual(nestedSource.Count, repeater.ItemsSourceView.Count);
                    Verify.AreEqual(nestedSource.Count, created.Count);
                    Verify.AreEqual(nestedSource.Count, VisualTreeHelper.GetChildrenCount(repeater));
                    Verify.AreEqual(oldElements.Count, recycled.Count, "Layout should not recycle anything else.");
                    for (int i = 0; i < nestedSource.Count; i++)
                    {
                        Verify.AreSame(created[i], repeater.TryGetElement(i));
                        Verify.AreEqual(i, repeater.GetElementIndex(created[i]));
                    }
                });
            }
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout and a recording element factory A shows 3 items from an
        //           ObservableCollection; ItemsSource is replaced, or the middle item is removed, and the first
        //           ElementClearing handler call sets ItemTemplate to another factory B, or sets Layout to another
        //           NonVirtualizingLayout (a nested walk that clears every realized element, including the one whose
        //           ElementClearing is still running).
        // Expected: no exception or assert; ElementClearing is raised once for each old element; every old element is
        //           returned exactly once, to factory A, with the repeater as Parent; nothing is returned to B; after layout
        //           the new elements come from B (ItemTemplate) or A (Layout) and have the right indices.
        // A failure means: the outer change recycles the element a second time after the nested walk already recycled it
        //           (into B after an ItemTemplate change, or into A twice), the nested walk raises a second ElementClearing
        //           for it, or the repeater crashes.
        [TestMethod]
        [TestProperty("Description", "Verifies an ItemTemplate or Layout change from ElementClearing during an ItemsSource replacement or a Remove on a NonVirtualizingLayout repeater recycles each element once, to the old factory.")]
        public void VerifyItemTemplateOrLayoutChangeFromElementClearingRecyclesEachElementOnceWithNonVirtualizingLayout()
        {
            foreach (var outerChange in new[] { "ItemsSource", "RemoveAt(1)" })
            {
                foreach (var nestedChange in new[] { "ItemTemplate", "Layout" })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        Log.Comment("Outer change: " + outerChange + ", nested change from ElementClearing: " + nestedChange);
                        var createdByA = new List<UIElement>();
                        var recycledByA = new List<(UIElement Element, UIElement Parent)>();
                        var createdByB = new List<UIElement>();
                        var recycledByB = new List<UIElement>();
                        var factoryA = new MockElementFactory()
                        {
                            GetElementFunc = (index, owner) =>
                            {
                                var element = new ContentControl { Width = 40, Height = 40 };
                                createdByA.Add(element);
                                return element;
                            },
                            ClearElementFunc = (element, parent) => recycledByA.Add((element, parent))
                        };
                        var factoryB = new MockElementFactory()
                        {
                            GetElementFunc = (index, owner) =>
                            {
                                var element = new ContentControl { Width = 40, Height = 40 };
                                createdByB.Add(element);
                                return element;
                            },
                            ClearElementFunc = (element, parent) => recycledByB.Add(element)
                        };

                        var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => "Item #" + i));
                        var repeater = new ItemsRepeater()
                        {
                            ItemsSource = data,
                            ItemTemplate = factoryA,
                            Layout = new MyCustomNonVirtualizingStackLayout(),
                        };
                        Content = repeater;
                        Content.UpdateLayout();

                        var oldElements = createdByA.ToList();
                        createdByA.Clear();
                        Verify.AreEqual(3, oldElements.Count);

                        bool nestedChangeDone = false;
                        var clearing = new List<UIElement>();
                        repeater.ElementClearing += (sender, args) =>
                        {
                            clearing.Add(args.Element);
                            if (!nestedChangeDone)
                            {
                                nestedChangeDone = true;
                                Log.Comment("Setting " + nestedChange + " from ElementClearing of element " + oldElements.IndexOf(args.Element));
                                if (nestedChange == "ItemTemplate")
                                {
                                    repeater.ItemTemplate = factoryB;
                                }
                                else
                                {
                                    repeater.Layout = new MyCustomNonVirtualizingStackLayout();
                                }
                            }
                        };

                        int expectedCount;
                        if (outerChange == "ItemsSource")
                        {
                            var newSource = new List<string> { "New #0", "New #1" };
                            repeater.ItemsSource = newSource;
                            expectedCount = newSource.Count;
                        }
                        else
                        {
                            data.RemoveAt(1);
                            expectedCount = data.Count;
                        }

                        Verify.IsTrue(nestedChangeDone);
                        Verify.AreEqual(oldElements.Count, recycledByA.Count, "Each old element should be recycled once, to the old factory.");
                        for (int i = 0; i < oldElements.Count; i++)
                        {
                            Log.Comment(string.Format("Element {0}: ElementClearing {1}, RecycleElement {2}", i,
                                clearing.Count(element => element == oldElements[i]), recycledByA.Count(r => r.Element == oldElements[i])));
                            Verify.AreEqual(1, recycledByA.Count(r => r.Element == oldElements[i]), "Element " + i + " should be recycled once.");
                            Verify.AreEqual(1, clearing.Count(element => element == oldElements[i]), "ElementClearing should be raised once for element " + i + ".");
                        }
                        Verify.AreEqual(oldElements.Count, clearing.Count, "ElementClearing should be raised once per old element.");
                        foreach (var r in recycledByA)
                        {
                            Verify.AreSame(repeater, r.Parent, "RecycleElement should get the repeater as Parent.");
                        }
                        Verify.AreEqual(0, recycledByB.Count, "No old element should be recycled to the new factory.");

                        Content.UpdateLayout();

                        var newElements = nestedChange == "ItemTemplate" ? createdByB : createdByA;
                        Verify.AreEqual(expectedCount, repeater.ItemsSourceView.Count);
                        Verify.AreEqual(expectedCount, newElements.Count, "The new elements should come from the current ItemTemplate.");
                        Verify.AreEqual(0, (nestedChange == "ItemTemplate" ? createdByA : createdByB).Count, "The other factory should not create elements.");
                        Verify.AreEqual(oldElements.Count, recycledByA.Count, "Layout should not recycle anything else.");
                        Verify.AreEqual(0, recycledByB.Count);
                        for (int i = 0; i < expectedCount; i++)
                        {
                            Verify.AreSame(newElements[i], repeater.TryGetElement(i));
                            Verify.AreEqual(i, repeater.GetElementIndex(newElements[i]));
                        }
                        for (int i = 0; i < oldElements.Count; i++)
                        {
                            Verify.AreEqual(-1, repeater.GetElementIndex(oldElements[i]), "Old element " + i + " should not be realized.");
                        }
                    });
                }
            }
        }

        // Scenario: two ItemsRepeaters with NonVirtualizingLayouts share one DataTemplate (and so one RecyclePool). The first
        //           repeater's ItemsSource is replaced (its elements go to the pool and it clears its children), then the
        //           second repeater realizes new items.
        // Expected: no exception; the second repeater reuses the pooled elements and they become its children.
        // A failure means: taking an element that its former owner already removed from its children throws
        //           "ItemsRepeater's child not found in its Children collection."
        [TestMethod]
        [TestProperty("Description", "Verifies elements recycled by an ItemsSource replacement on a NonVirtualizingLayout repeater can be reused by another repeater sharing the DataTemplate.")]
        public void VerifyElementsRecycledByItemsSourceReplacementCanBeReusedByAnotherRepeater()
        {
            RunOnUIThread.Execute(() =>
            {
                var template = CreateDataTemplateWithContent(@"<TextBlock Text='{Binding}' Height='20' />");
                var itemsB = new ObservableCollection<string>(Enumerable.Range(0, 2).Select(i => "B" + i));
                var repeaterA = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, 3).Select(i => "A" + i).ToList(),
                    ItemTemplate = template,
                    Layout = new MyCustomNonVirtualizingStackLayout(),
                };
                var repeaterB = new ItemsRepeater()
                {
                    ItemsSource = itemsB,
                    ItemTemplate = template,
                    Layout = new MyCustomNonVirtualizingStackLayout(),
                };
                var root = new StackPanel();
                root.Children.Add(repeaterA);
                root.Children.Add(repeaterB);
                Content = root;
                Content.UpdateLayout();

                var elementsA = Enumerable.Range(0, 3).Select(i => repeaterA.TryGetElement(i)).ToList();
                foreach (var element in elementsA)
                {
                    Verify.IsNotNull(element);
                }

                Log.Comment("Replace ItemsSource of the first repeater");
                repeaterA.ItemsSource = null;
                Content.UpdateLayout();
                Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(repeaterA));

                Log.Comment("Realize new items in the second repeater");
                itemsB.Add("B2");
                itemsB.Add("B3");
                itemsB.Add("B4");
                Content.UpdateLayout();

                Verify.AreEqual(5, VisualTreeHelper.GetChildrenCount(repeaterB));
                for (int i = 2; i < 5; i++)
                {
                    var element = (TextBlock)repeaterB.TryGetElement(i);
                    Verify.IsNotNull(element);
                    Verify.AreEqual("B" + i, element.Text);
                    Verify.AreSame(repeaterB, VisualTreeHelper.GetParent(element));
                    Verify.IsTrue(elementsA.Contains(element), "The element recycled by the first repeater is reused.");
                }

                Verify.AreEqual(0, VisualTreeHelper.GetChildrenCount(repeaterA));
            });
        }

        [TestMethod]
        public void ValidateGetSetBackground()
        {
            RunOnUIThread.Execute(() =>
            {
                ItemsRepeater repeater = new ItemsRepeater();
                var redBrush = new SolidColorBrush(Microsoft.UI.Colors.Red);
                repeater.SetValue(ItemsRepeater.BackgroundProperty, redBrush);
                Verify.AreSame(redBrush, repeater.GetValue(ItemsRepeater.BackgroundProperty) as Brush);
                Verify.AreSame(redBrush, repeater.Background);

                var blueBrush = new SolidColorBrush(Microsoft.UI.Colors.Blue);
                repeater.Background = blueBrush;
                Verify.AreSame(blueBrush, repeater.Background);
            });
        }

        //[TestMethod] 24022837
        public void VerifyCurrentAnchor()
        {
            if (PlatformConfiguration.IsDebugBuildConfiguration())
            {
                // Test is failing in chk configuration due to:
                // Bug #1726 Test Failure: RepeaterTests.VerifyCurrentAnchor
                Log.Warning("Skipping test for Debug builds.");
                return;
            }

            ItemsRepeater rootRepeater = null;
            ScrollViewer scrollViewer = null;
            ItemsRepeaterScrollHost scrollhost = null;
            ManualResetEvent viewChanged = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                scrollhost = (ItemsRepeaterScrollHost)XamlReader.Load(
                  @"<controls:ItemsRepeaterScrollHost Width='400' Height='600'
                     xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                     xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                     xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                    <controls:ItemsRepeaterScrollHost.Resources>
                        <DataTemplate x:Key='ItemTemplate' >
                            <TextBlock Text='{Binding}' Height='50'/>
                        </DataTemplate>
                    </controls:ItemsRepeaterScrollHost.Resources>
                    <ScrollViewer x:Name='scrollviewer'>
                        <controls:ItemsRepeater x:Name='rootRepeater' ItemTemplate='{StaticResource ItemTemplate}' VerticalCacheLength='0' />
                    </ScrollViewer>
                </controls:ItemsRepeaterScrollHost>");

                rootRepeater = (ItemsRepeater)scrollhost.FindName("rootRepeater");
                scrollViewer = (ScrollViewer)scrollhost.FindName("scrollviewer");
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChanged.Set();
                    }
                };

                rootRepeater.ItemsSource = Enumerable.Range(0, 500);
                Content = scrollhost;
            });

            // scroll down several times and validate current anchor
            for (int i = 1; i < 10; i++)
            {
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    scrollViewer.ChangeView(null, i * 200, null);
                });

                Verify.IsTrue(viewChanged.WaitOne(DefaultWaitTimeInMS));
                viewChanged.Reset();
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(i * 200, scrollViewer.VerticalOffset);
                    var anchor = scrollViewer.CurrentAnchor;
                    var anchorIndex = rootRepeater.GetElementIndex(anchor);
                    Log.Comment("CurrentAnchor: " + anchorIndex);
                    Verify.AreEqual(i * 4, anchorIndex);
                });
            }
        }

        // Ensure that scrolling a nested repeater works when the
        // Itemtemplates are data templates.
        [TestMethod]
        public void NestedRepeaterWithDataTemplateScenario()
        {
            NestedRepeaterWithDataTemplateScenario(disableAnimation: true);
            NestedRepeaterWithDataTemplateScenario(disableAnimation: false);
        }

        [TestMethod]
        public void VerifyFocusedItemIsRecycledOnCollectionReset()
        {
            List<Layout> layouts = new List<Layout>();
            RunOnUIThread.Execute(() =>
            {
                layouts.Add(new MyCustomNonVirtualizingStackLayout());
                layouts.Add(new StackLayout());
            });

            foreach (var layout in layouts)
            {
                List<string> items = new List<string> { "item0", "item1", "item2", "item3", "item4", "item5", "item6", "item7", "item8", "item9" };
                const int targetIndex = 4;
                string targetItem = items[targetIndex];
                ItemsRepeater repeater = null;

                RunOnUIThread.Execute(() =>
                {
                    repeater = new ItemsRepeater() {
                        ItemsSource = items,
                        ItemTemplate = CreateDataTemplateWithContent(@"<Button Content='{Binding}'/>"),
                        Layout = layout
                    };
                    Content = repeater;
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Setting Focus on item " + targetIndex);
                    Button toFocus = (Button)repeater.TryGetElement(targetIndex);
                    Verify.AreEqual(targetItem, toFocus.Content as string);
                    toFocus.Focus(FocusState.Keyboard);
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Removing focused element from collection");
                    items.Remove(targetItem);

                    Log.Comment("Reset the collection with an empty list");
                    repeater.ItemsSource = new List<string>();
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Verify new elements");
                    for (int i = 0; i < items.Count; i++)
                    {
                        Button currentButton = (Button)repeater.TryGetElement(i);
                        Verify.IsNull(currentButton);
                    }
                });
            }
        }

        private DataTemplate CreateDataTemplateWithContent(string content)
        {
            return (DataTemplate)XamlReader.Load(@"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" + content + @"</DataTemplate>");
        }

        private void NestedRepeaterWithDataTemplateScenario(bool disableAnimation)
        {
            if (!disableAnimation)
            {
                Log.Warning("This test is showing consistent issues with not scrolling enough, tracked by microsoft-ui-xaml#779");
                return;
            }

            // Example of how to include debug tracing in an ApiTests.ItemsRepeater test's output.
            // using (PrivateLoggingHelper privateLoggingHelper = new PrivateLoggingHelper("ItemsRepeater"))
            // {
            ItemsRepeater rootRepeater = null;
            ScrollViewer scrollViewer = null;
            ManualResetEvent viewChanged = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                var anchorProvider = (ItemsRepeaterScrollHost)XamlReader.Load(
                    @"<controls:ItemsRepeaterScrollHost Width='400' Height='600'
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                    <controls:ItemsRepeaterScrollHost.Resources>
                        <DataTemplate x:Key='ItemTemplate' >
                            <TextBlock Text='{Binding}' />
                        </DataTemplate>
                        <DataTemplate x:Key='GroupTemplate'>
                            <StackPanel>
                                <TextBlock Text='{Binding}' />
                                <controls:ItemsRepeater ItemTemplate='{StaticResource ItemTemplate}' ItemsSource='{Binding}' VerticalCacheLength='0'/>
                            </StackPanel>
                        </DataTemplate>
                    </controls:ItemsRepeaterScrollHost.Resources>
                    <ScrollViewer x:Name='scrollviewer'>
                        <controls:ItemsRepeater x:Name='rootRepeater' ItemTemplate='{StaticResource GroupTemplate}' VerticalCacheLength='0' />
                    </ScrollViewer>
                </controls:ItemsRepeaterScrollHost>");

                rootRepeater = (ItemsRepeater)anchorProvider.FindName("rootRepeater");
                rootRepeater.SizeChanged += (sender, args) =>
                {
                    Log.Comment($"SizeChanged: Size=({rootRepeater.ActualWidth} x {rootRepeater.ActualHeight})");
                };

                scrollViewer = (ScrollViewer)anchorProvider.FindName("scrollviewer");
                scrollViewer.ViewChanging += (sender, args) =>
                {
                    Log.Comment($"ViewChanging: Next VerticalOffset={args.NextView.VerticalOffset}, Final VerticalOffset={args.FinalView.VerticalOffset}");
                };
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    Log.Comment($"ViewChanged: VerticalOffset={scrollViewer.VerticalOffset}, IsIntermediate={args.IsIntermediate}");

                    if (!args.IsIntermediate)
                    {
                        viewChanged.Set();
                    }
                };

                var itemsSource = new ObservableCollection<ObservableCollection<int>>();
                for (int i = 0; i < 100; i++)
                {
                    itemsSource.Add(new ObservableCollection<int>(Enumerable.Range(0, 5)));
                };

                rootRepeater.ItemsSource = itemsSource;
                Content = anchorProvider;
            });

            // scroll down several times to cause recycling of elements
            for (int i = 1; i < 10; i++)
            {
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Size=({rootRepeater.ActualWidth} x {rootRepeater.ActualHeight})");
                    Log.Comment($"ChangeView(VerticalOffset={i * 200})");
                    scrollViewer.ChangeView(null, i * 200, null, disableAnimation);
                });

                Log.Comment("Waiting for view change completion...");
                Verify.IsTrue(viewChanged.WaitOne(DefaultWaitTimeInMS));
                viewChanged.Reset();
                Log.Comment("View change completed");

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(i * 200, scrollViewer.VerticalOffset);
                });
            }
            // }
        }

        // ScrollViewer scrolls vertically, but there is an inner
        // repeater which flows horizontally which needs corrections to be handled.
        //[TestMethod] 24022837
        public void VerifyCorrectionsInNonScrollableDirection()
        {
            ItemsRepeater rootRepeater = null;
            ScrollViewer scrollViewer = null;
            ItemsRepeaterScrollHost scrollhost = null;
            ManualResetEvent viewChanged = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                scrollhost = (ItemsRepeaterScrollHost)XamlReader.Load(
                  @"<controls:ItemsRepeaterScrollHost Width='400' Height='600'
                     xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                     xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                     xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                    <ScrollViewer Width='400' Height='400' x:Name='scrollviewer'>
                        <controls:ItemsRepeater x:Name='repeater'>
                            <DataTemplate>
                                <StackPanel>
                                    <controls:ItemsRepeater ItemsSource='{Binding}'>
                                        <controls:ItemsRepeater.Layout>
                                            <controls:StackLayout Orientation='Horizontal' />
                                        </controls:ItemsRepeater.Layout>
                                    </controls:ItemsRepeater>
                                </StackPanel>
                            </DataTemplate>
                        </controls:ItemsRepeater>
                    </ScrollViewer>
                </controls:ItemsRepeaterScrollHost>");

                rootRepeater = (ItemsRepeater)scrollhost.FindName("repeater");
                scrollViewer = (ScrollViewer)scrollhost.FindName("scrollviewer");
                scrollViewer.ViewChanged += (sender, args) =>
                {
                    if (!args.IsIntermediate)
                    {
                        viewChanged.Set();
                    }
                };

                List<List<int>> items = new List<List<int>>();
                for (int i = 0; i < 100; i++)
                {
                    items.Add(Enumerable.Range(0, 4).ToList());
                }
                rootRepeater.ItemsSource = items;
                Content = scrollhost;
            });

            // scroll down several times and validate no crash
            for (int i = 1; i < 5; i++)
            {
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    scrollViewer.ChangeView(null, i * 200, null);
                });

                Verify.IsTrue(viewChanged.WaitOne(DefaultWaitTimeInMS));
                viewChanged.Reset();
            }
        }


        [TestMethod]
        public void VerifyStoreScenarioCache()
        {
            ItemsRepeater rootRepeater = null;
            RunOnUIThread.Execute(() =>
            {
                var scrollhost = (ItemsRepeaterScrollHost)XamlReader.Load(
                  @" <controls:ItemsRepeaterScrollHost Width='400' Height='200'
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <controls:ItemsRepeaterScrollHost.Resources>
                            <DataTemplate x:Key='ItemTemplate' >
                                <TextBlock Text='{Binding}' Height='100' Width='100'/>
                            </DataTemplate>
                            <DataTemplate x:Key='GroupTemplate'>
                                <StackPanel>
                                    <TextBlock Text='{Binding}' />
                                    <controls:ItemsRepeaterScrollHost>
                                        <ScrollViewer HorizontalScrollMode='Enabled' VerticalScrollMode='Disabled' HorizontalScrollBarVisibility='Auto' VerticalScrollBarVisibility='Hidden'>
                                            <controls:ItemsRepeater ItemTemplate='{StaticResource ItemTemplate}' ItemsSource='{Binding}'>
                                                <controls:ItemsRepeater.Layout>
                                                    <controls:StackLayout Orientation='Horizontal' />
                                                </controls:ItemsRepeater.Layout>
                                            </controls:ItemsRepeater>
                                        </ScrollViewer>
                                    </controls:ItemsRepeaterScrollHost>
                                </StackPanel>
                            </DataTemplate>
                        </controls:ItemsRepeaterScrollHost.Resources>
                        <ScrollViewer x:Name='scrollviewer'>
                            <controls:ItemsRepeater x:Name='rootRepeater' ItemTemplate='{StaticResource GroupTemplate}'/>
                        </ScrollViewer>
                    </controls:ItemsRepeaterScrollHost>");

                rootRepeater = (ItemsRepeater)scrollhost.FindName("rootRepeater");

                List<List<int>> items = new List<List<int>>();
                for (int i = 0; i < 100; i++)
                {
                    items.Add(Enumerable.Range(0, 4).ToList());
                }
                rootRepeater.ItemsSource = items;
                Content = scrollhost;
            });

            IdleSynchronizer.Wait();

            // Verify that first items outside the visible range but in the realized range
            // for the inner of the nested repeaters are realized.
            RunOnUIThread.Execute(() =>
            {
                // Group2 will be outside the visible range but within the realized range.
                var group2 = rootRepeater.TryGetElement(2) as StackPanel;
                Verify.IsNotNull(group2);

                var group2Repeater = ((ItemsRepeaterScrollHost)group2.Children[1]).ScrollViewer.Content as ItemsRepeater;
                Verify.IsNotNull(group2Repeater);

                Verify.IsNotNull(group2Repeater.TryGetElement(0));
            });
        }


        [TestMethod]
        public void VerifyUIElementsInItemsSource()
        {
            ItemsRepeater repeater = null;
            RunOnUIThread.Execute(() =>
            {
                var scrollhost = (ItemsRepeaterScrollHost)XamlReader.Load(
                  @"<controls:ItemsRepeaterScrollHost
                     xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                     xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                     xmlns:local='using:MUXControlsTestApp.Samples'
                     xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <ScrollViewer>
                            <controls:ItemsRepeater x:Name='repeater'>
                                <controls:ItemsRepeater.ItemsSource>
                                    <local:UICollection>
                                        <Button>0</Button>
                                        <Button>1</Button>
                                        <Button>2</Button>
                                        <Button>3</Button>
                                        <Button>4</Button>
                                        <Button>5</Button>
                                        <Button>6</Button>
                                        <Button>7</Button>
                                        <Button>8</Button>
                                        <Button>9</Button>
                                    </local:UICollection>
                                </controls:ItemsRepeater.ItemsSource>
                            </controls:ItemsRepeater>
                        </ScrollViewer>
                    </controls:ItemsRepeaterScrollHost>");

                repeater = (ItemsRepeater)scrollhost.FindName("repeater");
                Content = scrollhost;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                for (int i = 0; i < 10; i++)
                {
                    var element = repeater.TryGetElement(i) as Button;
                    Verify.AreEqual(i.ToString(), element.Content);
                }
            });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Task 32541584: Failing test: RepeaterTests.VerifyRepeaterDoesNotLeakItemContainers
        public void VerifyRepeaterDoesNotLeakItemContainers()
        {
            ObservableCollection<int> items = new ObservableCollection<int>();
            for (int i = 0; i < 10; i++)
            {
                items.Add(i);
            }

            ItemsRepeater repeater = null;

            RunOnUIThread.Execute(() =>
            {
                var template = (DataTemplate)XamlReader.Load("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                    + "xmlns:local='using:MUXControlsTestApp.Samples'>"
                    + "<local:DisposableUserControl Number='{Binding}'/>"
                    + "</DataTemplate>");
                Verify.IsNotNull(template);
                Verify.AreEqual(0, MUXControlsTestApp.Samples.DisposableUserControl.OpenItems, "Verify we start with 0 DisposableUserControl");

                repeater = new ItemsRepeater() {
                    ItemsSource = items,
                    ItemTemplate = template,
                    VerticalAlignment = VerticalAlignment.Top,
                    HorizontalAlignment = HorizontalAlignment.Left
                };

                Content = repeater;

            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {

                Verify.IsGreaterThanOrEqual(MUXControlsTestApp.Samples.DisposableUserControl.OpenItems, 10, "Verify we created at least 10 DisposableUserControl");

                // Clear out the repeater and make sure everything gets cleaned up.
                Content = null;
                repeater = null;
            });

            IdleSynchronizer.Wait();

            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();

            Verify.AreEqual(0, MUXControlsTestApp.Samples.DisposableUserControl.OpenItems, "Verify we cleaned up all the DisposableUserControl that were created");
        }

        [TestMethod]
        public void BringIntoViewOfExistingItemsDoesNotChangeScrollOffset()
        {
            ScrollViewer scrollViewer = null;
            ItemsRepeater repeater = null;
            AutoResetEvent scrollViewerScrolledEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                repeater = new ItemsRepeater();
                repeater.ItemsSource = Enumerable.Range(0, 100).Select(x => x.ToString()).ToList();

                scrollViewer = new ScrollViewer() {
                    Content = repeater,
                    MaxHeight = 400,
                    MaxWidth = 200
                };


                Content = scrollViewer;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Scroll to end");
                scrollViewer.ViewChanged += (object sender, ScrollViewerViewChangedEventArgs e) =>
                {
                    if (!e.IsIntermediate)
                    {
                        Log.Comment("ScrollViewer scrolling finished");
                        scrollViewerScrolledEvent.Set();
                    }
                };
                scrollViewer.ChangeView(null, repeater.ActualHeight, null);
                scrollViewer.UpdateLayout();
            });

            Log.Comment("Wait for scrolling");
            if (Debugger.IsAttached)
            {
                scrollViewerScrolledEvent.WaitOne();
            }
            else
            {
                if (!scrollViewerScrolledEvent.WaitOne(TimeSpan.FromMilliseconds(5000)))
                {
                    throw new Exception("Timeout expiration in WaitForEvent.");
                }
            }

            IdleSynchronizer.Wait();

            double endOfScrollOffset = 0;
            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Determine scrolled offset");
                endOfScrollOffset = scrollViewer.VerticalOffset;
                // Idea: we might not have scrolled to the end, however we should at least have moved so much that the end is not too far away
                Verify.IsTrue(Math.Abs(endOfScrollOffset - repeater.ActualHeight) < 500, $"We should at least have scrolled some amount. " +
                    $"ScrollOffset:{endOfScrollOffset} Repeater height: {repeater.ActualHeight}");

                var lastItem = repeater.GetOrCreateElement(99);
                lastItem.UpdateLayout();
                Log.Comment("Bring last element into view");
                lastItem.StartBringIntoView();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verify position did not change");
                Verify.IsTrue(Math.Abs(endOfScrollOffset - scrollViewer.VerticalOffset) < 1);
            });
        }

        // Verifies that an ItemsRepeater using a DataTemplate can be garbage
        // collected after elements are recycled to the RecyclePool and the
        // repeater is removed from the tree. The recycling step is critical —
        // it creates the RecyclePool on the DataTemplate and populates it,
        // which forms the reference cycle that must be breakable by the tracker.
        [TestMethod]
        public void VerifyRepeaterWithRecycledElementsDoesNotLeak()
        {
            WeakReference repeaterWeakRef = null;
            var data = new ObservableCollection<string>(
                Enumerable.Range(0, 5).Select(i => string.Format("Item #{0}", i)));

            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater()
                {
                    ItemsSource = data,
                    ItemTemplate = CreateDataTemplateWithContent(@"<TextBlock Text='{Binding}' Height='50' />"),
                };
                repeaterWeakRef = new WeakReference(repeater);
                Content = repeater;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Verify elements are realized, then clear the data source to
                // force all elements into the RecyclePool on the DataTemplate.
                Verify.IsNotNull(repeaterWeakRef.Target);
                var repeater = (ItemsRepeater)repeaterWeakRef.Target;
                Verify.IsNotNull(repeater.TryGetElement(0), "Element 0 should be realized.");
                data.Clear();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Remove from tree. The RecyclePool now holds recycled elements
                // that reference the DataTemplate and the repeater (as owner).
                Content = null;
            });

            IdleSynchronizer.Wait();

            // Force GC — the tracker_ref on ItemTemplateWrapper's DataTemplate
            // makes the reference visible to the XAML reference tracker, which
            // detects the RecyclePool cycle and breaks it during collection.
            for (int i = 0; i < 5 && repeaterWeakRef.IsAlive; i++)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                IdleSynchronizer.Wait();
            }

            Verify.IsFalse(repeaterWeakRef.IsAlive,
                "ItemsRepeater should be collected after recycling elements and removal.");
        }

        // Scenario: an ItemsRepeater with a NonVirtualizingLayout and a DataTemplate has its ItemsSource replaced (its
        //           elements go to the template's RecyclePool with the repeater as owner), then it is removed from the tree.
        // Expected: the repeater is garbage collected.
        // A failure means: the RecyclePool entries keep a repeater with a NonVirtualizingLayout alive after an ItemsSource
        //           replacement.
        [TestMethod]
        [TestProperty("Description", "Verifies an ItemsRepeater with a NonVirtualizingLayout is collected after an ItemsSource replacement recycles its elements and it is removed from the tree.")]
        public void VerifyNonVirtualizingRepeaterDoesNotLeakAfterItemsSourceReplacement()
        {
            WeakReference repeaterWeakRef = null;

            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater()
                {
                    ItemsSource = Enumerable.Range(0, 5).Select(i => string.Format("Item #{0}", i)).ToList(),
                    ItemTemplate = CreateDataTemplateWithContent(@"<TextBlock Text='{Binding}' Height='50' />"),
                    Layout = new MyCustomNonVirtualizingStackLayout(),
                };
                repeaterWeakRef = new WeakReference(repeater);
                Content = repeater;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var repeater = (ItemsRepeater)repeaterWeakRef.Target;
                Verify.IsNotNull(repeater.TryGetElement(0), "Element 0 should be realized.");
                repeater.ItemsSource = new List<string>();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content = null;
            });

            IdleSynchronizer.Wait();

            for (int i = 0; i < 5 && repeaterWeakRef.IsAlive; i++)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                IdleSynchronizer.Wait();
            }

            Verify.IsFalse(repeaterWeakRef.IsAlive,
                "ItemsRepeater with a NonVirtualizingLayout should be collected after an ItemsSource replacement and removal.");
        }

        // Verifies that an ItemsRepeater using a RecyclingElementFactory (the
        // DataTemplateSelector equivalent) can be garbage collected after
        // elements are recycled and the repeater is removed from the tree.
        [TestMethod]
        public void VerifyRepeaterWithRecyclingElementFactoryDoesNotLeak()
        {
            WeakReference repeaterWeakRef = null;
            var data = new ObservableCollection<int>(Enumerable.Range(0, 6));

            RunOnUIThread.Execute(() =>
            {
                var elementFactory = new RecyclingElementFactory()
                {
                    RecyclePool = new RecyclePool(),
                };
                elementFactory.Templates["even"] = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <TextBlock Text='even' Height='50' />
                    </DataTemplate>");
                elementFactory.Templates["odd"] = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <TextBlock Text='odd' Height='50' />
                    </DataTemplate>");

                elementFactory.SelectTemplateKey +=
                    delegate (RecyclingElementFactory sender, SelectTemplateEventArgs args)
                    {
                        args.TemplateKey = ((int)args.DataContext % 2 == 0) ? "even" : "odd";
                    };

                var repeater = new ItemsRepeater()
                {
                    ItemsSource = data,
                    ItemTemplate = elementFactory,
                };
                repeaterWeakRef = new WeakReference(repeater);
                Content = repeater;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Clear items to force elements into the RecyclePool, then remove.
                data.Clear();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content = null;
            });

            IdleSynchronizer.Wait();

            for (int i = 0; i < 5 && repeaterWeakRef.IsAlive; i++)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                IdleSynchronizer.Wait();
            }

            Verify.IsFalse(repeaterWeakRef.IsAlive,
                "ItemsRepeater with RecyclingElementFactory should be collected after recycling and removal.");
        }

        // Verifies a code-authored DataTemplate (constructed with an element-factory callback, not markup)
        // realized by ItemsRepeater, including recycling: the callback produces a distinct element per item,
        // ItemsRepeater sets DataContext on each (raising DataContextChanged) so a classic {Binding} resolves,
        // and after clearing and re-populating the source the pooled elements are reused (callback not called
        // again) yet still receive the new item as DataContext, re-raise DataContextChanged, and update their
        // binding.
        [TestMethod]
        public void ValidateCodeAuthoredDataTemplateReceivesDataContext()
        {
            var items = new ObservableCollection<string>(new[] { "red", "green", "blue" });
            ItemsRepeater repeater = null;
            int callbackCount = 0;
            var created = new List<UIElement>();
            var changedContexts = new Dictionary<TextBlock, object>();

            RunOnUIThread.Execute(() =>
            {
                var template = new DataTemplate(() =>
                {
                    callbackCount++;
                    var tb = new TextBlock();
                    created.Add(tb);
                    tb.DataContextChanged += (sender, args) =>
                    {
                        changedContexts[(TextBlock)sender] = args.NewValue;
                    };
                    tb.SetBinding(TextBlock.TextProperty, new Microsoft.UI.Xaml.Data.Binding());
                    return tb;
                });

                repeater = new ItemsRepeater()
                {
                    ItemsSource = items,
                    ItemTemplate = template,
                };

                Content = new ItemsRepeaterScrollHost()
                {
                    Width = 400,
                    Height = 800,
                    ScrollViewer = new ScrollViewer { Content = repeater }
                };

                Content.UpdateLayout();

                // The default StackLayout creates one extra "prototype" element at index 0 via
                // GetOrCreateElementAt(ForceCreate) to estimate the average item size
                // (StackLayout::GetAverageElementSize), so callbackCount can exceed items.Count.
                // Assert the intent (at least one element per item) rather than an exact count.
                Verify.IsTrue(callbackCount >= items.Count,
                    string.Format("Callback should be invoked at least once per item (was {0} for {1} items).", callbackCount, items.Count));

                VerifyRealizedElements(repeater, items, created, changedContexts);

                // The callback returns a fresh instance every time it is invoked (never the same element twice).
                Verify.AreEqual(created.Count, created.Distinct().Count());

                int countAfterFirstRealization = callbackCount;

                // Recycle: clearing the source pushes realized elements into the pool; re-populating with the
                // same count should reuse them rather than invoke the callback again. Reset the recorded
                // contexts first so the checks below prove the event fired again on the reused elements.
                changedContexts.Clear();
                items.Clear();
                Content.UpdateLayout();

                items.Add("cyan");
                items.Add("magenta");
                items.Add("yellow");
                Content.UpdateLayout();

                Verify.AreEqual(countAfterFirstRealization, callbackCount,
                    "Recycled elements should be reused from the pool, so the callback is not invoked again.");

                // Reused elements still receive the new item as DataContext, re-raising DataContextChanged, so
                // their {Binding} updates to the new value.
                VerifyRealizedElements(repeater, items, created, changedContexts);
            });
        }

        // Asserts each realized ItemsRepeater element reflects its item via DataContext, a {Binding} on Text,
        // and DataContextChanged, and that every element is a distinct instance the callback produced.
        private static void VerifyRealizedElements(ItemsRepeater repeater, IList<string> items, List<UIElement> created, Dictionary<TextBlock, object> changedContexts)
        {
            var realized = new List<UIElement>();
            for (int i = 0; i < items.Count; i++)
            {
                var element = repeater.TryGetElement(i) as TextBlock;
                Verify.IsNotNull(element, $"Element {i} should be realized from the code-authored template.");
                Verify.IsTrue(created.Contains(element), "Realized element should be one produced by the callback.");
                Verify.IsTrue(changedContexts.ContainsKey(element), $"DataContextChanged should have fired for element {i}.");
                Verify.AreEqual(items[i], changedContexts[element] as string);
                Verify.AreEqual(items[i], element.DataContext as string);
                // {Binding} inside the code-built subtree resolved against that DataContext.
                Verify.AreEqual(items[i], element.Text);
                realized.Add(element);
            }

            // Each item maps to a distinct element instance.
            Verify.AreEqual(items.Count, realized.Distinct().Count());
        }

        // Scenario: call GetOrCreateElement without an ItemsSource and with indexes outside the items range.
        // Expected: without a source the call fails with E_FAIL; out-of-range indexes fail with E_INVALIDARG.
        // A failure means: invalid element requests could crash or return elements for items that do not exist.
        [TestMethod]
        [TestProperty("Description", "Verifies GetOrCreateElement fails with E_FAIL without an ItemsSource and with E_INVALIDARG for out-of-range indexes.")]
        public void VerifyGetOrCreateElementRejectsInvalidRequests()
        {
            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater();
                Verify.IsNull(repeater.ItemsSourceView);
                VerifyThrowsWithHResult(E_FAIL, () => repeater.GetOrCreateElement(0), "GetOrCreateElement without ItemsSource");

                repeater.ItemsSource = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList();
                Content = repeater;
                Content.UpdateLayout();

                VerifyThrowsWithHResult(E_INVALIDARG, () => repeater.GetOrCreateElement(-1), "GetOrCreateElement(-1)");
                VerifyThrowsWithHResult(E_INVALIDARG, () => repeater.GetOrCreateElement(3), "GetOrCreateElement(Count)");

                var element = repeater.GetOrCreateElement(2);
                Verify.IsNotNull(element, "GetOrCreateElement(Count - 1) is valid.");
                Verify.AreEqual(2, repeater.GetElementIndex(element));
            });
        }

        // Scenario: from inside a custom layout's measure and arrange passes, call GetOrCreateElement and change
        //           ItemsSource, ItemTemplate and Layout.
        // Expected: all four calls fail with E_FAIL, the original items and template stay realized, and the original
        //           layout is still in use and was never uninitialized.
        // A failure means: changing the repeater in the middle of layout could corrupt its elements or crash the app.
        [TestMethod]
        [TestProperty("Description", "Verifies that GetOrCreateElement and changes to ItemsSource, ItemTemplate and Layout are rejected with E_FAIL while the ItemsRepeater runs layout, leaving the processed repeater state unchanged.")]
        public void VerifyRepeaterApisThrowWhenCalledDuringLayout()
        {
            RunOnUIThread.Execute(() =>
            {
                var originalSource = Enumerable.Range(0, 5).Select(i => "Item #" + i).ToList();
                var originalTemplate = CreateDataTemplate("<TextBlock Text='{Binding}' Height='20'/>");
                var layout = new UninitializeCountingLayout();
                var repeater = new ItemsRepeater() {
                    ItemsSource = originalSource,
                    ItemTemplate = originalTemplate,
                    Layout = layout
                };

                var failures = new Dictionary<string, int>();
                bool attempted = false;
                bool layoutChangeAttempted = false;
                int layoutRestoreHResult = 0;
                layout.MeasureLayoutFunc = (availableSize, context) =>
                {
                    if (!attempted)
                    {
                        attempted = true;
                        failures["GetOrCreateElement"] = CaptureHResult(() => repeater.GetOrCreateElement(0));
                        failures["ItemsSource"] = CaptureHResult(() => repeater.ItemsSource = new List<string>() { "Other" });
                        failures["ItemTemplate"] = CaptureHResult(() => repeater.ItemTemplate = CreateDataTemplate("<Button Content='{Binding}'/>"));
                        // A Layout change is attempted from the arrange pass below instead: attempted here, the arrange
                        // that follows in the same pass would run the rejected, never-initialized layout instance.
                    }

                    for (int i = 0; i < context.ItemCount; i++)
                    {
                        var element = context.GetOrCreateElementAt(i);
                        element.Measure(availableSize);
                    }

                    return new Size(100, 20 * context.ItemCount);
                };
                layout.ArrangeLayoutFunc = (finalSize, context) =>
                {
                    for (int i = 0; i < context.ItemCount; i++)
                    {
                        context.GetOrCreateElementAt(i).Arrange(new Rect(0, 20 * i, 100, 20));
                    }

                    if (!layoutChangeAttempted)
                    {
                        // Attempted after this pass arranged its elements, so nothing else in the pass uses the new value.
                        layoutChangeAttempted = true;
                        failures["Layout"] = CaptureHResult(() => repeater.Layout = new StackLayout());

                        // The rejected value is still stored in the Layout property, and a later layout pass would run
                        // that never-initialized layout. Put the original layout back (also rejected during layout, but it
                        // restores the property value) so the repeater stays consistent for the checks below.
                        layoutRestoreHResult = CaptureHResult(() => repeater.Layout = layout);
                    }
                    return finalSize;
                };

                Content = repeater;
                Content.UpdateLayout();

                Verify.IsTrue(attempted);
                Verify.IsTrue(layoutChangeAttempted);
                Verify.AreEqual(4, failures.Count);
                foreach (var failure in failures)
                {
                    Log.Comment($"{failure.Key} during layout: HResult=0x{failure.Value:X8}");
                    Verify.AreEqual(E_FAIL, failure.Value, failure.Key + " change during layout must fail with E_FAIL.");
                }

                // The rejected changes were not applied to the processed repeater state.
                Verify.AreEqual(5, repeater.ItemsSourceView.Count);
                Verify.AreEqual("Item #0", repeater.ItemsSourceView.GetAt(0));
                for (int i = 0; i < 5; i++)
                {
                    var element = repeater.TryGetElement(i) as TextBlock;
                    Verify.IsNotNull(element, $"Element {i} is still produced by the original template.");
                    Verify.AreEqual("Item #" + i, element.Text);
                }

                // The rejected Layout change never reached the layout: the original layout was not uninitialized
                // (its realized elements, verified above, were not cleared).
                Verify.AreEqual(E_FAIL, layoutRestoreHResult, "Restoring Layout during layout is rejected as well.");
                Verify.AreSame(layout, repeater.Layout);
                Verify.AreEqual(0, layout.UninitializeForContextCallCount, "The original layout must not be uninitialized by a rejected Layout change.");

                Content = null;
            });
        }

        // Scenario: raise a collection change notification from the data source while the repeater runs layout.
        // Expected: the change is rejected with E_FAIL.
        // A failure means: data changes during layout could leave realized elements out of sync with the data.
        [TestMethod]
        [TestProperty("Description", "Verifies that a data source change raised while the ItemsRepeater runs layout is rejected with E_FAIL.")]
        public void VerifyCollectionChangeDuringLayoutThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new Common.CustomItemsSource(Enumerable.Range(0, 5).ToList());
                var layout = new MockVirtualizingLayout();
                var repeater = new ItemsRepeater() { ItemsSource = dataSource, Layout = layout };
                int hresult = 0;
                int measureCount = 0;

                layout.MeasureLayoutFunc = (availableSize, context) =>
                {
                    if (measureCount++ == 0)
                    {
                        Verify.AreEqual(5, context.ItemCount);
                        hresult = CaptureHResult(() => dataSource.Insert(index: 0, count: 1, reset: false));
                    }
                    return new Size(100, 100);
                };
                layout.ArrangeLayoutFunc = (finalSize, context) => finalSize;

                Content = repeater;
                Content.UpdateLayout();

                Log.Comment($"Collection change during layout: HResult=0x{hresult:X8}");
                Verify.AreEqual(E_FAIL, hresult);
                Content = null;
            });
        }

        // Scenario: from inside a custom layout's measure and arrange passes, invalidate the repeater and call its Measure or
        //           Arrange again.
        // Expected: the nested measure and the nested arrange are rejected with E_FAIL, and the outer layout pass completes
        //           with every item realized.
        // A failure means: re-entrant layout could corrupt the repeater's realized elements or loop endlessly.
        [TestMethod]
        [TestProperty("Description", "Verifies a nested ItemsRepeater measure or arrange started from within its own layout pass is rejected with E_FAIL.")]
        public void VerifyReentrantMeasureAndArrangeAreRejected()
        {
            RunOnUIThread.Execute(() =>
            {
                var layout = new MockVirtualizingLayout();
                var repeater = new ItemsRepeater() {
                    ItemsSource = Enumerable.Range(0, 3).ToList(),
                    ItemTemplate = CreateDataTemplate("<TextBlock Text='{Binding}' Height='20'/>"),
                    Layout = layout
                };
                int measureHResult = 0, arrangeHResult = 0;
                bool measureAttempted = false, arrangeAttempted = false;
                layout.MeasureLayoutFunc = (availableSize, context) =>
                {
                    if (!measureAttempted)
                    {
                        measureAttempted = true;
                        repeater.InvalidateMeasure();
                        measureHResult = CaptureHResult(() => repeater.Measure(availableSize));
                    }
                    for (int i = 0; i < context.ItemCount; i++)
                    {
                        context.GetOrCreateElementAt(i).Measure(availableSize);
                    }
                    return new Size(100, 20 * context.ItemCount);
                };
                layout.ArrangeLayoutFunc = (finalSize, context) =>
                {
                    if (!arrangeAttempted)
                    {
                        arrangeAttempted = true;
                        repeater.InvalidateArrange();
                        arrangeHResult = CaptureHResult(() => repeater.Arrange(new Rect(0, 0, finalSize.Width, finalSize.Height)));
                    }
                    for (int i = 0; i < context.ItemCount; i++)
                    {
                        context.GetOrCreateElementAt(i).Arrange(new Rect(0, 20 * i, 100, 20));
                    }
                    return finalSize;
                };

                Content = repeater;
                Content.UpdateLayout();

                Verify.IsTrue(measureAttempted && arrangeAttempted);
                Verify.AreEqual(E_FAIL, measureHResult, "Nested measure");
                Verify.AreEqual(E_FAIL, arrangeHResult, "Nested arrange");
                for (int i = 0; i < 3; i++)
                {
                    Verify.IsNotNull(repeater.TryGetElement(i), $"Item {i} is realized by the outer pass.");
                }
                Content = null;
            });
        }

        // Scenario: while the repeater's layout is notified of a data source change, first arrange the repeater again (its
        //           measure is still valid, so only ArrangeOverride runs), then invalidate it and force a layout pass.
        // Expected: both the arrange and the forced layout are rejected with E_FAIL; after the change completes, layout
        //           shows the updated items.
        // A failure means: layout could run against half-updated data and show wrong or duplicated items.
        [TestMethod]
        [TestProperty("Description", "Verifies that running ItemsRepeater arrange or layout while it processes a data source change is rejected with E_FAIL.")]
        public void VerifyLayoutDuringCollectionChangeIsRejected()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new Common.CustomItemsSource(Enumerable.Range(0, 5).ToList());
                var layout = new ItemsChangedCallbackStackLayout();
                var repeater = new ItemsRepeater() {
                    ItemsSource = dataSource,
                    Layout = layout,
                    ItemTemplate = CreateDataTemplate("<TextBlock Text='{Binding}' Height='20'/>")
                };
                Content = repeater;
                Content.UpdateLayout();

                int measureProbeHResult = -1, measureHResult = 0, arrangeHResult = 0, callCount = 0;
                layout.ItemsChangedFunc = (args) =>
                {
                    if (callCount++ == 0)
                    {
                        // Arrange first, while the repeater's measure is still valid. Measuring again with the previous
                        // constraint is then a no-op (it succeeds), which proves that the following Arrange call reaches
                        // ItemsRepeater's ArrangeOverride rather than re-running MeasureOverride.
                        var availableSize = Microsoft.UI.Xaml.Controls.Primitives.LayoutInformation.GetAvailableSize(repeater);
                        measureProbeHResult = CaptureHResult(() => repeater.Measure(availableSize));
                        repeater.InvalidateArrange();
                        arrangeHResult = CaptureHResult(() => repeater.Arrange(Microsoft.UI.Xaml.Controls.Primitives.LayoutInformation.GetLayoutSlot(repeater)));

                        repeater.InvalidateMeasure();
                        measureHResult = CaptureHResult(() => repeater.UpdateLayout());
                    }
                };

                dataSource.Insert(index: 0, count: 1, reset: false);
                Content.UpdateLayout();

                Log.Comment($"Measure probe: 0x{measureProbeHResult:X8}, arrange during change: 0x{arrangeHResult:X8}, measure during change: 0x{measureHResult:X8}");
                Verify.AreEqual(0, measureProbeHResult, "The repeater's measure was still valid when the arrange check ran.");
                Verify.AreEqual(E_FAIL, arrangeHResult, "Arrange during the collection change");
                Verify.AreEqual(E_FAIL, measureHResult, "Layout (measure) during the collection change");
                Verify.AreEqual(6, repeater.ItemsSourceView.Count);
                Verify.IsNotNull(repeater.TryGetElement(5), "After the change, the new item count is laid out.");
                Content = null;
            });
        }

        // Scenario: raise a second collection change from inside the handling of a first one.
        // Expected: the nested change is rejected with E_FAIL.
        // A failure means: nested data changes could leave the repeater's element indexes inconsistent.
        [TestMethod]
        [TestProperty("Description", "Verifies that a data source change raised while the ItemsRepeater processes another data source change is rejected with E_FAIL.")]
        public void VerifyCollectionChangeDuringCollectionChangeThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                var dataSource = new Common.CustomItemsSource(Enumerable.Range(0, 5).ToList());
                var layout = new ItemsChangedCallbackStackLayout();
                var repeater = new ItemsRepeater() {
                    ItemsSource = dataSource,
                    Layout = layout,
                    ItemTemplate = CreateDataTemplate("<TextBlock Text='{Binding}' Height='20'/>")
                };
                Content = repeater;
                Content.UpdateLayout();

                int nestedHResult = 0;
                int itemsChangedCount = 0;
                layout.ItemsChangedFunc = (args) =>
                {
                    if (itemsChangedCount++ == 0)
                    {
                        Verify.AreEqual(global::System.Collections.Specialized.NotifyCollectionChangedAction.Remove, args.Action);
                        nestedHResult = CaptureHResult(() => dataSource.Insert(index: 0, count: 1, reset: false));
                    }
                };

                dataSource.Remove(index: 4, count: 1, reset: false);

                Log.Comment($"Nested collection change: HResult=0x{nestedHResult:X8}");
                Verify.AreEqual(1, itemsChangedCount, "Only the outer change reaches the layout.");
                Verify.AreEqual(E_FAIL, nestedHResult);
                Content = null;
            });
        }

        // Scenario: set ItemTemplate to an object that is neither a template, a template selector nor an element
        //           factory.
        // Expected: the assignment fails with E_INVALIDARG and the previous template keeps producing the elements.
        // A failure means: an invalid template could be accepted silently and items would disappear or fail later.
        [TestMethod]
        [TestProperty("Description", "Verifies that setting ItemTemplate to an object that is not an element factory is rejected and the previous template keeps producing elements.")]
        public void VerifyInvalidItemTemplateTypeIsRejected()
        {
            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater() {
                    ItemsSource = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = CreateDataTemplate("<TextBlock Text='{Binding}' Height='20'/>")
                };
                Content = repeater;
                Content.UpdateLayout();

                int hresult = CaptureHResult(() => repeater.ItemTemplate = "not an element factory");
                Log.Comment($"ItemTemplate=string: HResult=0x{hresult:X8}");
                Verify.AreNotEqual(0, hresult, "A non-IElementFactory ItemTemplate must be rejected.");

                Content.UpdateLayout();
                for (int i = 0; i < 3; i++)
                {
                    var element = repeater.TryGetElement(i) as TextBlock;
                    Verify.IsNotNull(element, $"Element {i} is still produced by the original template.");
                    Verify.AreEqual("Item #" + i, element.Text);
                }
            });
        }

        // Scenario: show items with a custom DataTemplate, then set ItemTemplate back to null.
        // Expected: the change is accepted and items are shown with the default TextBlock template.
        // Ignored: reproduces clearing ItemTemplate being rejected (PC-ITEMTEMPLATE-NULL); currently fails because the
        //          assignment throws E_INVALIDARG and the old template keeps being used.
        [TestMethod]

        [TestProperty("Ignore", "True")] // Product concern PC-ITEMTEMPLATE-NULL (bug pending): setting ItemTemplate to null throws E_INVALIDARG (ItemsRepeater.cpp OnItemTemplateChanged) and leaves the old template active.
        [TestProperty("Description", "Verifies that clearing ItemTemplate back to null is accepted and items fall back to the default TextBlock template.")]
        public void VerifyClearingItemTemplateFallsBackToDefaultTemplate()
        {
            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater() {
                    ItemsSource = Enumerable.Range(0, 3).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = CreateDataTemplate("<Button Content='{Binding}' Height='20'/>")
                };
                Content = repeater;
                Content.UpdateLayout();
                Verify.IsTrue(repeater.TryGetElement(0) is Button);

                int hresult = CaptureHResult(() => repeater.ItemTemplate = null);
                Log.Comment($"ItemTemplate=null: HResult=0x{hresult:X8}");
                Verify.AreEqual(0, hresult, "Clearing ItemTemplate is expected to be accepted.");

                Content.UpdateLayout();
                for (int i = 0; i < 3; i++)
                {
                    var element = repeater.TryGetElement(i) as TextBlock;
                    Verify.IsNotNull(element, $"Element {i} uses the default TextBlock template.");
                    Verify.AreEqual("Item #" + i, element.Text);
                }
            });
        }

        // Scenario: replace the repeater's Layout while items are realized, then lay out again.
        // Expected: every realized element is cleared synchronously and the new layout realizes the items again.
        // A failure means: elements from the old layout could leak or be shown at stale positions after a layout
        //                  change.
        [TestMethod]
        [TestProperty("Description", "Verifies that replacing the Layout synchronously clears every realized element and that the new layout realizes them again.")]
        public void VerifyChangingLayoutClearsRealizedElements()
        {
            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater() {
                    ItemsSource = Enumerable.Range(0, 5).Select(i => "Item #" + i).ToList(),
                    ItemTemplate = CreateDataTemplate("<TextBlock Text='{Binding}' Height='20' Width='50'/>"),
                    Layout = new StackLayout()
                };
                Content = repeater;
                Content.UpdateLayout();

                var realized = Enumerable.Range(0, 5).Select(i => repeater.TryGetElement(i)).ToList();
                Verify.IsTrue(realized.All(e => e != null));

                var cleared = new List<UIElement>();
                var prepared = new List<int>();
                repeater.ElementClearing += (sender, args) => cleared.Add(args.Element);
                repeater.ElementPrepared += (sender, args) => prepared.Add(args.Index);

                var newLayout = new UniformGridLayout() { MinItemWidth = 50, MinItemHeight = 20 };
                repeater.Layout = newLayout;

                Verify.AreEqual(5, cleared.Count, "All realized elements are cleared synchronously when the Layout changes.");
                Verify.IsTrue(realized.All(e => cleared.Contains(e)));
                Verify.AreEqual(0, prepared.Count);
                for (int i = 0; i < 5; i++)
                {
                    Verify.IsNull(repeater.TryGetElement(i), $"Element {i} is no longer realized.");
                }

                Content.UpdateLayout();
                Verify.AreEqual(5, prepared.Distinct().Count(), "The new layout realizes every item.");
                for (int i = 0; i < 5; i++)
                {
                    var element = repeater.TryGetElement(i) as TextBlock;
                    Verify.IsNotNull(element);
                    Verify.AreEqual("Item #" + i, element.Text);
                }
            });
        }

        // Scenario: set HorizontalCacheLength and VerticalCacheLength to negative, infinite, NaN and zero values.
        // Expected: invalid values fail with E_INVALIDARG and zero is accepted.
        // A failure means: invalid cache lengths could be accepted and make the repeater realize no or unlimited items.
        [TestMethod]
        [TestProperty("Description", "Verifies HorizontalCacheLength and VerticalCacheLength reject negative, infinite and NaN values with E_INVALIDARG and accept zero.")]
        public void VerifyInvalidCacheLengthsAreRejected()
        {
            RunOnUIThread.Execute(() =>
            {
                var repeater = new ItemsRepeater();
                foreach (var invalid in new double[] { -1.0, double.PositiveInfinity, double.NaN })
                {
                    Verify.AreEqual(E_INVALIDARG, CaptureHResult(() => repeater.HorizontalCacheLength = invalid), $"HorizontalCacheLength={invalid}");
                    Verify.AreEqual(E_INVALIDARG, CaptureHResult(() => repeater.VerticalCacheLength = invalid), $"VerticalCacheLength={invalid}");
                }

                Verify.AreEqual(0, CaptureHResult(() => repeater.HorizontalCacheLength = 0.0));
                Verify.AreEqual(0, CaptureHResult(() => repeater.VerticalCacheLength = 0.0));
                Verify.AreEqual(0.0, repeater.HorizontalCacheLength);
                Verify.AreEqual(0.0, repeater.VerticalCacheLength);
            });
        }

        private static DataTemplate CreateDataTemplate(string content)
        {
            return (DataTemplate)XamlReader.Load(
                @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" + content + "</DataTemplate>");
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

        private static void VerifyThrowsWithHResult(int expectedHResult, Action action, string context)
        {
            int hresult = CaptureHResult(action);
            Verify.AreEqual(expectedHResult, hresult, context);
        }

        private partial class ItemsChangedCallbackStackLayout : StackLayout
        {
            public Action<global::System.Collections.Specialized.NotifyCollectionChangedEventArgs> ItemsChangedFunc { get; set; }

            protected override void OnItemsChangedCore(VirtualizingLayoutContext context, object source, global::System.Collections.Specialized.NotifyCollectionChangedEventArgs args)
            {
                ItemsChangedFunc?.Invoke(args);
                base.OnItemsChangedCore(context, source, args);
            }
        }

        // Element factory that runs ReentrantAction once from RecycleElement and records args.Element and args.Parent
        // before and after it.
        private partial class ArgsRereadingElementFactory : ElementFactory
        {
            public class RecycleCall
            {
                public UIElement ElementBefore;
                public UIElement ParentBefore;
                public UIElement ElementAfter;
                public UIElement ParentAfter;
            }

            public Action ReentrantAction { get; set; }
            public List<UIElement> Created { get; } = new List<UIElement>();
            public List<RecycleCall> Calls { get; } = new List<RecycleCall>();

            protected override UIElement GetElementCore(ElementFactoryGetArgs args)
            {
                var element = new ContentControl { Width = 40, Height = 40 };
                Created.Add(element);
                return element;
            }

            protected override void RecycleElementCore(ElementFactoryRecycleArgs args)
            {
                var call = new RecycleCall { ElementBefore = args.Element, ParentBefore = args.Parent };
                var reentrantAction = ReentrantAction;
                ReentrantAction = null;
                reentrantAction?.Invoke();
                call.ElementAfter = args.Element;
                call.ParentAfter = args.Parent;
                Calls.Add(call);
            }
        }

        // Element factory whose RecycleElement throws once when ThrowOnNextRecycle is set, and records the args object
        // and args.Element and args.Parent of every call.
        private partial class ThrowOnceElementFactory : ElementFactory
        {
            public class RecycleCall
            {
                public ElementFactoryRecycleArgs Args;
                public UIElement ElementBefore;
                public UIElement ParentBefore;
                public bool Threw;
            }

            public bool ThrowOnNextRecycle { get; set; }
            public List<UIElement> Created { get; } = new List<UIElement>();
            public List<RecycleCall> Calls { get; } = new List<RecycleCall>();

            protected override UIElement GetElementCore(ElementFactoryGetArgs args)
            {
                var element = new ContentControl { Width = 40, Height = 40 };
                Created.Add(element);
                return element;
            }

            protected override void RecycleElementCore(ElementFactoryRecycleArgs args)
            {
                var call = new RecycleCall { Args = args, ElementBefore = args.Element, ParentBefore = args.Parent, Threw = ThrowOnNextRecycle };
                Calls.Add(call);
                if (ThrowOnNextRecycle)
                {
                    ThrowOnNextRecycle = false;
                    Log.Comment("Throwing from RecycleElement");
                    throw new InvalidOperationException("RecycleElement failed (test).");
                }
            }
        }

        private partial class UninitializeCountingLayout : MockVirtualizingLayout
        {
            public int UninitializeForContextCallCount { get; private set; }

            protected override void UninitializeForContextCore(VirtualizingLayoutContext context)
            {
                UninitializeForContextCallCount++;
                base.UninitializeForContextCore(context);
            }
        }
    }
}
