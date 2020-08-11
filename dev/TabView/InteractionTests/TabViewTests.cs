// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.Collections.Generic;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using Windows.UI.Xaml.Media;
using Windows.Devices.Input;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class TabViewTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void SelectionTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Log.Comment("Verify content is displayed for initially selected tab.");
                UIObject tabContent = FindElement.ByName("FirstTabContent");
                Verify.IsNotNull(tabContent);

                Log.Comment("Changing selection.");
                UIObject lastTab = FindElement.ByName("LastTab");
                lastTab.Click();
                Wait.ForIdle();

                Log.Comment("Verify content is displayed for newly selected tab.");
                tabContent = FindElement.ByName("LastTabContent");
                Verify.IsNotNull(tabContent);

                Log.Comment("Verify that setting SelectedItem changes selection.");
                Button selectItemButton = FindElement.ByName<Button>("SelectItemButton");
                selectItemButton.InvokeAndWait();

                TextBlock selectedIndexTextBlock = FindElement.ByName<TextBlock>("SelectedIndexTextBlock");
                Verify.AreEqual("1", selectedIndexTextBlock.DocumentText);

                Log.Comment("Verify that setting SelectedIndex changes selection.");
                Button selectIndexButton = FindElement.ByName<Button>("SelectIndexButton");
                selectIndexButton.InvokeAndWait();
                Verify.AreEqual("2", selectedIndexTextBlock.DocumentText);

                Log.Comment("Verify that ctrl-click on tab selects it.");
                UIObject firstTab = FindElement.ByName("FirstTab");
                KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
                firstTab.Click();
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);
                Wait.ForIdle();
                Verify.AreEqual("0", selectedIndexTextBlock.DocumentText);

                Log.Comment("Verify that ctrl-click on tab does not deselect.");
                KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
                firstTab.Click();
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);
                Wait.ForIdle();
                Verify.AreEqual("0", selectedIndexTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void AddRemoveTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Log.Comment("Adding tab.");
                Button addTabButton = FindElement.ByName<Button>("Add New Tab");
                addTabButton.InvokeAndWait();

                ElementCache.Refresh();
                UIObject newTab = FindElement.ByName("New Tab 1");
                Verify.IsNotNull(newTab);

                Log.Comment("Removing tab.");
                Button removeTabButton = FindElement.ByName<Button>("RemoveTabButton");
                removeTabButton.InvokeAndWait();

                ElementCache.Refresh();
                newTab = FindElement.ByName("New Tab 1");
                Verify.IsNull(newTab);
            }
        }

        [TestMethod]
        public void TabSizeAndScrollButtonsTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                UIObject smallerTab = FindElement.ByName("SecondTab");
                UIObject largerTab = FindElement.ByName("LongHeaderTab");

                FindElement.ByName<Button>("SetTabViewWidth").InvokeAndWait();

                Verify.IsFalse(AreScrollButtonsVisible(), "Scroll buttons should not be visible");

                Log.Comment("Equal size tabs should all be the same size.");
                int diff = Math.Abs(largerTab.BoundingRectangle.Width - smallerTab.BoundingRectangle.Width);
                Verify.IsLessThanOrEqual(diff, 1);

                Log.Comment("Changing tab width mode to SizeToContent.");
                ComboBox tabWidthComboBox = FindElement.ByName<ComboBox>("TabWidthComboBox");
                tabWidthComboBox.SelectItemByName("SizeToContent");
                Wait.ForIdle();

                Log.Comment("Tab with larger content should be wider.");
                Verify.IsGreaterThan(largerTab.BoundingRectangle.Width, smallerTab.BoundingRectangle.Width);

                // With largerTab now rendering wider, the scroll buttons should appear:
                Verify.IsTrue(AreScrollButtonsVisible(), "Scroll buttons should appear");

                // Scroll all the way to the left and verify decrease/increase button visual state
                FindElement.ByName<Button>("ScrollTabViewToTheLeft").InvokeAndWait();
                Wait.ForIdle();
                Verify.IsFalse(IsScrollDecreaseButtonEnabled(), "Scroll decrease button should be disabled");
                Verify.IsTrue(IsScrollIncreaseButtonEnabled(), "Scroll increase button should be enabled");

                // Scroll to the middle position and verify decrease/increase button visual state
                FindElement.ByName<Button>("ScrollTabViewToTheMiddle").InvokeAndWait();
                Wait.ForIdle();
                Verify.IsTrue(IsScrollDecreaseButtonEnabled(), "Scroll decrease button should be enabled");
                Verify.IsTrue(IsScrollIncreaseButtonEnabled(), "Scroll increase button should be enabled");

                // Scroll all the way to the right and verify decrease/increase button visual state
                FindElement.ByName<Button>("ScrollTabViewToTheRight").InvokeAndWait();
                Wait.ForIdle();
                Verify.IsTrue(IsScrollDecreaseButtonEnabled(), "Scroll decrease button should be enabled");
                Verify.IsFalse(IsScrollIncreaseButtonEnabled(), "Scroll increase button should be disabled");

                // Close a tab to make room. The scroll buttons should disappear:
                Log.Comment("Closing a tab:");
                Button closeButton = FindCloseButton(FindElement.ByName("LongHeaderTab"));
                closeButton.MovePointer(0, 0);
                closeButton.InvokeAndWait();
                VerifyElement.NotFound("LongHeaderTab", FindBy.Name);

                Log.Comment("Scroll buttons should disappear");
                // Leaving tabstrip with this so the tabs update their width
                FindElement.ByName<Button>("IsClosableCheckBox").MovePointer(0,0);
                Wait.ForIdle();
                Verify.IsFalse(AreScrollButtonsVisible(), "Scroll buttons should disappear");

                // Make sure the scroll buttons can show up in 'Equal' sizing mode. 
                Log.Comment("Changing tab width mode to Equal");
                tabWidthComboBox.SelectItemByName("Equal");
                Wait.ForIdle();
                Verify.IsFalse(AreScrollButtonsVisible(), "Scroll buttons should not be visible");

                var addButton = FindElement.ByName<Button>("Add New Tab");
                Verify.IsNotNull(addButton, "addButton should be available");
                Log.Comment("Adding a tab");
                addButton.InvokeAndWait();
                Verify.IsFalse(AreScrollButtonsVisible(), "Scroll buttons should not be visible");
                Log.Comment("Adding another tab");
                addButton.InvokeAndWait();

                Verify.IsTrue(AreScrollButtonsVisible(), "Scroll buttons should appear");
            }
        }

        private bool AreScrollButtonsVisible()
        {
            FindElement.ByName<Button>("GetScrollButtonsVisible").InvokeAndWait();
            var scrollButtonsVisible = FindElement.ByName<TextBlock>("ScrollButtonsVisible").DocumentText;
            if(scrollButtonsVisible == "True")
            {
                return true;
            }
            else if(scrollButtonsVisible == "False")
            {
                return false;
            }
            else
            {
                Verify.Fail(string.Format("Unexpected value for ScrollButtonsVisible: '{0}'", scrollButtonsVisible));
                return false;
            }
        }

        private bool IsScrollIncreaseButtonEnabled()
        {
            FindElement.ByName<Button>("GetScrollIncreaseButtonEnabled").InvokeAndWait();
            var scrollIncreaseButtonEnabled = FindElement.ByName<TextBlock>("ScrollIncreaseButtonEnabled").DocumentText;
            return scrollIncreaseButtonEnabled == "True";
        }

        private bool IsScrollDecreaseButtonEnabled()
        {
            FindElement.ByName<Button>("GetScrollDecreaseButtonEnabled").InvokeAndWait();
            var scrollDecreaseButtonEnabled = FindElement.ByName<TextBlock>("ScrollDecreaseButtonEnabled").DocumentText;
            return scrollDecreaseButtonEnabled == "True";
        }

        [TestMethod]
        public void CloseSelectionTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Log.Comment("Hiding the disabled tab");
                var disabledTabCheckBox = FindElement.ByName<CheckBox>("IsDisabledTabVisibleCheckBox");
                Verify.IsNotNull(disabledTabCheckBox);
                disabledTabCheckBox.Uncheck();

                Log.Comment("Finding the first tab");
                UIObject firstTab = FindElement.ByName("FirstTab");
                Button closeButton = FindCloseButton(firstTab);
                Verify.IsNotNull(closeButton);

                TextBlock selectedIndexTextBlock = FindElement.ByName<TextBlock>("SelectedIndexTextBlock");
                Verify.AreEqual("0", selectedIndexTextBlock.DocumentText);

                Log.Comment("When the selected tab is closed, selection should move to the next one.");
                // Use Tab's close button:
                closeButton.InvokeAndWait();
                VerifyElement.NotFound("FirstTab", FindBy.Name);
                Verify.AreEqual("0", selectedIndexTextBlock.DocumentText);

                Log.Comment("Select last tab.");
                UIObject lastTab = FindElement.ByName("LastTab");
                lastTab.Click();
                Wait.ForIdle();
                Verify.AreEqual("3", selectedIndexTextBlock.DocumentText);

                Log.Comment("When the selected tab is last and is closed, selection should move to the previous item.");

                // Use Middle Click to close the tab:
                lastTab.Click(PointerButtons.Middle);
                Wait.ForIdle();
                VerifyElement.NotFound("LastTab", FindBy.Name);
                Verify.AreEqual("2", selectedIndexTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void IsClosableTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                UIObject firstTab = FindElement.ByName("FirstTab");
                Button closeButton = FindCloseButton(firstTab);
                Verify.IsNotNull(closeButton);

                Log.Comment("Setting IsClosable=false on the first tab.");
                CheckBox isClosableCheckBox = FindElement.ByName<CheckBox>("IsClosableCheckBox");
                isClosableCheckBox.Uncheck();
                Wait.ForIdle();

                ElementCache.Refresh();
                closeButton = FindCloseButton(firstTab);
                Verify.IsNull(closeButton);

                Log.Comment("Setting IsClosable=true on the first tab.");
                isClosableCheckBox.Check();
                Wait.ForIdle();

                ElementCache.Refresh();
                closeButton = FindCloseButton(firstTab);
                Verify.IsNotNull(closeButton);
            }
        }

        [TestMethod]
        public void HandleItemCloseRequestedTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                UIObject firstTab = FindElement.ByName("FirstTab");
                Button closeButton = FindCloseButton(firstTab);
                Verify.IsNotNull(closeButton);

                CheckBox tabCloseRequestedCheckBox = FindElement.ByName<CheckBox>("HandleTabCloseRequestedCheckBox");
                tabCloseRequestedCheckBox.Uncheck();
                CheckBox tabItemCloseRequestedCheckBox = FindElement.ByName<CheckBox>("HandleTabItemCloseRequestedCheckBox");
                tabItemCloseRequestedCheckBox.Check();
                Wait.ForIdle();

                Log.Comment("TabViewItem.CloseRequested should be raised when the close button is pressed.");
                closeButton.InvokeAndWait();

                ElementCache.Refresh();
                firstTab = TryFindElement.ByName("FirstTab");
                Verify.IsNull(firstTab);
            }
        }

        [TestMethod]
        public void DragBetweenTabViewsTest()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                // TODO 19727004: Re-enable this on versions below RS5 after fixing the bug where mouse click-and-drag doesn't work.
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                UIObject firstTab = FindElement.ByName("FirstTab");
                Verify.IsNotNull(firstTab);

                UIObject dropTarget = FindElement.ByName("TabInSecondTabView");
                Verify.IsNotNull(dropTarget);

                Log.Comment("Home tab should be in the first tab view.");
                PressButtonAndVerifyText("GetFirstTabLocationButton", "FirstTabLocationTextBlock", "FirstTabView");

                InputHelper.DragToTarget(firstTab, dropTarget);
                Wait.ForIdle();
                ElementCache.Refresh();

                Log.Comment("Home tab should now be in the second tab view.");
                PressButtonAndVerifyText("GetFirstTabLocationButton", "FirstTabLocationTextBlock", "SecondTabView");

                Log.Comment("Home tab content should be visible.");
                UIObject tabContent = FindElement.ByName("FirstTabContent");
                Verify.IsNotNull(tabContent);
            }
        }

        [TestMethod]
        public void ReorderItemsTest()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                // TODO 19727004: Re-enable this on versions below RS5 after fixing the bug where mouse click-and-drag doesn't work.
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Button tabItemsSourcePageButton = FindElement.ByName<Button>("TabViewTabItemsSourcePageButton");
                tabItemsSourcePageButton.InvokeAndWait();

                UIObject sourceTab = null;
                int attempts = 0;

                do
                {
                    Wait.ForMilliseconds(100);
                    ElementCache.Refresh();

                    sourceTab = FindElement.ByName("tabViewItem0");
                    attempts++;
                }
                while (sourceTab == null && attempts < 4);

                Verify.IsNotNull(sourceTab);

                UIObject dropTab = FindElement.ByName("tabViewItem2");
                Verify.IsNotNull(dropTab);

                Log.Comment("Reordering tabs with drag-drop operation...");
                InputHelper.DragToTarget(sourceTab, dropTab);
                Wait.ForIdle();
                ElementCache.Refresh();
                Log.Comment("...reordering done. Expecting a TabView.TabItemsChanged event was raised with CollectionChange=ItemInserted and Index=1.");

                TextBlock tblIVectorChangedEventArgsCollectionChange = FindElement.ByName<TextBlock>("tblIVectorChangedEventArgsCollectionChange");
                Verify.AreEqual("ItemInserted", tblIVectorChangedEventArgsCollectionChange.DocumentText);

                TextBlock tblIVectorChangedEventArgsIndex = FindElement.ByName<TextBlock>("tblIVectorChangedEventArgsIndex");
                Verify.AreEqual("1", tblIVectorChangedEventArgsIndex.DocumentText);
            }
        }

        [TestMethod]
        public void AddButtonTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Log.Comment("Add new tab button should be visible.");
                var addButton = FindElement.ByName("Add New Tab");
                Verify.IsNotNull(addButton);

                CheckBox isAddButtonVisibleCheckBox = FindElement.ByName<CheckBox>("IsAddButtonVisibleCheckBox");
                isAddButtonVisibleCheckBox.Uncheck();
                Wait.ForIdle();

                ElementCache.Refresh();
                Log.Comment("Add new tab button should not be visible.");
                addButton = TryFindElement.ByName("Add New Tab");
                Verify.IsNull(addButton);
            }
        }

        [TestMethod]
        public void KeyboardTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Log.Comment("Set focus inside the TabView");
                UIObject tabContent = FindElement.ByName("FirstTabContent");
                tabContent.SetFocus();

                TabItem firstTab = FindElement.ByName<TabItem>("FirstTab");
                TabItem secondTab = FindElement.ByName<TabItem>("SecondTab");
                TabItem lastTab = FindElement.ByName<TabItem>("LastTab");

                Button addButton = FindElement.ById<Button>("AddButton");

                Verify.IsTrue(firstTab.IsSelected, "First Tab should be selected initially");
                Button firstTabButton = FindElement.ByName<Button>("FirstTabButton");
                Verify.IsTrue(firstTabButton.HasKeyboardFocus, "Focus should start in the First Tab");

                // Ctrl+Tab to the second tab:
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Control);
                Verify.IsTrue(secondTab.IsSelected, "Ctrl+Tab should move selection to Second Tab");
                Button secondTabButton = FindElement.ByName<Button>("SecondTabButton");
                Verify.IsTrue(secondTabButton.HasKeyboardFocus, "Focus should move to the content of the Second Tab");

                // Ctrl+Shift+Tab to the first tab:
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Control | ModifierKey.Shift);
                Verify.IsTrue(firstTab.IsSelected, "Ctrl+Shift+Tab should move selection to First Tab");
                Verify.IsTrue(firstTabButton.HasKeyboardFocus, "Focus should move to the content of the First Tab");

                // Ctrl+Shift+Tab to the last tab:
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Control | ModifierKey.Shift);
                Verify.IsTrue(lastTab.IsSelected, "Ctrl+Shift+Tab should move selection to Last Tab");
                Verify.IsTrue(lastTab.HasKeyboardFocus, "Focus should move to the last tab (since it has no focusable content)");

                // Ctrl+Tab to the first tab:
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Control);
                Verify.IsTrue(firstTab.IsSelected, "Ctrl+Tab should move selection to First Tab");
                Verify.IsTrue(firstTab.HasKeyboardFocus, "Focus should move to the first tab");

                KeyboardHelper.PressKey(Key.Up);
                Verify.IsTrue(firstTab.HasKeyboardFocus, "Up key should not move focus");

                KeyboardHelper.PressKey(Key.Down);
                Verify.IsTrue(firstTab.HasKeyboardFocus, "Down key should not move focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(secondTab.HasKeyboardFocus, "Right Key should move focus to the second tab");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(firstTab.HasKeyboardFocus, "Left Key should move focus to the first tab");

                addButton.SetFocus();
                Verify.IsTrue(addButton.HasKeyboardFocus, "AddButton should have keyboard focus");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(lastTab.HasKeyboardFocus, "Left Key from AddButton should move focus to last tab");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(addButton.HasKeyboardFocus, "Right Key from Last Tab should move focus to Add Button");

                firstTab.SetFocus();

                // Ctrl+f4 to close the tab:
                Log.Comment("Verify that pressing ctrl-f4 closes the tab");
                KeyboardHelper.PressKey(Key.F4, ModifierKey.Control);
                Wait.ForIdle();

                VerifyElement.NotFound("FirstTab", FindBy.Name);

                // Move focus to the second tab content
                secondTabButton.SetFocus();
                Wait.ForIdle();
            }
        }

        [TestMethod]
        public void CloseButtonOverlayModeTests()
        {
            using(var setup = new TestSetupHelper("TabView Tests"))
            {
                ComboBox closeButtonOverlayModeComboBox = FindElement.ByName<ComboBox>("CloseButtonOverlayModeCombobox");
                closeButtonOverlayModeComboBox.SelectItemByName("OnPointerOver");
                Wait.ForIdle();

                Button closeUnselectedButton = FindCloseButton(FindElement.ByName("LongHeaderTab"));
                Button closeSelectedButton = FindCloseButton(FindElement.ByName("FirstTab"));
                Verify.IsNull(closeUnselectedButton);
                Verify.IsNotNull(closeSelectedButton);

                closeButtonOverlayModeComboBox.SelectItemByName("Always");
                Wait.ForIdle();

                // Verifiying "Always" works correctly
                closeSelectedButton = FindCloseButton(FindElement.ByName("FirstTab"));
                closeUnselectedButton = FindCloseButton(FindElement.ByName("LongHeaderTab"));
                Verify.IsNotNull(closeUnselectedButton);
                Verify.IsNotNull(closeSelectedButton);

                // Verifiying "OnPointerOver" works correctly
                closeButtonOverlayModeComboBox.SelectItemByName("OnPointerOver");
                Wait.ForIdle();

                closeSelectedButton = FindCloseButton(FindElement.ByName("FirstTab"));
                closeUnselectedButton = FindCloseButton(FindElement.ByName("LongHeaderTab"));
                Verify.IsNull(closeUnselectedButton);
                Verify.IsNotNull(closeSelectedButton);
               
                // Verifiying "Auto" works correctly
                closeButtonOverlayModeComboBox.SelectItemByName("Auto");
                Wait.ForIdle();

                closeSelectedButton = FindCloseButton(FindElement.ByName("FirstTab"));
                closeUnselectedButton = FindCloseButton(FindElement.ByName("LongHeaderTab"));
                Verify.IsNotNull(closeUnselectedButton);
                Verify.IsNotNull(closeSelectedButton);
            }
        } 

        [TestMethod]
        public void GamePadTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Button tabContent = FindElement.ByName<Button>("FirstTabButton");
                ToggleButton toggleThemeButton = FindElement.ById<ToggleButton>("__InnerFrameInLabDimensions");
                TabItem firstTab = FindElement.ByName<TabItem>("FirstTab");
                TabItem secondTab = FindElement.ByName<TabItem>("SecondTab");
                TabItem lastTab = FindElement.ByName<TabItem>("LastTab");
                Button addButton = FindElement.ById<Button>("AddButton");

                firstTab.SetFocus();

                GamepadHelper.PressButton(null, GamepadButton.LeftThumbstickRight);
                Wait.ForIdle();
                Verify.IsTrue(secondTab.HasKeyboardFocus, "GamePad Right should move focus to second tab");

                GamepadHelper.PressButton(null, GamepadButton.LeftThumbstickLeft);
                Wait.ForIdle();
                Verify.IsTrue(firstTab.HasKeyboardFocus, "GamePad Left should move focus to first tab");

                GamepadHelper.PressButton(null, GamepadButton.LeftThumbstickDown);
                Wait.ForIdle();
                Verify.IsTrue(tabContent.HasKeyboardFocus, "GamePad Down should move focus to tab content");

                GamepadHelper.PressButton(null, GamepadButton.LeftThumbstickUp);
                Wait.ForIdle();
                Verify.IsTrue(firstTab.HasKeyboardFocus, "GamePad Up should move focus to tabs");

                GamepadHelper.PressButton(null, GamepadButton.LeftThumbstickUp);
                Wait.ForIdle();
                Verify.IsTrue(toggleThemeButton.HasKeyboardFocus, "GamePad Up should move to toggle theme button");
            }
        }

        [TestMethod]
        public void DragOutsideTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                TextBlock dragOutsideTextBlock = FindElement.ByName<TextBlock>("TabDroppedOutsideTextBlock");
                Verify.AreEqual("", dragOutsideTextBlock.DocumentText);

                Log.Comment("Drag tab out");
                UIObject firstTab = TryFindElement.ByName("FirstTab");
                InputHelper.DragDistance(firstTab, 50, Direction.South);
                Wait.ForIdle();

                Log.Comment("Verify event fired");
                Verify.AreEqual("Home", dragOutsideTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void ToolTipDefaultTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Log.Comment("If the app sets custom tooltip text, it should be preserved.");
                PressButtonAndVerifyText("GetTab0ToolTipButton", "Tab0ToolTipTextBlock", "Custom Tooltip");

                Log.Comment("If the app does not set a custom tooltip, it should be the same as the header text.");
                PressButtonAndVerifyText("GetTab1ToolTipButton", "Tab1ToolTipTextBlock", "SecondTab");

                Button changeShopTextButton = FindElement.ByName<Button>("ChangeShopTextButton");
                changeShopTextButton.InvokeAndWait();

                Log.Comment("If the tab's header changes, the tooltip should update.");
                PressButtonAndVerifyText("GetTab1ToolTipButton", "Tab1ToolTipTextBlock", "Changed");
            }
        }

        [TestMethod]
        public void ToolTipUpdateTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Button customTooltipButton = FindElement.ByName<Button>("CustomTooltipButton");
                customTooltipButton.InvokeAndWait();

                Log.Comment("If the app updates the tooltip, it should change to their custom one.");
                PressButtonAndVerifyText("GetTab1ToolTipButton", "Tab1ToolTipTextBlock", "Custom");

                Button changeShopTextButton = FindElement.ByName<Button>("ChangeShopTextButton");
                changeShopTextButton.InvokeAndWait();

                Log.Comment("The tooltip should not update if the header changes.");
                PressButtonAndVerifyText("GetTab1ToolTipButton", "Tab1ToolTipTextBlock", "Custom");
            }
        }

        [TestMethod]
        public void CloseButtonDoesNotShowWhenVisibilityIsToggled()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                // Wait for the test page's timer to set visibility to the close button to visible
                Wait.ForMilliseconds(2);
                Wait.ForIdle();

                UIObject notCloseableTab = FindElement.ByName("NotCloseableTab");
                var closeButton = FindCloseButton(notCloseableTab);
                Verify.IsNull(closeButton);
            }
        }

        [TestMethod]
        public void SizingTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Button sizingPageButton = FindElement.ByName<Button>("TabViewSizingPageButton");
                sizingPageButton.InvokeAndWait();
                Wait.ForMilliseconds(200);
                ElementCache.Refresh();

                Button setSmallWidthButton = FindElement.ByName<Button>("SetSmallWidth");
                setSmallWidthButton.InvokeAndWait();

                Button getWidthsButton = FindElement.ByName<Button>("GetWidthsButton");
                getWidthsButton.InvokeAndWait();

                TextBlock widthEqualText = FindElement.ByName<TextBlock>("WidthEqualText");
                TextBlock widthSizeToContentText = FindElement.ByName<TextBlock>("WidthSizeToContentText");

                Verify.AreEqual("400", widthEqualText.DocumentText);
                Verify.AreEqual("400", widthSizeToContentText.DocumentText);

                Button setLargeWidthButton = FindElement.ByName<Button>("SetLargeWidth");
                setLargeWidthButton.InvokeAndWait();

                getWidthsButton.InvokeAndWait();

                Verify.AreEqual("700", widthEqualText.DocumentText);
                Verify.AreEqual("700", widthSizeToContentText.DocumentText);
            }
        }

        [TestMethod]
        public void ScrollButtonToolTipTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                PressButtonAndVerifyText("GetScrollDecreaseButtonToolTipButton", "ScrollDecreaseButtonToolTipTextBlock", "Scroll tab list backward");
                PressButtonAndVerifyText("GetScrollIncreaseButtonToolTipButton", "ScrollIncreaseButtonToolTipTextBlock", "Scroll tab list forward");
            }
        }

        [TestMethod]
        public void VerifyTabViewItemHeaderForegroundResource()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Button getSecondTabHeaderForegroundButton = FindElement.ByName<Button>("GetSecondTabHeaderForegroundButton");
                getSecondTabHeaderForegroundButton.InvokeAndWait();

                TextBlock secondTabHeaderForegroundTextBlock = FindElement.ByName<TextBlock>("SecondTabHeaderForegroundTextBlock");

                Verify.AreEqual("#FF008000", secondTabHeaderForegroundTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void VerifySizingBehaviorOnTabCloseComingFromScroll()
        {
            int pixelTolerance = 10;

            using (var setup = new TestSetupHelper(new[] { "TabView Tests", "TabViewTabClosingBehaviorButton" }))
            {

                Log.Comment("Verifying sizing behavior when closing a tab");
                CloseTabAndVerifyWidth("Tab 1", 500, "True;False;");

                CloseTabAndVerifyWidth("Tab 2", 500, "True;False;");

                CloseTabAndVerifyWidth("Tab 3", 500, "False;False;");

                CloseTabAndVerifyWidth("Tab 5", 401, "False;False;");

                CloseTabAndVerifyWidth("Tab 4", 401, "False;False;");

                Log.Comment("Leaving the pointer exited area");
                var readTabViewWidthButton = new Button(FindElement.ByName("GetActualWidthButton"));
                readTabViewWidthButton.Click();
                Wait.ForIdle();

                readTabViewWidthButton.Click();
                Wait.ForIdle();

                Log.Comment("Verify correct TabView width");
                Verify.IsTrue(Math.Abs(GetActualTabViewWidth() - 283) < pixelTolerance);
            }

            void CloseTabAndVerifyWidth(string tabName, int expectedValue, string expectedScrollbuttonStates)
            {
                Log.Comment("Closing tab:" + tabName);
                FindCloseButton(FindElement.ByName(tabName)).Click();
                Wait.ForIdle();
                Log.Comment("Verifying TabView width");
                Verify.IsTrue(Math.Abs(GetActualTabViewWidth() - expectedValue) < pixelTolerance);
                Verify.AreEqual(expectedScrollbuttonStates, FindElement.ByName("ScrollButtonStatus").GetText());

            }

            double GetActualTabViewWidth()
            {
                var tabviewWidth = new TextBlock(FindElement.ByName("TabViewWidth"));

                return Double.Parse(tabviewWidth.GetText());
            }
        }

        [TestMethod]
        public void VerifySizingBehaviorOnTabCloseComingFromNonScroll()
        {
            int pixelTolerance = 10;

            using (var setup = new TestSetupHelper(new[] { "TabView Tests", "TabViewTabClosingBehaviorButton" }))
            {

                Log.Comment("Verifying sizing behavior when closing a tab");
                CloseTabAndVerifyWidth("Tab 1", 500, "True;False;");

                CloseTabAndVerifyWidth("Tab 2", 500, "True;False;");

                CloseTabAndVerifyWidth("Tab 3", 500, "False;False;");

                var readTabViewWidthButton = new Button(FindElement.ByName("GetActualWidthButton"));
                readTabViewWidthButton.Click();
                Wait.ForIdle();

                CloseTabAndVerifyWidth("Tab 5", 500, "False;False;");

                CloseTabAndVerifyWidth("Tab 4", 500, "False;False;");

                Log.Comment("Leaving the pointer exited area");

                readTabViewWidthButton.Click();
                Wait.ForIdle();

                Log.Comment("Verify correct TabView width");
                Verify.IsTrue(Math.Abs(GetActualTabViewWidth() - 500) < pixelTolerance);
            }

            void CloseTabAndVerifyWidth(string tabName, int expectedValue, string expectedScrollbuttonStates)
            {
                Log.Comment("Closing tab:" + tabName);
                FindCloseButton(FindElement.ByName(tabName)).Click();
                Wait.ForIdle();
                Log.Comment("Verifying TabView width");
                Verify.IsTrue(Math.Abs(GetActualTabViewWidth() - expectedValue) < pixelTolerance);
                Verify.AreEqual(expectedScrollbuttonStates, FindElement.ByName("ScrollButtonStatus").GetText());

            }

            double GetActualTabViewWidth()
            {
                var tabviewWidth = new TextBlock(FindElement.ByName("TabViewWidth"));

                return Double.Parse(tabviewWidth.GetText());
            }
        }

        public void PressButtonAndVerifyText(String buttonName, String textBlockName, String expectedText)
        {
            Button button = FindElement.ByName<Button>(buttonName);
            button.InvokeAndWait();

            TextBlock textBlock = FindElement.ByName<TextBlock>(textBlockName);
            Verify.AreEqual(expectedText, textBlock.DocumentText);
        }

        Button FindCloseButton(UIObject tabItem)
        {
            foreach (UIObject elem in tabItem.Children)
            {
                if (elem.ClassName.Equals("Button"))
                {
                    Log.Comment("Found close button for object " + tabItem.Name);
                    return new Button(elem);
                }
            }
            Log.Comment("Did not find close button for object " + tabItem.Name);
            return null;
        }
    }
}
