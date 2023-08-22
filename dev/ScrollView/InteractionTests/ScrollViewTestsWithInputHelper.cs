// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Threading;
using Windows.UI.ViewManagement;
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
using Point = System.Drawing.Point;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class ScrollViewTestsWithInputHelper
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
        [TestProperty("Description", "Pans an Image in a ScrollView.")]
        public void PanScrollView()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            const double minHorizontalScrollPercent = 35.0;
            const double minVerticalScrollPercent = 35.0;

            Log.Comment("Selecting ScrollView tests");

            using (var setup = new TestSetupHelper(new[] { "ScrollView Tests", "navigateToSimpleContents" }))
            {
                Log.Comment("Retrieving cmbShowScrollView");
                ComboBox cmbShowScrollView = new ComboBox(FindElement.ByName("cmbShowScrollView"));
                Verify.IsNotNull(cmbShowScrollView, "Verifying that cmbShowScrollView was found");

                Log.Comment("Changing ScrollView selection to scrollView51");
                cmbShowScrollView.SelectItemByName("scrollView_51");
                Log.Comment("Selection is now {0}", cmbShowScrollView.Selection[0].Name);

                if (PlatformConfiguration.IsOsVersion(OSVersion.Redstone1))
                {
                    Log.Comment("On RS1 the ScrollPresenter's content is centered in an animated way when it's smaller than the viewport. Waiting for those animations to complete.");
                    WaitForScrollViewManipulationEnd("scrollView21");
                }

                Log.Comment("Retrieving img51");
                UIObject img51UIObject = FindElement.ByName("img51");
                Verify.IsNotNull(img51UIObject, "Verifying that img51 was found");

                Log.Comment("Retrieving scrollPresenter51");
                ScrollPresenter scrollPresenter51 = new ScrollPresenter(img51UIObject.Parent);
                Verify.IsNotNull(scrollPresenter51, "Verifying that scrollPresenter51 was found");

                WaitForScrollViewFinalSize(scrollPresenter51, 300.0 /*expectedWidth*/, 400.0 /*expectedHeight*/);

                // Tapping button before attempting pan operation to guarantee effective touch input
                TapResetViewsButton();

                Log.Comment("Panning ScrollView in diagonal");
                PrepareForScrollViewManipulationStart();

                InputHelper.Pan(
                    scrollPresenter51,
                    new Point(scrollPresenter51.BoundingRectangle.Left + 25, scrollPresenter51.BoundingRectangle.Top + 25),
                    new Point(scrollPresenter51.BoundingRectangle.Left - 25, scrollPresenter51.BoundingRectangle.Top - 25));

                Log.Comment("Waiting for scrollView51 pan completion");
                WaitForScrollViewManipulationEnd("scrollView51");

                Log.Comment("scrollPresenter51.HorizontalScrollPercent={0}", scrollPresenter51.HorizontalScrollPercent);
                Log.Comment("scrollPresenter51.VerticalScrollPercent={0}", scrollPresenter51.VerticalScrollPercent);

                if (scrollPresenter51.HorizontalScrollPercent <= minHorizontalScrollPercent || scrollPresenter51.VerticalScrollPercent <= minVerticalScrollPercent)
                {
                    LogAndClearTraces();
                }

                Verify.IsTrue(scrollPresenter51.HorizontalScrollPercent > minHorizontalScrollPercent, "Verifying scrollPresenter51 HorizontalScrollPercent is greater than " + minHorizontalScrollPercent + "%");
                Verify.IsTrue(scrollPresenter51.VerticalScrollPercent > minVerticalScrollPercent, "Verifying scrollPresenter51 VerticalScrollPercent is greater than " + minVerticalScrollPercent + "%");

                // scrollPresenter51's Content size is 800x800px.
                double horizontalOffset;
                double verticalOffset;
                double minHorizontalOffset = 800.0 * (1.0 - scrollPresenter51.HorizontalViewSize / 100.0) * minHorizontalScrollPercent / 100.0;
                double minVerticalOffset = 800.0 * (1.0 - scrollPresenter51.VerticalViewSize / 100.0) * minVerticalScrollPercent / 100.0;
                float zoomFactor;

                GetScrollPresenterView(out horizontalOffset, out verticalOffset, out zoomFactor);
                Log.Comment("horizontalOffset={0}", horizontalOffset);
                Log.Comment("verticalOffset={0}", verticalOffset);
                Log.Comment("zoomFactor={0}", zoomFactor);
                Verify.IsTrue(horizontalOffset > minHorizontalOffset, "Verifying horizontalOffset is greater than " + minHorizontalOffset);
                Verify.IsTrue(verticalOffset > minVerticalOffset, "Verifying verticalOffset is greater than " + minVerticalOffset);
                Verify.AreEqual(zoomFactor, 1.0f, "Verifying zoomFactor is 1.0f");

                // Output-debug-string-level "None" is automatically restored when landing back on the ScrollView test page.
            }
        }

        [TestMethod]
        [TestProperty("Description", "Scrolls an Image in a ScrollView using the mouse on the ScrollBar thumb, then pans it with touch.")]
        public void ScrollThenPanScrollView()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            Log.Comment("Selecting ScrollView tests");

            using (var setup = new TestSetupHelper(new[] { "ScrollView Tests", "navigateToSimpleContents" }))
            {
                const double minVerticalScrollPercentAfterScroll = 15.0;
                const double minHorizontalScrollPercentAfterPan = 35.0;
                const double minVerticalScrollPercentAfterPan = 50.0;

                double verticalScrollPercentAfterScroll = 0.0;

                Log.Comment("Retrieving cmbShowScrollView");
                ComboBox cmbShowScrollView = new ComboBox(FindElement.ByName("cmbShowScrollView"));
                Verify.IsNotNull(cmbShowScrollView, "Verifying that cmbShowScrollView was found");

                Log.Comment("Changing ScrollView selection to scrollView51");
                cmbShowScrollView.SelectItemByName("scrollView_51");
                Log.Comment("Selection is now {0}", cmbShowScrollView.Selection[0].Name);

                if (PlatformConfiguration.IsOsVersion(OSVersion.Redstone1))
                {
                    Log.Comment("On RS1 the ScrollPresenter's content is centered in an animated way when it's smaller than the viewport. Waiting for those animations to complete.");
                    WaitForScrollViewManipulationEnd("scrollView21");
                }

                Log.Comment("Retrieving img51");
                UIObject img51UIObject = FindElement.ByName("img51");
                Verify.IsNotNull(img51UIObject, "Verifying that img51 was found");

                Log.Comment("Retrieving scrollPresenter51");
                ScrollPresenter scrollPresenter51 = new ScrollPresenter(img51UIObject.Parent);
                Verify.IsNotNull(scrollPresenter51, "Verifying that scrollPresenter51 was found");

                WaitForScrollViewFinalSize(scrollPresenter51, 300.0 /*expectedWidth*/, 400.0 /*expectedHeight*/);

                // Tapping button before attempting pan operation to guarantee effective touch input
                TapResetViewsButton();

                Log.Comment("Left mouse buttom down over ScrollBar thumb");
                InputHelper.LeftMouseButtonDown(scrollPresenter51, 140 /*offsetX*/, -100 /*offsetY*/);

                Log.Comment("Mouse drag and left mouse buttom up over ScrollBar thumb");
                InputHelper.LeftMouseButtonUp(scrollPresenter51, 140 /*offsetX*/, -50 /*offsetY*/);

                Log.Comment("scrollPresenter51.HorizontalScrollPercent={0}", scrollPresenter51.HorizontalScrollPercent);
                Log.Comment("scrollPresenter51.VerticalScrollPercent={0}", scrollPresenter51.VerticalScrollPercent);

                verticalScrollPercentAfterScroll = scrollPresenter51.VerticalScrollPercent;

                if (scrollPresenter51.HorizontalScrollPercent != 0.0 || scrollPresenter51.VerticalScrollPercent <= minVerticalScrollPercentAfterScroll)
                {
                    LogAndClearTraces();
                }

                Verify.AreEqual(scrollPresenter51.HorizontalScrollPercent, 0.0, "Verifying scrollPresenter51 HorizontalScrollPercent is still 0%");
                Verify.IsTrue(verticalScrollPercentAfterScroll > minVerticalScrollPercentAfterScroll, "Verifying scrollPresenter51 VerticalScrollPercent is greater than " + minVerticalScrollPercentAfterScroll + "%");

                Log.Comment("Panning ScrollView in diagonal");
                PrepareForScrollViewManipulationStart();

                // Using a large enough span and duration for this diagonal pan so that it is not erroneously recognized as a horizontal pan.
                InputHelper.Pan(
                    scrollPresenter51,
                    new Point(scrollPresenter51.BoundingRectangle.Left + 30, scrollPresenter51.BoundingRectangle.Top + 30),
                    new Point(scrollPresenter51.BoundingRectangle.Left - 30, scrollPresenter51.BoundingRectangle.Top - 30),
                    InputHelper.DefaultPanHoldDuration,
                    InputHelper.DefaultPanAcceleration / 2.4f);

                Log.Comment("Waiting for scrollView51 pan completion");
                WaitForScrollViewManipulationEnd("scrollView51");

                Log.Comment("scrollPresenter51.HorizontalScrollPercent={0}", scrollPresenter51.HorizontalScrollPercent);
                Log.Comment("scrollPresenter51.VerticalScrollPercent={0}", scrollPresenter51.VerticalScrollPercent);

                if (scrollPresenter51.HorizontalScrollPercent <= minHorizontalScrollPercentAfterPan ||
                    scrollPresenter51.VerticalScrollPercent <= minVerticalScrollPercentAfterPan ||
                    scrollPresenter51.VerticalScrollPercent <= verticalScrollPercentAfterScroll)
                {
                    LogAndClearTraces();
                }

                Verify.IsTrue(scrollPresenter51.HorizontalScrollPercent > minHorizontalScrollPercentAfterPan, "Verifying scrollPresenter51 HorizontalScrollPercent is greater than " + minHorizontalScrollPercentAfterPan + "%");
                Verify.IsTrue(scrollPresenter51.VerticalScrollPercent > minVerticalScrollPercentAfterPan, "Verifying scrollPresenter51 VerticalScrollPercent is greater than " + minVerticalScrollPercentAfterPan + "%");
                Verify.IsTrue(scrollPresenter51.VerticalScrollPercent > verticalScrollPercentAfterScroll, "Verifying scrollPresenter51 VerticalScrollPercent is greater than " + verticalScrollPercentAfterScroll + "%");

                // scrollPresenter51's Content size is 800x800px.
                double horizontalOffset;
                double verticalOffset;
                double minHorizontalOffset = 800.0 * (1.0 - scrollPresenter51.HorizontalViewSize / 100.0) * minHorizontalScrollPercentAfterPan / 100.0;
                double minVerticalOffset = 800.0 * (1.0 - scrollPresenter51.VerticalViewSize / 100.0) * minVerticalScrollPercentAfterPan / 100.0;
                float zoomFactor;

                GetScrollPresenterView(out horizontalOffset, out verticalOffset, out zoomFactor);
                Log.Comment("horizontalOffset={0}", horizontalOffset);
                Log.Comment("verticalOffset={0}", verticalOffset);
                Log.Comment("zoomFactor={0}", zoomFactor);
                Verify.IsTrue(horizontalOffset > minHorizontalOffset, "Verifying horizontalOffset is greater than " + minHorizontalOffset);
                Verify.IsTrue(verticalOffset > minVerticalOffset, "Verifying verticalOffset is greater than " + minVerticalOffset);
                Verify.AreEqual(zoomFactor, 1.0f, "Verifying zoomFactor is 1.0f");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests Keyboard interaction (Up, Down, Left, Right, PageUp, PageDown, Home, End)")]
        public void VerifyScrollViewKeyboardInteraction()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollView not supported pre-RS2");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollView Tests", "navigateToSimpleContents" }))
            {
                UIObject buttonInScrollView11;
                ScrollPresenter scrollPresenter11;
                SetupSimpleSingleScrollViewTest(out buttonInScrollView11, out scrollPresenter11);

                var scrollAmountForDownOrUpKey = scrollPresenter11.BoundingRectangle.Height * 0.15;
                var scrollAmountForPageUpOrPageDownKey = scrollPresenter11.BoundingRectangle.Height;
                var scrollAmountForRightOrLeftKey = scrollPresenter11.BoundingRectangle.Width * 0.15;
                var maxScrollOffset = 2000 - scrollPresenter11.BoundingRectangle.Height;

                double expectedVerticalOffset = 0;
                double expectedHorizontalOffset = 0;

                Log.Comment("Pressing Down key");
                KeyboardHelper.PressKey(buttonInScrollView11, Key.Down, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedVerticalOffset += scrollAmountForDownOrUpKey;
                WaitForScrollViewOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing PageDown key");
                KeyboardHelper.PressKey(buttonInScrollView11, Key.PageDown, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedVerticalOffset += scrollAmountForPageUpOrPageDownKey;
                WaitForScrollViewOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing Home key");
                KeyboardHelper.PressKey(buttonInScrollView11, Key.Home, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedVerticalOffset = 0;
                WaitForScrollViewOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing End key");
                KeyboardHelper.PressKey(buttonInScrollView11, Key.End, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedVerticalOffset = maxScrollOffset;
                WaitForScrollViewOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing Up key");
                KeyboardHelper.PressKey(buttonInScrollView11, Key.Up, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedVerticalOffset -= scrollAmountForDownOrUpKey;
                WaitForScrollViewOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing PageUp key");
                KeyboardHelper.PressKey(buttonInScrollView11, Key.PageUp, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedVerticalOffset -= scrollAmountForPageUpOrPageDownKey;
                WaitForScrollViewOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing Right key");
                KeyboardHelper.PressKey(buttonInScrollView11, Key.Right, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedHorizontalOffset += scrollAmountForRightOrLeftKey;
                WaitForScrollViewOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing Left key");
                KeyboardHelper.PressKey(buttonInScrollView11, Key.Left, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedHorizontalOffset -= scrollAmountForRightOrLeftKey;
                WaitForScrollViewOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing Down key three times");
                KeyboardHelper.PressKey(buttonInScrollView11, Key.Down, modifierKey: ModifierKey.None, numPresses: 3, useDebugMode: true);
                expectedVerticalOffset += 3 * scrollAmountForDownOrUpKey;
                WaitForScrollViewOffsets(expectedHorizontalOffset, expectedVerticalOffset);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies keyboard input is ignored when ScrollView.IgnoredInputKinds is Keyboard.")]
        public void VerifyScrollViewIgnoresKeyboardInput()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollView not supported pre-RS2");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollView Tests", "navigateToSimpleContents" }))
            {
                UIObject img51;
                ScrollPresenter scrollPresenter51;

                SetupScrollViewTestWithImage("51", out img51, out scrollPresenter51);

                Log.Comment("Retrieving cmbIgnoredInputKinds");
                ComboBox cmbIgnoredInputKinds = new ComboBox(FindElement.ByName("cmbIgnoredInputKinds"));
                Verify.IsNotNull(cmbIgnoredInputKinds, "Verifying that cmbIgnoredInputKinds was found");

                Log.Comment("Changing ScrollView.IgnoredInputKinds to Keyboard");
                cmbIgnoredInputKinds.SelectItemByName("Keyboard");
                Log.Comment("Selection is now {0}", cmbIgnoredInputKinds.Selection[0].Name);

                Log.Comment("Pressing Down key");
                KeyboardHelper.PressKey(scrollPresenter51, Key.Down, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                VerifyScrollViewRemainsAtView(0.0, 0.0, 1.0f);

                Log.Comment("Pressing Right key");
                KeyboardHelper.PressKey(scrollPresenter51, Key.Right, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                VerifyScrollViewRemainsAtView(0.0, 0.0, 1.0f);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests keyboard interaction (Down, Up, PageDown, PageUp, End, Home, Right, Left) when ScrollView.XYFocusKeyboardNavigation is Enabled.")]
        public void VerifyScrollViewKeyboardInteractionWithXYFocusEnabled()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollView not supported pre-RS2");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollView Tests", "navigateToSimpleContents" }))
            {
                UIObject img51;
                ScrollPresenter scrollPresenter51;

                SetupScrollViewTestWithImage("51", out img51, out scrollPresenter51);

                var scrollAmountForDownOrUpKey = scrollPresenter51.BoundingRectangle.Height * 0.5;
                var scrollAmountForPageUpOrPageDownKey = scrollPresenter51.BoundingRectangle.Height;
                var scrollAmountForRightOrLeftKey = scrollPresenter51.BoundingRectangle.Width * 0.5;
                var maxScrollOffset = 800 - scrollPresenter51.BoundingRectangle.Height;

                Log.Comment("Pressing Down key");
                KeyboardHelper.PressKey(scrollPresenter51, Key.Down, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewOffsets(0, scrollAmountForDownOrUpKey);

                Log.Comment("Pressing Up key");
                KeyboardHelper.PressKey(scrollPresenter51, Key.Up, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewOffsets(0, 0);

                Log.Comment("Pressing PageDown key");
                KeyboardHelper.PressKey(scrollPresenter51, Key.PageDown, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewOffsets(0, scrollAmountForPageUpOrPageDownKey);

                Log.Comment("Pressing PageUp key");
                KeyboardHelper.PressKey(scrollPresenter51, Key.PageUp, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewOffsets(0, 0);

                Log.Comment("Pressing End key");
                KeyboardHelper.PressKey(scrollPresenter51, Key.End, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewOffsets(0, maxScrollOffset);

                Log.Comment("Pressing Home key");
                KeyboardHelper.PressKey(scrollPresenter51, Key.Home, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewOffsets(0, 0);

                Log.Comment("Pressing Right key");
                KeyboardHelper.PressKey(scrollPresenter51, Key.Right, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewOffsets(scrollAmountForRightOrLeftKey, 0);

                Log.Comment("Pressing Left key");
                KeyboardHelper.PressKey(scrollPresenter51, Key.Left, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewOffsets(0, 0);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests End and Home keys when ScrollView.VerticalScrollMode is Disabled.")]
        public void ScrollHorizontallyWithEndHomeKeys()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollView not supported pre-RS2");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollView Tests", "navigateToSimpleContents" }))
            {
                UIObject img31;
                ScrollPresenter scrollPresenter31;

                SetupScrollViewTestWithImage("31", out img31, out scrollPresenter31);

                Log.Comment("Pressing End key");
                KeyboardHelper.PressKey(scrollPresenter31, Key.End, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewOffsets(900 - scrollPresenter31.BoundingRectangle.Width, 0);

                Log.Comment("Pressing Home key");
                KeyboardHelper.PressKey(scrollPresenter31, Key.Home, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewOffsets(0, 0);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests End and Home keys when ScrollView.VerticalScrollMode is Disabled in RightToLeft flow direction.")]
        public void ScrollHorizontallyWithEndHomeKeysInRTL()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollView not supported pre-RS2");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollView Tests", "navigateToSimpleContents" }))
            {
                UIObject img32;
                ScrollPresenter scrollPresenter32;

                SetupScrollViewTestWithImage("32", out img32, out scrollPresenter32);

                Log.Comment("Pressing Home key");
                KeyboardHelper.PressKey(scrollPresenter32, Key.Home, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewOffsets(900 - scrollPresenter32.BoundingRectangle.Width, 0);

                Log.Comment("Pressing End key");
                KeyboardHelper.PressKey(scrollPresenter32, Key.End, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewOffsets(0, 0);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests GamePad interaction")]
        public void VerifyScrollViewGamePadInteraction()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
            {
                Log.Warning("Test is disabled on pre-RS4 because ScrollView Gamepad interaction is not supported pre-RS4");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollView Tests", "navigateToSimpleContents" }))
            {
                using (var loggingHelper = new LoggingHelper(this))
                {
                    UISettings settings = new UISettings();
                    bool areAnimationsEnabled = settings.AnimationsEnabled;

                    UIObject buttonInScrollView11;
                    ScrollPresenter scrollPresenter11;
                    SetupSimpleSingleScrollViewTest(out buttonInScrollView11, out scrollPresenter11);

                    Log.Comment("Tapping Button 1");
                    InputHelper.Tap(buttonInScrollView11);

                    Log.Comment($"Focused element. Expected=Button 1, Actual={UIObject.Focused.Name}.");
                    Verify.AreEqual("Button 1", UIObject.Focused.Name, "Verify focused element");

                    var scrollAmountForGamepadUpDown = scrollPresenter11.BoundingRectangle.Height * 0.5;
                    Log.Comment($"scrollAmountForGamepadUpDown={scrollAmountForGamepadUpDown}.");

                    double expectedVerticalOffset = 0;
                    double expectedHorizontalOffset = 0;
                    int? expectedViewChangeCount = areAnimationsEnabled ? null : (int?)1;

                    //Down. Change focus. Don't scroll.
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickDown, "Button 2", expectedHorizontalOffset, expectedVerticalOffset, 0);

                    //Down. Change focus. Scroll.
                    expectedVerticalOffset = 220;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickDown, "Button 3", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Down. Don't change focus. Scroll.
                    expectedVerticalOffset += scrollAmountForGamepadUpDown;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickDown, "Button 3", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Down. Change focus. Scroll.
                    expectedVerticalOffset = 920;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickDown, "Button 4", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Down. Change focus. Scroll.
                    expectedVerticalOffset = 1020;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickDown, "Button 5", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Up. Change focus. Don't scroll.
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickUp, "Button 4", expectedHorizontalOffset, expectedVerticalOffset, 0);

                    //Up. Don't change focus. Scroll.
                    expectedVerticalOffset -= scrollAmountForGamepadUpDown;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickUp, "Button 4", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Up. Change focus. Scroll.
                    expectedVerticalOffset = 480;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickUp, "Button 3", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies gamepad input is ignored when ScrollView.IgnoredInputKinds is Gamepad.")]
        public void VerifyScrollViewIgnoresGamepadInput()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollView not supported pre-RS2");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollView Tests", "navigateToSimpleContents" }))
            {
                UIObject img51;
                ScrollPresenter scrollPresenter51;

                SetupScrollViewTestWithImage("51", out img51, out scrollPresenter51);

                Log.Comment("Retrieving cmbIgnoredInputKinds");
                ComboBox cmbIgnoredInputKinds = new ComboBox(FindElement.ByName("cmbIgnoredInputKinds"));
                Verify.IsNotNull(cmbIgnoredInputKinds, "Verifying that cmbIgnoredInputKinds was found");

                Log.Comment("Changing ScrollView.IgnoredInputKinds to Gamepad");
                cmbIgnoredInputKinds.SelectItemByName("Gamepad");
                Log.Comment("Selection is now {0}", cmbIgnoredInputKinds.Selection[0].Name);

                Log.Comment("Pressing LeftThumbstick Down");
                GamepadHelper.PressButton(null, GamepadButton.LeftThumbstickDown);

                Log.Comment("Pressing LeftThumbstick Down");
                GamepadHelper.PressButton(null, GamepadButton.LeftThumbstickDown);
                VerifyScrollViewRemainsAtView(0.0, 0.0, 1.0f);

                Log.Comment("Pressing LeftThumbstick Right");
                GamepadHelper.PressButton(null, GamepadButton.LeftThumbstickRight);
                VerifyScrollViewRemainsAtView(0.0, 0.0, 1.0f);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests GamePad interaction")]
        public void VerifyScrollViewGamePadHorizontalInteraction()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
            {
                Log.Warning("Test is disabled on pre-RS4 because ScrollView Gamepad interaction is not supported pre-RS4");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollView Tests", "navigateToSimpleContents" }))
            {
                using (var loggingHelper = new LoggingHelper(this))
                {
                    UISettings settings = new UISettings();
                    bool areAnimationsEnabled = settings.AnimationsEnabled;

                    UIObject buttonInScrollView11;
                    ScrollPresenter scrollPresenter11;
                    SetupSimpleSingleScrollViewTest(out buttonInScrollView11, out scrollPresenter11);

                    var scrollAmountForGamepadLeftRight = scrollPresenter11.BoundingRectangle.Width * 0.5;

                    double expectedVerticalOffset = 0;
                    double expectedHorizontalOffset = 0;
                    int? expectedViewChangeCount = areAnimationsEnabled ? null : (int?)1;

                    //Right. Change focus. Don't scroll. 
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickRight, "Button 1B", expectedHorizontalOffset, expectedVerticalOffset, 0);

                    //Left. Change focus. Don't scroll. 
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickLeft, "Button 1", expectedHorizontalOffset, expectedVerticalOffset, 0);

                    //Right. Change focus. Don't scroll. 
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickRight, "Button 1B", expectedHorizontalOffset, expectedVerticalOffset, 0);

                    //Right. Change focus. Scroll. 
                    expectedHorizontalOffset = 320;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickRight, "Button 1C", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Right. Don't Change focus. Scroll. 
                    expectedHorizontalOffset += scrollAmountForGamepadLeftRight;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickRight, "Button 1C", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Right. Don't Change focus. Scroll. 
                    expectedHorizontalOffset += scrollAmountForGamepadLeftRight;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickRight, "Button 1C", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Left. Don't Change focus. Scroll. 
                    expectedHorizontalOffset -= scrollAmountForGamepadLeftRight;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickLeft, "Button 1C", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Left. Change focus. Scroll. 
                    expectedHorizontalOffset = 80;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftThumbstickLeft, "Button 1B", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests GamePad interaction")]
        public void VerifyScrollViewGamePadTriggerInteraction()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
            {
                Log.Warning("Test is disabled on pre-RS4 because ScrollView Gamepad interaction is not supported pre-RS4");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollView Tests", "navigateToSimpleContents" }))
            {
                using (var loggingHelper = new LoggingHelper(this))
                {
                    UISettings settings = new UISettings();
                    bool areAnimationsEnabled = settings.AnimationsEnabled;

                    UIObject buttonInScrollView11;
                    ScrollPresenter scrollPresenter11;
                    SetupSimpleSingleScrollViewTest(out buttonInScrollView11, out scrollPresenter11);

                    var scrollAmountForGamepadTrigger = scrollPresenter11.BoundingRectangle.Height;

                    double expectedVerticalOffset = 0;
                    double expectedHorizontalOffset = 0;
                    int? expectedViewChangeCount = areAnimationsEnabled ? null : (int?)1;

                    //Down. Change focus. Scroll.
                    expectedVerticalOffset = areAnimationsEnabled ? scrollAmountForGamepadTrigger : 220.0;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.RightTrigger, "Button 3", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Down. Don't change focus. Scroll.
                    expectedVerticalOffset += scrollAmountForGamepadTrigger;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.RightTrigger, "Button 3", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Up. Don't change focus. Scroll.
                    expectedVerticalOffset -= scrollAmountForGamepadTrigger;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftTrigger, "Button 3", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    double actualHorizontalOffset;
                    double actualVerticalOffset;
                    float actualZoomFactor;
                    GetScrollPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);

                    //Up. Change focus. Scroll.
                    expectedVerticalOffset = 0;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftTrigger, actualVerticalOffset < scrollAmountForGamepadTrigger ? "Button 1" : "Button 2", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests GamePad interaction")]
        public void VerifyScrollViewGamePadBumperInteraction()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
            {
                Log.Warning("Test is disabled on pre-RS4 because ScrollView Gamepad interaction is not supported pre-RS4");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollView Tests", "navigateToSimpleContents" }))
            {
                using (var loggingHelper = new LoggingHelper(this))
                {
                    UISettings settings = new UISettings();
                    bool areAnimationsEnabled = settings.AnimationsEnabled;

                    UIObject buttonInScrollView11;
                    ScrollPresenter scrollPresenter11;
                    SetupSimpleSingleScrollViewTest(out buttonInScrollView11, out scrollPresenter11);

                    var scrollAmountForBumper = scrollPresenter11.BoundingRectangle.Width;

                    double expectedVerticalOffset = 0;
                    double expectedHorizontalOffset = 0;
                    int? expectedViewChangeCount = areAnimationsEnabled ? null : (int?)1;

                    //Right. Change focus. Scroll.
                    expectedHorizontalOffset = areAnimationsEnabled ? scrollAmountForBumper : 320.0;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.RightShoulder, "Button 1C", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Right. Don't change focus. Scroll.
                    expectedHorizontalOffset += scrollAmountForBumper;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.RightShoulder, "Button 1C", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    //Left. Don't change focus. Scroll.
                    expectedHorizontalOffset -= scrollAmountForBumper;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftShoulder, "Button 1C", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);

                    double actualHorizontalOffset;
                    double actualVerticalOffset;
                    float actualZoomFactor;
                    GetScrollPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);

                    //Left. Change focus. Scroll.
                    expectedHorizontalOffset = areAnimationsEnabled ? 0 : 80.0;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftShoulder, actualHorizontalOffset > scrollAmountForBumper ? "Button 1B" : "Button 1", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);
                }
            }
        }

        private void SetupSimpleSingleScrollViewTest(out UIObject buttonInScrollView11, out ScrollPresenter scrollPresenter11)
        {
            Log.Comment("Retrieving cmbShowScrollView");
            ComboBox cmbShowScrollView = new ComboBox(FindElement.ByName("cmbShowScrollView"));
            Verify.IsNotNull(cmbShowScrollView, "Verifying that cmbShowScrollView was found");

            Log.Comment("Changing ScrollView selection to scrollView11");
            cmbShowScrollView.SelectItemByName("scrollView_11");
            Log.Comment("Selection is now {0}", cmbShowScrollView.Selection[0].Name);

            Log.Comment("Retrieving buttonInScrollView11");
            buttonInScrollView11 = FindElement.ById("buttonInScrollView11");
            Verify.IsNotNull(buttonInScrollView11, "Verifying that buttonInScrollView11 was found");

            Log.Comment("Retrieving scrollPresenter11");
            scrollPresenter11 = new ScrollPresenter(buttonInScrollView11.Parent);
            Verify.IsNotNull(scrollPresenter11, "Verifying that scrollPresenter11 was found");

            WaitForScrollViewFinalSize(scrollPresenter11, 300.0 /*expectedWidth*/, 400.0 /*expectedHeight*/);

            buttonInScrollView11.Click();
            Wait.ForIdle();
        }

        private void SetupScrollViewTestWithImage(string suffix, out UIObject imageInScrollView, out ScrollPresenter scrollPresenter)
        {
            Log.Comment("Retrieving cmbShowScrollView");
            ComboBox cmbShowScrollView = new ComboBox(FindElement.ByName("cmbShowScrollView"));
            Verify.IsNotNull(cmbShowScrollView, "Verifying that cmbShowScrollView was found");

            Log.Comment("Changing ScrollView selection to scrollView" + suffix);
            cmbShowScrollView.SelectItemByName("scrollView_" + suffix);
            Log.Comment("Selection is now {0}", cmbShowScrollView.Selection[0].Name);

            Log.Comment("Retrieving img" + suffix);
            imageInScrollView = FindElement.ById("img" + suffix);
            Verify.IsNotNull(imageInScrollView, "Verifying that img" + suffix + " was found");

            Log.Comment("Retrieving ScrollPresenter");
            scrollPresenter = new ScrollPresenter(imageInScrollView.Parent);
            Verify.IsNotNull(scrollPresenter, "Verifying that ScrollPresenter was found");

            WaitForScrollViewFinalSize(scrollPresenter, 300.0 /*expectedWidth*/, 400.0 /*expectedHeight*/);

            imageInScrollView.Click();
            Wait.ForIdle();
        }

        private void PressGamepadButtonAndVerifyOffsetAndFocus(
            GamepadButton gamepadButton,
            string expectedFocusedItemName,
            double expectedHorizontalOffset,
            double expectedVerticalOffset,
            int? expectedViewChangeCount = null)
        {
            Log.Comment($"PressGamepadButtonAndVerifyOffsetAndFocus. gamepadButton={gamepadButton}, expectedFocusedItemName={expectedFocusedItemName}, expectedViewChangeCount={expectedViewChangeCount}.");

            bool waitForFocusChange = UIObject.Focused.Name != expectedFocusedItemName;

            GamepadHelper.PressButton(null, gamepadButton);

            if (waitForFocusChange)
            {
                var focusChangedWaiter = new FocusAcquiredWaiter();

                focusChangedWaiter.Wait(TimeSpan.FromSeconds(2));
            }

            WaitForScrollViewOffsets(expectedHorizontalOffset, expectedVerticalOffset);

            Log.Comment($"Focused element. Expected={expectedFocusedItemName}, Actual={UIObject.Focused.Name}.");
            Verify.AreEqual(expectedFocusedItemName, UIObject.Focused.Name, "Verify focused element");

            int viewChangeCount = ViewChangeCount();
            if (expectedViewChangeCount == null)
            {
                Verify.IsGreaterThan(viewChangeCount, 1);
            }
            else
            {
                Verify.AreEqual(viewChangeCount, expectedViewChangeCount);
            }
        }

        private void WaitForScrollViewOffsets(double expectedHorizontalOffset, double expectedVerticalOffset)
        {
            Log.Comment("Waiting for ScrollView offsets: {0}, {1}", expectedHorizontalOffset, expectedVerticalOffset);

            double actualHorizontalOffset;
            double actualVerticalOffset;
            float actualZoomFactor;
            GetScrollPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);

            Func<bool> areOffsetsCorrect = () => AreClose(expectedHorizontalOffset, actualHorizontalOffset) && AreClose(expectedVerticalOffset, actualVerticalOffset);

            int triesRemaining = 10;
            while (!areOffsetsCorrect() && triesRemaining-- > 0)
            {
                Thread.Sleep(500);
                GetScrollPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);
            }

            if (triesRemaining >= 0)
            {
                // Allow the view to settle and the STateChanged, ScrollCompleted or ZoomCompleted events to be raised.
                Thread.Sleep(250);
                GetScrollPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);
            }

            Log.Comment($"Final ScrollView offsets. Expected={expectedHorizontalOffset},{expectedVerticalOffset}, Actual={actualHorizontalOffset},{actualVerticalOffset}.");
            if (!areOffsetsCorrect())
            {
                LogAndClearTraces();
            }
            Verify.IsTrue(areOffsetsCorrect(), String.Format("Verify ScrollView offsets. Expected={0},{1}, Actual={2},{3}.",
                expectedHorizontalOffset, expectedVerticalOffset, actualHorizontalOffset, actualVerticalOffset));
        }

        private void VerifyScrollViewRemainsAtView(double expectedHorizontalOffset, double expectedVerticalOffset, float expectedZoomFactor)
        {
            Log.Comment("Verifying ScrollView view remains at: {0}, {1}, {2}",
                expectedHorizontalOffset, expectedVerticalOffset, expectedZoomFactor);

            double actualHorizontalOffset;
            double actualVerticalOffset;
            float actualZoomFactor;

            GetScrollPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);

            Func<bool> isViewCorrect = () =>
                AreClose(expectedHorizontalOffset, actualHorizontalOffset) &&
                AreClose(expectedVerticalOffset, actualVerticalOffset) &&
                AreClose(expectedZoomFactor, actualZoomFactor);

            Verify.IsTrue(isViewCorrect(), String.Format("Verify ScrollView initial view. Expected={0},{1},{2}, Actual={3},{4},{5}.",
                    expectedHorizontalOffset, expectedVerticalOffset, expectedZoomFactor,
                    actualHorizontalOffset, actualVerticalOffset, actualZoomFactor));

            Thread.Sleep(750);

            GetScrollPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);

            Verify.IsTrue(isViewCorrect(), String.Format("Verify ScrollView final view. Expected={0},{1},{2}, Actual={3},{4},{5}.",
                    expectedHorizontalOffset, expectedVerticalOffset, expectedZoomFactor,
                    actualHorizontalOffset, actualVerticalOffset, actualZoomFactor));
        }

        private bool AreClose(double expected, double actual, double delta = 0.1)
        {
            return Math.Abs(expected - actual) <= delta;
        }

        private void VerifyAreClose(double expected, double actual, double delta, string message)
        {
            Verify.IsTrue(AreClose(expected, actual, delta), String.Format("{0}, expected={1}, actual={2}, delta={3}", message, expected, actual, delta));
        }

        private void TapResetViewsButton()
        {
            Log.Comment("Retrieving btnResetViews");
            UIObject resetViewsUIObject = FindElement.ByName("btnResetViews");
            Verify.IsNotNull(resetViewsUIObject, "Verifying that btnResetViews Button was found");

            Button resetViewsButton = new Button(resetViewsUIObject);
            InputHelper.Tap(resetViewsButton);
            if (!WaitForEditValue("txtResetStatus" /*editName*/, "Views reset" /*editValue*/, 4.0 /*secondsTimeout*/, false /*throwOnError*/))
            {
                InputHelper.Tap(resetViewsButton);
                WaitForEditValue("txtResetStatus" /*editName*/, "Views reset" /*editValue*/, 4.0 /*secondsTimeout*/, false /*throwOnError*/);
            }
        }

        private void GetScrollPresenterView(out double horizontalOffset, out double verticalOffset, out float zoomFactor)
        {
            horizontalOffset = 0.0;
            verticalOffset = 0.0;
            zoomFactor = 1.0f;

            UIObject viewUIObject = FindElement.ById("txtScrollPresenterHorizontalOffset");
            Edit viewTextBox = new Edit(viewUIObject);
            Log.Comment("Current HorizontalOffset: " + viewTextBox.Value);
            horizontalOffset = String.IsNullOrWhiteSpace(viewTextBox.Value) ? double.NaN : Convert.ToDouble(viewTextBox.Value);

            viewUIObject = FindElement.ById("txtScrollPresenterVerticalOffset");
            viewTextBox = new Edit(viewUIObject);
            Log.Comment("Current VerticalOffset: " + viewTextBox.Value);
            verticalOffset = String.IsNullOrWhiteSpace(viewTextBox.Value) ? double.NaN : Convert.ToDouble(viewTextBox.Value);

            viewUIObject = FindElement.ById("txtScrollPresenterZoomFactor");
            viewTextBox = new Edit(viewUIObject);
            Log.Comment("Current ZoomFactor: " + viewTextBox.Value);
            zoomFactor = String.IsNullOrWhiteSpace(viewTextBox.Value) ? float.NaN : Convert.ToSingle(viewTextBox.Value);
        }

        private void PrepareForScrollViewManipulationStart(string stateTextBoxName = "txtScrollPresenterState")
        {
            UIObject scrollPresenterStateUIObject = FindElement.ById(stateTextBoxName);
            Edit scrollPresenterStateTextBox = new Edit(scrollPresenterStateUIObject);
            Log.Comment("Pre-manipulation ScrollPresenterState: " + scrollPresenterStateTextBox.Value);
            Wait.ForIdle();
        }

        private void LogEditValue(string editName)
        {
            Edit edit = new Edit(FindElement.ById(editName));
            Verify.IsNotNull(edit);
            LogEditValue(editName, edit);
        }

        private void LogEditValue(string editName, Edit edit)
        {
            Log.Comment("Current value for " + editName + ": " + edit.Value);
        }

        private bool WaitForEditValue(string editName, string editValue, double secondsTimeout = 2.0, bool throwOnError = true)
        {
            Edit edit = new Edit(FindElement.ById(editName));
            Verify.IsNotNull(edit);
            LogEditValue(editName, edit);
            if (edit.Value != editValue)
            {
                using (var waiter = new ValueChangedEventWaiter(edit, editValue))
                {
                    Log.Comment("Waiting for " + editName + " to be set to " + editValue);

                    bool success = waiter.TryWait(TimeSpan.FromSeconds(secondsTimeout));
                    Log.Comment("Current value for " + editName + ": " + edit.Value);

                    if (success)
                    {
                        Log.Comment("Wait succeeded");
                    }
                    else
                    {
                        if (edit.Value == editValue)
                        {
                            Log.Warning("Wait failed but TextBox contains expected Text");
                            LogAndClearTraces();
                        }
                        else
                        {
                            if (throwOnError)
                            {
                                Log.Error("Wait for edit value failed");
                                LogAndClearTraces();
                                throw new WaiterException();
                            }
                            else
                            {
                                Log.Warning("Wait for edit value failed");
                                LogAndClearTraces();
                                return false;
                            }
                        }
                    }
                }
            }
            return true;
        }

        private void WaitForScrollViewManipulationEnd(string scrollViewName, string stateTextBoxName = "txtScrollPresenterState")
        {
            WaitForManipulationEnd(scrollViewName, stateTextBoxName);
        }

        private bool TryWaitForScrollViewManipulationEnd(string scrollViewName, string stateTextBoxName = "txtScrollPresenterState")
        {
            return WaitForManipulationEnd(scrollViewName, stateTextBoxName, false /*throwOnError*/);
        }

        private bool WaitForManipulationEnd(string elementName, string stateTextBoxName, bool throwOnError = true)
        {
            UIObject elementStateUIObject = FindElement.ById(stateTextBoxName);
            Edit elementStateTextBox = new Edit(elementStateUIObject);
            Log.Comment("Current State: " + elementStateTextBox.Value);
            if (elementStateTextBox.Value != elementName + ".PART_Root.PART_ScrollPresenter Idle")
            {
                using (var waiter = new ValueChangedEventWaiter(elementStateTextBox, elementName + ".PART_Root.PART_ScrollPresenter Idle"))
                {
                    int loops = 0;

                    Log.Comment("Waiting for " + elementName + "'s manipulation end.");
                    while (true)
                    {
                        bool success = waiter.TryWait(TimeSpan.FromMilliseconds(250));

                        Log.Comment("Current State: " + elementStateTextBox.Value);

                        if (success)
                        {
                            Log.Comment("Wait succeeded");
                            break;
                        }
                        else
                        {
                            if (elementStateTextBox.Value == elementName + ".PART_Root.PART_ScrollPresenter Idle")
                            {
                                Log.Warning("Wait failed but TextBox contains expected text");
                                LogAndClearTraces();
                                break;
                            }
                            else if (loops < 20)
                            {
                                loops++;
                                waiter.Reset();
                            }
                            else
                            {
                                if (throwOnError)
                                {
                                    Log.Error("Wait for manipulation end failed");
                                    LogAndClearTraces();
                                    throw new WaiterException();
                                }
                                else
                                {
                                    Log.Warning("Wait for manipulation end failed");
                                    LogAndClearTraces();
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
            return true;
        }

        private void WaitForScrollViewFinalSize(UIObject scrollViewUIObject, double expectedWidth, double expectedHeight)
        {
            int pauses = 0;
            int widthDelta = Math.Abs(scrollViewUIObject.BoundingRectangle.Width - (int)expectedWidth);
            int heightDelta = Math.Abs(scrollViewUIObject.BoundingRectangle.Height - (int)expectedHeight);

            Log.Comment("scrollViewUIObject.BoundingRectangle={0}", scrollViewUIObject.BoundingRectangle);

            while (widthDelta > 1 || heightDelta > 1 && pauses < 5)
            {
                Wait.ForMilliseconds(60);
                pauses++;
                Log.Comment("scrollViewUIObject.BoundingRectangle={0}", scrollViewUIObject.BoundingRectangle);
                widthDelta = Math.Abs(scrollViewUIObject.BoundingRectangle.Width - (int)expectedWidth);
                heightDelta = Math.Abs(scrollViewUIObject.BoundingRectangle.Height - (int)expectedHeight);
            };

            Verify.IsLessThanOrEqual(widthDelta, 1);
            Verify.IsLessThanOrEqual(heightDelta, 1);
        }

        private void WaitForBoxChecked(string checkBoxName)
        {
            Log.Comment($"Waiting for checkbox {checkBoxName} checked...");
            LogEditValue("txtResetStatus");

            UIObject checkBoxUIObject = FindElement.ById(checkBoxName);
            Verify.IsNotNull(checkBoxUIObject);
            CheckBox checkBox = new CheckBox(checkBoxUIObject);
            Verify.IsNotNull(checkBox);

            if (checkBox.ToggleState == ToggleState.On)
            {
                Log.Comment("CheckBox already checked.");
            }
            else
            {
                checkBox.GetToggledWaiter().TryWait();
                if (checkBox.ToggleState != ToggleState.On)
                {
                    Log.Warning($"{checkBoxName} was not checked.");
                    throw new WaiterException();
                }
                Log.Comment("CheckBox checked.");
            }
        }

        private void UpdateTraces()
        {
            Log.Comment("Updating full log:");
            LogEditValue("txtResetStatus");

            // Triggering ScrollViewsWithSimpleContentsPage.GetFullLog() call.
            TextInput.SendText("g");
            WaitForBoxChecked("chkLogUpdated");
        }

        private void LogTraces()
        {
            UpdateTraces();

            Log.Comment("Reading full log:");

            UIObject fullLogUIObject = FindElement.ById("cmbFullLog");
            Verify.IsNotNull(fullLogUIObject);
            ComboBox cmbFullLog = new ComboBox(fullLogUIObject);
            Verify.IsNotNull(cmbFullLog);

            foreach (ComboBoxItem item in cmbFullLog.AllItems)
            {
                Log.Comment(item.Name);
            }
        }

        private void ClearTraces()
        {
            Log.Comment("Clearing full log.");
            LogEditValue("txtResetStatus");

            // Triggering ScrollViewsWithSimpleContentsPage.ClearFullLog() call.
            TextInput.SendText("c");
            WaitForBoxChecked("chkLogCleared");
        }

        private void LogAndClearTraces()
        {
            LogTraces();
            ClearTraces();
        }

        private int ViewChangeCount()
        {
            int viewChangeCount = 0;

            Log.Comment("Counting ViewChanged events in full log:");

            UpdateTraces();

            UIObject fullLogUIObject = FindElement.ById("cmbFullLog");
            Verify.IsNotNull(fullLogUIObject);
            ComboBox cmbFullLog = new ComboBox(fullLogUIObject);
            Verify.IsNotNull(cmbFullLog);

            foreach (ComboBoxItem item in cmbFullLog.AllItems)
            {
                if (item.Name.Contains("PART_Root.PART_ScrollPresenter ViewChanged"))
                {
                    viewChangeCount++;
                }
            }

            Log.Comment($"Log Entries Count={cmbFullLog.AllItems.Count}.");
            Log.Comment($"ViewChanged Count={viewChangeCount}.");

            ClearTraces();

            return viewChangeCount;
        }

        private void SetScrollPresenterLoggingLevel(bool isPrivateLoggingEnabled)
        {
            Log.Comment("Retrieving chkLogScrollPresenterMessages");
            CheckBox chkLogScrollPresenterMessages = new CheckBox(FindElement.ById("chkLogScrollPresenterMessages"));
            Verify.IsNotNull(chkLogScrollPresenterMessages, "Verifying that chkLogScrollPresenterMessages was found");

            if (isPrivateLoggingEnabled && chkLogScrollPresenterMessages.ToggleState != ToggleState.On ||
                !isPrivateLoggingEnabled && chkLogScrollPresenterMessages.ToggleState != ToggleState.Off)
            {
                Log.Comment("Toggling chkLogScrollPresenterMessages.IsChecked to " + isPrivateLoggingEnabled);
                chkLogScrollPresenterMessages.Toggle();
                Wait.ForIdle();
            }
        }

        private void SetScrollViewLoggingLevel(bool isPrivateLoggingEnabled)
        {
            Log.Comment("Retrieving chkLogScrollViewMessages");
            CheckBox chkLogScrollViewMessages = new CheckBox(FindElement.ById("chkLogScrollViewMessages"));
            Verify.IsNotNull(chkLogScrollViewMessages, "Verifying that chkLogScrollViewMessages was found");

            if (isPrivateLoggingEnabled && chkLogScrollViewMessages.ToggleState != ToggleState.On ||
                !isPrivateLoggingEnabled && chkLogScrollViewMessages.ToggleState != ToggleState.Off)
            {
                Log.Comment("Toggling chkLogScrollViewMessages.IsChecked to " + isPrivateLoggingEnabled);
                chkLogScrollViewMessages.Toggle();
                Wait.ForIdle();
            }
        }

        private class LoggingHelper : IDisposable
        {
            private ScrollViewTestsWithInputHelper m_owner;

            public LoggingHelper(ScrollViewTestsWithInputHelper owner)
            {
                m_owner = owner;

                m_owner.SetScrollPresenterLoggingLevel(isPrivateLoggingEnabled: true);
                m_owner.SetScrollViewLoggingLevel(isPrivateLoggingEnabled: true);
            }

            public void Dispose()
            {
                m_owner.SetScrollPresenterLoggingLevel(isPrivateLoggingEnabled: false);
                m_owner.SetScrollViewLoggingLevel(isPrivateLoggingEnabled: false);
            }
        }
    }
}