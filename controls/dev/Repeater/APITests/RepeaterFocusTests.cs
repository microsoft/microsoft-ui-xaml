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
using System.Runtime.InteropServices;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public partial class RepeaterFocusTests : ApiTestBase
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
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved to a cleared element while the collection change is processed.");
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
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved to a cleared element while the collection change is processed.");
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
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved to a cleared element while the collection change is processed.");
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
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved to a cleared element while the template change is processed.");
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
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved to a cleared element while the ItemsSource change is processed.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => newData[i], expectedIndex: 3));
        }

        // Scenario: two changes before the next layout pass: RemoveAt(focused) then Insert(0); and RemoveAt(focused) then
        //           RemoveAt(0).
        // Expected: one focus move after layout, to the item that followed the removed item (the second change shifts it:
        //           index 4 after the Insert, index 2 after the second RemoveAt).
        // A failure means: the target is chosen during the first change, before the second change shifts the items, or the
        //           index recorded for the move is not shifted by the second change.
        [TestMethod]
        [TestProperty("Description", "Verifies that when the focused item is removed and another change follows before layout, focus moves after layout to the item that followed the removed item.")]
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
            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 4);
                Verify.AreEqual("Item #4", data[4]);
            });

            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);
            RunOnUIThread.Execute(() =>
            {
                data.RemoveAt(3);
                data.RemoveAt(0);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 2);
                Verify.AreEqual("Item #4", data[2]);
            });
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

        // Scenario: item 3 is the tracked focused item; focus moves programmatically to item 2 and, in the same tick
        //           (before GotFocus is raised), items 2 and 3 are removed by one notification. Item 2 is cleared before
        //           the tracked item 3.
        // Expected: after layout focus is on the first survivor at index 2.
        // A failure means: the move is keyed to the tracked element only, so focus stays on the cleared item 2.
        [TestMethod]
        [TestProperty("Description", "Verifies that focusing the item before the tracked focused item and removing both in the same tick moves focus, after layout, to the first survivor.")]
        public void VerifyFocusChangeToPreviousItemAndRemoveInSameTickMovesFocusToSurvivor()
        {
            CustomItemsSource dataSource = null;
            RunOnUIThread.Execute(() => dataSource = new CustomItemsSource(Enumerable.Range(0, 10).ToList()));
            var fixture = CreateFocusFixture(dataSource);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(((Control)fixture.Repeater.TryGetElement(2)).Focus(FocusState.Keyboard));
                dataSource.Remove(2 /* index */, 2 /* count */, false /* reset */);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => dataSource.Inner[i].ToString(), expectedIndex: 2));
        }

        // Scenario: keyed source with an item transition provider that animates removals; a Reset removes the focused
        //           item's key, so its element is a keyed leftover that is handed to the transition (it fades out).
        // Expected: focus moves once, after layout, to the live item at the removed index; never onto the fading element;
        //           completing the transition does not move focus again.
        // A failure means: focus stays on, or moves onto, the fading element, or moves more than once.
        [TestMethod]
        [TestProperty("Description", "Verifies that when a keyed Reset removes the focused item and its removal is animated, focus moves once to the live item at the removed index and never onto the fading element.")]
        public void VerifyKeyedResetWithRemoveTransitionMovesFocusToLiveItem()
        {
            CustomItemsSource dataSource = null;
            ItemCollectionTransitionProviderDerived provider = null;
            var pendingRemoves = new List<ItemCollectionTransitionProgress>();
            RunOnUIThread.Execute(() =>
            {
                dataSource = new CustomItemsSourceWithUniqueId(Enumerable.Range(0, 10).ToList());
                provider = new ItemCollectionTransitionProviderDerived
                {
                    ShouldAnimateFunc = transition => transition.Operation == ItemCollectionTransitionOperation.Remove,
                    StartTransitionsFunc = transitions =>
                    {
                        foreach (var transition in transitions)
                        {
                            var progress = transition.Start();
                            if (transition.Operation == ItemCollectionTransitionOperation.Remove)
                            {
                                pendingRemoves.Add(progress);
                            }
                            else
                            {
                                progress.Complete();
                            }
                        }
                    }
                };
            });
            var fixture = CreateFocusFixture(dataSource);
            RunOnUIThread.Execute(() => fixture.Repeater.ItemTransitionProvider = provider);
            IdleSynchronizer.Wait();
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            UIElement removedElement = null;
            int gettingFocusCount = 0;
            RunOnUIThread.Execute(() =>
            {
                removedElement = fixture.Repeater.TryGetElement(3);
                fixture.Repeater.GettingFocus += (s, e) =>
                {
                    gettingFocusCount++;
                    Log.Comment("GettingFocus: " + DescribeElement(fixture.Repeater, e.NewFocusedElement));
                };
                dataSource.Remove(3 /* index */, 1 /* count */, true /* reset */);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"Remove transitions in progress: {pendingRemoves.Count}");
                Verify.AreNotSame(removedElement, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus is not on the removed item's element.");
                VerifyFocusOnLiveItem(fixture.Repeater, i => dataSource.Inner[i].ToString(), expectedIndex: 3);
                Verify.AreEqual(1, gettingFocusCount, "Focus is moved once.");

                foreach (var progress in pendingRemoves)
                {
                    progress.Complete();
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOnLiveItem(fixture.Repeater, i => dataSource.Inner[i].ToString(), expectedIndex: 3);
                Verify.AreEqual(1, gettingFocusCount, "Completing the transition does not move focus again.");
            });
        }

        // Scenario: item containers are StackPanels with three Buttons; focus is on the middle Button of an item; Clear().
        // Expected: focus moves to the element after the repeater ("After").
        // A failure means: focus stays on a Button inside a cleared container.
        [TestMethod]
        [TestProperty("Description", "Verifies that clearing the ItemsSource with focus on a middle descendant of a Panel container moves focus to the element after the repeater.")]
        public void VerifyClearWithFocusInsidePanelContainerMovesFocusOutOfRepeater()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data, itemTemplate:
                "<StackPanel Orientation='Horizontal' Height='30'><Button Content='{Binding}'/><Button Content='Middle'/><Button Content='Last'/></StackPanel>");
            FocusDescendantAndWait(fixture.Repeater, 1, "Middle");

            RunOnUIThread.Execute(() => data.Clear());
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOutsideRepeater(fixture);
                Verify.AreSame(fixture.After, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus moves to the element after the repeater.");
            });
        }

        // Scenario: item containers are focusable ContentControls that contain two Buttons; focus is on the first inner
        //           Button of an item; Clear(). A Tab step from that Button stays inside the container (the second Button,
        //           or the ContentControl itself for Shift+Tab).
        // Expected: focus moves to the element after the repeater ("After").
        // A failure means: the move out of the repeater stops at elements inside the cleared container, so focus stays on it.
        [TestMethod]
        [TestProperty("Description", "Verifies that clearing the ItemsSource with focus inside a focusable Control container moves focus to the element after the repeater, not to elements of the cleared container.")]
        public void VerifyClearWithFocusInsideControlContainerMovesFocusOutOfRepeater()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data, itemTemplate:
                "<ContentControl IsTabStop='True' Height='30'><StackPanel Orientation='Horizontal'><Button Content='First'/><Button Content='Second'/></StackPanel></ContentControl>");
            FocusDescendantAndWait(fixture.Repeater, 1, "First");

            RunOnUIThread.Execute(() => data.Clear());
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOutsideRepeater(fixture);
                Verify.AreSame(fixture.After, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus moves to the element after the repeater.");
            });
        }

        // Scenario: the repeater is the last focusable content (no element after it); focus inside; Clear().
        // Expected: focus moves to the element just before the repeater ("Middle"); it does not cycle to the first
        //           focusable element of the window.
        // A failure means: focus jumps to the top of the window (Tab cycle) or stays on a cleared element.
        [TestMethod]
        [TestProperty("Description", "Verifies that clearing the ItemsSource of the last focusable repeater moves focus to the element just before it instead of cycling to the start of the window.")]
        public void VerifyClearOfLastRepeaterMovesFocusToPreviousElement()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            ItemsRepeater repeater = null;
            Button middle = null;
            RunOnUIThread.Execute(() =>
            {
                repeater = new ItemsRepeater { ItemsSource = data, ItemTemplate = CreateElementFactory(ButtonItemTemplate) };
                middle = new Button { Content = "Middle" };
                var root = new StackPanel();
                root.Children.Add(new Button { Content = "Top" });
                root.Children.Add(middle);
                root.Children.Add(new ScrollViewer { Content = repeater, Height = 200, Width = 200 });
                Content = root;
            });
            IdleSynchronizer.Wait();
            FocusItemAndWait(repeater, 2, FocusState.Keyboard);

            RunOnUIThread.Execute(() => data.Clear());
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(repeater.XamlRoot) as DependencyObject;
                Log.Comment("Focused: " + DescribeElement(repeater, focused));
                Verify.AreSame(middle, focused, "Focus moves to the element just before the repeater.");
            });
        }

        // Scenario: the repeater is inside an open Flyout, between two Buttons; focus inside; Clear().
        // Expected: focus moves to the Button after the repeater inside the flyout, and the flyout stays open.
        // A failure means: focus leaves the flyout (which light-dismisses it) or stays on a cleared element.
        [TestMethod]
        [TestProperty("Description", "Verifies that clearing the ItemsSource of a repeater inside an open Flyout keeps focus inside the flyout and the flyout open.")]
        public void VerifyClearInsideFlyoutKeepsFocusInFlyout()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            ItemsRepeater repeater = null;
            Button bottom = null;
            Button anchor = null;
            Flyout flyout = null;
            var opened = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                repeater = new ItemsRepeater { ItemsSource = data, ItemTemplate = CreateElementFactory(ButtonItemTemplate) };
                bottom = new Button { Content = "Bottom" };
                var panel = new StackPanel();
                panel.Children.Add(new Button { Content = "Top" });
                panel.Children.Add(repeater);
                panel.Children.Add(bottom);
                flyout = new Flyout { Content = panel };
                flyout.Opened += delegate { opened.Set(); };
                anchor = new Button { Content = "Anchor" };
                Content = anchor;
            });
            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() => flyout.ShowAt(anchor));
                Verify.IsTrue(opened.WaitOne(DefaultWaitTimeInMS), "Waiting for the flyout to open.");
                IdleSynchronizer.Wait();
                FocusItemAndWait(repeater, 1, FocusState.Keyboard);

                RunOnUIThread.Execute(() => data.Clear());
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var focused = FocusManager.GetFocusedElement(repeater.XamlRoot) as DependencyObject;
                    Log.Comment("Focused: " + DescribeElement(repeater, focused));
                    Verify.IsTrue(flyout.IsOpen, "The flyout stays open.");
                    Verify.AreSame(bottom, focused, "Focus moves to the Button after the repeater inside the flyout.");
                });
            }
            finally
            {
                RunOnUIThread.Execute(() => flyout.Hide());
                IdleSynchronizer.Wait();
            }
        }

        // Scenario: the repeater is inside a StackPanel with TabFocusNavigation=Cycle, after a Button ("InsideBefore"); a Button
        //           ("AfterGroup") follows the group outside it; focus on an item; Clear().
        // Expected: focus moves to "InsideBefore". Tab wraps inside a Cycle group, so the move does not leave the group.
        // A failure means: focus leaves a Cycle group, for example a NavigationView pane open in overlay mode, which traps focus.
        [TestMethod]
        [TestProperty("Description", "Verifies that clearing the ItemsSource of a repeater inside a TabFocusNavigation=Cycle group keeps focus inside the group.")]
        public void VerifyClearInsideCycleGroupKeepsFocusInGroup()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            ItemsRepeater repeater = null;
            Button insideBefore = null;
            RunOnUIThread.Execute(() =>
            {
                repeater = new ItemsRepeater { ItemsSource = data, ItemTemplate = CreateElementFactory(ButtonItemTemplate) };
                insideBefore = new Button { Content = "InsideBefore" };
                var group = new StackPanel { TabFocusNavigation = KeyboardNavigationMode.Cycle };
                group.Children.Add(insideBefore);
                group.Children.Add(new ScrollViewer { Content = repeater, Height = 200, Width = 200 });
                var root = new StackPanel();
                root.Children.Add(new Button { Content = "Top" });
                root.Children.Add(group);
                root.Children.Add(new Button { Content = "AfterGroup" });
                Content = root;
            });
            IdleSynchronizer.Wait();
            FocusItemAndWait(repeater, 2, FocusState.Keyboard);

            RunOnUIThread.Execute(() => data.Clear());
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(repeater.XamlRoot) as DependencyObject;
                Log.Comment("Focused: " + DescribeElement(repeater, focused));
                Verify.AreSame(insideBefore, focused, "Focus stays inside the Cycle group, on the element before the repeater.");
            });
        }

        // Scenario: the repeater is the content of a focusable ContentControl (IsTabStop=True), with a Button ("Before")
        //           before the ContentControl and nothing focusable after it; focus on an item; Clear().
        // Expected: focus moves to the ContentControl, the element that Shift+Tab reaches first from the repeater.
        // A failure means: the move skips the focusable parent Control and goes to "Before", or stays on a cleared element.
        [TestMethod]
        [TestProperty("Description", "Verifies that clearing the ItemsSource of a repeater hosted in a focusable Control, with nothing focusable after it, moves focus to that Control.")]
        public void VerifyClearOfRepeaterInFocusableControlMovesFocusToControl()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            ItemsRepeater repeater = null;
            ContentControl host = null;
            RunOnUIThread.Execute(() =>
            {
                repeater = new ItemsRepeater { ItemsSource = data, ItemTemplate = CreateElementFactory(ButtonItemTemplate) };
                host = new ContentControl { IsTabStop = true, Content = repeater };
                var root = new StackPanel();
                root.Children.Add(new Button { Content = "Before" });
                root.Children.Add(host);
                Content = root;
            });
            IdleSynchronizer.Wait();
            FocusItemAndWait(repeater, 2, FocusState.Keyboard);

            RunOnUIThread.Execute(() => data.Clear());
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(repeater.XamlRoot) as DependencyObject;
                Log.Comment("Focused: " + DescribeElement(repeater, focused));
                Verify.AreSame(host, focused, "Focus moves to the focusable Control that hosts the repeater.");
            });
        }

        // Scenario: nested repeaters (the outer ItemTemplate root is an inner ItemsRepeater); a group is inserted at index 0,
        //           so its outer element is the last outer child in visual order but the first by index; focus on an inner
        //           item of that group; the group is emptied (Clear()).
        // Expected: focus moves to the first inner item of the group at outer index 1, the next group in index (and Tab) order.
        // A failure means: the outer level is searched in visual child order, so the move skips the following groups and
        //                  goes to the element after the outer repeater ("After").
        [TestMethod]
        [TestProperty("Description", "Verifies that emptying an inner repeater with focus on its item moves focus to the first item of the next group in index order, not in visual child order.")]
        public void VerifyEmptyingInnerRepeaterMovesFocusToNextGroupInIndexOrder()
        {
            var groups = new ObservableCollection<ObservableCollection<string>>(Enumerable.Range(0, 3).Select(g =>
                new ObservableCollection<string>(Enumerable.Range(0, 2).Select(i => $"Item #{g}.{i}"))));
            var outer = CreateNestedRepeaters(groups, out _);

            RunOnUIThread.Execute(() =>
            {
                groups.Insert(0, new ObservableCollection<string>(Enumerable.Range(0, 2).Select(i => $"New #{i}")));
                outer.UpdateLayout();

                var first = outer.TryGetElement(0);
                var second = outer.TryGetElement(1);
                int firstChildIndex = IndexOfVisualChild(outer, first);
                int secondChildIndex = IndexOfVisualChild(outer, second);
                Log.Comment($"Visual child index of outer index 0: {firstChildIndex}, of outer index 1: {secondChildIndex}");
                Verify.IsTrue(firstChildIndex > secondChildIndex, "Precondition: the visual child order differs from the index order.");
            });
            IdleSynchronizer.Wait();
            FocusInnerItemAndWait(outer, 0, 1);

            RunOnUIThread.Execute(() => groups[0].Clear());
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(outer.XamlRoot) as Button;
                Log.Comment($"Focused: {focused?.GetType().Name} '{focused?.Content}'");
                Verify.IsNotNull(focused, "A Button is focused.");
                Verify.AreEqual("Item #0.0", focused.Content, "Focus is on the first item of the next group.");
                Verify.AreSame(outer.TryGetElement(1), VisualTreeHelper.GetParent(focused), "The focused item belongs to the group at outer index 1.");
            });
        }

        // Scenario: nested repeaters of string groups (the outer repeater sets the DataContext of each inner repeater and
        //           clears it when the element is cleared, which empties the inner repeater); an "After" Button follows the
        //           outer repeater; focus on an inner item of group k; groups.RemoveAt(k). Both repeaters then have a focus
        //           move pending, and the order of their LayoutUpdated handlers is not defined, so the scenario is repeated
        //           on six fresh trees (k = 1, 0, 2, twice).
        // Expected: focus moves to the nearest group (the next one, or the previous one when the last group was removed),
        //           whatever the handler order; it does not leave the outer repeater.
        // A failure means: the inner repeater moves focus out of the outer repeater ("After") because its own container
        //                  was cleared, although the outer repeater still has groups.
        [TestMethod]
        [TestProperty("Description", "Verifies that removing a group with focus on one of its inner items moves focus to an item of the nearest group, not out of the outer repeater.")]
        public void VerifyRemovingGroupWithFocusedInnerItemMovesFocusToNearestGroup()
        {
            foreach (int k in new[] { 1, 0, 2, 1, 0, 2 })
            {
                Log.Comment($"Removing group {k}");
                var groups = new ObservableCollection<ObservableCollection<string>>(Enumerable.Range(0, 3).Select(g =>
                    new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => $"Item #{g}.{i}"))));
                var outer = CreateNestedRepeaters(groups, out var after);
                FocusInnerItemAndWait(outer, k, 1);

                RunOnUIThread.Execute(() => groups.RemoveAt(k));
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var focused = FocusManager.GetFocusedElement(outer.XamlRoot) as Button;
                    Log.Comment($"Focused: {focused?.GetType().Name} '{focused?.Content}'");
                    Verify.IsNotNull(focused, "A Button is focused.");
                    Verify.AreNotSame(after, focused, "Focus does not leave the outer repeater.");

                    int expectedGroup = k < groups.Count ? k : k - 1;
                    string expectedContent = k < groups.Count ? groups[expectedGroup][0] : groups[expectedGroup][2];
                    Verify.AreEqual(expectedContent, focused.Content, "Focus is on the nearest item of the nearest group.");
                    var inner = VisualTreeHelper.GetParent(focused) as ItemsRepeater;
                    Verify.AreSame(outer.TryGetElement(expectedGroup), inner, "The focused item belongs to the live outer element of that group.");
                    Verify.IsTrue(inner.GetElementIndex(focused) >= 0, "The inner item is realized.");
                });
            }
        }

        // Scenario: nested repeaters of string groups; focus on inner item 1 of group k; group k is replaced by an empty group
        //           (groups[k] = new ObservableCollection). The outer repeater reuses the cleared element for the new group at
        //           index k, so the focused inner item is now a recycled child of that element's (empty) inner repeater.
        //           Repeated on fresh trees until both LayoutUpdated handler orders (inner first, outer first) have run.
        // Expected: focus moves to the first item of the next group (the last item of the previous group when the last group
        //           was replaced), whatever the handler order.
        // A failure means: focus stays on the recycled inner item inside the reused element (Tab and Narrator then reach an
        //                  element that is not shown), or leaves the outer repeater.
        [TestMethod]
        [TestProperty("Description", "Verifies that replacing a group by an empty group, with focus on one of its inner items, moves focus to the nearest item of the nearest group, whatever the LayoutUpdated handler order.")]
        public void VerifyReplacingGroupWithEmptyGroupMovesFocusToNearestGroup()
        {
            RunNestedGroupChangeInBothHandlerOrders(
                (groups, outer, k) =>
                {
                    groups[k] = new ObservableCollection<string>();
                    return groups;
                },
                (groups, k) => k + 1 < groups.Count ? (k + 1, groups[k + 1][0]) : (k - 1, groups[k - 1][groups[k - 1].Count - 1]));
        }

        // Scenario: nested repeaters of string groups; focus on inner item 1 of group k; the outer ItemsSource is replaced by a
        //           new list that holds the same groups, except that group k is a new empty group (Reset). The outer repeater
        //           reuses its cleared elements for the new list in another order (observed: new index i gets the element of
        //           old index i + 1, wrapping), so the element that held focus gets a non-empty group. Repeated on fresh trees
        //           until both LayoutUpdated handler orders have run.
        // Expected: focus moves to a live item of the next group (of the previous group when the last group was emptied).
        // A failure means: focus stays on the recycled inner item inside a reused element, or leaves the outer repeater.
        [TestMethod]
        [TestProperty("Description", "Verifies that reassigning the outer ItemsSource with the focused group emptied moves focus to a live item of the nearest group, whatever the LayoutUpdated handler order.")]
        public void VerifyResetWithFocusedGroupEmptiedMovesFocusToNearestGroup()
        {
            RunNestedGroupChangeInBothHandlerOrders(
                (groups, outer, k) =>
                {
                    var newGroups = new ObservableCollection<ObservableCollection<string>>(
                        groups.Select((group, i) => i == k ? new ObservableCollection<string>() : group));
                    outer.ItemsSource = newGroups;
                    return newGroups;
                },
                (groups, k) => k + 1 < groups.Count ? (k + 1, (string)null) : (k - 1, (string)null));
        }

        // Scenario: nested repeaters of string groups; focus on inner item 1 of group k; the outer ItemsSource is replaced by a
        //           list of three new empty groups (Reset). The outer repeater reuses its cleared elements, so the element
        //           that held focus is realized again with an empty inner repeater, whatever the reuse order. Repeated on
        //           fresh trees until both LayoutUpdated handler orders have run.
        // Expected: no group has an item, so focus moves to the element after the outer repeater ("After").
        // A failure means: focus stays on the recycled inner item inside a reused element.
        [TestMethod]
        [TestProperty("Description", "Verifies that reassigning the outer ItemsSource to empty groups, with focus on an inner item, moves focus out of the outer repeater, whatever the LayoutUpdated handler order.")]
        public void VerifyResetToEmptyGroupsMovesFocusOutOfOuterRepeater()
        {
            RunNestedGroupChangeInBothHandlerOrders(
                (groups, outer, k) =>
                {
                    var newGroups = new ObservableCollection<ObservableCollection<string>>(
                        groups.Select(group => new ObservableCollection<string>()));
                    outer.ItemsSource = newGroups;
                    return newGroups;
                },
                (groups, k) => (-1, (string)null));
        }

        // Scenario: nested repeaters of string groups; focus on inner item 1 of group k; group k is replaced by a new group of
        //           three items. The outer repeater reuses the cleared element for the new group, and its inner repeater
        //           reuses the focused inner element for the new item 1. Repeated on fresh trees until both LayoutUpdated
        //           handler orders have run.
        // Expected: focus stays on item 1 of the new group (the live item at the focused item's index).
        // A failure means: the outer repeater moves focus to another item or group although the reused element has a live
        //                  item at the focused index.
        [TestMethod]
        [TestProperty("Description", "Verifies that replacing a group by a non-empty group, with focus on one of its inner items, moves focus to the new group's item at the same index, whatever the LayoutUpdated handler order.")]
        public void VerifyReplacingGroupWithNonEmptyGroupFocusesItemAtSameIndex()
        {
            RunNestedGroupChangeInBothHandlerOrders(
                (groups, outer, k) =>
                {
                    groups[k] = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => $"New #{k}.{i}"));
                    return groups;
                },
                (groups, k) => (k, groups[k][1]));
        }

        // Scenario: nested repeaters built with element factories: the outer factory pools the inner repeaters it recycles
        //           and reuses them first in, first out; the inner factory never reuses a Button, so a cleared inner Button
        //           stays a recycled child of its repeater. Focus on inner item 2 (the last) of group k; group k is replaced
        //           by a new group of three items, so the outer repeater reuses the cleared element for the new group while
        //           focus is on a recycled inner Button. Repeated on fresh trees until both LayoutUpdated handler orders
        //           have run.
        // Expected: focus moves to item 2 of the new group, the live inner item at the focused item's index.
        // A failure means: focus moves to the first item of the new group (the outer repeater chose the target instead of
        //                  the inner repeater, so the result depends on the handler order), or stays on the recycled Button.
        [TestMethod]
        [TestProperty("Description", "Verifies that when the outer repeater reuses the cleared element for a replaced group while focus is on a recycled inner element, focus moves to the new group's item at the focused index, whatever the LayoutUpdated handler order.")]
        public void VerifyReplacingGroupInReusedElementFocusesInnerItemAtSameIndex()
        {
            RunNestedGroupChangeInBothHandlerOrders(
                (groups, outer, k) =>
                {
                    groups[k] = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => $"New #{k}.{i}"));
                    return groups;
                },
                (groups, k) => (k, groups[k][2]),
                focusedItem: 2,
                createItemTemplate: () => new PoolingGroupFactory());
        }

        // Scenario: the trees of VerifyReplacingGroupInReusedElementFocusesInnerItemAtSameIndex (focus on a recycled inner Button
        //           inside the reused element after group k is replaced by three new items). A GettingFocus handler replaces,
        //           once, the inner item that the focus move targets, so the inner repeater clears the target while focus moves
        //           to it. Repeated on fresh trees until both LayoutUpdated handler orders have run.
        // Expected: focus ends on a live inner item of the new group k, not on the target that the handler's change cleared.
        // A failure means: when the outer repeater makes the inner repeater move focus, a handler's change that clears the
        //                  target is not followed by another move, so focus stays on a recycled Button.
        [TestMethod]
        [TestProperty("Description", "Verifies that when a GettingFocus handler clears the inner item that a nested focus move targets, focus still ends on a live inner item, whatever the LayoutUpdated handler order.")]
        public void VerifyGettingFocusHandlerClearingNestedTargetLeavesFocusOnLiveInnerItem()
        {
            int changes = 0;
            int handlerChanges = 0;
            RunNestedGroupChangeInBothHandlerOrders(
                (groups, outer, k) =>
                {
                    changes++;
                    bool changed = false;
                    outer.GettingFocus += (s, e) =>
                    {
                        if (changed || !(e.NewFocusedElement is Button target) || !(target.Content as string ?? "").StartsWith("New #"))
                        {
                            return;
                        }

                        var targetRepeater = VisualTreeHelper.GetParent(target) as ItemsRepeater;
                        int index = targetRepeater?.GetElementIndex(target) ?? -1;
                        if (index < 0)
                        {
                            return;
                        }

                        changed = true;
                        handlerChanges++;
                        Log.Comment($"GettingFocus target: '{target.Content}' (inner index {index}); replacing that inner item from the handler.");
                        ((ObservableCollection<string>)targetRepeater.ItemsSource)[index] = $"Changed #{k}.{index}";
                    };
                    groups[k] = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => $"New #{k}.{i}"));
                    return groups;
                },
                (groups, k) => (k, (string)null),
                focusedItem: 2,
                createItemTemplate: () => new PoolingGroupFactory());

            Verify.AreEqual(changes, handlerChanges, "Precondition: in every tree the GettingFocus handler replaced the targeted inner item.");
        }

        // Outer ItemTemplate: an inner ItemsRepeater per group, pooled on recycle (its ItemsSource is cleared) and reused
        // first in, first out.
        private partial class PoolingGroupFactory : ElementFactory
        {
            private readonly Queue<ItemsRepeater> _pool = new Queue<ItemsRepeater>();

            protected override UIElement GetElementCore(ElementFactoryGetArgs args)
            {
                var repeater = _pool.Count > 0 ? _pool.Dequeue() : new ItemsRepeater { ItemTemplate = new NonRecyclingButtonFactory() };
                repeater.ItemsSource = args.Data;
                return repeater;
            }

            protected override void RecycleElementCore(ElementFactoryRecycleArgs args)
            {
                var repeater = (ItemsRepeater)args.Element;
                repeater.ItemsSource = null;
                _pool.Enqueue(repeater);
            }
        }

        // Inner ItemTemplate: a new Button for every item; recycled Buttons are never reused.
        private partial class NonRecyclingButtonFactory : ElementFactory
        {
            protected override UIElement GetElementCore(ElementFactoryGetArgs args)
            {
                return new Button { Content = args.Data, Height = 30 };
            }

            protected override void RecycleElementCore(ElementFactoryRecycleArgs args)
            {
            }
        }

        // Runs a change of the nested trees' outer source, with focus on inner item focusedItem of group k (k = 1, 0, 2, ...),
        // on fresh trees (at least 6, so that each k runs twice) until the LayoutUpdated handler of the focused group's inner
        // repeater has run before the outer repeater's handler at least twice and after it at least once (the order follows
        // the element addresses), or 12 trees.
        // `expected` returns the group index of the expected focused item and its content (null: any live item of that group).
        // `createItemTemplate` (optional) creates the outer ItemTemplate; by default it is CreateNestedRepeaters' DataTemplate.
        private void RunNestedGroupChangeInBothHandlerOrders(
            Func<ObservableCollection<ObservableCollection<string>>, ItemsRepeater, int, ObservableCollection<ObservableCollection<string>>> change,
            Func<ObservableCollection<ObservableCollection<string>>, int, (int group, string content)> expected,
            int focusedItem = 1,
            Func<object> createItemTemplate = null)
        {
            int[] groupIndices = { 1, 0, 2 };
            int innerFirst = 0;
            int outerFirst = 0;
            int trees = 0;
            while (trees < 12 && (trees < 6 || innerFirst < 2 || outerFirst < 1))
            {
                int k = groupIndices[trees % groupIndices.Length];
                trees++;
                var groups = new ObservableCollection<ObservableCollection<string>>(Enumerable.Range(0, 3).Select(g =>
                    new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => $"Item #{g}.{i}"))));
                var outer = CreateNestedRepeaters(groups, out var after, createItemTemplate);
                FocusInnerItemAndWait(outer, k, focusedItem);

                string firstHandler = null;
                Button focusedBefore = null;
                UIElement[] elementsBefore = null;
                ObservableCollection<ObservableCollection<string>> source = null;
                RunOnUIThread.Execute(() =>
                {
                    focusedBefore = (Button)FocusManager.GetFocusedElement(outer.XamlRoot);
                    elementsBefore = Enumerable.Range(0, groups.Count).Select(i => outer.TryGetElement(i)).ToArray();
                    var inner = (ItemsRepeater)outer.TryGetElement(k);
                    inner.LayoutUpdated += delegate { firstHandler = firstHandler ?? "inner"; };
                    outer.LayoutUpdated += delegate { firstHandler = firstHandler ?? "outer"; };
                    source = change(groups, outer, k);
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    if (firstHandler == "inner") { innerFirst++; } else if (firstHandler == "outer") { outerFirst++; }
                    var focused = FocusManager.GetFocusedElement(outer.XamlRoot) as Button;
                    Log.Comment($"Tree {trees}, group {k}: first LayoutUpdated handler '{firstHandler}'; focused {focused?.GetType().Name} '{focused?.Content}'" +
                        (focused == focusedBefore ? " (the item focused before the change)" : ""));
                    Log.Comment("Outer element reuse (new index <- old index): " + string.Join(", ", Enumerable.Range(0, source.Count).Select(i =>
                        $"{i} <- {Array.IndexOf(elementsBefore, outer.TryGetElement(i))}")));
                    Verify.IsNotNull(focused, "A Button is focused.");
                    var (expectedGroup, expectedContent) = expected(source, k);
                    if (expectedGroup < 0)
                    {
                        Verify.AreSame(after, focused, "Focus moves to the element after the outer repeater.");
                        return;
                    }

                    Verify.AreNotSame(after, focused, "Focus does not leave the outer repeater.");
                    var inner = VisualTreeHelper.GetParent(focused) as ItemsRepeater;
                    Verify.IsNotNull(inner, "The focused Button is an inner item.");
                    Verify.IsTrue(inner.GetElementIndex(focused) >= 0, "The focused inner item is realized (not a recycled element).");

                    Verify.AreSame(outer.TryGetElement(expectedGroup), inner, $"The focused item belongs to the live outer element of group {expectedGroup}.");
                    if (expectedContent != null)
                    {
                        Verify.AreEqual(expectedContent, focused.Content, "Focus is on the expected item.");
                    }
                });
            }

            Log.Comment($"Trees: {trees}; inner handler first: {innerFirst}; outer handler first: {outerFirst}");
            Verify.IsTrue(innerFirst > 0, "Precondition: at least one tree ran the inner repeater's LayoutUpdated handler first.");
        }

        // Scenario: nested repeaters of string groups; the outer repeater has an ItemTransitionProvider that animates removals
        //           (the transitions are kept running); focus on inner item 1 of group k; groups.RemoveAt(k). The removed
        //           element fades out with its inner items still realized. Run for k = 1 and k = 2 (last group).
        // Expected: after the layout pass, focus is on the nearest item of the nearest live group; completing the transitions
        //           does not move focus again.
        // A failure means: focus stays inside the fading element until its transition completes, or moves twice.
        [TestMethod]
        [TestProperty("Description", "Verifies that removing a group whose removal is animated, with focus on one of its inner items, moves focus once to the nearest live group, not inside the fading element.")]
        public void VerifyAnimatedGroupRemovalWithFocusedInnerItemMovesFocusToNearestGroup()
        {
            foreach (int k in new[] { 1, 2 })
            {
                Log.Comment($"Removing group {k}");
                var groups = new ObservableCollection<ObservableCollection<string>>(Enumerable.Range(0, 3).Select(g =>
                    new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => $"Item #{g}.{i}"))));
                var outer = CreateNestedRepeaters(groups, out var after);
                var pendingRemoves = new List<ItemCollectionTransitionProgress>();
                RunOnUIThread.Execute(() =>
                {
                    outer.ItemTransitionProvider = new ItemCollectionTransitionProviderDerived
                    {
                        ShouldAnimateFunc = transition => transition.Operation == ItemCollectionTransitionOperation.Remove,
                        StartTransitionsFunc = transitions =>
                        {
                            foreach (var transition in transitions)
                            {
                                var progress = transition.Start();
                                if (transition.Operation == ItemCollectionTransitionOperation.Remove)
                                {
                                    pendingRemoves.Add(progress);
                                }
                                else
                                {
                                    progress.Complete();
                                }
                            }
                        }
                    };
                });
                IdleSynchronizer.Wait();
                FocusInnerItemAndWait(outer, k, 1);

                UIElement removedElement = null;
                Button focusedAfterLayout = null;
                int gettingFocusCount = 0;
                RunOnUIThread.Execute(() =>
                {
                    removedElement = outer.TryGetElement(k);
                    outer.GettingFocus += (s, e) =>
                    {
                        gettingFocusCount++;
                        Log.Comment($"GettingFocus: {(e.NewFocusedElement as Button)?.Content}");
                    };
                    groups.RemoveAt(k);
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Remove transitions in progress: {pendingRemoves.Count}");
                    Verify.IsTrue(pendingRemoves.Count > 0, "Precondition: the removal is animated.");
                    focusedAfterLayout = FocusManager.GetFocusedElement(outer.XamlRoot) as Button;
                    Log.Comment($"Focused: {focusedAfterLayout?.GetType().Name} '{focusedAfterLayout?.Content}'");
                    Verify.IsNotNull(focusedAfterLayout, "A Button is focused.");
                    Verify.AreNotSame(after, focusedAfterLayout, "Focus does not leave the outer repeater.");
                    Verify.AreNotSame(removedElement, VisualTreeHelper.GetParent(focusedAfterLayout), "Focus is not inside the fading element.");

                    int expectedGroup = k < groups.Count ? k : k - 1;
                    string expectedContent = k < groups.Count ? groups[expectedGroup][0] : groups[expectedGroup][2];
                    Verify.AreEqual(expectedContent, focusedAfterLayout.Content, "Focus is on the nearest item of the nearest group.");
                    Verify.AreSame(outer.TryGetElement(expectedGroup), VisualTreeHelper.GetParent(focusedAfterLayout), "The focused item belongs to the live outer element of that group.");
                    Verify.AreEqual(1, gettingFocusCount, "Focus is moved once.");

                    foreach (var progress in pendingRemoves)
                    {
                        progress.Complete();
                    }
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreSame(focusedAfterLayout, FocusManager.GetFocusedElement(outer.XamlRoot), "Completing the transition does not move focus.");
                    Verify.AreEqual(1, gettingFocusCount, "Focus is still moved once.");
                });
            }
        }

        // Scenario: a SplitView with DisplayMode=Overlay and the pane open; the pane holds a Button ("PaneTop") and the repeater;
        //           the SplitView content is a Button ("ContentButton"); focus on an item; Clear().
        // Expected: focus moves to "PaneTop" and the pane stays open. Tab keeps focus inside an open pane that can be light
        //           dismissed, so the move does not leave the pane.
        // A failure means: focus moves to the SplitView content behind the open pane.
        [TestMethod]
        [TestProperty("Description", "Verifies that clearing the ItemsSource of a repeater in the open pane of an overlay SplitView keeps focus inside the pane.")]
        public void VerifyClearInsideOpenOverlaySplitViewPaneKeepsFocusInPane()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            ItemsRepeater repeater = null;
            Button paneTop = null;
            SplitView splitView = null;
            RunOnUIThread.Execute(() =>
            {
                repeater = new ItemsRepeater { ItemsSource = data, ItemTemplate = CreateElementFactory(ButtonItemTemplate) };
                paneTop = new Button { Content = "PaneTop" };
                var pane = new StackPanel();
                pane.Children.Add(paneTop);
                pane.Children.Add(repeater);
                splitView = new SplitView
                {
                    DisplayMode = SplitViewDisplayMode.Overlay,
                    IsPaneOpen = true,
                    OpenPaneLength = 200,
                    Pane = pane,
                    Content = new Button { Content = "ContentButton" },
                    Width = 400,
                    Height = 400
                };
                Content = splitView;
            });
            IdleSynchronizer.Wait();
            FocusItemAndWait(repeater, 2, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(splitView.IsPaneOpen, "Precondition: the pane is open.");
                data.Clear();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(repeater.XamlRoot) as DependencyObject;
                Log.Comment("Focused: " + DescribeElement(repeater, focused) + "; pane open: " + splitView.IsPaneOpen);
                Verify.AreSame(paneTop, focused, "Focus stays inside the open pane, on the element before the repeater.");
                Verify.IsTrue(splitView.IsPaneOpen, "The pane stays open.");
            });
        }

        // Scenario: a SplitView with DisplayMode=Overlay and the pane open; the pane holds only the repeater; the SplitView
        //           content is a Button ("ContentButton"); focus on an item; Clear().
        // Expected: the pane has nothing left to focus, so focus leaves it, as Tab does (the SplitView keeps Tab inside the
        //           pane only when the pane has a focusable element): focus moves to "ContentButton".
        // A failure means: focus stays on the cleared item (Tab and Narrator then reach an element that is not shown).
        [TestMethod]
        [TestProperty("Description", "Verifies that clearing the ItemsSource of the only focusable content of an open overlay SplitView pane moves focus out of the pane, not leaving it on the cleared item.")]
        public void VerifyClearOfOnlyRepeaterInOpenOverlaySplitViewPaneMovesFocusOutOfPane()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            ItemsRepeater repeater = null;
            Button contentButton = null;
            SplitView splitView = null;
            RunOnUIThread.Execute(() =>
            {
                repeater = new ItemsRepeater { ItemsSource = data, ItemTemplate = CreateElementFactory(ButtonItemTemplate) };
                var pane = new StackPanel();
                pane.Children.Add(repeater);
                contentButton = new Button { Content = "ContentButton" };
                splitView = new SplitView
                {
                    DisplayMode = SplitViewDisplayMode.Overlay,
                    IsPaneOpen = true,
                    OpenPaneLength = 200,
                    Pane = pane,
                    Content = contentButton,
                    Width = 400,
                    Height = 400
                };
                Content = splitView;
            });
            IdleSynchronizer.Wait();
            FocusItemAndWait(repeater, 2, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(splitView.IsPaneOpen, "Precondition: the pane is open.");
                data.Clear();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(repeater.XamlRoot) as DependencyObject;
                Log.Comment("Focused: " + DescribeElement(repeater, focused) + "; pane open: " + splitView.IsPaneOpen);
                Verify.AreSame(contentButton, focused, "Focus leaves the pane, which has nothing else to focus, to the SplitView content.");
            });
        }

        // Creates an outer repeater whose ItemTemplate root is an inner repeater bound to the group, between a "Before" and
        // an "After" Button, without a ScrollViewer (every group is realized). createItemTemplate (optional) replaces the
        // ItemTemplate.
        private ItemsRepeater CreateNestedRepeaters(ObservableCollection<ObservableCollection<string>> groups, out Button after, Func<object> createItemTemplate = null)
        {
            ItemsRepeater outer = null;
            Button afterButton = null;
            RunOnUIThread.Execute(() =>
            {
                outer = new ItemsRepeater
                {
                    ItemsSource = groups,
                    ItemTemplate = createItemTemplate?.Invoke() ?? SharedHelpers.GetDataTemplate(
                        @"<ItemsRepeater ItemsSource='{Binding}'>
                            <ItemsRepeater.ItemTemplate>
                                <DataTemplate><Button Content='{Binding}' Height='30'/></DataTemplate>
                            </ItemsRepeater.ItemTemplate>
                          </ItemsRepeater>"),
                    VerticalAlignment = VerticalAlignment.Top
                };
                afterButton = new Button { Content = "After" };
                var root = new StackPanel();
                root.Children.Add(new Button { Content = "Before" });
                root.Children.Add(outer);
                root.Children.Add(afterButton);
                Content = root;
            });
            IdleSynchronizer.Wait();
            after = afterButton;
            return outer;
        }

        private void FocusInnerItemAndWait(ItemsRepeater outer, int group, int item)
        {
            var gotFocus = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                var inner = (ItemsRepeater)outer.TryGetElement(group);
                var target = (Button)inner.TryGetElement(item);
                target.GotFocus += delegate { gotFocus.Set(); };
                Verify.IsTrue(target.Focus(FocusState.Keyboard));
            });
            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), $"Waiting for inner item {item} of group {group} to get focus.");
            IdleSynchronizer.Wait();
        }

        private static int IndexOfVisualChild(DependencyObject parent, DependencyObject child)
        {
            int count = VisualTreeHelper.GetChildrenCount(parent);
            for (int i = 0; i < count; i++)
            {
                if (VisualTreeHelper.GetChild(parent, i) == child)
                {
                    return i;
                }
            }
            return -1;
        }

        // Scenario: every item except the focused one is disabled; the focused item is removed.
        // Expected: no live item can take focus, so focus moves out of the repeater to the element after it.
        // A failure means: focus stays on the removed, cleared element.
        [TestMethod]
        [TestProperty("Description", "Verifies that when the focused item is removed and no remaining item can take focus, focus moves to the element after the repeater.")]
        public void VerifyRemovingFocusedItemWithAllOthersDisabledMovesFocusOutOfRepeater()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                for (int i = 0; i < 5; i++)
                {
                    if (i != 2)
                    {
                        ((Control)fixture.Repeater.TryGetElement(i)).IsEnabled = false;
                    }
                }
                data.RemoveAt(2);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOutsideRepeater(fixture);
                Verify.AreSame(fixture.After, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot));
            });
        }

        // Scenario: 5 items (all visible, so no element is reused by the next layout pass); a GettingFocus handler on the
        //           repeater cancels every focus move into the repeater; the focused item is removed.
        // Expected: each live item is tried once, then focus moves to the element after the repeater.
        // A failure means: a cancelled move is taken as done, so focus is left on the cleared element; or the repeater
        //                  tries an item more than once.
        [TestMethod]
        [TestProperty("Description", "Verifies that when a GettingFocus handler cancels every focus move into the items, each item is tried once and focus moves to the element after the repeater.")]
        public void VerifyCancelledFocusMovesIntoItemsMoveFocusOutOfRepeater()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            var attempts = new Dictionary<DependencyObject, int>();
            RunOnUIThread.Execute(() =>
            {
                fixture.Repeater.GettingFocus += (s, e) =>
                {
                    var target = e.NewFocusedElement;
                    attempts[target] = attempts.TryGetValue(target, out int count) ? count + 1 : 1;
                    Log.Comment("Cancelling GettingFocus: " + DescribeElement(fixture.Repeater, target));
                    e.Cancel = true;
                };
                data.RemoveAt(3);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOutsideRepeater(fixture);
                Verify.AreSame(fixture.After, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot));
                Verify.AreEqual(4, attempts.Count, "Every live item was tried.");
                foreach (var pair in attempts)
                {
                    Verify.AreEqual(1, pair.Value, "Each item is tried once.");
                }
            });
        }

        // Scenario: a GettingFocus handler redirects the repeater's focus move to item 5; the focused item 3 is removed.
        // Expected: focus is on item 5, and item 5 (the element that actually got focus) is the one the repeater tracks: it
        //           stays realized and focused when it is scrolled out of view.
        // A failure means: the repeater tracks or pins the element it tried instead of the one that got focus.
        [TestMethod]
        [TestProperty("Description", "Verifies that when a GettingFocus handler redirects the repeater's focus move, the repeater tracks and pins the element that actually got focus.")]
        public void VerifyRedirectedFocusMoveIsTrackedByRepeater()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 50).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            UIElement redirectTarget = null;
            RunOnUIThread.Execute(() =>
            {
                bool redirected = false;
                fixture.Repeater.GettingFocus += (s, e) =>
                {
                    if (!redirected)
                    {
                        redirected = true;
                        redirectTarget = fixture.Repeater.TryGetElement(5);
                        Log.Comment("Redirecting GettingFocus from " + DescribeElement(fixture.Repeater, e.NewFocusedElement) + " to " + DescribeElement(fixture.Repeater, redirectTarget));
                        Verify.IsTrue(e.TrySetNewFocusedElement(redirectTarget));
                    }
                };
                data.RemoveAt(3);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(redirectTarget, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus is where the handler redirected it.");
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 5);
            });

            ScrollToOffset(fixture.ScrollViewer, 900);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(5, fixture.Repeater.GetElementIndex(redirectTarget), "The focused element is pinned and keeps its index out of view.");
                Verify.AreSame(redirectTarget, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "The focused element keeps focus out of view.");
            });
        }

        // Scenario: Reset (Clear + Add) with focus on item 3; the recycle pool gives the cleared, still-focused container
        //           back to the layout for some index before the focus move runs.
        // Expected: after layout focus is on the live item at index 3; if the old container was reused for another index,
        //           it is not kept realized (pinned) when that index is scrolled out of view.
        // A failure means: focus stays on the reused container at the wrong index, or the reused container stays pinned.
        [TestMethod]
        [TestProperty("Description", "Verifies that when the recycle pool reuses the cleared focused container during the next layout, focus ends on the live item at the old index and the reused container is not left pinned.")]
        public void VerifyResetWithReusedFocusedContainerMovesFocusToItemAtOldIndex()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 50).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);
            VerifyResetWithReusedFocusedContainer(fixture, data);
        }

        // Scenario: as above, with an app IElementFactory that returns the cleared, still-focused container for the first item
        //           that the layout realizes after the change (not item 3).
        // Expected: focus moves to the live item at index 3 (the nearest to the cleared index), and the reused container is
        //           not left pinned at its new index.
        // A failure means: focus stays on the reused container, or it stays pinned out of view.
        [TestMethod]
        [TestProperty("Description", "Verifies that when an app element factory reuses the cleared focused container for another index, focus moves to the live item at the old index and the reused container is not left pinned.")]
        public void VerifyFactoryReusingFocusedContainerForAnotherIndexMovesFocusToItemAtOldIndex()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 50).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            UIElement focusedContainer = null;
            bool reuseFocusedContainer = false;
            var pool = new List<UIElement>();
            RunOnUIThread.Execute(() =>
            {
                fixture.Repeater.ItemTemplate = new MockElementFactory
                {
                    GetElementFunc = (index, owner) =>
                    {
                        UIElement element;
                        if (reuseFocusedContainer && pool.Contains(focusedContainer))
                        {
                            reuseFocusedContainer = false;
                            element = focusedContainer;
                            Log.Comment($"The factory returns the cleared focused container for index {index}");
                        }
                        else if (pool.Count > 0)
                        {
                            element = pool[pool.Count - 1];
                        }
                        else
                        {
                            element = new Button { Height = 30 };
                        }
                        pool.Remove(element);
                        ((Button)element).Content = data[index];
                        return element;
                    },
                    ClearElementFunc = (element, owner) => pool.Add(element)
                };
            });
            IdleSynchronizer.Wait();
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                focusedContainer = fixture.Repeater.TryGetElement(3);
                reuseFocusedContainer = true;
                data.Clear();
                for (int i = 0; i < 50; i++)
                {
                    data.Add("New #" + i);
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                int reusedIndex = fixture.Repeater.GetElementIndex(focusedContainer);
                Log.Comment($"The cleared focused container now has index {reusedIndex}");
                Verify.IsTrue(reusedIndex >= 0 && reusedIndex != 3, "Precondition: the cleared focused container was reused for another index.");
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 3);
            });

            ScrollToOffset(fixture.ScrollViewer, 900);
            RunOnUIThread.Execute(() =>
            {
                int index = fixture.Repeater.GetElementIndex(focusedContainer);
                Log.Comment($"Reused container index after scrolling: {index}");
                Verify.IsTrue(index < 0 || index >= 25, "The reused container is not kept realized out of view.");
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 3);
            });
        }

        // Scenario: two repeaters share one RecyclingElementFactory (one RecyclePool); in the same tick repeater A is cleared
        //           with focus on its item 3, and repeater B gets items, so B can take A's cleared elements.
        // Expected: no exception; focus is not on an element of A (A has no items); if focus is in B, it is on a live item.
        // A failure means: A moves focus into an element that now belongs to B, or focus stays on a stale element.
        [TestMethod]
        [TestProperty("Description", "Verifies that clearing a repeater with focus while another repeater that shares its RecyclePool takes the cleared elements does not throw and leaves focus on no stale element.")]
        public void VerifyClearWithSharedRecyclePoolDoesNotLeaveFocusOnStaleElement()
        {
            var dataA = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "A #" + i));
            var dataB = new ObservableCollection<string>();
            ItemsRepeater repeaterA = null;
            ItemsRepeater repeaterB = null;
            RunOnUIThread.Execute(() =>
            {
                var factory = CreateElementFactory(ButtonItemTemplate);
                repeaterA = new ItemsRepeater { ItemsSource = dataA, ItemTemplate = factory, VerticalCacheLength = 0 };
                repeaterB = new ItemsRepeater { ItemsSource = dataB, ItemTemplate = factory, VerticalCacheLength = 0 };
                var root = new StackPanel();
                root.Children.Add(new Button { Content = "Before" });
                root.Children.Add(new ScrollViewer { Content = repeaterA, Height = 150, Width = 200 });
                root.Children.Add(new ScrollViewer { Content = repeaterB, Height = 150, Width = 200 });
                root.Children.Add(new Button { Content = "After" });
                Content = root;
            });
            IdleSynchronizer.Wait();
            FocusItemAndWait(repeaterA, 3, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                dataA.Clear();
                for (int i = 0; i < 10; i++)
                {
                    dataB.Add("B #" + i);
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var containerA = GetFocusedContainer(repeaterA, out var focused);
                Log.Comment("Focused (A): " + DescribeElement(repeaterA, focused));
                Log.Comment("Focused (B): " + DescribeElement(repeaterB, focused));
                Verify.IsNull(containerA, "Focus is not on an element of the empty repeater A.");
                if (GetFocusedContainer(repeaterB, out _) != null)
                {
                    VerifyFocusOnLiveItem(repeaterB, i => dataB[i]);
                }
            });
        }

        // Scenario: the ScrollViewer is scrolled to item 50; focus is on visible item 52; Reset with the same count.
        // Expected: focus moves to the live item 52; the scroll offset stays where it was (the target is already in view);
        //           no layout cycle.
        // A failure means: the focus move scrolls the list away (for example back to the top) or does not settle.
        [TestMethod]
        [TestProperty("Description", "Verifies that a Reset with focus on a visible item of a scrolled list moves focus to the live item at the same index without changing the scroll offset.")]
        public void VerifyResetInScrolledListKeepsScrollOffset()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 100).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            ScrollToOffset(fixture.ScrollViewer, 1500);
            FocusItemAndWait(fixture.Repeater, 52, FocusState.Keyboard);

            double offsetBefore = 0;
            RunOnUIThread.Execute(() =>
            {
                offsetBefore = fixture.ScrollViewer.VerticalOffset;
                data.Clear();
                for (int i = 0; i < 100; i++)
                {
                    data.Add("New #" + i);
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"VerticalOffset before {offsetBefore}, after {fixture.ScrollViewer.VerticalOffset}");
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 52);
                Verify.IsTrue(Math.Abs(fixture.ScrollViewer.VerticalOffset - offsetBefore) <= 30, "The scroll offset stays where it was.");
            });
        }

        // Scenario: auto-recycling custom layout; an ElementClearing handler calls UpdateLayout() (with another element
        //           dirty, so LayoutUpdated is raised) while a Reset is processed with focus on item 2.
        // Expected: no focus move while the change is processed (also not from that LayoutUpdated); after layout focus is on
        //           the live item at index 2.
        // A failure means: the focus move runs inside the collection change and moves focus through cleared elements.
        [TestMethod]
        [TestProperty("Description", "Verifies that a LayoutUpdated raised by an UpdateLayout call from an ElementClearing handler during a Reset does not move focus inside the change, and focus ends on the live item.")]
        public void VerifyUpdateLayoutFromElementClearingDuringResetDoesNotMoveFocusInsideChange()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            VirtualizingLayout layout = null;
            RunOnUIThread.Execute(() => layout = CreateAutoRecyclingLayout(() => 0, int.MaxValue));
            var fixture = CreateFocusFixture(data, layout);
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                bool updated = false;
                fixture.Repeater.ElementClearing += (s, e) =>
                {
                    if (!updated)
                    {
                        updated = true;
                        fixture.After.Width = 120;
                        fixture.Root.UpdateLayout();
                    }
                };
            });

            int gettingFocusDuringChange = CountGettingFocusDuring(fixture.Repeater, () =>
            {
                data.Clear();
                for (int i = 0; i < 5; i++)
                {
                    data.Add("New #" + i);
                }
            });
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved to a cleared element while the collection change is processed.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 2));
        }

        // Scenario: focus item 0, scroll it away (pinned), remove it; then Tab from the element before the repeater
        //           through the items.
        // Expected: every item that Tab reaches is a realized item that shows the current data; the removed item's element
        //           is never reached.
        // A failure means: the stale pinned element is still a tab stop.
        [TestMethod]
        [TestProperty("Description", "Verifies that after removing a focused, scrolled-out item, Tab navigation into the repeater reaches only live items.")]
        public void VerifyTabAfterRemovingPinnedFocusedItemReachesOnlyLiveItems()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 100).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data, itemTemplate: TallButtonItemTemplate);
            var element0 = FocusItemAndScrollAway(fixture);

            RunOnUIThread.Execute(() => data.RemoveAt(0));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.IsTrue(fixture.Before.Focus(FocusState.Keyboard)));
            IdleSynchronizer.Wait();

            for (int step = 0; step < 5; step++)
            {
                bool reachedAfter = false;
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(FocusManager.TryMoveFocus(FocusNavigationDirection.Next, new FindNextElementOptions { SearchRoot = fixture.Repeater.XamlRoot.Content }));
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var focused = FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot);
                    Log.Comment($"Tab {step}: " + DescribeElement(fixture.Repeater, focused as DependencyObject));
                    Verify.AreNotSame(element0, focused, "The removed item's element is not a tab stop.");
                    reachedAfter = focused == fixture.After;
                    if (!reachedAfter)
                    {
                        VerifyFocusOnLiveItem(fixture.Repeater, i => data[i]);
                    }
                });
                if (reachedAfter)
                {
                    break;
                }
            }
        }

        // Scenario: nested repeaters in a ScrollViewer; focus is on item 1 of the inner repeater of group 1; the inner source
        //           is reset and refilled. Then focus moves out and group 1 is scrolled out of view.
        // Expected: focus moves to the live inner item at index 1; after focus leaves, the outer element of group 1 is not
        //           kept realized out of view (the ancestor pins added for focus are released).
        // A failure means: focus ends on a cleared inner element, or the outer element stays pinned.
        [TestMethod]
        [TestProperty("Description", "Verifies that resetting an inner repeater's source with focus on its item moves focus to the live inner item and leaves no pin on the outer element.")]
        public void VerifyInnerRepeaterResetMovesFocusToLiveItemAndReleasesOuterPin()
        {
            var groups = new ObservableCollection<ObservableCollection<string>>(Enumerable.Range(0, 20).Select(g =>
                new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => $"Item #{g}.{i}"))));
            ItemsRepeater outer = null;
            ScrollViewer scrollViewer = null;
            Button after = null;
            Button focusTarget = null;
            var gotFocus = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                outer = new ItemsRepeater
                {
                    ItemsSource = groups,
                    VerticalCacheLength = 0,
                    ItemTemplate = SharedHelpers.GetDataTemplate(
                        @"<ItemsRepeater ItemsSource='{Binding}'>
                            <ItemsRepeater.ItemTemplate>
                                <DataTemplate><Button Content='{Binding}' Height='30'/></DataTemplate>
                            </ItemsRepeater.ItemTemplate>
                          </ItemsRepeater>")
                };
                scrollViewer = new ScrollViewer { Content = outer, Height = 200, Width = 200 };
                after = new Button { Content = "After" };
                var root = new StackPanel();
                root.Children.Add(scrollViewer);
                root.Children.Add(after);
                Content = root;
                Content.UpdateLayout();

                var inner = (ItemsRepeater)outer.TryGetElement(1);
                focusTarget = (Button)inner.TryGetElement(1);
                focusTarget.GotFocus += delegate { gotFocus.Set(); };
                Verify.IsTrue(focusTarget.Focus(FocusState.Keyboard));
            });
            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), "Waiting for the inner item to get focus.");
            IdleSynchronizer.Wait();

            ItemsRepeater innerOfGroup1 = null;
            RunOnUIThread.Execute(() =>
            {
                innerOfGroup1 = (ItemsRepeater)outer.TryGetElement(1);
                groups[1].Clear();
                for (int i = 0; i < 3; i++)
                {
                    groups[1].Add($"New #1.{i}");
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(outer.XamlRoot) as Button;
                Log.Comment($"Focused: {focused?.Content}");
                Verify.IsNotNull(focused, "A Button is focused.");
                Verify.AreEqual("New #1.1", focused.Content, "Focus is on the new inner item at the same index.");
                Verify.AreSame(innerOfGroup1, VisualTreeHelper.GetParent(focused), "The focused item belongs to the inner repeater of group 1.");
                Verify.AreEqual(1, innerOfGroup1.GetElementIndex(focused), "The inner item is realized.");
                Verify.IsTrue(after.Focus(FocusState.Programmatic));
            });
            IdleSynchronizer.Wait();

            ScrollToOffset(scrollViewer, 1000);
            RunOnUIThread.Execute(() =>
            {
                int index = outer.GetElementIndex(innerOfGroup1);
                Log.Comment($"Outer index of the group 1 element after scrolling: {index}");
                Verify.IsTrue(index != 1, "The outer element of group 1 is not kept realized (pinned) out of view.");
            });
        }

        // Scenario: NonVirtualizingLayout; in one tick the Layout is replaced (the focused element goes to the pinned pool)
        //           and a new ItemsSource is assigned; later another ItemsSource is assigned (elements come back from the
        //           recycle pool).
        // Expected: no crash; the old focused element is cleared; focus is not left on it (the reassignment removes it
        //           from Children, so the core moves focus, as in VerifyNonVirtualizingReassignmentDoesNotLeaveFocusOnClearedElement);
        //           the later reassignment shows its items.
        // A failure means: the pinned element survives Children().Clear() of the reassignment or breaks the recycle pool.
        [TestMethod]
        [TestProperty("Description", "Verifies that with a NonVirtualizingLayout, a layout change followed by an ItemsSource reassignment in the same tick clears the pinned focused element and does not leave focus on it.")]
        public void VerifyNonVirtualizingLayoutChangeAndReassignmentClearsPinnedFocusedElement()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var newData = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Filtered #" + i));
            var lastData = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Last #" + i));
            Layout layout = null;
            RunOnUIThread.Execute(() => layout = new NonVirtualStackLayout());
            var fixture = CreateFocusFixture(data, layout);
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            UIElement focusedElement = null;
            RunOnUIThread.Execute(() =>
            {
                focusedElement = fixture.Repeater.TryGetElement(2);
                fixture.Repeater.Layout = new NonVirtualStackLayout();
                fixture.Repeater.ItemsSource = newData;
                Verify.AreEqual(-1, fixture.Repeater.GetElementIndex(focusedElement), "The pinned focused element is cleared by the reassignment.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreNotSame(focusedElement, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus is not left on the cleared element.");
                VerifyFocusNotOnClearedElement(fixture.Repeater, i => newData[i]);
                Verify.AreEqual(newData[0], ((Button)fixture.Repeater.TryGetElement(0)).Content);
                fixture.Repeater.ItemsSource = lastData;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(lastData[0], ((Button)fixture.Repeater.TryGetElement(0)).Content);
                VerifyFocusNotOnClearedElement(fixture.Repeater, i => lastData[i]);
            });
        }

        // Scenario: NonVirtualizingLayout; in one tick the Layout is replaced (the focused element goes to the pinned pool)
        //           and the ItemTemplate is changed.
        // Expected: the old focused element is cleared exactly once (one ElementClearing); after layout focus is on the
        //           new-template element at the same index.
        // A failure means: the old-template element is kept, cleared twice, or keeps focus.
        [TestMethod]
        [TestProperty("Description", "Verifies that with a NonVirtualizingLayout, a layout change followed by an ItemTemplate change in the same tick clears the pinned focused element once and moves focus to the new-template element.")]
        public void VerifyNonVirtualizingLayoutChangeAndTemplateChangeClearsPinnedFocusedElementOnce()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            Layout layout = null;
            RunOnUIThread.Execute(() => layout = new NonVirtualStackLayout());
            var fixture = CreateFocusFixture(data, layout);
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            UIElement focusedElement = null;
            int clearing = 0;
            RunOnUIThread.Execute(() =>
            {
                focusedElement = fixture.Repeater.TryGetElement(2);
                fixture.Repeater.ElementClearing += (s, e) => { if (e.Element == focusedElement) { clearing++; } };
                fixture.Repeater.Layout = new NonVirtualStackLayout();
                fixture.Repeater.ItemTemplate = CreateElementFactory("<Button Content='{Binding}' Height='30' Tag='NewTemplate'/>");
                Verify.AreEqual(-1, fixture.Repeater.GetElementIndex(focusedElement), "The pinned focused element is cleared by the template change.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, clearing, "ElementClearing is raised once for the old focused element.");
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 2);
                Verify.AreEqual("NewTemplate", ((Button)FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot)).Tag);
            });
        }

        // Scenario: NonVirtualizingLayout without an ItemTemplate; the items are Buttons, so a cleared item is removed from
        //           Children. Focus is on item 0. In one tick the Layout is replaced (item 0 goes to the pinned pool and
        //           stays in Children, with item 2, which the layout-change walk skips after removing item 1) and a new
        //           ItemsSource is assigned.
        // Expected: no exception; only the new items are children and they are shown at their indices.
        // A failure means: the reassignment walks Children while it removes the pinned element from them (the walk runs
        //                  past the end), or an old item stays realized.
        [TestMethod]
        [TestProperty("Description", "Verifies that with a NonVirtualizingLayout and UIElement items, a layout change followed by an ItemsSource reassignment in the same tick clears the pinned focused item and shows only the new items.")]
        public void VerifyNonVirtualizingLayoutChangeAndReassignmentWithElementItemsShowsNewItems()
        {
            List<Button> oldItems = null;
            List<Button> newItems = null;
            ObservableCollection<Button> data = null;
            Layout layout = null;
            RunOnUIThread.Execute(() =>
            {
                oldItems = Enumerable.Range(0, 3).Select(i => new Button { Content = "Item #" + i, Height = 30 }).ToList();
                newItems = Enumerable.Range(0, 3).Select(i => new Button { Content = "New #" + i, Height = 30 }).ToList();
                data = new ObservableCollection<Button>(oldItems);
                layout = new NonVirtualStackLayout();
            });
            var fixture = CreateFocusFixture(data, layout, itemTemplate: null);
            FocusItemAndWait(fixture.Repeater, 0, FocusState.Keyboard);

            Exception exception = null;
            RunOnUIThread.Execute(() =>
            {
                fixture.Repeater.Layout = new NonVirtualStackLayout();
                Log.Comment($"Children after the layout change: {VisualTreeHelper.GetChildrenCount(fixture.Repeater)}; item 0 index {fixture.Repeater.GetElementIndex(oldItems[0])}");
                try
                {
                    fixture.Repeater.ItemsSource = new ObservableCollection<Button>(newItems);
                }
                catch (Exception ex)
                {
                    exception = ex;
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(exception, "Assigning ItemsSource does not throw: " + exception?.Message);
                Log.Comment("Focused: " + DescribeFocus(fixture.Repeater));
                Verify.AreEqual(newItems.Count, VisualTreeHelper.GetChildrenCount(fixture.Repeater), "Only the new items are children.");
                for (int i = 0; i < newItems.Count; i++)
                {
                    Verify.AreSame(newItems[i], fixture.Repeater.TryGetElement(i), $"Item {i} is the new item.");
                    Verify.IsNull(VisualTreeHelper.GetParent(oldItems[i]), $"Old item {i} is not in the tree.");
                }
            });
        }

        // Scenario: the focused item 3 is removed, and in the same tick (before the repeater's LayoutUpdated) the app moves
        //           focus to another live item (index 5).
        // Expected: focus stays on the item the app focused.
        // A failure means: the pending focus move of the removal overrides the app's focus change and moves focus to index 3.
        [TestMethod]
        [TestProperty("Description", "Verifies that when the app moves focus to another live item after removing the focused item and before layout, the repeater does not move focus again.")]
        public void VerifyFocusMovedToAnotherLiveItemAfterRemoveIsKept()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 20).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            Button target = null;
            RunOnUIThread.Execute(() =>
            {
                data.RemoveAt(3);
                target = (Button)fixture.Repeater.TryGetElement(5);
                Verify.AreEqual("Item #6", target.Content, "Precondition: index 5 shows item 6 after the removal.");
                Verify.IsTrue(target.Focus(FocusState.Keyboard));
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 5);
                Verify.AreSame(target, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus stays on the item the app focused.");
            });
        }

        // Scenario: as VerifyGettingFocusHandlerCanChangeItemsSourceDuringFocusMove: after a Reset the repeater moves focus to
        //           the new item 3, and a GettingFocus handler removes that item, which clears the element focus is moving to.
        // Expected: the repeater moves focus again after the next layout pass, to the live item at index 3; focus is not on
        //           the element the handler's change cleared, even if the layout reused it for another item.
        // A failure means: focus is left on the cleared element (or on whatever item the layout reused it for).
        [TestMethod]
        [TestProperty("Description", "Verifies that when a GettingFocus handler removes the item the repeater moves focus to, the repeater moves focus again to the live item at the cleared index.")]
        public void VerifyGettingFocusHandlerRemovingTargetMovesFocusToItemAtClearedIndex()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            DependencyObject handlerTarget = null;
            RunOnUIThread.Execute(() =>
            {
                bool armed = false;
                fixture.Repeater.GettingFocus += (s, e) =>
                {
                    if (armed && handlerTarget == null)
                    {
                        handlerTarget = e.NewFocusedElement;
                        Log.Comment("GettingFocus target: " + DescribeElement(fixture.Repeater, e.NewFocusedElement) + "; removing item 3 from the handler.");
                        data.RemoveAt(3);
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
                Verify.IsNotNull(handlerTarget, "Precondition: the handler ran.");
                Log.Comment("Element cleared by the handler's change now: " + DescribeElement(fixture.Repeater, handlerTarget));
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 3);
                Verify.AreNotSame(handlerTarget, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus is not on the element the handler's change cleared.");
            });
        }

        // Scenario: auto-recycling custom layout; a Reset is processed with focus on item 2; an ElementClearing handler calls
        //           UpdateLayout() (with another element dirty, so LayoutUpdated is raised) on the first clear AFTER the
        //           focused element's clear, so a focus move is pending while the change is still processed. The items have
        //           a fixed size: a cleared item whose desired size changes (its DataContext is cleared) invalidates the
        //           repeater's measure, and the UpdateLayout call would then run the repeater's layout inside the change,
        //           which throws (also without the fix).
        // Expected: no exception; no focus move while the change is processed; after layout focus is on the live item at
        //           index 2.
        // A failure means: the pending focus move runs inside the collection change and moves focus to an element that the
        //                  same change clears next, or the repeater's measure is invalidated inside the change.
        [TestMethod]
        [TestProperty("Description", "Verifies that a LayoutUpdated raised by an UpdateLayout call from an ElementClearing handler after the focused element is cleared during a Reset does not move focus inside the change.")]
        public void VerifyUpdateLayoutAfterFocusedElementClearedDuringResetDoesNotMoveFocusInsideChange()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            VirtualizingLayout layout = null;
            RunOnUIThread.Execute(() => layout = CreateAutoRecyclingLayout(() => 0, int.MaxValue));
            var fixture = CreateFocusFixture(data, layout, itemTemplate: "<Button Content='{Binding}' Width='150' Height='30'/>");
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            bool updated = false;
            RunOnUIThread.Execute(() =>
            {
                var focusedElement = fixture.Repeater.TryGetElement(2);
                bool focusedCleared = false;
                fixture.Repeater.ElementClearing += (s, e) =>
                {
                    if (e.Element == focusedElement)
                    {
                        focusedCleared = true;
                    }
                    else if (focusedCleared && !updated)
                    {
                        updated = true;
                        Log.Comment("UpdateLayout from the ElementClearing of index " + fixture.Repeater.GetElementIndex(e.Element));
                        fixture.After.Width = 120;
                        fixture.Root.UpdateLayout();
                    }
                };
            });

            int gettingFocusDuringChange = CountGettingFocusDuring(fixture.Repeater, () =>
            {
                data.Clear();
                for (int i = 0; i < 5; i++)
                {
                    data.Add("New #" + i);
                }
            });
            Verify.IsTrue(updated, "Precondition: UpdateLayout was called after the focused element was cleared.");
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved to a cleared element while the collection change is processed.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 2));
        }

        // Scenario: auto-recycling custom layout with fixed-size items and an item transition provider that animates only the
        //           removal of the focused item 2; a Reset hands the focused element to the transition and the items after it
        //           to the element factory. The ElementClearing handler of the first item cleared after the focused one calls
        //           UpdateLayout() (with another element dirty, so LayoutUpdated is raised).
        // Expected: no exception; no focus move while the change is processed; after layout focus is on the live item at
        //           index 2 and not on the fading element; completing the transition does not move focus again.
        // A failure means: handing the focused element to the transition invalidated the repeater's measure inside the change,
        //                  so the UpdateLayout call ran the repeater's layout inside the change, which throws; or the pending
        //                  focus move ran inside the change.
        [TestMethod]
        [TestProperty("Description", "Verifies that an UpdateLayout call from an ElementClearing handler after the focused element was handed to a remove transition during a Reset does not throw or move focus inside the change.")]
        public void VerifyUpdateLayoutAfterAnimatedFocusedElementClearedDuringResetDoesNotMoveFocusInsideChange()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateAnimatedFocusedRemoveFixture(data, out var state);

            RunOnUIThread.Execute(() =>
            {
                fixture.Repeater.ElementClearing += (s, e) =>
                {
                    if (state.FocusedAnimated && !state.Updated && e.Element != state.FocusedElement)
                    {
                        state.Updated = true;
                        Log.Comment("UpdateLayout from the ElementClearing of index " + fixture.Repeater.GetElementIndex(e.Element));
                        fixture.After.Width = 120;
                        fixture.Root.UpdateLayout();
                    }
                };
            });

            int gettingFocusDuringChange = CountGettingFocusDuring(fixture.Repeater, () =>
            {
                data.Clear();
                for (int i = 0; i < 5; i++)
                {
                    data.Add("New #" + i);
                }
            });
            VerifyAnimatedFocusedRemoveOutcome(fixture, state, gettingFocusDuringChange, i => data[i]);
        }

        // Scenario: as above, but the focused item 2 is removed (Remove) and an ElementIndexChanged handler of the first item
        //           whose index the Remove shifts calls UpdateLayout().
        // Expected: no exception; no focus move while the change is processed; after layout focus is on the live item at
        //           index 2 (the former item 3) and not on the fading element.
        // A failure means: handing the focused element to the transition invalidated the repeater's measure inside the change,
        //                  so the UpdateLayout call ran the repeater's layout inside the change, which throws.
        [TestMethod]
        [TestProperty("Description", "Verifies that an UpdateLayout call from an ElementIndexChanged handler after the focused element was handed to a remove transition during a Remove does not throw or move focus inside the change.")]
        public void VerifyUpdateLayoutFromElementIndexChangedAfterAnimatedFocusedElementRemovedDoesNotMoveFocusInsideChange()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateAnimatedFocusedRemoveFixture(data, out var state);

            RunOnUIThread.Execute(() =>
            {
                fixture.Repeater.ElementIndexChanged += (s, e) =>
                {
                    if (state.FocusedAnimated && !state.Updated)
                    {
                        state.Updated = true;
                        Log.Comment($"UpdateLayout from the ElementIndexChanged {e.OldIndex} -> {e.NewIndex}");
                        fixture.After.Width = 120;
                        fixture.Root.UpdateLayout();
                    }
                };
            });

            int gettingFocusDuringChange = CountGettingFocusDuring(fixture.Repeater, () => data.RemoveAt(2));
            VerifyAnimatedFocusedRemoveOutcome(fixture, state, gettingFocusDuringChange, i => data[i]);
        }

        private class AnimatedFocusedRemoveState
        {
            public UIElement FocusedElement;
            public bool FocusedAnimated;
            public bool Updated;
            public List<ItemCollectionTransitionProgress> PendingRemoves = new List<ItemCollectionTransitionProgress>();
        }

        // Fixed-size items in an auto-recycling layout, with focus on item 2 and a transition provider that animates only the
        // removal of the element focused now.
        private FocusFixture CreateAnimatedFocusedRemoveFixture(ObservableCollection<string> data, out AnimatedFocusedRemoveState state)
        {
            var localState = new AnimatedFocusedRemoveState();
            VirtualizingLayout layout = null;
            RunOnUIThread.Execute(() => layout = CreateAutoRecyclingLayout(() => 0, int.MaxValue));
            var fixture = CreateFocusFixture(data, layout, itemTemplate: "<Button Content='{Binding}' Width='150' Height='30'/>");
            RunOnUIThread.Execute(() =>
            {
                fixture.Repeater.ItemTransitionProvider = new ItemCollectionTransitionProviderDerived
                {
                    // Start() is the only way to read the transition's element. A started transition is not completed
                    // automatically, so the focused element's removal stays pending until the test completes it, whether
                    // or not system animations are enabled. A transition for which this returns false is not queued.
                    ShouldAnimateFunc = transition =>
                    {
                        if (transition.Operation == ItemCollectionTransitionOperation.Remove && localState.FocusedElement != null)
                        {
                            var progress = transition.Start();
                            if (progress.Element == localState.FocusedElement)
                            {
                                if (!localState.FocusedAnimated)
                                {
                                    localState.FocusedAnimated = true;
                                    localState.PendingRemoves.Add(progress);
                                }
                                return true;
                            }
                        }
                        return false;
                    },
                    StartTransitionsFunc = transitions =>
                    {
                        foreach (var transition in transitions)
                        {
                            if (transition.Operation != ItemCollectionTransitionOperation.Remove)
                            {
                                transition.Start().Complete();
                            }
                        }
                    }
                };
            });
            IdleSynchronizer.Wait();
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);
            RunOnUIThread.Execute(() => localState.FocusedElement = fixture.Repeater.TryGetElement(2));
            state = localState;
            return fixture;
        }

        private static void VerifyAnimatedFocusedRemoveOutcome(FocusFixture fixture, AnimatedFocusedRemoveState state, int gettingFocusDuringChange, Func<int, string> contentAt)
        {
            Verify.IsTrue(state.FocusedAnimated, "Precondition: the focused element was handed to a remove transition.");
            Verify.IsTrue(state.Updated, "Precondition: UpdateLayout was called after the focused element was handed to the transition.");
            Verify.AreEqual(0, gettingFocusDuringChange, "Focus is not moved to a cleared element while the collection change is processed.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"Remove transitions in progress: {state.PendingRemoves.Count}");
                Verify.AreNotSame(state.FocusedElement, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus is not on the fading element.");
                VerifyFocusOnLiveItem(fixture.Repeater, contentAt, expectedIndex: 2);
                foreach (var progress in state.PendingRemoves)
                {
                    progress.Complete();
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, contentAt, expectedIndex: 2));
        }

        // Scenario: the focused item 3 is removed and an item is appended, and the repeater leaves the tree in the same tick.
        //           When it is added back, the layout reuses the cleared element for the item that it realizes at index 6,
        //           and the app focuses that element from ElementPrepared.
        // Expected: focus stays on the element the app focused (index 6); the repeater raises no GettingFocus after it.
        // A failure means: a focus move recorded before the repeater left the tree runs after it is added back and moves
        //                  focus away from the element the app focused.
        [TestMethod]
        [TestProperty("Description", "Verifies that a focus move pending when the repeater leaves the tree does not run after it is added back and override focus set by the app.")]
        public void VerifyPendingFocusMoveDoesNotRunAfterUnloadAndReload()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);

            UIElement cleared = null;
            RunOnUIThread.Execute(() =>
            {
                cleared = fixture.Repeater.TryGetElement(3);
                data.RemoveAt(3);
                data.Add("Appended");
                Content = null;
            });
            IdleSynchronizer.Wait();

            bool appFocused = false;
            int gettingFocusAfterAppFocus = 0;
            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Focused while unloaded: " + DescribeFocus(fixture.Repeater));
                fixture.Repeater.ElementPrepared += (s, e) =>
                {
                    if (!appFocused && e.Element == cleared)
                    {
                        Log.Comment($"The cleared element is prepared for index {e.Index}; the app focuses it.");
                        Verify.IsTrue(((Control)e.Element).Focus(FocusState.Programmatic));
                        appFocused = true;
                    }
                };
                fixture.Repeater.GettingFocus += (s, e) =>
                {
                    if (appFocused)
                    {
                        gettingFocusAfterAppFocus++;
                        Log.Comment("GettingFocus after the app's focus: " + DescribeElement(fixture.Repeater, e.NewFocusedElement));
                    }
                };
                Content = fixture.Root;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(appFocused, "Precondition: the layout reused the cleared element and the app focused it.");
                int reusedIndex = fixture.Repeater.GetElementIndex(cleared);
                Log.Comment($"The cleared element now has index {reusedIndex}");
                Verify.AreNotEqual(3, reusedIndex, "Precondition: the cleared element is reused for another index.");
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: reusedIndex);
                Verify.AreSame(cleared, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus stays on the element the app focused.");
                Verify.AreEqual(0, gettingFocusAfterAppFocus, "The repeater does not move focus after the app's focus.");
            });
        }

        // Scenario: the repeater is the last element inside an open Flyout, after a Button ("Top"); another open Popup with
        //           a Button is opened before the flyout and one after it; focus on an item; Clear().
        // Expected: focus moves to "Top" inside the flyout (the search does not leave the popup that contains the repeater,
        //           so it does not reach the other popups) and the flyout stays open.
        // A failure means: focus moves to an element of another popup (or of the window), which light-dismisses the flyout.
        [TestMethod]
        [TestProperty("Description", "Verifies that clearing the ItemsSource of the last repeater inside an open Flyout moves focus to the element before it in the flyout, not to another open popup.")]
        public void VerifyClearOfLastRepeaterInFlyoutMovesFocusToPrecedingElementInFlyout()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            ItemsRepeater repeater = null;
            Button top = null;
            Button anchor = null;
            Flyout flyout = null;
            Popup popupBefore = null;
            Popup popupAfter = null;
            var opened = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                repeater = new ItemsRepeater { ItemsSource = data, ItemTemplate = CreateElementFactory(ButtonItemTemplate) };
                top = new Button { Content = "Top" };
                var panel = new StackPanel();
                panel.Children.Add(top);
                panel.Children.Add(repeater);
                flyout = new Flyout { Content = panel };
                flyout.Opened += delegate { opened.Set(); };
                anchor = new Button { Content = "Anchor" };
                var root = new StackPanel();
                root.Children.Add(anchor);
                root.Children.Add(new Button { Content = "WindowAfter" });
                Content = root;
            });
            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    popupBefore = new Popup { Child = new Button { Content = "PopupBefore" }, XamlRoot = anchor.XamlRoot, VerticalOffset = 300 };
                    popupBefore.IsOpen = true;
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() => flyout.ShowAt(anchor));
                Verify.IsTrue(opened.WaitOne(DefaultWaitTimeInMS), "Waiting for the flyout to open.");
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    popupAfter = new Popup { Child = new Button { Content = "PopupAfter" }, XamlRoot = anchor.XamlRoot, VerticalOffset = 350 };
                    popupAfter.IsOpen = true;
                });
                IdleSynchronizer.Wait();
                FocusItemAndWait(repeater, 1, FocusState.Keyboard);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(flyout.IsOpen, "Precondition: the flyout is open.");
                    var chain = new List<string>();
                    for (var current = VisualTreeHelper.GetParent(repeater); current != null; current = VisualTreeHelper.GetParent(current))
                    {
                        chain.Add(current.GetType().Name);
                    }
                    Log.Comment("Ancestors of the repeater: " + string.Join(" > ", chain));
                    data.Clear();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var focused = FocusManager.GetFocusedElement(repeater.XamlRoot) as DependencyObject;
                    Log.Comment("Focused: " + DescribeElement(repeater, focused));
                    Verify.IsTrue(flyout.IsOpen, "The flyout stays open.");
                    Verify.AreSame(top, focused, "Focus moves to the Button before the repeater inside the flyout.");
                });
            }
            finally
            {
                RunOnUIThread.Execute(() =>
                {
                    flyout.Hide();
                    if (popupBefore != null) { popupBefore.IsOpen = false; }
                    if (popupAfter != null) { popupAfter.IsOpen = false; }
                });
                IdleSynchronizer.Wait();
            }
        }

        // Scenario: ItemsSource of 8 Buttons (no ItemTemplate, so a cleared element is removed from Children and the core
        //           moves focus to the next tab stop, the "After" Button); keyboard focus on item 2; (1) RemoveAt(2);
        //           (2) the item at index 2 is replaced; (3) the last item is removed with focus on it.
        // Expected: after layout focus is on the item that took the cleared item's place: (1) "Item #3" at index 2,
        //           (2) the replacement at index 2, (3) the new last item.
        // A failure means: the repeater treats the core's focus move out of the repeater as a user move and leaves focus
        //           outside the list (or on another item).
        [TestMethod]
        [TestProperty("Description", "Verifies that removing or replacing the focused item of a UIElement ItemsSource moves focus to the item that takes its place, not out of the repeater.")]
        public void VerifyRemovingOrReplacingFocusedUIElementItemMovesFocusToItemAtSameIndex()
        {
            ObservableCollection<UIElement> data = null;
            RunOnUIThread.Execute(() =>
            {
                data = new ObservableCollection<UIElement>(Enumerable.Range(0, 8).Select(i => (UIElement)new Button { Content = "Item #" + i, Height = 30 }));
            });
            var fixture = CreateFocusFixture(data, itemTemplate: null);
            Func<int, string> contentAt = i => (string)((Button)data[i]).Content;

            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);
            RunOnUIThread.Execute(() =>
            {
                data.RemoveAt(2);
                Log.Comment("Focused right after RemoveAt: " + DescribeFocus(fixture.Repeater));
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOnLiveItem(fixture.Repeater, contentAt, expectedIndex: 2, expectedFocusState: FocusState.Keyboard);
                Verify.AreEqual("Item #3", contentAt(2));
            });

            RunOnUIThread.Execute(() =>
            {
                data[2] = new Button { Content = "Replacement", Height = 30 };
                Log.Comment("Focused right after Replace: " + DescribeFocus(fixture.Repeater));
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOnLiveItem(fixture.Repeater, contentAt, expectedIndex: 2, expectedFocusState: FocusState.Keyboard);
                Verify.AreEqual("Replacement", contentAt(2));
            });

            int last = -1;
            RunOnUIThread.Execute(() => last = data.Count - 1);
            FocusItemAndWait(fixture.Repeater, last, FocusState.Keyboard);
            RunOnUIThread.Execute(() => data.RemoveAt(last));
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, contentAt, expectedIndex: last - 1));
        }

        // Scenario: keyboard focus on item 3; a Flyout opened from the item has the focus (on its "Remove" Button) when
        //           the item is removed, and the flyout is closed in the same tick (as a Remove button's Click handler
        //           does). The popup returns focus to the element that had focus when it opened, which was cleared.
        //           Then the same with a MenuFlyout whose "Delete" item is invoked through UI Automation.
        // Expected: after layout focus is on the item that took the removed item's place, at index 3.
        // A failure means: focus is left on the cleared, recycled element (index -1), the bug's symptom through a context
        //           menu or flyout.
        [TestMethod]
        [TestProperty("Description", "Verifies that when a Flyout or MenuFlyout opened from the focused item removes the item and closes, focus moves to the item that takes its place.")]
        public void VerifyRemovingItemFromFlyoutOpenedFromItMovesFocusToItemAtSameIndex()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);

            // Flyout: the removal and Hide run in the same tick.
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);
            Flyout flyout = null;
            Button remove = null;
            var opened = new ManualResetEvent(false);
            var closed = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                remove = new Button { Content = "Remove" };
                flyout = new Flyout { Content = remove };
                flyout.Opened += delegate { opened.Set(); };
                flyout.Closed += delegate { closed.Set(); };
                flyout.ShowAt((FrameworkElement)fixture.Repeater.TryGetElement(3));
            });
            Verify.IsTrue(opened.WaitOne(DefaultWaitTimeInMS), "Waiting for the flyout to open.");
            IdleSynchronizer.Wait();
            FocusAndWait(remove, FocusState.Keyboard);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(GetFocusedContainer(fixture.Repeater, out _), "Precondition: focus is in the flyout.");
                data.RemoveAt(3);
                flyout.Hide();
                Log.Comment("Focused right after RemoveAt and Hide: " + DescribeFocus(fixture.Repeater));
            });
            Verify.IsTrue(closed.WaitOne(DefaultWaitTimeInMS), "Waiting for the flyout to close.");
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 3);
                Verify.AreEqual("Item #4", data[3]);
            });

            // MenuFlyout: the Delete item's Click handler removes the item; the menu closes itself.
            FocusItemAndWait(fixture.Repeater, 3, FocusState.Keyboard);
            MenuFlyout menu = null;
            MenuFlyoutItem delete = null;
            opened.Reset();
            closed.Reset();
            RunOnUIThread.Execute(() =>
            {
                delete = new MenuFlyoutItem { Text = "Delete" };
                delete.Click += delegate
                {
                    data.RemoveAt(3);
                    Log.Comment("Focused right after RemoveAt in Click: " + DescribeFocus(fixture.Repeater));
                };
                menu = new MenuFlyout();
                menu.Items.Add(delete);
                menu.Opened += delegate { opened.Set(); };
                menu.Closed += delegate { closed.Set(); };
                menu.ShowAt((FrameworkElement)fixture.Repeater.TryGetElement(3));
            });
            Verify.IsTrue(opened.WaitOne(DefaultWaitTimeInMS), "Waiting for the menu to open.");
            IdleSynchronizer.Wait();
            FocusAndWait(delete, FocusState.Keyboard);
            RunOnUIThread.Execute(() =>
            {
                var invoke = (Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider)FrameworkElementAutomationPeer.CreatePeerForElement(delete)
                    .GetPattern(PatternInterface.Invoke);
                invoke.Invoke();
            });
            Verify.IsTrue(closed.WaitOne(DefaultWaitTimeInMS), "Waiting for the menu to close.");
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 3);
                Verify.AreEqual("Item #5", data[3]);
            });
        }

        // Scenario: nested repeaters in a ScrollViewer (20 groups of 3); keyboard focus on item 1 of group 1; the view is
        //           scrolled so that group 1 is out of view (its outer element stays realized because it is pinned for
        //           focus); then the focused inner item is removed.
        // Expected: focus moves to a live inner item of group 1 (the inner repeater of a group out of view realizes only
        //           some of its items), whose outer element is still realized.
        // A failure means: the inner repeater released the outer pin when it cleared its focused element, the outer
        //           repeater cleared the group's element before the move, and focus left the list.
        [TestMethod]
        [TestProperty("Description", "Verifies that removing the focused inner item of a group scrolled out of view moves focus to a live inner item of that group.")]
        public void VerifyRemovingFocusedInnerItemOfScrolledOutGroupKeepsFocusInGroup()
        {
            var groups = CreateGroups(20, 3);
            var outer = CreateScrolledOutFocusedGroup(groups, out var innerOfGroup1);

            RunOnUIThread.Execute(() => groups[1].RemoveAt(1));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(outer.XamlRoot) as Button;
                Log.Comment($"Focused: {focused?.Content}; outer index of group 1: {outer.GetElementIndex(innerOfGroup1)}");
                Verify.IsNotNull(focused, "A Button is focused.");
                Verify.AreSame(innerOfGroup1, VisualTreeHelper.GetParent(focused), "The focused item belongs to group 1.");
                int item = innerOfGroup1.GetElementIndex(focused);
                Log.Comment($"Focused inner index: {item}");
                Verify.IsTrue(item >= 0, "The inner item is realized.");
                if (item >= 0)
                {
                    Verify.AreEqual(groups[1][item], focused.Content, "The focused item shows the current data.");
                }
                Verify.AreEqual(1, outer.GetElementIndex(innerOfGroup1), "The outer element of group 1 is realized.");
            });
        }

        // Scenario: as above, but the inner repeater of the scrolled-out group is emptied (Clear).
        // Expected: focus moves to a live inner item of a realized group, not out of the list.
        // A failure means: the outer repeater cleared the group's element before the inner repeater moved focus, so the
        //           move skipped the outer repeater and focus left the list although it has live groups.
        [TestMethod]
        [TestProperty("Description", "Verifies that emptying the inner repeater of a focused group scrolled out of view keeps focus in the outer repeater.")]
        public void VerifyClearingInnerRepeaterOfScrolledOutGroupKeepsFocusInOuterRepeater()
        {
            var groups = CreateGroups(20, 3);
            var outer = CreateScrolledOutFocusedGroup(groups, out var innerOfGroup1);

            RunOnUIThread.Execute(() => groups[1].Clear());
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(outer.XamlRoot) as Button;
                Log.Comment($"Focused: {focused?.Content}");
                Verify.IsNotNull(focused, "A Button is focused.");
                var inner = VisualTreeHelper.GetParent(focused) as ItemsRepeater;
                Verify.IsNotNull(inner, "The focused Button is an inner item.");
                if (inner != null)
                {
                    int group = outer.GetElementIndex(inner);
                    int item = inner.GetElementIndex(focused);
                    Log.Comment($"Focused group {group}, item {item}");
                    Verify.IsTrue(group >= 0 && item >= 0, "The focused inner item and its group are realized.");
                    if (group >= 0 && item >= 0)
                    {
                        Verify.AreEqual(groups[group][item], focused.Content, "The focused item shows the current data.");
                    }
                }
            });
        }

        // Scenario: 5 items (all realized); the repeater has an ItemTransitionProvider that animates the removal of the item
        //           focused last, and the test completes the transition. Keyboard focus on item 2; a Flyout opened from the
        //           item has the focus (on its "Remove" Button) when the item is removed. (1) The flyout closes in the same
        //           tick, before the transition completes. (2) The transition completes first (its completion clears the
        //           element a second time, with no index) and the flyout closes after it.
        // Expected: in both parts, after layout focus is on the item that took the removed item's place, at index 2.
        // A failure means: (2) the transition's completion overwrote the index remembered for the element, so focus stays
        //           on the recycled element when the flyout returns focus to it (the bug's symptom in an animated list).
        [TestMethod]
        [TestProperty("Description", "Verifies that when a Flyout opened from the focused item removes the item with a remove transition, focus moves to the item that takes its place whether the flyout closes before or after the transition completes.")]
        public void VerifyRemovingItemFromFlyoutWithRemoveTransitionMovesFocusToItemAtSameIndex()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 5).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            // Returns null for the index of a cleared element, so that a failed check does not throw on the UI thread.
            Func<int, string> contentAt = i => i >= 0 && i < data.Count ? data[i] : null;
            UIElement animated = null;
            var pendingRemoves = new List<ItemCollectionTransitionProgress>();
            RunOnUIThread.Execute(() =>
            {
                fixture.Repeater.ItemTransitionProvider = new ItemCollectionTransitionProviderDerived
                {
                    // A started transition is not completed automatically, so the removal stays pending until the test
                    // completes it. A transition for which this returns false is not queued.
                    ShouldAnimateFunc = transition =>
                    {
                        if (transition.Operation == ItemCollectionTransitionOperation.Remove && animated != null)
                        {
                            var progress = transition.Start();
                            if (progress.Element == animated)
                            {
                                // ShouldAnimate can be called more than once for the same transition; keep the first.
                                if (pendingRemoves.Count == 0)
                                {
                                    pendingRemoves.Add(progress);
                                }
                                return true;
                            }
                        }
                        return false;
                    },
                    StartTransitionsFunc = transitions =>
                    {
                        foreach (var transition in transitions)
                        {
                            if (transition.Operation != ItemCollectionTransitionOperation.Remove)
                            {
                                transition.Start().Complete();
                            }
                        }
                    }
                };
            });
            IdleSynchronizer.Wait();

            foreach (bool completeBeforeClose in new[] { false, true })
            {
                Log.Comment(completeBeforeClose ?
                    "Part 2: the remove transition completes before the flyout closes." :
                    "Part 1: the flyout closes before the remove transition completes.");
                FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);
                Flyout flyout = null;
                Button remove = null;
                var opened = new ManualResetEvent(false);
                var closed = new ManualResetEvent(false);
                RunOnUIThread.Execute(() =>
                {
                    animated = fixture.Repeater.TryGetElement(2);
                    pendingRemoves.Clear();
                    remove = new Button { Content = "Remove" };
                    flyout = new Flyout { Content = remove };
                    flyout.Opened += delegate { opened.Set(); };
                    flyout.Closed += delegate { closed.Set(); };
                    flyout.ShowAt((FrameworkElement)animated);
                });
                Verify.IsTrue(opened.WaitOne(DefaultWaitTimeInMS), "Waiting for the flyout to open.");
                IdleSynchronizer.Wait();
                FocusAndWait(remove, FocusState.Keyboard);

                string expectedContent = null;
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNull(GetFocusedContainer(fixture.Repeater, out _), "Precondition: focus is in the flyout.");
                    data.RemoveAt(2);
                    expectedContent = data[2];
                    Verify.AreEqual(1, pendingRemoves.Count, "Precondition: the removal of the item is animated.");
                    if (!completeBeforeClose)
                    {
                        flyout.Hide();
                    }
                });
                IdleSynchronizer.Wait();

                if (completeBeforeClose)
                {
                    RunOnUIThread.Execute(() =>
                    {
                        Verify.IsTrue(flyout.IsOpen, "Precondition: the flyout is still open.");
                        foreach (var progress in pendingRemoves)
                        {
                            progress.Complete();
                        }
                        pendingRemoves.Clear();
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        Log.Comment("Index of the removed item's element after the transition completed: " + fixture.Repeater.GetElementIndex(animated));
                        flyout.Hide();
                    });
                }
                Verify.IsTrue(closed.WaitOne(DefaultWaitTimeInMS), "Waiting for the flyout to close.");
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Log.Comment("Focused after the flyout closed: " + DescribeFocus(fixture.Repeater));
                    Verify.AreNotSame(animated, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus is not on the removed item's element.");
                    VerifyFocusOnLiveItem(fixture.Repeater, contentAt, expectedIndex: 2);
                    Verify.AreEqual(expectedContent, data[2]);
                    foreach (var progress in pendingRemoves)
                    {
                        progress.Complete();
                    }
                    pendingRemoves.Clear();
                    animated = null;
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() => VerifyFocusOnLiveItem(fixture.Repeater, contentAt, expectedIndex: 2));
            }
        }

        private static ObservableCollection<ObservableCollection<string>> CreateGroups(int groupCount, int itemCount)
        {
            return new ObservableCollection<ObservableCollection<string>>(Enumerable.Range(0, groupCount).Select(g =>
                new ObservableCollection<string>(Enumerable.Range(0, itemCount).Select(i => $"Item #{g}.{i}"))));
        }

        // Creates nested repeaters in a 200px ScrollViewer, focuses item 1 of group 1 and scrolls group 1 out of view.
        private ItemsRepeater CreateScrolledOutFocusedGroup(ObservableCollection<ObservableCollection<string>> groups, out ItemsRepeater innerOfGroup1)
        {
            ItemsRepeater outer = null;
            ScrollViewer scrollViewer = null;
            RunOnUIThread.Execute(() =>
            {
                outer = new ItemsRepeater
                {
                    ItemsSource = groups,
                    VerticalCacheLength = 0,
                    ItemTemplate = SharedHelpers.GetDataTemplate(
                        @"<ItemsRepeater ItemsSource='{Binding}'>
                            <ItemsRepeater.ItemTemplate>
                                <DataTemplate><Button Content='{Binding}' Height='30'/></DataTemplate>
                            </ItemsRepeater.ItemTemplate>
                          </ItemsRepeater>")
                };
                scrollViewer = new ScrollViewer { Content = outer, Height = 200, Width = 200 };
                var root = new StackPanel();
                root.Children.Add(new Button { Content = "Before" });
                root.Children.Add(scrollViewer);
                root.Children.Add(new Button { Content = "After" });
                Content = root;
            });
            IdleSynchronizer.Wait();

            FocusInnerItemAndWait(outer, 1, 1);
            ItemsRepeater inner = null;
            RunOnUIThread.Execute(() => inner = (ItemsRepeater)outer.TryGetElement(1));
            ScrollToOffset(scrollViewer, 1000);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, outer.GetElementIndex(inner), "Precondition: the outer element of group 1 is pinned out of view.");
                Verify.IsTrue(outer.TryGetElement(2) == null, "Precondition: group 2 is not realized.");
                var focused = FocusManager.GetFocusedElement(outer.XamlRoot) as Button;
                Verify.AreEqual("Item #1.1", focused?.Content, "Precondition: focus stays on the inner item.");
            });
            innerOfGroup1 = inner;
            return outer;
        }

        private void FocusAndWait(Control target, FocusState focusState)
        {
            var gotFocus = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                // A Flyout focuses its content when it opens after keyboard input, so the target can already have focus.
                if (FocusManager.GetFocusedElement(target.XamlRoot) == target)
                {
                    gotFocus.Set();
                    return;
                }

                target.GotFocus += delegate { gotFocus.Set(); };
                Verify.IsTrue(target.Focus(focusState));
            });
            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), "Waiting for the element to get focus.");
            IdleSynchronizer.Wait();
        }

        private void VerifyResetWithReusedFocusedContainer(FocusFixture fixture, ObservableCollection<string> data)
        {
            UIElement focusedContainer = null;
            RunOnUIThread.Execute(() =>
            {
                focusedContainer = fixture.Repeater.TryGetElement(3);
                data.Clear();
                for (int i = 0; i < 50; i++)
                {
                    data.Add("New #" + i);
                }
            });
            IdleSynchronizer.Wait();

            int reusedIndex = -1;
            RunOnUIThread.Execute(() =>
            {
                reusedIndex = fixture.Repeater.GetElementIndex(focusedContainer);
                Log.Comment($"The cleared focused container now has index {reusedIndex}");
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 3);
            });

            ScrollToOffset(fixture.ScrollViewer, 900);
            RunOnUIThread.Execute(() =>
            {
                int index = fixture.Repeater.GetElementIndex(focusedContainer);
                Log.Comment($"Old focused container index after scrolling: {index}");
                if (reusedIndex != 3)
                {
                    Verify.IsTrue(index < 0 || index >= 25, "The reused container is not kept realized out of view.");
                }
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 3);
            });
        }

        private void FocusDescendantAndWait(ItemsRepeater repeater, int index, string content)
        {
            var gotFocus = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                var target = FindDescendant<Button>(repeater.TryGetElement(index), b => (b.Content as string) == content);
                Verify.IsNotNull(target, $"Item {index} has a Button '{content}'.");
                target.GotFocus += delegate { gotFocus.Set(); };
                Verify.IsTrue(target.Focus(FocusState.Keyboard));
            });
            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), $"Waiting for '{content}' of item {index} to get focus.");
            IdleSynchronizer.Wait();
        }

        private static T FindDescendant<T>(DependencyObject root, Func<T, bool> predicate) where T : DependencyObject
        {
            if (root is T match && predicate(match))
            {
                return match;
            }

            int count = VisualTreeHelper.GetChildrenCount(root);
            for (int i = 0; i < count; i++)
            {
                var found = FindDescendant(VisualTreeHelper.GetChild(root, i), predicate);
                if (found != null)
                {
                    return found;
                }
            }
            return null;
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

        // Scenario: StackLayout, 8 items, keyboard focus on item 2. The item of the focused element is removed three times in
        //           the same tick, as an app's KeyDown handler does for Delete keys that are queued before the next layout
        //           pass (each key goes to the element that has focus when the key is delivered).
        // Expected: right after each removal focus is already on a live item, the one that took the removed index, so the
        //           removals remove items 2, 3 and 4; after layout focus is on "Item #5" at index 2 with the keyboard FocusState.
        // A failure means: focus stays on the cleared, recycled element (index -1, no DataContext) until the layout pass, so
        //           keys delivered before it reach an element that no longer shows an item.
        [TestMethod]
        [TestProperty("Description", "Verifies that removing the focused item moves focus to a live item before the next layout pass, so that input delivered before it does not reach the cleared element.")]
        public void VerifyRemovingFocusedItemMovesFocusToLiveItemBeforeLayout()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 8).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                for (int step = 0; step < 3; step++)
                {
                    var container = GetFocusedContainer(fixture.Repeater, out var focused);
                    Log.Comment($"Removal {step}: focused " + DescribeElement(fixture.Repeater, focused));
                    Verify.IsNotNull(container, $"Removal {step}: focus is inside the repeater.");
                    int index = fixture.Repeater.GetElementIndex(container);
                    Verify.IsTrue(index >= 0, $"Removal {step}: focus is on a live item, not on a cleared element.");
                    Verify.AreEqual(2, index, $"Removal {step}: focus is on the item that took the removed index.");
                    Verify.AreEqual(data[index], (focused as ContentControl)?.Content?.ToString(), $"Removal {step}: the focused item shows its data.");
                    data.RemoveAt(index);
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Item #0, Item #1, Item #5, Item #6, Item #7", string.Join(", ", data));
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 2, expectedFocusState: FocusState.Keyboard);
            });
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct KeyboardInputData { public ushort wVk; public ushort wScan; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
        [StructLayout(LayoutKind.Sequential)]
        private struct MouseInputData { public int dx; public int dy; public uint mouseData; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
        [StructLayout(LayoutKind.Explicit)]
        private struct InputUnion { [FieldOffset(0)] public MouseInputData mi; [FieldOffset(0)] public KeyboardInputData ki; }
        [StructLayout(LayoutKind.Sequential)]
        private struct InputData { public uint type; public InputUnion u; }
        [DllImport("user32.dll", SetLastError = true)]
        private static extern uint SendInput(uint count, InputData[] inputs, int size);

        // Scenario: StackLayout, 8 items, keyboard focus on item 2; the repeater's KeyDown handler removes the item of the
        //           element that got a Delete key. Three Delete presses are sent at once (one SendInput call), so the second
        //           and third keys are queued before the layout pass that follows the first removal. Run with a handler that
        //           returns at once and with one that takes 40 ms.
        // Expected: every key is delivered to a live item: items 2, 3 and 4 are removed, focus ends on "Item #5".
        // A failure means: a key is delivered to the cleared, recycled element (index -1, null DataContext), so it is lost,
        //           and an app handler that uses the element's DataContext runs with no item.
        [TestMethod]
        [TestProperty("Description", "Verifies that Delete keys queued right after the focused item is removed are delivered to live items, not to the cleared element.")]
        public void VerifyQueuedKeysAfterRemovingFocusedItemReachLiveItems()
        {
            foreach (int handlerDelayMs in new[] { 0, 40 })
            {
                Log.Comment($"Handler delay {handlerDelayMs} ms");
                var data = new ObservableCollection<string>(Enumerable.Range(0, 8).Select(i => "Item #" + i));
                var fixture = CreateFocusFixture(data);
                var keyLog = new List<string>();
                int keysOnClearedElement = 0;
                var allKeysHandled = new ManualResetEvent(false);
                RunOnUIThread.Execute(() =>
                {
                    fixture.Repeater.KeyDown += (s, e) =>
                    {
                        if (e.Key != global::Windows.System.VirtualKey.Delete)
                        {
                            return;
                        }

                        var current = e.OriginalSource as DependencyObject;
                        while (current != null && VisualTreeHelper.GetParent(current) != fixture.Repeater)
                        {
                            current = VisualTreeHelper.GetParent(current);
                        }

                        int index = current is UIElement element ? fixture.Repeater.GetElementIndex(element) : -2;
                        keyLog.Add($"Delete on element index {index}, content '{(current as ContentControl)?.Content}', DataContext '{(current as FrameworkElement)?.DataContext}'");
                        if (index >= 0)
                        {
                            data.RemoveAt(index);
                        }
                        else
                        {
                            keysOnClearedElement++;
                        }

                        if (handlerDelayMs > 0)
                        {
                            Thread.Sleep(handlerDelayMs);
                        }

                        e.Handled = true;
                        if (keyLog.Count == 3)
                        {
                            allKeysHandled.Set();
                        }
                    };
                });
                FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

                var inputs = new List<InputData>();
                for (int k = 0; k < 3; k++)
                {
                    inputs.Add(new InputData { type = 1 /* INPUT_KEYBOARD */, u = new InputUnion { ki = new KeyboardInputData { wVk = 0x2E /* VK_DELETE */ } } });
                    inputs.Add(new InputData { type = 1 /* INPUT_KEYBOARD */, u = new InputUnion { ki = new KeyboardInputData { wVk = 0x2E, dwFlags = 2 /* KEYEVENTF_KEYUP */ } } });
                }
                Verify.AreEqual((uint)inputs.Count, SendInput((uint)inputs.Count, inputs.ToArray(), Marshal.SizeOf<InputData>()), "Precondition: the keys are sent.");
                Verify.IsTrue(allKeysHandled.WaitOne(DefaultWaitTimeInMS), "Precondition: the three Delete keys reached the repeater.");
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    foreach (var line in keyLog)
                    {
                        Log.Comment(line);
                    }
                    Verify.AreEqual(0, keysOnClearedElement, "No key is delivered to a cleared element.");
                    Verify.AreEqual("Item #0, Item #1, Item #5, Item #6, Item #7", string.Join(", ", data));
                    VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 2);
                });
            }
        }

        // Scenario: StackLayout, 8 items, keyboard focus on item 2. The focused item is replaced (data[2] = x), and focus is
        //           read right after the change, before the next layout pass, as a handler for a queued key would see it.
        // Expected: right after the Replace focus is on a live item that shows its data; after layout focus is on the
        //           replacement at index 2 with the keyboard FocusState.
        // A failure means: focus stays on the cleared, recycled element (index -1, no DataContext) until the layout pass, so
        //           keys delivered before it reach an element that no longer shows an item.
        [TestMethod]
        [TestProperty("Description", "Verifies that replacing the focused item moves focus to a live item before the next layout pass, and to the replacement after it.")]
        public void VerifyReplacingFocusedItemMovesFocusToLiveItemBeforeLayout()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 8).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                data[2] = "Replaced #2";
                var container = GetFocusedContainer(fixture.Repeater, out var focused);
                Log.Comment("Right after the Replace: focused " + DescribeElement(fixture.Repeater, focused));
                Verify.IsNotNull(container, "Focus is inside the repeater.");
                int index = fixture.Repeater.GetElementIndex(container);
                Verify.IsTrue(index >= 0, "Right after the Replace focus is on a live item, not on a cleared element.");
                Verify.AreEqual(data[index], (focused as ContentControl)?.Content?.ToString(), "The focused item shows its data.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i], expectedIndex: 2, expectedFocusState: FocusState.Keyboard);
            });
        }

        // Scenario: StackLayout, 8 items, keyboard focus on item 2; the repeater's KeyDown handler replaces the item of the
        //           element that got a Delete key (data[i] = data[i] + "*"). Three Delete presses are sent at once (one
        //           SendInput call), so the second and third keys are queued before the layout pass that follows the first Replace.
        // Expected: every key is delivered to a live item; after layout focus is on a live item.
        // A failure means: a key is delivered to the cleared, recycled element (index -1, null DataContext).
        [TestMethod]
        [TestProperty("Description", "Verifies that Delete keys queued right after the focused item is replaced are delivered to live items, not to the cleared element.")]
        public void VerifyQueuedKeysAfterReplacingFocusedItemReachLiveItems()
        {
            var data = new ObservableCollection<string>(Enumerable.Range(0, 8).Select(i => "Item #" + i));
            var fixture = CreateFocusFixture(data);
            var keyLog = new List<string>();
            int keysOnClearedElement = 0;
            var allKeysHandled = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                fixture.Repeater.KeyDown += (s, e) =>
                {
                    if (e.Key != global::Windows.System.VirtualKey.Delete)
                    {
                        return;
                    }

                    var current = e.OriginalSource as DependencyObject;
                    while (current != null && VisualTreeHelper.GetParent(current) != fixture.Repeater)
                    {
                        current = VisualTreeHelper.GetParent(current);
                    }

                    int index = current is UIElement element ? fixture.Repeater.GetElementIndex(element) : -2;
                    keyLog.Add($"Delete on element index {index}, content '{(current as ContentControl)?.Content}', DataContext '{(current as FrameworkElement)?.DataContext}'");
                    if (index >= 0)
                    {
                        data[index] = data[index] + "*";
                    }
                    else
                    {
                        keysOnClearedElement++;
                    }

                    e.Handled = true;
                    if (keyLog.Count == 3)
                    {
                        allKeysHandled.Set();
                    }
                };
            });
            FocusItemAndWait(fixture.Repeater, 2, FocusState.Keyboard);

            var inputs = new List<InputData>();
            for (int k = 0; k < 3; k++)
            {
                inputs.Add(new InputData { type = 1 /* INPUT_KEYBOARD */, u = new InputUnion { ki = new KeyboardInputData { wVk = 0x2E /* VK_DELETE */ } } });
                inputs.Add(new InputData { type = 1 /* INPUT_KEYBOARD */, u = new InputUnion { ki = new KeyboardInputData { wVk = 0x2E, dwFlags = 2 /* KEYEVENTF_KEYUP */ } } });
            }
            Verify.AreEqual((uint)inputs.Count, SendInput((uint)inputs.Count, inputs.ToArray(), Marshal.SizeOf<InputData>()), "Precondition: the keys are sent.");
            Verify.IsTrue(allKeysHandled.WaitOne(DefaultWaitTimeInMS), "Precondition: the three Delete keys reached the repeater.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                foreach (var line in keyLog)
                {
                    Log.Comment(line);
                }
                Log.Comment("Data: " + string.Join(", ", data));
                Verify.AreEqual(0, keysOnClearedElement, "No key is delivered to a cleared element.");
                VerifyFocusOnLiveItem(fixture.Repeater, i => data[i]);
            });
        }

        // Scenario: no ItemTemplate, 8 UIElement items, keyboard focus on the last item. In the same handler the app removes
        //           the item and focuses the "After" Button. When the element leaves the tree the core already moves focus to
        //           the next tab stop, "After", so the app's Focus call does not change focus.
        // Expected: after layout focus is still on "After", where the app put it.
        // A failure means: the repeater takes focus back into the list after layout, overriding the app's focus move.
        [TestMethod]
        [TestProperty("Description", "Verifies that when the app focuses another element right after removing the focused UIElement item, the repeater does not take focus back after layout.")]
        public void VerifyAppFocusAfterRemovingFocusedUIElementItemIsKept()
        {
            ObservableCollection<UIElement> data = null;
            RunOnUIThread.Execute(() =>
            {
                data = new ObservableCollection<UIElement>(Enumerable.Range(0, 8).Select(i => (UIElement)new Button { Content = "Item #" + i, Height = 30 }));
            });
            var fixture = CreateFocusFixture(data, itemTemplate: null);
            FocusItemAndWait(fixture.Repeater, 7, FocusState.Keyboard);

            RunOnUIThread.Execute(() =>
            {
                data.RemoveAt(7);
                Log.Comment("Focused right after RemoveAt: " + DescribeFocus(fixture.Repeater));
                fixture.After.Focus(FocusState.Programmatic);
                Log.Comment("Focused after the app's Focus call: " + DescribeFocus(fixture.Repeater));
                Verify.AreSame(fixture.After, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Precondition: the app focused \"After\".");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Focused after layout: " + DescribeFocus(fixture.Repeater));
                Verify.AreSame(fixture.After, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus stays where the app put it.");
            });
        }

        // Scenario: no ItemTemplate; the items are 40 Buttons followed by 100 strings. Focus on Button 0, then scroll so that
        //           only string items are in view: Button 0 is kept (pinned) out of view, and the first string item realized
        //           makes the repeater set its default ItemTemplate during measure.
        // Expected: Button 0 stays the realized element of item 0 and keeps focus.
        // A failure means: setting the default template clears the pinned focused element and moves focus while the user
        //           scrolls.
        [TestMethod]
        [TestProperty("Description", "Verifies that the default ItemTemplate set during layout (first non-UIElement item realized) does not clear a pinned focused UIElement item.")]
        public void VerifyDefaultItemTemplateSetDuringScrollKeepsPinnedFocusedElement()
        {
            ObservableCollection<object> data = null;
            RunOnUIThread.Execute(() =>
            {
                data = new ObservableCollection<object>(
                    Enumerable.Range(0, 40).Select(i => (object)new Button { Content = "Button #" + i, Height = 30 })
                    .Concat(Enumerable.Range(0, 100).Select(i => (object)("Text #" + i))));
            });
            var fixture = CreateFocusFixture(data, itemTemplate: null);
            FocusItemAndWait(fixture.Repeater, 0, FocusState.Programmatic);
            Button element0 = null;
            RunOnUIThread.Execute(() =>
            {
                element0 = (Button)fixture.Repeater.TryGetElement(0);
                Verify.IsNull(fixture.Repeater.ItemTemplate, "Precondition: no ItemTemplate before a string item is realized.");
            });

            ScrollToOffset(fixture.ScrollViewer, 1500);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(fixture.Repeater.ItemTemplate, "Precondition: the repeater set its default ItemTemplate.");
                Log.Comment("Focused: " + DescribeFocus(fixture.Repeater));
                Verify.AreEqual(0, fixture.Repeater.GetElementIndex(element0), "The focused element is still the realized element of item 0.");
                Verify.AreSame(element0, FocusManager.GetFocusedElement(fixture.Repeater.XamlRoot), "Focus stays on the focused element.");
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

        // Counts the GettingFocus events raised while the change runs whose target is not a live item when the change returns:
        // a cleared element, or an element that the same change cleared after focus moved to it. A move to a live item when
        // the change is processed (the repeater moves focus off a cleared element before the next layout pass) is not counted.
        private static int CountGettingFocusDuring(ItemsRepeater repeater, Action change)
        {
            int count = 0;
            RunOnUIThread.Execute(() =>
            {
                bool inChange = true;
                var targets = new List<DependencyObject>();
                TypedEventHandler<UIElement, GettingFocusEventArgs> handler = (s, e) =>
                {
                    if (inChange)
                    {
                        targets.Add(e.NewFocusedElement);
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

                foreach (var target in targets)
                {
                    var current = target;
                    while (current != null && VisualTreeHelper.GetParent(current) != repeater)
                    {
                        current = VisualTreeHelper.GetParent(current);
                    }

                    if (current is UIElement container && repeater.GetElementIndex(container) < 0)
                    {
                        count++;
                        Log.Comment("Focus moved during the change to an element that is not a live item: " + DescribeElement(repeater, target));
                    }
                }
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
