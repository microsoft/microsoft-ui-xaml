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
    public class PaneBehaviorTests : NavigationViewTestsBase
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
        public void PaneClosedUponLaunch()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "Navigation Minimal Test" }))
                {
                    Log.Comment("Verify that NavigationView with DisplayMode set to 'Auto' and a narrow width does not display pane on load.");
                    CheckBox isAutoPaneOpenCheckBox = new CheckBox(FindElement.ById("IsAutoPaneOpenCheckBox"));
                    Wait.ForIdle();
                    Verify.IsTrue(isAutoPaneOpenCheckBox.ToggleState == ToggleState.Off);

                    Log.Comment("Verify that NavigationView with DisplayMode set to 'LeftMinimal' does not display pane on load.");
                    CheckBox isLeftMinimalPaneOpenCheckBox = new CheckBox(FindElement.ById("IsLeftMinimalPaneOpenCheckBox"));
                    Verify.IsTrue(isLeftMinimalPaneOpenCheckBox.ToggleState == ToggleState.Off);

                    Log.Comment("Verify that NavigationView with DisplayMode set to 'LeftCompact' does not display pane on load.");
                    CheckBox isLeftCompactPaneOpenCheckBox = new CheckBox(FindElement.ById("IsLeftCompactPaneOpenCheckBox"));
                    Verify.IsTrue(isLeftCompactPaneOpenCheckBox.ToggleState == ToggleState.Off);
                }

            }
        }

        [TestMethod]
        public void PaneOpenCloseTestPartTwo() // Otherwise this test will exceed the 30 second timeout in catgates chk runs
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                    // On phone, the pane will initially be in the closed compact state, so open it before
                    // proceeding with the test.
                    if (isPaneOpenCheckBox.ToggleState == ToggleState.Off)
                    {
                        using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                        {
                            isPaneOpenCheckBox.Toggle();
                            waiter.Wait();
                        }
                    }

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");

                    Button navButton = new Button(FindElement.ById("TogglePaneButton"));

                    Log.Comment("Verify that after explicitly closing the nav pane, changing display mode doesn't reopen it");
                    navButton.Invoke();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False after explicitly closing the nav pane & changing display mode");
                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False");
                    SetNavViewWidth(ControlWidth.Wide);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False");

                    Log.Comment("Verify that selecting a menu item in minimal display mode opens the pane");
                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    navButton.Invoke();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True after selecting a menu item in minimal display mode");

                    Log.Comment("Invoke Music item to close the pane");
                    var music = new Button(FindElement.ByName("Music"));
                    music.Click();
                    Wait.ForIdle();

                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False after invoking Music item");
                }
            }
        }

        [TestMethod]
        public void PaneOpenCloseEventsTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
                    {
                        Log.Warning("Test is disabled on RS2 and earlier because SplitView lacks the requisite events.");
                        return;
                    }

                    TextBlock lastIngEventTextblock = new TextBlock(FindElement.ByName("LastIngEventText"));
                    TextBlock lastEdEventTextblock = new TextBlock(FindElement.ByName("LastEdEventText"));

                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    togglePaneButton.Invoke();
                    Wait.ForIdle();

                    WaitAndAssertPaneStatus(PaneOpenStatus.Closed);
                    Verify.AreEqual("PaneClosed event fired", lastEdEventTextblock.DocumentText);
                    Verify.AreEqual("PaneClosing event fired", lastIngEventTextblock.DocumentText);

                    togglePaneButton.Invoke();
                    Wait.ForIdle();

                    WaitAndAssertPaneStatus(PaneOpenStatus.Opened);
                    Verify.AreEqual("PaneOpened event fired", lastEdEventTextblock.DocumentText);
                    Verify.AreEqual("PaneOpening event fired", lastIngEventTextblock.DocumentText);
                }
            }
        }

        // Test for issue 450 https://github.com/Microsoft/microsoft-ui-xaml/issues/450
        [TestMethod]
        public void CompactModeAutoPaneClosingTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                // Unmaximize the window
                KeyboardHelper.PressKey(Key.Down, ModifierKey.Windows, 1);

                // Resize window quickly
                KeyboardHelper.PressDownModifierKey(ModifierKey.Windows);
                KeyboardHelper.PressKeySequence(new[] { Key.Left, Key.Right, Key.Left, Key.Right, Key.Left });
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Windows);

                Wait.ForIdle();

                CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False");

                var getVisualStateButton = new Button(FindElement.ByName("GetNavViewActiveVisualStates"));
                getVisualStateButton.Invoke();
                Wait.ForIdle();
                var result = new TextBlock(FindElement.ByName("NavViewActiveVisualStatesResult"));
                Verify.IsTrue(result.GetText().Contains("ListSizeCompact"), "Verify pane list is in ListSizeCompact state");

                // Maximize the window
                KeyboardHelper.PressKey(Key.Right, ModifierKey.Windows, 1);
                KeyboardHelper.PressKey(Key.Up, ModifierKey.Windows, 1);
            }
        }

        [TestMethod]
        public void PaneTabNavigationTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                    {
                        Log.Warning("Skipping: Correct pane tab navigation only works in RS2 and above");
                        return;
                    }

                    SetNavViewWidth(ControlWidth.Wide);

                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    UIObject firstItem = FindElement.ByName("Home");
                    UIObject settingsItem = FindElement.ByName("Settings");
                    UIObject nextTabTarget = FindElement.ByName("WidthComboBox");

                    CheckBox autoSuggestCheckBox = new CheckBox(FindElement.ByName("AutoSuggestCheckbox"));
                    autoSuggestCheckBox.Uncheck();
                    Wait.ForIdle();

                    Log.Comment("Verify that in Expanded mode, tab navigation can leave the pane");
                    firstItem.SetFocus();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                    Wait.ForIdle();

                    Wait.RetryUntilEvalFuncSuccessOrTimeout(
                        () => { return togglePaneButton.HasKeyboardFocus; },
                        retryTimoutByMilliseconds: 3000
                    );

                    Log.Comment("Verify pressing shift-tab from the first menu item goes to the toggle button");
                    Verify.IsTrue(togglePaneButton.HasKeyboardFocus);

                    settingsItem.SetFocus();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Log.Comment("Verify pressing tab from settings goes to the first tab stop in the content area");
                    Verify.IsTrue(nextTabTarget.HasKeyboardFocus);

                    SetNavViewWidth(ControlWidth.Medium);

                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    isPaneOpenCheckBox.Check();
                    Wait.ForIdle();

                    Log.Comment("Verify that in an overlay mode, tab navigation cannot leave the pane while the pane is open");
                    firstItem.SetFocus();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift, 1);
                    Wait.ForIdle();
                    Log.Comment("Verify pressing shift-tab from the first menu item goes to settings");
                    Verify.IsTrue(settingsItem.HasKeyboardFocus);

                    settingsItem.SetFocus();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();
                    Log.Comment("Verify pressing tab from settings goes to the first menu item");
                    Verify.IsTrue(firstItem.HasKeyboardFocus);
                }
            }
        }

        [TestMethod]
        public void ForceIsPaneOpenToFalseOnLeftNavTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "Navigation IsPaneOpen Test" }))
            {
                Log.Comment("Verify IsPaneOpen=False would not open the pane by default");
                CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False");

                Button navButton = new Button(FindElement.ById("TogglePaneButton"));

                Log.Comment("Verify that clicking the navigation button opens the nav pane");
                navButton.Invoke();
                Wait.ForIdle();
                Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True after clicking the navigation button");
            }
        }

        [TestMethod]
        public void PaneOpenCloseTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                    // On phone, the pane will initially be in the closed compact state, so open it before
                    // proceeding with the test.
                    if (isPaneOpenCheckBox.ToggleState == ToggleState.Off)
                    {
                        using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                        {
                            isPaneOpenCheckBox.Toggle();
                            waiter.Wait();
                        }
                    }

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");

                    Button navButton = new Button(FindElement.ById("TogglePaneButton"));

                    Log.Comment("Verify that clicking the navigation button closes the nav pane");
                    navButton.Invoke();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False after clicking the navigation button");

                    Log.Comment("Verify that clicking the navigation button opens the nav pane");
                    navButton.Invoke();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True after clicking the navigation button");

                    Log.Comment("Verify that decreasing the width of the control from expanded to compact closes the pane");
                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False after decreasing the width of the control from expanded to compact");

                    Log.Comment("Verify that increasing the width of the control from compact to expanded opens the pane");
                    SetNavViewWidth(ControlWidth.Wide);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True after increasing the width of the control from compact to expanded");
                }
            }
        }

        [TestMethod]
        public void PaneNotOpeningTopMode()
        {

            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
                {
                    Log.Warning("Test is disabled on pre-RS4 because NavigationView Gamepad interaction is not supported pre-RS4");
                    return;
                }
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView TopNav Test" }))
                {
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be false");

                    GamepadHelper.PressButton(FindElement.ById("NavView"), GamepadButton.View);

                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True after increasing the width of the control from compact to expanded");
                }
            }
        }

        [TestMethod] // Bug 18159731
        public void PaneOpenForceCloseTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");

                    Button navButton = new Button(FindElement.ById("TogglePaneButton"));

                    Log.Comment("Verify that clicking the navigation button closes the nav pane");
                    navButton.Invoke();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False after clicking the navigation button");

                    TextBlock displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));

                    Log.Comment("Verify that decreasing the width of the control from expanded to Narrow and force closed pane");
                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False after decreasing the width of the control from expanded to Narrow");
                    Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                    Log.Comment("Verify that decreasing the width of the control from Narrow to compact and force closed pane");
                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False after decreasing the width of the control from Narrow to compact");
                    Verify.AreEqual(compact, displayModeTextBox.DocumentText);

                    Log.Comment("Verify that increasing the width of the control from compact to expanded and force closed pane");
                    SetNavViewWidth(ControlWidth.Wide);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False after increasing the width of the control from compact to expanded");
                    Verify.AreEqual(expanded, displayModeTextBox.DocumentText);
                }
            }
        }

        [TestMethod]
        public void VerifyPaneTitlePresentAndUpdates()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    TextBlock paneTitleTextBlock = new TextBlock(FindElement.ByName("NavView Test"));

                    Button changePaneTitleButton = new Button(FindElement.ByName("ChangePaneTitleButton"));
                    changePaneTitleButton.Invoke();
                    Wait.ForIdle();

                    Verify.AreEqual("", paneTitleTextBlock.DocumentText, "Verify that the pane title is empty");
                }
            }
        }

        [TestMethod]
        public void VerifyPaneVisibleOnInit()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Init Test" }))
            {
                Log.Comment("Verify PaneIsVisibleItem is invisible");
                VerifyElement.NotFound("PaneIsVisibleItem", FindBy.Name);

                FindElement.ByName<Button>("ChangePaneVisible").Invoke();
                Wait.ForIdle();

                Log.Comment("Verify PaneIsVisibleItem is visible");
                VerifyElement.Found("PaneIsVisibleItem", FindBy.Name);

            }
        }

        [TestMethod]
        public void PaneFooterContentTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                Log.Comment("Verify that button in PaneFooterContent exists");
                VerifyElement.Found("FooterButton", FindBy.Id);
            }
        }

        [TestMethod]
        public void VerifyPaneIsClosedWhenClickingOnSelectedItem()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                var displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));

                Log.Comment("Test PaneDisplayMode=LeftMinimal");
                panelDisplayModeComboBox.SelectItemByName("LeftMinimal");
                Wait.ForIdle();

                WaitAndAssertPaneStatus(PaneOpenStatus.Closed);

                Log.Comment("Click on ToggleButton");
                Button navButton = new Button(FindElement.ById("TogglePaneButton"));
                navButton.Invoke();
                Wait.ForIdle();

                WaitAndAssertPaneStatus(PaneOpenStatus.Opened);

                Log.Comment("Select Apps");
                UIObject appsItem = FindElement.ByName("Apps");
                appsItem.Click();
                Wait.ForIdle();

                WaitAndAssertPaneStatus(PaneOpenStatus.Closed);

                Log.Comment("Click on ToggleButton");
                navButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Click on SelectedItem Apps");
                appsItem.Click();
                Wait.ForIdle();

                WaitAndAssertPaneStatus(PaneOpenStatus.Closed);
            }
        }

        [TestMethod]
        public void VerifyPaneIsClosedAfterSelectingItem()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                var displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));

                Log.Comment("Test PaneDisplayMode=LeftMinimal");
                panelDisplayModeComboBox.SelectItemByName("LeftMinimal");
                Wait.ForIdle();

                WaitAndAssertPaneStatus(PaneOpenStatus.Closed);

                Log.Comment("Click on ToggleButton");
                Button navButton = new Button(FindElement.ById("TogglePaneButton"));
                navButton.Invoke();
                Wait.ForIdle();

                WaitAndAssertPaneStatus(PaneOpenStatus.Opened);

                TextBlock selectionRaisedIndicator = new TextBlock(FindElement.ById("SelectionChangedRaised"));

                Log.Comment("Select Apps");
                ComboBox selectedItem = new ComboBox(FindElement.ById("SelectedItemCombobox"));
                selectedItem.SelectItemByName("Apps");
                Verify.AreEqual("True", selectionRaisedIndicator.GetText());
                Wait.ForIdle();

                WaitAndAssertPaneStatus(PaneOpenStatus.Closed);
            }
        }

        [TestMethod]
        public void PaneDisplayModeLeftLeftCompactLeftMinimalTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                var displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));

                // Tests with PaneDisplayMode=Left/LeftCompact/LeftMinimal.
                // This disables all adaptive layout behavior.

                Log.Comment("Test PaneDisplayMode=Left");
                panelDisplayModeComboBox.SelectItemByName("Left");
                Wait.ForIdle();

                Log.Comment("DisplayMode should be 'Expanded' regardless of size");
                SetNavViewWidth(ControlWidth.Narrow);
                Wait.ForIdle();
                Verify.AreEqual(expanded, displayModeTextBox.DocumentText);

                SetNavViewWidth(ControlWidth.Medium);
                Wait.ForIdle();
                Verify.AreEqual(expanded, displayModeTextBox.DocumentText);

                SetNavViewWidth(ControlWidth.Wide);
                Wait.ForIdle();
                Verify.AreEqual(expanded, displayModeTextBox.DocumentText);


                Log.Comment("Test PaneDisplayMode=LeftCompact");
                panelDisplayModeComboBox.SelectItemByName("LeftCompact");
                Wait.ForIdle();

                Log.Comment("DisplayMode should be 'Compact' regardless of size");
                SetNavViewWidth(ControlWidth.Narrow);
                Wait.ForIdle();
                Verify.AreEqual(compact, displayModeTextBox.DocumentText);

                SetNavViewWidth(ControlWidth.Medium);
                Wait.ForIdle();
                Verify.AreEqual(compact, displayModeTextBox.DocumentText);

                SetNavViewWidth(ControlWidth.Wide);
                Wait.ForIdle();
                Verify.AreEqual(compact, displayModeTextBox.DocumentText);


                Log.Comment("Test PaneDisplayMode=LeftMinimal");
                panelDisplayModeComboBox.SelectItemByName("LeftMinimal");
                Wait.ForIdle();

                Log.Comment("DisplayMode should be 'Minimal' regardless of size");
                SetNavViewWidth(ControlWidth.Narrow);
                Wait.ForIdle();
                Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                SetNavViewWidth(ControlWidth.Medium);
                Wait.ForIdle();
                Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                SetNavViewWidth(ControlWidth.Wide);
                Wait.ForIdle();
                Verify.AreEqual(minimal, displayModeTextBox.DocumentText);
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125 and internal issue 19342138
        public void EnsurePaneCanBeHidden()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                var paneRoot = FindElement.ById("PaneRoot");
                Verify.IsFalse(paneRoot.IsOffscreen);

                var paneVisibleCheckBox = new CheckBox(FindElement.ByName("IsPaneVisibleCheckBox"));
                paneVisibleCheckBox.Uncheck();
                Wait.ForIdle();

                Verify.IsTrue(paneRoot.IsOffscreen);
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125 and internal issue 19342138
        public void EnsurePaneCanBeHiddenWithFixedWindowSize()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                var paneRoot = FindElement.ById("PaneRoot");
                Verify.IsFalse(paneRoot.IsOffscreen);

                SetNavViewWidth(ControlWidth.Wide);

                var paneVisibleCheckBox = new CheckBox(FindElement.ByName("IsPaneVisibleCheckBox"));

                paneVisibleCheckBox.Uncheck();
                Wait.ForIdle();
                Verify.IsTrue(paneRoot.IsOffscreen);

                paneVisibleCheckBox.Check();
                Wait.ForIdle();
                Verify.IsFalse(paneRoot.IsOffscreen);
            }
        }

        [TestMethod]
        public void VerifyIconsRespectCompactPaneLength()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView compact pane length test" }))
            {
                var checkMenuItemsButton = FindElement.ByName("CheckMenuItemsOffset");
                var compactpaneCheckbox = new ComboBox(FindElement.ByName("CompactPaneLengthComboBox"));
                var displayModeToggle = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                var currentStatus = new CheckBox(FindElement.ByName("MenuItemsCorrectOffset"));

                checkMenuItemsButton.Click();
                Log.Comment("Verifying compact pane length set before loading working");
                Wait.ForIdle();
                compactpaneCheckbox = new ComboBox(FindElement.ByName("CompactPaneLengthComboBox"));
                Verify.IsTrue(currentStatus.ToggleState == ToggleState.On);

                Log.Comment("Verifying changing compact pane length in left mode working");
                compactpaneCheckbox.SelectItemByName("96");
                Wait.ForIdle();

                checkMenuItemsButton.Click();
                Wait.ForIdle();
                compactpaneCheckbox = new ComboBox(FindElement.ByName("CompactPaneLengthComboBox"));
                Verify.IsTrue(currentStatus.ToggleState == ToggleState.On);

                // Check if changing displaymode to top and then changing length gets used correctly
                Log.Comment("Verifying changing compact pane length during top mode working");
                displayModeToggle.SelectItemByName("Top");
                compactpaneCheckbox.SelectItemByName("48");
                Wait.ForIdle();

                displayModeToggle.SelectItemByName("Left");
                Wait.ForIdle();

                checkMenuItemsButton.Click();
                Wait.ForIdle();
                compactpaneCheckbox = new ComboBox(FindElement.ByName("CompactPaneLengthComboBox"));
                Verify.IsTrue(currentStatus.ToggleState == ToggleState.On);
            }
        }

        [TestMethod]
        public void VerifyCorrectVisualStateWhenClosingPaneInLeftDisplayMode()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                // Test for explicit pane close

                // make sure the NavigationView is in left mode with pane expanded
                Log.Comment("Change display mode to left expanded");
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                panelDisplayModeComboBox.SelectItemByName("Left");
                Wait.ForIdle();

                TextBlock displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));
                Verify.AreEqual(expanded, displayModeTextBox.DocumentText);

                Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));

                // manually close pane
                Log.Comment("Close NavView pane explicitly");
                togglePaneButton.Invoke();
                Wait.ForIdle();

                WaitAndAssertPaneStatus(PaneOpenStatus.Closed);

                Log.Comment("Get NavView Active VisualStates");
                var getNavViewActiveVisualStatesButton = new Button(FindElement.ByName("GetNavViewActiveVisualStates"));
                getNavViewActiveVisualStatesButton.Invoke();
                Wait.ForIdle();

                // check visual state
                var visualStateName = "ListSizeCompact";
                var result = new TextBlock(FindElement.ByName("NavViewActiveVisualStatesResult"));

                Verify.IsTrue(result.GetText().Contains(visualStateName), "active VisualStates doesn't include " + visualStateName);

                // Test for light dismiss pane close

                Log.Comment("Change display mode to left compact");
                panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                panelDisplayModeComboBox.SelectItemByName("LeftCompact");
                Wait.ForIdle();

                displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));
                Verify.AreEqual(compact, displayModeTextBox.DocumentText);

                // expand pane
                Log.Comment("Expand NavView pane");
                togglePaneButton.Invoke();
                Wait.ForIdle();

                WaitAndAssertPaneStatus(PaneOpenStatus.Opened);

                // light dismiss pane
                Log.Comment("Light dismiss NavView pane");
                getNavViewActiveVisualStatesButton.Click(); // NOTE: Must be Click because this is verifying that the mouse light dismiss behavior closes the nav view
                Wait.ForIdle();

                WaitAndAssertPaneStatus(PaneOpenStatus.Closed);

                Log.Comment("Get NavView Active VisualStates");
                getNavViewActiveVisualStatesButton.Invoke();
                Wait.ForIdle();

                // check visual state
                result = new TextBlock(FindElement.ByName("NavViewActiveVisualStatesResult"));
                Verify.IsTrue(result.GetText().Contains(visualStateName), "active VisualStates doesn't include " + visualStateName);
            }
        }

        [TestMethod]
        [TestProperty("Description", "VisualState DisplayModeGroup is decoupled from DisplayMode, and it has strong connection with PaneDisplayMode")]
        public void VerifyCorrectVisualStateWhenChangingPaneDisplayMode()
        {
            // We expect this mapping:
            //  Top, and LeftMinimal -> VisualState Minimal
            //  LeftCompact -> VisualState Compact
            //  Left -> VisualState Expanded

            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "Top NavigationView Test" }))
            {
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                var getActiveVisualStateButton = new Button(FindElement.ByName("GetActiveVisualState"));
                var invokeResult = new Edit(FindElement.ById("TestResult"));
                var isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                Log.Comment("Set PaneDisplayMode to Top");
                panelDisplayModeComboBox.SelectItemByName("Top");
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("Minimal"));

                Log.Comment("Set PaneDisplayMode to Left");
                panelDisplayModeComboBox.SelectItemByName("Left");
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("Expanded"));

                Log.Comment("Set PaneDisplayMode to Top");
                panelDisplayModeComboBox.SelectItemByName("Top");
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("Minimal"));

                Log.Comment("Set PaneDisplayMode to LeftCompact");
                panelDisplayModeComboBox.SelectItemByName("LeftCompact");
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("Compact"));

                Log.Comment("Set PaneDisplayMode to LeftMinimal");
                panelDisplayModeComboBox.SelectItemByName("LeftMinimal");

                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("Minimal"));
                Log.Comment("Verify Pane is closed automatically when PaneDisplayMode is Minimal");
                Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False when PaneDisplayMode is Minimal");

                Log.Comment("Set DisplayMode to Left");
                panelDisplayModeComboBox.SelectItemByName("Left");
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("Expanded"));
                Log.Comment("Verify Pane is opened automatically when PaneDisplayMode is changed from Minimal to Left");
                Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True when PaneDisplayMode is changed from Minimal to Left");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the back button is visible when the pane is closed and the close button is visible when the pane is open, in LeftMinimal pane display mode")]
        public void VerifyBackAndCloseButtonsVisibilityInLeftMinimalPaneDisplayMode()
        {
            VerifyBackAndCloseButtonsVisibility(inLeftMinimalPanelDisplayMode: true);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the close button is visible when the pane is open, in Auto pane display mode and Minimal display mode")]
        public void VerifyBackAndCloseButtonsVisibilityInAutoPaneDisplayMode()
        {
            VerifyBackAndCloseButtonsVisibility(inLeftMinimalPanelDisplayMode: false);
        }

        [TestMethod] // bug 16644730
        public void VerifySettingsWidthOnLeftNavMediumMode()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                SetNavViewWidth(ControlWidth.Wide);
                Wait.ForIdle();

                Log.Comment("Bring Settings into view.");
                FindElement.ByName<Button>("BringSettingsIntoViewButton").Invoke();
                Wait.ForIdle();

                Button navButton = new Button(FindElement.ById("SettingsItem"));
                Log.Comment("Verify that the SettingsItem size in Expanded mode and actual width is " + navButton.BoundingRectangle.Width);

                // NavigationViewCompactPaneLength is 40 or 48 in different release. This test case doesn't need an exactly number of width, so just choose 48 as the boundary
                Verify.IsTrue(navButton.BoundingRectangle.Width > 48);

                SetNavViewWidth(ControlWidth.Medium);
                Wait.ForIdle();

                Log.Comment("Verify that the SettingsItem size in Medium mode and actual width is " + navButton.BoundingRectangle.Width);
                Verify.IsTrue(navButton.BoundingRectangle.Width <= 48);
            }
        }

        [TestMethod]
        public void LightDismissTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
                    {
                        Log.Warning("Test is disabled on RS1 and older");
                        return;
                    }

                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");

                    SetNavViewWidth(ControlWidth.Medium);
                    WaitAndAssertPaneStatus(PaneOpenStatus.Closed);

                    using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                    {
                        isPaneOpenCheckBox.Toggle();
                        waiter.Wait();
                    }
                    WaitAndAssertPaneStatus(PaneOpenStatus.Opened);

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");

                    PaneOpenCloseTestCaseRetry(3, () =>
                    {
                        KeyboardHelper.PressKey(Key.Backspace, ModifierKey.Windows);
                        Wait.ForIdle();
                        WaitAndAssertPaneStatus(PaneOpenStatus.Closed);
                        Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "Verify Windows+Back light dismisses the pane");
                    });

                    isPaneOpenCheckBox.Toggle();
                    Wait.ForIdle();
                    WaitAndAssertPaneStatus(PaneOpenStatus.Opened);
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");

                    PaneOpenCloseTestCaseRetry(3, () =>
                    {
                        KeyboardHelper.PressKey(Key.Left, ModifierKey.Alt);
                        Wait.ForIdle();
                        WaitAndAssertPaneStatus(PaneOpenStatus.Closed);
                        Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "Verify Alt+Left light dismisses the pane");
                    });

                    isPaneOpenCheckBox.Toggle();
                    Wait.ForIdle();
                    WaitAndAssertPaneStatus(PaneOpenStatus.Opened);
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");
                }
            }
        }

        [TestMethod]
        public void EnsureDynamicSizeForPaneHeaderFooterAndCustomContent()
        {
            if (PlatformConfiguration.IsDebugBuildConfiguration())
            {
                // Test is failing in chk configuration due to:
                // Bug #1734 NavigationViewTests.EnsureDynamicSizeForPaneHeaderFooterAndCustomContent fails in CHK configuration
                Log.Warning("Skipping test for Debug builds.");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Stretch Test" }))
            {
                Button navButton = new Button(FindElement.ById("TogglePaneButton"));

                // NavigationViewCompactPaneLength is 40 or 48 in different release. This test case doesn't need an exactly number of width, so just choose 48 as the boundary
                // PaneHeader share the same row with ToggleButton, so it's width is not the same with other buttons
                var widthCompactBoundary = 48;
                var widthOpenPaneLength = 320;

                Button paneHeaderButton = new Button(FindElement.ById("PaneHeader"));
                Log.Comment("PaneHeader size actual width is " + paneHeaderButton.BoundingRectangle.Width);
                Verify.IsTrue(paneHeaderButton.BoundingRectangle.Width > widthCompactBoundary && paneHeaderButton.BoundingRectangle.Width < widthOpenPaneLength);

                Button paneFooterButton = new Button(FindElement.ById("PaneFooter"));
                Log.Comment("PaneFooter size actual width is " + paneFooterButton.BoundingRectangle.Width);
                Verify.IsTrue(paneFooterButton.BoundingRectangle.Width == widthOpenPaneLength);

                Button paneCustomContentButton = new Button(FindElement.ById("PaneCustomContent"));
                Log.Comment("paneCustomContentButton size actual width is " + paneCustomContentButton.BoundingRectangle.Width);
                Verify.IsTrue(paneCustomContentButton.BoundingRectangle.Width == widthOpenPaneLength);

                Log.Comment("Verify that clicking the navigation button closes the nav pane");
                navButton.Invoke();
                Wait.ForIdle();

                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                {
                    // It returns negative width on RS1 and cause test failure.
                    // Error System.ArgumentException: Size_WidthAndHeightCannotBeNegative at 
                    // Microsoft.Windows.Apps.Test.Automation.UiaConvert.ConvertPropertyValue(AutomationProperty property, Object propertyValue) 
                    // at Microsoft.Windows.Apps.Test.Automation.AutomationElement.GetCurrentPropertyValue(AutomationProperty property, Boolean ignoreDefaultValue)
                    Log.Comment("Skip PaneHeader verification");
                }
                else
                {
                    paneHeaderButton = new Button(FindElement.ById("PaneHeader"));
                    Log.Comment("PaneHeader is collapsed");
                    Verify.IsTrue(paneHeaderButton.BoundingRectangle.Width == 0);
                }
                Log.Comment("PaneFooter size actual width is " + paneFooterButton.BoundingRectangle.Width);
                Verify.IsTrue(paneFooterButton.BoundingRectangle.Width <= widthCompactBoundary && paneFooterButton.BoundingRectangle.Width > 0);

                Log.Comment("paneCustomContentButton size actual width is " + paneCustomContentButton.BoundingRectangle.Width);
                Verify.IsTrue(paneCustomContentButton.BoundingRectangle.Width <= widthCompactBoundary && paneCustomContentButton.BoundingRectangle.Width > 0);
            }
        }
    }
}
