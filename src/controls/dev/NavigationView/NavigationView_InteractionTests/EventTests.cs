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
    public class EventTests : NavigationViewTestsBase
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
        public void VerifyCorrectNumberOfEventsRaised()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                TextBlock itemInvokedCountTextBlock = new TextBlock(FindElement.ByName("NumberOfItemInvokedEventsRaisedTextBlock"));
                TextBlock selectionChangedCountTextBlock = new TextBlock(FindElement.ByName("NumberOfSelectionChangedEventsRaisedTextBlock"));

                Log.Comment("Verify that only one SelectionChanged event was raised.");
                Verify.AreEqual(itemInvokedCountTextBlock.GetText(), "0");
                Verify.AreEqual(selectionChangedCountTextBlock.GetText(), "1");

                Log.Comment("Invoke MusicItem.");
                UIObject item1 = FindElement.ByName("Music");
                item1.Click();
                Wait.ForIdle();

                Log.Comment("Verify event counts.");
                Verify.AreEqual(itemInvokedCountTextBlock.GetText(), "1");
                Verify.AreEqual(selectionChangedCountTextBlock.GetText(), "2");

                Log.Comment("Invoke selected item (MusicItem).");
                item1.Click();
                Wait.ForIdle();
                Verify.AreEqual(itemInvokedCountTextBlock.GetText(), "2");
                Verify.AreEqual(selectionChangedCountTextBlock.GetText(), "2");

                Log.Comment("Clear event counters.");
                Button resetEventCounters = new Button(FindElement.ByName("ResetEventCounters"));
                resetEventCounters.Invoke();
                Wait.ForIdle();
                Verify.AreEqual(itemInvokedCountTextBlock.GetText(), "0");
                Verify.AreEqual(selectionChangedCountTextBlock.GetText(), "0");

                Log.Comment("Verify that switching pane mode to top does not raise any events.");
                var flipOrientationButton = new Button(FindElement.ByName("FlipOrientationButton"));
                flipOrientationButton.Invoke();
                Wait.ForIdle();
                Verify.AreEqual(itemInvokedCountTextBlock.GetText(), "0");
                Verify.AreEqual(selectionChangedCountTextBlock.GetText(), "0");

                Log.Comment("Verify that switching pane mode to auto does not raise any events.");
                flipOrientationButton.Invoke();
                Wait.ForIdle();
                Verify.AreEqual(itemInvokedCountTextBlock.GetText(), "0");
                Verify.AreEqual(selectionChangedCountTextBlock.GetText(), "0");
            }
        }

        [TestMethod]
        public void VerifyEventsReturnExpectedDataTypesMenuItems()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                const string navigationViewItemType = "Microsoft.UI.Xaml.Controls.NavigationViewItem";
                const string stringType = "System.String";

                var itemInvokedItemType = new Edit(FindElement.ById("ItemInvokedItemType"));
                var itemInvokedItemContainerType = new Edit(FindElement.ById("ItemInvokedItemContainerType"));
                var selectionChangedItemtype = new Edit(FindElement.ById("SelectionChangedItemType"));
                var selectionChangedItemContainerType = new Edit(FindElement.ById("SelectionChangedItemContainerType"));

                Log.Comment("Click music item");
                var menuItem = FindElement.ByName("Music");
                InputHelper.LeftClick(menuItem);
                Wait.ForIdle();

                Log.Comment("Verify that item invoked returns expected parameters.");
                Verify.IsTrue(itemInvokedItemType.Value == stringType);
                Verify.IsTrue(itemInvokedItemContainerType.Value == navigationViewItemType);

                Log.Comment("Verify that selection changed event returns expected parameters");
                Verify.IsTrue(selectionChangedItemtype.Value == navigationViewItemType);
                Verify.IsTrue(selectionChangedItemContainerType.Value == navigationViewItemType);
            }
        }

        [TestMethod]
        public void VerifyEventsReturnExpectedDataTypesMenuItemsSource()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Init Test" }))
            {
                const string navigationViewItemType = "Microsoft.UI.Xaml.Controls.NavigationViewItem";
                const string itemType = "System.String";

                var itemInvokedItemType = new Edit(FindElement.ById("ItemInvokedItemType"));
                var itemInvokedItemContainerType = new Edit(FindElement.ById("ItemInvokedItemContainerType"));
                var selectionChangedItemtype = new Edit(FindElement.ById("SelectionChangedItemType"));
                var selectionChangedItemContainerType = new Edit(FindElement.ById("SelectionChangedItemContainerType"));

                Log.Comment("Click music item");
                var menuItem = FindElement.ByName("Music");
                InputHelper.LeftClick(menuItem);
                Wait.ForIdle();

                Log.Comment("Verify that item invoked returns expected parameters.");
                Verify.IsTrue(itemInvokedItemType.Value == itemType);
                Verify.IsTrue(itemInvokedItemContainerType.Value == navigationViewItemType);

                Log.Comment("Verify that selection changed event returns expected parameters");
                Verify.IsTrue(selectionChangedItemtype.Value == itemType);
                Verify.IsTrue(selectionChangedItemContainerType.Value == navigationViewItemType);
            }
        }

        [TestMethod]
        public void VerifyEventsReturnExpectedDataTypesItemTemplate()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView ItemTemplate Test" }))
            {
                const string navigationViewItemType = "Microsoft.UI.Xaml.Controls.NavigationViewItem";
                const string itemType = "MUXControlsTestApp.Customer";

                var itemInvokedItemType = new Edit(FindElement.ById("ItemInvokedItemType"));
                var itemInvokedItemContainerType = new Edit(FindElement.ById("ItemInvokedItemContainerType"));
                var selectionChangedItemtype = new Edit(FindElement.ById("SelectionChangedItemType"));
                var selectionChangedItemContainerType = new Edit(FindElement.ById("SelectionChangedItemContainerType"));

                Log.Comment("Click Michael item");
                var menuItem = FindElement.ByName("Michael");
                InputHelper.LeftClick(menuItem);
                Wait.ForIdle();

                Log.Comment("Verify that item invoked returns expected parameters.");
                Verify.IsTrue(itemInvokedItemType.Value == itemType);
                Verify.IsTrue(itemInvokedItemContainerType.Value == navigationViewItemType);

                Log.Comment("Verify that selection changed event returns expected parameters");
                Verify.IsTrue(selectionChangedItemtype.Value == itemType);
                Verify.IsTrue(selectionChangedItemContainerType.Value == navigationViewItemType);
            }
        }

        [TestMethod]
        public void VerifyLightDismissDoesntSendDuplicateEvents()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");

                    SetNavViewWidth(ControlWidth.Medium);
                    WaitAndAssertPaneStatus(PaneOpenStatus.Closed);

                    PaneOpenCloseTestCaseRetry(3, () =>
                    {
                        // recover from the exception if needed
                        if (isPaneOpenCheckBox.ToggleState != ToggleState.Off)
                        {
                            using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                            {
                                isPaneOpenCheckBox.Toggle();
                                waiter.Wait();
                            }
                            WaitAndAssertPaneStatus(PaneOpenStatus.Closed);
                        }

                        Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False");

                        Log.Comment("Reset the event count");
                        new Button(FindElement.ById("ClosingEventCountResetButton")).Invoke();
                        Wait.ForIdle();

                        Log.Comment("Open the pane");
                        using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                        {
                            isPaneOpenCheckBox.Toggle();
                            waiter.Wait();
                        }
                        WaitAndAssertPaneStatus(PaneOpenStatus.Opened);

                        Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");

                        var closingCounts = new Edit(FindElement.ByName("ClosingEventCountTextBlock"));
                        var expectedString = "1-1";

                        //  trigger a light dismiss
                        KeyboardHelper.PressKey(Key.Left, ModifierKey.Alt);
                        Wait.ForIdle();

                        WaitAndAssertPaneStatus(PaneOpenStatus.Closed);
                        Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False");
                        Verify.AreEqual(expectedString, closingCounts.GetText());
                    });
                }
            }
        }

        [TestMethod]
        public void BackRequestedTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    TextBlock textBlock = new TextBlock(FindElement.ByName("BackRequestedStateTextBlock"));
                    Button button = new Button(FindElement.ByName("NavigationViewBackButton"));
                    CheckBox checkBox = new CheckBox(FindElement.ByName("BackButtonEnabledCheckbox"));

                    Verify.AreEqual("Test Not Started [2]", textBlock.DocumentText);

                    checkBox.Check();
                    Wait.ForIdle();

                    button.Invoke();
                    Wait.ForIdle();

                    Verify.AreEqual("Back was requested", textBlock.DocumentText);
                }
            }
        }

        [TestMethod]
        public void MenuItemInvokedTest()
        {
            var testScenarios = RegressionTestScenario.BuildTopNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    Log.Comment("Click games item");
                    UIObject menuItem = FindElement.ByName("Games");
                    InputHelper.LeftClick(menuItem);
                    Wait.ForIdle();
                    TextBlock header = new TextBlock(FindElement.ByName("Games as header"));
                    Verify.AreEqual("Games as header", header.DocumentText);

                    menuItem = FindElement.ByName("Music");

                    Log.Comment("Verify XButton1 mouse button click does not change selection");
                    InputHelper.Click(menuItem, PointerButtons.XButton1);
                    Wait.ForIdle();
                    Verify.AreEqual("Games as header", header.DocumentText);

                    Log.Comment("Verify XButton2 mouse button click does not change selection");
                    InputHelper.Click(menuItem, PointerButtons.XButton2);
                    Wait.ForIdle();
                    Verify.AreEqual("Games as header", header.DocumentText);

                    Log.Comment("Verify Secondary mouse button click does not change selection");
                    InputHelper.Click(menuItem, PointerButtons.Secondary);
                    Wait.ForIdle();
                    Verify.AreEqual("Games as header", header.DocumentText);

                    Log.Comment("Left Click music item");
                    InputHelper.LeftClick(menuItem);
                    Wait.ForIdle();
                    Verify.AreEqual("Music as header", header.DocumentText);

                    if (testScenario.IsLeftNavTest)
                    {
                        Log.Comment("Bring Settings into view.");
                        FindElement.ByName<Button>("BringSettingsIntoViewButton").Invoke();
                        Wait.ForIdle();
                    }

                    Log.Comment("Click settings item");
                    menuItem = FindElement.ByName("Settings");
                    InputHelper.LeftClick(menuItem);
                    Wait.ForIdle();
                    header = new TextBlock(FindElement.ByName("Settings as header"));
                    Verify.AreEqual("Settings as header", header.DocumentText);

                    Log.Comment("Move mouse to upper left to ensure that tooltip on settings closes.");
                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        public void SettingsItemClickTest()
        {
            var testScenarios = RegressionTestScenario.BuildAllRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    if (testScenario.IsLeftNavTest)
                    {
                        Log.Comment("Bring Settings into view.");
                        FindElement.ByName<Button>("BringSettingsIntoViewButton").Invoke();
                        Wait.ForIdle();
                    }

                    UIObject settingsItem = FindElement.ByName("Settings");

                    settingsItem.SetFocus();
                    Wait.ForIdle();

                    Log.Comment("Click settings");
                    settingsItem.Click();
                    Wait.ForIdle();

                    Log.Comment("Verify settings is selected");
                    TextBlock header = new TextBlock(FindElement.ByName("Settings as header"));
                    Verify.AreEqual("Settings as header", header.DocumentText);
                }
            }
        }

        [TestMethod]
        public void VerifyNavigationViewItemResponseToClickAfterBeingMovedBetweenFrames()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Init Test" }))
            {
                var myLocationButton = FindElement.ByName<Button>("MyLocation");
                var switchFrameButton = FindElement.ByName<Button>("SwitchFrame");
                var result = new TextBlock(FindElement.ByName("MyLocationResult"));

                Log.Comment("Click on MyLocation Item and verify it's on Frame1");
                myLocationButton.Click();
                Wait.ForIdle();
                Verify.AreEqual(result.GetText(), "Frame1");

                Log.Comment("Click on SwitchFrame");
                switchFrameButton.Click();
                Wait.ForIdle();

                // tree structure changed and rebuild the cache.
                ElementCache.Clear();

                Log.Comment("Click on MyLocation Item and verify it's on Frame2");
                myLocationButton = FindElement.ByName<Button>("MyLocation");
                myLocationButton.Click();
                Wait.ForIdle();
                Verify.AreEqual(result.GetText(), "Frame2");
            }
        }

        [TestMethod]
        public void VerifyHoldingKeyOnlyInvokesOnce()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView compact pane length test" }))
            {
                Verify.AreEqual("0", GetInvokeCount());
                var homeItem = FindElement.ByName("NavViewInvokeItem");
                FocusHelper.SetFocus(homeItem);

                KeyboardHelper.PressKey(Key.Enter);

                Verify.AreEqual("1", GetInvokeCount());

                KeyboardHelper.HoldKeyMilliSeconds(Key.Enter, 2000);
                Wait.ForIdle();
                // Should have invoked once, not multiple times
                Verify.AreEqual("2", GetInvokeCount());
            }

            string GetInvokeCount()
            {
                var textBlock = new TextBlock(FindElement.ByName("HomeItemInvokedCount"));
                return textBlock.GetText();
            }
        }
    }
}
