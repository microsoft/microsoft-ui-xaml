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
using System.Globalization;

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
       [TestProperty("TestPass:MaxOSVer", "22621")] // This test is currently failing on 23h2, hence stop at 22h2 which is 22621.
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
        [TestProperty("TestPass:MaxOSVer", "22621")] // This test is currently failing on 23h2, hence stop at 22h2 which is 22621.
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

                Verify.AreEqual("30", testMenuFlyoutItemHeightTextBlock.GetText(), "Comparing height of MenuFlyoutItem after Flyout was opened with mouse");
                Verify.IsGreaterThan(width, 0.0, "Comparing height of MenuFlyoutItem after Flyout was opened with mouse");
                Verify.IsLessThan(width, 200.0, "Comparing height of MenuFlyoutItem after Flyout was opened with mouse");

                InputHelper.LeftClick(testMenuFlyoutItemHeightTextBlock);
                Wait.ForIdle();
                InputHelper.Tap(testMenuFlyoutButton);
                Wait.ForIdle();
                InputHelper.Tap(testMenuFlyoutItem);
                Wait.ForIdle();

                width = Convert.ToDouble(testMenuFlyoutItemWidthTextBlock.GetText());
                Verify.AreEqual("38", testMenuFlyoutItemHeightTextBlock.GetText(), "Comparing height of MenuFlyoutItem after Flyout was opened with touch");
                Verify.IsGreaterThan(width, 200.0, "Comparing width of MenuFlyoutItem after Flyout was opened with touch");
            }
        }

        [TestMethod]
        public void TopBottomAppBarTest()
        {
            using (var setup = new TestSetupHelper(new[] { "CommandBar Tests", "navigateToCommandBar" }))
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

        [TestMethod]
        public void ListViewBaseItemPageNavigationTest()
        {
            using (var setup = new TestSetupHelper("CommonStyles Tests"))
            {
                Log.Comment("Click on NavigateToNewPageButton");
                var button = new Button(FindElement.ByName("NavigateToNewPageButton"));

                InputHelper.ScrollToElement(button);
                Wait.ForIdle();

                // LeftClick is required in order for PointerCapture to happen
                InputHelper.LeftClick(button);
                Wait.ForIdle();

                Log.Comment("Verify successful page navigation");
                var buttonNavigateToGridView = FindElement.ById("navigateToGridView");
                Verify.IsNotNull(buttonNavigateToGridView, "GridViewPage is expected to be loaded.");
            }
        }


        [TestMethod]
        public void ListViewKeyboardScrollingDoesNotJump()
        {
            foreach (var configuration in new[]
            {
                (Vertical: false, CacheLength: "0"), (Vertical: false, CacheLength: "2"),
                (Vertical: true, CacheLength: "0"), (Vertical: true, CacheLength: "2")
            })
            {
                using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
                {
                    bool vertical = configuration.Vertical;
                    if (vertical)
                    {
                        new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                    }
                    new ComboBox(FindElement.ById("CacheLength")).SelectItemByName(configuration.CacheLength);
                    new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                    Wait.ForIdle();

                    int keysProcessed = 0;
                    double previousOffset = 0;
                    int itemSize = vertical ? 100 : 200;
                    int viewportSize = itemSize * 3;
                    int maximumOffset = itemSize * 36;
                    for (int index = 1; index < 39; index++)
                    {
                        KeyboardHelper.PressKey(vertical ? Key.Down : Key.Right);
                        VerifyKeyboardScrollInput(++keysProcessed);
                        double expectedOffset = Math.Min(maximumOffset, Math.Max(0, (index + 1) * itemSize - viewportSize + 20));
                        VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                        previousOffset = expectedOffset;
                    }

                    KeyboardHelper.PressKey(vertical ? Key.Down : Key.Right);
                    VerifyKeyboardScrollInput(++keysProcessed);
                    VerifyKeyboardScroll(38, maximumOffset, maximumOffset);
                    VerifyKeyboardScrollEvents(false, 0);

                    for (int index = 37; index >= 0; index--)
                    {
                        KeyboardHelper.PressKey(vertical ? Key.Up : Key.Left);
                        VerifyKeyboardScrollInput(++keysProcessed);
                        double expectedOffset = Math.Min(previousOffset, Math.Max(0, index * itemSize - 20));
                        VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                        previousOffset = expectedOffset;
                    }
                }
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingPageKeysDoNotJump()
        {
            VerifyPageKeyScrolling(partialViewport: false);
        }

        [TestMethod]
        public void ListViewKeyboardScrollingPageKeysWithPartialViewportDoNotJump()
        {
            VerifyPageKeyScrolling(partialViewport: true);
        }

        private void VerifyPageKeyScrolling(bool partialViewport)
        {
            foreach (var configuration in new[]
            {
                (Vertical: false, CacheLength: "0"), (Vertical: false, CacheLength: "2"),
                (Vertical: true, CacheLength: "0"), (Vertical: true, CacheLength: "2")
            })
            {
                using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
                {
                    Log.Comment($"Paging: vertical={configuration.Vertical}, cache={configuration.CacheLength}, partial={partialViewport}.");
                    if (configuration.Vertical)
                    {
                        new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                    }
                    if (partialViewport)
                    {
                        new CheckBox(FindElement.ById("PartialViewport")).Check();
                    }
                    new ComboBox(FindElement.ById("CacheLength")).SelectItemByName(configuration.CacheLength);
                    new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                    Wait.ForIdle();

                    int itemSize = configuration.Vertical ? 100 : 200;
                    int viewportSize = itemSize * 3 - (partialViewport ? 50 : 0);
                    int maximumOffset = 39 * itemSize - viewportSize;
                    double previousOffset = 0;
                    int index = 0;
                    while (index < 38)
                    {
                        int lastFullyVisible = (int)Math.Floor((previousOffset + viewportSize) / itemSize) - 1;
                        if (index >= lastFullyVisible)
                        {
                            double pageOffset = Math.Min(maximumOffset, previousOffset + viewportSize);
                            lastFullyVisible = (int)Math.Floor((pageOffset + viewportSize) / itemSize) - 1;
                        }
                        index = Math.Min(38, Math.Max(index + 1, lastFullyVisible));
                        Log.Comment($"PageDown to {index} from offset {previousOffset}.");
                        KeyboardHelper.PressKey(Key.PageDown);
                        double expectedOffset = Math.Min(maximumOffset,
                            Math.Max(previousOffset, (index + 1) * itemSize - viewportSize + 20));
                        VerifyKeyboardScroll(index, previousOffset, expectedOffset, paging: true);
                        Verify.AreEqual(index.ToString(CultureInfo.InvariantCulture),
                            new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText());
                        previousOffset = expectedOffset;
                    }
                    KeyboardHelper.PressKey(Key.PageDown);
                    VerifyKeyboardScroll(38, previousOffset, previousOffset, paging: true);
                    while (index > 0)
                    {
                        int firstFullyVisible = (int)Math.Ceiling(previousOffset / itemSize);
                        if (index <= firstFullyVisible)
                        {
                            double pageOffset = Math.Max(0, previousOffset - viewportSize);
                            firstFullyVisible = (int)Math.Ceiling(pageOffset / itemSize);
                        }
                        index = Math.Max(0, Math.Min(index - 1, firstFullyVisible));
                        Log.Comment($"PageUp to {index} from offset {previousOffset}.");
                        KeyboardHelper.PressKey(Key.PageUp);
                        double expectedOffset = Math.Min(previousOffset, Math.Max(0, index * itemSize - 20));
                        VerifyKeyboardScroll(index, previousOffset, expectedOffset, paging: true);
                        Verify.AreEqual(index.ToString(CultureInfo.InvariantCulture),
                            new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText());
                        previousOffset = expectedOffset;
                    }
                    KeyboardHelper.PressKey(Key.PageUp);
                    VerifyKeyboardScroll(0, 0, 0, paging: true);
                }
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingHomeEndDoNotJump()
        {
            foreach (bool vertical in new[] { false, true })
            {
                using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
                {
                    if (vertical)
                    {
                        new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                    }
                    new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                    Wait.ForIdle();
                    int maximumOffset = vertical ? 3600 : 7200;
                    KeyboardHelper.PressKey(Key.End);
                    VerifyKeyboardScroll(38, 0, maximumOffset);
                    KeyboardHelper.PressKey(Key.End);
                    VerifyKeyboardScroll(38, maximumOffset, maximumOffset);
                    VerifyKeyboardScrollEvents(false, 0);
                    KeyboardHelper.PressKey(Key.Home);
                    VerifyKeyboardScroll(0, maximumOffset, 0);
                    KeyboardHelper.PressKey(Key.Home);
                    VerifyKeyboardScroll(0, 0, 0);
                    VerifyKeyboardScrollEvents(false, 0);
                }
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingModifiedKeysKeepSelectionSemantics()
        {
            foreach (bool vertical in new[] { false, true })
            foreach (ModifierKey modifier in new[]
            {
                ModifierKey.Control, ModifierKey.Shift, ModifierKey.Control | ModifierKey.Shift
            })
            {
                using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
                {
                    if (vertical)
                    {
                        new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                    }
                    new CheckBox(FindElement.ById("ExtendedSelection")).Check();
                    new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                    Wait.ForIdle();
                    int itemSize = vertical ? 100 : 200;
                    double previousOffset = 0;
                    int largestSelectedIndex = 0;
                    foreach (var step in new[]
                    {
                        (Key: Key.PageDown, Index: 2), (Key: Key.PageDown, Index: 5),
                        (Key: vertical ? Key.Down : Key.Right, Index: 6),
                        (Key: vertical ? Key.Up : Key.Left, Index: 5),
                        (Key: Key.PageUp, Index: 2), (Key: Key.Home, Index: 0),
                        (Key: Key.End, Index: 38), (Key: Key.Home, Index: 0)
                    })
                    {
                        KeyboardHelper.PressKey(step.Key, modifier);
                        double expectedOffset = previousOffset;
                        if (step.Index * itemSize - 20 < previousOffset)
                        {
                            expectedOffset = Math.Max(0, step.Index * itemSize - 20);
                        }
                        else if ((step.Index + 1) * itemSize + 20 > previousOffset + 3 * itemSize)
                        {
                            expectedOffset = Math.Min(36 * itemSize, (step.Index - 2) * itemSize + 20);
                        }
                        bool paging = step.Key == Key.PageDown || step.Key == Key.PageUp;
                        VerifyKeyboardScroll(step.Index, previousOffset, expectedOffset, paging, selectedIndex: 0);
                        Verify.AreEqual(step.Index.ToString(CultureInfo.InvariantCulture),
                            new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText());
                        largestSelectedIndex = Math.Max(largestSelectedIndex, step.Index);
                        int expectedSelectionCount = modifier == ModifierKey.Control ? 1 :
                            modifier == ModifierKey.Shift ? step.Index + 1 : largestSelectedIndex + 1;
                        Verify.AreEqual(expectedSelectionCount.ToString(CultureInfo.InvariantCulture),
                            new TextBlock(FindElement.ById("KeyboardScrollSelectedCount")).GetText(),
                            "Ctrl moves only focus; Shift replaces the anchor range; Ctrl+Shift adds to it.");
                        previousOffset = expectedOffset;
                    }
                }
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingPageKeysDoNotDuplicateEvents()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                foreach (Key key in new[] { Key.PageDown, Key.PageDown, Key.PageDown, Key.PageUp, Key.PageUp })
                {
                    KeyboardHelper.PressKey(key);
                    VerifyKeyboardScrollEvents(true, 1, paging: true);
                }
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingPageAndEndpointKeysHonorFocusHandlers()
        {
            foreach (string setting in new[]
            {
                "CancelFocusChanges", "RedirectFocusChanges", "DisableFocusScrollingDuringFocus"
            })
            foreach (var step in new[]
            {
                // The list scrolls the newly selected item to its padded position before focus moves, so focus
                // handlers that cancel, redirect, or stop focus scrolling do not change where it lands.
                (Key: Key.PageDown, Index: 5, Offset: 620),
                (Key: Key.PageUp, Index: 1, Offset: 20),
                (Key: Key.Home, Index: 0, Offset: 0),
                (Key: Key.End, Index: 38, Offset: 7200)
            })
            {
                using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
                {
                    new CheckBox(FindElement.ById(setting)).Check();
                    new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(step.Key);
                    VerifyKeyboardScroll(step.Index, 20, step.Offset,
                        paging: step.Key == Key.PageDown || step.Key == Key.PageUp);
                    int expectedFocus = setting == "CancelFocusChanges" ? 2 :
                        setting == "RedirectFocusChanges" ? -1 : step.Index;
                    Verify.AreEqual(expectedFocus.ToString(CultureInfo.InvariantCulture),
                        new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText(),
                        $"Focus handlers must retain control of navigation for {step.Key}.");
                }
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingTabKeepsViewport()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Right);
                VerifyKeyboardScroll(2, 0, 20);
                KeyboardHelper.PressKey(Key.Tab);
                Verify.AreEqual("-1", new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText());
                Verify.AreEqual("20", new TextBlock(FindElement.ById("KeyboardScrollOffset")).GetText());
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Verify.AreEqual("2", new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText());
                Verify.AreEqual("20", new TextBlock(FindElement.ById("KeyboardScrollOffset")).GetText(),
                    "Tab re-entry must restore the focused item without nudging the viewport.");
            }
        }

        [TestMethod]
        public void ListViewFocusEntryKeepsVisibleSelectedItemInPlace()
        {
            foreach (bool vertical in new[] { false, true })
            foreach (bool fromAfterList in new[] { false, true })
            foreach (string position in new[] { "Leading", "Trailing" })
            {
                VerifyFocusEntry(vertical, fromAfterList, position);
            }
        }

        [TestMethod]
        public void ListViewFocusEntryScrollsHiddenSelectedItemOnce()
        {
            foreach (bool vertical in new[] { false, true })
            foreach (bool fromAfterList in new[] { false, true })
            foreach (string position in new[] { "Before", "After", "Partial" })
            {
                VerifyFocusEntry(vertical, fromAfterList, position);
            }
        }

        // The first focus entry into a list that never had focus must not first visit the default (first) item.
        [TestMethod]
        public void ListViewFocusEntryIntoNeverFocusedList()
        {
            foreach (bool vertical in new[] { false, true })
            foreach (bool fromAfterList in new[] { false, true })
            foreach (string position in new[] { "Leading", "Trailing", "Before", "After", "Partial" })
            {
                VerifyFocusEntry(vertical, fromAfterList, position, neverFocused: true);
            }
        }

        // Selects item index 10, positions the viewport relative to it, then moves focus into the list with
        // Tab (from before the list) or Shift+Tab (from after it). Focus is redirected to the selected item.
        private void VerifyFocusEntry(bool vertical, bool fromAfterList, string position, bool neverFocused = false)
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                Log.Comment($"Focus entry: vertical={vertical}, fromAfterList={fromAfterList}, position={position}, neverFocused={neverFocused}.");
                if (vertical)
                {
                    new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                }
                if (neverFocused)
                {
                    new CheckBox(FindElement.ById("ResetWithoutFocus")).Check();
                }
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                new ComboBox(FindElement.ById("FocusEntryPosition")).SelectItemByName(position);
                new Button(FindElement.ById(fromAfterList ? "PrepareFocusEntryAfter" : "PrepareFocusEntryBefore")).Invoke();
                Wait.ForIdle();

                int itemSize = vertical ? 100 : 200;
                int viewportSize = 3 * itemSize;
                bool itemVisible = position == "Leading" || position == "Trailing";
                double initialOffset =
                    position == "Leading" ? 10 * itemSize :
                    position == "Trailing" ? 11 * itemSize - viewportSize :
                    position == "Before" ? 0 :
                    position == "After" ? 15 * itemSize :
                    10.5 * itemSize;
                double expectedOffset =
                    itemVisible ? initialOffset :
                    position == "Before" ? 11 * itemSize - viewportSize + 20 :
                    10 * itemSize - 20;
                string initialOffsetText = initialOffset.ToString(CultureInfo.InvariantCulture);
                Wait.RetryUntilEvalFuncSuccessOrTimeout(() =>
                    new TextBlock(FindElement.ById("KeyboardScrollOffset")).GetText() == initialOffsetText &&
                    new TextBlock(FindElement.ById("KeyboardScrollChanges")).GetText() == "0");
                Verify.AreEqual(initialOffsetText, new TextBlock(FindElement.ById("KeyboardScrollOffset")).GetText());
                Verify.AreEqual("10", new TextBlock(FindElement.ById("KeyboardScrollSelectedIndex")).GetText());
                Verify.AreEqual("-1", new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText());

                if (fromAfterList)
                {
                    KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                }
                else
                {
                    KeyboardHelper.PressKey(Key.Tab);
                }
                Wait.RetryUntilEvalFuncSuccessOrTimeout(() =>
                    new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText() == "10");
                Wait.ForIdle();

                Verify.AreEqual("10", new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText(),
                    "Focus entering the list must go to the selected item.");
                double actualOffset = double.Parse(
                    new TextBlock(FindElement.ById("KeyboardScrollOffset")).GetText(), CultureInfo.InvariantCulture);
                string diagnostics =
                    $"offset changes={new TextBlock(FindElement.ById("KeyboardScrollChanges")).GetText()}, " +
                    $"offset history={new TextBlock(FindElement.ById("KeyboardScrollOffsetHistory")).GetText()}, " +
                    $"offset at focus={new TextBlock(FindElement.ById("KeyboardScrollOffsetAtFocus")).GetText()}, " +
                    $"focus sequence={new TextBlock(FindElement.ById("KeyboardScrollFocusSequence")).GetText()}, " +
                    $"prepared={new TextBlock(FindElement.ById("KeyboardScrollPreparedIndices")).GetText()}";
                Verify.IsTrue(Math.Abs(actualOffset - expectedOffset) < 0.1,
                    $"Expected offset {expectedOffset} after focus entered the list, actual {actualOffset}; {diagnostics}.");
                double offsetAtFocus = double.Parse(
                    new TextBlock(FindElement.ById("KeyboardScrollOffsetAtFocus")).GetText(), CultureInfo.InvariantCulture);
                Verify.IsTrue(Math.Abs(offsetAtFocus - expectedOffset) < 0.1,
                    $"The viewport must already be at {expectedOffset} when the item gets focus, not {offsetAtFocus}; " +
                    $"a later correction is visible as a jump; {diagnostics}.");
                if (itemVisible)
                {
                    Verify.AreEqual("0", new TextBlock(FindElement.ById("KeyboardScrollChanges")).GetText(),
                        $"Focus entering the list must not move an item that is already fully visible; {diagnostics}.");
                }
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingDoesNotDuplicateEvents()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                double previousOffset = 0;
                for (int index = 1; index <= 5; index++)
                {
                    KeyboardHelper.PressKey(Key.Right);
                    double expectedOffset = Math.Max(0, (index + 1) * 200 - 600 + 20);
                    VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                    VerifyKeyboardScrollEvents(previousOffset != expectedOffset, 1);
                    previousOffset = expectedOffset;
                }
                for (int index = 4; index >= 0; index--)
                {
                    KeyboardHelper.PressKey(Key.Left);
                    double expectedOffset = Math.Min(previousOffset, Math.Max(0, index * 200 - 20));
                    VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                    VerifyKeyboardScrollEvents(previousOffset != expectedOffset, 1);
                    previousOffset = expectedOffset;
                }
                KeyboardHelper.PressKey(Key.Left);
                VerifyKeyboardScroll(0, 0, 0);
                VerifyKeyboardScrollEvents(false, 0);
            }
        }

        private void VerifyKeyboardScrollEvents(bool scrolled, int focusChanges, bool paging = false,
            int? maximumViewportEvents = null, int? maximumPreparedContainers = null)
        {
            Wait.ForIdle();
            foreach (string id in new[]
            {
                "KeyboardScrollBringRequests", "KeyboardScrollAncestorRequests",
                "KeyboardScrollSelectionChanges", "KeyboardScrollFocusChanges"
            })
            {
                Verify.AreEqual(focusChanges.ToString(CultureInfo.InvariantCulture),
                    new TextBlock(FindElement.ById(id)).GetText(),
                    $"Each focus change must produce one notification per route, not duplicate {id} notifications. " +
                    $"Focus sequence={new TextBlock(FindElement.ById("KeyboardScrollFocusSequence")).GetText()}.");
            }
            foreach (string id in new[] { "KeyboardScrollViewChangingEvents", "KeyboardScrollViewChangedEvents" })
            {
                string count = new TextBlock(FindElement.ById(id)).GetText();
                if (paging)
                {
                    Verify.IsTrue(int.Parse(count, CultureInfo.InvariantCulture) <= (maximumViewportEvents ?? 4),
                        $"Paging must not exceed its realization/restoration/alignment work: {id}={count}.");
                }
                else
                {
                    Verify.AreEqual(scrolled ? "1" : "0", count,
                        $"Scrolling must produce one {id} notification; no-op navigation must produce none.");
                }
            }
            int preparedContainers = int.Parse(
                new TextBlock(FindElement.ById("KeyboardScrollPreparedContainers")).GetText(), CultureInfo.InvariantCulture);
            Verify.IsTrue(preparedContainers <= (maximumPreparedContainers ?? (paging ? 12 : 2)),
                $"One navigation key must not cause excessive container preparation: {preparedContainers}; " +
                $"indices={new TextBlock(FindElement.ById("KeyboardScrollPreparedIndices")).GetText()}.");
        }

        [TestMethod]
        public void ListViewKeyboardScrollingDoesNotForceLayoutWithoutScrolling()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new CheckBox(FindElement.ById("ProbeLayout")).Check();
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                foreach (var step in new[]
                {
                    (Key: Key.Right, Index: 1, Previous: 0, Offset: 0, Measures: "0"),
                    (Key: Key.Right, Index: 2, Previous: 0, Offset: 20, Measures: "1"),
                    (Key: Key.Left, Index: 1, Previous: 20, Offset: 20, Measures: "0"),
                    (Key: Key.Left, Index: 0, Previous: 20, Offset: 0, Measures: "1"),
                    (Key: Key.Left, Index: 0, Previous: 0, Offset: 0, Measures: "0")
                })
                {
                    KeyboardHelper.PressKey(step.Key);
                    VerifyKeyboardScroll(step.Index, step.Previous, step.Offset);
                    Verify.AreEqual(step.Measures, new TextBlock(FindElement.ById("KeyboardScrollLayoutAtKeyDown")).GetText(),
                        "Only a changed viewport may synchronously flush pending layout, once.");
                }
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingKeepsStickyHeadersClear()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                new CheckBox(FindElement.ById("GroupedItems")).Check();
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                for (int index = 1; index <= 5; index++)
                {
                    KeyboardHelper.PressKey(Key.Down);
                }
                KeyboardHelper.PressKey(Key.Up);
                KeyboardHelper.PressKey(Key.Up);
                Verify.AreEqual("3", new TextBlock(FindElement.ById("KeyboardScrollSelectedIndex")).GetText());
                Verify.AreEqual("3", new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText());
                double focusedTop = double.Parse(
                    new TextBlock(FindElement.ById("KeyboardScrollFocusedTop")).GetText(), CultureInfo.InvariantCulture);
                Verify.IsTrue(focusedTop >= 49.9,
                    $"The focused item must clear the 50-pixel sticky header, not start at {focusedTop}.");
            }
        }

        [TestMethod]
        public void ListViewGroupedKeyboardScrollingDoesNotJump()
        {
            VerifyGroupedArrowScrolling(partialViewport: false);
        }

        [TestMethod]
        public void ListViewGroupedKeyboardScrollingWithPartialViewportDoesNotJump()
        {
            VerifyGroupedArrowScrolling(partialViewport: true);
        }

        private void VerifyGroupedArrowScrolling(bool partialViewport)
        {
            foreach (var configuration in new[]
            {
                (Vertical: true, CacheLength: "0"), (Vertical: true, CacheLength: "2"),
                (Vertical: false, CacheLength: "0"), (Vertical: false, CacheLength: "2")
            })
            {
                using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
                {
                    Log.Comment($"Grouped arrows: vertical={configuration.Vertical}, cache={configuration.CacheLength}, partial={partialViewport}.");
                    new CheckBox(FindElement.ById("GroupedItems")).Check();
                    if (configuration.Vertical)
                    {
                        new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                    }
                    if (partialViewport)
                    {
                        new CheckBox(FindElement.ById("PartialViewport")).Check();
                    }
                    new ComboBox(FindElement.ById("CacheLength")).SelectItemByName(configuration.CacheLength);
                    new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                    Wait.ForIdle();

                    int itemSize = configuration.Vertical ? 100 : 200;
                    int viewportSize = 3 * itemSize - (partialViewport ? 50 : 0);
                    int maximumOffset = 39 * itemSize + (configuration.Vertical ? 150 : 0) - viewportSize;
                    double previousOffset = 0;
                    for (int index = 1; index < 39; index++)
                    {
                        if (index % 13 == 0)
                        {
                            previousOffset = VerifyGroupedHeaderNavigation(
                                configuration.Vertical ? Key.Down : Key.Right, index / 13, index - 1,
                                configuration.Vertical, viewportSize, previousOffset);
                        }
                        int itemStart = index * itemSize + (configuration.Vertical ? 50 * (index / 13 + 1) : 0);
                        double expectedOffset = Math.Min(maximumOffset,
                            itemStart - (configuration.Vertical ? 50 : 20) < previousOffset ?
                                Math.Max(0, itemStart - (configuration.Vertical ? 50 : 20)) :
                                Math.Max(previousOffset, itemStart + itemSize + 20 - viewportSize));
                        KeyboardHelper.PressKey(configuration.Vertical ? Key.Down : Key.Right);
                        VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                        if (index == 13 || index == 26)
                        {
                            VerifyKeyboardScrollEvents(previousOffset != expectedOffset, 1);
                        }
                        previousOffset = expectedOffset;
                    }
                    KeyboardHelper.PressKey(configuration.Vertical ? Key.Down : Key.Right);
                    VerifyKeyboardScroll(38, previousOffset, previousOffset);
                    VerifyKeyboardScrollEvents(false, 0);

                    for (int index = 37; index >= 0; index--)
                    {
                        if ((index + 1) % 13 == 0)
                        {
                            previousOffset = VerifyGroupedHeaderNavigation(
                                configuration.Vertical ? Key.Up : Key.Left, (index + 1) / 13, index + 1,
                                configuration.Vertical, viewportSize, previousOffset);
                        }
                        int itemStart = index * itemSize + (configuration.Vertical ? 50 * (index / 13 + 1) : 0);
                        double expectedOffset = Math.Min(previousOffset,
                            Math.Max(0, itemStart - (configuration.Vertical ? 50 : 20)));
                        KeyboardHelper.PressKey(configuration.Vertical ? Key.Up : Key.Left);
                        VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                        if (configuration.Vertical)
                        {
                            double focusedTop = double.Parse(
                                new TextBlock(FindElement.ById("KeyboardScrollFocusedTop")).GetText(), CultureInfo.InvariantCulture);
                            Verify.IsTrue(focusedTop >= 49.9,
                                $"Item {index} must clear its sticky header, not start at {focusedTop}.");
                        }
                        if (index == 12 || index == 25)
                        {
                            VerifyKeyboardScrollEvents(previousOffset != expectedOffset, 1);
                        }
                        previousOffset = expectedOffset;
                    }
                    KeyboardHelper.PressKey(configuration.Vertical ? Key.Up : Key.Left);
                    VerifyKeyboardScroll(0, 0, 0);
                    Verify.AreEqual("Header", new TextBlock(FindElement.ById("KeyboardScrollFocusedKind")).GetText());
                    KeyboardHelper.PressKey(configuration.Vertical ? Key.Up : Key.Left);
                    VerifyKeyboardScroll(0, 0, 0);
                    VerifyKeyboardScrollEvents(false, 0);
                }
            }
        }

        private double VerifyGroupedHeaderNavigation(Key key, int groupIndex, int selectedIndex,
            bool vertical, int viewportSize, double previousOffset)
        {
            int headerStart = groupIndex * (vertical ? 1350 : 2600);
            double expectedOffset = Math.Max(0,
                headerStart - 20 < previousOffset ? headerStart - 20 :
                    Math.Max(previousOffset, headerStart + (vertical ? 50 : 80) + 20 - viewportSize));
            int keysProcessed = int.Parse(
                new TextBlock(FindElement.ById("KeyboardScrollKeysProcessed")).GetText(), CultureInfo.InvariantCulture);
            KeyboardHelper.PressKey(key);
            VerifyKeyboardScrollInput(keysProcessed + 1);
            Wait.RetryUntilEvalFuncSuccessOrTimeout(() =>
                new TextBlock(FindElement.ById("KeyboardScrollFocusedKind")).GetText() == "Header");
            Verify.AreEqual("Header", new TextBlock(FindElement.ById("KeyboardScrollFocusedKind")).GetText(),
                $"Group {groupIndex}, key={key}, vertical={vertical}, previousOffset={previousOffset}, " +
                $"offset={new TextBlock(FindElement.ById("KeyboardScrollOffset")).GetText()}, " +
                $"selected={new TextBlock(FindElement.ById("KeyboardScrollSelectedIndex")).GetText()}, " +
                $"focus sequence={new TextBlock(FindElement.ById("KeyboardScrollFocusSequence")).GetText()}.");
            VerifyKeyboardScroll(selectedIndex, previousOffset, expectedOffset);
            return expectedOffset;
        }

        [TestMethod]
        public void ListViewGroupedKeyboardScrollingDoesNotForceLayoutWithoutScrolling()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new CheckBox(FindElement.ById("GroupedItems")).Check();
                new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                new CheckBox(FindElement.ById("ProbeLayout")).Check();
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                foreach (var step in new[]
                {
                    (Key: Key.Down, Index: 1, Previous: 0, Offset: 0, Measures: "0"),
                    (Key: Key.Down, Index: 2, Previous: 0, Offset: 70, Measures: "1"),
                    (Key: Key.Up, Index: 1, Previous: 70, Offset: 70, Measures: "0"),
                    (Key: Key.Up, Index: 0, Previous: 70, Offset: 0, Measures: "1"),
                    (Key: Key.Up, Index: 0, Previous: 0, Offset: 0, Measures: "0")
                })
                {
                    KeyboardHelper.PressKey(step.Key);
                    VerifyKeyboardScroll(step.Index, step.Previous, step.Offset);
                    Verify.AreEqual(step.Measures,
                        new TextBlock(FindElement.ById("KeyboardScrollLayoutAtKeyDown")).GetText(),
                        "Grouped navigation must not flush layout unless it requests an offset change.");
                }
            }
        }

        [TestMethod]
        public void ListViewGroupedKeyboardScrollingHonorsVaryingHeaderHeights()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new CheckBox(FindElement.ById("GroupedItems")).Check();
                new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                new CheckBox(FindElement.ById("VaryHeaderHeights")).Check();
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                // Measure preceding groups so exact offsets do not depend on estimates for unseen headers.
                double visitedOffset = 0;
                for (int index = 1; index <= 12; index++)
                {
                    double expectedOffset = Math.Max(0, 50 + (index + 1) * 100 + 20 - 300);
                    KeyboardHelper.PressKey(Key.Down);
                    VerifyKeyboardScroll(index, visitedOffset, expectedOffset);
                    visitedOffset = expectedOffset;
                }
                KeyboardHelper.PressKey(Key.Down);
                VerifyKeyboardScroll(12, 1070, 1150);
                KeyboardHelper.PressKey(Key.Down);
                VerifyKeyboardScroll(13, 1150, 1250);
                KeyboardHelper.PressKey(Key.End);
                VerifyKeyboardScroll(38, 1250, 3840, maximumOffsetChanges: 2);
                double previousOffset = 3840;
                for (int index = 37; index >= 26; index--)
                {
                    double expectedOffset = Math.Min(previousOffset, index * 100 + 130);
                    KeyboardHelper.PressKey(Key.Up);
                    VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                    previousOffset = expectedOffset;
                }
                KeyboardHelper.PressKey(Key.Up);
                Verify.AreEqual("Header", new TextBlock(FindElement.ById("KeyboardScrollFocusedKind")).GetText());
                VerifyKeyboardScroll(26, previousOffset, 2710);
                KeyboardHelper.PressKey(Key.Up);
                VerifyKeyboardScroll(25, 2710, 2550);
                double focusedTop = double.Parse(
                    new TextBlock(FindElement.ById("KeyboardScrollFocusedTop")).GetText(), CultureInfo.InvariantCulture);
                Verify.IsTrue(Math.Abs(focusedTop - 80) < 0.1,
                    "Crossing backward into a group must use that group's 80-pixel header, not the previous 110-pixel header.");
            }
        }

        [TestMethod]
        public void ListViewGroupedKeyboardScrollingWithFocusScrollingDisabled()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new CheckBox(FindElement.ById("GroupedItems")).Check();
                new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                new CheckBox(FindElement.ById("DisableFocusScrolling")).Check();
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                double previousOffset = 0;
                for (int index = 1; index <= 5; index++)
                {
                    double expectedOffset = Math.Max(0, 50 + (index + 1) * 100 - 300);
                    KeyboardHelper.PressKey(Key.Down);
                    VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                    previousOffset = expectedOffset;
                }
            }
        }

        [TestMethod]
        public void ListViewGroupedKeyboardScrollingUpFromEndDoesNotJump()
        {
            foreach (string cacheLength in new[] { "0", "2" })
            {
                using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
                {
                    new CheckBox(FindElement.ById("GroupedItems")).Check();
                    new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                    new ComboBox(FindElement.ById("CacheLength")).SelectItemByName(cacheLength);
                    new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.End);
                    VerifyKeyboardScroll(38, 0, 3750, maximumOffsetChanges: 2);
                    double previousOffset = 3750;
                    for (int index = 37; index >= 26; index--)
                    {
                        double expectedOffset = Math.Min(previousOffset, index * 100 + 100);
                        KeyboardHelper.PressKey(Key.Up);
                        VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                        previousOffset = expectedOffset;
                    }
                }
            }
        }

        [TestMethod]
        public void ListViewGroupedKeyboardScrollingPageAndEndpointKeysDoNotJump()
        {
            foreach (bool vertical in new[] { false, true })
            {
                using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
                {
                    new CheckBox(FindElement.ById("GroupedItems")).Check();
                    if (vertical)
                    {
                        new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                    }
                    new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                    Wait.ForIdle();
                    double previousOffset = 0;
                    foreach (var step in new[]
                    {
                        (Key: Key.PageDown, Index: vertical ? 1 : 2, Offset: vertical ? 0 : 20),
                        (Key: Key.PageDown, Index: vertical ? 4 : 5, Offset: vertical ? 270 : 620),
                        (Key: Key.PageUp, Index: vertical ? 3 : 4, Offset: vertical ? 270 : 620),
                        (Key: Key.Home, Index: 0, Offset: 0),
                        (Key: Key.End, Index: 38, Offset: vertical ? 3750 : 7200),
                        (Key: Key.End, Index: 38, Offset: vertical ? 3750 : 7200),
                        (Key: Key.Home, Index: 0, Offset: 0)
                    })
                    {
                        Log.Comment($"Grouped key: vertical={vertical}, key={step.Key}, target={step.Index}, from={previousOffset}.");
                        bool paging = step.Key == Key.PageDown || step.Key == Key.PageUp;
                        KeyboardHelper.PressKey(step.Key);
                        VerifyKeyboardScroll(step.Index, previousOffset, step.Offset,
                            paging: paging, maximumOffsetChanges: paging ? 5 : 2);
                        Verify.AreEqual(step.Index.ToString(CultureInfo.InvariantCulture),
                            new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText());
                        if (paging)
                        {
                            VerifyKeyboardScrollEvents(previousOffset != step.Offset, 1, paging: true,
                                maximumViewportEvents: 5);
                        }
                        previousOffset = step.Offset;
                    }
                }
            }
        }

        [TestMethod]
        public void ListViewGroupedKeyboardScrollingHonorsHeaderAndFocusSettings()
        {
            foreach (bool sticky in new[] { true, false })
            foreach (bool scaled in new[] { false, true })
            {
                using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
                {
                    new CheckBox(FindElement.ById("GroupedItems")).Check();
                    new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                    new CheckBox(FindElement.ById("PartialViewport")).Check();
                    new CheckBox(FindElement.ById("LargeFocusMargin")).Check();
                    if (!sticky)
                    {
                        new CheckBox(FindElement.ById("DisableStickyHeaders")).Check();
                    }
                    if (scaled)
                    {
                        new CheckBox(FindElement.ById("ScaleItems")).Check();
                    }
                    var baselinePreparations = new int[8];
                    if (scaled)
                    {
                        // Compare against ordinary realization with the synchronous focus path disabled.
                        new CheckBox(FindElement.ById("DisableFocusScrolling")).Check();
                        new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                        Wait.ForIdle();
                        for (int index = 1; index <= 8; index++)
                        {
                            KeyboardHelper.PressKey(Key.Down);
                            VerifyKeyboardScrollInput(index);
                            Wait.ForIdle();
                            Verify.AreEqual(index.ToString(CultureInfo.InvariantCulture),
                                new TextBlock(FindElement.ById("KeyboardScrollSelectedIndex")).GetText());
                            baselinePreparations[index - 1] = int.Parse(
                                new TextBlock(FindElement.ById("KeyboardScrollPreparedContainers")).GetText(),
                                CultureInfo.InvariantCulture);
                            Log.Comment($"Ordinary scaled realization: sticky={sticky}, index={index}, " +
                                $"prepared={baselinePreparations[index - 1]}, " +
                                $"indices={new TextBlock(FindElement.ById("KeyboardScrollPreparedIndices")).GetText()}.");
                            Verify.IsTrue(baselinePreparations[index - 1] <= 3,
                                "Ordinary uncached realization must remain bounded.");
                        }
                        new CheckBox(FindElement.ById("DisableFocusScrolling")).Uncheck();
                    }
                    new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                    Wait.ForIdle();
                    double previousOffset = 0;
                    for (int index = 1; index <= 8; index++)
                    {
                        Log.Comment($"Grouped settings: sticky={sticky}, scaled={scaled}, index={index}.");
                        // Uncached scaled targets first use their 100-pixel layout height for realization.
                        double expectedOffset = Math.Max(previousOffset, 50 + index * 100 + (scaled ? 100 : 140) - 250);
                        KeyboardHelper.PressKey(Key.Down);
                        VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                        VerifyKeyboardScrollEvents(previousOffset != expectedOffset, 1,
                            maximumPreparedContainers: scaled ? baselinePreparations[index - 1] : 2);
                        previousOffset = expectedOffset;
                    }
                    for (int index = 7; index >= 0; index--)
                    {
                        double expectedOffset = Math.Min(previousOffset, 50 + index * 100 - (sticky ? 50 : 40));
                        KeyboardHelper.PressKey(Key.Up);
                        VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                        previousOffset = expectedOffset;
                    }
                }
            }
        }

        [TestMethod]
        public void ListViewGroupedKeyboardScrollingHonorsFocusHandlers()
        {
            foreach (string setting in new[]
            {
                "CancelFocusChanges", "RedirectFocusChanges", "DisableFocusScrollingDuringFocus"
            })
            foreach (var step in new[]
            {
                // The list scrolls the newly selected item to its padded position before focus moves, so focus
                // handlers that cancel, redirect, or stop focus scrolling do not change where it lands.
                (Key: Key.Down, Index: 3, Offset: 170),
                (Key: Key.PageDown, Index: 4, Offset: 270),
                (Key: Key.PageUp, Index: 1, Offset: 70),
                (Key: Key.Home, Index: 0, Offset: 0),
                (Key: Key.End, Index: 38, Offset: 3750)
            })
            {
                using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
                {
                    new CheckBox(FindElement.ById("GroupedItems")).Check();
                    new CheckBox(FindElement.ById("VerticalScrolling")).Check();
                    new CheckBox(FindElement.ById(setting)).Check();
                    new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(step.Key);
                    bool paging = step.Key == Key.PageDown || step.Key == Key.PageUp;
                    VerifyKeyboardScroll(step.Index, 70, step.Offset,
                        paging: paging, maximumOffsetChanges: paging ? 5 : 2);
                    int expectedFocus = setting == "CancelFocusChanges" ? 2 :
                        setting == "RedirectFocusChanges" ? -1 : step.Index;
                    Verify.AreEqual(expectedFocus.ToString(CultureInfo.InvariantCulture),
                        new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText());
                }
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingToUnrealizedItem()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                Verify.AreEqual("False", new TextBlock(FindElement.ById("KeyboardScrollLastContainerRealized")).GetText(),
                    "The last item must not be realized before navigating to it.");
                KeyboardHelper.PressKey(Key.End);
                Wait.ForIdle();
                Verify.AreEqual("38", new TextBlock(FindElement.ById("KeyboardScrollSelectedIndex")).GetText());
                Verify.AreEqual("38", new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText());
                Verify.AreEqual("7200", new TextBlock(FindElement.ById("KeyboardScrollOffset")).GetText());
                KeyboardHelper.PressKey(Key.Home);
                Wait.ForIdle();
                Verify.AreEqual("0", new TextBlock(FindElement.ById("KeyboardScrollSelectedIndex")).GetText());
                Verify.AreEqual("0", new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText());
                Verify.AreEqual("0", new TextBlock(FindElement.ById("KeyboardScrollOffset")).GetText());
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingHonorsLargeFocusMargin()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new CheckBox(FindElement.ById("LargeFocusMargin")).Check();
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                double previousOffset = 0;
                for (int index = 1; index <= 5; index++)
                {
                    KeyboardHelper.PressKey(Key.Right);
                    double expectedOffset = Math.Max(0, (index + 1) * 200 - 600 + 40);
                    VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                    previousOffset = expectedOffset;
                }
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingWhenFocusIsRedirected()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new CheckBox(FindElement.ById("RedirectFocusChanges")).Check();
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.Right);
                // The newly selected item is scrolled to its padded position before the app redirects focus.
                VerifyKeyboardScroll(3, 20, 220);
                Verify.AreEqual("-1", new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText(),
                    "App-directed focus must remain outside the list.");
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingHonorsFocusHandlerDisablingScrolling()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new CheckBox(FindElement.ById("DisableFocusScrollingDuringFocus")).Check();
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.Right);
                // The newly selected item is scrolled to its padded position before GettingFocus runs.
                VerifyKeyboardScroll(3, 20, 220);
            }
        }

        private void VerifyKeyboardScroll(int index, double previousOffset, double expectedOffset,
            bool paging = false, int? selectedIndex = null, int? maximumOffsetChanges = null)
        {
            // Input delivery can follow the idle handshake. Scrolling must still finish inside the key handler.
            Wait.RetryUntilEvalFuncSuccessOrTimeout(() =>
                new TextBlock(FindElement.ById("KeyboardScrollSelectedIndex")).GetText() ==
                    (selectedIndex ?? index).ToString(CultureInfo.InvariantCulture));
            Verify.AreEqual((selectedIndex ?? index).ToString(CultureInfo.InvariantCulture),
                new TextBlock(FindElement.ById("KeyboardScrollSelectedIndex")).GetText(),
                $"Navigation to {index} from offset {previousOffset}; focused kind " +
                $"{new TextBlock(FindElement.ById("KeyboardScrollFocusedKind")).GetText()}, focused index " +
                $"{new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText()}, offset at key return " +
                $"{new TextBlock(FindElement.ById("KeyboardScrollOffsetAtKeyDown")).GetText()}.");
            double actualOffset = double.Parse(
                new TextBlock(FindElement.ById("KeyboardScrollOffset")).GetText(), CultureInfo.InvariantCulture);
            Verify.IsTrue(Math.Abs(actualOffset - expectedOffset) < 0.1,
                $"Expected offset {expectedOffset}, actual offset {actualOffset}, selected index {index}, " +
                $"focused index {new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText()}, " +
                $"offset at key return {new TextBlock(FindElement.ById("KeyboardScrollOffsetAtKeyDown")).GetText()}.");
            string changes = new TextBlock(FindElement.ById("KeyboardScrollChanges")).GetText();
            if (paging || maximumOffsetChanges.HasValue)
            {
                int changeCount = int.Parse(changes, CultureInfo.InvariantCulture);
                Verify.IsTrue(changeCount <= (maximumOffsetChanges ?? 4),
                    $"Navigation must stay within its bounded realization/restoration/alignment work: {changeCount} changes.");
            }
            else
            {
                Verify.AreEqual(previousOffset == expectedOffset ? "0" : "1", changes,
                    "Navigation must scroll directly to the padded focus bounds, without a second edge correction.");
            }
            Verify.AreEqual(changes,
                new TextBlock(FindElement.ById("KeyboardScrollChangesAtKeyDown")).GetText(),
                "No offset change may be deferred until after keyboard navigation returns.");
            double offsetAtKeyDown = double.Parse(
                new TextBlock(FindElement.ById("KeyboardScrollOffsetAtKeyDown")).GetText(), CultureInfo.InvariantCulture);
            Verify.IsTrue(Math.Abs(offsetAtKeyDown - expectedOffset) < 0.1,
                $"The viewport must already be at {expectedOffset} when keyboard navigation returns, not {offsetAtKeyDown}.");
        }

        private void VerifyKeyboardScrollInput(int expectedCount)
        {
            string expected = expectedCount.ToString(CultureInfo.InvariantCulture);
            Wait.RetryUntilEvalFuncSuccessOrTimeout(() =>
                new TextBlock(FindElement.ById("KeyboardScrollKeysProcessed")).GetText() == expected);
            Verify.AreEqual(expected,
                new TextBlock(FindElement.ById("KeyboardScrollKeysProcessed")).GetText(),
                "The test app must receive each injected navigation key before its behavior is checked.");
        }

        [TestMethod]
        public void ListViewKeyboardScrollingWithFocusScrollingDisabled()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new CheckBox(FindElement.ById("DisableFocusScrolling")).Check();
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();

                double previousOffset = 0;
                for (int index = 1; index < 10; index++)
                {
                    KeyboardHelper.PressKey(Key.Right);
                    double expectedOffset = Math.Max(0, (index + 1) * 200 - 600);
                    VerifyKeyboardScroll(index, previousOffset, expectedOffset);
                    previousOffset = expectedOffset;
                }
            }
        }

        [TestMethod]
        public void ListViewKeyboardScrollingWhenFocusIsCanceled()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new CheckBox(FindElement.ById("CancelFocusChanges")).Check();
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();

                KeyboardHelper.PressKey(Key.Right);
                // The newly selected item is scrolled to its padded position before the app cancels focus.
                VerifyKeyboardScroll(3, 20, 220);
                Verify.AreEqual("2", new TextBlock(FindElement.ById("KeyboardScrollFocusedIndex")).GetText(),
                    "Canceled navigation must leave focus on the previous item.");
            }
        }

        [TestMethod]
        public void ListViewPublicScrollIntoViewKeepsDefaultAlignment()
        {
            using (var setup = new TestSetupHelper("ListView Keyboard Scrolling Tests"))
            {
                new Button(FindElement.ById("ResetKeyboardScrolling")).Invoke();
                Wait.ForIdle();
                new Button(FindElement.ById("PublicScrollIntoView")).Invoke();
                Wait.ForIdle();

                Verify.AreEqual("1600", new TextBlock(FindElement.ById("KeyboardScrollOffset")).GetText(),
                    "Public ScrollIntoView must keep its unpadded default alignment.");
                Verify.AreEqual("0", new TextBlock(FindElement.ById("KeyboardScrollSelectedIndex")).GetText(),
                    "Public ScrollIntoView must not change selection.");
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
