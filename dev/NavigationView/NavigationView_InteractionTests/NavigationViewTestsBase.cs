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
    public class NavigationViewTestsBase
    {
        protected const string minimal = "Minimal";
        protected const string compact = "Compact";
        protected const string expanded = "Expanded";

        protected enum ControlWidth { Narrow, Medium, Wide }
        protected enum ControlHeight { Default, Small }
        protected enum Threshold { Low, High }
        protected enum ComboBoxName { CompactModeComboBox, ExpandedModeComboBox }
        protected enum TopNavPosition { Primary, Overflow }
        protected enum PaneOpenStatus { Opened, Closed }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        protected void SetNavViewWidth(ControlWidth width)
        {
            ComboBox widthComboBox = new ComboBox(FindElement.ByName("WidthComboBox"));
            string currentWidth = "unset";

            if (widthComboBox.Selection.Count > 0)
            {
                ComboBoxItem selectedComboBoxItem = widthComboBox.Selection[0];
                currentWidth = selectedComboBoxItem.Name;
                Log.Comment("Current width " + currentWidth);
            }
            string widthString = width.ToString();

            if (currentWidth != widthString)
            {
                Wait.ForIdle();
                Log.Comment("Changing to width " + widthString);
                widthComboBox.SelectItemByName(widthString);
            }
        }

        protected void SetNavViewHeight(ControlHeight height)
        {
            ComboBox heightComboBox = new ComboBox(FindElement.ByName("HeightCombobox"));
            string currentHeight = "Default";

            if (heightComboBox.Selection.Count > 0)
            {
                ComboBoxItem selectedComboBoxItem = heightComboBox.Selection[0];
                currentHeight = selectedComboBoxItem.Name;
                Log.Comment("Current height " + currentHeight);
            }

            Log.Comment("Changing height to " + height.ToString());
            heightComboBox.SelectItemByName(height.ToString());
        }

        protected void SetThreshold(Threshold threshold, ComboBoxName name)
        {
            ComboBox thresholdComboBox = new ComboBox(FindElement.ByName(name.ToString()));
            string currentThreshold = "unset";

            if (thresholdComboBox.Selection.Count > 0)
            {
                ComboBoxItem selectedComboBoxItem = thresholdComboBox.Selection[0];
                currentThreshold = selectedComboBoxItem.Name;
                Log.Comment("Current threshold " + currentThreshold);
            }
            string thresholdString = threshold.ToString();

            if (currentThreshold != thresholdString)
            {
                Log.Comment("Changing to width " + thresholdString);
                thresholdComboBox.SelectItemByName(thresholdString);
            }
        }

        protected void ClickClearSelectionButton()
        {
            Log.Comment("Clear the selection by set NavView.SelectedItem to null");
            var ClearSelectedItemButton = new Button(FindElement.ByName("ClearSelectedItemButton"));
            ClearSelectedItemButton.Invoke();
            Wait.ForIdle();
        }

        protected void WaitAndAssertPaneStatus(PaneOpenStatus status)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone3))
            {
                // PaneOpened and PaneClosed is introduced in RS3 for SplitView, and NavigationView depends on SplitView
                // So we can't wait for the pane to opened/closed event but just delay seconds to make animation complete.
                Wait.ForSeconds(2);

                var expectToggleStatus = status == PaneOpenStatus.Opened ? ToggleState.On : ToggleState.Off;
                CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                TestEnvironment.VerifyAreEqualWithRetry(60, // wait max to 3s
                    () => isPaneOpenCheckBox.ToggleState,
                    () => expectToggleStatus);
            }
            else
            {
                string expectString = status == PaneOpenStatus.Opened ? "Opened" : "Closed";
                var eventTextBlock = new TextBlock(FindElement.ByName("PaneOpenedOrClosedEvent"));

                Log.Comment("PaneOpenedOrClosedEvent before wait: " + eventTextBlock.GetText());
                TestEnvironment.VerifyAreEqualWithRetry(100, // wait max to 5s
                    () => expectString,
                    () => eventTextBlock.GetText());
            }
        }

        protected void EnsurePaneHeaderCanBeModifiedHelper(RegressionTestType navviewMode)
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on RS1 and earlier because Pane Header is on RS2.");
                return;
            }

            if (navviewMode == RegressionTestType.TopNav)
            {
                var flipOrientationButton = new Button(FindElement.ByName("FlipOrientationButton"));
                flipOrientationButton.Invoke();
                Wait.ForIdle();
            }

            var changePaneHeaderbutton = new Button(FindElement.ByName("ChangePaneHeader"));
            changePaneHeaderbutton.Invoke();
            Wait.ForIdle();

            UIObject paneHeaderContent = null;

            if (navviewMode == RegressionTestType.TopNav)
            {
                paneHeaderContent = FindElement.ById("PaneHeaderOnTopPane");
            }
            else
            {
                paneHeaderContent = FindElement.ById("PaneHeaderContentBorder");
            }

            TextBlock text = new TextBlock(paneHeaderContent.FirstChild);
            Verify.AreEqual("Modified Pane Header", text.DocumentText);

            if (navviewMode == RegressionTestType.LeftNav)
            {
                // In Closed Compact mode, the PaneHeader should not be visible:

                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                panelDisplayModeComboBox.SelectItemByName("LeftCompact");
                Wait.ForIdle();

                EnsureNavViewClosed();

                ElementCache.Clear();
                VerifyElement.NotFound("PaneHeaderContentBorder", FindBy.Name);
            }
        }

        protected void EnsureNavViewClosed()
        {
            CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
            if (isPaneOpenCheckBox.ToggleState == ToggleState.On)
            {
                using (var waiter = isPaneOpenCheckBox.GetToggledWaiter())
                {
                    isPaneOpenCheckBox.Uncheck();
                    waiter.Wait();
                }
            }
            Wait.ForIdle();
        }

        protected List<UIObject> GetTopNavigationItems(TopNavPosition position)
        {
            string hostId = position == TopNavPosition.Overflow ? "TopNavMenuItemsOverflowHost" : "TopNavMenuItemsHost";
            List<UIObject> collection = new List<UIObject>();

            var host = TryFindElement.ById(hostId);
            if (host != null)
            {
                collection.AddRange(host.Children);
            }
            else
            {
                Log.Warning("Can't find container " + hostId);
            }
            return collection;
        }

        protected void InvokeOverflowButton()
        {
            Log.Comment("Invoke More button to open/close Overflow menu");
            var moreButton = TryFindElement.ById("TopNavOverflowButton");
            Verify.IsNotNull(moreButton, "Overflow button should exist");
            new Button(moreButton).InvokeAndWait();
        }
        protected bool IsItemInTopNavPrimaryList(string text)
        {
            var list = TryFindElement.ById("TopNavMenuItemsHost");
            Verify.IsTrue(list != null, "TopNavMenuItemsHost exists");
            foreach (var item in list.Children)
            {
                if (item != null)
                {
                    foreach (var v in new List<string> { item.AutomationId, item.ClassName, item.Name })
                    {
                        if (v != null && v.Contains(text))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        protected void PaneOpenCloseTestCaseRetry(int retryNumber, Action action)
        {
            for (int i = 0; i < retryNumber; i++)
            {
                try
                {
                    if (i > 0)
                    {
                        Log.Comment("Retry on " + i);
                    }
                    action();
                    return;
                }
                catch (Exception e)
                {
                    CheckBox isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));
                    Log.Comment("IsPaneOpenCheckBox toggle status: " + isPaneOpenCheckBox.ToggleState);

                    Edit closingCounts = new Edit(FindElement.ByName("ClosingEventCountTextBlock"));
                    Log.Comment("ClosingEventCountTextBlock text: " + closingCounts.GetText());

                    TextBlock eventTextBlock = new TextBlock(FindElement.ByName("PaneOpenedOrClosedEvent"));
                    Log.Comment("PaneOpenedOrClosedEvent text: " + eventTextBlock.GetText());

                    Log.Comment(e.Message);
                }
            }

            throw new Exception("Reach max number of retry " + retryNumber);
        }

        protected string UIObjectToString(UIObject uIObject)
        {
            return (uIObject == null) ? "" :
                string.Join("/",
                    new[] { uIObject.Name, uIObject.ClassName, uIObject.AutomationId }.
                        Where(s => s != null));
        }

        protected bool UIObjectContains(UIObject uIObject, string itemName)
        {
            return UIObjectToString(uIObject).Contains(itemName);
        }

        protected void OpenOverflowMenuAndInvokeItem(string itemName)
        {
            InvokeOverflowButton();

            var host = TryFindElement.ById("TopNavMenuItemsOverflowHost");
            Verify.IsNotNull(host, "Overflow menu should be opened");

            var overflowItems = GetTopNavigationItems(TopNavPosition.Overflow);
            var items = overflowItems.
                Where(item => UIObjectContains(item, itemName));

            var count = items.Count();
            if (count == 0)
            {
                Log.Comment("Items in overflow: ", String.Join("@", overflowItems.Select(item => UIObjectToString(item))));
            }
            Verify.IsTrue(count > 0, "There should be at least one item match with " + itemName);

            if (count > 1)
            {
                Log.Warning("There is more than one item match with" + itemName + " and first item is invoked");
            }

            var itemToBeClicked = items.ElementAt(0);
            Log.Comment("Invoke the item " + UIObjectToString(itemToBeClicked));
            new Button(itemToBeClicked).Invoke();
            Wait.ForIdle();
            //When a overflow item is clicked, NavView depends on another UI ticket to update the layout.
            Wait.ForSeconds(1);
            Wait.ForIdle();
        }

        protected void InvokeNavigationViewAccessKeyAndVerifyKeyTipPlacement(string expectedKeyTipTargetElementId)
        {
            string keyTipText = "H";
            Log.Comment("Send AccessKey to invoke toggle button for left nav or more button for top nav");
            KeyboardHelper.PressDownModifierKey(ModifierKey.Alt);
            KeyboardHelper.ReleaseModifierKey(ModifierKey.Alt);
            Wait.ForIdle();

            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
            {
                // Verify that KeyTip appears near the target element.
                // This scenario only works on RS4+
                var keytip = TryFindElement.ByName(keyTipText);
                Verify.IsNotNull(keytip, "keytip");
                var keyTipPopup = keytip.Parent;
                Verify.IsNotNull(keyTipPopup, "keyTipPopup");
                var keyTipBounds = keyTipPopup.BoundingRectangle;
                Log.Comment("KeyTip bounds are: " + keyTipBounds);

                var target = FindElement.ById(expectedKeyTipTargetElementId);
                var targetBounds = target.BoundingRectangle;
                Log.Comment("Target bounds are: " + targetBounds);
                targetBounds.Inflate(20, 20);

                Verify.IsTrue(keyTipBounds.IntersectsWith(targetBounds), "KeyTip bounds should be close to target bounds.");
            }


            // Invoke the AccessKey:
            TextInput.SendText(keyTipText);
            Wait.ForIdle();
        }
        protected void VerifyBackAndCloseButtonsVisibility(bool inLeftMinimalPanelDisplayMode)
        {
            using (var setup = new TestSetupHelper(new[] { "NavigationView Tests", "NavigationView Test" }))
            {
                var displayModeTextBox = new TextBlock(FindElement.ByName("DisplayModeTextBox"));
                var panelDisplayModeComboBox = new ComboBox(FindElement.ByName("PaneDisplayModeCombobox"));
                var isPaneOpenCheckBox = new CheckBox(FindElement.ById("IsPaneOpenCheckBox"));

                Verify.AreEqual(expanded, displayModeTextBox.DocumentText, "Original DisplayMode expected to be Expanded");

                if (inLeftMinimalPanelDisplayMode)
                {
                    Log.Comment("Set PaneDisplayMode to LeftMinimal");
                    panelDisplayModeComboBox.SelectItemByName("LeftMinimal");
                    Wait.ForIdle();

                    Log.Comment("Verify Pane is closed automatically when PaneDisplayMode becomes LeftMinimal");
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False when PaneDisplayMode becomes LeftMinimal");
                }
                else
                {
                    Log.Comment("Set PaneDisplayMode to Auto");
                    panelDisplayModeComboBox.SelectItemByName("Auto");
                    Wait.ForIdle();

                    Log.Comment("Verify Pane remains open when PaneDisplayMode becomes Auto");
                    Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to remain True when PaneDisplayMode becomes Auto");

                    Log.Comment("Verify back button is visible when pane is open in Expanded DisplayMode");
                    VerifyElement.Found("NavigationViewBackButton", FindBy.Id);

                    Log.Comment("Verify close button is not visible when pane is open in Expanded DisplayMode");
                    VerifyElement.NotFound("NavigationViewCloseButton", FindBy.Id);

                    Log.Comment("Decrease the width of the control from Wide to Narrow and force pane closure");
                    SetNavViewWidth(ControlWidth.Narrow);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.Off, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be False after decreasing width");
                    Verify.AreEqual(minimal, displayModeTextBox.DocumentText);
                }

                Log.Comment("Verify toggle-pane button is visible when pane is closed");
                VerifyElement.Found("TogglePaneButton", FindBy.Id);

                Log.Comment("Verify back button is visible when pane is closed");
                VerifyElement.Found("NavigationViewBackButton", FindBy.Id);

                Log.Comment("Verify close button is not visible when pane is closed");
                VerifyElement.NotFound("NavigationViewCloseButton", FindBy.Id);

                Log.Comment("Open the pane");
                isPaneOpenCheckBox.Check();
                Wait.ForIdle();

                Verify.AreEqual(ToggleState.On, isPaneOpenCheckBox.ToggleState, "IsPaneOpen expected to be True");

                Log.Comment("Verify toggle-pane button is visible when pane is open");
                VerifyElement.Found("TogglePaneButton", FindBy.Id);

                Log.Comment("Verify back button is not visible when pane is open");
                VerifyElement.NotFound("NavigationViewBackButton", FindBy.Id);

                Log.Comment("Verify close button is visible when pane is open");
                VerifyElement.Found("NavigationViewCloseButton", FindBy.Id);

                Button closeButton = new Button(FindElement.ById("NavigationViewCloseButton"));
                Verify.IsNotNull(closeButton);
                Verify.IsTrue(closeButton.IsEnabled, "Close button is expected to be enabled");

                CheckBox backButtonVisibilityCheckbox = new CheckBox(FindElement.ByName("BackButtonVisibilityCheckbox"));

                backButtonVisibilityCheckbox.Uncheck();
                Wait.ForIdle();

                Log.Comment("Verify back button is not visible when pane is open");
                VerifyElement.NotFound("NavigationViewBackButton", FindBy.Id);

                Log.Comment("Verify close button is no longer visible when pane is open");
                VerifyElement.NotFound("NavigationViewCloseButton", FindBy.Id);
            }
        }

    }

    [Flags]
    public enum RegressionTestType
    {
        LeftNav = 1,
        TopNav = 2,
        LeftNavRS4 = 4,
        HierarchyMarkup = 8,
        HierarchyDatabinding = 16
    }

    public class RegressionTestScenario
    {
        private RegressionTestScenario(string testPagename, bool isLeftnavTest, bool isUsingRS4Style)
        {
            TestPageName = testPagename;
            IsLeftNavTest = isLeftnavTest;
            IsUsingRS4Style = isUsingRS4Style;
        }
        public string TestPageName { get; private set; }
        public bool IsLeftNavTest { get; private set; }
        public bool IsUsingRS4Style { get; private set; }
        public static List<RegressionTestScenario> BuildLeftNavRegressionTestScenarios()
        {
            return BuildTestScenarios(RegressionTestType.LeftNav);
        }
        public static List<RegressionTestScenario> BuildAllRegressionTestScenarios()
        {
            return BuildTestScenarios(RegressionTestType.LeftNav | RegressionTestType.TopNav);
        }
        public static List<RegressionTestScenario> BuildTopNavRegressionTestScenarios()
        {
            return BuildTestScenarios(RegressionTestType.LeftNav | RegressionTestType.TopNav);
        }
        public static List<RegressionTestScenario> BuildHierarchicalNavRegressionTestScenarios()
        {
            return BuildTestScenarios(RegressionTestType.HierarchyMarkup | RegressionTestType.HierarchyDatabinding);
        }
        private static List<RegressionTestScenario> BuildTestScenarios(RegressionTestType types)
        {
            Dictionary<RegressionTestType, RegressionTestScenario> map =
                new Dictionary<RegressionTestType, RegressionTestScenario>
            {
                    { RegressionTestType.LeftNav, new RegressionTestScenario("NavigationView Test", isLeftnavTest: true, isUsingRS4Style: false)},
                    { RegressionTestType.TopNav, new RegressionTestScenario("NavigationView TopNav Test", isLeftnavTest: false, isUsingRS4Style: false)},
                    { RegressionTestType.HierarchyMarkup, new RegressionTestScenario("HierarchicalNavigationView Markup Test", isLeftnavTest: false, isUsingRS4Style: false)},
                    { RegressionTestType.HierarchyDatabinding, new RegressionTestScenario("HierarchicalNavigationView DataBinding Test", isLeftnavTest: false, isUsingRS4Style: false)},
            };

            List<RegressionTestScenario> scenarios = new List<RegressionTestScenario>();
            foreach (RegressionTestType type in Enum.GetValues(typeof(RegressionTestType)))
            {
                if (types.HasFlag(type))
                {
                    scenarios.Add(map[type]);
                }
            }
            return scenarios;
        }
    }
}
