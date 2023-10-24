// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using System;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class CommonStylesTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
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
        public void SliderDensityTest()
        {
            RunDensityTests("SliderDensityTest");
        }

        [TestMethod]
        public void ToggleSwitchDensityTest()
        {
            RunDensityTests("ToggleSwitchDensityTest");
        }

        [TestMethod]
        public void DatePickerDensityTest()
        {
            RunDensityTests("DatePickerDensityTest");
        }

        [TestMethod]
        public void TimePickerDensityTest()
        {
            RunDensityTests("TimePickerDensityTest");
        }

        [TestMethod]
        public void AutoSuggestBoxDensityTest()
        {
            RunDensityTests("AutoSuggestBoxDensityTest");
        }

        [TestMethod]
        public void ListViewItemDensityTest()
        {
            RunDensityTests("ListViewItemDensityTest");
        }

        [TestMethod]
        public void TextBoxDensityTest()
        {
            RunDensityTests("TextBoxDensityTest");
        }

        [TestMethod]
        public void PasswordBoxDensityTest()
        {
            RunDensityTests("PasswordBoxDensityTest");
        }

        [TestMethod]
        public void ComboBoxDensityTest()
        {
            RunDensityTests("ComboBoxDensityTest");
        }

        [TestMethod]
        public void RichEditBoxDensityTest()
        {
            RunDensityTests("RichEditBoxDensityTest");
        }

        [TestMethod]
        public void AppBarToggleButtonDensityTest()
        {
            RunDensityTests("AppBarToggleButtonDensityTest");
        }

        [TestMethod]
        public void AppBarButtonDensityTest()
        {
            RunDensityTests("AppBarButtonDensityTest");
        }

        private void RunDensityTests(string buttonName)
        {
            using (var setup = new TestSetupHelper("CommonStyles Tests"))
            {
                Log.Comment("Click on " + buttonName);
                var button = new Button(FindElement.ByName(buttonName));
                button.Invoke();
                Wait.ForIdle();

                var densityTestResult = new TextBlock(FindElement.ByName("DensityTestResult")).GetText();
                Verify.AreEqual(densityTestResult, "Pass", "We expect density test result is Pass");
            }
        }

        [TestMethod]
        public void RunCompactTests()
        {
            using (var setup = new TestSetupHelper("Compact Tests"))
            {
                Log.Comment("Click on RunTest");
                var button = new Button(FindElement.ByName("RunTest"));
                button.Invoke();
                Wait.ForIdle();

                var testResult = new TextBlock(FindElement.ById("CompactTestResult")).GetText();
                Verify.AreEqual(testResult, "Pass", "We expect compact test result is Pass"); // "Pass" string matches value used by MUXControlsTestApp.SimpleVerify
            }
        }

        //[TestMethod]
        // Disabled due to: Bug 24013494: DCPP: CommonStylesTests.CornerRadiusTest is failing due to hitting an Assert in TickBar::ArrangeOverride
        public void CornerRadiusTest()
        {
            using (var setup = new TestSetupHelper("CornerRadius Tests"))
            {
                var textBlock = FindElement.ByName("CornerRadius");
                Verify.IsNotNull(textBlock, "Verify corner radius page doesn't crash");
            }
        }

        //[TestMethod]
        // InkToolbar is not presently in WinUI 3.
        public void InkToolbarTest()
        {
            using (var setup = new TestSetupHelper("InkToolbar Tests"))
            {
                var textBlock = FindElement.ByName("InkToolbar");
                Verify.IsNotNull(textBlock, "Verify InkToolbar page doesn't crash");

                var verticalInkToolbar = FindElement.ById("VerticalInkToolbar");
                Verify.IsNotNull(textBlock, "Verify verticalInkToolbar doesn't crash");

                Log.Comment("Click on " + "InkToolbarBallpointPenButton");
                var radioButton = new RadioButton(FindElement.ById("InkToolbarBallpointPenButton"));
                radioButton.Select();
                Wait.ForIdle();
            }
        }

        [TestMethod]
        public void MenuFlyoutItemSizeTest()
        {
            using (var setup = new TestSetupHelper("MenuFlyout Tests"))
            {
                Log.Comment("Mouse click on Button to verify MenuFlyoutItem size.");

                var testMenuFlyoutButton = FindElement.ByName("TestMenuFlyoutButton");
                Verify.IsNotNull(testMenuFlyoutButton, "Verifying that we found a UIElement called TestMenuFlyoutButton");

                InputHelper.LeftClick(testMenuFlyoutButton);
                Wait.ForIdle();

                var testMenuFlyoutItem = FindElement.ByName("TestMenuFlyoutItem");
                Verify.IsNotNull(testMenuFlyoutItem, "Verifying that we found a UIElement called TestMenuFlyoutItem");

                InputHelper.LeftClick(testMenuFlyoutItem);
                Wait.ForIdle();

                var testMenuFlyoutItemHeightTextBlock = FindElement.ByName<TextBlock>("TestMenuFlyoutItemHeightTextBlock");
                var testMenuFlyoutItemWidthTextBlock = FindElement.ByName<TextBlock>("TestMenuFlyoutItemWidthTextBlock");

                Verify.IsNotNull(testMenuFlyoutItemHeightTextBlock, "Verifying that we found a UIElement called TestMenuFlyoutItemHeightTextBlock");
                Verify.IsNotNull(testMenuFlyoutItemWidthTextBlock, "Verifying that we found a UIElement called TestMenuFlyoutItemWidthTextBlock");

                var width = Convert.ToDouble(testMenuFlyoutItemWidthTextBlock.GetText());

                Verify.AreEqual("32", testMenuFlyoutItemHeightTextBlock.GetText(), "Comparing height of MenuFlyoutItem after Flyout was opened with mouse");
                Verify.IsGreaterThan(width, 0.0, "Comparing height of MenuFlyoutItem after Flyout was opened with mouse");
                Verify.IsLessThan(width, 200.0, "Comparing height of MenuFlyoutItem after Flyout was opened with mouse");

                InputHelper.LeftClick(testMenuFlyoutItemHeightTextBlock);
                Wait.ForIdle();
                InputHelper.Tap(testMenuFlyoutButton);
                Wait.ForIdle();
                InputHelper.Tap(testMenuFlyoutItem);
                Wait.ForIdle();

                width = Convert.ToDouble(testMenuFlyoutItemWidthTextBlock.GetText());
                Verify.AreEqual("40", testMenuFlyoutItemHeightTextBlock.GetText(), "Comparing height of MenuFlyoutItem after Flyout was opened with touch");
                Verify.IsGreaterThan(width, 200.0, "Comparing width of MenuFlyoutItem after Flyout was opened with touch");
            }
        }

        [TestMethod]
        public void TopBottomAppBarTest()
        {
            using (var setup = new TestSetupHelper("CommandBar Tests"))
            {
                var topAppBar = FindElement.ByName("TopCmdBar");
                Verify.IsNotNull(topAppBar);
                var topBounds = topAppBar.BoundingRectangle;
                Verify.IsTrue(topBounds.Width > 0 && topBounds.Height > 0,
                        string.Format("Top CommandBar bounds are ({0}, {1})", topBounds.Width, topBounds.Height));

                var bottomAppBar = FindElement.ByName("BottomCmdBar");
                Verify.IsNotNull(bottomAppBar);
                var bottomBounds = bottomAppBar.BoundingRectangle;
                Verify.IsTrue(bottomBounds.Width > 0 && bottomBounds.Height > 0,
                        string.Format("Bottom CommandBar bounds are ({0}, {1})", bottomBounds.Width, bottomBounds.Height));

                Log.Comment("Find Restore/Maximize button");
                ElementCache.Clear();
                var maxRestoreButton = new Button(FindElement.ByNameOrId("Restore"));

                maxRestoreButton.Invoke();
                Wait.ForMilliseconds(1000);

                var newTopBounds = topAppBar.BoundingRectangle;
                Verify.IsTrue(newTopBounds.Width != topBounds.Width,
                        string.Format("New top CommandBar bounds are ({0}, {1})", newTopBounds.Width, newTopBounds.Height));

                var newBottomBounds = topAppBar.BoundingRectangle;
                Verify.IsTrue(newBottomBounds.Width != bottomBounds.Width,
                        string.Format("New bottom CommandBar bounds are ({0}, {1})", newBottomBounds.Width, newBottomBounds.Height));

                maxRestoreButton.Invoke();
                Wait.ForMilliseconds(1000);
            }
        }

        [TestMethod]
        public void HyperlinkIsOffscreenTest()
        {
            using (var setup = new TestSetupHelper("TextControls Tests"))
            {
                var hyperlink = FindElement.ByName("hyperlinkText");
                Verify.IsNotNull(hyperlink);
                var hyperlinkStartingBounds = hyperlink.BoundingRectangle;
                Verify.IsTrue(hyperlinkStartingBounds.X > 0 && hyperlinkStartingBounds.Y > 0,
                        string.Format("Hyperlink corner is at ({0}, {1})", hyperlinkStartingBounds.X, hyperlinkStartingBounds.Y));

                Verify.IsTrue(hyperlink.IsOffscreen == false, "IsOffscreen == false");

                var resizeButton = new Button(FindElement.ByName("resizeWindowButton"));
                resizeButton.Invoke();
                Wait.ForIdle();

                var hyperlinkEndingBounds = hyperlink.BoundingRectangle;
                Verify.IsTrue(hyperlinkEndingBounds.X <= 0 && hyperlinkEndingBounds.Y <= 0,
                        string.Format("Hyperlink corner is at ({0}, {1})", hyperlinkEndingBounds.X, hyperlinkEndingBounds.Y));

                Verify.IsTrue(hyperlink.IsOffscreen == true, "IsOffscreen == true");

                resizeButton.Invoke();
                Wait.ForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Scrolls a vertical grouped GridView with Mandatory snap points and with the mouse-wheel.")]
        public void ScrollGroupedGridViewWithMandatorySnapPointsTest()
        {
            using (var setup = new TestSetupHelper(new[] { "GridView Tests", "navigateToGroupedGridView" }))
            {
                Log.Comment("Retrieving cmbScrollViewerVerticalSnapPointsType");
                UIObject scrollViewerVerticalSnapPointsType = FindElement.ById("cmbScrollViewerVerticalSnapPointsType");
                Verify.IsNotNull(scrollViewerVerticalSnapPointsType, "Verifying that cmbScrollViewerVerticalSnapPointsType was found");
                ComboBox cmbScrollViewerVerticalSnapPointsType = new ComboBox(scrollViewerVerticalSnapPointsType);

                Log.Comment("Changing cmbScrollViewerVerticalSnapPointsType selection to 'Mandatory'");
                cmbScrollViewerVerticalSnapPointsType.SelectItemByName("Mandatory");
                Log.Comment("Selection is now {0}", cmbScrollViewerVerticalSnapPointsType.Selection[0].Name);

                Log.Comment("Retrieving cmbItemsPanelGroupHeaderPlacement");
                UIObject itemsPanelGroupHeaderPlacement = FindElement.ById("cmbItemsPanelGroupHeaderPlacement");
                Verify.IsNotNull(itemsPanelGroupHeaderPlacement, "Verifying that cmbItemsPanelGroupHeaderPlacement was found");
                ComboBox cmbItemsPanelGroupHeaderPlacement = new ComboBox(itemsPanelGroupHeaderPlacement);

                Log.Comment("Changing cmbItemsPanelGroupHeaderPlacement selection to 'Left'");
                cmbItemsPanelGroupHeaderPlacement.SelectItemByName("Left");
                Log.Comment("Selection is now {0}", cmbItemsPanelGroupHeaderPlacement.Selection[0].Name);

                Log.Comment("Retrieving btnSetItemsPanelGroupHeaderPlacement");
                UIObject setItemsPanelGroupHeaderPlacement = FindElement.ById("btnSetItemsPanelGroupHeaderPlacement");
                Verify.IsNotNull(setItemsPanelGroupHeaderPlacement, "Verifying that btnSetItemsPanelGroupHeaderPlacement was found");
                Button btnSetItemsPanelGroupHeaderPlacement = new Button(setItemsPanelGroupHeaderPlacement);

                Log.Comment("Invoking btnSetItemsPanelGroupHeaderPlacement");
                btnSetItemsPanelGroupHeaderPlacement.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving btnGetItemsWrapGridItemHeight");
                UIObject getItemsWrapGridItemHeight = FindElement.ById("btnGetItemsWrapGridItemHeight");
                Verify.IsNotNull(getItemsWrapGridItemHeight, "Verifying that btnGetItemsWrapGridItemHeight was found");
                Button btnGetItemsWrapGridItemHeight = new Button(getItemsWrapGridItemHeight);

                Log.Comment("Invoking btnGetItemsWrapGridItemHeight");
                btnGetItemsWrapGridItemHeight.Invoke();
                Wait.ForIdle();
                
                Log.Comment("Retrieving txtItemsWrapGridItemHeight");
                UIObject itemsWrapGridItemHeight = FindElement.ById("txtItemsWrapGridItemHeight");
                Verify.IsNotNull(itemsWrapGridItemHeight);
                Edit txtItemsWrapGridItemHeight = new Edit(itemsWrapGridItemHeight);

                Log.Comment("txtItemsWrapGridItemHeight.Value: " + txtItemsWrapGridItemHeight.Value);
                Verify.AreEqual("150", txtItemsWrapGridItemHeight.Value);

                Log.Comment("Retrieving gridView");
                UIObject gridViewUIObject = FindElement.ById("gridView");
                Verify.IsNotNull(gridViewUIObject, "Verifying that gridView was found");                
                Wait.ForIdle();

                Log.Comment("Scrolling gridView with mouse wheel");
                InputHelper.RotateWheel(gridViewUIObject, -120);
                Wait.ForIdle();

                Log.Comment("Verifying gridView scrolled to mandatory snap point, i.e. a small multiple of 150");
                bool scrolledToMandatorySnapPoint =
                    TryWaitForEditValues(editName: "txtScrollViewerVerticalOffset", editValue: "150") ||
                    TryWaitForEditValues(editName: "txtScrollViewerVerticalOffset", editValue: "300");
                Log.Comment("scrolledToMandatorySnapPoint:" + scrolledToMandatorySnapPoint);
                Verify.IsTrue(scrolledToMandatorySnapPoint);
            }
        }

        private bool TryWaitForEditValues(string editName, string editValue)
        {
            UIObject editUIObject = FindElement.ById(editName);
            Verify.IsNotNull(editUIObject);
            Edit edit = new Edit(editUIObject);

            Log.Comment("Current value for " + editName + ": " + edit.Value);
            if (edit.Value != editValue)
            {
                using (var waiter = new ValueChangedEventWaiter(edit, editValue))
                {
                    Log.Comment("Waiting for " + editName + " to be set to " + editValue);

                    bool success = waiter.TryWait(TimeSpan.FromSeconds(2.0));
                    Log.Comment("Current value for " + editName + ": " + edit.Value + ", (success:" + success + ")");
                    return success;
                }
            }
            return true;
        }
    }
}
