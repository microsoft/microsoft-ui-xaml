// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

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
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests.NavigationViewTests
{
    [TestClass]
    public class SelectionTests : NavigationViewTestsBase
    {

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("MUXControlsTestEnabledForPhone", "True")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        //[TestMethod]
        // Disabled due to: Multiple unreliable NavigationView tests #134
        public void SuppressSelectionItemInvokeTest()
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                 page2 = new TestSetupHelper("Top NavigationView Store Test"))
            {
                var removeItemButton = new Button(FindElement.ById("RemoveItemButton"));
                var addItemSuppressSelectionButton = new Button(FindElement.ById("AddItemSuppressSelectionButton"));
                var clearItemInvokedTextButton = new Button(FindElement.ById("ClearItemInvokedTextButton"));

                removeItemButton.Click();
                Wait.ForIdle();
                removeItemButton.Click();
                Wait.ForIdle();
                removeItemButton.Click();
                Wait.ForIdle();
                addItemSuppressSelectionButton.Click();
                Wait.ForIdle();

                VerifyElement.Found("sup-selection-nav-item-0", FindBy.Id);
                var supSelectItem0 = TryFindElement.ById("sup-selection-nav-item-0");
                supSelectItem0.Click();
                Wait.ForIdle();

                var itemInvokedText = TryFindElement.ById("ItemInvokedText");
                Verify.AreEqual("New Menu Item S.S", itemInvokedText.GetText());

                clearItemInvokedTextButton.Click();
                Wait.ForIdle();
                Verify.AreEqual(string.Empty, itemInvokedText.GetText());

                UIObject moreButton = null;
                Log.Comment("Adding items until the More button shows up.");

                while (moreButton == null)
                {
                    moreButton = TryFindElement.ById("TopNavOverflowButton");

                    if (moreButton == null)
                    {
                        addItemSuppressSelectionButton.Click();
                        Log.Comment("Item added.");
                        Wait.ForIdle();
                    }
                }

                moreButton.Click();
                Wait.ForIdle();

                var firstChildOverflow = GetTopNavigationItems(TopNavPosition.Overflow)[0];
                firstChildOverflow.Click();
                Wait.ForIdle();

                itemInvokedText = TryFindElement.ById("ItemInvokedText");
                Verify.AreEqual("New Menu Item S.S", itemInvokedText.GetText());
            }
        }

        [TestMethod]
        public void SelectionFollowFocusTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                Log.Warning("Test is disabled on RS2 and earlier because SingleSelectionFollowFocus isn't on RS1 and scrollviewer handles arrow keys on RS2.");
                return;
            }
            var testScenarios = RegressionTestScenario.BuildTopNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {

                    Log.Comment("Check SelectionFollowFocus");
                    CheckBox selectionFollowFocusCheckbox = new CheckBox(FindElement.ById("SelectionFollowFocusCheckbox"));
                    selectionFollowFocusCheckbox.Check();
                    Wait.ForIdle();

                    UIObject firstItem = FindElement.ByName("Apps");
                    UIObject secondItem = FindElement.ByName("Games");

                    Log.Comment("Verify the second item is not already selected");
                    Verify.IsFalse(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    firstItem.Click();
                    Wait.ForIdle();

                    Verify.IsTrue(Convert.ToBoolean(firstItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    Log.Comment("Move focus to the second item by pressing down(left nav)/right(right nav) arrow once");
                    var key = Key.Right;
                    if (testScenario.IsLeftNavTest)
                    {
                        key = Key.Down;
                    }
                    KeyboardHelper.PressKey(key);
                    Wait.ForIdle();

                    Log.Comment("Verify second item is selected and has focus because of SelectionFollowFocus");
                    Verify.IsTrue(secondItem.HasKeyboardFocus);
                    Verify.IsTrue(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    ClickClearSelectionButton();
                    Log.Comment("second item is unselected");
                    Verify.IsFalse(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
                }
            }
        }

        [TestMethod]
        public void MenuItemAutomationSelectionTest()
        {
            // On RS2 scrollviewer handles arrow keys and this causes an issue with the current setup of the "NavigationView Test" test page
            // used for the left NavigationView test. So instead we now execute this test on the "NavigationView Regression Test" test page for
            // left navigation.
            (string testPageName, bool isLeftNavTest)[] testScenarios = new (string, bool)[] 
                { ("NavigationView Regression Test", true), ("NavigationView TopNav Test", false) };

            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.testPageName }))
                {
                    UIObject firstItem = FindElement.ByName("Home");
                    UIObject secondItem = FindElement.ByName("Apps");
                    UIObject thirdItem = FindElement.ByName("Games");

                    Log.Comment("Verify the second item is not already selected");
                    Verify.IsFalse(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    firstItem.SetFocus();
                    AutomationElement firstItemAE = AutomationElement.FocusedElement;
                    SelectionItemPattern firstItemSIP = firstItemAE.GetCurrentPattern(SelectionItemPattern.Pattern) as SelectionItemPattern;

                    Log.Comment("Move focus to the second item by pressing down(left nav)/right(right nav) arrow once");
                    var key = Key.Right;
                    if (testScenario.isLeftNavTest)
                    {
                        key = Key.Down;
                    }
                    KeyboardHelper.PressKey(key);
                    Wait.ForIdle();
                    Verify.IsTrue(secondItem.HasKeyboardFocus);

                    AutomationElement secondItemAE = AutomationElement.FocusedElement;
                    SelectionItemPattern secondItemSIP = secondItemAE.GetCurrentPattern(SelectionItemPattern.Pattern) as SelectionItemPattern;

                    Log.Comment("Select the second item using SelectionItemPattern.Select and verify");
                    secondItemSIP.Select();
                    Wait.ForIdle();
                    Verify.IsTrue(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    Log.Comment("Deselect the second item");
                    firstItemSIP.Select();
                    Wait.ForIdle();
                    Verify.IsTrue(Convert.ToBoolean(firstItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));


                    Log.Comment("Select the second item using SelectionItemPattern.AddToSelection and verify");
                    secondItemSIP.AddToSelection();
                    Wait.ForIdle();
                    Verify.IsTrue(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    ClickClearSelectionButton();
                    Log.Comment("second item is unselected");
                    Verify.IsFalse(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
                }
            }
        }

        [TestMethod]
        public void SettingsCanBeUnselected()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                var readSettingsSelectedButton = new Button(FindElement.ByName("ReadSettingsSelected"));
                var SettingsSelectionStateTextBlock = new TextBlock(FindElement.ByName("SettingsSelectedState"));

                var settings = new Button(FindElement.ByName("Settings"));
                settings.Click();
                Wait.ForIdle();

                Log.Comment("Verify the top settings item is selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");

                ClickClearSelectionButton();

                Log.Comment("Verify the top settings item is unselected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "False");
            }
        }

        [TestMethod]
        public void VerifyDeselectionDisabled()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    UIObject homeItem = FindElement.ByName("Home");
                    Verify.IsNotNull(homeItem);
                    Verify.IsTrue(Convert.ToBoolean(homeItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                    KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
                    homeItem.Click(); // Explicitly testing ctrl+click here
                    Wait.ForIdle();
                    Verify.IsTrue(Convert.ToBoolean(homeItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
                    KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);
                }
            }
        }

        [TestMethod]
        public void CanSelectNavigationViewItemWithChildren()
        {
            var testScenarios = RegressionTestScenario.BuildHierarchicalNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    TextBlock displayModeTextBox = new TextBlock(FindElement.ByName("SelectedItemLabel"));

                    Log.Comment("Verify that first menu item is not expanded.");
                    var firstItem = FindElement.ByName("Menu Item 6 (Selectable)");
                    var childItem = FindElement.ByName("Menu Item 7 (Selectable)");
                    Verify.IsNull(childItem);
                    Verify.AreEqual(displayModeTextBox.DocumentText, "uninitialized");

                    InputHelper.LeftClick(firstItem);
                    Wait.ForIdle();

                    var getSelectItemButton = new Button(FindElement.ByName("GetSelectedItemLabelButton"));
                    getSelectItemButton.Invoke();
                    Wait.ForIdle();

                    childItem = FindElement.ByName("Menu Item 7 (Selectable)");
                    Verify.IsNotNull(childItem, "Child item should be visible after expanding parent item.");
                    Verify.AreEqual(displayModeTextBox.DocumentText, "Menu Item 6 (Selectable)");

                }
            }
        }

        [TestMethod]
        public void SelectingNonTopLevelItemInOverflow()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "HierarchicalNavigationView Markup Test" }))
            {
                TextBlock displayModeTextBox = new TextBlock(FindElement.ByName("SelectedItemLabel"));

                Log.Comment("Put NavigationView into Top Mode.");
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                panelDisplayModeComboBox.SelectItemByName("Top");
                Wait.ForIdle();

                InvokeOverflowButton();

                Log.Comment("Invoke 'Menu Item 29 (Selectable)'.");
                var item = FindElement.ByName("Menu Item 29 (Selectable)");
                InputHelper.LeftClick(item);
                Wait.ForIdle();

                Log.Comment("Invoke 'Menu Item 30 (Selectable)'.");
                item = FindElement.ByName("Menu Item 30 (Selectable)");
                InputHelper.LeftClick(item);
                Wait.ForIdle();

                Log.Comment("Invoke 'Menu Item 31");
                item = FindElement.ByName("Menu Item 31");
                InputHelper.LeftClick(item);
                Wait.ForIdle();

                Log.Comment("Invoke GetSelectedItemLabelButton and verify selected item is correct.");
                var getSelectItemButton = new Button(FindElement.ByName("GetSelectedItemLabelButton"));
                getSelectItemButton.Invoke();
                Wait.ForIdle();
                Verify.AreEqual(displayModeTextBox.DocumentText, "Menu Item 31");
            }
        }

        [TestMethod]
        public void VerifySelectedItemInInvokedItem()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                var invokedItem = FindElement.ByName("Music");

                invokedItem.Click();

                Wait.ForIdle();

                var result = new TextBlock(FindElement.ByName("InvokedItemState"));
                Log.Comment("Verify item is selected when Invoked event got raised");
                Verify.AreEqual("ItemWasSelectedInItemInvoked", result.GetText());

                invokedItem.Click();

                Wait.ForIdle();

                Log.Comment("Verify item invoked was raised despite item already selected");
                result = new TextBlock(FindElement.ByName("InvokedItemState"));
                Verify.AreEqual("ItemWasInvokedSecomdTimeWithCorrectSelection", result.GetText());
            }
        }

        [TestMethod]
        public void VerifyNavigationViewItemIsSelectedWorks()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Init Test" }))
            {
                Log.Comment("Verify the 1st NavItem.IsSelected=true works");
                UIObject item1 = FindElement.ByName("Albums");
                Verify.IsNotNull(item1);
                Verify.IsTrue(Convert.ToBoolean(item1.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                Log.Comment("Verify the 2nd NavItem.IsSelected=true is ignored");
                UIObject item2 = FindElement.ByName("People");
                Verify.IsNotNull(item2);
                Verify.IsFalse(Convert.ToBoolean(item2.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
            }
        }
        
        [TestMethod]
        public void VerifyNoCrashWhenSelectedItemIsInvalidItem()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "Top NavigationView Test" }))
            {

                Button setInvalidSelectedItemButton = new Button(FindElement.ById("SetInvalidSelectedItem"));
                var apps = new Button(FindElement.ById("AppsItem"));

                var invokeResult = new Edit(FindElement.ById("ItemInvokedResult"));
                var selectResult = new Edit(FindElement.ById("SelectionChangedResult"));

                // Select apps
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    apps.Click();
                    waiter.Wait();
                }

                Verify.AreEqual(selectResult.Value, "Apps");

                setInvalidSelectedItemButton.Click();
                Wait.ForIdle();

                Verify.AreEqual(selectResult.Value, "Null");
            }
        }
        
        [TestMethod]
        public void CheckSelectedItemEdgeCase()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "SelectedItem edge case test" }))
            {
                Button button = new Button(FindElement.ByName("CopyStatusButton"));
                TextBlock textBlock = new TextBlock(FindElement.ByName("StatusTextBlock"));

                button.Invoke();
                Wait.ForIdle();
                Verify.AreEqual("False True", textBlock.DocumentText);
            }
        }
        
        [TestMethod]
        public void CanSelectItemInFlyoutAndNVIGetsCollapsedOnFlyoutClose()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "HierarchicalNavigationView Markup Test" }))
            {
                Log.Comment("Put NavigationView into Left Compact Mode.");
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                panelDisplayModeComboBox.SelectItemByName("LeftCompact");
                Wait.ForIdle();

                TextBlock displayModeTextBox = new TextBlock(FindElement.ByName("SelectedItemLabel"));
                Verify.AreEqual(displayModeTextBox.DocumentText, "uninitialized");

                Log.Comment("Select Menu Item 11 which should open flyout.");
                var item = FindElement.ByName("Menu Item 11");
                InputHelper.LeftClick(item);
                Wait.ForIdle();

                Log.Comment("Select Menu Item 12 which should keep flyout open.");
                item = FindElement.ByName("Menu Item 12");
                InputHelper.LeftClick(item);
                Wait.ForIdle();

                Verify.IsNotNull(FindElement.ById("ChildrenFlyout"), "Flyout should still be open.");

                Log.Comment("Select Menu Item 14.");
                item = FindElement.ByName("Menu Item 14");
                InputHelper.LeftClick(item);
                Wait.ForIdle();

                // Refresh the cache to make sure that the flyout object we are going to be searching for
                // does not return as a false positive due to the caching mechanism.
                ElementCache.Refresh();
                Verify.IsNull(FindElement.ById("ChildrenFlyout"), "Flyout should be closed.");

                Log.Comment("Verify that the correct item has been selected");
                var getSelectItemButton = new Button(FindElement.ByName("GetSelectedItemLabelButton"));
                getSelectItemButton.Invoke();
                Wait.ForIdle();
                Verify.AreEqual(displayModeTextBox.DocumentText, "Menu Item 14");

                Log.Comment("Verify that parent has been collapsed");
                TextBlock textBlockCollapsedItem = new TextBlock(FindElement.ByName("TextBlockCollapsedItem"));
                Verify.AreEqual(textBlockCollapsedItem.DocumentText, "Menu Item 11");
            }
        }

        [TestMethod]
        public void VerifyDataContextCanBeUsedForNavigation()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationViewPageDataContext" }))
            {
                TextBlock navViewSelectedDataContext = new TextBlock(FindElement.ByName("NavViewSelectedDataContext"));
                Verify.IsTrue(navViewSelectedDataContext.GetText() == "Item #0_DataContext");

                Log.Comment("Click Item #3");
                var menuItem = FindElement.ByName("Item #3");
                InputHelper.LeftClick(menuItem);
                Wait.ForIdle();

                Verify.IsTrue(navViewSelectedDataContext.GetText() == "Item #3_DataContext");
            }
        }

        [TestMethod]
        public void DisplayModeChangeSelectionEventTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
                {
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
                    {
                        Log.Warning("Test is disabled on RS2 and earlier because SplitView lacks the requisite events.");
                        return;
                    }
                    Button clearSelectedItem = new Button(FindElement.ById("ClearSelectionChangeIndicatorButton"));
                    TextBlock selectionRaisedIndicator = new TextBlock(FindElement.ById("SelectionChangedRaised"));

                    ComboBox selectedItem = new ComboBox(FindElement.ById("SelectedItemCombobox"));
                    selectedItem.SelectItemByName("Settings");
                    Verify.AreEqual("True", selectionRaisedIndicator.GetText());

                    ComboBox displayMode = new ComboBox(FindElement.ById("PaneDisplayModeCombobox"));
                    clearSelectedItem.InvokeAndWait();
                    displayMode.SelectItemByName("Top");
                    Verify.AreEqual("False", selectionRaisedIndicator.GetText());
                    Wait.ForIdle();


                    displayMode.SelectItemByName("Left");
                    Wait.ForIdle();
                    Verify.AreEqual("False", selectionRaisedIndicator.GetText());
                }
            }
        }
    }
}
