// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading;
using Windows.Foundation;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;
using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common.Mocks;
using System;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class RepeaterFocusTests : ApiTestBase
    {
        [TestMethod]
        [TestProperty("Ignore", "True")] // TODO 33633693: RepeaterFocusTests.ValidateTabNavigation fails on pipeline
        public void ValidateTabNavigation()
        {
            ItemsRepeater repeater = null;
            ScrollViewer scrollViewer = null;
            var data = new ObservableCollection<string>(Enumerable.Range(0, 50).Select(i => "Item #" + i));
            var viewChangedEvent = new AutoResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                var itemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <Button Content='{Binding}' Height='100' />
                        </DataTemplate>");

                var elementFactory = new RecyclingElementFactory()
                {
                    RecyclePool = new RecyclePool(),
                    Templates =
                    {
                        { "itemTemplate", itemTemplate },
                    }
                };

                Content = CreateAndInitializeRepeater
                (
                   data,
                   new StackLayout(),
                   elementFactory,
                   ref repeater,
                   ref scrollViewer
                );

                scrollViewer.ViewChanged += (s, e) =>
                {
                    if (!e.IsIntermediate)
                    {
                        viewChangedEvent.Set();
                    }
                };

                repeater.TabFocusNavigation = KeyboardNavigationMode.Local;
                Content.UpdateLayout();
                ValidateTabNavigationOrder(repeater);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Move window - disconnected
                scrollViewer.ChangeView(null, 2000, null, true);
            });

            Verify.IsTrue(viewChangedEvent.WaitOne(DefaultWaitTimeInMS), "Waiting for final ViewChanged event.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                ValidateTabNavigationOrder(repeater);

                // Delete a couple of elements and validate we
                // ignore unrealized elements.
                // First visible element is expected to be index 20.
                while (data.Count > 21) { data.RemoveAt(21); }

                repeater.UpdateLayout();
                ValidateTabNavigationOrder(repeater);
            });
        }

        #region Focus after ItemsSource changes (task.ms/56189664)

        // The tests in this region check where keyboard focus ends up when the ItemsSource changes while focus is inside
        // the repeater. A cleared element stays in the repeater's Children (it is recycled and arranged off-screen), so
        // focus must not stay on it, and the repeater must not move focus through elements that the same change clears.

        private const string ButtonItemTemplate = "<Button Content='{Binding}' Height='30'/>";
        private const string TallButtonItemTemplate = "<Button Content='{Binding}' Height='100'/>";

        private class FocusFixture
        {
            public ItemsRepeater Repeater;
            public ScrollViewer ScrollViewer;
            public Button Before;
            public Button After;
            public StackPanel Root;
        }

        // Scenario: Reset (Clear + Add in the same tick) while a visible item of a StackLayout has keyboard focus.
        // Expected: no focus move while the change is processed; after layout, focus is on the new item at the same index,
        //           with the same FocusState, and arrow keys move between live items.
        // A failure means: focus hops through the cleared items and ends on an off-screen element with old content.
        [TestMethod]
        [TestProperty("Description", "Verifies that a Reset with focus on a StackLayout item moves focus once, after layout, to the new item at the same index and keeps the keyboard FocusState.")]
        public void VerifyResetMovesFocusToLiveItemAfterLayout()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 20).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            int gettingFocusDuringChange = 0;
            bool inChange = false;
            RunOnUIThread.Execute(() =>
            {
                fixture.Repeater.GettingFocus += (s, e) =>
                {
                    if (inChange)
                    {
                        gettingFocusDuringChange++;
                        Log.Comment("GettingFocus during the change: " + DescribeElement(fixture.Repeater, e.NewFocusedElement));
                    }
                };

                inChange = true;
                data.Clear();
                for (int i = 0; i < 20; i++)
                {
                    data.Add("New #" + i);
                }
                inChange = false;

                Log.Comment("Focused right after the change: " + DescribeFocus(fixture.Repeater));
                Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved while the collection change is processed.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 3, expectedFocusState: FocusState.Keyboard);
                Verify.IsTrue(FocusManager.TryMoveFocus(FocusNavigationDirection.Down, new FindNextElementOptions { SearchRoot = fixture.Repeater.XamlRoot.Content }));
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 4));
        }

        // Scenario: Reset (Clear + Add) with focus on an item of a custom layout that realizes items with
        //           GetOrCreateElementAt(index) (auto-recycled by the repeater).
        // Expected: no focus move during the change; after layout focus is on the new item at the same index.
        // A failure means: focus ends on the last element cleared by the repeater's own Reset loop.
        [TestMethod]
        [TestProperty("Description", "Verifies that a Reset with focus on an item of an auto-recycling custom layout moves focus once, after layout, to the new item at the same index.")]
        public void VerifyResetWithAutoRecyclingLayoutMovesFocusToLiveItem()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            VirtualizingLayout layout = null;
            RunOnUIThread.Execute(() => layout = CreateAutoRecyclingLayout(() => 0, int.MaxValue));
            var fixture = CreateFocusFixture(data, layout);
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            int gettingFocusDuringChange = CountGettingFocusDuring(fixture.Repeater, () =>
            {
                data.Clear();
                for (int i = 0; i < 5; i++)
                {
                    data.Add("New #" + i);
                }
            });
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved while the collection change is processed.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 2, expectedFocusState: FocusState.Keyboard));
        }

        // Scenario: Reset with focus on an item of a NonVirtualizingLayout (items are cleared by the repeater's Reset loop).
        // Expected: after layout focus is on the new item at the same index.
        // A failure means: focus ends on a cleared element of a non-virtualizing repeater.
        [TestMethod]
        [TestProperty("Description", "Verifies that a Reset with focus on an item of a NonVirtualizingLayout moves focus, after layout, to the new item at the same index.")]
        public void VerifyResetWithNonVirtualizingLayoutMovesFocusToLiveItem()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            Layout layout = null;
            RunOnUIThread.Execute(() => layout = new NonVirtualStackLayout());
            var fixture = CreateFocusFixture(data, layout);
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                data.Clear();
                for (int i = 0; i < 5; i++)
                {
                    data.Add("New #" + i);
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 2));
        }

        // Scenario: ItemsSource reassignment of a NonVirtualizingLayout repeater while an item has focus.
        // Expected: no exception; focus is not left on a cleared element.
        // A failure means: reassignment of a non-virtualizing repeater leaves focus on a recycled element or throws.
        [TestMethod]
        [TestProperty("Description", "Verifies that reassigning ItemsSource of a NonVirtualizingLayout repeater with a focused item does not throw and does not leave focus on a cleared element.")]
        public void VerifyNonVirtualizingReassignmentDoesNotLeaveFocusOnClearedElement()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            Layout layout = null;
            RunOnUIThread.Execute(() => layout = new NonVirtualStackLayout());
            var fixture = CreateFocusFixture(data, layout);
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            var newData = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Filtered #" + i));
            RunOnUIThread.Execute(() => fixture.Repeater.ItemsSource = newData);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusNotOnClearedElement(fixture.Repeater, i => newData[i]));
        }

        // Scenario: Clear() with focus on an item, then a layout pass with an empty source, then items are added.
        // Expected: focus leaves the repeater (it has no item to focus); the later Adds do not pull focus back.
        // A failure means: focus stays on an off-screen element whose old content Narrator reads.
        [TestMethod]
        [TestProperty("Description", "Verifies that clearing the ItemsSource with focus on an item moves focus out of the repeater, and that items added later do not take focus.")]
        public void VerifyClearMovesFocusOutOfRepeater()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 1, FocusState.Keyboard);

            RunOnUIThread.Execute(() => data.Clear());
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => VerifyFocusOutsideRepeater(fixture));

            RunOnUIThread.Execute(() =>
            {
                for (int i = 0; i < 3; i++)
                {
                    data.Add("New #" + i);
                }
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(fixture.Repeater.TryGetElement(0), "The new items are realized.");
                VerifyFocusOutsideRepeater(fixture);
            });
        }

        // Scenario: the only item has focus and is removed.
        // Expected: focus leaves the repeater.
        // A failure means: focus stays on the removed, off-screen element.
        [TestMethod]
        [TestProperty("Description", "Verifies that removing the only, focused item moves focus out of the repeater.")]
        public void VerifyRemovingOnlyFocusedItemMovesFocusOutOfRepeater()
        {
            var data = new ObservableCollection<string> { "Item #0" };
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 0, FocusState.Keyboard);

            RunOnUIThread.Execute(() => data.RemoveAt(0));
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => VerifyFocusOutsideRepeater(fixture));
        }

        // Scenario: a single Remove notification for three items covers the focused item.
        // Expected: no focus move during the change; after layout focus is on the first survivor at the removed index.
        // A failure means: focus is moved to the first item, or onto an item that the same Remove clears.
        [TestMethod]
        [TestProperty("Description", "Verifies that a multi-item Remove covering the focused item moves focus once, after layout, to the first survivor at the removed index.")]
        public void VerifyMultiItemRemoveMovesFocusToFirstSurvivor()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList()));
            var fixture = CreateFocusFixture(dataSource);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            int gettingFocusDuringChange = CountGettingFocusDuring(fixture.Repeater, () => dataSource.Remove(2 /* index */, 3 /* count */, false /* reset */));
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved while the collection change is processed.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => dataSource.Inner[i].ToString(), expectedIndex: 2));
            RunOnUIThread.Execute(() => Verify.AreEqual(5, dataSource.Inner[2]));
        }

        // Scenario: the focused, visible item is replaced (items[k] = x).
        // Expected: after layout focus is on the replacement element at index k.
        // A failure means: focus moves to the first item or stays on the recycled old element.
        [TestMethod]
        [TestProperty("Description", "Verifies that replacing the focused item moves focus, after layout, to the replacement element at the same index.")]
        public void VerifyReplacingFocusedItemMovesFocusToReplacement()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            int gettingFocusDuringChange = CountGettingFocusDuring(fixture.Repeater, () => data[2] = "Replaced #2");
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved while the collection change is processed.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 2, expectedFocusState: FocusState.Keyboard));
        }

        // Scenario: keyed source, item containers are StackPanels (not Controls); the focused last item is removed.
        //           The keyed Reset path clears the leftover element after arrange and picks a target immediately.
        // Expected: focus moves to the Button inside the previous item.
        // A failure means: the previous-item fallback does not look for a focusable descendant, or the cleared index is lost.
        [TestMethod]
        [TestProperty("Description", "Verifies that removing the focused last item of a keyed source with non-Control containers moves focus to the focusable descendant of the previous item.")]
        public void VerifyRemovingLastKeyedItemFocusesPreviousItemDescendant()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSourceWithUniqueId(Enumerable.Range(0, 5).ToList()));
            var fixture = CreateFocusFixture(dataSource, itemTemplate: "<StackPanel Height='30'><Button Content='{Binding}'/></StackPanel>");
            FocusItemAndWait(fixture.Repeater, 4, FocusState.Keyboard);

            RunOnUIThread.Execute(() => dataSource.Remove(4 /* index */, 1 /* count */, true /* reset */));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => dataSource.Inner[i].ToString(), expectedIndex: 3));
        }

        // Scenarios: focus item 0, scroll it out of view (it is kept in the pinned pool), then change the data.
        // Expected: the pinned element of the changed item is cleared by the change (ElementClearing once, index -1, not in
        //           the UIA children); focus is not left on it; scrolling back shows the current item 0.
        // A failure means: the stale element keeps focus and is shown again for index 0 with old content.
        [TestMethod]
        [TestProperty("Description", "Verifies that removing a focused item that is scrolled out of view (pinned) clears its element, moves focus to a live item and does not reuse the stale element.")]
        public void VerifyRemovingPinnedFocusedItemClearsItsElement()
        {
            VerifyPinnedFocusedElementIsCleared(PinnedChange.Remove);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that replacing a focused item that is scrolled out of view (pinned) clears its element and shows the replacement when scrolled back.")]
        public void VerifyReplacingPinnedFocusedItemClearsItsElement()
        {
            VerifyPinnedFocusedElementIsCleared(PinnedChange.Replace);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that a Reset (Clear + Add) clears the element of a focused item that is scrolled out of view (pinned) and does not shift its index.")]
        public void VerifyResetClearsPinnedFocusedElement()
        {
            VerifyPinnedFocusedElementIsCleared(PinnedChange.Reset);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that assigning a new ItemsSource clears the element of a focused item that is scrolled out of view (pinned).")]
        public void VerifyItemsSourceReassignmentClearsPinnedFocusedElement()
        {
            VerifyPinnedFocusedElementIsCleared(PinnedChange.Reassign);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that changing the ItemTemplate clears the element of a focused item that is scrolled out of view (pinned), so the old-template element is not reused.")]
        public void VerifyItemTemplateChangeClearsPinnedFocusedElement()
        {
            VerifyPinnedFocusedElementIsCleared(PinnedChange.TemplateChange);
        }

        // Scenario: auto-recycling custom layout; item 0 is focused and then no longer requested by the layout (pinned
        //           pool); the item is removed, or the source is reset and refilled.
        // Expected: the pinned element is cleared, keeps no index, and focus moves to a live item.
        // A failure means: the repeater keeps the removed item's element focused and shifts it to a new index.
        [TestMethod]
        [TestProperty("Description", "Verifies that with an auto-recycling custom layout, removing a focused item held in the pinned pool clears its element and moves focus to a live item.")]
        public void VerifyAutoRecyclingLayoutRemoveClearsPinnedFocusedElement()
        {
            VerifyAutoRecyclingLayoutPinnedFocusedElementIsCleared(reset: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that with an auto-recycling custom layout, a Reset + Add clears a focused element held in the pinned pool instead of shifting it to a new index.")]
        public void VerifyAutoRecyclingLayoutResetClearsPinnedFocusedElement()
        {
            VerifyAutoRecyclingLayoutPinnedFocusedElementIsCleared(reset: true);
        }

        // Scenario: keyed source, focused item 0 is scrolled out of view (pinned), then the Layout is replaced.
        // Expected: no ownership assert or crash; the pinned element is not sent to the keyed reset pool by the layout
        //           change, and focus ends on an element that shows the current data for its index (the pinned element
        //           itself, or a live item if the new layout realizes and discards index 0 during its keyed reset).
        // A failure means: a pinned element is sent to the keyed reset pool (an ownership assert in checked builds) and
        //                  focus is moved to the lowest realized item or left on a cleared element.
        [TestMethod]
        [TestProperty("Description", "Verifies that replacing the Layout of a keyed repeater with a focused, scrolled-out (pinned) element does not assert and leaves focus on an element that shows current data.")]
        public void VerifyLayoutChangeKeepsPinnedFocusedElementWithKeyedSource()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSourceWithUniqueId(Enumerable.Range(0, 100).ToList()));
            var fixture = CreateFocusFixture(dataSource, itemTemplate: TallButtonItemTemplate);
            var element0 = FocusItemAndScrollAway(fixture);

            RunOnUIThread.Execute(() =>
            {
                fixture.Repeater.Layout = new StackLayout();
                Verify.AreEqual(0, fixture.Repeater.GetElementIndex(element0), "The pinned element keeps its index across the layout change.");
                Verify.AreSame(element0, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "The pinned element keeps focus across the layout change.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"Pinned element index after layout: {fixture.Repeater.GetElementIndex(element0)}");
                VerifyFocusOnLiveItem(fixture.Repeater, i => dataSource.Inner[i].ToString());
            });
        }

        // Scenario: keyed source, focused item 0 is scrolled out of view (pinned), then the source raises Reset.
        // Expected: the pinned element is cleared (its index is stale after a Reset) and focus moves to a live item.
        // A failure means: the stale element stays focused and is reused for index 0 (issue #12659 class).
        [TestMethod]
        [TestProperty("Description", "Verifies that a Reset of a keyed source clears a focused element that is scrolled out of view (pinned) and moves focus to a live item.")]
        public void VerifyKeyedResetClearsPinnedFocusedElement()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSourceWithUniqueId(Enumerable.Range(0, 100).ToList()));
            var fixture = CreateFocusFixture(dataSource, itemTemplate: TallButtonItemTemplate);
            var element0 = FocusItemAndScrollAway(fixture);

            RunOnUIThread.Execute(() =>
            {
                dataSource.Reset();
                Verify.AreEqual(-1, fixture.Repeater.GetElementIndex(element0), "The pinned element is cleared by the Reset.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => dataSource.Inner[i].ToString()));
        }

        // Scenario: an ElementClearing handler moves focus to a control outside the repeater during a Reset.
        // Expected: focus stays where the handler put it.
        // A failure means: the repeater moves focus back into elements that the same change clears.
        [TestMethod]
        [TestProperty("Description", "Verifies that when an ElementClearing handler moves focus out of the repeater during a Reset, the repeater does not move focus back.")]
        public void VerifyFocusMoveFromElementClearingHandlerIsKept()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                bool moved = false;
                fixture.Repeater.ElementClearing += (s, e) =>
                {
                    if (!moved)
                    {
                        moved = true;
                        Verify.IsTrue(fixture.After.Focus(FocusState.Programmatic));
                    }
                };

                data.Clear();
                for (int i = 0; i < 10; i++)
                {
                    data.Add("New #" + i);
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Focused: " + DescribeFocus(fixture.Repeater));
                Verify.AreSame(fixture.After, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot));
            });
        }

        // Scenario: a GettingFocus handler removes the item that the repeater is moving focus to after a Reset.
        // Expected: no exception (the focus move does not run inside the collection change); focus ends on a live item.
        // A failure means: the focus move runs inside the change (the handler's change throws) or focus is left on the
        //                  element that the handler's change cleared.
        [TestMethod]
        [TestProperty("Description", "Verifies that a GettingFocus handler can change the ItemsSource while the repeater moves focus after a Reset, and that focus ends on a live item.")]
        public void VerifyGettingFocusHandlerCanChangeItemsSourceDuringFocusMove()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            Exception handlerException = null;
            RunOnUIThread.Execute(() =>
            {
                bool armed = false;
                bool mutated = false;
                fixture.Repeater.GettingFocus += (s, e) =>
                {
                    if (armed && !mutated)
                    {
                        mutated = true;
                        Log.Comment("GettingFocus target: " + DescribeElement(fixture.Repeater, e.NewFocusedElement) + "; removing item 3 from the handler.");
                        try
                        {
                            data.RemoveAt(3);
                        }
                        catch (Exception ex)
                        {
                            handlerException = ex;
                        }
                    }
                };

                armed = true;
                data.Clear();
                for (int i = 0; i < 10; i++)
                {
                    data.Add("New #" + i);
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(handlerException, "The ItemsSource can be changed from a GettingFocus handler: " + handlerException?.Message);
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i]);
            });
        }

        // Scenario: a Reset with focus inside the repeater, and the repeater leaves the tree before the next layout pass.
        // Expected: no exception; after it is added back, focus is not on a cleared element.
        // A failure means: a pending focus move runs into an unloaded tree or keeps the cleared element alive.
        [TestMethod]
        [TestProperty("Description", "Verifies that a Reset with focus followed by unloading the repeater before layout does not throw and does not leave focus on a cleared element after reloading.")]
        public void VerifyUnloadAfterResetWithFocusDoesNotThrow()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                data.Clear();
                for (int i = 0; i < 10; i++)
                {
                    data.Add("New #" + i);
                }
                Content = null;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Content = fixture.Root);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusNotOnClearedElement(fixture.Repeater, i => data[i]));
        }

        // Scenario: nested repeaters; focus is on an item of an inner repeater; the outer source is reset and refilled.
        // Expected: focus ends on a live inner item of a live outer item.
        // A failure means: focus ends on an off-screen outer element of the old source.
        [TestMethod]
        [TestProperty("Description", "Verifies that resetting the outer source of nested repeaters with focus on an inner item moves focus to a live inner item of a live outer item.")]
        public void VerifyNestedRepeaterResetMovesFocusToLiveItem()
        {
            var groups = new ObservableCollection<ObservableCollection<string>>(Enumerable.Range(0, 3).Select(g =>
                new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => $"Item #{g}.{i}"))));
            ItemsRepeater outer = null;
            Button focusTarget = null;
            var gotFocus = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                outer = new ItemsRepeater
                {
                    ItemsSource = groups,
                    ItemTemplate = SharedHelpers.GetDataTemplate(
                        @"<ItemsRepeater ItemsSource='{Binding}'>
                            <ItemsRepeater.ItemTemplate>
                                <DataTemplate><Button Content='{Binding}' Height='30'/></DataTemplate>
                            </ItemsRepeater.ItemTemplate>
                          </ItemsRepeater>"),
                    VerticalAlignment = VerticalAlignment.Top
                };
                Content = outer;
                Content.UpdateLayout();

                var inner = (ItemsRepeater)outer.TryGetElement(1);
                focusTarget = (Button)inner.TryGetElement(1);
                focusTarget.GotFocus += delegate { gotFocus.Set(); };
                Verify.IsTrue(focusTarget.Focus(FocusState.Keyboard));
            });
            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), "Waiting for the inner item to get focus.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                groups.Clear();
                for (int g = 0; g < 3; g++)
                {
                    groups.Add(new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => $"New #{g}.{i}")));
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(outer.XamlRoot) as Button;
                Log.Comment($"Focused: {focused?.Content}");
                Verify.IsNotNull(focused, "A Button is focused.");
                Verify.IsTrue(((string)focused.Content).StartsWith("New #"), "The focused Button shows an item of the new source.");

                var innerContainer = VisualTreeHelper.GetParent(focused) as ItemsRepeater;
                Verify.IsNotNull(innerContainer, "The focused Button is an inner repeater item.");
                Verify.IsTrue(innerContainer.GetElementIndex(focused) >= 0, "The inner item is realized.");
                Verify.IsTrue(outer.GetElementIndex(innerContainer) >= 0, "The outer item is realized.");
            });
        }

        // Scenario: ItemsSource of UIElements (no ItemTemplate); Reset with focus on an item.
        // Expected: no exception; focus is not left on an element that was removed from the source.
        // A failure means: focus handling breaks when cleared elements are removed from Children.
        [TestMethod]
        [TestProperty("Description", "Verifies that a Reset of a UIElement ItemsSource with a focused item does not throw and does not leave focus on a removed element.")]
        public void VerifyResetOfUIElementItemsWithFocusDoesNotLeaveFocusOnRemovedElement()
        {
            ObservableCollection<UIElement> data = null;
            List<UIElement> oldItems = null;
            RunOnUIThread.Execute(() =>
            {
                data = new ObservableCollection<UIElement>(Enumerable.Range(0, 5).Select(i => (UIElement)new Button { Content = "Item #" + i, Height = 30 }));
                oldItems = data.ToList();
            });
            var fixture = CreateFocusFixture(data, itemTemplate: null);
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                data.Clear();
                for (int i = 0; i < 5; i++)
                {
                    data.Add(new Button { Content = "New #" + i, Height = 30 });
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot) as UIElement;
                Log.Comment("Focused: " + DescribeFocus(fixture.Repeater));
                Verify.IsFalse(oldItems.Contains(focused), "Focus is not on a removed item.");
                VerifyFocusNotOnClearedElement(fixture.Repeater, i => (string)((Button)data[i]).Content);
            });
        }

        // Scenario: change the ItemTemplate while a visible item has focus.
        // Expected: no focus move during the change; after layout focus is on the new-template element at the same index.
        // A failure means: focus ends on an off-screen element of the old template.
        [TestMethod]
        [TestProperty("Description", "Verifies that changing the ItemTemplate with focus on an item moves focus, after layout, to the new-template element at the same index.")]
        public void VerifyItemTemplateChangeMovesFocusToNewTemplateElement()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            int gettingFocusDuringChange = CountGettingFocusDuring(fixture.Repeater, () =>
                fixture.Repeater.ItemTemplate = CreateElementFactory("<Button Content='{Binding}' Height='30' Tag='NewTemplate'/>"));
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved while the template change is processed.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 3);
                Verify.AreEqual("NewTemplate", ((Button)FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot)).Tag);
            });
        }

        // Scenario: assign a new ItemsSource while a visible item has focus.
        // Expected: after layout focus is on the item of the new source at the same index.
        // A failure means: focus ends on an off-screen element of the old source.
        [TestMethod]
        [TestProperty("Description", "Verifies that assigning a new ItemsSource with focus on an item moves focus, after layout, to the new item at the same index.")]
        public void VerifyItemsSourceReassignmentMovesFocusToLiveItem()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var newData = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Filtered #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            int gettingFocusDuringChange = CountGettingFocusDuring(fixture.Repeater, () => fixture.Repeater.ItemsSource = newData);
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved while the ItemsSource change is processed.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => newData[i], expectedIndex: 3));
        }

        // Scenario: two changes before the next layout pass: RemoveAt(focused) then Insert(0); and RemoveAt(focused) then
        //           RemoveAt(0).
        // Expected: one focus move after layout, to the live item at the focused item's index.
        // A failure means: the target is chosen during the first change, before the second change shifts the items.
        [TestMethod]
        [TestProperty("Description", "Verifies that when the focused item is removed and another change follows before layout, focus moves after layout to the live item at the removed index.")]
        public void VerifyRemoveFollowedByAnotherChangeBeforeLayoutMovesFocusToLiveItem()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);

            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);
            RunOnUIThread.Execute(() =>
            {
                data.RemoveAt(3);
                data.Insert(0, "Inserted");
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 3));

            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);
            RunOnUIThread.Execute(() =>
            {
                data.RemoveAt(3);
                data.RemoveAt(0);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 3));
        }

        // Scenario: two Resets (Clear + Add twice) in the same tick with focus on an item.
        // Expected: exactly one focus move, after layout, to the live item at the same index.
        // A failure means: focus is moved during the changes (once per cleared element).
        [TestMethod]
        [TestProperty("Description", "Verifies that two Resets before layout with focus on an item move focus exactly once, after layout, to the live item at the same index.")]
        public void VerifyTwoResetsBeforeLayoutMoveFocusOnce()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            int gettingFocusCount = 0;
            RunOnUIThread.Execute(() =>
            {
                fixture.Repeater.GettingFocus += (s, e) =>
                {
                    gettingFocusCount++;
                    Log.Comment("GettingFocus: " + DescribeElement(fixture.Repeater, e.NewFocusedElement));
                };

                for (int round = 0; round < 2; round++)
                {
                    data.Clear();
                    for (int i = 0; i < 10; i++)
                    {
                        data.Add($"Round {round} #{i}");
                    }
                }
                Verify.AreEqual(0, gettingFocusCount, "Focus is not moved while the changes are processed.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 3);
                Verify.AreEqual(1, gettingFocusCount, "Focus is moved exactly once.");
            });
        }

        // Scenario: the item after the focused one is disabled; the focused item is removed.
        // Expected: focus moves to the nearest focusable live item (the disabled item is skipped).
        // A failure means: focus moves to the first item, or stays on the removed element because the target refused focus.
        [TestMethod]
        [TestProperty("Description", "Verifies that when the item that takes the removed item's index is disabled, focus moves to the nearest focusable live item.")]
        public void VerifyRemovingFocusedItemSkipsDisabledNeighbour()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                ((Control)fixture.Repeater.TryGetElement(4)).IsEnabled = false;
                data.RemoveAt(3);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(((Control)fixture.Repeater.TryGetElement(3)).IsEnabled, "Precondition: the item now at index 3 is disabled.");
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 4);
            });
        }

        // Scenario: focus moves programmatically to item 3 and, in the same tick (before GotFocus is raised), items 2 and 3
        //           (the previously tracked focused item and the newly focused one) are removed by one notification.
        // Expected: after layout focus is on the first survivor at index 2.
        // A failure means: a late focus event or the stale tracked element makes the repeater skip or misplace the move.
        [TestMethod]
        [TestProperty("Description", "Verifies that focusing an item and removing it together with the previously focused item in the same tick moves focus, after layout, to the first survivor.")]
        public void VerifyFocusChangeAndRemoveInSameTickMovesFocusToSurvivor()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList()));
            var fixture = CreateFocusFixture(dataSource);
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(((Control)fixture.Repeater.TryGetElement(3)).Focus(FocusState.Keyboard));
                dataSource.Remove(2 /* index */, 2 /* count */, false /* reset */);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => dataSource.Inner[i].ToString(), expectedIndex: 2));
        }

        private enum PinnedChange { Remove, Replace, Reset, Reassign, TemplateChange }

        private void VerifyPinnedFocusedElementIsCleared(PinnedChange change)
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 100).Select(i => "Item #" + i));
            var current = data;
            var fixture = CreateFocusFixture(data, itemTemplate: TallButtonItemTemplate);
            var element0 = FocusItemAndScrollAway(fixture);

            RunOnUIThread.Execute(() =>
            {
                int clearing = 0;
                int indexChanged = 0;
                fixture.Repeater.ElementClearing += (s, e) => { if (e.Element == element0) { clearing++; } };
                fixture.Repeater.ElementIndexChanged += (s, e) => { if (e.Element == element0) { indexChanged++; } };

                Log.Comment("Change: " + change);
                switch (change)
                {
                    case PinnedChange.Remove:
                        data.RemoveAt(0);
                        break;
                    case PinnedChange.Replace:
                        data[0] = "Replaced #0";
                        break;
                    case PinnedChange.Reset:
                        data.Clear();
                        for (int i = 0; i < 100; i++)
                        {
                            data.Add("New #" + i);
                        }
                        break;
                    case PinnedChange.Reassign:
                        current = new ObservableCollection<string>(Enumerable.Range(0, 100).Select(i => "Filtered #" + i));
                        fixture.Repeater.ItemsSource = current;
                        break;
                    case PinnedChange.TemplateChange:
                        fixture.Repeater.ItemTemplate = CreateElementFactory("<Button Content='{Binding}' Height='100' Tag='NewTemplate'/>");
                        break;
                }

                Verify.AreEqual(-1, fixture.Repeater.GetElementIndex(element0), "The pinned element of the changed item is cleared by the change.");
                Verify.AreEqual(1, clearing, "ElementClearing is raised once for the pinned element.");
                Verify.AreEqual(0, indexChanged, "The cleared element gets no new index.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreNotSame(element0, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus is not left on the cleared element.");
                VerifyFocusOnLiveItem(fixture.Repeater, i => current[i]);

                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(fixture.Repeater);
                var children = peer.GetChildren();
                foreach (var childPeer in children ?? Enumerable.Empty<AutomationPeer>())
                {
                    // The cleared element can be reused by the layout for another item; then it is a live item.
                    var owner = (childPeer as FrameworkElementAutomationPeer)?.Owner;
                    Verify.IsTrue(owner == null || fixture.Repeater.GetElementIndex(owner) >= 0, "Every UIA child of the repeater is a realized item.");
                }
            });

            ScrollToOffset(fixture.ScrollViewer, 0);
            RunOnUIThread.Execute(() =>
            {
                var realized = Enumerable.Range(0, VisualTreeHelper.GetChildrenCount(fixture.Repeater))
                    .Select(i => (UIElement)VisualTreeHelper.GetChild(fixture.Repeater, i))
                    .Select(e => fixture.Repeater.GetElementIndex(e))
                    .Where(i => i >= 0)
                    .OrderBy(i => i);
                Log.Comment($"VerticalOffset={fixture.ScrollViewer.VerticalOffset} realized indexes: {string.Join(",", realized)}");
                var first = (Button)fixture.Repeater.TryGetElement(0);
                Verify.IsNotNull(first);
                Log.Comment($"Index 0 shows '{first.Content}' (Tag '{first.Tag}')");
                Verify.AreEqual(current[0], first.Content);
                if (change == PinnedChange.TemplateChange)
                {
                    Verify.AreEqual("NewTemplate", first.Tag, "Index 0 uses the new template.");
                }
            });
        }

        private void VerifyAutoRecyclingLayoutPinnedFocusedElementIsCleared(bool reset)
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 20).Select(i => "Item #" + i));
            int firstIndex = 0;
            MockVirtualizingLayout layout = null;
            RunOnUIThread.Execute(() => layout = CreateAutoRecyclingLayout(() => firstIndex, 5));
            var fixture = CreateFocusFixture(data, layout);
            FocusItemAndWait(fixture.Repeater, 0, FocusState.Keyboard);

            UIElement element0 = null;
            RunOnUIThread.Execute(() =>
            {
                element0 = fixture.Repeater.TryGetElement(0);
                firstIndex = 10;
                layout.InvalidateMeasure();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, fixture.Repeater.GetElementIndex(element0), "Precondition: the focused element is pinned with index 0.");
                Verify.IsNull(fixture.Repeater.TryGetElement(1), "Precondition: item 1 is not realized.");

                if (reset)
                {
                    data.Clear();
                    for (int i = 0; i < 20; i++)
                    {
                        data.Add("New #" + i);
                    }
                }
                else
                {
                    data.RemoveAt(0);
                }

                Verify.AreEqual(-1, fixture.Repeater.GetElementIndex(element0), "The pinned element is cleared by the change.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreNotSame(element0, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus is not left on the cleared element.");
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 10);
            });
        }

        private Button FocusItemAndScrollAway(FocusFixture fixture)
        {
            FocusItemAndWait(fixture.Repeater, 0, FocusState.Programmatic);
            Button element0 = null;
            RunOnUIThread.Execute(() => element0 = (Button)fixture.Repeater.TryGetElement(0));
            ScrollToOffset(fixture.ScrollViewer, 5000);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(fixture.Repeater.TryGetElement(50), "Precondition: item 50 is realized after scrolling.");
                Verify.AreEqual(0, fixture.Repeater.GetElementIndex(element0), "Precondition: the focused element is pinned and keeps its index.");
                Verify.AreSame(element0, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Precondition: focus stays on the pinned element.");
            });
            return element0;
        }

        private FocusFixture CreateFocusFixture(object itemsSource, Layout layout = null, string itemTemplate = ButtonItemTemplate)
        {
            var fixture = new FocusFixture();
            RunOnUIThread.Execute(() =>
            {
                fixture.Repeater = new ItemsRepeater
                {
                    ItemsSource = itemsSource,
                    HorizontalCacheLength = 0,
                    VerticalCacheLength = 0
                };
                if (itemTemplate != null)
                {
                    fixture.Repeater.ItemTemplate = CreateElementFactory(itemTemplate);
                }
                if (layout != null)
                {
                    fixture.Repeater.Layout = layout;
                }

                fixture.Before = new Button { Content = "Before" };
                fixture.After = new Button { Content = "After" };
                fixture.ScrollViewer = new ScrollViewer { Content = fixture.Repeater, Height = 200, Width = 200 };
                fixture.Root = new StackPanel();
                fixture.Root.Children.Add(fixture.Before);
                fixture.Root.Children.Add(fixture.ScrollViewer);
                fixture.Root.Children.Add(fixture.After);
                Content = fixture.Root;
            });
            IdleSynchronizer.Wait();
            return fixture;
        }

        private static RecyclingElementFactory CreateElementFactory(string itemTemplate)
        {
            var factory = new RecyclingElementFactory { RecyclePool = new RecyclePool() };
            factory.Templates["Item"] = SharedHelpers.GetDataTemplate(itemTemplate);
            return factory;
        }

        // A custom layout that realizes items [first, first + count) with GetOrCreateElementAt(index), so the repeater
        // auto-recycles the items it no longer requests.
        private static MockVirtualizingLayout CreateAutoRecyclingLayout(Func<int> first, int count)
        {
            const double itemHeight = 30;
            return new MockVirtualizingLayout
            {
                MeasureLayoutFunc = (availableSize, context) =>
                {
                    int start = first();
                    int end = (int)Math.Min((long)context.ItemCount, (long)start + count);
                    for (int i = start; i < end; i++)
                    {
                        context.GetOrCreateElementAt(i).Measure(new Size(200, itemHeight));
                    }
                    return new Size(200, itemHeight * context.ItemCount);
                },
                ArrangeLayoutFunc = (finalSize, context) =>
                {
                    int start = first();
                    int end = (int)Math.Min((long)context.ItemCount, (long)start + count);
                    for (int i = start; i < end; i++)
                    {
                        context.GetOrCreateElementAt(i).Arrange(new Rect(0, itemHeight * i, 200, itemHeight));
                    }
                    return finalSize;
                }
            };
        }

        private static int CountGettingFocusDuring(ItemsRepeater repeater, Action change)
        {
            int count = 0;
            RunOnUIThread.Execute(() =>
            {
                bool inChange = true;
                TypedEventHandler<UIElement, GettingFocusEventArgs> handler = (s, e) =>
                {
                    if (inChange)
                    {
                        count++;
                        Log.Comment("GettingFocus during the change: " + DescribeElement(repeater, e.NewFocusedElement));
                    }
                };
                repeater.GettingFocus += handler;
                try
                {
                    change();
                }
                finally
                {
                    inChange = false;
                    repeater.GettingFocus -= handler;
                }
                Log.Comment("Focused right after the change: " + DescribeFocus(repeater));
            });
            return count;
        }

        private void FocusItemAndWait(ItemsRepeater repeater, int index, FocusState focusState)
        {
            var gotFocus = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                var element = repeater.TryGetElement(index);
                var target = element as Control ?? (Control)FocusManager.FindFirstFocusableElement(element);
                if (FocusManager.GetFocusedElement(repeater.XamlRoot) == target)
                {
                    gotFocus.Set();
                }
                else
                {
                    target.GotFocus += delegate { gotFocus.Set(); };
                    Verify.IsTrue(target.Focus(focusState));
                }
            });
            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), $"Waiting for item {index} to get focus.");
            IdleSynchronizer.Wait();
        }

        private void ScrollToOffset(ScrollViewer scrollViewer, double verticalOffset)
        {
            // A view change that is already in progress (for example a bring-into-view after a focus move) can complete
            // first, so wait until the requested offset is reached.
            var viewChanged = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                if (scrollViewer.VerticalOffset == verticalOffset)
                {
                    viewChanged.Set();
                    return;
                }

                int attempts = 0;
                scrollViewer.ViewChanged += OnViewChanged;
                Verify.IsTrue(scrollViewer.ChangeView(null, verticalOffset, null, disableAnimation: true));

                void OnViewChanged(object sender, ScrollViewerViewChangedEventArgs args)
                {
                    if (!args.IsIntermediate)
                    {
                        if (Math.Abs(scrollViewer.VerticalOffset - verticalOffset) < 0.5 || ++attempts > 5)
                        {
                            scrollViewer.ViewChanged -= OnViewChanged;
                            viewChanged.Set();
                        }
                        else
                        {
                            scrollViewer.ChangeView(null, verticalOffset, null, disableAnimation: true);
                        }
                    }
                }
            });
            Verify.IsTrue(viewChanged.WaitOne(DefaultWaitTimeInMS), $"Waiting for ViewChanged to offset {verticalOffset}.");
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.AreEqual(verticalOffset, scrollViewer.VerticalOffset, "The requested offset is reached."));
        }

        // Returns the repeater's direct child that contains the focused element, or null when focus is outside.
        private static UIElement GetFocusedContainer(ItemsRepeater repeater, out DependencyObject focused)
        {
            focused = FocusManager.GetFocusedElement(repeater.XamlRoot) as DependencyObject;
            var current = focused;
            while (current != null)
            {
                var parent = VisualTreeHelper.GetParent(current);
                if (parent == repeater)
                {
                    return current as UIElement;
                }
                current = parent;
            }
            return null;
        }

        private static string DescribeElement(ItemsRepeater repeater, DependencyObject element)
        {
            if (element == null)
            {
                return "<null>";
            }

            var current = element;
            UIElement container = null;
            while (current != null)
            {
                var parent = VisualTreeHelper.GetParent(current);
                if (parent == repeater)
                {
                    container = current as UIElement;
                    break;
                }
                current = parent;
            }

            string content = (element as ContentControl)?.Content?.ToString();
            string state = (element as Control)?.FocusState.ToString();
            string where = container == null ? "outside the repeater" : $"repeater index {repeater.GetElementIndex(container)}";
            return $"{element.GetType().Name} '{content}' FocusState={state} ({where})";
        }

        private static string DescribeFocus(ItemsRepeater repeater)
        {
            return DescribeElement(repeater, FocusManager.GetFocusedElement(repeater.XamlRoot) as DependencyObject);
        }

        // Focus is on a realized item of the repeater and the item shows the current data for its index.
        private static void VerifyFocusOnLiveItem(ItemsRepeater repeater, Func<int, string> contentAt, int expectedIndex = -1, FocusState? expectedFocusState = null)
        {
            var container = GetFocusedContainer(repeater, out var focused);
            Log.Comment("Focused: " + DescribeElement(repeater, focused));
            Verify.IsNotNull(container, "Focus is inside the repeater.");

            int index = repeater.GetElementIndex(container);
            Verify.IsTrue(index >= 0, "The focused item is realized (not a cleared element).");
            Verify.AreSame(container, repeater.TryGetElement(index));
            Verify.AreEqual(contentAt(index), (focused as ContentControl)?.Content?.ToString(), "The focused item shows the current data for its index.");

            if (expectedIndex >= 0)
            {
                Verify.AreEqual(expectedIndex, index);
            }

            if (expectedFocusState.HasValue)
            {
                Verify.AreEqual(expectedFocusState.Value, ((Control)focused).FocusState);
            }
        }

        // Focus is either outside the repeater or on a live item.
        private static void VerifyFocusNotOnClearedElement(ItemsRepeater repeater, Func<int, string> contentAt)
        {
            var container = GetFocusedContainer(repeater, out var focused);
            Log.Comment("Focused: " + DescribeElement(repeater, focused));
            if (container != null)
            {
                VerifyFocusOnLiveItem(repeater, contentAt);
            }
        }

        private static void VerifyFocusOutsideRepeater(FocusFixture fixture)
        {
            var container = GetFocusedContainer(fixture.Repeater, out var focused);
            Log.Comment("Focused: " + DescribeElement(fixture.Repeater, focused));
            Verify.IsNull(container, "Focus is not inside the repeater.");
            Verify.IsNotNull(focused, "Focus is on an element.");
            Verify.AreNotEqual(FocusState.Unfocused, (focused as Control)?.FocusState ?? FocusState.Unfocused, "The focused element is a focused Control.");
        }

        #endregion

        private static void ValidateTabNavigationOrder(ItemsRepeater repeater)
        {
            var expectedSequence = GetRealizedRange(repeater);

            expectedSequence.Last().Focus(FocusState.Keyboard);
            Log.Comment("\n\nStart point: " + expectedSequence.Last().Content);

            Log.Comment("Validating forward tab navigation...");

            FindNextElementOptions options = new FindNextElementOptions {
                SearchRoot = repeater.XamlRoot.Content
            };

            foreach(var expectedElement in expectedSequence)
            {
                var actualElement = FocusManager.FindNextElement(FocusNavigationDirection.Next, options);
                var actualElementAsbutton = FocusManager.FindNextElement(FocusNavigationDirection.Next, options) as Button;
                var actualElementAsToggleButton = FocusManager.FindNextElement(FocusNavigationDirection.Next, options) as ToggleButton;

                string content = actualElementAsbutton != null ? (string)actualElementAsbutton.Content : (string)actualElementAsToggleButton.Content;
                // We need to ignore the toggle theme button, so lets set its tabstop to false and get next element.
                if (content == "Toggle theme")
                {
                    actualElementAsbutton.IsTabStop = false;
                    actualElement = FocusManager.FindNextElement(FocusNavigationDirection.Next, options);
                }
                actualElementAsbutton = FocusManager.FindNextElement(FocusNavigationDirection.Next, options) as Button;
                actualElementAsToggleButton = FocusManager.FindNextElement(FocusNavigationDirection.Next, options) as ToggleButton;

                content = actualElementAsbutton != null ? (string)actualElementAsbutton.Content : (string)actualElementAsToggleButton.Content;
                // We need to ignore the lab dimensions button, so lets set its tabstop to false and get next element.
                if (content== "Render innerframe in lab dimensions")
                {
                    actualElementAsToggleButton.IsTabStop = false;
                    actualElement = FocusManager.FindNextElement(FocusNavigationDirection.Next, options);
                }
                Log.Comment("Expected: " + expectedElement.Content);
                Log.Comment("Actual: " + content);
                Verify.AreEqual(expectedElement, actualElement);
                expectedElement.Focus(FocusState.Keyboard);
            }

            Log.Comment("Validating backward tab navigation...");

            foreach (var expectedElement in Enumerable.Reverse(expectedSequence).Skip(1))
            {
                var actualElement = (Button)FocusManager.FindNextElement(FocusNavigationDirection.Previous, options);
                Log.Comment("Expected: " + expectedElement.Content);
                Log.Comment("Actual: " + actualElement.Content);
                Verify.AreEqual(expectedElement, actualElement);
                expectedElement.Focus(FocusState.Keyboard);
            }
        }

        private static Button[] GetRealizedRange(ItemsRepeater repeater)
        {
            return Enumerable.Range(0, VisualTreeHelper.GetChildrenCount(repeater))
                .Select(i => (Button)VisualTreeHelper.GetChild(repeater, i))
                .Where(e => repeater.GetElementIndex(e) >= 0)
                .OrderBy(e => repeater.GetElementIndex(e))
                .ToArray();
        }

        private static ItemsRepeaterScrollHost CreateAndInitializeRepeater(
           object itemsSource,
           VirtualizingLayout layout,
           ElementFactory elementFactory,
           ref ItemsRepeater repeater,
           ref ScrollViewer scrollViewer)
        {
            repeater = new ItemsRepeater()
            {
                ItemsSource = itemsSource,
                Layout = layout,
                ItemTemplate = elementFactory,
                VerticalCacheLength = 0
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
    }
}
