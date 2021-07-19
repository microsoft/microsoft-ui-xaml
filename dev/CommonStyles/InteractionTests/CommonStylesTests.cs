﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
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
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using System;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
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

        [TestMethod]
        public void CornerRadiusTest()
        {
            using (var setup = new TestSetupHelper("CornerRadius Tests"))
            {
                var textBlock = FindElement.ByName("CornerRadius");
                Verify.IsNotNull(textBlock, "Verify corner radius page doesn't crash");
            }
        }

        [TestMethod]
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
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

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
    }
}
