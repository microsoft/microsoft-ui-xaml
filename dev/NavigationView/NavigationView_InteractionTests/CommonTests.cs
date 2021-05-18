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
    public class CommonTests : NavigationViewTestsBase
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
        public void DisplayModeTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    TextBlock displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));

                    // Tests with PaneDisplayMode='Auto', which enables adaptive layout.

                    Log.Comment("Test the adaptive layout with the default Compact and Expanded mode Thresholds");
                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    Verify.AreEqual(compact, displayModeTextBox.DocumentText);

                    SetNavViewWidth(ControlWidth.Wide);
                    Wait.ForIdle();
                    Verify.AreEqual(expanded, displayModeTextBox.DocumentText);

                    Log.Comment("Test adaptive layout when the compact mode threshold is larger than the expanded mode threshold");
                    SetThreshold(Threshold.High, ComboBoxName.CompactModeComboBox);
                    SetThreshold(Threshold.Low, ComboBoxName.ExpandedModeComboBox);

                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    Verify.AreEqual(expanded, displayModeTextBox.DocumentText);

                    SetNavViewWidth(ControlWidth.Wide);
                    Wait.ForIdle();
                    Verify.AreEqual(expanded, displayModeTextBox.DocumentText);

                    Log.Comment("Test adaptive layout when the compact mode threshold is equal to the expanded mode threshold");
                    SetThreshold(Threshold.Low, ComboBoxName.CompactModeComboBox);
                    SetThreshold(Threshold.Low, ComboBoxName.ExpandedModeComboBox);

                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    Verify.AreEqual(expanded, displayModeTextBox.DocumentText);
                }
            }
        }

        [TestMethod]
        public void IsSettingsVisibleTest()
        {
            var testScenarios = RegressionTestScenario.BuildAllRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    Log.Comment("Verify that settings item is enabled by default");
                    VerifyElement.Found("Settings", FindBy.Name);

                    CheckBox settingsCheckbox = new CheckBox(FindElement.ByName("SettingsItemVisibilityCheckbox"));

                    Log.Comment("Verify that settings item is not visible when IsSettingsVisible == false");
                    settingsCheckbox.Uncheck();
                    ElementCache.Clear();
                    Wait.ForIdle();
                    VerifyElement.NotFound("Settings", FindBy.Name);

                    Log.Comment("Verify that settings item is visible when IsSettingsVisible == true");
                    settingsCheckbox.Check();
                    Wait.ForIdle();
                    VerifyElement.Found("Settings", FindBy.Name);
                }
            }
        }

        [TestMethod]
        public void IsPaneToggleButtonVisibleTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    Log.Comment("Verify that toggle button item is enabled by default");
                    VerifyElement.Found("TogglePaneButton", FindBy.Id);

                    CheckBox toggleCheckbox = new CheckBox(FindElement.ByName("PaneToggleButtonVisiblityCheckbox"));

                    Log.Comment("Verify that toggle button is not visible when IsPaneToggleButtonVisible == false");
                    toggleCheckbox.Uncheck();
                    Wait.ForIdle();
                    VerifyElement.NotFound("TogglePaneButton", FindBy.Id);

                    Log.Comment("Verify that settings item is visible when IsSettingsVisible == true");
                    toggleCheckbox.Check();
                    Wait.ForIdle();
                    VerifyElement.Found("SettingsItem", FindBy.Id);
                }
            }
        }

        [TestMethod]
        public void AlwaysShowHeaderTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    Log.Comment("Verify that header is visible by default");
                    VerifyElement.Found("Home as header", FindBy.Name);

                    CheckBox headerVisibilityCheckbox = new CheckBox(FindElement.ByName("HeaderVisiblityCheckbox"));

                    Log.Comment("Verify that header is not visible in display mode expanded when AlwaysShowHeader == false");
                    headerVisibilityCheckbox.Uncheck();
                    Wait.ForIdle();
                    VerifyElement.NotFound("Home as header", FindBy.Name);

                    Log.Comment("Verify that header is visible in display mode minimal when AlwaysShowHeader == false");
                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    VerifyElement.Found("Home as header", FindBy.Name);

                    Log.Comment("Verify that header is not visible in display mode compact when AlwaysShowHeader == false");
                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();
                    VerifyElement.NotFound("Home as header", FindBy.Name);

                    Log.Comment("Verify that header is visible in display mode compact when AlwaysShowHeader == true");
                    headerVisibilityCheckbox.Check();
                    Wait.ForIdle();
                    VerifyElement.Found("Home as header", FindBy.Name);

                    // PaneDisplayMode and Top option were added on RS5, so just run the next tests if we are not using RS4 Style
                    if (!testScenario.IsUsingRS4Style)
                    {
                        var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                        Log.Comment("Set PaneDisplayMode to Top");
                        panelDisplayModeComboBox.SelectItemByName("Top");
                        Wait.ForIdle();

                        Log.Comment("Verify that header is visible in Top display mode when AlwaysShowHeader == true");
                        VerifyElement.Found("Home as header", FindBy.Name);

                        Log.Comment("Verify that header is not visible in Top display mode when AlwaysShowHeader == false");
                        headerVisibilityCheckbox.Uncheck();
                        Wait.ForIdle();
                        VerifyElement.NotFound("Home as header", FindBy.Name);
                    }
                }
            }
        }

        [TestMethod]
        public void AddRemoveItemTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    var addButton = FindElement.ById<Button>("AddItemButton");
                    var removeButton = FindElement.ById<Button>("RemoveItemButton");

                    Log.Comment("Verify that menu items can be added");
                    addButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.Found("New Menu Item 0", FindBy.Name);

                    Log.Comment("Verify that more menu items can be added");
                    addButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.Found("New Menu Item 1", FindBy.Name);

                    Log.Comment("Verify that menu items can be removed");
                    removeButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.NotFound("New Menu Item 1", FindBy.Name);
                    VerifyElement.Found("New Menu Item 0", FindBy.Name);

                    Log.Comment("Verify that more menu items can be removed");
                    removeButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.NotFound("New Menu Item 0", FindBy.Name);
                }
            }
        }

        [TestMethod]
        public void AddRemoveOriginalItemTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    var addButton = FindElement.ById<Button>("AddItemButton");
                    var removeButton = FindElement.ById<Button>("RemoveItemButton");

                    Log.Comment("Verify that original menu items can be removed");
                    removeButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.NotFound("HasChildItem", FindBy.Name);

                    Log.Comment("Verify that menu items can be added after removing");
                    addButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.Found("New Menu Item 0", FindBy.Name);
                }
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void AddRemoveFooterItemTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    var addButton = FindElement.ById<Button>("AddFooterItemButton");
                    var removeButton = FindElement.ById<Button>("RemoveFooterItemButton");

                    Log.Comment("Verify that footer menu items can be added");
                    addButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.Found("New Footer Menu Item 0", FindBy.Name);

                    Log.Comment("Verify that more footer menu items can be added");
                    addButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.Found("New Footer Menu Item 1", FindBy.Name);

                    Log.Comment("Verify that footer menu items can be removed");
                    removeButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.NotFound("New Footer Menu Item 1", FindBy.Name);
                    VerifyElement.Found("New Footer Menu Item 0", FindBy.Name);

                    Log.Comment("Verify that more Footer menu items can be removed");
                    removeButton.Invoke();
                    Wait.ForIdle();
                    VerifyElement.NotFound("New Menu Item 0", FindBy.Name);
                }
            }
        }

        [TestMethod]
        public void ItemSourceTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Init Test" }))
            {
                var addButton = FindElement.ByName<Button>("AddItemButton");
                var removeButton = FindElement.ByName<Button>("RemoveItemButton");

                var addFooterButton = FindElement.ByName<Button>("AddFooterItemButton");
                var removeFooterButton = FindElement.ByName<Button>("RemoveFooterItemButton");

                Log.Comment("Verify that the MenuItemsSource was loaded and is selected");
                UIObject item1 = FindElement.ByName("Menu Item 1");
                Verify.IsNotNull(item1);
                Verify.IsTrue(Convert.ToBoolean(item1.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                Log.Comment("Verify that menu items added to MenuItemsSource appear in the list");
                addButton.Invoke();
                Wait.ForIdle();
                VerifyElement.Found("New Menu Item", FindBy.Name);

                Log.Comment("Verify that menu items removed from MenuItemsSource disappear from the list");
                removeButton.Invoke();
                Wait.ForIdle();
                VerifyElement.NotFound("New Menu Item", FindBy.Name);

                Log.Comment("Verify that footer menu items added to FooterMenuItemsSource appear in the list");
                addFooterButton.Invoke();
                Wait.ForIdle();
                VerifyElement.Found("New Footer Item", FindBy.Name);

                Log.Comment("Verify that footer menu items removed from FooterMenuItemsSource disappear from the list");
                removeFooterButton.Invoke();
                Wait.ForIdle();
                VerifyElement.NotFound("New Footer Item", FindBy.Name);
            }
        }


        [TestMethod]
        public void DisabledItemTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    UIObject moviesItem = FindElement.ByName("Movies");
                    CheckBox moviesEnabledCheckbox = new CheckBox(FindElement.ByName("MoviesEnabledCheckbox"));

                    Log.Comment("Verify that Movies item is enabled");
                    Verify.IsTrue(moviesItem.IsEnabled);

                    Log.Comment("Uncheck checkbox to disable Movies item");
                    moviesEnabledCheckbox.Uncheck();
                    Wait.ForIdle();

                    Log.Comment("Verify that Movies item is disabled");
                    Verify.IsFalse(moviesItem.IsEnabled);

                    Log.Comment("Check checkbox to enable Movies item");
                    moviesEnabledCheckbox.Check();
                    Wait.ForIdle();

                    Log.Comment("Verify that Movies item is enabled");
                    Verify.IsTrue(moviesItem.IsEnabled);
                }
            }
        }

        [TestMethod]
        public void AutoSuggestBoxTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    SetNavViewWidth(ControlWidth.Wide);
                    Wait.ForIdle();

                    Log.Comment("Verify that the AutoSuggestBox is visible and the search button is not in Expanded mode");
                    VerifyElement.Found("PaneAutoSuggestBox", FindBy.Name);
                    VerifyElement.NotFound("Click to search", FindBy.Name);

                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();

                    Log.Comment("Verify that the AutoSuggestBox is not visible and the search button is in Compact mode with the pane closed");
                    VerifyElement.NotFound("PaneAutoSuggestBox", FindBy.Name);
                    Button searchButton = new Button(FindElement.ByName("Click to search"));
                    Verify.IsNotNull(searchButton);

                    Log.Comment("Verify that invoking the search button opens the pane and put focus in the AutoSuggestBox");
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False");

                    searchButton.Invoke();
                    Wait.ForIdle();

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");

                    // AutoSuggestBox never gets keyboard focus itself; focus goes to the edit box inside it
                    UIObject autoSuggestEditBox = FindElement.ByNameAndClassName("PaneAutoSuggestBox", "TextBox");
                    Verify.IsNotNull(autoSuggestEditBox);
                    Verify.IsTrue(autoSuggestEditBox.HasKeyboardFocus);

                    Log.Comment("Verify that setting AutoSuggestBox to null removes it and the search button");
                    CheckBox autoSuggestCheckBox = new CheckBox(FindElement.ByName("AutoSuggestCheckbox"));
                    autoSuggestCheckBox.Uncheck();
                    Wait.ForIdle();

                    VerifyElement.NotFound("PaneAutoSuggestBox", FindBy.Name);
                    VerifyElement.NotFound("Click to search", FindBy.Name);

                    Log.Comment("Verify that setting AutoSuggestBox puts it back");
                    autoSuggestCheckBox.Check();
                    Wait.ForIdle();

                    VerifyElement.Found("PaneAutoSuggestBox", FindBy.Name);
                }
            }
        }

        [TestMethod]
        public void AutoSuggestBoxOnTopNavTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView TopNav Test" }))
            {
                ElementCache.Refresh();
                ElementCache.Dump();
                Log.Comment("Verify that the AutoSuggestBox is visible by default");
                VerifyElement.Found("PaneAutoSuggestBox", FindBy.Name);

                Log.Comment("Verify that setting AutoSuggestBox to null removes it and the search button");
                CheckBox autoSuggestCheckBox = new CheckBox(FindElement.ByName("AutoSuggestCheckbox"));
                autoSuggestCheckBox.Uncheck();
                Wait.ForIdle();

                VerifyElement.NotFound("PaneAutoSuggestBox", FindBy.Name);

                Log.Comment("Verify that setting AutoSuggestBox puts it back");
                autoSuggestCheckBox.Check();
                Wait.ForIdle();

                VerifyElement.Found("PaneAutoSuggestBox", FindBy.Name);
            }
        }

        [TestMethod] //bug 17792706
        public void BackButtonPlaceHolderOnTopNavTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "Top NavigationView Test" }))
            {
                var getActiveVisualStateButton = new Button(FindElement.ByName("GetActiveVisualState"));
                var invokeResult = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }

                Verify.IsTrue(invokeResult.Value.Contains("BackButtonVisible"));

                Log.Comment("Hide backbutton");
                var backButtonCheckBox = new CheckBox(FindElement.ByName("BackButtonVisibilityCheckbox"));

                backButtonCheckBox.Uncheck();
                Wait.ForIdle();

                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }

                Verify.IsTrue(invokeResult.Value.Contains("BackButtonCollapsed"));

                Log.Comment("Show backbutton");
                backButtonCheckBox.Check();
                Wait.ForIdle();

                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    getActiveVisualStateButton.Click();
                    waiter.Wait();
                }
                Verify.IsTrue(invokeResult.Value.Contains("BackButtonVisible"));
            }
        }

        [TestMethod]
        public void NavigationViewDensityChange()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                int height = FindElement.ById("AppsItem").BoundingRectangle.Height;
                Verify.AreEqual(height, 36);
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125 and internal issue 18650478
        public void TitleBarTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                    page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    CheckBox titleBarCheckbox = new CheckBox(FindElement.ByName("TitleBarCheckbox"));
                    CheckBox testFrameCheckbox = new CheckBox(FindElement.ByName("TestFrameCheckbox"));
                    Button navButton = new Button(FindElement.ById("TogglePaneButton"));
                    Button backButton = new Button(FindElement.ByName("NavigationViewBackButton"));
                    Button fullScreenButton = new Button(FindElement.ById("FullScreenInvokerButton"));

                    int testFrameBarHeight = FindElement.ById("__BackButton").BoundingRectangle.Height;

                    // The title bar is a peer of the CoreWindow, so we have to search one level up.
                    UIObject titleBar;
                    TestEnvironment.Application.CoreWindow.Parent.Descendants.TryFind(UICondition.CreateFromClassName("ApplicationFrameTitleBarWindow"), out titleBar);
                    int titleBarHeight = titleBar.BoundingRectangle.Height;

                    double backButtonSpace = backButton.BoundingRectangle.Height;
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                    {
                        backButtonSpace += 4; // back button is bigger on older OSes
                    }

                    int tabbedShellAffordance = 0;
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                    {
                        if (TestHelpers.SystemTabbedShellIsEnabled)
                        {
                            titleBarHeight = 0;
                            tabbedShellAffordance = testFrameBarHeight;
                            Log.Comment("Tabbed shell is enabled, adjusting expectations...");
                        }
                    }

                    Log.Comment("Test scenario: Standard NavView control.");
                    Log.Comment("Verify that the toggle button y = height of title bar + height of test frame bar + spacing of back button");
                    Verify.AreEqual(testFrameBarHeight + titleBarHeight + backButtonSpace + tabbedShellAffordance, navButton.BoundingRectangle.Y);

                    Log.Comment("Test scenario: ExtendViewIntoTitleBar:");
                    titleBarCheckbox.Uncheck();
                    Wait.ForIdle();

                    Log.Comment("Verify that the toggle button y = height of test frame bar + back button spacing");

                    TestEnvironment.VerifyAreEqualWithRetry(20,
                        () => (int)(testFrameBarHeight + backButtonSpace + tabbedShellAffordance),
                        () => (int)(navButton.BoundingRectangle.Y));


                    Log.Comment("Test scenario: Hide TestFrame:");
                    titleBarCheckbox.Check();
                    testFrameCheckbox.Uncheck();
                    Wait.ForIdle();

                    Log.Comment("Verify that the toggle button y = height of title bar + back button spacing");
                    Verify.AreEqual(titleBarHeight + backButtonSpace + tabbedShellAffordance, navButton.BoundingRectangle.Y);


                    Log.Comment("Test scenario: Hide TestFrame and ExtendViewIntoTitleBar:");
                    titleBarCheckbox.Uncheck();
                    Wait.ForIdle();

                    if (!testScenario.IsUsingRS4Style)
                    {
                        // If we extend the backbutton to titlebar area, the button is not clickable. so the new implementation keeps backbutton not in titlebar area.
                        Log.Comment("Verify that the toggle button y = height of title bar + back button spacing");
                        Verify.AreEqual(titleBarHeight + backButtonSpace + tabbedShellAffordance, navButton.BoundingRectangle.Y);
                    }
                    else
                    {
                        // To maintain back compat we maintain RS4 behavior when using the RS4 style:
                        Log.Comment("Verify that the toggle button y = back button spacing");
                        Verify.AreEqual(backButtonSpace + tabbedShellAffordance, navButton.BoundingRectangle.Y);
                    }

                    Log.Comment("Test scenario: Fullscreen mode:");
                    Log.Comment("Invoking fullscreen button:");
                    fullScreenButton.Invoke();
                    Wait.ForIdle();

                    TestEnvironment.VerifyAreEqualWithRetry(20,
                        () => 0,
                        () => backButton.BoundingRectangle.Y);
                    Log.Comment("Verify that the toggle button y = back button spacing");
                    Verify.AreEqual(backButtonSpace + tabbedShellAffordance, navButton.BoundingRectangle.Y);


                    fullScreenButton.Invoke();
                    Wait.ForIdle();
                    titleBarCheckbox.Check();
                    testFrameCheckbox.Check();
                }
            }
        }

        [TestMethod]
        public void VerifyBackButtonHidesWhenInMinimalOpenState()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    TextBlock displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));

                    Log.Comment("Test the adaptive layout with the default Compact and Expanded mode Thresholds");
                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    isPaneOpenCheckBox.Check();
                    Wait.ForIdle();

                    VerifyElement.NotFound("NavigationViewBackButton", FindBy.Name);
                }
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabling test as Reveal style is being deprecated
        public void LeftNavigationFocusKindRevealTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                {
                    Log.Warning("Test is disabled because FocusVisualKind.Reveal requires rs4+");
                    return;
                }

                Log.Comment("Invoke button GetHomeItemRevealVisualState");
                Button getHomeItemRevealVisualState = new Button(FindElement.ByName("GetHomeItemRevealVisualState"));
                getHomeItemRevealVisualState.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify No OnLeftNavigationReveal visualstate");
                var state = TryFindElement.ById("OnLeftNavigationRevealVisualState");
                Verify.AreEqual("False", state.GetText());

                Log.Comment("Change FocusVisualKind to Reveal");
                Button changeFocusVisualKind = new Button(FindElement.ByName("ChangeFocusVisualKind"));
                changeFocusVisualKind.Invoke();
                Wait.ForIdle();

                Log.Comment("Invoke button GetHomeItemRevealVisualState");
                getHomeItemRevealVisualState.Invoke();
                Wait.ForIdle();
                // changing FocusVisualKind impacts others test cases, and we need to change it back immediately.
                // So recover it to default before verify state1 visualstate
                var state1 = state.GetText();

                Log.Comment("Change FocusVisualKind to default");
                changeFocusVisualKind.Invoke();
                Wait.ForIdle();

                Log.Comment("Invoke button GetHomeItemRevealVisualState");
                getHomeItemRevealVisualState.Invoke();
                Wait.ForIdle();
                var state2 = state.GetText();

                Verify.AreEqual("True", state1, "There is OnLeftNavigationReveal visualstate");
                Verify.AreEqual("False", state2, "No OnLeftNavigationReveal visualstate");
            }
        }


        [TestMethod]
        public void ContentOverlayTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "Top NavigationView Test" }))
            {
                var topNavAutomationId = "TopNavMenuItemsHost";
                var contentOverlayName = "CONTENT OVERLAY";
                var addRemoveContentOverlayButton = new Button(FindElement.ById("AddRemoveContentOverlay"));
                var changeTopNavVisibilityButton = new Button(FindElement.ById("ChangeTopNavVisibility"));

                Log.Comment("Verify that after NavView loads, the top nav is visible and that there is no content overlay");
                VerifyElement.Found(topNavAutomationId, FindBy.Id);
                VerifyElement.NotFound(contentOverlayName, FindBy.Name);

                addRemoveContentOverlayButton.Click();
                Wait.ForIdle();

                Log.Comment("Verify that after content overlay is added, the top nav is visible and the content overlay too");
                VerifyElement.Found(topNavAutomationId, FindBy.Id);
                VerifyElement.Found(contentOverlayName, FindBy.Name);

                changeTopNavVisibilityButton.Click();
                Wait.ForIdle();

                Log.Comment("Verify that after setting IsPaneVisible to false, the top nav is hidden and the content overlay is still visible");
                VerifyElement.NotFound(topNavAutomationId, FindBy.Id);
                VerifyElement.Found(contentOverlayName, FindBy.Name);

                changeTopNavVisibilityButton.Click();
                Wait.ForIdle();

                Log.Comment("Verify that after setting IsPaneVisible to true, the top nav is visible and the content overlay remains visible");
                VerifyElement.Found(topNavAutomationId, FindBy.Id);
                VerifyElement.Found(contentOverlayName, FindBy.Name);
            }
        }

        [TestMethod]
        public void LeftNavigationWithAccessKeysTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    Log.Comment("Set control to compact");
                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();

                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                    Log.Comment("Verify that the pane is closed");
                    TestEnvironment.VerifyAreEqualWithRetry(20,
                        () => ToggleState.Off,
                        () => isPaneOpenCheckBox.ToggleState,
                        () =>
                        {
                            Task.Delay(TimeSpan.FromMilliseconds(100)).Wait(); // UIA's state isn't updating immediately. Wait a sec.
                            ElementCache.Clear(); /* Test is flaky sometimes -- perhaps element cache is stale? Clear it and try again. */
                        });

                    InvokeNavigationViewAccessKeyAndVerifyKeyTipPlacement("TogglePaneButton");

                    Log.Comment("Verify that the pane is open");
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");
                }
            }
        }

        [TestMethod]
        public void MenuItemKeyboardInvokeTest()
        {
            // On RS2 scrollviewer handles arrow keys and this causes an issue with the current setup of the "NavigationView Test" test page
            // used for the left NavigationView test. So instead we now execute this test on the "NavigationView Regression Test" test page for
            // left navigation.
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Regression Test" }))
            {
                Log.Comment("Verify the first menu item has focus and is selected");
                UIObject firstItem = FindElement.ByName("Home");
                firstItem.SetFocus();
                Verify.IsTrue(firstItem.HasKeyboardFocus);
                Verify.IsTrue(Convert.ToBoolean(firstItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                Log.Comment("Move focus to the second menu item by pressing down arrow");
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();

                Log.Comment("Verify second menu item has focus but is not selected");
                UIObject secondItem = FindElement.ByName("Apps");
                Verify.IsTrue(secondItem.HasKeyboardFocus);
                Verify.IsFalse(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));

                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                {
                    Log.Comment("Select the second item by pressing enter");
                    KeyboardHelper.PressKey(Key.Enter);
                    Wait.ForIdle();
                    Verify.IsTrue(Convert.ToBoolean(secondItem.GetProperty(UIProperty.Get("SelectionItem.IsSelected"))));
                }
                else
                {
                    Log.Warning("Full test is not executing due to lack of selection on keyboard selection behaviour in older versions of ListView");
                }
            }
        }

        // Like bug 17517627, Customer like WallPaper Studio 10 expects a HeaderContent visual even if Header() is null. 
        // App crashes when they have dependency on that visual, but the crash is not directly state that it's a header problem.   
        // NavigationView doesn't use quirk, but we determine the version by themeresource.
        // As a workaround, we 'quirk' it for RS4 or before release. if it's RS4 or before, HeaderVisible is not related to Header().
        [TestMethod]
        public void HeaderIsVisibleForTargetRS4OrBelowApp()
        {
            if (!(PlatformConfiguration.IsOsVersion(OSVersion.Redstone3) || PlatformConfiguration.IsOsVersion(OSVersion.Redstone4)))
            {
                Log.Warning("No need to run on rs2 and below or RS5+ (see method comment)");
                return;
            }
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Regression Test" }))
            {
                Button button = new Button(FindElement.ById("ClearHeaderButton"));
                var invokeResult = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(invokeResult))
                {
                    Log.Comment("Set Header to null");

                    button.Invoke();
                    waiter.Wait();
                }
                Verify.AreEqual(invokeResult.Value, "FoundHeaderContent");
            }
        }

        [TestMethod]
        public void SettingsAccessibilitySetTest()
        {
            var testScenarios = RegressionTestScenario.BuildAllRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    Log.Comment("Setting focus to Settings");
                    UIObject settingsItem = FindElement.ByName("Settings");
                    settingsItem.SetFocus();
                    Wait.ForIdle();

                    AutomationElement ae = AutomationElement.FocusedElement;
                    int positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                    int sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);

                    Verify.AreEqual(1, positionInSet, "Position in set");
                    Verify.AreEqual(1, sizeOfSet, "Size of set");

                    var addButton = FindElement.ById<Button>("AddFooterItemButton");
                    var removeButton = FindElement.ById<Button>("RemoveFooterItemButton");

                    addButton.Invoke();
                    addButton.Invoke();
                    Wait.ForIdle();

                    positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                    sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);

                    Verify.AreEqual(3, positionInSet, "Position in set");
                    Verify.AreEqual(3, sizeOfSet, "Size of set");

                    removeButton.Invoke();
                    removeButton.Invoke();
                }
            }
        }

        [TestMethod]
        public void ItemsAccessibilitySetTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    Log.Comment("Setting focus to Games");
                    UIObject gamesItem = FindElement.ByName("Games");
                    gamesItem.SetFocus();
                    Wait.ForIdle();

                    AutomationElement ae = AutomationElement.FocusedElement;
                    int positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                    int sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);

                    Verify.AreEqual(3, positionInSet, "Position in set");
                    Verify.AreEqual(3, sizeOfSet, "Size of set");

                    Log.Comment("Setting focus to Movies");
                    UIObject moviesItem = FindElement.ByName("Movies");
                    moviesItem.SetFocus();
                    Wait.ForIdle();

                    ae = AutomationElement.FocusedElement;
                    positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                    sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);

                    Verify.AreEqual(2, positionInSet, "Position in set, not including separator/header");
                    Verify.AreEqual(2, sizeOfSet, "Size of set");
                }
            }
        }

        [TestMethod]
        public void HierarchyItemsAccessibilitySetAndLevelTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "HierarchicalNavigationView Markup Test" }))
            {
                Log.Comment("Setting focus to Menu Item 15");
                UIObject menuItem15 = FindElement.ByName("Menu Item 15");
                menuItem15.SetFocus();
                Wait.ForIdle();

                AutomationElement ae = AutomationElement.FocusedElement;
                int positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                int sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);
                int itemLevel = (int)ae.GetCurrentPropertyValue(AutomationElement.LevelProperty);

                Verify.AreEqual(4, positionInSet, "Position in set");
                Verify.AreEqual(15, sizeOfSet, "Size of set");
                Verify.AreEqual(1, itemLevel, "Level of item");

                Log.Comment("Expanding Menu Item 15.");
                InputHelper.LeftClick(menuItem15);
                Wait.ForIdle();

                Log.Comment("Setting focus to Menu Item 17");
                UIObject menuItem17 = FindElement.ByName("Menu Item 17");
                menuItem17.SetFocus();
                Wait.ForIdle();

                ae = AutomationElement.FocusedElement;
                positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);
                itemLevel = (int)ae.GetCurrentPropertyValue(AutomationElement.LevelProperty);

                Verify.AreEqual(2, positionInSet, "Position in set, not including separator/header");
                Verify.AreEqual(3, sizeOfSet, "Size of set");
                Verify.AreEqual(2, itemLevel, "Level of item");
            }
        }

        [TestMethod]
        public void ItemsSourceAccessibilitySetTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Init Test" }))
            {
                Log.Comment("Set focus to second menu item");
                UIObject item = FindElement.ByName("Menu Item 2");
                item.SetFocus();
                Wait.ForIdle();

                AutomationElement ae = AutomationElement.FocusedElement;
                int positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                int sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);

                // choose 2nd item so if it accidentally succeeds
                // on firstor last item, we don't get false positives
                Verify.AreEqual(2, positionInSet, "Position in set");
                Verify.AreEqual(4, sizeOfSet, "Size of set");

                // Perform the test again with an IIterable (IEnumerable in C# projection)
                Button changeButton = new Button(FindElement.ByName("ChangeToIEnumerableButton"));
                changeButton.Invoke();
                Wait.ForIdle();
                ElementCache.Clear();

                Log.Comment("Set focus to second menu item [2]");
                item = FindElement.ByName("IIterator/Enumerable/LinkedList Item 2");
                item.SetFocus();
                Wait.ForIdle();

                ae = AutomationElement.FocusedElement;
                positionInSet = (int)ae.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                sizeOfSet = (int)ae.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);

                // choose 2nd item so if it accidentally succeeds
                // on firstor last item, we don't get false positives
                Verify.AreEqual(2, positionInSet, "Position in set [2]");
                Verify.AreEqual(3, sizeOfSet, "Size of set [2]");
            }
        }

        [TestMethod]
        public void SettingsItemGamepadTest()
        {
            var testScenarios = RegressionTestScenario.BuildAllRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone1))
                    {
                        Log.Warning("Test is disabled on TH2 and older due to lack of gamepad engagement feature.");
                        return;
                    }

                    UIObject settingsItem = FindElement.ByName("Settings");

                    settingsItem.SetFocus();
                    Wait.ForIdle();

                    Log.Comment("Invoking settings through the gamepad");
                    GamepadHelper.PressButton(settingsItem, GamepadButton.A);
                    Wait.ForIdle();

                    Log.Comment("Verify settings is selected");
                    TextBlock header = new TextBlock(FindElement.ByName("Settings as header"));
                    Verify.AreEqual("Settings as header", header.DocumentText);
                }
            }
        }

        [TestMethod]
        public void ScrollToMenuItemTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    //TODO: Re-enable for RS2 once repeater behavior is fixed
                    if (PlatformConfiguration.IsOsVersion(OSVersion.Redstone2))
                    {
                        Log.Warning("Test is disabled on RS2 because repeater has a bug where arrow navigation scrolls instead of going to next item.");
                        return;
                    }

                    SetNavViewHeight(ControlHeight.Small);

                    UIObject lastItem = FindElement.ByName("TV");
                    UIObject firstItem = FindElement.ByName("Home");

                    Log.Comment("Verify last item is offscreen");
                    Verify.AreEqual(true, lastItem.IsOffscreen);

                    Log.Comment("Move focus to the last item by giving focus to the toggle pane button then tab twice then down arrow 5 times");
                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));

                    togglePaneButton.SetFocus();
                    Wait.ForIdle();

                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.None, 2);
                    Wait.ForIdle();

                    Verify.IsTrue(firstItem.HasKeyboardFocus);
                    AutomationElement firstItemAE = AutomationElement.FocusedElement;
                    KeyboardHelper.PressKey(Key.Down, ModifierKey.None, 5);
                    Wait.ForIdle();

                    Verify.IsTrue(lastItem.HasKeyboardFocus, "Verify last item has keyboard focus");

                    Log.Comment("Verify the last item is now onscreen");
                    Verify.AreEqual(false, lastItem.IsOffscreen);
                    Log.Comment("Verify the first item is now offscreen");
                    Verify.AreEqual(true, firstItem.IsOffscreen);

                    Log.Comment("Scroll to the first item using the automation ScrollItemPattern");
                    ScrollItemPattern firstItemSIP = firstItemAE.GetCurrentPattern(ScrollItemPattern.Pattern) as ScrollItemPattern;
                    firstItemSIP.ScrollIntoView();
                    Wait.ForIdle();

                    Log.Comment("Verify the first item is onscreen again");
                    Verify.AreEqual(false, firstItem.IsOffscreen);
                }
            }
        }

        [TestMethod]
        public void SystemBackTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    Log.Comment("Set control to compact");
                    SetNavViewWidth(ControlWidth.Medium);
                    Wait.ForIdle();

                    Log.Comment("Open pane");
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    isPaneOpenCheckBox.Check();
                    Wait.ForIdle();

                    Log.Comment("Click back button (invoking does not work)");
                    Button backButton = new Button(FindElement.ById("__BackButton"));
                    backButton.Click(); // NOTE: Must be Click because this is verifying that the mouse light dismiss behavior closes the nav view
                    Wait.ForIdle();

                    Log.Comment("Verify that the pane is closed");

                    TestEnvironment.VerifyAreEqualWithRetry(20,
                        () => ToggleState.Off,
                        () => isPaneOpenCheckBox.ToggleState,
                        () =>
                        {
                            Task.Delay(TimeSpan.FromMilliseconds(100)).Wait(); // UIA's state isn't updating immediately. Wait a sec.
                            ElementCache.Clear(); /* Test is flaky sometimes -- perhaps element cache is stale? Clear it and try again. */
                        });

                    Log.Comment("Click pane toggle button");
                    Button navButton = new Button(FindElement.ById("TogglePaneButton"));
                    navButton.Invoke();
                    Wait.ForIdle();

                    Log.Comment("Verify that the pane is open");
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");
                }
            }
        }

        [TestMethod]
        public void AccTypeTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    UIObject menuItem = FindElement.ByName("Games");
                    Verify.AreEqual(ControlType.ListItem, menuItem.ControlType);
                }
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125
        public void ToolTipTest() // Verify tooltips appear, and that their contents change when headers change
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                    page2 = new TestSetupHelper(testScenario.TestPageName))
                {
                    // Close the pane
                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    togglePaneButton.Invoke();
                    Wait.ForIdle();

                    UIObject menuItem = FindElement.ByName("Games");
                    Verify.AreEqual(ControlType.ListItem, menuItem.ControlType);

                    Button reverseButton = new Button(FindElement.ById("ReverseButton"));

                    Button copyTextButton = new Button(FindElement.ById("CopyGamesLabelButton"));
                    copyTextButton.Invoke();
                    Wait.ForIdle();

                    TextBlock toolTipStatusTextBlock = new TextBlock(FindElement.ByName("ToolTipStatusTextBlock"));
                    Verify.AreEqual("There are no popups", toolTipStatusTextBlock.DocumentText);

                    using (var waiter = new ToolTipOpenedWaiter())
                    {
                        menuItem.Click(); // I think the underlying SinglePointGesture doesn't do what you expect sometimes unless you click first
                        menuItem.MovePointer();
                        Log.Comment("Waiting for tooltip open event");
                        if (waiter != null)
                        {
                            waiter.Wait();
                        }

                        copyTextButton.Invoke();

                        Verify.AreEqual("Games", toolTipStatusTextBlock.DocumentText); // Verify default case
                    }

                    reverseButton.MovePointer(); // dismissssss the tooltip
                    reverseButton.Invoke();
                    Wait.ForIdle();

                    using (var waiter = new ToolTipOpenedWaiter())
                    {
                        menuItem.Click(); // I think the underlying SinglePointGesture doesn't do what you expect sometimes unless you click first
                        menuItem.MovePointer();
                        Log.Comment("Waiting for tooltip open event [2]");
                        if (waiter != null)
                        {
                            waiter.Wait();
                        }

                        copyTextButton.Invoke();

                        Verify.AreEqual("semaG", toolTipStatusTextBlock.DocumentText); // Verify tooltips change when content changes
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")]
        // Disabled due to: Multiple unreliable NavigationView tests #134
        public void KeyboardFocusToolTipTest() // Verify tooltips appear when Keyboard focused
        {
            using (IDisposable page1 = new TestSetupHelper("NavigationView Tests"),
                page2 = new TestSetupHelper("NavigationView Test"))
            {
                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                {
                    Log.Warning("Test is disabled on RS1 and earlier because XYFocusKeyboardNavigation is not supported.");
                    return;
                }

                SetNavViewWidth(ControlWidth.Medium);
                Wait.ForIdle();

                // ToolTip is not reliable on RS2, try many times
                bool foundToolTip = false;
                for (int i = 0; i < 5; i++)
                {
                    Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                    togglePaneButton.SetFocus();
                    Wait.ForIdle();

                    UIObject home = FindElement.ByName("Home");
                    Button copyTextButton = new Button(FindElement.ById("CopyGamesLabelButton"));
                    TextBlock toolTipStatusTextBlock = new TextBlock(FindElement.ByName("ToolTipStatusTextBlock"));

                    Log.Comment("tab from the TogglePaneButton goes to the search box");
                    KeyboardHelper.PressKey(Key.Tab);
                    Wait.ForIdle();

                    Log.Comment("Down key from search box goes to the first item");
                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();

                    Verify.IsTrue(home.HasKeyboardFocus);
                    using (var waiter = new ToolTipOpenedWaiter())
                    {
                        KeyboardHelper.PressKey(Key.Down);

                        Log.Comment("Waiting for tooltip open event");
                        waiter.Wait(TimeSpan.FromSeconds(5));
                    }

                    copyTextButton.Invoke();
                    Wait.ForIdle();
                    if (toolTipStatusTextBlock.DocumentText != null && !toolTipStatusTextBlock.DocumentText.Contains("popups"))
                    {
                        Log.Comment("Found ToolTip on Iteration " + i);
                        Verify.AreEqual("Apps", toolTipStatusTextBlock.DocumentText);
                        foundToolTip = true;
                        break;
                    }
                    else
                    {
                        Log.Comment("ToolTip Window may be closed, and didn't find ToolTip on Iteration " + i);
                    }

                }
                Verify.IsTrue(foundToolTip, "Found ToolTip");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125
        public void ToolTipCustomContentTest() // Verify tooltips don't appear for custom NavViewItems (split off due to CatGates timeout)
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                Log.Warning("We are running with RS4 resource, not need to run on rs2 or below");
                return;
            }

            // Since RS5, ToolTip is removed from ControlTemplate. and this test case can't be run on "NavigationView Tests" page 
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Regression Test" }))
            {
                // Close the pane
                Button togglePaneButton = new Button(FindElement.ById("TogglePaneButton"));
                Verify.IsNotNull(togglePaneButton, "Finding TogglePaneButton");
                togglePaneButton.Invoke();
                Wait.ForIdle();

                Button volumeToolTipCopyButton = new Button(FindElement.ByName("CopyVolumeToolTipButton"));
                Verify.IsNotNull(togglePaneButton, "Finding CopyVolumeToolTipButton");

                TextBlock toolTipStatusTextBlock = new TextBlock(FindElement.ByName("ToolTipStatusTextBlock"));
                Verify.IsNotNull(togglePaneButton, "Finding ToolTipStatusTextBlock");

                volumeToolTipCopyButton.Invoke();
                Wait.ForIdle();

                Verify.AreEqual("The volume navigation view item tooltip content is null", toolTipStatusTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void VerifyCustomHeaderContentTest()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    TextBlock tb = new TextBlock(FindElement.ByName("Home as header"));
                    Verify.AreEqual("Home as header", tb.DocumentText, "Verify initial Header text");

                    Button button = new Button(FindElement.ByName("SetHeaderButton"));
                    button.Invoke();
                    Wait.ForIdle();

                    tb = new TextBlock(FindElement.ByName("Bananas"));
                    Verify.AreEqual("Bananas", tb.DocumentText, "Verify new TextBlock child was set as the Header");
                }
            }
        }

        [TestMethod]
        public void BackToolTipTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                Button button = new Button(FindElement.ByName("NavigationViewBackButton"));
                CheckBox checkBox = new CheckBox(FindElement.ByName("BackButtonEnabledCheckbox"));

                checkBox.Check();
                Wait.ForIdle();

                using (var waiter = new ToolTipOpenedWaiter())
                {
                    Log.Comment("Moving pointer around, over back button");
                    button.MovePointer();
                    Wait.ForIdle();
                    button.MovePointer(offsetX: 1, offsetY: 1);
                    Wait.ForIdle();
                    button.MovePointer(offsetX: -1, offsetY: -1);
                    Wait.ForIdle();

                    Log.Comment("Waiting for tooltip to open");
                    waiter.Wait(TimeSpan.FromSeconds(5));
                }
            }
        }

        [TestMethod]
        public void CloseToolTipTest()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));

                Log.Comment("Set PaneDisplayMode to LeftMinimal");
                panelDisplayModeComboBox.SelectItemByName("LeftMinimal");
                Wait.ForIdle();

                var isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                Log.Comment("Open the pane");
                isPaneOpenCheckBox.Check();
                Wait.ForIdle();

                Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");

                Button closeButton = new Button(FindElement.ById("NavigationViewCloseButton"));
                Verify.IsNotNull(closeButton);

                using (var waiter = new ToolTipOpenedWaiter())
                {
                    Log.Comment("Moving pointer around, over close button");
                    closeButton.MovePointer();
                    Wait.ForIdle();
                    closeButton.MovePointer(offsetX: 1, offsetY: 1);
                    Wait.ForIdle();
                    closeButton.MovePointer(offsetX: -1, offsetY: -1);
                    Wait.ForIdle();

                    Log.Comment("Waiting for tooltip to open");
                    waiter.Wait(TimeSpan.FromSeconds(5));
                }
            }
        }

        [TestMethod]
        public void VerifyCanCancelClosing()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone3))
                    {
                        Log.Warning("Test is disabled on RS2 and older due to lack of SplitView events");
                        return;
                    }

                    SetNavViewWidth(ControlWidth.Medium);

                    var cancelClosingCheckbox = new CheckBox(FindElement.ById("CancelClosingEvents"));
                    cancelClosingCheckbox.Toggle();
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, cancelClosingCheckbox.ToggleState);

                    var isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False");

                    Log.Comment("Reset the event count");
                    new Button(FindElement.ById("ClosingEventCountResetButton")).Invoke();
                    Wait.ForIdle();

                    Log.Comment("Open the Pane");
                    using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                    {
                        isPaneOpenCheckBox.Toggle();
                        waiter.Wait();
                    }
                    WaitAndAssertPaneStatus(PaneOpenStatus.Opened);

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");

                    var closingCounts = new Edit(FindElement.ByName("ClosingEventCountTextBlock"));
                    var expectedString = "1-0";

                    //  trigger a light dismiss
                    KeyboardHelper.PressKey(Key.Left, ModifierKey.Alt);
                    Wait.ForIdle();

                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState,
                        "Verify Alt+Left light dismiss doesn't dismiss the pane when closing events are being canceled");

                    Verify.AreEqual(expectedString, closingCounts.GetText());
                }
            }
        }

        [TestMethod]
        public void EnsureClearingListIsSafe()
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    Button clearButton = new Button(FindElement.ByName("ClearMenuButton"));
                    Log.Comment("About to invoke list clear button");
                    clearButton.Invoke();
                    Log.Comment("About to wait for idle");
                    Wait.ForIdle();

                    // that's it, it's a stability test
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Ensure that the NavigationView button isn't running with rs3+ themeresource on when they're off :)")]
        public void VerifyNotShouldPreserveNavigationViewRS3Behavior() // Regression test to make sure that we aren't accidentally running quirks all the time
        {
            var testScenarios = RegressionTestScenario.BuildLeftNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                    {
                        Log.Warning("This test is only designed to run on RS4+ machines");
                        return;
                    }
                    else
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

                        Button navButton = new Button(FindElement.ById("TogglePaneButton"));
                        Verify.AreEqual(312, navButton.BoundingRectangle.Width);
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Ensure that the NavigationView button is rendering as expected if it's targeting RS3")]
        public void VerifyShouldPreserveNavigationViewRS3Behavior()
        {
            // This test exercises how the navigation view intereacts with the titlebar, thus setting shouldRestrictInnerFrameSize to true (the default) doesn't allow us
            // to exercise the scenario, thus requiring us to disable this. 
            using (var setup = new TestSetupHelper(testNames: new[] { "NavigationView Tests", "NavigationView PreserveRS3 Test" }, shouldRestrictInnerFrameSize: false))
            {
                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                {
                    Log.Warning("This test is only designed to run on RS4+ machines");
                    return;
                }

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

                // In RS4 or late application, togglePaneTopPadding is 0 when ExtendViewIntoTitleBar=true, 
                // but for RS3 application, we expected it be not 0 because apps like Wallpaper make use of it
                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button button = new Button(FindElement.ById("GetTopPaddingHeight"));
                    button.Invoke();
                    waiter.Wait();
                }
                var togglePaneTopPadding = Convert.ToInt32(result.Value);
                Verify.AreNotEqual(0, togglePaneTopPadding);

                // TestFrame is disabled before the testcase. we should enable it and prepare for next test case
                var testFrame = new CheckBox(FindElement.ById("TestFrameCheckbox"));
                testFrame.Check();
                Wait.ForIdle();
            }
        }

        [TestMethod]
        public void EnsurePaneHeaderCanBeModifiedForLeftNav()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                EnsurePaneHeaderCanBeModifiedHelper(RegressionTestType.LeftNav);
            }
        }

        [TestMethod]
        public void EnsurePaneHeaderCanBeModifiedForTopNav()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                EnsurePaneHeaderCanBeModifiedHelper(RegressionTestType.TopNav);
            }
        }

        [TestMethod]
        public void EnsureDisplayModeGroupUpdatesWhenBackButtonVisibilityChanged()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                Log.Comment("Setup test page to be in the minimal display mode with the backbutton hidden...");
                Log.Comment("Hide backbutton");
                var backButtonCheckBox = new CheckBox(FindElement.ByName("BackButtonVisibilityCheckbox"));
                backButtonCheckBox.Uncheck();
                Wait.ForIdle();

                Log.Comment("Change display mode to left minimal");
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                panelDisplayModeComboBox.SelectItemByName("LeftMinimal");
                Wait.ForIdle();

                TextBlock displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));
                Verify.AreEqual(minimal, displayModeTextBox.DocumentText);

                Log.Comment("Get NavView Active VisualStates");
                var getNavViewActiveVisualStatesButton = new Button(FindElement.ByName("GetNavViewActiveVisualStates"));
                getNavViewActiveVisualStatesButton.Invoke();
                Wait.ForIdle();

                const string visualStateName = "MinimalWithBackButton";
                var result = new TextBlock(FindElement.ByName("NavViewActiveVisualStatesResult"));
                Verify.IsFalse(result.GetText().Contains(visualStateName), "Active VisualStates should not include " + visualStateName);

                Log.Comment("Show backbutton");
                backButtonCheckBox.Check();
                Wait.ForIdle();

                Log.Comment("Get NavView Active VisualStates");
                getNavViewActiveVisualStatesButton.Invoke();
                Wait.ForIdle();
                Verify.IsTrue(result.GetText().Contains(visualStateName), "Active VisualStates should include " + visualStateName);
            }
        }

        [TestMethod]
        public void EnsureDisplayModeGroupUpdatesOnPaneClosedToMinimalWithBackButton()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "Navigation Minimal Test" }))
            {
                Log.Comment("Click on ToggleButton");
                FindElement.ById<Button>("TogglePaneButton").InvokeAndWait();

                Log.Comment("Get NavView Active VisualStates");
                FindElement.ByName<Button>("GetNavViewActiveVisualStates").InvokeAndWait();

                const string visualStateName = "MinimalWithBackButton";
                var result = new TextBlock(FindElement.ByName("NavViewActiveVisualStatesResult"));
                Verify.IsTrue(result.GetText().Contains(visualStateName), "Active VisualStates should include " + visualStateName);
            }
        }

        [TestMethod]
        public void VerifyExpandCollpaseFunctionality()
        {
            var testScenarios = RegressionTestScenario.BuildHierarchicalNavRegressionTestScenarios();
            foreach (var testScenario in testScenarios)
            {
                using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", testScenario.TestPageName }))
                {
                    
                    TextBlock textblockExpandingItemAndContainerMatch = new TextBlock(FindElement.ByName("TextblockExpandingItemAndContainerMatch"));
                    TextBlock textblockCollapsedItemAndContainerMatch = new TextBlock(FindElement.ByName("TextblockCollapsedItemAndContainerMatch"));
                    TextBlock textBlockExpandingItem = new TextBlock(FindElement.ByName("TextBlockExpandingItem"));
                    TextBlock textBlockCollapsedItem = new TextBlock(FindElement.ByName("TextBlockCollapsedItem"));

                    Log.Comment("Verify that first menu item is not expanded.");
                    // Verify through test page elements
                    Verify.AreEqual(textblockExpandingItemAndContainerMatch.DocumentText, "N/A");
                    Verify.AreEqual(textblockCollapsedItemAndContainerMatch.DocumentText, "N/A");
                    Verify.AreEqual(textBlockExpandingItem.DocumentText, "N/A");
                    Verify.AreEqual(textBlockCollapsedItem.DocumentText, "N/A");
                    // Verify that second menu item is not in tree
                    var firstItem = FindElement.ByName("Menu Item 1");
                    var childItem = FindElement.ByName("Menu Item 2");
                    Verify.IsNull(childItem);

                    Log.Comment("Expand first menu item.");
                    InputHelper.LeftClick(firstItem);
                    Wait.ForIdle();

                    Log.Comment("Verify that first menu item was expanded correctly.");
                    // Verify through test page elements
                    Verify.AreEqual(textblockExpandingItemAndContainerMatch.DocumentText, "true");
                    Verify.AreEqual(textblockCollapsedItemAndContainerMatch.DocumentText, "N/A");
                    Verify.AreEqual(textBlockExpandingItem.DocumentText, "Menu Item 1");
                    Verify.AreEqual(textBlockCollapsedItem.DocumentText, "N/A");
                    // Verify that second menu item is in tree
                    childItem = FindElement.ByName("Menu Item 2");
                    Verify.IsNotNull(childItem, "Child item should be visible after expanding parent item.");

                    Log.Comment("Expand child of first menu item.");
                    InputHelper.LeftClick(childItem);
                    Wait.ForIdle();

                    Log.Comment("Verify that child of first menu item was expanded correctly.");
                    // Verify through test page elements
                    Verify.AreEqual(textblockExpandingItemAndContainerMatch.DocumentText, "true");
                    Verify.AreEqual(textblockCollapsedItemAndContainerMatch.DocumentText, "N/A");
                    Verify.AreEqual(textBlockExpandingItem.DocumentText, "Menu Item 2");
                    Verify.AreEqual(textBlockCollapsedItem.DocumentText, "N/A");
                    // Verify that third child menu item is in tree
                    var secondChildItem = FindElement.ByName("Menu Item 4");
                    Verify.IsNotNull(secondChildItem, "Child item should be visible after expanding parent item.");

                    Log.Comment("Collapse child of first menu item.");
                    InputHelper.LeftClick(childItem, 5, 5);
                    Wait.ForIdle();

                    Log.Comment("Verify that child of first menu item was collapsed correctly.");
                    // Verify through test page elements
                    Verify.AreEqual(textblockExpandingItemAndContainerMatch.DocumentText, "true");
                    Verify.AreEqual(textblockCollapsedItemAndContainerMatch.DocumentText, "true");
                    Verify.AreEqual(textBlockExpandingItem.DocumentText, "Menu Item 2");
                    Verify.AreEqual(textBlockCollapsedItem.DocumentText, "Menu Item 2");


                    Log.Comment("Collapse first menu item.");
                    InputHelper.LeftClick(firstItem, 5, 5);
                    Wait.ForIdle();

                    Log.Comment("Verify that first menu item was collapsed correctly.");
                    // Verify through test page elements
                    Verify.AreEqual(textblockExpandingItemAndContainerMatch.DocumentText, "true");
                    Verify.AreEqual(textblockCollapsedItemAndContainerMatch.DocumentText, "true");
                    Verify.AreEqual(textBlockExpandingItem.DocumentText, "Menu Item 2");
                    Verify.AreEqual(textBlockCollapsedItem.DocumentText, "Menu Item 1");
                }
            }
        }

        [TestMethod]
        public void VerifyNoCrashWhenSwitchingPaneDisplayModeWithAutoWrappedElements()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView ItemTemplate Test" }))
            {
                Log.Comment("Verify that switching pane mode to auto does not crash.");
                var flipOrientationButton = new Button(FindElement.ByName("FlipOrientationButton"));
                flipOrientationButton.Invoke();
                Wait.ForIdle();
            }
        }

        [TestMethod]
        public void VerifyNoCrashWhenNavViewInContentDialog()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                Log.Comment("Open a ContentDialog with a NavView inside.");
                Button openContentDialogButton = new Button(FindElement.ById("ContentDialogNavViewButton"));
                openContentDialogButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Close the ContentDialog with a NavView inside.");
                Button closeContentDialogButton = new Button(FindElement.ByName("Button1ContentDialog"));
                closeContentDialogButton.Invoke();
                Wait.ForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // 32134869: Temporarily disabling 
        public void VerifyNavigationViewItemContentPresenterMargin()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                var getTopLevelContentPresenterMarginButton = FindElement.ById<Button>("GetTopLevelNavViewItemContentPresenterMarginButton");
                var getChildContentPresenterMarginButton = FindElement.ById<Button>("GetChildNavViewItemContentPresenterMarginButton");
                var contentPresenterMarginTextBlock = new TextBlock(FindElement.ByName("NavViewItemContentPresenterMarginTextBlock"));

                var scrollItemIntoViewComboBox = new ComboBox(FindElement.ByName("ScrollItemIntoViewComboBox"));
                var scrollItemIntoViewButton = FindElement.ById<Button>("ScrollItemIntoViewButton");

                // Switch the NavigationView to closed compact mode
                Log.Comment("Switch NavigationView to closed compact mode");
                SetNavViewWidth(ControlWidth.Medium);
                Wait.ForIdle();

                Log.Comment("Ensure test menu item is in view");
                scrollItemIntoViewComboBox.SelectItemByName("HasChildItem");
                Wait.ForIdle();
                scrollItemIntoViewButton.InvokeAndWait();

                // Verify that top-level items use the correct content margin
                getTopLevelContentPresenterMarginButton.InvokeAndWait();
                Verify.AreEqual("0,0,0,0", contentPresenterMarginTextBlock.DocumentText);

                // Child items in closed compact mode are shown in a flyout. Verify that they are using the correct margin 
                Log.Comment("Expand item with children");
                UIObject hasChildItem = FindElement.ByName("HasChildItem");
                InputHelper.LeftClick(hasChildItem);
                Wait.ForMilliseconds(100); // Give a little bit of time for the flyout to open, then wait for idle.
                Wait.ForIdle();

                getChildContentPresenterMarginButton.InvokeAndWait();
                Verify.AreEqual("4,0,20,0", contentPresenterMarginTextBlock.DocumentText);

                // Close opened flyout
                InputHelper.LeftClick(hasChildItem);
                Wait.ForIdle();

                // Switch the NavigationView to expanded mode
                Log.Comment("Switch NavigationView to expanded mode");
                SetNavViewWidth(ControlWidth.Wide);
                Wait.ForIdle();

                Log.Comment("Ensure test menu item is in view");
                scrollItemIntoViewComboBox.SelectItemByName("HasChildItem");
                Wait.ForIdle();
                scrollItemIntoViewButton.InvokeAndWait();

                // Verify that top-level items use the correct content margin
                getTopLevelContentPresenterMarginButton.InvokeAndWait();
                Verify.AreEqual("4,0,20,0", contentPresenterMarginTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void VerifyNavigationViewItemChildrenFlyoutMenuCornerRadius()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "HierarchicalNavigationView Markup Test" }))
            {
                Log.Comment("Set PaneDisplayMode to LeftCompact.");
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                panelDisplayModeComboBox.SelectItemByName("LeftCompact");
                Wait.ForIdle();

                VerifyChildrenFlyoutMenuCornerRadius();

                // Refresh the cache to make sure that the flyout object we are going to be searching for
                // does not return as a false positive due to the caching mechanism.
                ElementCache.Clear();

                Log.Comment("Set PaneDisplayMode to Top.");
                panelDisplayModeComboBox.SelectItemByName("Top");
                Wait.ForIdle();

                VerifyChildrenFlyoutMenuCornerRadius();

                void VerifyChildrenFlyoutMenuCornerRadius()
                {
                    Log.Comment("Verify that the children menu flyout used in this test is closed.");
                    var childItem = FindElement.ByName("Menu Item 2");
                    Verify.IsNull(childItem, "Menu Item 1's children menu flyout should have been closed.");

                    Log.Comment("Select Menu Item 1 which should open children flyout.");
                    var item = FindElement.ByName("Menu Item 1");
                    InputHelper.LeftClick(item);
                    Wait.ForIdle();

                    childItem = FindElement.ByName("Menu Item 2");
                    Verify.IsNotNull(childItem, "Menu Item 1's children menu flyout should have been open.");

                    Log.Comment("Get CornerRadius of Menu Item 1's children menu flyout.");
                    FindElement.ByName<Button>("GetMenuItem1ChildrenFlyoutCornerRadiusButton").Invoke();

                    // A CornerRadius of (8,8,8,8) is the current default value for flyouts.
                    TextBlock menuItem1ChildrenFlyoutCornerRadiusTextBlock = new TextBlock(FindElement.ByName("MenuItem1ChildrenFlyoutCornerRadiusTextBlock"));
                    Verify.AreEqual("8,8,8,8", menuItem1ChildrenFlyoutCornerRadiusTextBlock.DocumentText);

                    // Close flyout
                    InputHelper.LeftClick(item);
                    Wait.ForIdle();
                }
            }
        }

        [TestMethod]
        public void VerifyNavigationViewItemInPaneFooterHasTemplateSettingBindings()
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "PaneFooterTestPage" }))
            {
                TextBlock paneFooterNavViewItemWidthTextBlock = new TextBlock(FindElement.ByName("PaneFooterNavViewItemWidth"));
                ComboBox compactPaneLengthComboBox = new ComboBox(FindElement.ByName("CompactPaneLengthComboBox"));
                UIObject getNavViewItemWidth = FindElement.ByName("GetNavViewItemWidth");

                getNavViewItemWidth.Click();

                Log.Comment("Verify NavigationViewItem in PaneFooter has the correct width from templateSettings bindings");
                Verify.AreEqual("48", paneFooterNavViewItemWidthTextBlock.DocumentText);

                Log.Comment("Change CompactPaneLength to 40px");
                compactPaneLengthComboBox.SelectItemByName("40");
                Wait.ForIdle();
                getNavViewItemWidth.Click();
                Wait.ForIdle();

                Verify.AreEqual("40", paneFooterNavViewItemWidthTextBlock.DocumentText);

                Log.Comment("Change CompactPaneLength to 96px");
                compactPaneLengthComboBox.SelectItemByName("96");
                Wait.ForIdle();
                getNavViewItemWidth.Click();
                Wait.ForIdle();

                Verify.AreEqual("96", paneFooterNavViewItemWidthTextBlock.DocumentText);
            }
        }
    }
}
