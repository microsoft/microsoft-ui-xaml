// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Collections.Generic;
using System.Drawing;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using Point = System.Drawing.Point;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class ItemsViewTestsWithInputHelper
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("IsolationLevel", "Method")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Verifies that ItemContainer DragStarting event gets raised.")]
        [TestProperty("Ignore", "True")] // Bug 43507766 - Disabled because of an assertion failure in CEventManager::Raise
        public void ItemsViewItemsContainerDragStartingEventRaised()
        {
            using (var setup = new TestSetupHelper(new[] { "ItemsView Tests", "navigateToIntegration" }))
            {
                Log.Comment("Retrieving cmbItemTemplate");
                ComboBox cmbItemTemplate = new ComboBox(FindElement.ById("cmbItemTemplate"));
                Verify.IsNotNull(cmbItemTemplate, "Verifying that cmbItemTemplate was found");

                Log.Comment("Retrieving lstLogs");
                ListBox lstLogs = new ListBox(FindElement.ById("lstLogs"));
                Verify.IsNotNull(lstLogs, "Verifying that lstLogs was found");

                Log.Comment("Retrieving PART_ScrollPresenter");
                var scrollPresenter = FindElement.ById("PART_ScrollPresenter");
                Verify.IsNotNull(scrollPresenter, "Verifying that PART_ScrollPresenter was found");

                Log.Comment("Changing ItemTemplate selection to 'Small Image'");
                cmbItemTemplate.SelectItemByName("Small Image");
                Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);

                // Retrieve ItemContainer to drag
                Log.Comment("scrollPresenter.Children[0].Name=" + scrollPresenter.Children[0].Name);
                Verify.AreEqual("ItemContainer", scrollPresenter.Children[0].Name);

                Rectangle itemContainerBounds = scrollPresenter.Children[0].BoundingRectangle;
                Log.Comment("itemContainerBounds Bounds= X:{0}, Y:{1}, Width:{2}, Height:{3}",
                    itemContainerBounds.X, itemContainerBounds.Y, itemContainerBounds.Width, itemContainerBounds.Height);

                Point itemContainerCenter = new Point(
                    itemContainerBounds.X + itemContainerBounds.Width / 2,
                    itemContainerBounds.Y + itemContainerBounds.Height / 2);

                Point itemContainerBoundsOffsetCenter = new Point(
                    itemContainerCenter.X,
                    itemContainerCenter.Y + 400);

                Log.Comment("Dragging ItemContainer from its center down 400px");
                InputHelper.Pan(
                    obj: scrollPresenter.Children[0],
                    start: itemContainerCenter,
                    end: itemContainerBoundsOffsetCenter,
                    holdDuration: 1000,
                    panAcceleration: 0.0f,
                    dragDuration: 1000,
                    waitForIdle: false);
                Wait.ForIdle();

                bool foundDragStartingEvent = false;
                foreach (var item in lstLogs.AllItems)
                {
                    ListBoxItem lbItem = item as ListBoxItem;
                    string name = lbItem.Name;
                    foundDragStartingEvent = name == "ItemContainer.DragStarting";
                    if(foundDragStartingEvent) { break; }
                }

                Verify.IsTrue(foundDragStartingEvent, "Verifying that DragStarted was raised.");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Selects items via various interactions in Multiple Mode.")]
        public void ItemsViewMultipleSelectionMode()
        {
            using (var setup = new TestSetupHelper(new[] { "ItemsView Tests", "navigateToSummary" }))
            {
                SetUpItemsViewForSelectionTests("Multiple");

                Log.Comment("Retrieving PART_ScrollPresenter");
                var scrollPresenter = FindElement.ById("PART_ScrollPresenter");
                Verify.IsNotNull(scrollPresenter, "Verifying that PART_ScrollPresenter was found");

                // PRIMARY INPUT
                Log.Comment("scrollPresenter.Children[0].Name=" + scrollPresenter.Children[0].Name);
                Verify.AreEqual("ItemContainer", scrollPresenter.Children[0].Name);

                Log.Comment("Select two items via left click.");
                InputHelper.LeftClick(scrollPresenter.Children[9]);
                InputHelper.LeftClick(scrollPresenter.Children[10]);

                var strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (1): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "9 - Lorem, 10 - Lore, ");

                Log.Comment("Deselect item via left click.");
                InputHelper.LeftClick(scrollPresenter.Children[10]);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (2): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "9 - Lorem, ");

                Log.Comment("Select multiple items using shift + left click.");

                Log.Comment("Set an anchor item.");
                InputHelper.LeftClick(scrollPresenter.Children[9]);
                InputHelper.LeftClick(scrollPresenter.Children[9]);

                Log.Comment("Shift + left click a second item.");
                KeyboardHelper.PressDownModifierKey(ModifierKey.Shift, true, false);
                InputHelper.LeftClick(scrollPresenter.Children[14]);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Shift, true, false);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (3): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "9 - Lorem, 10 - Lore, 11 - Lore, 12 - Lore, 13 - Lore, 14 - Lore, ");

                // FOCUS INPUT

                Log.Comment("Deselect last selected item using LeftClick");
                InputHelper.LeftClick(scrollPresenter.Children[14]);

                Log.Comment("Deselect items using shift + arrow");
                KeyboardHelper.PressDownModifierKey(ModifierKey.Shift, true, false);
                KeyboardHelper.PressKey(Key.Up);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Shift, true, false);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (4): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "");

                Log.Comment("Select item to make it a selected anchor using LeftClick.");
                InputHelper.LeftClick(scrollPresenter.Children[9]);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (5): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "9 - Lorem, ");

                scrollPresenter.Children[10].SetFocus();

                Log.Comment("Select items using shift + arrow");
                KeyboardHelper.PressDownModifierKey(ModifierKey.Shift, true, false);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Shift, true, false);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (6): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "9 - Lorem, 10 - Lore, 11 - Lore, 12 - Lore, 13 - Lore, 14 - Lore, ");

                Log.Comment("Verify navigating using the arrow keys does not change selection.");

                scrollPresenter.Children[16].SetFocus();

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Right);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (7): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "9 - Lorem, 10 - Lore, 11 - Lore, 12 - Lore, 13 - Lore, 14 - Lore, ");

                ResetItemsView(cmbItemsSource: null, cmbLayout: null);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Selects items via various interactions in Extended Mode.")]
        public void ItemsViewExtendedSelectionMode()
        {
            using (var setup = new TestSetupHelper(new[] { "ItemsView Tests", "navigateToSummary" }))
            {
                SetUpItemsViewForSelectionTests("Extended");

                Log.Comment("Retrieving PART_ScrollPresenter");
                var scrollPresenter = FindElement.ById("PART_ScrollPresenter");
                Verify.IsNotNull(scrollPresenter, "Verifying that PART_ScrollPresenter was found");

                // PRIMARY INPUT
                Log.Comment("scrollPresenter.Children[0].Name=" + scrollPresenter.Children[0].Name);
                Verify.AreEqual("ItemContainer", scrollPresenter.Children[0].Name);

                Log.Comment("Select single item via LeftClick.");
                InputHelper.LeftClick(scrollPresenter.Children[8]);

                var strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (1): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "8 - Lorem, ");

                Log.Comment("Select a different single item via LeftClick.");
                InputHelper.LeftClick(scrollPresenter.Children[10]);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (2): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "10 - Lore, ");

                Log.Comment("Select two items via Ctrl + LeftClick.");
                KeyboardHelper.PressDownModifierKey(ModifierKey.Control, true, false);
                InputHelper.LeftClick(scrollPresenter.Children[12]);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control, true, false);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (3): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "10 - Lore, 12 - Lore, ");

                Log.Comment("Verify Ctrl + LeftClick on a selected item deselects it.");
                KeyboardHelper.PressDownModifierKey(ModifierKey.Control, true, false);
                InputHelper.LeftClick(scrollPresenter.Children[10]);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control, true, false);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (4): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "12 - Lore, ");

                Log.Comment("Use Shift + LeftClick to select a range of items.");

                InputHelper.LeftClick(scrollPresenter.Children[13]);

                KeyboardHelper.PressDownModifierKey(ModifierKey.Shift, true, false);
                InputHelper.LeftClick(scrollPresenter.Children[18]);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Shift, true, false);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (5): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "13 - Lore, 14 - Lore, 15 - Lore, 16 - Lore, 17 - Lore, 18 - Lore, ");

                Log.Comment("Verify selecting a new range of items deselects the previous range.");

                KeyboardHelper.PressDownModifierKey(ModifierKey.Control, true, false);
                InputHelper.LeftClick(scrollPresenter.Children[2]);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control, true, false);

                KeyboardHelper.PressDownModifierKey(ModifierKey.Shift, true, false);
                InputHelper.LeftClick(scrollPresenter.Children[7]);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Shift, true, false);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (6): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "2 - Lorem, 3 - Lorem, 4 - Lorem, 5 - Lorem, 6 - Lorem, 7 - Lorem, ");

                // FOCUS INPUT

                Log.Comment("Verify focus changes selection when navigating using arrow keys.");

                InputHelper.LeftClick(scrollPresenter.Children[9]);
                KeyboardHelper.PressKey(Key.Down);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (7): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "14 - Lore, ");

                Log.Comment("Verify focus does not change selection when navigating using ctrl + arrow keys.");

                KeyboardHelper.PressDownModifierKey(ModifierKey.Control, true, false);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control, true, false);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (8): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "14 - Lore, ");

                Log.Comment("Verify Shift + Arrow Key selects a range.");

                scrollPresenter.Children[16].SetFocus();
                KeyboardHelper.PressDownModifierKey(ModifierKey.Shift, true, false);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Shift, true, false);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (9): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "14 - Lore, 15 - Lore, 16 - Lore, 17 - Lore, 18 - Lore, 19 - Lore, ");

                Log.Comment("Verify Shift + Ctrl + Arrow Key selects a range while preserving previously selected range.");

                KeyboardHelper.PressDownModifierKey(ModifierKey.Control, true, false);
                InputHelper.LeftClick(scrollPresenter.Children[2]);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control, true, false);

                KeyboardHelper.PressDownModifierKey(ModifierKey.Shift, true, false);
                KeyboardHelper.PressDownModifierKey(ModifierKey.Control, true, false);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Shift, true, false);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control, true, false);

                strSelectedItems = GetSelectedItems();
                Log.Comment($"SelectedItems (10): {strSelectedItems}");
                Verify.AreEqual(strSelectedItems, "2 - Lorem, 3 - Lorem, 4 - Lorem, 5 - Lorem, 6 - Lorem, 7 - Lorem, 14 - Lore, 15 - Lore, 16 - Lore, 17 - Lore, 18 - Lore, 19 - Lore, ");

                ResetItemsView(cmbItemsSource: null, cmbLayout: null);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Listens to the ItemsView.ItemInvoked event with ItemContainer and SelectionMode=None.")]
        [TestProperty("Ignore", "True")] // Bug 45295321
        public void VerifyItemsViewIsItemInvokedEnabledWithItemContainerAndNoSelection()
        {
            VerifyItemsViewItemInvokedEvent(useItemContainer: true, isItemInvokedEnabled: true, selectionMode: "None");
            VerifyItemsViewItemInvokedEvent(useItemContainer: true, isItemInvokedEnabled: false, selectionMode: "None");
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Listens to the ItemsView.ItemInvoked event with ItemContainer and SelectionMode=Single.")]
        public void VerifyItemsViewIsItemInvokedEnabledWithItemContainerAndSingleSelection()
        {
            VerifyItemsViewItemInvokedEvent(useItemContainer: true, isItemInvokedEnabled: true, selectionMode: "Single");
            VerifyItemsViewItemInvokedEvent(useItemContainer: true, isItemInvokedEnabled: false, selectionMode: "Single");
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Listens to the ItemsView.ItemInvoked event with ItemContainer and SelectionMode=Multiple.")]
        public void VerifyItemsViewIsItemInvokedEnabledWithItemContainerAndMultipleSelection()
        {
            VerifyItemsViewItemInvokedEvent(useItemContainer: true, isItemInvokedEnabled: true, selectionMode: "Multiple");
            VerifyItemsViewItemInvokedEvent(useItemContainer: true, isItemInvokedEnabled: false, selectionMode: "Multiple");
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Listens to the ItemsView.ItemInvoked event without ItemContainer and SelectionMode=None.")]
        [TestProperty("Ignore", "True")] // Turned off for Bug 45295321 & while ItemTemplate's root element must be an ItemContainer.
        public void VerifyItemsViewIsItemInvokedEnabledWithoutItemContainerAndNoSelection()
        {
            VerifyItemsViewItemInvokedEvent(useItemContainer: false, isItemInvokedEnabled: true, selectionMode: "None");
            VerifyItemsViewItemInvokedEvent(useItemContainer: false, isItemInvokedEnabled: false, selectionMode: "None");
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Listens to the ItemsView.ItemInvoked event without ItemContainer and SelectionMode=Single.")]
        [TestProperty("Ignore", "True")] // Turned off while ItemTemplate's root element must be an ItemContainer.
        public void VerifyItemsViewIsItemInvokedEnabledWithoutItemContainerAndSingleSelection()
        {
            VerifyItemsViewItemInvokedEvent(useItemContainer: false, isItemInvokedEnabled: true, selectionMode: "Single");
            VerifyItemsViewItemInvokedEvent(useItemContainer: false, isItemInvokedEnabled: false, selectionMode: "Single");
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Listens to the ItemsView.ItemInvoked event without ItemContainer and SelectionMode=Multiple.")]
        [TestProperty("Ignore", "True")] // Turned off while ItemTemplate's root element must be an ItemContainer.
        public void VerifyItemsViewIsItemInvokedEnabledWithoutItemContainerAndMultipleSelection()
        {
            VerifyItemsViewItemInvokedEvent(useItemContainer: false, isItemInvokedEnabled: true, selectionMode: "Multiple");
            VerifyItemsViewItemInvokedEvent(useItemContainer: false, isItemInvokedEnabled: false, selectionMode: "Multiple");
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Pans the ScrollView in an ItemsView.")]
        [TestProperty("Ignore", "True")] // Bug 41224237: ItemsViewTestsWithInputHelper.PanItemsView fails in the lab
        public void PanItemsView()
        {
            Log.Comment("Selecting ItemsView tests");

            using (var setup = new TestSetupHelper(new[] { "ItemsView Tests", "navigateToInteractiveTests" }))
            {
                Log.Comment("Retrieving cmbItemTemplate");
                ComboBox cmbItemTemplate = new ComboBox(FindElement.ById("cmbItemTemplate"));
                Verify.IsNotNull(cmbItemTemplate, "Verifying that cmbItemTemplate was found");

                Log.Comment("Retrieving cmbItemsSource");
                ComboBox cmbItemsSource = new ComboBox(FindElement.ById("cmbItemsSource"));
                Verify.IsNotNull(cmbItemsSource, "Verifying that cmbItemsSource was found");

                Log.Comment("Changing ItemTemplate selection to 'Recipe DataTemplate (Medium StackPanel)'");
                cmbItemTemplate.SelectItemByName("Recipe DataTemplate (Medium StackPanel)");
                Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);

                Log.Comment("Changing ItemsSource selection to 'List<Recipe>'");
                cmbItemsSource.SelectItemByName("List<Recipe>");
                Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);

                // Tapping button before attempting pan operation to guarantee effective touch input
                TapResetScrollViewButton();

                Log.Comment("Panning ScrollView in diagonal");
                PrepareForScrollViewManipulationStart();

                Log.Comment("Retrieving itemsView");
                UIObject itemsViewUIObject = FindElement.ByName("itemsView");
                Verify.IsNotNull(itemsViewUIObject, "Verifying that itemsView was found");

                Log.Comment("Retrieving PART_ScrollView");
                UIObject scrollViewUIObject = itemsViewUIObject.FirstChild;
                Verify.IsNotNull(scrollViewUIObject, "Verifying that PART_ScrollView was found");

                Log.Comment("Retrieving PART_ScrollPresenter as a UIObject");
                UIObject scrollPresenterUIObject = FindElement.ById("PART_ScrollPresenter");
                Verify.IsNotNull(scrollPresenterUIObject, "Verifying that PART_ScrollView.PART_Root.PART_ScrollPresenter was found");

                Log.Comment("Retrieving PART_ScrollPresenter as a ScrollPresenter");
                ScrollPresenter scrollPresenter = new ScrollPresenter(scrollPresenterUIObject);
                Verify.IsNotNull(scrollPresenter, "Verifying that scrollPresenter was found");

                InputHelper.Pan(
                    scrollPresenter,
                    new Point(scrollPresenter.BoundingRectangle.Left + 25, scrollPresenter.BoundingRectangle.Top + 25),
                    new Point(scrollPresenter.BoundingRectangle.Left - 25, scrollPresenter.BoundingRectangle.Top - 25));

                Log.Comment("Waiting for PART_ScrollView pan completion");
                WaitForScrollViewManipulationEnd("PART_ScrollView");

                Log.Comment("scrollPresenter.HorizontalScrollPercent={0}", scrollPresenter.HorizontalScrollPercent);
                Log.Comment("scrollPresenter.VerticalScrollPercent={0}", scrollPresenter.VerticalScrollPercent);
                Log.Comment("scrollPresenter.VerticalViewSize={0}", scrollPresenter.VerticalViewSize);
                Log.Comment("scrollPresenter.BoundingRectangle.Height={0}", scrollPresenter.BoundingRectangle.Height);

                if (scrollPresenter.HorizontalScrollPercent > 0.0 || scrollPresenter.VerticalScrollPercent <= 1.0)
                {
                    LogAndClearTraces();
                }

                Verify.AreEqual(-1.0, scrollPresenter.HorizontalScrollPercent, "Verifying scrollPresenter HorizontalScrollPercent is -1.");
                Verify.IsLessThan(1.0, scrollPresenter.VerticalScrollPercent, "Verifying scrollPresenter VerticalScrollPercent is greater than 1%");

                double horizontalOffset;
                double verticalOffset;
                double verticalExtent = 100.0 * scrollPresenter.BoundingRectangle.Height / scrollPresenter.VerticalViewSize;
                double minVerticalOffset = verticalExtent / 100.0;
                float zoomFactor;

                GetScrollViewView(out horizontalOffset, out verticalOffset, out zoomFactor);
                Log.Comment("horizontalOffset={0}", horizontalOffset);
                Log.Comment("verticalOffset={0}", verticalOffset);
                Log.Comment("zoomFactor={0}", zoomFactor);
                Verify.AreEqual(0.0, horizontalOffset, "Verifying horizontalOffset is 0.0");
                Verify.IsTrue(verticalOffset > minVerticalOffset, "Verifying verticalOffset is greater than " + minVerticalOffset);
                Verify.AreEqual(1.0f, zoomFactor, "Verifying zoomFactor is 1.0f");

                ResetItemsView(cmbItemsSource, cmbLayout: null);

                // Output-debug-string-level "None" is automatically restored when landing back on the ScrollView test page.
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Exercises the PageDown/PageUp, Home/End, Left/Right/Down/Up keys in an ItemsView with LinedFlowLayout.")]
        public void KeyboardNavigationInItemsViewWithLinedFlowLayout()
        {
            KeyboardNavigationWithLayout(layout: "LinedFlowLayout", useLateralMoves: true);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Exercises the PageDown/PageUp, Home/End, Left/Right/Down/Up keys in an ItemsView with StackLayout.")]
        public void KeyboardNavigationInItemsViewWithStackLayout()
        {
            KeyboardNavigationWithLayout(layout: "StackLayout", useLateralMoves: false);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Ignore", "True")] // Bug 42152134: UniformGridLayout: Application hangs when hitting End key in ItemsView
        [TestProperty("Description", "Exercises the PageDown/PageUp, Home/End, Left/Right/Down/Up keys in an ItemsView with UniformGridLayout.")]
        public void KeyboardNavigationInItemsViewWithUniformGridLayout()
        {
            KeyboardNavigationWithLayout(layout: "UniformGridLayout", useLateralMoves: true);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Exercises the Down key at a rapid pace in an ItemsView with LinedFlowLayout.")]
        public void KeyDownInItemsViewAndLinedFlowLayoutWithFastKeystrokes()
        {
            KeyboardNavigationWithFastKeystrokes(layout: "LinedFlowLayout", key: Key.Down);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Exercises the PageDown key at a rapid pace in an ItemsView with LinedFlowLayout.")]
        public void KeyPageDownInItemsViewAndLinedFlowLayoutWithFastKeystrokes()
        {
            KeyboardNavigationWithFastKeystrokes(layout: "LinedFlowLayout", key: Key.PageDown);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Exercises the Up key at a rapid pace in an ItemsView with LinedFlowLayout.")]
        public void KeyUpInItemsViewAndLinedFlowLayoutWithFastKeystrokes()
        {
            KeyboardNavigationWithFastKeystrokes(layout: "LinedFlowLayout", key: Key.Up);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Exercises the PageUp key at a rapid pace in an ItemsView with LinedFlowLayout.")]
        public void KeyPageUpInItemsViewAndLinedFlowLayoutWithFastKeystrokes()
        {
            KeyboardNavigationWithFastKeystrokes(layout: "LinedFlowLayout", key: Key.PageUp);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Exercises the Down key at a rapid pace in an ItemsView with StackLayout.")]
        public void KeyDownInItemsViewAndStackLayoutWithFastKeystrokes()
        {
            KeyboardNavigationWithFastKeystrokes(layout: "StackLayout", key: Key.Down);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Exercises the PageDown key at a rapid pace in an ItemsView with StackLayout.")]
        public void KeyPageDownInItemsViewAndStackLayoutWithFastKeystrokes()
        {
            KeyboardNavigationWithFastKeystrokes(layout: "StackLayout", key: Key.PageDown);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Exercises the Up key at a rapid pace in an ItemsView with StackLayout.")]
        public void KeyUpInItemsViewAndStackLayoutWithFastKeystrokes()
        {
            KeyboardNavigationWithFastKeystrokes(layout: "StackLayout", key: Key.Up);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Description", "Exercises the PageUp key at a rapid pace in an ItemsView with StackLayout.")]
        public void KeyPageUpInItemsViewAndStackLayoutWithFastKeystrokes()
        {
            KeyboardNavigationWithFastKeystrokes(layout: "StackLayout", key: Key.PageUp);
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Exercises the Tab and Shift-Tab keystrokes to navigate through the ItemsView control with ItemContainers and Single SelectionMode.")]
        public void TabNavigationThroughItemsViewWithItemContainersAndSingleSelectionMode()
        {
            TabNavigationThroughItemsView(useItemContainer: true, selectionMode: "Single");
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Exercises the Tab and Shift-Tab keystrokes to navigate through the ItemsView control with ItemContainers and Multiple SelectionMode.")]
        public void TabNavigationThroughItemsViewWithItemContainersAndMultipleSelectionMode()
        {
            TabNavigationThroughItemsView(useItemContainer: true, selectionMode: "Multiple");
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Exercises the Tab and Shift-Tab keystrokes to navigate through the ItemsView control without ItemContainers and Single SelectionMode.")]
        [TestProperty("Ignore", "True")] // Turned off while ItemTemplate's root element must be an ItemContainer.
        public void TabNavigationThroughItemsViewWithContentControlsAndSingleSelectionMode()
        {
            TabNavigationThroughItemsView(useItemContainer: false, selectionMode: "Single");
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Scrolls & zooms the ItemsView. Then tabs into it. Ensures first displayed items gets focus.")]
        public void TabIntoItemsViewWithoutCurrentItem()
        {
            TabIntoItemsViewWithoutCurrentItem(useShiftTab: false);
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Scrolls & zooms the ItemsView. Then shift-tabs into it. Ensures last displayed items gets focus.")]
        public void ShiftTabIntoItemsViewWithoutCurrentItem()
        {
            TabIntoItemsViewWithoutCurrentItem(useShiftTab: true);
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Pans the AnnotatedScrollBar vertically to jump to a disconnected offset in the ItemsView and trigger multiple more scrolls, using the LinedFlowLayout fast path.")]
        [TestProperty("Ignore", "True")] // Bug 43507766 - Disabled because of an assertion failure in CEventManager::Raise
        public void PanAnnotatedScrollBarWithItemsInfo()
        {
            PanAnnotatedScrollBar(handleItemsInfoRequested: true);
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Pans the AnnotatedScrollBar vertically to jump to a disconnected offset in the ItemsView and trigger multiple more scrolls, using the LinedFlowLayout regular path.")]
        [TestProperty("Ignore", "True")] // Bug 43507766 - Disabled because of an assertion failure in CEventManager::Raise
        public void PanAnnotatedScrollBarWithoutItemsInfo()
        {
            PanAnnotatedScrollBar(handleItemsInfoRequested: false);
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Exercises the ItemsView.StartBringItemIntoView method to bring items to the top of the viewport.")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion._21H2)] // Test hangs with Windows 10.
        public void StartBringItemIntoView()
        {
            StartBringItemIntoView(false /*clearItemAspectRatios*/);
            StartBringItemIntoView(true /*clearItemAspectRatios*/);
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Exercises the ItemsView.StartBringItemIntoView method asynchronously to bring the scrolled-off focused item into the viewport.")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion._21H2)] // Test hangs with Windows 10.
        public void BringFocusedItemIntoView()
        {
            Log.Comment("Selecting ItemsView summary page");

            ICollection<string> outputDebugStringComponentTypes = new[] { "ScrollView", "LinedFlowLayout", "ItemsView" };

            using (var setup = new ItemsViewTestSetupHelper(
                testNames: new[] { "ItemsView Tests", "navigateToSummary" },
                options: null,
                shouldRestrictInnerFrameSize: false,
                outputDebugStringLevel: "Info",
                outputDebugStringComponentTypes: outputDebugStringComponentTypes))
            {
                using (var loggingHelper = new LoggingHelper(this, outputDebugStringComponentTypes))
                {
                    Log.Comment("Retrieving cmbLayout");
                    UIObject cmbLayoutUIObject = FindElement.ById("cmbLayout");
                    Verify.IsNotNull(cmbLayoutUIObject, "Verifying that cmbLayout was found");
                    ComboBox layoutComboBox = new ComboBox(cmbLayoutUIObject);

                    Log.Comment("Changing cmbLayout selection to 'LinedFlowLayout'");
                    layoutComboBox.SelectItemByName("LinedFlowLayout");
                    Log.Comment("Selection is now {0}", layoutComboBox.Selection[0].Name);

                    Log.Comment("Retrieving cmbItemTemplate");
                    UIObject cmbItemTemplateUIObject = FindElement.ById("cmbItemTemplate");
                    Verify.IsNotNull(cmbItemTemplateUIObject, "Verifying that cmbItemTemplate was found");
                    ComboBox itemTemplateComboBox = new ComboBox(cmbItemTemplateUIObject);

                    Log.Comment("Changing cmbItemTemplate selection to 'Fixed-Height ItemContainer (Image + TextBlock)'");
                    itemTemplateComboBox.SelectItemByName("Fixed-Height ItemContainer (Image + TextBlock)");
                    Log.Comment("Selection is now {0}", itemTemplateComboBox.Selection[0].Name);

                    Log.Comment("Retrieving cmbItemsSource");
                    UIObject cmbItemsSourceUIObject = FindElement.ById("cmbItemsSource");
                    Verify.IsNotNull(cmbItemsSourceUIObject, "Verifying that cmbItemsSource was found");
                    ComboBox itemsSourceComboBox = new ComboBox(cmbItemsSourceUIObject);

                    Log.Comment("Changing cmbItemsSource selection to 'ObservableCollection<Recipe>'");
                    itemsSourceComboBox.SelectItemByName("ObservableCollection<Recipe>");
                    Log.Comment("Selection is now {0}", itemsSourceComboBox.Selection[0].Name);

                    Log.Comment("Retrieving & filling txtItemsViewMethodIndex");
                    UIObject txtItemsViewMethodIndexUIObject = FindElement.ById("txtItemsViewMethodIndex");
                    Verify.IsNotNull(txtItemsViewMethodIndexUIObject, "Verifying that itemsViewMethodIndexTextBox was found");
                    Edit itemsViewMethodIndexTextBox = new Edit(txtItemsViewMethodIndexUIObject);

                    itemsViewMethodIndexTextBox.SetValueAndWait("5");
                    Wait.ForIdle();

                    Log.Comment("Retrieving & filling txtBringIntoViewOptionsVerticalAlignmentRatio");
                    UIObject txtBringIntoViewOptionsVerticalAlignmentRatioUIObject = FindElement.ById("txtBringIntoViewOptionsVerticalAlignmentRatio");
                    Verify.IsNotNull(txtBringIntoViewOptionsVerticalAlignmentRatioUIObject, "Verifying that txtBringIntoViewOptionsVerticalAlignmentRatio was found");
                    Edit bringIntoViewOptionsVerticalAlignmentRatioTextBox = new Edit(txtBringIntoViewOptionsVerticalAlignmentRatioUIObject);

                    bringIntoViewOptionsVerticalAlignmentRatioTextBox.SetValueAndWait("0");
                    Wait.ForIdle();

                    Log.Comment("Retrieving btnItemsViewStartBringItemIntoView");
                    UIObject btnItemsViewStartBringItemIntoViewUIObject = FindElement.ById("btnItemsViewStartBringItemIntoView");
                    Verify.IsNotNull(btnItemsViewStartBringItemIntoViewUIObject, "Verifying that btnItemsViewStartBringItemIntoView was found");
                    Button itemsViewStartBringItemIntoViewButton = new Button(btnItemsViewStartBringItemIntoViewUIObject);

                    Log.Comment("Invoking btnItemsViewStartBringItemIntoView");
                    itemsViewStartBringItemIntoViewButton.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Retrieving btnGetVerticalOffset");
                    UIObject btnGetVerticalOffsetUIObject = FindElement.ById("btnGetVerticalOffset");
                    Verify.IsNotNull(btnGetVerticalOffsetUIObject, "Verifying that btnGetVerticalOffset was found");
                    Button getVerticalOffsetButton = new Button(btnGetVerticalOffsetUIObject);

                    Log.Comment("Invoking btnGetVerticalOffset");
                    getVerticalOffsetButton.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Retrieving txtVerticalOffset");
                    UIObject txtVerticalOffsetUIObject = FindElement.ById("txtVerticalOffset");
                    Verify.IsNotNull(txtVerticalOffsetUIObject, "Verifying that txtVerticalOffset was found");
                    Edit verticalOffsetTextBox = new Edit(txtVerticalOffsetUIObject);

                    Log.Comment($"ScrollView.VerticalOffset={verticalOffsetTextBox.Value}");
                    Verify.AreEqual("96", verticalOffsetTextBox.Value, "Verifying that one line was scrolled off");

                    Log.Comment("Retrieving btnItemsViewStartBringItemIntoViewAsync");
                    Button btnItemsViewStartBringItemIntoViewAsync = new Button(FindElement.ById("btnItemsViewStartBringItemIntoViewAsync"));
                    Verify.IsNotNull(btnItemsViewStartBringItemIntoViewAsync, "Verifying that btnItemsViewStartBringItemIntoViewAsync was found");

                    Log.Comment("Invoking btnItemsViewStartBringItemIntoViewAsync");
                    btnItemsViewStartBringItemIntoViewAsync.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Retrieving markupItemsView");
                    UIObject markupItemsViewUIObject = FindElement.ById("markupItemsView");
                    Verify.IsNotNull(markupItemsViewUIObject, "Verifying that markupItemsView was found");

                    Log.Comment("Clicking on ItemsView to focus first item.");
                    markupItemsViewUIObject.Click(PointerButtons.Primary, 20.0, 20.0);
                    Wait.ForIdle();

                    for (int mouseWheelIncrement = 0; mouseWheelIncrement < 4; mouseWheelIncrement++)
                    {
                        Log.Comment("Scrolling ItemsView with mouse wheel so focused first item gets put into the pinned pool");
                        InputHelper.RotateWheel(markupItemsViewUIObject, -2400);
                        Wait.ForIdle();
                    }

                    Log.Comment("Waiting for asynchronous StartBringItemIntoView gets launched and completes.");

                    Log.Comment("Invoking btnGetVerticalOffset until VerticalOffset is back to 96.");
                    for (int waitCount = 0; waitCount < 100; waitCount++)
                    {
                        Wait.ForMilliseconds(125);
                        getVerticalOffsetButton.Invoke();
                        Wait.ForIdle();
                        if (verticalOffsetTextBox.Value == "96")
                        {
                            Log.Comment($"VerticalOffset back to 96 when waitCount={waitCount}");
                            break;
                        }
                    }

                    Verify.AreEqual("96", verticalOffsetTextBox.Value, "Verifying that one line is scrolled off again");

                    Log.Comment("Retrieving btnClearLogs");
                    Button btnClearLogs = new Button(FindElement.ById("btnClearLogs"));
                    Verify.IsNotNull(btnClearLogs, "Verifying that btnClearLogs was found");

                    Log.Comment("Invoke btnClearLogs.");
                    btnClearLogs.Invoke();
                    Wait.ForIdle();

                    LogAndClearTraces();
                    Wait.ForIdle();

                    ResetItemsView(itemsSourceComboBox, layoutComboBox);
                }
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Ensures clicking ItemContainer in ItemsView brings it into view.")]
        public void ClickedItemBroughtIntoView()
        {
            Log.Comment("Selecting ItemsView tests");

            using (var setup = new TestSetupHelper(new[] { "ItemsView Tests", "navigateToInteractiveTests" }))
            {
                Log.Comment("Retrieving cmbItemTemplate");
                ComboBox cmbItemTemplate = new ComboBox(FindElement.ById("cmbItemTemplate"));
                Verify.IsNotNull(cmbItemTemplate, "Verifying that cmbItemTemplate was found");

                Log.Comment("Retrieving cmbItemsSource");
                ComboBox cmbItemsSource = new ComboBox(FindElement.ById("cmbItemsSource"));
                Verify.IsNotNull(cmbItemsSource, "Verifying that cmbItemsSource was found");

                Log.Comment("Changing ItemTemplate selection to 'Recipe DataTemplate (Medium ItemContainer)'");
                cmbItemTemplate.SelectItemByName("Recipe DataTemplate (Medium ItemContainer)");
                Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);

                Log.Comment("Changing ItemsSource selection to 'List<Recipe>'");
                cmbItemsSource.SelectItemByName("List<Recipe>");
                Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);

                Log.Comment("Retrieving itemsView");
                UIObject itemsViewUIObject = FindElement.ByName("itemsView");
                Verify.IsNotNull(itemsViewUIObject, "Verifying that itemsView was found");

                int currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(-1, currentItemIndex, "Verifying CurrentItemIndex is initially -1");

                Log.Comment("Clicking on ItemsView to select last partially visible item.");
                itemsViewUIObject.Click(PointerButtons.Primary, 40.0, itemsViewUIObject.BoundingRectangle.Height - 4.0);
                Wait.ForIdle();

                LogScrollViewInfo(itemsViewUIObject, expectedVerticalOffset: 76);
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(5, currentItemIndex, "Verifying CurrentItemIndex is 5");

                Log.Comment("Clicking on ItemsView to select first partially visible item.");
                itemsViewUIObject.Click(PointerButtons.Primary, 40.0, 4.0);
                Wait.ForIdle();

                LogScrollViewInfo(itemsViewUIObject, expectedVerticalOffset: 0);
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(0, currentItemIndex, "Verifying CurrentItemIndex is 0");

                ResetItemsView(cmbItemsSource, cmbLayout: null);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Starts an item drag operation after a pan.")]
        [TestProperty("Ignore", "True")] // Bug 44960897 - Inability to initiate item drag in ItemsView, on Win10 OS
        public void CanDragItemContainerAfterPan()
        {
            using (var setup = new TestSetupHelper(new[] { "ItemsView Tests", "navigateToSummary" }))
            {
                Log.Comment("Turning on item drag option.");

                Log.Comment("Retrieving chkSetItemsRepeaterElementCanDrag");
                CheckBox chkSetItemsRepeaterElementCanDrag = new CheckBox(FindElement.ById("chkSetItemsRepeaterElementCanDrag"));
                Verify.IsNotNull(chkSetItemsRepeaterElementCanDrag, "Verifying that chkSetItemsRepeaterElementCanDrag was found");

                if (ToggleState.Off == chkSetItemsRepeaterElementCanDrag.ToggleState)
                {
                    Log.Comment("Toggling chkSetItemsRepeaterElementCanDrag.IsChecked to ToggleState.On");
                    chkSetItemsRepeaterElementCanDrag.Toggle();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, chkSetItemsRepeaterElementCanDrag.ToggleState);
                }

                Log.Comment("Retrieving chkItemsRepeaterElementCanDrag");
                CheckBox chkItemsRepeaterElementCanDrag = new CheckBox(FindElement.ById("chkItemsRepeaterElementCanDrag"));
                Verify.IsNotNull(chkItemsRepeaterElementCanDrag, "Verifying that chkItemsRepeaterElementCanDrag was found");

                if (ToggleState.Off == chkItemsRepeaterElementCanDrag.ToggleState)
                {
                    Log.Comment("Toggling chkItemsRepeaterElementCanDrag.IsChecked to ToggleState.On");
                    chkItemsRepeaterElementCanDrag.Toggle();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, chkItemsRepeaterElementCanDrag.ToggleState);
                }

                Log.Comment("Setting up ItemsView.");

                Log.Comment("Retrieving cmbItemTemplate");
                ComboBox cmbItemTemplate = new ComboBox(FindElement.ById("cmbItemTemplate"));
                Verify.IsNotNull(cmbItemTemplate, "Verifying that cmbItemTemplate was found");

                Log.Comment("Changing cmbItemTemplate selection to 'ItemContainer (Image)'");
                cmbItemTemplate.SelectItemByName("ItemContainer (Image)");
                Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);

                Log.Comment("Retrieving cmbItemsSource");
                ComboBox cmbItemsSource = new ComboBox(FindElement.ById("cmbItemsSource"));
                Verify.IsNotNull(cmbItemsSource, "Verifying that cmbItemsSource was found");

                Log.Comment("Changing cmbItemsSource selection to 'ObservableCollection<Recipe>'");
                cmbItemsSource.SelectItemByName("ObservableCollection<Recipe>");
                Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);

                Log.Comment("Retrieving and selecting first ItemContainer.");

                Log.Comment("Retrieving PART_ScrollPresenter");
                var scrollPresenter = FindElement.ById("PART_ScrollPresenter");
                Verify.IsNotNull(scrollPresenter, "Verifying that PART_ScrollPresenter was found");

                UIObject itemContainer = scrollPresenter.Children[0];
                Verify.IsNotNull(itemContainer, "Verifying that itemContainer is set");

                Log.Comment("itemContainer.Name=" + itemContainer.Name);
                Verify.AreEqual("ItemContainer", itemContainer.Name);

                Log.Comment("Select single item via LeftClick.");
                InputHelper.LeftClick(itemContainer);

                Rectangle itemContainerBounds = itemContainer.BoundingRectangle;
                Log.Comment("itemContainerBounds Bounds= X:{0}, Y:{1}, Width:{2}, Height:{3}",
                    itemContainerBounds.X, itemContainerBounds.Y, itemContainerBounds.Width, itemContainerBounds.Height);

                Point itemContainerCenter = new Point(
                    itemContainerBounds.X + itemContainerBounds.Width / 2,
                    itemContainerBounds.Y + itemContainerBounds.Height / 2);

                Point itemContainerBoundsOffsetCenterForPan = new Point(
                    itemContainerCenter.X,
                    itemContainerCenter.Y - 50);

                Log.Comment("Panning down with touch.");
                InputHelper.Pan(
                    obj: itemContainer,
                    start: itemContainerCenter,
                    end: itemContainerBoundsOffsetCenterForPan,
                    holdDuration: 20,
                    panAcceleration: 0.0f,
                    dragDuration: 500,
                    waitForIdle: false);
                Wait.ForIdle();

                Point itemContainerBoundsOffsetCenterForDrag = new Point(
                    itemContainerCenter.X + 200,
                    itemContainerCenter.Y);

                Log.Comment("Dragging ItemContainer from its center, 200px to the right");
                InputHelper.Pan(
                    obj: itemContainer,
                    start: itemContainerCenter,
                    end: itemContainerBoundsOffsetCenterForDrag,
                    holdDuration: 1500,
                    panAcceleration: 0.0f,
                    dragDuration: 1000,
                    waitForIdle: false);
                Wait.ForIdle();

                Log.Comment("Retrieving lstLogs");
                ListBox lstLogs = new ListBox(FindElement.ById("lstLogs"));
                Verify.IsNotNull(lstLogs, "Verifying that lstLogs was found");

                bool foundDragStartingEvent = false;

                foreach (var item in lstLogs.AllItems)
                {
                    ListBoxItem lbItem = item as ListBoxItem;
                    string log = lbItem.Name;

                    Log.Comment("Log:" + log);

                    if (!foundDragStartingEvent)
                    {
                        foundDragStartingEvent = log == "UIElement_DragStarting AllowedOperations=Copy, Move, Link";
                    }
                }

                Verify.IsTrue(foundDragStartingEvent, "Verifying that UIElement.DragStarting was raised.");

                ResetItemsView(cmbItemsSource, cmbLayout: null);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Verifies that the ItemCollectionTransitionProvider animations complete with the expected values in the TransitionCompleted event args.")]
        public void VerifyItemCollectionTransitionProviderAnimations()
        {
            using (var setup = new TestSetupHelper(new[] { "ItemsView Tests", "navigateToTransitionProvider" }))
            {
                var addButton = FindElement.ById<Button>("AddButton");
                var deleteButton = FindElement.ById<Button>("DeleteButton");
                var moveButton = FindElement.ById<Button>("MoveButton");
                var refreshButton = FindElement.ById<Button>("RefreshButton");
                var clearHistoryButton = FindElement.ById<Button>("ClearHistoryButton");
                var historyTextBox = FindElement.ById<Edit>("TransitionCompletedHistoryTextBox");

                Log.Comment("Refreshing the item list...");
                string expectedHistoryAfterRefresh = "Operation = Add, Triggers = CollectionChangeReset, OldBounds = (0, 0, 0, 0), NewBounds = (0, 0, 0, 0)\r\nOperation = Move, Triggers = LayoutTransition, OldBounds = (0, 0, 1, 96), NewBounds = (0, 0, 160, 96)";

                using (var eventWaiter = new ValueChangedEventWaiter(historyTextBox, expectedHistoryAfterRefresh))
                {
                    refreshButton.InvokeAndWait();
                }

                clearHistoryButton.InvokeAndWait();
                Log.Comment("Adding a new item to the start...");
                string expectedHistoryAfterAdd = "Operation = Add, Triggers = CollectionChangeAdd, OldBounds = 0,0,0,0, NewBounds = 0,0,0,0\r\nOperation = Move, Triggers = CollectionChangeAdd, OldBounds = (0, 0, 160, 96), NewBounds = (281, 0, 160, 96)";

                using (var eventWaiter = new ValueChangedEventWaiter(historyTextBox, expectedHistoryAfterAdd))
                {
                    addButton.InvokeAndWait();
                }

                clearHistoryButton.InvokeAndWait();
                Log.Comment("Selecting the second item...");
                ListViewItem secondItem = FindElement.ById<ListViewItem>("ms-appx:///Images/vette1.jpg");
                secondItem.Select();
                Wait.ForIdle();
                Log.Comment("Moving the second item to be first...");
                string expectedHistoryAfterMove = "Operation = Add, Triggers = CollectionChangeAdd, OldBounds = (0, 0, 0, 0), NewBounds = (0, 0, 0, 0)\r\nOperation = Move, Triggers = CollectionChangeAdd, CollectionChangeRemove, OldBounds = (0, 0, 277, 96), NewBounds = (164, 0, 277, 96)\r\nOperation = Remove, Triggers = CollectionChangeRemove, OldBounds = (0, 0, 0, 0), NewBounds = (0, 0, 0, 0)";

                using (var eventWaiter = new ValueChangedEventWaiter(historyTextBox, expectedHistoryAfterMove))
                {
                    moveButton.InvokeAndWait();
                }

                clearHistoryButton.InvokeAndWait();
                Log.Comment("Selecting the first item...");
                ListViewItem firstItem = FindElement.ById<ListViewItem>("ms-appx:///Images/vette1.jpg");
                firstItem.Select();
                Wait.ForIdle();
                Log.Comment("Deleting the first item...");
                string expectedHistoryAfterDelete = "Operation = Move, Triggers = CollectionChangeRemove, OldBounds = (164, 0, 277, 96), NewBounds = (0, 0, 277, 96)\r\nOperation = Remove, Triggers = CollectionChangeRemove, OldBounds = (0, 0, 0, 0), NewBounds = (0, 0, 0, 0)";
                
                using (var eventWaiter = new ValueChangedEventWaiter(historyTextBox, expectedHistoryAfterDelete))
                {
                    deleteButton.InvokeAndWait();
                }
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Verifies that the LinedFlowLayout can use ItemContainer without MinWidth.")]
        public void UseLinedFlowLayoutWithoutMinWidth()
        {
            Log.Comment("Selecting ItemsView summary page");

            ICollection<string> outputDebugStringComponentTypes = new[] { "ScrollView", "LinedFlowLayout", "ItemsView" };

            using (var setup = new ItemsViewTestSetupHelper(
                testNames: new[] { "ItemsView Tests", "navigateToSummary" },
                options: null,
                shouldRestrictInnerFrameSize: false,
                outputDebugStringLevel: "Verbose",
                outputDebugStringComponentTypes: outputDebugStringComponentTypes))
            {
                using (var loggingHelper = new LoggingHelper(this, outputDebugStringComponentTypes))
                {
                    Log.Comment("Disabling ItemsInfoRequested event handler.");

                    Log.Comment("Retrieving chkPageMethods");
                    UIObject chkPageMethodsUIObject = FindElement.ById("chkPageMethods");
                    Verify.IsNotNull(chkPageMethodsUIObject, "Verifying that chkPageMethods was found");
                    CheckBox chkPageMethods = new CheckBox(chkPageMethodsUIObject);
                    Verify.AreEqual(ToggleState.Off, chkPageMethods.ToggleState);

                    Log.Comment("Toggling chkPageMethods.IsChecked to ToggleState.On");
                    chkPageMethods.Toggle();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, chkPageMethods.ToggleState);

                    Log.Comment("Retrieving chkProvideDesiredAspectRatioItemsInfo");
                    UIObject chkProvideDesiredAspectRatioItemsInfoUIObject = FindElement.ById("chkProvideDesiredAspectRatioItemsInfo");
                    Verify.IsNotNull(chkProvideDesiredAspectRatioItemsInfoUIObject, "Verifying that chkProvideDesiredAspectRatioItemsInfo was found");
                    CheckBox chkProvideDesiredAspectRatioItemsInfo = new CheckBox(chkProvideDesiredAspectRatioItemsInfoUIObject);
                    if (ToggleState.On == chkProvideDesiredAspectRatioItemsInfo.ToggleState)
                    {
                        Log.Comment("Toggling chkProvideDesiredAspectRatioItemsInfo.IsChecked to ToggleState.Off");
                        chkProvideDesiredAspectRatioItemsInfo.Toggle();
                        Wait.ForIdle();
                    }
                    Verify.AreEqual(ToggleState.Off, chkProvideDesiredAspectRatioItemsInfo.ToggleState);

                    Log.Comment("Toggling chkPageMethods.IsChecked to ToggleState.Off");
                    chkPageMethods.Toggle();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, chkPageMethods.ToggleState);

                    Log.Comment("Setting up LinedFlowLayout.");

                    Log.Comment("Retrieving & filling txtLinedFlowLayoutLineHeight");
                    UIObject linedFlowLayoutLineHeightTextBoxUIObject = FindElement.ById("txtLinedFlowLayoutLineHeight");
                    Verify.IsNotNull(linedFlowLayoutLineHeightTextBoxUIObject, "Verifying that linedFlowLayoutLineHeightTextBox was found");
                    Edit linedFlowLayoutLineHeightTextBox = new Edit(linedFlowLayoutLineHeightTextBoxUIObject);

                    linedFlowLayoutLineHeightTextBox.SetValueAndWait("196");
                    Wait.ForIdle();

                    Log.Comment("Retrieving btnSetLinedFlowLayoutLineHeight");
                    UIObject btnSetLinedFlowLayoutLineHeightUIObject = FindElement.ById("btnSetLinedFlowLayoutLineHeight");
                    Verify.IsNotNull(btnSetLinedFlowLayoutLineHeightUIObject, "Verifying that btnSetLinedFlowLayoutLineHeight was found");
                    Button btnSetLinedFlowLayoutLineHeight = new Button(btnSetLinedFlowLayoutLineHeightUIObject);

                    Log.Comment("Invoking btnSetLinedFlowLayoutLineHeight");
                    btnSetLinedFlowLayoutLineHeight.InvokeAndWait();

                    Log.Comment("Retrieving & filling txtLinedFlowLayoutLineSpacing");
                    UIObject txtLinedFlowLayoutLineSpacingUIObject = FindElement.ById("txtLinedFlowLayoutLineSpacing");
                    Verify.IsNotNull(txtLinedFlowLayoutLineSpacingUIObject, "Verifying that txtLinedFlowLayoutLineSpacing was found");
                    Edit txtLinedFlowLayoutLineSpacing = new Edit(txtLinedFlowLayoutLineSpacingUIObject);

                    txtLinedFlowLayoutLineSpacing.SetValueAndWait("4");
                    Wait.ForIdle();

                    Log.Comment("Retrieving btnSetLinedFlowLayoutLineSpacing");
                    UIObject btnSetLinedFlowLayoutLineSpacingUIObject = FindElement.ById("btnSetLinedFlowLayoutLineSpacing");
                    Verify.IsNotNull(btnSetLinedFlowLayoutLineSpacingUIObject, "Verifying that btnSetLinedFlowLayoutLineSpacing was found");
                    Button btnSetLinedFlowLayoutLineSpacing = new Button(btnSetLinedFlowLayoutLineSpacingUIObject);

                    Log.Comment("Invoking btnSetLinedFlowLayoutLineSpacing");
                    btnSetLinedFlowLayoutLineSpacing.InvokeAndWait();

                    Log.Comment("Retrieving & filling txtLinedFlowLayoutMinItemSpacing");
                    UIObject txtLinedFlowLayoutMinItemSpacingUIObject = FindElement.ById("txtLinedFlowLayoutMinItemSpacing");
                    Verify.IsNotNull(txtLinedFlowLayoutMinItemSpacingUIObject, "Verifying that txtLinedFlowLayoutMinItemSpacing was found");
                    Edit txtLinedFlowLayoutMinItemSpacing = new Edit(txtLinedFlowLayoutMinItemSpacingUIObject);

                    txtLinedFlowLayoutMinItemSpacing.SetValueAndWait("4");
                    Wait.ForIdle();

                    Log.Comment("Retrieving btnSetLinedFlowLayoutMinItemSpacing");
                    UIObject btnSetLinedFlowLayoutMinItemSpacingUIObject = FindElement.ById("btnSetLinedFlowLayoutMinItemSpacing");
                    Verify.IsNotNull(btnSetLinedFlowLayoutMinItemSpacingUIObject, "Verifying that btnSetLinedFlowLayoutMinItemSpacing was found");
                    Button btnSetLinedFlowLayoutMinItemSpacing = new Button(btnSetLinedFlowLayoutMinItemSpacingUIObject);

                    Log.Comment("Invoking btnSetLinedFlowLayoutMinItemSpacing");
                    btnSetLinedFlowLayoutMinItemSpacing.InvokeAndWait();

                    Log.Comment("Retrieving cmbLinedFlowLayoutItemsStretch");
                    UIObject cmbLinedFlowLayoutItemsStretchUIObject = FindElement.ById("cmbLinedFlowLayoutItemsStretch");
                    Verify.IsNotNull(cmbLinedFlowLayoutItemsStretchUIObject, "Verifying that cmbLinedFlowLayoutItemsStretch was found");
                    ComboBox cmbLinedFlowLayoutItemsStretch = new ComboBox(cmbLinedFlowLayoutItemsStretchUIObject);

                    Log.Comment("Changing cmbLinedFlowLayoutItemsStretch selection to 'Fill'");
                    cmbLinedFlowLayoutItemsStretch.SelectItemByName("Fill");
                    Log.Comment("Selection is now {0}", cmbLinedFlowLayoutItemsStretch.Selection[0].Name);

                    Log.Comment("Setting up ItemsView.");

                    Log.Comment("Retrieving cmbLayout");
                    UIObject cmbLayoutUIObject = FindElement.ById("cmbLayout");
                    Verify.IsNotNull(cmbLayoutUIObject, "Verifying that cmbLayout was found");
                    ComboBox cmbLayout = new ComboBox(cmbLayoutUIObject);

                    Log.Comment("Changing cmbLayout selection to 'LinedFlowLayout'");
                    cmbLayout.SelectItemByName("LinedFlowLayout");
                    Log.Comment("Selection is now {0}", cmbLayout.Selection[0].Name);

                    Log.Comment("Retrieving cmbItemTemplate");
                    UIObject cmbItemTemplateUIObject = FindElement.ById("cmbItemTemplate");
                    Verify.IsNotNull(cmbItemTemplateUIObject, "Verifying that cmbItemTemplate was found");
                    ComboBox cmbItemTemplate = new ComboBox(cmbItemTemplateUIObject);

                    Log.Comment("Changing cmbItemTemplate selection to 'ItemContainer without MinWidth (Image)'");
                    cmbItemTemplate.SelectItemByName("ItemContainer without MinWidth (Image)");
                    Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);

                    Log.Comment("Retrieving cmbItemsSource");
                    UIObject cmbItemsSourceUIObject = FindElement.ById("cmbItemsSource");
                    Verify.IsNotNull(cmbItemsSourceUIObject, "Verifying that cmbItemsSource was found");
                    ComboBox cmbItemsSource = new ComboBox(cmbItemsSourceUIObject);

                    Log.Comment("Changing cmbItemsSource selection to 'ObservableCollection<Recipe>'");
                    cmbItemsSource.SelectItemByName("ObservableCollection<Recipe>");
                    Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);
                    Wait.ForIdle();

                    Log.Comment("Retrieving markupItemsView");
                    UIObject itemsViewUIObject = FindElement.ById("markupItemsView");
                    Verify.IsNotNull(itemsViewUIObject, "Verifying that itemsView was found");

                    int currentItemIndex = GetCurrentItemIndex();
                    Verify.AreEqual(-1, currentItemIndex, "Verifying CurrentItemIndex is initially -1");

                    Log.Comment("Clicking on ItemsView to select first item.");
                    itemsViewUIObject.Click(PointerButtons.Primary, 40.0, 40.0);
                    Wait.ForIdle();

                    LogScrollViewInfo(itemsViewUIObject, expectedVerticalOffset: null, getScrollViewView: false);
                    currentItemIndex = GetCurrentItemIndex();
                    Verify.AreEqual(0, currentItemIndex, "Verifying CurrentItemIndex is 0");

                    Log.Comment("Clicking on ItemsView to focus item again.");
                    itemsViewUIObject.Click(PointerButtons.Primary, 40.0, 40.0);
                    Wait.ForIdle();

                    Log.Comment("Pressing End key");
                    KeyboardHelper.PressKey(Key.End);
                    Wait.ForIdle();

                    LogScrollViewInfo(itemsViewUIObject, expectedVerticalOffset: null, getScrollViewView: false);
                    currentItemIndex = GetCurrentItemIndex();
                    Verify.AreEqual(299, currentItemIndex, "Verifying CurrentItemIndex is last item 299");

                    ResetItemsView(cmbItemsSource, cmbLayout);
                }
            }
        }


        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Description", "Tests LinedFlowLayout without setting its LineHeight in the fast path.")]
        public void UseLinedFlowLayoutWithoutLineHeight()
        {
            Log.Comment("Selecting ItemsView summary page");

            ICollection<string> outputDebugStringComponentTypes = new[] { "ScrollView", "LinedFlowLayout", "ItemsView" };

            using (var setup = new ItemsViewTestSetupHelper(
                testNames: new[] { "ItemsView Tests", "navigateToSummary" },
                options: null,
                shouldRestrictInnerFrameSize: false,
                outputDebugStringLevel: "Info",
                outputDebugStringComponentTypes: outputDebugStringComponentTypes))
            {
                using (var loggingHelper = new LoggingHelper(this, outputDebugStringComponentTypes))
                {
                    Log.Comment("Setting up ItemsView.");

                    Log.Comment("Retrieving cmbLayout");
                    UIObject cmbLayoutUIObject = FindElement.ById("cmbLayout");
                    Verify.IsNotNull(cmbLayoutUIObject, "Verifying that cmbLayout was found");
                    ComboBox cmbLayout = new ComboBox(cmbLayoutUIObject);

                    Log.Comment("Changing cmbLayout selection to 'LinedFlowLayout'");
                    cmbLayout.SelectItemByName("LinedFlowLayout");
                    Log.Comment("Selection is now {0}", cmbLayout.Selection[0].Name);

                    Log.Comment("Retrieving cmbItemTemplate");
                    UIObject cmbItemTemplateUIObject = FindElement.ById("cmbItemTemplate");
                    Verify.IsNotNull(cmbItemTemplateUIObject, "Verifying that cmbItemTemplate was found");
                    ComboBox cmbItemTemplate = new ComboBox(cmbItemTemplateUIObject);

                    Log.Comment("Changing cmbItemTemplate selection to 'ItemContainer without MinWidth (Image)'");
                    cmbItemTemplate.SelectItemByName("ItemContainer without MinWidth (Image)");
                    Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);

                    Log.Comment("Retrieving cmbItemsSource");
                    UIObject cmbItemsSourceUIObject = FindElement.ById("cmbItemsSource");
                    Verify.IsNotNull(cmbItemsSourceUIObject, "Verifying that cmbItemsSource was found");
                    ComboBox cmbItemsSource = new ComboBox(cmbItemsSourceUIObject);

                    Log.Comment("Changing cmbItemsSource selection to 'ObservableCollection<Recipe>'");
                    cmbItemsSource.SelectItemByName("ObservableCollection<Recipe>");
                    Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);
                    Wait.ForIdle();

                    Log.Comment("Retrieving txtExtentWidth");
                    UIObject txtExtentWidthUIObject = FindElement.ById("txtExtentWidth");
                    Verify.IsNotNull(txtExtentWidthUIObject, "Verifying that txtExtentWidth was found");
                    Edit txtExtentWidth = new Edit(txtExtentWidthUIObject);

                    Log.Comment("Retrieving txtExtentHeight");
                    UIObject txtExtentHeightUIObject = FindElement.ById("txtExtentHeight");
                    Verify.IsNotNull(txtExtentHeightUIObject, "Verifying that txtExtentHeight was found");
                    Edit txtExtentHeight = new Edit(txtExtentHeightUIObject);

                    Log.Comment("Retrieving btnGetExtentHeight");
                    UIObject btnGetExtentHeightUIObject = FindElement.ById("btnGetExtentHeight");
                    Verify.IsNotNull(btnGetExtentHeightUIObject, "Verifying that btnGetExtentHeight was found");
                    Button btnGetExtentHeight = new Button(btnGetExtentHeightUIObject);

                    Log.Comment("Retrieving btnGetExtentWidth");
                    UIObject btnGetExtentWidthUIObject = FindElement.ById("btnGetExtentWidth");
                    Verify.IsNotNull(btnGetExtentWidthUIObject, "Verifying that btnGetExtentWidth was found");
                    Button btnGetExtentWidth = new Button(btnGetExtentWidthUIObject);

                    Log.Comment("Invoking btnGetExtentWidth");
                    btnGetExtentWidth.InvokeAndWait();

                    Log.Comment("Invoking btnGetExtentHeight");
                    btnGetExtentHeight.InvokeAndWait();

                    Log.Comment($"ScrollView.ExtentWidth={txtExtentWidth.Value}");
                    Log.Comment($"ScrollView.ExtentHeight={txtExtentHeight.Value}");

                    double extentWidth = double.Parse(txtExtentWidth.Value);
                    double extentHeight = double.Parse(txtExtentHeight.Value);

                    Verify.AreEqual(extentWidth, 500.0);
                    Verify.IsGreaterThan(extentHeight, 50000.0);

                    ResetItemsView(cmbItemsSource, cmbLayout);
                }
            }
        }

        // When clearItemAspectRatios is True, the LinedFlowLayout::m_aspectRatios field is cleared before an ItemsView.StartBringItemIntoView call
        // to minimize the average-items-per-line stability during the operation. 
        private void StartBringItemIntoView(bool clearItemAspectRatios)
        {
            Log.Comment("Selecting ItemsView summary page");

            ICollection<string> outputDebugStringComponentTypes = new[] { "ScrollView", "LinedFlowLayout", "ItemsView" };

            using (var setup = new ItemsViewTestSetupHelper(
                testNames: new[] { "ItemsView Tests", "navigateToSummary" },
                options: null,
                shouldRestrictInnerFrameSize: false,
                outputDebugStringLevel: "Verbose",
                outputDebugStringComponentTypes: outputDebugStringComponentTypes))
            {
                using (var loggingHelper = new LoggingHelper(this, outputDebugStringComponentTypes))
                {
                    Log.Comment("Disabling fast path.");

                    Log.Comment("Retrieving chkPageMethods");
                    CheckBox chkPageMethods = new CheckBox(FindElement.ById("chkPageMethods"));
                    Verify.IsNotNull(chkPageMethods, "Verifying that chkPageMethods was found");
                    Verify.AreEqual(ToggleState.Off, chkPageMethods.ToggleState);

                    Log.Comment("Toggling chkPageMethods.IsChecked to ToggleState.On");
                    chkPageMethods.Toggle();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, chkPageMethods.ToggleState);

                    Log.Comment("Retrieving chkProvideExtraDesiredAspectRatioItemsInfo");
                    CheckBox chkProvideExtraDesiredAspectRatioItemsInfo = new CheckBox(FindElement.ById("chkProvideExtraDesiredAspectRatioItemsInfo"));
                    Verify.IsNotNull(chkProvideExtraDesiredAspectRatioItemsInfo, "Verifying that chkProvideExtraDesiredAspectRatioItemsInfo was found");
                    Verify.AreEqual(ToggleState.On, chkProvideExtraDesiredAspectRatioItemsInfo.ToggleState);

                    Log.Comment("Toggling chkProvideExtraDesiredAspectRatioItemsInfo.IsChecked to ToggleState.Off");
                    chkProvideExtraDesiredAspectRatioItemsInfo.Toggle();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, chkProvideExtraDesiredAspectRatioItemsInfo.ToggleState);

                    Log.Comment("Toggling chkPageMethods.IsChecked to ToggleState.Off");
                    chkPageMethods.Toggle();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, chkPageMethods.ToggleState);

                    Log.Comment("Setting up ItemsView.");

                    Log.Comment("Retrieving cmbLayout");
                    ComboBox cmbLayout = new ComboBox(FindElement.ById("cmbLayout"));
                    Verify.IsNotNull(cmbLayout, "Verifying that cmbLayout was found");

                    Log.Comment("Changing cmbLayout selection to 'LinedFlowLayout'");
                    cmbLayout.SelectItemByName("LinedFlowLayout");
                    Log.Comment("Selection is now {0}", cmbLayout.Selection[0].Name);

                    Log.Comment("Retrieving cmbItemTemplate");
                    ComboBox cmbItemTemplate = new ComboBox(FindElement.ById("cmbItemTemplate"));
                    Verify.IsNotNull(cmbItemTemplate, "Verifying that cmbItemTemplate was found");

                    Log.Comment("Changing cmbItemTemplate selection to 'Fixed-Height ItemContainer (Image + TextBlock)'");
                    cmbItemTemplate.SelectItemByName("Fixed-Height ItemContainer (Image + TextBlock)");
                    Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);

                    Log.Comment("Retrieving cmbItemsSource");
                    ComboBox cmbItemsSource = new ComboBox(FindElement.ById("cmbItemsSource"));
                    Verify.IsNotNull(cmbItemsSource, "Verifying that cmbItemsSource was found");

                    Log.Comment("Changing cmbItemsSource selection to 'ObservableCollection<Recipe>'");
                    cmbItemsSource.SelectItemByName("ObservableCollection<Recipe>");
                    Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);

                    Log.Comment("Setting up ItemsView.StartBringItemIntoView parameters.");

                    Log.Comment("Retrieving & filling txtItemsViewMethodIndex");
                    Edit itemsViewMethodIndexTextBox = new Edit(FindElement.ById("txtItemsViewMethodIndex"));
                    Verify.IsNotNull(itemsViewMethodIndexTextBox, "Verifying that itemsViewMethodIndexTextBox was found");

                    itemsViewMethodIndexTextBox.SetValueAndWait("222");
                    Wait.ForIdle();

                    Log.Comment("Retrieving & filling txtBringIntoViewOptionsVerticalAlignmentRatio");
                    Edit bringIntoViewOptionsVerticalAlignmentRatioTextBox = new Edit(FindElement.ById("txtBringIntoViewOptionsVerticalAlignmentRatio"));
                    Verify.IsNotNull(bringIntoViewOptionsVerticalAlignmentRatioTextBox, "Verifying that bringIntoViewOptionsVerticalAlignmentRatioTextBox was found");

                    bringIntoViewOptionsVerticalAlignmentRatioTextBox.SetValueAndWait("0");
                    Wait.ForIdle();

                    Button btnLinedFlowLayoutClearItemAspectRatios = null;

                    if (clearItemAspectRatios)
                    {
                        Log.Comment("Retrieving btnLinedFlowLayoutClearItemAspectRatios");
                        btnLinedFlowLayoutClearItemAspectRatios = new Button(FindElement.ById("btnLinedFlowLayoutClearItemAspectRatios"));
                        Verify.IsNotNull(btnLinedFlowLayoutClearItemAspectRatios, "Verifying that btnLinedFlowLayoutClearItemAspectRatios was found");

                        Log.Comment("Invoking btnLinedFlowLayoutClearItemAspectRatios");
                        btnLinedFlowLayoutClearItemAspectRatios.Invoke();
                        Wait.ForIdle();
                    }

                    Log.Comment("Retrieving btnItemsViewStartBringItemIntoView");
                    Button btnItemsViewStartBringItemIntoView = new Button(FindElement.ById("btnItemsViewStartBringItemIntoView"));
                    Verify.IsNotNull(btnItemsViewStartBringItemIntoView, "Verifying that btnItemsViewStartBringItemIntoView was found");

                    Log.Comment("Invoking btnItemsViewStartBringItemIntoView");
                    btnItemsViewStartBringItemIntoView.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Retrieving btnGetVerticalOffset");
                    Button btnGetVerticalOffset = new Button(FindElement.ById("btnGetVerticalOffset"));
                    Verify.IsNotNull(btnGetVerticalOffset, "Verifying that btnGetVerticalOffset was found");

                    Log.Comment("Invoking btnGetVerticalOffset");
                    btnGetVerticalOffset.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Retrieving & reading txtVerticalOffset");
                    Edit verticalOffsetTextBox = new Edit(FindElement.ById("txtVerticalOffset"));
                    Verify.IsNotNull(verticalOffsetTextBox, "Verifying that verticalOffsetTextBox was found");
                    Wait.ForIdle();

                    Log.Comment($"ScrollView.VerticalOffset={verticalOffsetTextBox.Value}");

                    Log.Comment("Retrieving & filling txtLinedFlowLayoutMethodIndex");
                    Edit linedFlowLayoutMethodIndexTextBox = new Edit(FindElement.ById("txtLinedFlowLayoutMethodIndex"));
                    Verify.IsNotNull(linedFlowLayoutMethodIndexTextBox, "Verifying that linedFlowLayoutMethodIndexTextBox was found");

                    linedFlowLayoutMethodIndexTextBox.SetValueAndWait("222");
                    Wait.ForIdle();

                    Log.Comment("Retrieving btnLinedFlowLayoutGetLineIndex");
                    Button btnLinedFlowLayoutGetLineIndex = new Button(FindElement.ById("btnLinedFlowLayoutGetLineIndex"));
                    Verify.IsNotNull(btnLinedFlowLayoutGetLineIndex, "Verifying that btnLinedFlowLayoutGetLineIndex was found");

                    Log.Comment("Invoking btnLinedFlowLayoutGetLineIndex");
                    btnLinedFlowLayoutGetLineIndex.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Retrieving & reading txtLinedFlowLayoutLineIndex");
                    Edit linedFlowLayoutLineIndexTextBox = new Edit(FindElement.ById("txtLinedFlowLayoutLineIndex"));
                    Verify.IsNotNull(linedFlowLayoutLineIndexTextBox, "Verifying that linedFlowLayoutLineIndexTextBox was found");
                    Wait.ForIdle();

                    Log.Comment($"LinedFlowLayout's GetLineIndex(222)={linedFlowLayoutLineIndexTextBox.Value}");
                    Verify.IsLessThan(Math.Abs(double.Parse(linedFlowLayoutLineIndexTextBox.Value) * 96.0 - double.Parse(verticalOffsetTextBox.Value)), 0.01, "Verifying vertical offset is line index x LineHeight");

                    Log.Comment("Retrieving & filling txtTryGetItemIndexHorizontalViewportRatio");
                    Edit tryGetItemIndexHorizontalViewportRatioTextBox = new Edit(FindElement.ById("txtTryGetItemIndexHorizontalViewportRatio"));
                    Verify.IsNotNull(tryGetItemIndexHorizontalViewportRatioTextBox, "Verifying that tryGetItemIndexHorizontalViewportRatioTextBox was found");

                    tryGetItemIndexHorizontalViewportRatioTextBox.SetValueAndWait("0.0");
                    Wait.ForIdle();

                    Log.Comment("Retrieving & filling txtTryGetItemIndexVerticalViewportRatio");
                    Edit tryGetItemIndexVerticalViewportRatioTextBox = new Edit(FindElement.ById("txtTryGetItemIndexVerticalViewportRatio"));
                    Verify.IsNotNull(tryGetItemIndexVerticalViewportRatioTextBox, "Verifying that tryGetItemIndexVerticalViewportRatioTextBox was found");

                    tryGetItemIndexVerticalViewportRatioTextBox.SetValueAndWait("0.0");
                    Wait.ForIdle();

                    Log.Comment("Retrieving btnItemsViewTryGetItemIndex");
                    Button btnItemsViewTryGetItemIndex = new Button(FindElement.ById("btnItemsViewTryGetItemIndex"));
                    Verify.IsNotNull(btnItemsViewTryGetItemIndex, "Verifying that btnItemsViewTryGetItemIndex was found");

                    Log.Comment("Invoking btnItemsViewTryGetItemIndex");
                    btnItemsViewTryGetItemIndex.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Retrieving & reading txtItemsViewTryGetItemIndex");
                    Edit itemsViewTryGetItemIndexTextBox = new Edit(FindElement.ById("txtItemsViewTryGetItemIndex"));
                    Verify.IsNotNull(itemsViewTryGetItemIndexTextBox, "Verifying that itemsViewTryGetItemIndexTextBox was found");
                    Wait.ForIdle();

                    Log.Comment($"ItemsView.TryGetItemIndex(0.0, 0.0, out index)={itemsViewTryGetItemIndexTextBox.Value}");
                    Verify.IsGreaterThanOrEqual(222, int.Parse(itemsViewTryGetItemIndexTextBox.Value), "Verifying item index brought to the top of the viewport is greater than or equal to first displayed item index on top line");

                    Log.Comment("Filling txtTryGetItemIndexHorizontalViewportRatio");
                    tryGetItemIndexHorizontalViewportRatioTextBox.SetValueAndWait("1.0");
                    Wait.ForIdle();

                    Log.Comment("Invoking btnItemsViewTryGetItemIndex");
                    btnItemsViewTryGetItemIndex.Invoke();
                    Wait.ForIdle();

                    Log.Comment($"ItemsView.TryGetItemIndex(1.0, 0.0, out index)={itemsViewTryGetItemIndexTextBox.Value}");
                    Verify.IsLessThanOrEqual(222, int.Parse(itemsViewTryGetItemIndexTextBox.Value), "Verifying item index brought to the top of the viewport is less than or equal to last displayed item index on top line");

                    Log.Comment("Filling txtItemsViewMethodIndex");
                    itemsViewMethodIndexTextBox.SetValueAndWait("22");
                    Wait.ForIdle();

                    if (clearItemAspectRatios)
                    {
                        Log.Comment("Invoking btnLinedFlowLayoutClearItemAspectRatios");
                        btnLinedFlowLayoutClearItemAspectRatios.Invoke();
                        Wait.ForIdle();
                    }

                    Log.Comment("Invoking btnItemsViewStartBringItemIntoView");
                    btnItemsViewStartBringItemIntoView.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Invoking btnGetVerticalOffset");
                    btnGetVerticalOffset.Invoke();
                    Wait.ForIdle();

                    Log.Comment($"ScrollView.VerticalOffset={verticalOffsetTextBox.Value}");

                    Log.Comment("Filling txtLinedFlowLayoutMethodIndex");
                    linedFlowLayoutMethodIndexTextBox.SetValueAndWait("22");
                    Wait.ForIdle();

                    Log.Comment("Invoking btnLinedFlowLayoutGetLineIndex");
                    btnLinedFlowLayoutGetLineIndex.Invoke();
                    Wait.ForIdle();

                    Log.Comment($"LinedFlowLayout's GetLineIndex(22)={linedFlowLayoutLineIndexTextBox.Value}");
                    Verify.IsLessThan(Math.Abs(double.Parse(linedFlowLayoutLineIndexTextBox.Value) * 96.0 - double.Parse(verticalOffsetTextBox.Value)), 0.01, "Verifying vertical offset is line index x LineHeight");

                    Log.Comment("Filling txtTryGetItemIndexHorizontalViewportRatio");
                    tryGetItemIndexHorizontalViewportRatioTextBox.SetValueAndWait("0.0");
                    Wait.ForIdle();

                    Log.Comment("Invoking btnItemsViewTryGetItemIndex");
                    btnItemsViewTryGetItemIndex.Invoke();
                    Wait.ForIdle();

                    Log.Comment($"ItemsView.TryGetItemIndex(0.0, 0.0, out index)={itemsViewTryGetItemIndexTextBox.Value}");
                    Verify.IsGreaterThanOrEqual(22, int.Parse(itemsViewTryGetItemIndexTextBox.Value), "Verifying item index brought to the top of the viewport is greater than or equal to first displayed item index on top line");

                    Log.Comment("Filling txtTryGetItemIndexHorizontalViewportRatio");
                    tryGetItemIndexHorizontalViewportRatioTextBox.SetValueAndWait("1.0");
                    Wait.ForIdle();

                    Log.Comment("Invoking btnItemsViewTryGetItemIndex");
                    btnItemsViewTryGetItemIndex.Invoke();
                    Wait.ForIdle();

                    Log.Comment($"ItemsView.TryGetItemIndex(1.0, 0.0, out index)={itemsViewTryGetItemIndexTextBox.Value}");
                    Verify.IsLessThanOrEqual(22, int.Parse(itemsViewTryGetItemIndexTextBox.Value), "Verifying item index brought to the top of the viewport is less than or equal to last displayed item index on top line");

                    Log.Comment("Retrieving btnClearLogs");
                    Button btnClearLogs = new Button(FindElement.ById("btnClearLogs"));
                    Verify.IsNotNull(btnClearLogs, "Verifying that btnClearLogs was found");

                    Log.Comment("Invoking btnClearLogs.");
                    btnClearLogs.Invoke();
                    Wait.ForIdle();

                    LogAndClearTraces();
                    Wait.ForIdle();

                    ResetItemsView(cmbItemsSource, cmbLayout);
                }
            }
        }

        private void PanAnnotatedScrollBar(bool handleItemsInfoRequested)
        {
            Log.Comment("Retrieving RasterizationScaleTextBox");
            Edit rasterizationScaleTextBox = new Edit(FindElement.ById("RasterizationScaleTextBox"));
            if (rasterizationScaleTextBox == null)
            {
                Log.Warning("rasterizationScaleTextBox not found.");
            }
            else
            {
                string rasterizationScale = rasterizationScaleTextBox.Value;
                Log.Comment("RasterizationScale:" + rasterizationScale);
                if (rasterizationScale != "1")
                {
                    Log.Warning("Skipping test as panning is only supported for RasterizationScale == 1.");
                    return;
                }
            }

            Log.Comment("Selecting ItemsView/ItemContainer/AnnotatedScrollBar integration page");

            ICollection<string> outputDebugStringComponentTypes = new[] { "ScrollView", "ScrollPresenter", "LinedFlowLayout", "AnnotatedScrollBar", /*"ItemsRepeater",*/ "ItemsView" };

            using (var setup = new ItemsViewTestSetupHelper(
                testNames: new[] { "ItemsView Tests", "navigateToIntegration" },
                options: null,
                shouldRestrictInnerFrameSize: false,
                outputDebugStringLevel: "Info",
                outputDebugStringComponentTypes: outputDebugStringComponentTypes))
            {
                using (var loggingHelper = new LoggingHelper(this, outputDebugStringComponentTypes))
                {
                    Log.Comment("Retrieving chkHandleItemsInfoRequested");
                    CheckBox chkHandleItemsInfoRequested = new CheckBox(FindElement.ById("chkHandleItemsInfoRequested"));
                    Verify.IsNotNull(chkHandleItemsInfoRequested, "Verifying that chkHandleItemsInfoRequested was found");

                    Log.Comment("Retrieving chkGeneralInfoLogs");
                    CheckBox chkGeneralInfoLogs = new CheckBox(FindElement.ById("chkGeneralInfoLogs"));
                    Verify.IsNotNull(chkGeneralInfoLogs, "Verifying that chkGeneralInfoLogs was found");

                    Log.Comment("Retrieving chkGeneralVerboseLogs");
                    CheckBox chkGeneralVerboseLogs = new CheckBox(FindElement.ById("chkGeneralVerboseLogs"));
                    Verify.IsNotNull(chkGeneralVerboseLogs, "Verifying that chkGeneralVerboseLogs was found");

                    Log.Comment("Retrieving btnClearLogs");
                    Button btnClearLogs = new Button(FindElement.ById("btnClearLogs"));
                    Verify.IsNotNull(btnClearLogs, "Verifying that btnClearLogs was found");

                    Log.Comment("Retrieving logging checkboxes");
                    CheckBox chkLogItemsViewEvents = new CheckBox(FindElement.ById("chkLogItemsViewEvents"));
                    CheckBox chkLogLinedFlowLayoutEvents = new CheckBox(FindElement.ById("chkLogLinedFlowLayoutEvents"));
                    CheckBox chkLogAnnotatedScrollBarEvents = new CheckBox(FindElement.ById("chkLogAnnotatedScrollBarEvents"));
                    CheckBox chkLogItemsRepeaterEvents = new CheckBox(FindElement.ById("chkLogItemsRepeaterEvents"));
                    CheckBox chkLogScrollViewEvents = new CheckBox(FindElement.ById("chkLogScrollViewEvents"));
                    CheckBox chkLogScrollPresenterEvents = new CheckBox(FindElement.ById("chkLogScrollPresenterEvents"));

                    Log.Comment("Retrieving txtYearCount");
                    Edit yearCountTextBox = new Edit(FindElement.ById("txtYearCount"));
                    Verify.IsNotNull(yearCountTextBox, "Verifying that yearCountTextBox was found");

                    Log.Comment("Retrieving txtAverageMonthCount");
                    Edit averageMonthCountTextBox = new Edit(FindElement.ById("txtAverageMonthCount"));
                    Verify.IsNotNull(averageMonthCountTextBox, "Verifying that averageMonthCountTextBox was found");

                    Log.Comment("Retrieving btnSetYearCount");
                    Button btnSetYearCount = new Button(FindElement.ById("btnSetYearCount"));
                    Verify.IsNotNull(btnSetYearCount, "Verifying that btnSetYearCount was found");

                    Log.Comment("Retrieving btnSetAverageMonthCount");
                    Button btnSetAverageMonthCount = new Button(FindElement.ById("btnSetAverageMonthCount"));
                    Verify.IsNotNull(btnSetAverageMonthCount, "Verifying that btnSetAverageMonthCount was found");

                    Log.Comment("Retrieving btnApplyAverageMonthCount");
                    Button btnApplyAverageMonthCount = new Button(FindElement.ById("btnApplyAverageMonthCount"));
                    Verify.IsNotNull(btnApplyAverageMonthCount, "Verifying that btnApplyAverageMonthCount was found");

                    Log.Comment("Retrieving btnGetScrollViewVerticalOffset");
                    Button btnGetScrollViewVerticalOffset = new Button(FindElement.ById("btnGetScrollViewVerticalOffset"));
                    Verify.IsNotNull(btnGetScrollViewVerticalOffset, "Verifying that btnGetScrollViewVerticalOffset was found");

                    Log.Comment("Retrieving txtScrollViewVerticalOffset.");
                    Edit scrollViewVerticalOffsetTextBox = new Edit(FindElement.ById("txtScrollViewVerticalOffset"));
                    Verify.IsNotNull(scrollViewVerticalOffsetTextBox, "Verifying that scrollViewVerticalOffsetTextBox was found");

                    Log.Comment("Retrieving txtStatus");
                    UIObject statusUIObject = FindElement.ById("txtStatus");
                    Verify.IsNotNull(statusUIObject, "Verifying that txtStatus was found");

                    Log.Comment("Retrieving cmbItemTemplate");
                    ComboBox cmbItemTemplate = new ComboBox(FindElement.ById("cmbItemTemplate"));
                    Verify.IsNotNull(cmbItemTemplate, "Verifying that cmbItemTemplate was found");

                    Log.Comment("Retrieving annotatedScrollBar");
                    UIObject annotatedScrollBar = FindElement.ById("annotatedScrollBar");
                    Verify.IsNotNull(annotatedScrollBar, "Verifying that annotatedScrollBar was found");

                    if (ToggleState.Off == chkHandleItemsInfoRequested.ToggleState && handleItemsInfoRequested)
                    {
                        Log.Comment("Toggling chkHandleItemsInfoRequested.IsChecked to ToggleState.On");
                        chkHandleItemsInfoRequested.Toggle();
                        Wait.ForIdle();
                    }
                    else if (ToggleState.On == chkHandleItemsInfoRequested.ToggleState && !handleItemsInfoRequested)
                    {
                        Log.Comment("Toggling chkHandleItemsInfoRequested.IsChecked to ToggleState.Off");
                        chkHandleItemsInfoRequested.Toggle();
                        Wait.ForIdle();
                    }

                    Log.Comment("Changing ItemTemplate selection to 'Extra Large Image with caption'");
                    cmbItemTemplate.SelectItemByName("Extra Large Image with caption");
                    Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);

                    Log.Comment("Setting YearCount to 30.");
                    yearCountTextBox.SetValueAndWait("30");
                    btnSetYearCount.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Setting AverageMonthCount to 6.");
                    averageMonthCountTextBox.SetValueAndWait("6");
                    btnSetAverageMonthCount.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Applying new YearCount & AverageMonthCount.");
                    btnApplyAverageMonthCount.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Toggling logging checkboxes");

                    if (ToggleState.Off == chkGeneralInfoLogs.ToggleState)
                    {
                        Log.Comment("Toggling chkGeneralInfoLogs.IsChecked to ToggleState.On");
                        chkGeneralInfoLogs.Toggle();
                        Wait.ForIdle();
                    }

                    if (ToggleState.On == chkGeneralVerboseLogs.ToggleState)
                    {
                        Log.Comment("Toggling chkGeneralVerboseLogs.IsChecked to ToggleState.Off");
                        chkGeneralVerboseLogs.Toggle();
                        Wait.ForIdle();
                    }

                    if (ToggleState.Off == chkLogItemsViewEvents.ToggleState)
                    {
                        Log.Comment("Toggling chkLogItemsViewEvents.IsChecked to ToggleState.On");
                        chkLogItemsViewEvents.Toggle();
                        Wait.ForIdle();
                    }

                    if (ToggleState.Off == chkLogLinedFlowLayoutEvents.ToggleState)
                    {
                        Log.Comment("Toggling chkLogLinedFlowLayoutEvents.IsChecked to ToggleState.On");
                        chkLogLinedFlowLayoutEvents.Toggle();
                        Wait.ForIdle();
                    }

                    if (ToggleState.Off == chkLogAnnotatedScrollBarEvents.ToggleState)
                    {
                        Log.Comment("Toggling chkLogAnnotatedScrollBarEvents.IsChecked to ToggleState.On");
                        chkLogAnnotatedScrollBarEvents.Toggle();
                        Wait.ForIdle();
                    }

                    if (ToggleState.Off == chkLogItemsRepeaterEvents.ToggleState)
                    {
                        Log.Comment("Toggling chkLogItemsRepeaterEvents.IsChecked to ToggleState.On");
                        chkLogItemsRepeaterEvents.Toggle();
                        Wait.ForIdle();
                    }

                    if (ToggleState.Off == chkLogScrollViewEvents.ToggleState)
                    {
                        Log.Comment("Toggling chkLogScrollViewEvents.IsChecked to ToggleState.On");
                        chkLogScrollViewEvents.Toggle();
                        Wait.ForIdle();
                    }

                    if (ToggleState.Off == chkLogScrollPresenterEvents.ToggleState)
                    {
                        Log.Comment("Toggling chkLogScrollPresenterEvents.IsChecked to ToggleState.On");
                        chkLogScrollPresenterEvents.Toggle();
                        Wait.ForIdle();
                    }

                    Log.Comment("Clearing log list.");
                    btnClearLogs.Invoke();
                    ClearTraces();
                    Wait.ForIdle();

                    Rectangle annotatedScrollBarBounds = annotatedScrollBar.BoundingRectangle;
                    Log.Comment("AnnotatedScrollBar Bounds= X:{0}, Y:{1}, Width:{2}, Height:{3}",
                        annotatedScrollBarBounds.X, annotatedScrollBarBounds.Y, annotatedScrollBarBounds.Width, annotatedScrollBarBounds.Height);

                    Point annotatedScrollBarCenter = new Point(
                        annotatedScrollBarBounds.X + annotatedScrollBarBounds.Width / 2,
                        annotatedScrollBarBounds.Y + annotatedScrollBarBounds.Height / 2);

                    Point annotatedScrollBarOffsetCenter = new Point(
                        annotatedScrollBarCenter.X,
                        annotatedScrollBarCenter.Y + 400);

                    Log.Comment("Dragging touch pointer over AnnotatedScrollBar from its center down 400px");
                    InputHelper.Pan(
                        obj: annotatedScrollBar,
                        start: annotatedScrollBarCenter,
                        end: annotatedScrollBarOffsetCenter,
                        holdDuration: InputHelper.DefaultPanHoldDuration,
                        panAcceleration: 0.0f,
                        dragDuration: 5000,
                        waitForIdle: false);
                    Wait.ForIdle();

                    Log.Comment("Clicking center of btnClearLogs.");
                    btnClearLogs.Click();
                    Wait.ForIdle();

                    LogAndClearTraces();
                    Wait.ForIdle();

                    Log.Comment("Invoking btnGetScrollViewVerticalOffset.");
                    btnGetScrollViewVerticalOffset.Invoke();
                    Wait.ForIdle();

                    double verticalOffset = string.IsNullOrWhiteSpace(scrollViewVerticalOffsetTextBox.Value) ? -1.0 : Convert.ToDouble(scrollViewVerticalOffsetTextBox.Value);
                    Log.Comment("verticalOffset={0}", verticalOffset);
                    Verify.IsGreaterThan(verticalOffset, 100000.0);
                }
            }
        }

        private void TabIntoItemsViewWithoutCurrentItem(bool useShiftTab)
        {
            Log.Comment("Selecting ItemsView interactive tests");

            using (var setup = new TestSetupHelper(new[] { "ItemsView Tests", "navigateToInteractiveTests" }))
            {
                Log.Comment("Retrieving btnBefore");
                Button btnBefore = new Button(FindElement.ById("btnBefore"));
                Verify.IsNotNull(btnBefore, "Verifying that btnBefore was found");

                Log.Comment("Retrieving btnAfter");
                Button btnAfter = new Button(FindElement.ById("btnAfter"));
                Verify.IsNotNull(btnAfter, "Verifying that btnAfter was found");

                Log.Comment("Retrieving cmbLayout");
                ComboBox cmbLayout = new ComboBox(FindElement.ById("cmbLayout"));
                Verify.IsNotNull(cmbLayout, "Verifying that cmbLayout was found");

                Log.Comment("Retrieving cmbItemTemplate");
                ComboBox cmbItemTemplate = new ComboBox(FindElement.ById("cmbItemTemplate"));
                Verify.IsNotNull(cmbItemTemplate, "Verifying that cmbItemTemplate was found");

                Log.Comment("Retrieving cmbItemsSource");
                ComboBox cmbItemsSource = new ComboBox(FindElement.ById("cmbItemsSource"));
                Verify.IsNotNull(cmbItemsSource, "Verifying that cmbItemsSource was found");

                Log.Comment("Retrieving txtScrollViewHorizontalOffset");
                Edit scrollViewHorizontalOffsetTextBox = new Edit(FindElement.ById("txtScrollViewHorizontalOffset"));
                Verify.IsNotNull(scrollViewHorizontalOffsetTextBox, "Verifying that scrollViewHorizontalOffsetTextBox was found");

                Log.Comment("Retrieving txtScrollViewVerticalOffset");
                Edit scrollViewVerticalOffsetTextBox = new Edit(FindElement.ById("txtScrollViewVerticalOffset"));
                Verify.IsNotNull(scrollViewVerticalOffsetTextBox, "Verifying that scrollViewVerticalOffsetTextBox was found");

                Log.Comment("Retrieving txtScrollViewZoomFactor");
                Edit scrollViewZoomFactorTextBox = new Edit(FindElement.ById("txtScrollViewZoomFactor"));
                Verify.IsNotNull(scrollViewZoomFactorTextBox, "Verifying that scrollViewZoomFactorTextBox was found");

                Log.Comment("Changing cmbLayout selection to 'LinedFlowLayout'");
                cmbLayout.SelectItemByName("LinedFlowLayout");
                Log.Comment("Selection is now {0}", cmbLayout.Selection[0].Name);

                Log.Comment("Changing ItemTemplate selection to 'Recipe DataTemplate (Medium ItemContainer)'");
                cmbItemTemplate.SelectItemByName("Recipe DataTemplate (Medium ItemContainer)");
                Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);

                Log.Comment("Changing ItemsSource selection to 'List<Recipe>'");
                cmbItemsSource.SelectItemByName("List<Recipe>");
                Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);

                Log.Comment("Changing txtScrollViewHorizontalOffset text to '0'");
                scrollViewHorizontalOffsetTextBox.SetValueAndWait("0");

                Log.Comment("Changing txtScrollViewVerticalOffset text to '250'");
                scrollViewVerticalOffsetTextBox.SetValueAndWait("250");

                InvokeScrollViewScrollToButton();

                Log.Comment("Changing txtScrollViewZoomFactor text to '2'");
                scrollViewZoomFactorTextBox.SetValueAndWait("2");

                InvokeScrollViewZoomToButton();

                Log.Comment("Retrieving itemsView");
                UIObject itemsViewUIObject = FindElement.ByName("itemsView");
                Verify.IsNotNull(itemsViewUIObject, "Verifying that itemsView was found");

                Log.Comment("Retrieving PART_ScrollView");
                UIObject scrollViewUIObject = itemsViewUIObject.FirstChild;
                Verify.IsNotNull(scrollViewUIObject, "Verifying that PART_ScrollView was found");

                Log.Comment("Retrieving PART_ScrollPresenter as a UIObject");
                UIObject scrollPresenterUIObject = FindElement.ById("PART_ScrollPresenter");
                Verify.IsNotNull(scrollPresenterUIObject, "Verifying that PART_ScrollView.PART_Root.PART_ScrollPresenter was found");

                Log.Comment("Retrieving PART_ScrollPresenter as a ScrollPresenter");
                ScrollPresenter scrollPresenter = new ScrollPresenter(scrollPresenterUIObject);
                Verify.IsNotNull(scrollPresenter, "Verifying that scrollPresenter was found");

                Log.Comment("scrollPresenter.HorizontalScrollPercent={0}", scrollPresenter.HorizontalScrollPercent);
                Log.Comment("scrollPresenter.VerticalScrollPercent={0}", scrollPresenter.VerticalScrollPercent);
                Log.Comment("scrollPresenter.VerticalViewSize={0}", scrollPresenter.VerticalViewSize);

                double horizontalOffset;
                double verticalOffset;
                float zoomFactor;

                GetScrollViewView(out horizontalOffset, out verticalOffset, out zoomFactor);
                Log.Comment("horizontalOffset={0}", horizontalOffset);
                Log.Comment("verticalOffset={0}", verticalOffset);
                Log.Comment("zoomFactor={0}", zoomFactor);

                if (useShiftTab)
                {
                    Log.Comment("Clicking on ButtonAfter.");
                    btnAfter.Click(PointerButtons.Primary);
                    Wait.ForIdle();

                    Log.Comment("Pressing Shift-Tab keys");
                    KeyboardHelper.PressKey(btnAfter, Key.Tab, modifierKey: ModifierKey.Shift, numPresses: 1, useDebugMode: true);
                    Wait.ForIdle();

                    int currentItemIndex = GetCurrentItemIndex();

                    Verify.AreEqual(21, currentItemIndex, "Verifying CurrentItemIndex is 21");
                }
                else
                {
                    Log.Comment("Clicking on ButtonBefore.");
                    btnBefore.Click(PointerButtons.Primary);
                    Wait.ForIdle();

                    Log.Comment("Pressing Tab key");
                    KeyboardHelper.PressKey(btnBefore, Key.Tab, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                    Wait.ForIdle();

                    int currentItemIndex = GetCurrentItemIndex();

                    Verify.AreEqual(15, currentItemIndex, "Verifying CurrentItemIndex is 15");
                }

                ResetItemsView(cmbItemsSource, cmbLayout);

                // Output-debug-string-level "None" is automatically restored when landing back on the ScrollView test page.
            }
        }

        // When provideExtraDesiredAspectRatioItemsInfo is True, the LinedFlowLayout.ItemsInfoRequested event handler provides sizing information for the entire source
        // collection. This is done by checking the chkProvideExtraDesiredAspectRatioItemsInfo CheckBox in the UI.
        // When provideExtraDesiredAspectRatioItemsInfo is False, the handler only provides information for the requested range and the fast path can not be performed. 
        private void SetUpItemsViewForSelectionTests(string selectionMode, bool provideExtraDesiredAspectRatioItemsInfo = false)
        {
            Log.Comment("SetUpItemsViewForSelectionTests");

            if (!provideExtraDesiredAspectRatioItemsInfo)
            {
                Log.Comment("Retrieving chkPageMethods");
                CheckBox chkPageMethods = new CheckBox(FindElement.ById("chkPageMethods"));
                Verify.IsNotNull(chkPageMethods, "Verifying that chkPageMethods was found");
                Verify.AreEqual(ToggleState.Off, chkPageMethods.ToggleState);

                Log.Comment("Toggling chkPageMethods.IsChecked to ToggleState.On");
                chkPageMethods.Toggle();
                Wait.ForIdle();

                Log.Comment("Retrieving chkProvideExtraDesiredAspectRatioItemsInfo");
                CheckBox chkProvideExtraDesiredAspectRatioItemsInfo = new CheckBox(FindElement.ById("chkProvideExtraDesiredAspectRatioItemsInfo"));
                Verify.IsNotNull(chkProvideExtraDesiredAspectRatioItemsInfo, "Verifying that chkProvideExtraDesiredAspectRatioItemsInfo was found");
                Verify.AreEqual(ToggleState.On, chkProvideExtraDesiredAspectRatioItemsInfo.ToggleState);

                Log.Comment("Toggling chkProvideExtraDesiredAspectRatioItemsInfo.IsChecked to ToggleState.Off");
                chkProvideExtraDesiredAspectRatioItemsInfo.Toggle();
                Wait.ForIdle();
            }

            Log.Comment("Retrieving cmbSelectionMode");
            ComboBox cmbSelectionMode = new ComboBox(FindElement.ById("cmbSelectionMode"));
            Verify.IsNotNull(cmbSelectionMode, "Verifying that cmbSelectionMode was found");

            Log.Comment("Changing cmbSelectionMode selection to " + selectionMode);
            cmbSelectionMode.SelectItemByName(selectionMode);
            Log.Comment("Selection is now {0}", cmbSelectionMode.Selection[0].Name);

            Log.Comment("Retrieving cmbLayout");
            ComboBox cmbLayout = new ComboBox(FindElement.ById("cmbLayout"));
            Verify.IsNotNull(cmbLayout, "Verifying that cmbLayout was found");

            Log.Comment("Changing cmbLayout selection to 'LinedFlowLayout'");
            cmbLayout.SelectItemByName("LinedFlowLayout");
            Log.Comment("Selection is now {0}", cmbLayout.Selection[0].Name);

            Log.Comment("Retrieving cmbItemTemplate");
            ComboBox cmbItemTemplate = new ComboBox(FindElement.ById("cmbItemTemplate"));
            Verify.IsNotNull(cmbItemTemplate, "Verifying that cmbItemTemplate was found");

            Log.Comment("Changing cmbItemTemplate selection to 'Lightweight Square ItemContainer (Rectangle + TextBlock)'");
            cmbItemTemplate.SelectItemByName("Lightweight Square ItemContainer (Rectangle + TextBlock)");
            Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);

            Log.Comment("Retrieving cmbItemsSource");
            ComboBox cmbItemsSource = new ComboBox(FindElement.ById("cmbItemsSource"));
            Verify.IsNotNull(cmbItemsSource, "Verifying that cmbItemsSource was found");

            Log.Comment("Changing cmbItemsSource selection to 'List<Recipe>'");
            cmbItemsSource.SelectItemByName("List<Recipe>");
            Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);
        }

        private string GetSelectedItems()
        {
            Log.Comment("Retrieving GetSelectedItems");

            Log.Comment("Retrieving txtSelectedItems");
            Edit txtSelectedItems = new Edit(FindElement.ById("txtSelectedItems"));
            Verify.IsNotNull(txtSelectedItems, "Verifying that txSelectedItems was found");

            Log.Comment("Retrieving btnGetSelectedItems");
            Button btnGetSelectedItems = new Button(FindElement.ById("btnGetSelectedItems"));
            Verify.IsNotNull(btnGetSelectedItems, "Verifying that btnGetSelectedItems was found");

            btnGetSelectedItems.Invoke();
            Wait.ForIdle();

            return txtSelectedItems.Value;
        }

        private void KeyboardNavigationWithLayout(string layout, bool useLateralMoves)
        {
            Log.Comment("Selecting ItemsView tests");

            using (var setup = new TestSetupHelper(new[] { "ItemsView Tests", "navigateToInteractiveTests" }))
            {
                Log.Comment("Retrieving cmbItemTemplate");
                ComboBox cmbItemTemplate = new ComboBox(FindElement.ById("cmbItemTemplate"));
                Verify.IsNotNull(cmbItemTemplate, "Verifying that cmbItemTemplate was found");

                Log.Comment("Retrieving cmbItemsSource");
                ComboBox cmbItemsSource = new ComboBox(FindElement.ById("cmbItemsSource"));
                Verify.IsNotNull(cmbItemsSource, "Verifying that cmbItemsSource was found");

                Log.Comment("Retrieving cmbLayout");
                ComboBox cmbLayout = new ComboBox(FindElement.ById("cmbLayout"));
                Verify.IsNotNull(cmbLayout, "Verifying that cmbLayout was found");

                Log.Comment($"Changing Layout selection to '{layout}'");
                cmbLayout.SelectItemByName(layout);
                Log.Comment("Selection is now {0}", cmbLayout.Selection[0].Name);

                Log.Comment("Changing ItemTemplate selection to 'Recipe DataTemplate (Medium ItemContainer)'");
                cmbItemTemplate.SelectItemByName("Recipe DataTemplate (Medium ItemContainer)");
                Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);

                Log.Comment("Changing ItemsSource selection to 'List<Recipe>'");
                cmbItemsSource.SelectItemByName("List<Recipe>");
                Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);

                Log.Comment("Retrieving itemsView");
                UIObject itemsViewUIObject = FindElement.ByName("itemsView");
                Verify.IsNotNull(itemsViewUIObject, "Verifying that itemsView was found");

                Log.Comment("Clicking on ItemsView to select first item.");
                itemsViewUIObject.Click(PointerButtons.Primary, 40.0, 40.0);
                Wait.ForIdle();
                LogScrollViewInfo(itemsViewUIObject);
                int currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(0, currentItemIndex, "Verifying CurrentItemIndex is 0");

                Log.Comment("Pressing End key");
                KeyboardHelper.PressKey(Key.End);
                Wait.ForIdle();
                LogScrollViewInfo(itemsViewUIObject);
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(49, currentItemIndex, "Verifying CurrentItemIndex is 49");

                Log.Comment("Pressing Right key");
                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();
                LogScrollViewInfo(itemsViewUIObject);
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(49, currentItemIndex, "Verifying CurrentItemIndex is still 49");

                Log.Comment("Pressing Home key");
                KeyboardHelper.PressKey(Key.Home);
                Wait.ForIdle();
                LogScrollViewInfo(itemsViewUIObject);
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(0, currentItemIndex, "Verifying CurrentItemIndex is 0");

                if (useLateralMoves)
                {
                    Log.Comment("Pressing Right key");
                    KeyboardHelper.PressKey(Key.Right);
                    Wait.ForIdle();
                    LogScrollViewInfo(itemsViewUIObject);
                    currentItemIndex = GetCurrentItemIndex();
                    Verify.AreEqual(1, currentItemIndex, "Verifying CurrentItemIndex is 1");
                }

                Log.Comment("Pressing Down key");
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                LogScrollViewInfo(itemsViewUIObject);
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreNotEqual(0, currentItemIndex, "Verifying CurrentItemIndex is not 0");

                if (useLateralMoves)
                {
                    Log.Comment("Pressing Left key");
                    KeyboardHelper.PressKey(Key.Left);
                    Wait.ForIdle();
                    LogScrollViewInfo(itemsViewUIObject);
                    currentItemIndex = GetCurrentItemIndex();
                    Verify.AreNotEqual(0, currentItemIndex, "Verifying CurrentItemIndex is not 0");
                    Verify.AreNotEqual(1, currentItemIndex, "Verifying CurrentItemIndex is not 1");
                }

                Log.Comment("Pressing Up key");
                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                LogScrollViewInfo(itemsViewUIObject);
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(0, currentItemIndex, "Verifying CurrentItemIndex is 0");

                Log.Comment("Pressing PageDown key");
                KeyboardHelper.PressKey(Key.PageDown);
                Wait.ForIdle();
                LogScrollViewInfo(itemsViewUIObject);
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreNotEqual(0, currentItemIndex, "Verifying CurrentItemIndex is not 0");

                Log.Comment("Pressing PageDown key");
                KeyboardHelper.PressKey(Key.PageDown);
                Wait.ForIdle();
                LogScrollViewInfo(itemsViewUIObject);
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreNotEqual(0, currentItemIndex, "Verifying CurrentItemIndex is not 0");

                Log.Comment("Pressing PageUp key");
                KeyboardHelper.PressKey(Key.PageUp);
                Wait.ForIdle();
                LogScrollViewInfo(itemsViewUIObject);
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreNotEqual(0, currentItemIndex, "Verifying CurrentItemIndex is not 0");

                Log.Comment("Pressing PageUp key");
                KeyboardHelper.PressKey(Key.PageUp);
                Wait.ForIdle();
                LogScrollViewInfo(itemsViewUIObject);
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(0, currentItemIndex, "Verifying CurrentItemIndex is 0");

                ResetItemsView(cmbItemsSource, cmbLayout);
                // Output-debug-string-level "None" is automatically restored when landing back on the ScrollView test page.
            }
        }

        private void KeyboardNavigationWithFastKeystrokes(string layout, Key key)
        {
            Log.Comment("Selecting ItemsView tests");

            ICollection<string> outputDebugStringComponentTypes = new[] { "ScrollView", "ItemsView" };

            using (var setup = new ItemsViewTestSetupHelper(
                testNames: new[] { "ItemsView Tests", "navigateToInteractiveTests" },
                options: null,
                shouldRestrictInnerFrameSize: false,
                outputDebugStringLevel: "Verbose",
                outputDebugStringComponentTypes: outputDebugStringComponentTypes))
            {
                using (var loggingHelper = new LoggingHelper(this, outputDebugStringComponentTypes))
                {
                    Log.Comment("Retrieving cmbItemTemplate");
                    ComboBox cmbItemTemplate = new ComboBox(FindElement.ById("cmbItemTemplate"));
                    Verify.IsNotNull(cmbItemTemplate, "Verifying that cmbItemTemplate was found");

                    Log.Comment("Retrieving cmbItemsSource");
                    ComboBox cmbItemsSource = new ComboBox(FindElement.ById("cmbItemsSource"));
                    Verify.IsNotNull(cmbItemsSource, "Verifying that cmbItemsSource was found");

                    Log.Comment("Retrieving cmbLayout");
                    ComboBox cmbLayout = new ComboBox(FindElement.ById("cmbLayout"));
                    Verify.IsNotNull(cmbLayout, "Verifying that cmbLayout was found");

                    Log.Comment($"Changing Layout selection to '{layout}'");
                    cmbLayout.SelectItemByName(layout);
                    Log.Comment("Selection is now {0}", cmbLayout.Selection[0].Name);

                    if (layout == "LinedFlowLayout")
                    {
                        Log.Comment("Changing ItemTemplate selection to 'Recipe DataTemplate (Square ItemContainer)'");
                        cmbItemTemplate.SelectItemByName("Recipe DataTemplate (Square ItemContainer)");
                    }
                    else
                    {
                        Log.Comment("Changing ItemTemplate selection to 'Recipe DataTemplate (Medium ItemContainer)'");
                        cmbItemTemplate.SelectItemByName("Recipe DataTemplate (Medium ItemContainer)");
                    }
                    Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);

                    Log.Comment("Changing ItemsSource selection to 'Large List<Recipe>'");
                    cmbItemsSource.SelectItemByName("Large List<Recipe>");
                    Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);

                    if (layout == "LinedFlowLayout")
                    {
                        Log.Comment("Retrieving chkHandleItemsInfoRequested");
                        CheckBox chkHandleItemsInfoRequested = new CheckBox(FindElement.ById("chkHandleItemsInfoRequested"));
                        Verify.IsNotNull(chkHandleItemsInfoRequested, "Verifying that chkHandleItemsInfoRequested was found");

                        if (ToggleState.Off == chkHandleItemsInfoRequested.ToggleState)
                        {
                            Log.Comment("Toggling chkHandleItemsInfoRequested.IsChecked to ToggleState.On");
                            chkHandleItemsInfoRequested.Toggle();
                            Wait.ForIdle();
                            Verify.AreEqual(ToggleState.On, chkHandleItemsInfoRequested.ToggleState);
                        }

                        Log.Comment("Retrieving chkUseFastPath");
                        CheckBox chkUseFastPath = new CheckBox(FindElement.ById("chkUseFastPath"));
                        Verify.IsNotNull(chkUseFastPath, "Verifying that chkUseFastPath was found");

                        if (ToggleState.Off == chkUseFastPath.ToggleState)
                        {
                            Log.Comment("Toggling chkUseFastPath.IsChecked to ToggleState.On");
                            chkUseFastPath.Toggle();
                            Wait.ForIdle();
                            Verify.AreEqual(ToggleState.On, chkUseFastPath.ToggleState);
                        }
                    }

                    Log.Comment("Retrieving itemsView");
                    UIObject itemsViewUIObject = FindElement.ByName("itemsView");
                    Verify.IsNotNull(itemsViewUIObject, "Verifying that itemsView was found");

                    Log.Comment("Clicking on ItemsView to select first item.");
                    itemsViewUIObject.Click(PointerButtons.Primary, 40.0, 40.0);
                    Wait.ForIdle();
                    LogScrollViewInfo(itemsViewUIObject);
                    int currentItemIndex = GetCurrentItemIndex();
                    Verify.AreEqual(0, currentItemIndex, "Verifying CurrentItemIndex is 0");

                    if (key == Key.Up || key == Key.PageUp)
                    {
                        Log.Comment("Pressing End key");
                        KeyboardHelper.PressKey(Key.End);
                        Wait.ForIdle();
                        LogScrollViewInfo(itemsViewUIObject);
                        currentItemIndex = GetCurrentItemIndex();
                        Verify.AreEqual(999, currentItemIndex, "Verifying CurrentItemIndex is 999");
                    }

                    uint numPresses = (key == Key.Up || key == Key.Down) ? 8u : 4u;

                    Log.Comment("Pressing repeated key " + numPresses + " times");
                    KeyboardHelper.PressKey(key, ModifierKey.None, numPresses);

                    Wait.ForIdle();
                    currentItemIndex = GetCurrentItemIndex();

                    int expectedCurrentItemIndex = 0;

                    if (layout == "LinedFlowLayout")
                    {
                        switch (key)
                        {
                            case Key.Down:
                                expectedCurrentItemIndex = 48;
                                break;
                            case Key.Up:
                                expectedCurrentItemIndex = 951;
                                break;
                            case Key.PageDown:
                                expectedCurrentItemIndex = 138;
                                break;
                            case Key.PageUp:
                                expectedCurrentItemIndex = 861;
                                break;
                        }
                    }
                    else
                    {
                        // layout == "StackLayout"
                        switch (key)
                        {
                            case Key.Down:
                                expectedCurrentItemIndex = 8;
                                break;
                            case Key.Up:
                                expectedCurrentItemIndex = 991;
                                break;
                            case Key.PageDown:
                                expectedCurrentItemIndex = 19;
                                break;
                            case Key.PageUp:
                                expectedCurrentItemIndex = 980;
                                break;
                        }
                    }

                    if (currentItemIndex != expectedCurrentItemIndex)
                    {
                        Log.Warning($"Unexpected current element index {currentItemIndex} instead of {expectedCurrentItemIndex}");
                        LogAndClearTraces();
                        Wait.ForIdle();
                    }

                    if (layout == "LinedFlowLayout")
                    {
                        switch (key)
                        {
                            case Key.Down:
                                LogScrollViewInfo(itemsViewUIObject, expectedVerticalOffset: 272.0);
                                break;
                            case Key.Up:
                                LogScrollViewInfo(itemsViewUIObject, expectedVerticalOffset: 13252.0);
                                break;
                            case Key.PageDown:
                                LogScrollViewInfo(itemsViewUIObject, expectedVerticalOffset: 1532.0);
                                break;
                            case Key.PageUp:
                                LogScrollViewInfo(itemsViewUIObject, expectedVerticalOffset: 11992.0);
                                break;
                        }
                    }
                    else
                    {
                        // layout == "StackLayout"
                        switch (key)
                        {
                            case Key.Down:
                                LogScrollViewInfo(itemsViewUIObject, expectedVerticalOffset: 384.0);
                                break;
                            case Key.Up:
                                LogScrollViewInfo(itemsViewUIObject, expectedVerticalOffset: 95116.0);
                                break;
                            case Key.PageDown:
                                LogScrollViewInfo(itemsViewUIObject, expectedVerticalOffset: 1440.0);
                                break;
                            case Key.PageUp:
                                LogScrollViewInfo(itemsViewUIObject, expectedVerticalOffset: 94060.0);
                                break;
                        }
                    }

                    Verify.AreEqual(expectedCurrentItemIndex, currentItemIndex, "Verifying CurrentItemIndex.");

                    ResetItemsView(cmbItemsSource, cmbLayout);
                }
            }
        }

        private void VerifyItemsViewItemInvokedEvent(bool useItemContainer, bool isItemInvokedEnabled, string selectionMode)
        {
            Log.Comment("Selecting ItemsView tests");

            using (var setup = new TestSetupHelper(new[] { "ItemsView Tests", "navigateToInteractiveTests" }))
            {
                Log.Comment("Retrieving chkIsItemInvokedEnabled");
                CheckBox chkIsItemInvokedEnabled = new CheckBox(FindElement.ById("chkIsItemInvokedEnabled"));
                Verify.IsNotNull(chkIsItemInvokedEnabled, "Verifying that chkIsItemInvokedEnabled was found");

                Log.Comment("Retrieving cmbSelectionMode");
                ComboBox cmbSelectionMode = new ComboBox(FindElement.ById("cmbSelectionMode"));
                Verify.IsNotNull(cmbSelectionMode, "Verifying that cmbSelectionMode was found");

                Log.Comment("Retrieving cmbItemTemplate");
                ComboBox cmbItemTemplate = new ComboBox(FindElement.ById("cmbItemTemplate"));
                Verify.IsNotNull(cmbItemTemplate, "Verifying that cmbItemTemplate was found");

                Log.Comment("Retrieving cmbItemsSource");
                ComboBox cmbItemsSource = new ComboBox(FindElement.ById("cmbItemsSource"));
                Verify.IsNotNull(cmbItemsSource, "Verifying that cmbItemsSource was found");

                Log.Comment("Retrieving cmbLayout");
                ComboBox cmbLayout = new ComboBox(FindElement.ById("cmbLayout"));
                Verify.IsNotNull(cmbLayout, "Verifying that cmbLayout was found");

                Log.Comment("Changing chkIsItemInvokedEnabled to " + isItemInvokedEnabled);
                if (isItemInvokedEnabled == true) { chkIsItemInvokedEnabled.Check(); } else { chkIsItemInvokedEnabled.Uncheck();  };
                Log.Comment("IsChecked ToggleState is now {0}", chkIsItemInvokedEnabled.ToggleState);

                Log.Comment("Changing SelectionMode selection to " + selectionMode);
                cmbSelectionMode.SelectItemByName(selectionMode);
                Log.Comment("Selection is now {0}", cmbSelectionMode.Selection[0].Name);

                Log.Comment("Changing Layout selection to 'UniformGridLayout'");
                cmbLayout.SelectItemByName("UniformGridLayout");
                Log.Comment("Selection is now {0}", cmbLayout.Selection[0].Name);

                if (useItemContainer)
                {
                    Log.Comment("Changing ItemTemplate selection to 'Recipe DataTemplate (Medium ItemContainer)'");
                    cmbItemTemplate.SelectItemByName("Recipe DataTemplate (Medium ItemContainer)");
                    Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);
                }
                else
                {
                    Log.Comment("Changing ItemTemplate selection to 'Recipe DataTemplate (Small ContentControl)'");
                    cmbItemTemplate.SelectItemByName("Recipe DataTemplate (Small ContentControl)");
                    Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);
                }

                Log.Comment("Changing ItemsSource selection to 'List<Recipe>'");
                cmbItemsSource.SelectItemByName("List<Recipe>");
                Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);

                Log.Comment("Retrieving itemsView");
                UIObject itemsViewUIObject = FindElement.ByName("itemsView");
                Verify.IsNotNull(itemsViewUIObject, "Verifying that itemsView was found");

                Log.Comment("Clicking on ItemsView to attempt to invoke first item.");
                itemsViewUIObject.Click(PointerButtons.Primary, useItemContainer ? 40.0 : 20.0, useItemContainer ? 40.0 : 20.0);
                Wait.ForIdle();

                int itemInvokedCount = ItemInvokedCount();

                Verify.AreEqual((isItemInvokedEnabled == true && selectionMode == "None") ? 1 : 0, itemInvokedCount, "Verifying itemInvoked occurrences");

                Log.Comment("Double-clicking on ItemsView to attempt to invoke first item.");
                itemsViewUIObject.DoubleClick(PointerButtons.Primary, useItemContainer ? 40.0 : 20.0, useItemContainer ? 40.0 : 20.0);
                Wait.ForIdle();

                itemInvokedCount = ItemInvokedCount();

                Verify.AreEqual((isItemInvokedEnabled == true && selectionMode != "None") ? 1 : 0, itemInvokedCount, "Verifying itemInvoked occurrences");

                ResetItemsView(cmbItemsSource, cmbLayout);

                // Output-debug-string-level "None" is automatically restored when landing back on the ScrollView test page.
            }
        }

        private void TabNavigationThroughItemsView(bool useItemContainer, string selectionMode)
        {
            Log.Comment("Selecting ItemsView tests");

            using (var setup = new TestSetupHelper(new[] { "ItemsView Tests", "navigateToInteractiveTests" }))
            {
                Log.Comment("Retrieving cmbSelectionMode");
                ComboBox cmbSelectionMode = new ComboBox(FindElement.ById("cmbSelectionMode"));
                Verify.IsNotNull(cmbSelectionMode, "Verifying that cmbSelectionMode was found");

                Log.Comment("Retrieving cmbItemTemplate");
                ComboBox cmbItemTemplate = new ComboBox(FindElement.ById("cmbItemTemplate"));
                Verify.IsNotNull(cmbItemTemplate, "Verifying that cmbItemTemplate was found");

                Log.Comment("Retrieving cmbItemsSource");
                ComboBox cmbItemsSource = new ComboBox(FindElement.ById("cmbItemsSource"));
                Verify.IsNotNull(cmbItemsSource, "Verifying that cmbItemsSource was found");

                Log.Comment("Retrieving cmbLayout");
                ComboBox cmbLayout = new ComboBox(FindElement.ById("cmbLayout"));
                Verify.IsNotNull(cmbLayout, "Verifying that cmbLayout was found");

                Log.Comment("Retrieving btnBefore");
                Button btnBefore = new Button(FindElement.ById("btnBefore"));
                Verify.IsNotNull(btnBefore, "Verifying that btnBefore was found");

                Log.Comment("Retrieving btnAfter");
                Button btnAfter = new Button(FindElement.ById("btnAfter"));
                Verify.IsNotNull(btnAfter, "Verifying that btnAfter was found");

                Log.Comment("Changing SelectionMode selection to " + selectionMode);
                cmbSelectionMode.SelectItemByName(selectionMode);
                Log.Comment("Selection is now {0}", cmbSelectionMode.Selection[0].Name);

                Log.Comment("Changing Layout selection to 'UniformGridLayout'");
                cmbLayout.SelectItemByName("UniformGridLayout");
                Log.Comment("Selection is now {0}", cmbLayout.Selection[0].Name);

                if (useItemContainer)
                {
                    Log.Comment("Changing ItemTemplate selection to 'Recipe DataTemplate (Medium ItemContainer)'");
                    cmbItemTemplate.SelectItemByName("Recipe DataTemplate (Medium ItemContainer)");
                    Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);
                }
                else
                {
                    Log.Comment("Changing ItemTemplate selection to 'Recipe DataTemplate (Small ContentControl)'");
                    cmbItemTemplate.SelectItemByName("Recipe DataTemplate (Small ContentControl)");
                    Log.Comment("Selection is now {0}", cmbItemTemplate.Selection[0].Name);
                }

                Log.Comment("Changing ItemsSource selection to 'List<Recipe>'");
                cmbItemsSource.SelectItemByName("List<Recipe>");
                Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);

                Log.Comment("Retrieving itemsView");
                UIObject itemsViewUIObject = FindElement.ByName("itemsView");
                Verify.IsNotNull(itemsViewUIObject, "Verifying that itemsView was found");

                Log.Comment("Clicking on ButtonBefore.");
                btnBefore.Click(PointerButtons.Primary);
                Wait.ForIdle();

                Log.Comment("Pressing Tab key");
                KeyboardHelper.PressKey(btnBefore, Key.Tab, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                Wait.ForIdle();

                int currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(0, currentItemIndex, "Verifying CurrentItemIndex is 0");

                Log.Comment("Pressing Right key");
                KeyboardHelper.PressKey(itemsViewUIObject, Key.Right, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                Wait.ForIdle();
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(1, currentItemIndex, "Verifying CurrentItemIndex is 1");

                Log.Comment("Pressing Tab key");
                KeyboardHelper.PressKey(itemsViewUIObject, Key.Tab, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                Wait.ForIdle();

                Verify.IsTrue(btnAfter.HasKeyboardFocus, "Verifying AfterButton has keyboard focus");
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(1, currentItemIndex, "Verifying CurrentItemIndex is still 1");

                Log.Comment("Pressing Shift-Tab key");
                KeyboardHelper.PressKey(btnAfter, Key.Tab, modifierKey: ModifierKey.Shift, numPresses: 1, useDebugMode: true);
                Wait.ForIdle();

                currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(1, currentItemIndex, "Verifying CurrentItemIndex is still 1");

                Log.Comment("Pressing Shift-Tab key");
                KeyboardHelper.PressKey(itemsViewUIObject, Key.Tab, modifierKey: ModifierKey.Shift, numPresses: 1, useDebugMode: true);
                Wait.ForIdle();

                Verify.IsTrue(btnBefore.HasKeyboardFocus, "Verifying BeforeButton has keyboard focus");
                currentItemIndex = GetCurrentItemIndex();
                Verify.AreEqual(1, currentItemIndex, "Verifying CurrentItemIndex is still 1");

                ResetItemsView(cmbItemsSource, cmbLayout);

                // Output-debug-string-level "None" is automatically restored when landing back on the ScrollView test page.
            }
        }

        private void InvokeScrollViewScrollToButton()
        {
            Log.Comment("Retrieving btnScrollViewScrollTo");
            UIObject scrollViewScrollToUIObject = FindElement.ById("btnScrollViewScrollTo");
            Verify.IsNotNull(scrollViewScrollToUIObject, "Verifying that btnScrollViewScrollTo Button was found");

            Button scrollViewScrollToButton = new Button(scrollViewScrollToUIObject);

            Log.Comment("Invoking scrollViewScrollToButton");
            scrollViewScrollToButton.Invoke();
            WaitForEditValue("txtStatus" /*editName*/, "ScrollView scrolled" /*editValue*/, 4.0 /*secondsTimeout*/, false /*throwOnError*/);
        }

        private void InvokeScrollViewZoomToButton()
        {
            Log.Comment("Retrieving btnScrollViewZoomTo");
            UIObject scrollViewZoomToUIObject = FindElement.ById("btnScrollViewZoomTo");
            Verify.IsNotNull(scrollViewZoomToUIObject, "Verifying that btnScrollViewZoomTo Button was found");

            Button scrollViewZoomToButton = new Button(scrollViewZoomToUIObject);

            Log.Comment("Invoking scrollViewZoomToButton");
            scrollViewZoomToButton.Invoke();
            WaitForEditValue("txtStatus" /*editName*/, "ScrollView zoomed" /*editValue*/, 4.0 /*secondsTimeout*/, false /*throwOnError*/);
        }

        private void TapResetScrollViewButton()
        {
            Log.Comment("Retrieving btnResetScrollView");
            UIObject resetScrollViewUIObject = FindElement.ById("btnResetScrollView");
            Verify.IsNotNull(resetScrollViewUIObject, "Verifying that btnResetScrollView Button was found");

            Button resetScrollViewButton = new Button(resetScrollViewUIObject);
            InputHelper.Tap(resetScrollViewButton);
            if (!WaitForEditValue("txtStatus" /*editName*/, "ScrollView reset" /*editValue*/, 4.0 /*secondsTimeout*/, false /*throwOnError*/))
            {
                InputHelper.Tap(resetScrollViewButton);
                WaitForEditValue("txtStatus" /*editName*/, "ScrollView reset" /*editValue*/, 4.0 /*secondsTimeout*/, false /*throwOnError*/);
            }
        }

        private void ResetItemsView(ComboBox cmbItemsSource, ComboBox cmbLayout)
        {
            if (cmbItemsSource == null)
            {
                Log.Comment("Retrieving cmbItemsSource");
                cmbItemsSource = new ComboBox(FindElement.ById("cmbItemsSource"));
            }

            Verify.IsNotNull(cmbItemsSource);

            Log.Comment("Resetting ItemsSource");
            cmbItemsSource.SelectItemByName("null");
            Log.Comment("Selection is now {0}", cmbItemsSource.Selection[0].Name);

            Wait.ForIdle();

            if (cmbLayout == null)
            {
                Log.Comment("Retrieving cmbLayout");
                cmbLayout = new ComboBox(FindElement.ById("cmbLayout"));
            }

            Verify.IsNotNull(cmbLayout);

            Log.Comment("Resetting Layout");
            cmbLayout.SelectItemByName("null");
            Log.Comment("Selection is now {0}", cmbLayout.Selection[0].Name);
                
            Wait.ForIdle();
        }

        private int GetCurrentItemIndex()
        {
            Log.Comment("Retrieving btnGetCurrentItemIndex");
            UIObject getCurrentItemIndexUIObject = FindElement.ById("btnGetCurrentItemIndex");
            Button getCurrentItemIndexButton = new Button(getCurrentItemIndexUIObject);
            Verify.IsNotNull(getCurrentItemIndexButton, "Verifying that btnGetCurrentItemIndex Button was found");

            getCurrentItemIndexButton.Invoke();
            Wait.ForIdle();

            Log.Comment("Retrieving txtCurrentItemIndex");
            UIObject currentItemIndexUIObject = FindElement.ById("txtCurrentItemIndex");
            Edit currentItemIndexTextBox = new Edit(currentItemIndexUIObject);
            Verify.IsNotNull(currentItemIndexTextBox, "Verifying that txtCurrentItemIndex TextBox was found");

            Log.Comment($"CurrentItemIndex={currentItemIndexTextBox.Value}");

            return int.Parse(currentItemIndexTextBox.Value);
        }

        private void GetScrollViewView(out double horizontalOffset, out double verticalOffset, out float zoomFactor)
        {
            Log.Comment("Retrieving ScrollView view.");
            horizontalOffset = -1.0;
            verticalOffset = -1.0;
            zoomFactor = -1.0f;

            Log.Comment("Retrieving txtScrollViewHorizontalOffset.");
            UIObject viewUIObject = FindElement.ById("txtScrollViewHorizontalOffset");
            if (viewUIObject == null)
            {
                Log.Comment("txtScrollViewHorizontalOffset not found.");
            }
            else
            {
                Log.Comment("txtScrollViewHorizontalOffset found.");
                Edit viewTextBox = new Edit(viewUIObject);
                Log.Comment("Current HorizontalOffset: " + viewTextBox.Value);
                horizontalOffset = string.IsNullOrWhiteSpace(viewTextBox.Value) ? double.NaN : Convert.ToDouble(viewTextBox.Value);
            }

            Log.Comment("Retrieving txtScrollViewVerticalOffset.");
            viewUIObject = FindElement.ById("txtScrollViewVerticalOffset");
            if (viewUIObject == null)
            {
                Log.Comment("txtScrollViewVerticalOffset not found.");
            }
            else
            {
                Log.Comment("txtScrollViewVerticalOffset found.");
                Edit viewTextBox = new Edit(viewUIObject);
                Log.Comment("Current VerticalOffset: " + viewTextBox.Value);
                verticalOffset = string.IsNullOrWhiteSpace(viewTextBox.Value) ? double.NaN : Convert.ToDouble(viewTextBox.Value);
            }

            Log.Comment("Retrieving txtScrollViewZoomFactor.");
            viewUIObject = FindElement.ById("txtScrollViewZoomFactor");
            if (viewUIObject == null)
            {
                Log.Comment("txtScrollViewZoomFactor not found.");
            }
            else
            {
                Log.Comment("txtScrollViewZoomFactor found.");
                Edit viewTextBox = new Edit(viewUIObject);
                Log.Comment("Current ZoomFactor: " + viewTextBox.Value);
                zoomFactor = string.IsNullOrWhiteSpace(viewTextBox.Value) ? float.NaN : Convert.ToSingle(viewTextBox.Value);
            }
        }

        private void PrepareForScrollViewManipulationStart(string stateTextBoxName = "txtScrollViewState")
        {
            UIObject scrollViewStateUIObject = FindElement.ById(stateTextBoxName);
            Edit scrollViewStateTextBox = new Edit(scrollViewStateUIObject);
            Log.Comment("Pre-manipulation ScrollViewState: " + scrollViewStateTextBox.Value);
            Wait.ForIdle();
        }

        private void LogScrollViewInfo(UIObject itemsViewUIObject, double? expectedVerticalOffset = null, bool? getScrollViewView = true)
        {
            Log.Comment("Retrieving PART_ScrollView");
            UIObject scrollViewUIObject = itemsViewUIObject.FirstChild;
            Verify.IsNotNull(scrollViewUIObject, "Verifying that PART_ScrollView was found");

            Log.Comment("Retrieving PART_ScrollPresenter as a UIObject");
            UIObject scrollPresenterUIObject = FindElement.ById("PART_ScrollPresenter");
            Verify.IsNotNull(scrollPresenterUIObject, "Verifying that PART_ScrollView.PART_Root.PART_ScrollPresenter was found");

            Log.Comment("Retrieving PART_ScrollPresenter as a ScrollPresenter");
            ScrollPresenter scrollPresenter = new ScrollPresenter(scrollPresenterUIObject);
            Verify.IsNotNull(scrollPresenter, "Verifying that scrollPresenter was found");

            Log.Comment("scrollPresenter.HorizontalScrollPercent={0}", scrollPresenter.HorizontalScrollPercent);
            Log.Comment("scrollPresenter.VerticalScrollPercent={0}", scrollPresenter.VerticalScrollPercent);
            Log.Comment("scrollPresenter.VerticalViewSize={0}", scrollPresenter.VerticalViewSize);
            Log.Comment("scrollPresenter.BoundingRectangle.Height={0}", scrollPresenter.BoundingRectangle.Height);

            if (getScrollViewView == true)
            {
                int attempts = 0;
                double verticalOffsetDelta = 0.0;
                bool verticalOffsetDeltaTooLarge = false;

                do
                {
                    Wait.ForIdle();

                    double horizontalOffset;
                    double verticalOffset;
                    float zoomFactor;

                    GetScrollViewView(out horizontalOffset, out verticalOffset, out zoomFactor);
                    Log.Comment("horizontalOffset={0}", horizontalOffset);
                    Log.Comment("verticalOffset={0}", verticalOffset);
                    Log.Comment("zoomFactor={0}", zoomFactor);

                    if (expectedVerticalOffset != null)
                    {
                        attempts++;
                        verticalOffsetDelta = Math.Abs((double)expectedVerticalOffset - verticalOffset);
                        verticalOffsetDeltaTooLarge = verticalOffsetDelta > 0.5;
                    }
                }
                while (verticalOffsetDeltaTooLarge && attempts <= 3);

                if (expectedVerticalOffset != null)
                {
                    Verify.IsLessThanOrEqual(verticalOffsetDelta, 0.5, "Verifying verticalOffset is close to " + expectedVerticalOffset);
                }
            }
        }

        private void LogEditValue(string editName)
        {
            Edit edit = new Edit(FindElement.ById(editName));
            Verify.IsNotNull(edit);
            LogEditValue(editName, edit);
        }

        private void LogEditValue(string editName, Edit edit)
        {
            Log.Comment("Current value for " + editName + ": " + edit.Value);
        }

        private void WaitForScrollViewManipulationEnd(string scrollViewName, string stateTextBoxName = "txtScrollViewState")
        {
            WaitForManipulationEnd(scrollViewName, stateTextBoxName);
        }

        private bool WaitForManipulationEnd(string elementName, string stateTextBoxName, bool throwOnError = true)
        {
            UIObject elementStateUIObject = FindElement.ById(stateTextBoxName);
            Edit elementStateTextBox = new Edit(elementStateUIObject);
            Log.Comment("Current State: " + elementStateTextBox.Value);
            if (elementStateTextBox.Value != "Idle")
            {
                using (var waiter = new ValueChangedEventWaiter(elementStateTextBox, "Idle"))
                {
                    int loops = 0;

                    Log.Comment("Waiting for " + elementName + "'s manipulation end.");
                    while (true)
                    {
                        bool success = waiter.TryWait(TimeSpan.FromMilliseconds(250));

                        Log.Comment("Current State: " + elementStateTextBox.Value);

                        if (success)
                        {
                            Log.Comment("Wait succeeded");
                            break;
                        }
                        else
                        {
                            if (elementStateTextBox.Value == "Idle")
                            {
                                Log.Warning("Wait failed but TextBox contains expected text");
                                LogAndClearTraces();
                                break;
                            }
                            else if (loops < 20)
                            {
                                loops++;
                                waiter.Reset();
                            }
                            else
                            {
                                if (throwOnError)
                                {
                                    Log.Error("Wait for manipulation end failed");
                                    LogAndClearTraces();
                                    throw new WaiterException();
                                }
                                else
                                {
                                    Log.Warning("Wait for manipulation end failed");
                                    LogAndClearTraces();
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
            return true;
        }

        private bool WaitForEditValue(string editName, string editValue, double secondsTimeout = 2.0, bool throwOnError = true)
        {
            Edit edit = new Edit(FindElement.ById(editName));
            Verify.IsNotNull(edit);
            LogEditValue(editName, edit);
            if (edit.Value != editValue)
            {
                using (var waiter = new ValueChangedEventWaiter(edit, editValue))
                {
                    Log.Comment("Waiting for " + editName + " to be set to " + editValue);

                    bool success = waiter.TryWait(TimeSpan.FromSeconds(secondsTimeout));
                    Log.Comment("Current value for " + editName + ": " + edit.Value);

                    if (success)
                    {
                        Log.Comment("Wait succeeded");
                    }
                    else
                    {
                        if (edit.Value == editValue)
                        {
                            Log.Warning("Wait failed but TextBox contains expected Text");
                            LogAndClearTraces();
                        }
                        else
                        {
                            if (throwOnError)
                            {
                                Log.Error("Wait for edit value failed");
                                LogAndClearTraces();
                                throw new WaiterException();
                            }
                            else
                            {
                                Log.Warning("Wait for edit value failed");
                                LogAndClearTraces();
                                return false;
                            }
                        }
                    }
                }
            }
            return true;
        }

        private void WaitForBoxChecked(string checkBoxName)
        {
            Log.Comment($"Waiting for checkbox {checkBoxName} checked...");
            LogEditValue("txtStatus");

            UIObject checkBoxUIObject = FindElement.ById(checkBoxName);
            Verify.IsNotNull(checkBoxUIObject);
            CheckBox checkBox = new CheckBox(checkBoxUIObject);
            Verify.IsNotNull(checkBox);

            if (checkBox.ToggleState == ToggleState.On)
            {
                Log.Comment("CheckBox already checked.");
            }
            else
            {
                checkBox.GetToggledWaiter().TryWait();
                if (checkBox.ToggleState != ToggleState.On)
                {
                    Log.Warning($"{checkBoxName} was not checked.");
                    throw new WaiterException();
                }
                Log.Comment("CheckBox checked.");
            }
        }

        private int ItemInvokedCount()
        {
            int itemInvokedCount = 0;

            Log.Comment("Counting ItemsView.ItemInvoked events in full log:");

            UpdateTraces();

            UIObject fullLogUIObject = FindElement.ById("cmbFullLog");
            Verify.IsNotNull(fullLogUIObject);
            ComboBox cmbFullLog = new ComboBox(fullLogUIObject);
            Verify.IsNotNull(cmbFullLog);

            foreach (ComboBoxItem item in cmbFullLog.AllItems)
            {
                if (item.Name.Contains("ItemsView.ItemInvoked"))
                {
                    itemInvokedCount++;
                }
            }

            Log.Comment($"Log Entries Count={cmbFullLog.AllItems.Count}.");
            Log.Comment($"ViewChanged Count={itemInvokedCount}.");

            ClearTraces();

            return itemInvokedCount;
        }

        private void UpdateTraces()
        {
            Log.Comment("Updating full log:");
            LogEditValue("txtStatus");

            TextInput.SendText("g");
            WaitForBoxChecked("chkLogUpdated");
        }

        private void LogTraces()
        {
            UpdateTraces();

            Log.Comment("Reading full log:");

            UIObject fullLogUIObject = FindElement.ById("cmbFullLog");
            Verify.IsNotNull(fullLogUIObject);
            ComboBox cmbFullLog = new ComboBox(fullLogUIObject);
            Verify.IsNotNull(cmbFullLog);

            foreach (ComboBoxItem item in cmbFullLog.AllItems)
            {
                Log.Comment(item.Name);
            }
        }

        private void ClearTraces()
        {
            Log.Comment("Clearing full log.");
            LogEditValue("txtStatus");

            TextInput.SendText("c");
            WaitForBoxChecked("chkLogCleared");
        }

        private void LogAndClearTraces()
        {
            LogTraces();
            ClearTraces();
        }

        private void SetCheckBoxValue(string checkBoxName, bool checkBoxValue)
        {
            Log.Comment("Retrieving " + checkBoxName);
            CheckBox checkBox = new CheckBox(FindElement.ById(checkBoxName));
            if (checkBox == null)
            {
                Log.Warning(checkBoxName + " was not found");
            }
            else if (checkBoxValue && checkBox.ToggleState != ToggleState.On ||
                    !checkBoxValue && checkBox.ToggleState != ToggleState.Off)
            {
                Log.Comment(checkBoxName + " was found. Toggling its IsChecked to " + checkBoxValue);
                checkBox.Toggle();
                Wait.ForIdle();
            }
        }

        private class ItemsViewTestSetupHelper : TestSetupHelper
        {
            public ItemsViewTestSetupHelper(
                ICollection<string> testNames,
                TestSetupHelperOptions options,
                bool shouldRestrictInnerFrameSize,
                string outputDebugStringLevel,
                ICollection<string> outputDebugStringComponentTypes)
                : base(testNames, options, shouldRestrictInnerFrameSize, outputDebugStringLevel, outputDebugStringComponentTypes)
            {
            }

            protected override void TestPageLoaded(string testName)
            {
                Log.Comment("ItemsViewTestSetupHelper - " + testName + " loaded.");

                if (testName == "ItemsView Tests")
                {
                    SetCheckBoxValue(checkBoxName: "chkItemContainer", checkBoxValue: false);
                    SetCheckBoxValue(checkBoxName: "chkItemsView", checkBoxValue: false);
                    SetCheckBoxValue(checkBoxName: "chkItemsRepeater", checkBoxValue: false);
                    SetCheckBoxValue(checkBoxName: "chkItemContainer", checkBoxValue: false);
                    SetCheckBoxValue(checkBoxName: "chkLinedFlowLayout", checkBoxValue: false);
                    SetCheckBoxValue(checkBoxName: "chkScrollView", checkBoxValue: false);
                    SetCheckBoxValue(checkBoxName: "chkScrollPresenter", checkBoxValue: false);
                    SetCheckBoxValue(checkBoxName: "chkAnnotatedScrollBar", checkBoxValue: false);

                    if (OutputDebugStringComponentTypes != null)
                    {
                        foreach (string componentType in OutputDebugStringComponentTypes)
                        {
                            SetCheckBoxValue(checkBoxName: "chk" + componentType, checkBoxValue: true);
                        }
                    }

                    SetOutputDebugStringLevel(OutputDebugStringLevel);
                }
            }

            // outputDebugStringLevel can be "None", "Info" or "Verbose"
            private void SetOutputDebugStringLevel(string outputDebugStringLevel)
            {
                Log.Comment("Retrieving cmbItemsViewOutputDebugStringLevel");
                ComboBox cmbItemsViewOutputDebugStringLevel = new ComboBox(FindElement.ById("cmbItemsViewOutputDebugStringLevel"));
                Verify.IsNotNull(cmbItemsViewOutputDebugStringLevel, "Verifying that cmbItemsViewOutputDebugStringLevel was found");

                Log.Comment("Changing output-debug-string-level selection to " + outputDebugStringLevel);
                cmbItemsViewOutputDebugStringLevel.SelectItemByName(outputDebugStringLevel);
                Log.Comment("Selection is now {0}", cmbItemsViewOutputDebugStringLevel.Selection[0].Name);
            }

            private void SetCheckBoxValue(string checkBoxName, bool checkBoxValue)
            {
                Log.Comment("Retrieving " + checkBoxName);
                CheckBox checkBox = new CheckBox(FindElement.ById(checkBoxName));
                if (checkBox == null)
                {
                    Log.Warning(checkBoxName + " was not found");
                }
                else if (checkBoxValue && checkBox.ToggleState != ToggleState.On ||
                        !checkBoxValue && checkBox.ToggleState != ToggleState.Off)
                {
                    Log.Comment(checkBoxName + " was found. Toggling its IsChecked to " + checkBoxValue);
                    checkBox.Toggle();
                    Wait.ForIdle();
                }
            }
        }

        private class LoggingHelper : IDisposable
        {
            private ItemsViewTestsWithInputHelper m_owner;
            private ICollection<string> m_componentTypes;

            public LoggingHelper(ItemsViewTestsWithInputHelper owner, ICollection<string> componentTypes)
            {
                m_owner = owner;
                m_componentTypes = componentTypes;

                SetLoggingLevel(true, m_componentTypes);
            }

            public void Dispose()
            {
                SetLoggingLevel(false, m_componentTypes);
            }

            private void SetLoggingLevel(bool set, ICollection<string> componentTypes)
            {
                if (componentTypes != null)
                {
                    foreach (string componentName in componentTypes)
                    {
                        switch (componentName)
                        {
                            case "ScrollView":
                                m_owner.SetCheckBoxValue(checkBoxName: "chkLogScrollViewMessages", checkBoxValue: set);
                                break;
                            case "ScrollPresenter":
                                m_owner.SetCheckBoxValue(checkBoxName: "chkLogScrollPresenterMessages", checkBoxValue: set);
                                break;
                            case "ItemsRepeater":
                                m_owner.SetCheckBoxValue(checkBoxName: "chkLogItemsRepeaterMessages", checkBoxValue: set);
                                break;
                            case "ItemsView":
                                m_owner.SetCheckBoxValue(checkBoxName: "chkLogItemsViewMessages", checkBoxValue: set);
                                break;
                            case "AnnotatedScrollBar":
                                m_owner.SetCheckBoxValue(checkBoxName: "chkLogAnnotatedScrollBarMessages", checkBoxValue: set);
                                break;
                            case "LinedFlowLayout":
                                m_owner.SetCheckBoxValue(checkBoxName: "chkLogLinedFlowLayoutMessages", checkBoxValue: set);
                                break;
                            default:
                                Log.Error("SetLoggingLevel - " + componentName + " unexpected.");
                                break;
                        }
                    }
                }
            }
        }
    }
}
