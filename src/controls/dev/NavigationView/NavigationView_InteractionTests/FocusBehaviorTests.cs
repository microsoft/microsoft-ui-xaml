﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Common;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;


namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.NavigationViewTests
{
    [TestClass]
    public class FocusBehaviorTests : NavigationViewTestsBase
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

        [TestMethod]
        public void EnsureTopSettingsRetainsFocusAfterOrientationChanges()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                var readSettingsSelectedButton = new Button(FindElement.ByName("ReadSettingsSelected"));
                var SettingsSelectionStateTextBlock = new TextBlock(FindElement.ByName("SettingsSelectedState"));

                Log.Comment("Bring Settings into view.");
                FindElement.ByName<Button>("BringSettingsIntoViewButton").Invoke();
                Wait.ForIdle();

                var leftSettingsItem = new Button(FindElement.ByName("Settings"));
                leftSettingsItem.Click();

                Log.Comment("Verify the left settings item is selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");

                Log.Comment("Flipping orientation: Left -> Top.");
                var flipOrientationButton = new Button(FindElement.ByName("FlipOrientationButton"));
                flipOrientationButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify the top settings item is selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");

                Log.Comment("Flipping orientation: Top -> Left.");
                flipOrientationButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify the left settings item is still selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");
            }
        }

        [TestMethod]
        public void VerifyFocusNotLostWhenTabbingWithBackButtonEnabled()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView TopNav Test" }))
            {
                CheckBox checkBox = new CheckBox(FindElement.ByName("BackButtonEnabledCheckbox"));
                Log.Comment("Checking Back Enabled");

                checkBox.Check();
                Wait.ForIdle();

                // Pick an item close to the end of the content and set focus on it.
                CheckBox cancelClosingCheckbox = new CheckBox(FindElement.ById("CancelClosingEvents"));
                cancelClosingCheckbox.SetFocus();
                Wait.ForIdle();

                // Tab a number of times so that we get past the last item
                // in the content.
                for (int i = 0; i < 10; i++)
                {
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                }

                // Verify that we do not lose focus.
                AutomationElement focusedElement = AutomationElement.FocusedElement;
                Verify.IsNotNull(focusedElement);
            }
        }

        [TestMethod]
        public void EnsureLeftSettingsRetainsFocusAfterOrientationChanges()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                var readSettingsSelectedButton = new Button(FindElement.ByName("ReadSettingsSelected"));
                var SettingsSelectionStateTextBlock = new TextBlock(FindElement.ByName("SettingsSelectedState"));

                Log.Comment("Flipping orientation: Left -> Top.");
                var flipOrientationButton = new Button(FindElement.ByName("FlipOrientationButton"));
                flipOrientationButton.Invoke();
                Wait.ForIdle();

                var topSettingsItem = new Button(FindElement.ByName("Settings"));
                topSettingsItem.Click();

                Log.Comment("Verify the top settings item is selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");

                Log.Comment("Flipping orientation: Top -> Left.");
                flipOrientationButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify the left settings item is selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");

                Log.Comment("Flipping orientation: Left -> Top.");
                flipOrientationButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify the left top item is still selected.");
                readSettingsSelectedButton.Invoke();
                Verify.AreEqual(SettingsSelectionStateTextBlock.GetText(), "True");
            }
        }

        [TestMethod]
        public void HomeEndNavigationTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    UIObject firstItem = FindElement.ByName("Home");
                    UIObject appsItem = FindElement.ByName("Apps");
                    UIObject lastItem = FindElement.ByName("HasChildItem");

                    Log.Comment("Make sure something inside the ListView other than the first item has input focus");
                    appsItem.SetFocus();
                    Wait.ForIdle();
                    Verify.IsFalse(firstItem.HasKeyboardFocus);

                    Log.Comment("Verify the Home key puts focus on the first menu item");
                    KeyboardHelper.PressKey(Key.Home);
                    Wait.ForIdle();
                    Verify.IsTrue(firstItem.HasKeyboardFocus);

                    Log.Comment("Verify the End key puts focus on the last menu item");
                    KeyboardHelper.PressKey(Key.End);
                    Wait.ForIdle();
                    Verify.IsTrue(lastItem.HasKeyboardFocus);
                }
            }
        }

        [TestMethod]
        public void HomeEndExtendedCasesNavigationTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "HierarchicalNavigationView Markup Test" }))
            {                
                // Set the pane display mode to left compact
                var paneDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                Verify.IsNotNull(paneDisplayModeComboBox);
                Log.Comment("Set PaneDisplayMode to LeftCompact");
                paneDisplayModeComboBox.SelectItemByName("LeftCompact");
                Wait.ForIdle();

                // Click on the first item to open flyout
                var item1 = FindElement.ByName("Menu Item 1");
                Verify.IsNotNull(item1);
                Log.Comment("Expand Menu Item 1");
                InputHelper.LeftClick(item1);
                Wait.ForIdle();

                Log.Comment("Find first and last items in the flyout");
                var item2 = FindElement.ByName("Menu Item 2");
                Verify.IsNotNull(item2);
                var itemB = FindElement.ByName("Menu Item B");
                Verify.IsNotNull(itemB);

                Log.Comment("Verify the End key puts focus on the last menu item");
                KeyboardHelper.PressKey(Key.End);
                Wait.ForIdle();
                Verify.IsTrue(itemB.HasKeyboardFocus);

                Log.Comment("Verify the Home key puts focus on the first menu item");
                KeyboardHelper.PressKey(Key.Home);
                Wait.ForIdle();
                Verify.IsTrue(item2.HasKeyboardFocus);

                Log.Comment("Press esc key to dismiss flyout");
                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();

                // Set the pane display mode to left
                Log.Comment("Set PaneDisplayMode to Left");
                paneDisplayModeComboBox.SelectItemByName("Left");
                Wait.ForIdle();

                Log.Comment("Disable Menu Item 1");
                UIObject disableItemButtonObject = FindElement.ByName("Disable Menu Item 1");
                Verify.IsNotNull(disableItemButtonObject);
                Button disableItemButton = new Button(disableItemButtonObject);
                disableItemButton.Click();
                Wait.ForIdle();

                // Click on the first item to open flyout
                var item20 = FindElement.ByName("Menu Item 20");
                Verify.IsNotNull(item20);
                Log.Comment("Click on Menu Item 20");
                InputHelper.LeftClick(item20);
                Wait.ForIdle();

                UIObject MI6 = FindElement.ByName("Menu Item 6 (Selectable)");
                Verify.IsNotNull(MI6);

                Log.Comment("If first item is not focusable, verify it finds the next focusable item.");
                KeyboardHelper.PressKey(Key.Home);
                Wait.ForIdle();
                Verify.IsTrue(MI6.HasKeyboardFocus);
            }
        }

        [TestMethod]
        public void TabNavigationTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    SetNavViewWidth(ControlWidth.Wide);

                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    UIObject searchBox = FindElement.ByNameAndClassName("PaneAutoSuggestBox", "TextBox");                   
                    UIObject settingsItem = FindElement.ByName("Settings");

                    VerifyTabNavigationWithoutMenuItemSelected();
                    VerifyTabNavigationWithMenuItemSelected();

                    void VerifyTabNavigationWithoutMenuItemSelected()
                    {
                        Log.Comment("Verify Tab navigation without a selected menu item");

                        // Clear any item selection
                        var clearSelectedItemButton = new Button(FindElement.ByName("ClearSelectedItemButton"));
                        clearSelectedItemButton.Click();
                        Wait.ForIdle();

                        Verify.AreEqual("null", GetSelectedItem());

                        UIObject firstMenuItem = FindElement.ByName("Home");
                        UIObject lastMenuItem = FindElement.ByName("HasChildItem");

                        // Set focus on the pane's toggle button.
                        togglePaneButton.SetFocus();
                        Wait.ForIdle();

                        Log.Comment("Verify that pressing tab while TogglePaneButton has focus moves to the search box");
                        KeyboardHelper.PressKey(Key.Tab);
                        Wait.ForIdle();
                        Verify.IsTrue(searchBox.HasKeyboardFocus);

                        Log.Comment("Verify that pressing tab while the search box has focus moves to the first menu item");
                        KeyboardHelper.PressKey(Key.Tab);
                        Wait.ForIdle();
                        Verify.IsTrue(firstMenuItem.HasKeyboardFocus);

                        Log.Comment("Verify that pressing tab thrice more will move focus to the settings item");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.None, 3);
                        Wait.ForIdle();
                        Verify.IsTrue(settingsItem.HasKeyboardFocus);

                        Log.Comment("Verify that pressing SHIFT+tab thrice will move focus to the last menu item");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 3);
                        Wait.ForIdle();
                        Verify.IsTrue(lastMenuItem.HasKeyboardFocus);

                        Log.Comment("Verify that pressing SHIFT+tab will move focus to the search box");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                        Wait.ForIdle();
                        Verify.IsTrue(searchBox.HasKeyboardFocus);

                        Log.Comment("Verify that pressing SHIFT+tab will move focus to the TogglePaneButton");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                        Wait.ForIdle();
                        Verify.IsTrue(togglePaneButton.HasKeyboardFocus);
                    }

                    void VerifyTabNavigationWithMenuItemSelected()
                    {
                        Log.Comment("Verify Tab navigation with a selected menu item");

                        // Select a menu item (preferably not the first or last menu item)
                        UIObject thirdMenuItem = FindElement.ByName("Games");

                        var selectedItemComboBox = new ComboBox(FindElement.ById("SelectedItemCombobox"));
                        selectedItemComboBox.SelectItemByName("Games");
                        Wait.ForIdle();

                        Verify.IsTrue(Convert.ToBoolean(thirdMenuItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                        // Set focus on the pane's toggle button.
                        togglePaneButton.SetFocus();
                        Wait.ForIdle();

                        Log.Comment("Verify that pressing tab while TogglePaneButton has focus moves to the search box");
                        KeyboardHelper.PressKey(Key.Tab);
                        Wait.ForIdle();
                        Verify.IsTrue(searchBox.HasKeyboardFocus);

                        Log.Comment("Verify that pressing tab while the search box has focus moves to the selected menu item");
                        KeyboardHelper.PressKey(Key.Tab);
                        Wait.ForIdle();
                        Verify.IsTrue(thirdMenuItem.HasKeyboardFocus);

                        Log.Comment("Verify that pressing tab thrice more will move focus to the settings item");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.None, 3);
                        Wait.ForIdle();
                        Verify.IsTrue(settingsItem.HasKeyboardFocus);

                        Log.Comment("Verify that pressing SHIFT+tab thrice will move focus to the selected menu item");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 3);
                        Wait.ForIdle();
                        Verify.IsTrue(thirdMenuItem.HasKeyboardFocus);

                        Log.Comment("Verify that pressing SHIFT+tab will move focus to the search box");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                        Wait.ForIdle();
                        Verify.IsTrue(searchBox.HasKeyboardFocus);

                        Log.Comment("Verify that pressing SHIFT+tab will move focus to the TogglePaneButton");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                        Wait.ForIdle();
                        Verify.IsTrue(togglePaneButton.HasKeyboardFocus);
                    }
                }

                string GetSelectedItem()
                {
                    return FindElement.ByName("SelectionChangedItemType").GetText();
                }
            }
        }

        [TestMethod]
        public void TabNavigationHierarchicalTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "HierarchicalNavigationView Markup Test" }))
                {
                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    Button getSelectItemButton = new Button(FindElement.ByName("GetSelectedItemLabelButton"));

                    Log.Comment("Deselect Menu Item 1");
                    UIObject menuItem1 = FindElement.ByName("Menu Item 1");
                    menuItem1.SetFocus();
                    AutomationElement menuItem1AE = AutomationElement.FocusedElement;
                    SelectionItemPattern menuItem1SIP = menuItem1AE.GetCurrentPattern(SelectionItemPattern.Pattern) as SelectionItemPattern;
                    menuItem1SIP.RemoveFromSelection();

                    VerifyTabNavigationWithoutMenuItemSelected();
                    VerifyTabNavigationWithMenuItemSelected();
                    VerifyTabNavigationWithSettingsItemVisible();

                    void VerifyTabNavigationWithoutMenuItemSelected()
                    {
                        Log.Comment("Verify Tab navigation without a selected menu item");

                        getSelectItemButton.Invoke();
                        Wait.ForIdle();

                        Verify.AreEqual("No Item Selected", GetSelectedItem());

                        UIObject menuItem29 = FindElement.ByName("Menu Item 29 (Selectable)");

                        // Set focus on the pane's toggle button.
                        togglePaneButton.SetFocus();
                        Wait.ForIdle();

                        Log.Comment("Verify that pressing tab while TogglePaneButton has focus moves to Menu Item 1.");
                        KeyboardHelper.PressKey(Key.Tab);
                        Wait.ForIdle();
                        Verify.IsTrue(menuItem1.HasKeyboardFocus);

                        Log.Comment("Verify that pressing tab while Menu Item 1 has focus moves to 'Get Selected Item Label' Button item");
                        KeyboardHelper.PressKey(Key.Tab);
                        Wait.ForIdle();
                        Verify.IsTrue(getSelectItemButton.HasKeyboardFocus);

                        Log.Comment("Verify that pressing SHIFT+tab will move focus to Menu Item 29");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                        Wait.ForIdle();
                        Verify.IsTrue(menuItem29.HasKeyboardFocus);

                        Log.Comment("Verify that pressing SHIFT+tab will move focus to the TogglePaneButton");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                        Wait.ForIdle();
                        Verify.IsTrue(togglePaneButton.HasKeyboardFocus);
                    }

                    void VerifyTabNavigationWithMenuItemSelected()
                    {
                        Log.Comment("Verify Tab navigation with a selected menu item");

                        Log.Comment("Expand Menu Item 15.");
                        UIObject menuItem15 = FindElement.ByName("Menu Item 15");
                        InputHelper.LeftClick(menuItem15);

                        Log.Comment("Select Menu Item 16.");
                        UIObject menuItem16 = FindElement.ByName("Menu Item 16");
                        InputHelper.LeftClick(menuItem16);

                        getSelectItemButton.Invoke();
                        Wait.ForIdle();

                        Verify.AreEqual("Menu Item 16", GetSelectedItem());

                        // Set focus on the pane's toggle button.
                        togglePaneButton.SetFocus();
                        Wait.ForIdle();

                        Log.Comment("Verify that pressing tab while TogglePaneButton has focus moves to Menu Item 16.");
                        KeyboardHelper.PressKey(Key.Tab);
                        Wait.ForIdle();
                        Verify.IsTrue(menuItem16.HasKeyboardFocus);

                        Log.Comment("Verify that pressing tab while Menu Item 16 has focus moves to 'Get Selected Item Label' Button item");
                        KeyboardHelper.PressKey(Key.Tab);
                        Wait.ForIdle();
                        Verify.IsTrue(getSelectItemButton.HasKeyboardFocus);

                        Log.Comment("Verify that pressing SHIFT+tab will move focus to Menu Item 16");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                        Wait.ForIdle();
                        Verify.IsTrue(menuItem16.HasKeyboardFocus);

                        Log.Comment("Verify that pressing SHIFT+tab will move focus to the TogglePaneButton");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                        Wait.ForIdle();
                        Verify.IsTrue(togglePaneButton.HasKeyboardFocus);

                        Log.Comment("Verify that pressing tab from parent of item will move focus to 'Get Selected Item Label' Button item");
                        KeyboardHelper.PressKey(Key.Down, ModifierKey.None, 3);
                        Wait.ForIdle();
                        Verify.IsTrue(menuItem15.HasKeyboardFocus);

                        KeyboardHelper.PressKey(Key.Tab);
                        Wait.ForIdle();
                        Verify.IsTrue(getSelectItemButton.HasKeyboardFocus);
                    }

                    void VerifyTabNavigationWithSettingsItemVisible()
                    {
                        Log.Comment("Verify tab navigation with settings item visible.");

                        Log.Comment("Check IsSettingsVisible");
                        CheckBox checkBoxIsSettingsVisible = FindElement.ByName<CheckBox>("Settings Item Visibility");
                        checkBoxIsSettingsVisible.Check();
                        Wait.ForIdle();

                        UIObject settingsItem = FindElement.ByName("Settings");

                        getSelectItemButton.Invoke();
                        Wait.ForIdle();

                        Verify.AreEqual("Menu Item 16", GetSelectedItem());

                        UIObject menuItem16 = FindElement.ByName("Menu Item 16");

                        // Set focus on the pane's toggle button.
                        togglePaneButton.SetFocus();
                        Wait.ForIdle();

                        Log.Comment("Verify that pressing tab while TogglePaneButton has focus moves to Menu Item 16.");
                        KeyboardHelper.PressKey(Key.Tab);
                        Wait.ForIdle();
                        Verify.IsTrue(menuItem16.HasKeyboardFocus);

                        Log.Comment("Verify that pressing tab while Menu Item 16 has focus moves to Settings item");
                        KeyboardHelper.PressKey(Key.Tab);
                        Wait.ForIdle();
                        Verify.IsTrue(settingsItem.HasKeyboardFocus);

                        Log.Comment("Verify that pressing SHIFT+tab will move focus to Menu Item 16");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                        Wait.ForIdle();
                        Verify.IsTrue(menuItem16.HasKeyboardFocus);

                        Log.Comment("Verify that pressing SHIFT+tab will move focus to the TogglePaneButton");
                        KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                        Wait.ForIdle();
                        Verify.IsTrue(togglePaneButton.HasKeyboardFocus);
                    }
                }

                string GetSelectedItem()
                {
                    return new TextBlock(FindElement.ByName("SelectedItemLabel")).DocumentText;
                }
            }
        }

        [TestMethod]
        public void CanDoSelectionChangedOfItemTemplate()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView ItemTemplate Test" }))
            {
                // Set focus inside page
                var focusButton = FindElement.ByName("FocusAnchorButton");
                focusButton.SetFocus();

                // Navigate to NavView
                KeyboardHelper.PressKey(Key.Tab);
                // Select the first item.
                KeyboardHelper.PressKey(Key.Space);

                Log.Comment("Verify correct items have been passed to selection event");
                var selectedItem = FindElement.ByName("SelectionEventResult");
                Verify.AreEqual("Passed", selectedItem.GetText());
                // Go to the second item.
                KeyboardHelper.PressKey(Key.Right);
                // Select the second item.
                KeyboardHelper.PressKey(Key.Space);
                Log.Comment("Verify correct items have been passed to selection event");
                Verify.AreEqual("Passed", selectedItem.GetText());
            }
        }

        [TestMethod]
        public void ArrowKeyNavigationTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    SetNavViewWidth(ControlWidth.Wide);

                    // Clear any item selection
                    var clearSelectedItemButton = new Button(FindElement.ByName("ClearSelectedItemButton"));
                    clearSelectedItemButton.Click();
                    Wait.ForIdle();

                    Verify.AreEqual("null", GetSelectedItem());

                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    togglePaneButton.SetFocus();
                    Wait.ForIdle();           

                    // Grab references to all the menu items in the test UI
                    UIObject searchBox = FindElement.ByNameAndClassName("PaneAutoSuggestBox", "TextBox");
                    UIObject item1 = FindElement.ByName("Home");
                    UIObject item2 = FindElement.ByName("Apps");
                    UIObject item3 = FindElement.ByName("Games");
                    UIObject item4 = FindElement.ByName("Music");
                    UIObject item5 = FindElement.ByName("Movies");
                    UIObject item6 = FindElement.ByName("TV");
                    UIObject item7 = FindElement.ByName("Volume");
                    UIObject item8 = FindElement.ByName("Integer");
                    UIObject item9 = FindElement.ByName("AcceptItem");
                    UIObject item10 = FindElement.ByName("HasChildItem");
                    UIObject settingsItem = FindElement.ByName("Settings");

                    Log.Comment("Verify that tab from the TogglePaneButton goes to the search box");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Verify.IsTrue(searchBox.HasKeyboardFocus);

                    Log.Comment("Verify that tab from search box goes to the first item");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Verify.IsTrue(item1.HasKeyboardFocus);

                    Log.Comment("Verify that down arrow can navigate through all items");
                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item2.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item3.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item4.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item5.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item6.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item7.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item8.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item9.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                    Verify.IsTrue(item10.HasKeyboardFocus);

                    Log.Comment("Verify that tab thrice from the last menu item goes to the settings item");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.None, 3);
                    Wait.ForIdle();
                    Verify.IsTrue(settingsItem.HasKeyboardFocus);

                    Log.Comment("Verify that shift+tab thrice from the settings item goes to the last menu item");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 3);
                    Wait.ForIdle();
                    Verify.IsTrue(item10.HasKeyboardFocus);

                    Log.Comment("Verify that up arrow can navigate through all items");
                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item9.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item8.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item7.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item6.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item5.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item4.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item3.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item2.HasKeyboardFocus);

                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                    Verify.IsTrue(item1.HasKeyboardFocus);

                    Log.Comment("Verify that shift+tab from the first menu item goes to the search box");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();
                    Verify.IsTrue(searchBox.HasKeyboardFocus);

                    Log.Comment("Verify that shift+tab from the search box goes to the TogglePaneButton");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                    Wait.ForIdle();
                    Verify.IsTrue(togglePaneButton.HasKeyboardFocus);
                }

                string GetSelectedItem()
                {
                    return FindElement.ByName("SelectionChangedItemType").GetText();
                }
            }
        }

        [TestMethod]
        public void ArrowKeyHierarchicalNavigationTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "HierarchicalNavigationView Markup Test" }))
            {
                // Set up tree and get references to all required elements
                UIObject item1 = FindElement.ByName("Menu Item 1");

                Log.Comment("Expand Menu Item 1");
                InputHelper.LeftClick(item1);
                Wait.ForIdle();

                UIObject item2 = FindElement.ByName("Menu Item 2");
                UIObject item3 = FindElement.ByName("Menu Item 3");

                Log.Comment("Expand Menu Item 2");
                InputHelper.LeftClick(item2);
                Wait.ForIdle();

                UIObject item4 = FindElement.ByName("Menu Item 4");
                UIObject item5 = FindElement.ByName("Menu Item 5");

                // Set up initial focus
                Log.Comment("Set focus on the pane toggle button");
                Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                togglePaneButton.SetFocus();
                Wait.ForIdle();

                // Start down arrow key navigation test

                Log.Comment("Verify that down arrow navigates to Menu Item 1");
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                Verify.IsTrue(item1.HasKeyboardFocus);

                Log.Comment("Verify that down arrow navigates to Menu Item 2");
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                Verify.IsTrue(item2.HasKeyboardFocus);

                Log.Comment("Verify that down arrow navigates to Menu Item 4");
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                Verify.IsTrue(item4.HasKeyboardFocus);

                Log.Comment("Verify that down arrow navigates to Menu Item 5");
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                Verify.IsTrue(item5.HasKeyboardFocus);

                Log.Comment("Verify that down arrow navigates to Menu Item 3");
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                Verify.IsTrue(item3.HasKeyboardFocus);

                // Start up arrow key navigation test

                Log.Comment("Verify that up arrow navigates to Menu Item 5");
                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                Verify.IsTrue(item5.HasKeyboardFocus);

                Log.Comment("Verify that up arrow navigates to Menu Item 4");
                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                Verify.IsTrue(item4.HasKeyboardFocus);

                Log.Comment("Verify that up arrow navigates to Menu Item 2");
                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                Verify.IsTrue(item2.HasKeyboardFocus);

                Log.Comment("Verify that up arrow navigates to Menu Item 1");
                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                Verify.IsTrue(item1.HasKeyboardFocus);

                Log.Comment("Verify that up arrow navigates to the pane toggle button");
                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                Verify.IsTrue(togglePaneButton.HasKeyboardFocus);

            }
        }

        [TestMethod]
        public void VerifyShoulderNavigationEnabledAlwaysIsConsistent()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "Top NavigationView Test" }))
            {
                var navView = FindElement.ByName("NavView");
                var shoulderNavigationEnabled = new ComboBox(FindElement.ByName("ShoulderNavigationEnabledSetter"));
                var selectionFollowsFocusComboBox = new ComboBox(FindElement.ByName("SelectionFollowsFocusSetter"));

                Log.Comment("Set ShoulderNavigation to always");
                shoulderNavigationEnabled.SelectItemByName("ShoulderNavigationEnabledAlways");

                Log.Comment("Set SelectionFollowsFocus to enabled");
                selectionFollowsFocusComboBox.SelectItemByName("SelectionFollowsFocusEnabled");

                Log.Comment("Select first item");
                FindElement.ByName("Home").Click();
                Wait.ForIdle();
                Verify.AreEqual("Home", GetSelectedItem());

                GamepadHelper.PressButton(navView, GamepadButton.RightShoulder);
                Wait.ForIdle();
                Verify.AreEqual("Apps", GetSelectedItem());

                Log.Comment("Set SelectionFollowsFocus to disabled");
                selectionFollowsFocusComboBox.SelectItemByName("SelectionFollowsFocusDisabled");
                Wait.ForIdle();

                GamepadHelper.PressButton(navView, GamepadButton.RightShoulder);

                Wait.ForIdle();
                Verify.AreEqual("Games", GetSelectedItem());
            }

            string GetSelectedItem()
            {
                return FindElement.ByName("SelectionChangedResult").GetText();
            }
        }

        [TestMethod]
        public void VerifyShoulderNavigationEnabledOnlySelectionFollowsFocusIsCorrect()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "Top NavigationView Test" }))
            {
                var navView = FindElement.ByName("NavView");
                var shoulderNavigationEnabled = new ComboBox(FindElement.ByName("ShoulderNavigationEnabledSetter"));
                var selectionFollowsFocusComboBox = new ComboBox(FindElement.ByName("SelectionFollowsFocusSetter"));

                Log.Comment("Set ShoulderNavigation to always");
                shoulderNavigationEnabled.SelectItemByName("ShoulderNavigationEnabledWhenSelectionFollowsFocus");

                Log.Comment("Set SelectionFollowsFocus to enabled");
                selectionFollowsFocusComboBox.SelectItemByName("SelectionFollowsFocusEnabled");

                Log.Comment("Select first item");
                FindElement.ByName("Home").Click();
                Wait.ForIdle();
                Verify.AreEqual("Home", GetSelectedItem());

                GamepadHelper.PressButton(navView, GamepadButton.RightShoulder);
                Wait.ForIdle();
                Verify.AreEqual("Apps", GetSelectedItem());

                Log.Comment("Set SelectionFollowsFocus to disabled");
                selectionFollowsFocusComboBox.SelectItemByName("SelectionFollowsFocusDisabled");
                Wait.ForIdle();

                GamepadHelper.PressButton(navView, GamepadButton.RightShoulder);

                Wait.ForIdle();
                Verify.AreEqual("Apps", GetSelectedItem());
            }

            string GetSelectedItem()
            {
                return FindElement.ByName("SelectionChangedResult").GetText();
            }
        }
    }
}
