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
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                    {
                        Log.Warning("Skipping: Home/End behavior only works in RS2");
                        return;
                    }

                    UIObject firstItem = FindElement.ByName("Home");
                    UIObject appsItem = FindElement.ByName("Apps");
                    UIObject lastItem = FindElement.ByName("Integer");

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
        public void TabNavigationTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    //TODO: Update RS3 and below tab behavior to match RS4+
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                    {
                        Log.Warning("Test is disabled because the repeater tab behavior is current different rs3 and below.");
                        return;
                    }

                    SetNavViewWidth(ControlWidth.Wide);

                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    UIObject searchBox = FindElement.ByNameAndClassName("PaneAutoSuggestBox", "TextBox");
                    UIObject firstItem = FindElement.ByName("Home");
                    UIObject settingsItem = FindElement.ByName("Settings");
                    togglePaneButton.SetFocus();
                    Wait.ForIdle();

                    Log.Comment("Verify that pressing tab while TogglePaneButton has focus moves to the search box");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Verify.IsTrue(searchBox.HasKeyboardFocus);

                    Log.Comment("Verify that pressing tab while the search box has focus moves to the first menu item");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Verify.IsTrue(firstItem.HasKeyboardFocus);

                    Log.Comment("Verify that pressing tab twice more will move focus to the settings item");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.None, 2);
                    Wait.ForIdle();
                    Verify.IsTrue(settingsItem.HasKeyboardFocus);

                    // TODO: Re-enable test part and remove workaround once saving tab state is fixed

                    Log.Comment("Move Focus to first item");
                    firstItem.SetFocus();
                    Wait.ForIdle();

                    //Log.Comment("Verify that pressing SHIFT+tab twice will move focus to the first menu item");
                    //KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 2);
                    //Wait.ForIdle();
                    //Verify.IsTrue(firstItem.HasKeyboardFocus);

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
                    //TODO: Update RS3 and below tab behavior to match RS4+
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                    {
                        Log.Warning("Test is disabled because the repeater tab behavior is current different rs3 and below.");
                        return;
                    }

                    SetNavViewWidth(ControlWidth.Wide);

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

                    Log.Comment("Verify that tab twice from the last menu item goes to the settings item");
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.None, 2);
                    Wait.ForIdle();
                    Verify.IsTrue(settingsItem.HasKeyboardFocus);

                    // TODO: Re-enable test part and remove workaround once saving tab state is fixed

                    Log.Comment("Move Focus to TV item");
                    item6.SetFocus();
                    Wait.ForIdle();

                    //Log.Comment("Verify that shift+tab twice from the settings item goes to the last menu item");
                    //KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 2);
                    //Wait.ForIdle();
                    //Verify.IsTrue(item6.HasKeyboardFocus);

                    Log.Comment("Verify that up arrow can navigate through all items");
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
            }
        }

        [TestMethod]
        public void ArrowKeyHierarchicalNavigationTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "HierarchicalNavigationView Markup Test" }))
            {

                //TODO: Re-enable for RS2 once arrow key behavior is matched with above versions
                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
                {
                    Log.Warning("Test is disabled because the repeater arrow behavior is currently different for rs2.");
                    return;
                }

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
    }
}
