// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Common;
using System;
using System.Collections.Generic;
using System.Globalization;
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
        public void NavigationViewItemMouseDragStarting()
        {
            VerifyNavigationViewItemDragStarting(PointerInputType.Mouse);
        }

        [TestMethod]
        public void NavigationViewItemPenDragStarting()
        {
            VerifyNavigationViewItemDragStarting(PointerInputType.Pen);
        }

        private static void VerifyNavigationViewItemDragStarting(PointerInputType inputType)
        {
            using var inputController = InputController.Activate(inputType);
            foreach (bool topMode in new[] { false, true })
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Drag Test" }))
                {
                    if (topMode)
                    {
                        new CheckBox(FindElement.ById("TopMode")).Check();
                        Wait.ForIdle();
                    }

                    int expectedDragCount = 0;
                    var sourceIds = topMode ? new[] { "DragItem" } : new[] { "DragItem", "NestedDragItem" };
                    foreach (var sourceId in sourceIds)
                    {
                        for (int attempt = 0; attempt < 2; ++attempt)
                        {
                            // Cancel at DragStarting to isolate gesture detection and capture cleanup
                            // from the system drag/drop service.
                            InputHelper.MouseDragToTarget(FindElement.ById(sourceId), FindElement.ById("DropTarget"));
                            Verify.AreEqual(inputType.ToString(), new TextBlock(FindElement.ById("LastPointerDevice")).GetText());
                            Verify.AreEqual((++expectedDragCount).ToString(), new TextBlock(FindElement.ById("DragCount")).GetText());
                            Verify.AreEqual(sourceId, new TextBlock(FindElement.ById("DragSource")).GetText());
                            Verify.AreEqual("0", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                            VerifyNavigationViewDragStateReset();
                        }
                    }

                    InputHelper.LeftClick(FindElement.ById("DragItem"));
                    Wait.ForIdle();
                    Verify.AreEqual("1", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                    VerifyNavigationViewDragStateReset();

                    InputHelper.MouseDragToTarget(FindElement.ById("DragItem"), FindElement.ById("DropTarget"));
                    Verify.AreEqual((++expectedDragCount).ToString(), new TextBlock(FindElement.ById("DragCount")).GetText());
                    Verify.AreEqual("1", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                    VerifyNavigationViewDragStateReset();

                    new CheckBox(FindElement.ById("Draggable")).Uncheck();
                    Wait.ForIdle();
                    InputHelper.MouseDragToTarget(FindElement.ById("DragItem"), FindElement.ById("DropTarget"));
                    Verify.AreEqual(expectedDragCount.ToString(), new TextBlock(FindElement.ById("DragCount")).GetText());
                    Verify.AreEqual("1", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                    VerifyNavigationViewDragStateReset();

                    InputHelper.LeftClick(FindElement.ById("DragItem"));
                    Wait.ForIdle();
                    Verify.AreEqual("2", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                    VerifyNavigationViewDragStateReset();
                }
            }
        }

        [TestMethod]
        public void NavigationViewItemPenDragStartingWithDeferral()
        {
            using var inputController = InputController.Activate(PointerInputType.Pen);
            foreach (bool topMode in new[] { false, true })
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Drag Test" }))
                {
                    if (topMode)
                    {
                        new CheckBox(FindElement.ById("TopMode")).Check();
                    }
                    new CheckBox(FindElement.ById("DeferDragStart")).Check();
                    Wait.ForIdle();

                    for (int attempt = 0; attempt < 2; ++attempt)
                    {
                        try
                        {
                            InputHelper.MouseDragToTarget(FindElement.ById("DragItem"), FindElement.ById("DropTarget"));
                            Verify.AreEqual("Pen", new TextBlock(FindElement.ById("LastPointerDevice")).GetText());
                            Verify.AreEqual((attempt + 1).ToString(), new TextBlock(FindElement.ById("DragCount")).GetText());
                            Verify.AreEqual("0", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                            Verify.IsTrue(new Button(FindElement.ById("CompleteDragDeferral")).IsEnabled);
                            VerifyNavigationViewDragStateReset();
                        }
                        finally
                        {
                            var completeDeferral = new Button(FindElement.ById("CompleteDragDeferral"));
                            if (completeDeferral.IsEnabled)
                            {
                                completeDeferral.Invoke();
                            }
                            Wait.ForIdle();
                        }
                    }

                    InputHelper.LeftClick(FindElement.ById("DragItem"));
                    Wait.ForIdle();
                    Verify.AreEqual("1", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                    VerifyNavigationViewDragStateReset();
                }
            }
        }

        [TestMethod]
        public void NavigationViewItemPenTapDoesNotDragOnHover()
        {
            VerifyNavigationViewItemTapDoesNotDragOnHover(PointerInputType.Pen);
        }

        [TestMethod]
        public void NavigationViewItemMouseClickDoesNotDragOnHover()
        {
            VerifyNavigationViewItemTapDoesNotDragOnHover(PointerInputType.Mouse);
        }

        private static void VerifyNavigationViewItemTapDoesNotDragOnHover(PointerInputType inputType)
        {
            using var inputController = InputController.Activate(inputType);
            foreach (bool topMode in new[] { false, true })
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Drag Test" }))
                {
                    if (topMode)
                    {
                        new CheckBox(FindElement.ById("TopMode")).Check();
                        Wait.ForIdle();
                    }

                    var source = FindElement.ById("DragItem");
                    var bounds = source.BoundingRectangle;
                    for (int attempt = 1; attempt <= 3; ++attempt)
                    {
                        PointerInput.Move(new System.Drawing.Point(
                            (int)(bounds.Left + bounds.Width / 2), (int)(bounds.Top + bounds.Height / 2)));
                        PointerInput.Press(PointerButtons.Primary);
                        PointerInput.Release(PointerButtons.Primary);
                        Wait.ForIdle();
                        Verify.AreEqual(inputType.ToString(), new TextBlock(FindElement.ById("LastPointerDevice")).GetText());
                        Verify.AreEqual(attempt.ToString(), new TextBlock(FindElement.ById("InvocationCount")).GetText());

                        int hoverMoves = int.Parse(new TextBlock(FindElement.ById("HoverMoveCount")).GetText());
                        foreach (double fraction in new[] { 0.7, 0.3 })
                        {
                            PointerInput.Move(new System.Drawing.Point(
                                (int)(bounds.Left + bounds.Width * fraction), (int)(bounds.Top + bounds.Height / 2)));
                            Wait.ForIdle();
                        }
                        Verify.IsTrue(int.Parse(new TextBlock(FindElement.ById("HoverMoveCount")).GetText()) > hoverMoves);
                        Verify.AreEqual("0", new TextBlock(FindElement.ById("DragCount")).GetText());
                        Verify.AreEqual(attempt.ToString(), new TextBlock(FindElement.ById("InvocationCount")).GetText());
                        VerifyNavigationViewDragStateReset();
                    }

                    InputHelper.MouseDragToTarget(source, FindElement.ById("DropTarget"));
                    Verify.AreEqual("1", new TextBlock(FindElement.ById("DragCount")).GetText());
                    Verify.AreEqual("3", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                    VerifyNavigationViewDragStateReset();

                    InputHelper.LeftClick(source);
                    Verify.AreEqual("4", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                    VerifyNavigationViewDragStateReset();
                }
            }
        }

        [TestMethod]
        public void NavigationViewItemPenSubthresholdExitClearsHover()
        {
            using var inputController = InputController.Activate(PointerInputType.Pen);
            foreach (bool topMode in new[] { false, true })
            {
                using var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Drag Test" });
                if (topMode)
                {
                    new CheckBox(FindElement.ById("TopMode")).Check();
                    Wait.ForIdle();
                }

                new Button(FindElement.ById("Snapshot")).Invoke();
                Wait.ForIdle();
                double scale = double.Parse(new TextBlock(FindElement.ById("ItemRasterizationScale")).GetText(), CultureInfo.InvariantCulture);
                double width = double.Parse(new TextBlock(FindElement.ById("ItemWidth")).GetText(), CultureInfo.InvariantCulture);
                var bounds = FindElement.ById("DragItem").BoundingRectangle;
                double right = bounds.Left + bounds.Width / 2 + width * scale / 2;
                var start = new System.Drawing.Point((int)(right - 5 * scale), (int)(bounds.Top + bounds.Height / 2));
                var end = new System.Drawing.Point((int)(right + scale), start.Y);
                PointerInput.Move(start);
                Wait.ForIdle();
                new Button(FindElement.ById("Snapshot")).Invoke();
                Wait.ForIdle();
                Verify.AreEqual("True", new TextBlock(FindElement.ById("ItemPointerOver")).GetText());

                PointerInput.Press(PointerButtons.Primary);
                try
                {
                    Wait.ForIdle();
                    Verify.AreEqual("Pen", new TextBlock(FindElement.ById("LastPointerDevice")).GetText());
                    PointerInput.Move(end);
                    Wait.ForIdle();
                    Verify.AreEqual("0", new TextBlock(FindElement.ById("DragCount")).GetText());
                }
                finally
                {
                    PointerInput.Release(PointerButtons.Primary);
                }

                Wait.ForIdle();
                VerifyNavigationViewDragStateReset();
                Verify.AreEqual("0", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                Verify.AreEqual("False", new TextBlock(FindElement.ById("ItemPointerOver")).GetText());
            }
        }

        [TestMethod]
        public void NavigationViewItemPenDragWithinBounds()
        {
            using var inputController = InputController.Activate(PointerInputType.Pen);
            foreach (bool topMode in new[] { false, true })
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Drag Test" }))
                {
                    if (topMode)
                    {
                        new CheckBox(FindElement.ById("TopMode")).Check();
                        Wait.ForIdle();
                    }

                    var bounds = FindElement.ById("DragItem").BoundingRectangle;
                    // Start near the right edge so physical and logical bounds differ at higher DPI.
                    var start = new System.Drawing.Point(
                        (int)(bounds.Left + bounds.Width * 0.8), (int)(bounds.Top + bounds.Height / 2));
                    var end = new System.Drawing.Point(
                        (int)(bounds.Left + bounds.Width * 0.95), start.Y);
                    PointerInput.Move(start);
                    PointerInput.ClickDrag(end, PointerButtons.Primary, 2000);
                    Wait.ForIdle();

                    Verify.AreEqual("Pen", new TextBlock(FindElement.ById("LastPointerDevice")).GetText());
                    Verify.AreEqual("1", new TextBlock(FindElement.ById("DragCount")).GetText());
                    Verify.AreEqual("True", new TextBlock(FindElement.ById("DragStartPointerOver")).GetText());
                    Verify.AreEqual("0", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                    VerifyNavigationViewDragStateReset();
                }
            }
        }

        [TestMethod]
        public void NavigationViewItemRetemplateDuringPointerPress()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Drag Test" }))
            {
                new CheckBox(FindElement.ById("RetemplateOnPress")).Check();
                InputHelper.LeftClick(FindElement.ById("DragItem"));
                Wait.ForIdle();
                Verify.AreEqual("1", new TextBlock(FindElement.ById("RetemplateCount")).GetText());
                Verify.AreEqual("0", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                VerifyNavigationViewDragStateReset();

                new CheckBox(FindElement.ById("RetemplateOnPress")).Uncheck();
                InputHelper.LeftClick(FindElement.ById("DragItem"));
                Wait.ForIdle();
                Verify.AreEqual("1", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                VerifyNavigationViewDragStateReset();
            }
        }

        [TestMethod]
        public void NavigationViewItemMouseDrop()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Drag Test" }))
            {
                new CheckBox(FindElement.ById("CancelDrag")).Uncheck();
                Wait.ForIdle();
                InputHelper.MouseDragToTarget(FindElement.ById("DragItem"), FindElement.ById("DropTarget"));
                Verify.AreEqual("1", new TextBlock(FindElement.ById("DragCount")).GetText());
                Verify.AreEqual("DragItem", new TextBlock(FindElement.ById("DroppedText")).GetText());
                Verify.AreEqual("Copy", new TextBlock(FindElement.ById("CompletionResult")).GetText());
                Verify.AreEqual("0", new TextBlock(FindElement.ById("InvocationCount")).GetText());
                VerifyNavigationViewDragStateReset();
            }
        }

        private static void VerifyNavigationViewDragStateReset()
        {
            new Button(FindElement.ById("Snapshot")).Invoke();
            Wait.ForIdle();
            Verify.AreEqual("0", new TextBlock(FindElement.ById("CaptureCount")).GetText());
            Verify.AreEqual("None", new TextBlock(FindElement.ById("PressedStates")).GetText());
        }

        [TestMethod]
        public void VerifyCorrectNumberOfEventsRaised()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            using (var orientation = new OrientationResetHelper())
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
                orientation.Flip();
                Verify.AreEqual(itemInvokedCountTextBlock.GetText(), "0");
                Verify.AreEqual(selectionChangedCountTextBlock.GetText(), "0");

                Log.Comment("Verify that switching pane mode to auto does not raise any events.");
                orientation.Flip();
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
