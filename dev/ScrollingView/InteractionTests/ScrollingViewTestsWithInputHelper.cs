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
    public class ScrollViewerTestsWithInputHelper
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
        [TestProperty("Description", "Pans an Image in a ScrollViewer.")]
        public void PanScrollViewer()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            const double minHorizontalScrollPercent = 35.0;
            const double minVerticalScrollPercent = 35.0;

            Log.Comment("Selecting ScrollViewer tests");

            using (var setup = new TestSetupHelper(new[] { "ScrollViewer Tests", "navigateToSimpleContents" }))
            {
                Log.Comment("Retrieving cmbShowScrollViewer");
                ComboBox cmbShowScrollViewer = new ComboBox(FindElement.ByName("cmbShowScrollViewer"));
                Verify.IsNotNull(cmbShowScrollViewer, "Verifying that cmbShowScrollViewer was found");

                Log.Comment("Changing ScrollViewer selection to scrollViewer51");
                cmbShowScrollViewer.SelectItemByName("scrollViewer_51");
                Log.Comment("Selection is now {0}", cmbShowScrollViewer.Selection[0].Name);

                if (PlatformConfiguration.IsOsVersion(OSVersion.Redstone1))
                {
                    Log.Comment("On RS1 the ScrollingPresenter's content is centered in an animated way when it's smaller than the viewport. Waiting for those animations to complete.");
                    WaitForScrollViewerManipulationEnd("scrollViewer21");
                }

                Log.Comment("Retrieving img51");
                UIObject img51UIObject = FindElement.ByName("img51");
                Verify.IsNotNull(img51UIObject, "Verifying that img51 was found");

                Log.Comment("Retrieving scrollingPresenter51");
                ScrollingPresenter scrollingPresenter51 = new ScrollingPresenter(img51UIObject.Parent);
                Verify.IsNotNull(scrollingPresenter51, "Verifying that scrollingPresenter51 was found");

                WaitForScrollViewerFinalSize(scrollingPresenter51, 300.0 /*expectedWidth*/, 400.0 /*expectedHeight*/);

                // Tapping button before attempting pan operation to guarantee effective touch input
                TapResetViewsButton();

                Log.Comment("Panning ScrollViewer in diagonal");
                PrepareForScrollViewerManipulationStart();

                InputHelper.Pan(
                    scrollingPresenter51,
                    new Point(scrollingPresenter51.BoundingRectangle.Left + 25, scrollingPresenter51.BoundingRectangle.Top + 25),
                    new Point(scrollingPresenter51.BoundingRectangle.Left - 25, scrollingPresenter51.BoundingRectangle.Top - 25));

                Log.Comment("Waiting for scrollViewer51 pan completion");
                WaitForScrollViewerManipulationEnd("scrollViewer51");

                Log.Comment("scrollingPresenter51.HorizontalScrollPercent={0}", scrollingPresenter51.HorizontalScrollPercent);
                Log.Comment("scrollingPresenter51.VerticalScrollPercent={0}", scrollingPresenter51.VerticalScrollPercent);

                if (scrollingPresenter51.HorizontalScrollPercent <= minHorizontalScrollPercent || scrollingPresenter51.VerticalScrollPercent <= minVerticalScrollPercent)
                {
                    LogAndClearTraces();
                }

                Verify.IsTrue(scrollingPresenter51.HorizontalScrollPercent > minHorizontalScrollPercent, "Verifying scrollingPresenter51 HorizontalScrollPercent is greater than " + minHorizontalScrollPercent + "%");
                Verify.IsTrue(scrollingPresenter51.VerticalScrollPercent > minVerticalScrollPercent, "Verifying scrollingPresenter51 VerticalScrollPercent is greater than " + minVerticalScrollPercent + "%");

                // scrollingPresenter51's Content size is 800x800px.
                double horizontalOffset;
                double verticalOffset;
                double minHorizontalOffset = 800.0 * (1.0 - scrollingPresenter51.HorizontalViewSize / 100.0) * minHorizontalScrollPercent / 100.0;
                double minVerticalOffset = 800.0 * (1.0 - scrollingPresenter51.VerticalViewSize / 100.0) * minVerticalScrollPercent / 100.0;
                float zoomFactor;

                GetScrollingPresenterView(out horizontalOffset, out verticalOffset, out zoomFactor);
                Log.Comment("horizontalOffset={0}", horizontalOffset);
                Log.Comment("verticalOffset={0}", verticalOffset);
                Log.Comment("zoomFactor={0}", zoomFactor);
                Verify.IsTrue(horizontalOffset > minHorizontalOffset, "Verifying horizontalOffset is greater than " + minHorizontalOffset);
                Verify.IsTrue(verticalOffset > minVerticalOffset, "Verifying verticalOffset is greater than " + minVerticalOffset);
                Verify.AreEqual(zoomFactor, 1.0f, "Verifying zoomFactor is 1.0f");

                // Output-debug-string-level "None" is automatically restored when landing back on the ScrollViewer test page.
            }
        }

        [TestMethod]
        [TestProperty("Description", "Scrolls an Image in a ScrollViewer using the mouse on the ScrollBar thumb, then pans it with touch.")]
        public void ScrollThenPanScrollViewer()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            Log.Comment("Selecting ScrollViewer tests");

            using (var setup = new TestSetupHelper(new[] { "ScrollViewer Tests", "navigateToSimpleContents" }))
            {
                const double minVerticalScrollPercentAfterScroll = 15.0;
                const double minHorizontalScrollPercentAfterPan = 35.0;
                const double minVerticalScrollPercentAfterPan = 50.0;

                double verticalScrollPercentAfterScroll = 0.0;

                Log.Comment("Retrieving cmbShowScrollViewer");
                ComboBox cmbShowScrollViewer = new ComboBox(FindElement.ByName("cmbShowScrollViewer"));
                Verify.IsNotNull(cmbShowScrollViewer, "Verifying that cmbShowScrollViewer was found");

                Log.Comment("Changing ScrollViewer selection to scrollViewer51");
                cmbShowScrollViewer.SelectItemByName("scrollViewer_51");
                Log.Comment("Selection is now {0}", cmbShowScrollViewer.Selection[0].Name);

                if (PlatformConfiguration.IsOsVersion(OSVersion.Redstone1))
                {
                    Log.Comment("On RS1 the ScrollingPresenter's content is centered in an animated way when it's smaller than the viewport. Waiting for those animations to complete.");
                    WaitForScrollViewerManipulationEnd("scrollViewer21");
                }

                Log.Comment("Retrieving img51");
                UIObject img51UIObject = FindElement.ByName("img51");
                Verify.IsNotNull(img51UIObject, "Verifying that img51 was found");

                Log.Comment("Retrieving scrollingPresenter51");
                ScrollingPresenter scrollingPresenter51 = new ScrollingPresenter(img51UIObject.Parent);
                Verify.IsNotNull(scrollingPresenter51, "Verifying that scrollingPresenter51 was found");

                WaitForScrollViewerFinalSize(scrollingPresenter51, 300.0 /*expectedWidth*/, 400.0 /*expectedHeight*/);

                // Tapping button before attempting pan operation to guarantee effective touch input
                TapResetViewsButton();

                Log.Comment("Left mouse buttom down over ScrollBar thumb");
                InputHelper.LeftMouseButtonDown(scrollingPresenter51, 140 /*offsetX*/, -100 /*offsetY*/);

                Log.Comment("Mouse drag and left mouse buttom up over ScrollBar thumb");
                InputHelper.LeftMouseButtonUp(scrollingPresenter51, 140 /*offsetX*/, -50 /*offsetY*/);

                Log.Comment("scrollingPresenter51.HorizontalScrollPercent={0}", scrollingPresenter51.HorizontalScrollPercent);
                Log.Comment("scrollingPresenter51.VerticalScrollPercent={0}", scrollingPresenter51.VerticalScrollPercent);

                verticalScrollPercentAfterScroll = scrollingPresenter51.VerticalScrollPercent;

                if (scrollingPresenter51.HorizontalScrollPercent != 0.0 || scrollingPresenter51.VerticalScrollPercent <= minVerticalScrollPercentAfterScroll)
                {
                    LogAndClearTraces();
                }

                Verify.AreEqual(scrollingPresenter51.HorizontalScrollPercent, 0.0, "Verifying scrollingPresenter51 HorizontalScrollPercent is still 0%");
                Verify.IsTrue(verticalScrollPercentAfterScroll > minVerticalScrollPercentAfterScroll, "Verifying scrollingPresenter51 VerticalScrollPercent is greater than " + minVerticalScrollPercentAfterScroll + "%");

                Log.Comment("Panning ScrollViewer in diagonal");
                PrepareForScrollViewerManipulationStart();

                // Using a large enough span and duration for this diagonal pan so that it is not erroneously recognized as a horizontal pan.
                InputHelper.Pan(
                    scrollingPresenter51,
                    new Point(scrollingPresenter51.BoundingRectangle.Left + 30, scrollingPresenter51.BoundingRectangle.Top + 30),
                    new Point(scrollingPresenter51.BoundingRectangle.Left - 30, scrollingPresenter51.BoundingRectangle.Top - 30),
                    InputHelper.DefaultPanHoldDuration,
                    InputHelper.DefaultPanAcceleration / 2.4f);

                Log.Comment("Waiting for scrollViewer51 pan completion");
                WaitForScrollViewerManipulationEnd("scrollViewer51");

                Log.Comment("scrollingPresenter51.HorizontalScrollPercent={0}", scrollingPresenter51.HorizontalScrollPercent);
                Log.Comment("scrollingPresenter51.VerticalScrollPercent={0}", scrollingPresenter51.VerticalScrollPercent);

                if (scrollingPresenter51.HorizontalScrollPercent <= minHorizontalScrollPercentAfterPan ||
                    scrollingPresenter51.VerticalScrollPercent <= minVerticalScrollPercentAfterPan ||
                    scrollingPresenter51.VerticalScrollPercent <= verticalScrollPercentAfterScroll)
                {
                    LogAndClearTraces();
                }

                Verify.IsTrue(scrollingPresenter51.HorizontalScrollPercent > minHorizontalScrollPercentAfterPan, "Verifying scrollingPresenter51 HorizontalScrollPercent is greater than " + minHorizontalScrollPercentAfterPan + "%");
                Verify.IsTrue(scrollingPresenter51.VerticalScrollPercent > minVerticalScrollPercentAfterPan, "Verifying scrollingPresenter51 VerticalScrollPercent is greater than " + minVerticalScrollPercentAfterPan + "%");
                Verify.IsTrue(scrollingPresenter51.VerticalScrollPercent > verticalScrollPercentAfterScroll, "Verifying scrollingPresenter51 VerticalScrollPercent is greater than " + verticalScrollPercentAfterScroll + "%");

                // scrollingPresenter51's Content size is 800x800px.
                double horizontalOffset;
                double verticalOffset;
                double minHorizontalOffset = 800.0 * (1.0 - scrollingPresenter51.HorizontalViewSize / 100.0) * minHorizontalScrollPercentAfterPan / 100.0;
                double minVerticalOffset = 800.0 * (1.0 - scrollingPresenter51.VerticalViewSize / 100.0) * minVerticalScrollPercentAfterPan / 100.0;
                float zoomFactor;

                GetScrollingPresenterView(out horizontalOffset, out verticalOffset, out zoomFactor);
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
        public void VerifyScrollViewerKeyboardInteraction()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollViewer not supported pre-RS2");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollViewer Tests", "navigateToSimpleContents" }))
            {
                UIObject buttonInScrollViewer11;
                ScrollingPresenter scrollingPresenter11;
                SetupSimpleSingleScrollViewerTest(out buttonInScrollViewer11, out scrollingPresenter11);

                var scrollAmountForDownOrUpKey = scrollingPresenter11.BoundingRectangle.Height * 0.15;
                var scrollAmountForPageUpOrPageDownKey = scrollingPresenter11.BoundingRectangle.Height;
                var scrollAmountForRightOrLeftKey = scrollingPresenter11.BoundingRectangle.Width * 0.15;
                var maxScrollOffset = 2000 - scrollingPresenter11.BoundingRectangle.Height;

                double expectedVerticalOffset = 0;
                double expectedHorizontalOffset = 0;

                Log.Comment("Pressing Down key");
                KeyboardHelper.PressKey(buttonInScrollViewer11, Key.Down, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedVerticalOffset += scrollAmountForDownOrUpKey;
                WaitForScrollViewerOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing PageDown key");
                KeyboardHelper.PressKey(buttonInScrollViewer11, Key.PageDown, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedVerticalOffset += scrollAmountForPageUpOrPageDownKey;
                WaitForScrollViewerOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing Home key");
                KeyboardHelper.PressKey(buttonInScrollViewer11, Key.Home, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedVerticalOffset = 0;
                WaitForScrollViewerOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing End key");
                KeyboardHelper.PressKey(buttonInScrollViewer11, Key.End, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedVerticalOffset = maxScrollOffset;
                WaitForScrollViewerOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing Up key");
                KeyboardHelper.PressKey(buttonInScrollViewer11, Key.Up, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedVerticalOffset -= scrollAmountForDownOrUpKey;
                WaitForScrollViewerOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing PageUp key");
                KeyboardHelper.PressKey(buttonInScrollViewer11, Key.PageUp, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedVerticalOffset -= scrollAmountForPageUpOrPageDownKey;
                WaitForScrollViewerOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing Right key");
                KeyboardHelper.PressKey(buttonInScrollViewer11, Key.Right, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedHorizontalOffset += scrollAmountForRightOrLeftKey;
                WaitForScrollViewerOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing Left key");
                KeyboardHelper.PressKey(buttonInScrollViewer11, Key.Left, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                expectedHorizontalOffset -= scrollAmountForRightOrLeftKey;
                WaitForScrollViewerOffsets(expectedHorizontalOffset, expectedVerticalOffset);

                Log.Comment("Pressing Down key three times");
                KeyboardHelper.PressKey(buttonInScrollViewer11, Key.Down, modifierKey: ModifierKey.None, numPresses: 3, useDebugMode: true);
                expectedVerticalOffset += 3 * scrollAmountForDownOrUpKey;
                WaitForScrollViewerOffsets(expectedHorizontalOffset, expectedVerticalOffset);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies keyboard input is ignored when ScrollViewer.IgnoredInputKind is Keyboard.")]
        public void VerifyScrollViewerIgnoresKeyboardInput()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollViewer not supported pre-RS2");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollViewer Tests", "navigateToSimpleContents" }))
            {
                UIObject img51;
                ScrollingPresenter scrollingPresenter51;

                SetupScrollViewerTestWithImage("51", out img51, out scrollingPresenter51);

                Log.Comment("Retrieving cmbIgnoredInputKind");
                ComboBox cmbIgnoredInputKind = new ComboBox(FindElement.ByName("cmbIgnoredInputKind"));
                Verify.IsNotNull(cmbIgnoredInputKind, "Verifying that cmbIgnoredInputKind was found");

                Log.Comment("Changing ScrollViewer.IgnoredInputKind to Keyboard");
                cmbIgnoredInputKind.SelectItemByName("Keyboard");
                Log.Comment("Selection is now {0}", cmbIgnoredInputKind.Selection[0].Name);

                Log.Comment("Pressing Down key");
                KeyboardHelper.PressKey(scrollingPresenter51, Key.Down, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                VerifyScrollViewerRemainsAtView(0.0, 0.0, 1.0f);

                Log.Comment("Pressing Right key");
                KeyboardHelper.PressKey(scrollingPresenter51, Key.Right, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                VerifyScrollViewerRemainsAtView(0.0, 0.0, 1.0f);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests keyboard interaction (Down, Up, PageDown, PageUp, End, Home, Right, Left) when ScrollViewer.XYFocusKeyboardNavigation is Enabled.")]
        public void VerifyScrollViewerKeyboardInteractionWithXYFocusEnabled()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollViewer not supported pre-RS2");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollViewer Tests", "navigateToSimpleContents" }))
            {
                UIObject img51;
                ScrollingPresenter scrollingPresenter51;

                SetupScrollViewerTestWithImage("51", out img51, out scrollingPresenter51);

                var scrollAmountForDownOrUpKey = scrollingPresenter51.BoundingRectangle.Height * 0.5;
                var scrollAmountForPageUpOrPageDownKey = scrollingPresenter51.BoundingRectangle.Height;
                var scrollAmountForRightOrLeftKey = scrollingPresenter51.BoundingRectangle.Width * 0.5;
                var maxScrollOffset = 800 - scrollingPresenter51.BoundingRectangle.Height;

                Log.Comment("Pressing Down key");
                KeyboardHelper.PressKey(scrollingPresenter51, Key.Down, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewerOffsets(0, scrollAmountForDownOrUpKey);

                Log.Comment("Pressing Up key");
                KeyboardHelper.PressKey(scrollingPresenter51, Key.Up, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewerOffsets(0, 0);

                Log.Comment("Pressing PageDown key");
                KeyboardHelper.PressKey(scrollingPresenter51, Key.PageDown, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewerOffsets(0, scrollAmountForPageUpOrPageDownKey);

                Log.Comment("Pressing PageUp key");
                KeyboardHelper.PressKey(scrollingPresenter51, Key.PageUp, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewerOffsets(0, 0);

                Log.Comment("Pressing End key");
                KeyboardHelper.PressKey(scrollingPresenter51, Key.End, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewerOffsets(0, maxScrollOffset);

                Log.Comment("Pressing Home key");
                KeyboardHelper.PressKey(scrollingPresenter51, Key.Home, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewerOffsets(0, 0);

                Log.Comment("Pressing Right key");
                KeyboardHelper.PressKey(scrollingPresenter51, Key.Right, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewerOffsets(scrollAmountForRightOrLeftKey, 0);

                Log.Comment("Pressing Left key");
                KeyboardHelper.PressKey(scrollingPresenter51, Key.Left, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewerOffsets(0, 0);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests End and Home keys when ScrollViewer.VerticalScrollMode is Disabled.")]
        public void ScrollHorizontallyWithEndHomeKeys()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollViewer not supported pre-RS2");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollViewer Tests", "navigateToSimpleContents" }))
            {
                UIObject img31;
                ScrollingPresenter scrollingPresenter31;

                SetupScrollViewerTestWithImage("31", out img31, out scrollingPresenter31);

                Log.Comment("Pressing End key");
                KeyboardHelper.PressKey(scrollingPresenter31, Key.End, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewerOffsets(900 - scrollingPresenter31.BoundingRectangle.Width, 0);

                Log.Comment("Pressing Home key");
                KeyboardHelper.PressKey(scrollingPresenter31, Key.Home, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewerOffsets(0, 0);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests End and Home keys when ScrollViewer.VerticalScrollMode is Disabled in RightToLeft flow direction.")]
        public void ScrollHorizontallyWithEndHomeKeysInRTL()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollViewer not supported pre-RS2");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollViewer Tests", "navigateToSimpleContents" }))
            {
                UIObject img32;
                ScrollingPresenter scrollingPresenter32;

                SetupScrollViewerTestWithImage("32", out img32, out scrollingPresenter32);

                Log.Comment("Pressing Home key");
                KeyboardHelper.PressKey(scrollingPresenter32, Key.Home, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewerOffsets(900 - scrollingPresenter32.BoundingRectangle.Width, 0);

                Log.Comment("Pressing End key");
                KeyboardHelper.PressKey(scrollingPresenter32, Key.End, modifierKey: ModifierKey.None, numPresses: 1, useDebugMode: true);
                WaitForScrollViewerOffsets(0, 0);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests GamePad interaction")]
        public void VerifyScrollViewerGamePadInteraction()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
            {
                Log.Warning("Test is disabled on pre-RS4 because ScrollViewer Gamepad interaction is not supported pre-RS4");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollViewer Tests", "navigateToSimpleContents" }))
            {
                using (var loggingHelper = new LoggingHelper(this))
                {
                    UISettings settings = new UISettings();
                    bool areAnimationsEnabled = settings.AnimationsEnabled;

                    UIObject buttonInScrollViewer11;
                    ScrollingPresenter scrollingPresenter11;
                    SetupSimpleSingleScrollViewerTest(out buttonInScrollViewer11, out scrollingPresenter11);

                    Log.Comment("Tapping Button 1");
                    InputHelper.Tap(buttonInScrollViewer11);

                    Log.Comment($"Focused element. Expected=Button 1, Actual={UIObject.Focused.Name}.");
                    Verify.AreEqual("Button 1", UIObject.Focused.Name, "Verify focused element");

                    var scrollAmountForGamepadUpDown = scrollingPresenter11.BoundingRectangle.Height * 0.5;
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
        [TestProperty("Description", "Verifies gamepad input is ignored when ScrollViewer.IgnoredInputKind is Gamepad.")]
        public void VerifyScrollViewerIgnoresGamepadInput()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 because ScrollViewer not supported pre-RS2");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollViewer Tests", "navigateToSimpleContents" }))
            {
                UIObject img51;
                ScrollingPresenter scrollingPresenter51;

                SetupScrollViewerTestWithImage("51", out img51, out scrollingPresenter51);

                Log.Comment("Retrieving cmbIgnoredInputKind");
                ComboBox cmbIgnoredInputKind = new ComboBox(FindElement.ByName("cmbIgnoredInputKind"));
                Verify.IsNotNull(cmbIgnoredInputKind, "Verifying that cmbIgnoredInputKind was found");

                Log.Comment("Changing ScrollViewer.IgnoredInputKind to Gamepad");
                cmbIgnoredInputKind.SelectItemByName("Gamepad");
                Log.Comment("Selection is now {0}", cmbIgnoredInputKind.Selection[0].Name);

                Log.Comment("Pressing LeftThumbstick Down");
                GamepadHelper.PressButton(null, GamepadButton.LeftThumbstickDown);

                Log.Comment("Pressing LeftThumbstick Down");
                GamepadHelper.PressButton(null, GamepadButton.LeftThumbstickDown);
                VerifyScrollViewerRemainsAtView(0.0, 0.0, 1.0f);

                Log.Comment("Pressing LeftThumbstick Right");
                GamepadHelper.PressButton(null, GamepadButton.LeftThumbstickRight);
                VerifyScrollViewerRemainsAtView(0.0, 0.0, 1.0f);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests GamePad interaction")]
        public void VerifyScrollViewerGamePadHorizontalInteraction()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
            {
                Log.Warning("Test is disabled on pre-RS4 because ScrollViewer Gamepad interaction is not supported pre-RS4");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollViewer Tests", "navigateToSimpleContents" }))
            {
                using (var loggingHelper = new LoggingHelper(this))
                {
                    UISettings settings = new UISettings();
                    bool areAnimationsEnabled = settings.AnimationsEnabled;

                    UIObject buttonInScrollViewer11;
                    ScrollingPresenter scrollingPresenter11;
                    SetupSimpleSingleScrollViewerTest(out buttonInScrollViewer11, out scrollingPresenter11);

                    var scrollAmountForGamepadLeftRight = scrollingPresenter11.BoundingRectangle.Width * 0.5;

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
        public void VerifyScrollViewerGamePadTriggerInteraction()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
            {
                Log.Warning("Test is disabled on pre-RS4 because ScrollViewer Gamepad interaction is not supported pre-RS4");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollViewer Tests", "navigateToSimpleContents" }))
            {
                using (var loggingHelper = new LoggingHelper(this))
                {
                    UISettings settings = new UISettings();
                    bool areAnimationsEnabled = settings.AnimationsEnabled;

                    UIObject buttonInScrollViewer11;
                    ScrollingPresenter scrollingPresenter11;
                    SetupSimpleSingleScrollViewerTest(out buttonInScrollViewer11, out scrollingPresenter11);

                    var scrollAmountForGamepadTrigger = scrollingPresenter11.BoundingRectangle.Height;

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
                    GetScrollingPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);

                    //Up. Change focus. Scroll.
                    expectedVerticalOffset = 0;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftTrigger, actualVerticalOffset < scrollAmountForGamepadTrigger ? "Button 1" : "Button 2", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Tests GamePad interaction")]
        public void VerifyScrollViewerGamePadBumperInteraction()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone4))
            {
                Log.Warning("Test is disabled on pre-RS4 because ScrollViewer Gamepad interaction is not supported pre-RS4");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "ScrollViewer Tests", "navigateToSimpleContents" }))
            {
                using (var loggingHelper = new LoggingHelper(this))
                {
                    UISettings settings = new UISettings();
                    bool areAnimationsEnabled = settings.AnimationsEnabled;

                    UIObject buttonInScrollViewer11;
                    ScrollingPresenter scrollingPresenter11;
                    SetupSimpleSingleScrollViewerTest(out buttonInScrollViewer11, out scrollingPresenter11);

                    var scrollAmountForBumper = scrollingPresenter11.BoundingRectangle.Width;

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
                    GetScrollingPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);

                    //Left. Change focus. Scroll.
                    expectedHorizontalOffset = areAnimationsEnabled ? 0 : 80.0;
                    PressGamepadButtonAndVerifyOffsetAndFocus(GamepadButton.LeftShoulder, actualHorizontalOffset > scrollAmountForBumper ? "Button 1B" : "Button 1", expectedHorizontalOffset, expectedVerticalOffset, expectedViewChangeCount);
                }
            }
        }

        private void SetupSimpleSingleScrollViewerTest(out UIObject buttonInScrollViewer11, out ScrollingPresenter scrollingPresenter11)
        {
            Log.Comment("Retrieving cmbShowScrollViewer");
            ComboBox cmbShowScrollViewer = new ComboBox(FindElement.ByName("cmbShowScrollViewer"));
            Verify.IsNotNull(cmbShowScrollViewer, "Verifying that cmbShowScrollViewer was found");

            Log.Comment("Changing ScrollViewer selection to scrollViewer11");
            cmbShowScrollViewer.SelectItemByName("scrollViewer_11");
            Log.Comment("Selection is now {0}", cmbShowScrollViewer.Selection[0].Name);

            Log.Comment("Retrieving buttonInScrollViewer11");
            buttonInScrollViewer11 = FindElement.ById("buttonInScrollViewer11");
            Verify.IsNotNull(buttonInScrollViewer11, "Verifying that buttonInScrollViewer11 was found");

            Log.Comment("Retrieving scrollingPresenter11");
            scrollingPresenter11 = new ScrollingPresenter(buttonInScrollViewer11.Parent);
            Verify.IsNotNull(scrollingPresenter11, "Verifying that scrollingPresenter11 was found");

            WaitForScrollViewerFinalSize(scrollingPresenter11, 300.0 /*expectedWidth*/, 400.0 /*expectedHeight*/);

            buttonInScrollViewer11.Click();
            Wait.ForIdle();
        }

        private void SetupScrollViewerTestWithImage(string suffix, out UIObject imageInScrollViewer, out ScrollingPresenter scrollingPresenter)
        {
            Log.Comment("Retrieving cmbShowScrollViewer");
            ComboBox cmbShowScrollViewer = new ComboBox(FindElement.ByName("cmbShowScrollViewer"));
            Verify.IsNotNull(cmbShowScrollViewer, "Verifying that cmbShowScrollViewer was found");

            Log.Comment("Changing ScrollViewer selection to scrollViewer" + suffix);
            cmbShowScrollViewer.SelectItemByName("scrollViewer_" + suffix);
            Log.Comment("Selection is now {0}", cmbShowScrollViewer.Selection[0].Name);

            Log.Comment("Retrieving img" + suffix);
            imageInScrollViewer = FindElement.ById("img" + suffix);
            Verify.IsNotNull(imageInScrollViewer, "Verifying that img" + suffix + " was found");

            Log.Comment("Retrieving ScrollingPresenter");
            scrollingPresenter = new ScrollingPresenter(imageInScrollViewer.Parent);
            Verify.IsNotNull(scrollingPresenter, "Verifying that ScrollingPresenter was found");

            WaitForScrollViewerFinalSize(scrollingPresenter, 300.0 /*expectedWidth*/, 400.0 /*expectedHeight*/);

            imageInScrollViewer.Click();
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

            WaitForScrollViewerOffsets(expectedHorizontalOffset, expectedVerticalOffset);

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

        private void WaitForScrollViewerOffsets(double expectedHorizontalOffset, double expectedVerticalOffset)
        {
            Log.Comment("Waiting for ScrollViewer offsets: {0}, {1}", expectedHorizontalOffset, expectedVerticalOffset);

            double actualHorizontalOffset;
            double actualVerticalOffset;
            float actualZoomFactor;
            GetScrollingPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);

            Func<bool> areOffsetsCorrect = () => AreClose(expectedHorizontalOffset, actualHorizontalOffset) && AreClose(expectedVerticalOffset, actualVerticalOffset);

            int triesRemaining = 10;
            while (!areOffsetsCorrect() && triesRemaining-- > 0)
            {
                Thread.Sleep(500);
                GetScrollingPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);
            }

            if (triesRemaining >= 0)
            {
                // Allow the view to settle and the STateChanged, ScrollCompleted or ZoomCompleted events to be raised.
                Thread.Sleep(250);
                GetScrollingPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);
            }

            Log.Comment($"Final ScrollViewer offsets. Expected={expectedHorizontalOffset},{expectedVerticalOffset}, Actual={actualHorizontalOffset},{actualVerticalOffset}.");
            if (!areOffsetsCorrect())
            {
                LogAndClearTraces();
            }
            Verify.IsTrue(areOffsetsCorrect(), String.Format("Verify ScrollViewer offsets. Expected={0},{1}, Actual={2},{3}.",
                expectedHorizontalOffset, expectedVerticalOffset, actualHorizontalOffset, actualVerticalOffset));
        }

        private void VerifyScrollViewerRemainsAtView(double expectedHorizontalOffset, double expectedVerticalOffset, float expectedZoomFactor)
        {
            Log.Comment("Verifying ScrollViewer view remains at: {0}, {1}, {2}",
                expectedHorizontalOffset, expectedVerticalOffset, expectedZoomFactor);

            double actualHorizontalOffset;
            double actualVerticalOffset;
            float actualZoomFactor;

            GetScrollingPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);

            Func<bool> isViewCorrect = () =>
                AreClose(expectedHorizontalOffset, actualHorizontalOffset) &&
                AreClose(expectedVerticalOffset, actualVerticalOffset) &&
                AreClose(expectedZoomFactor, actualZoomFactor);

            Verify.IsTrue(isViewCorrect(), String.Format("Verify ScrollViewer initial view. Expected={0},{1},{2}, Actual={3},{4},{5}.",
                    expectedHorizontalOffset, expectedVerticalOffset, expectedZoomFactor,
                    actualHorizontalOffset, actualVerticalOffset, actualZoomFactor));

            Thread.Sleep(750);

            GetScrollingPresenterView(out actualHorizontalOffset, out actualVerticalOffset, out actualZoomFactor);

            Verify.IsTrue(isViewCorrect(), String.Format("Verify ScrollViewer final view. Expected={0},{1},{2}, Actual={3},{4},{5}.",
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

        private void GetScrollingPresenterView(out double horizontalOffset, out double verticalOffset, out float zoomFactor)
        {
            horizontalOffset = 0.0;
            verticalOffset = 0.0;
            zoomFactor = 1.0f;

            UIObject viewUIObject = FindElement.ById("txtScrollingPresenterHorizontalOffset");
            Edit viewTextBox = new Edit(viewUIObject);
            Log.Comment("Current HorizontalOffset: " + viewTextBox.Value);
            horizontalOffset = String.IsNullOrWhiteSpace(viewTextBox.Value) ? double.NaN : Convert.ToDouble(viewTextBox.Value);

            viewUIObject = FindElement.ById("txtScrollingPresenterVerticalOffset");
            viewTextBox = new Edit(viewUIObject);
            Log.Comment("Current VerticalOffset: " + viewTextBox.Value);
            verticalOffset = String.IsNullOrWhiteSpace(viewTextBox.Value) ? double.NaN : Convert.ToDouble(viewTextBox.Value);

            viewUIObject = FindElement.ById("txtScrollingPresenterZoomFactor");
            viewTextBox = new Edit(viewUIObject);
            Log.Comment("Current ZoomFactor: " + viewTextBox.Value);
            zoomFactor = String.IsNullOrWhiteSpace(viewTextBox.Value) ? float.NaN : Convert.ToSingle(viewTextBox.Value);
        }

        private void PrepareForScrollViewerManipulationStart(string stateTextBoxName = "txtScrollingPresenterState")
        {
            UIObject scrollingPresenterStateUIObject = FindElement.ById(stateTextBoxName);
            Edit scrollingPresenterStateTextBox = new Edit(scrollingPresenterStateUIObject);
            Log.Comment("Pre-manipulation ScrollingPresenterState: " + scrollingPresenterStateTextBox.Value);
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

        private void WaitForScrollViewerManipulationEnd(string scrollViewerName, string stateTextBoxName = "txtScrollingPresenterState")
        {
            WaitForManipulationEnd(scrollViewerName, stateTextBoxName);
        }

        private bool TryWaitForScrollViewerManipulationEnd(string scrollViewerName, string stateTextBoxName = "txtScrollingPresenterState")
        {
            return WaitForManipulationEnd(scrollViewerName, stateTextBoxName, false /*throwOnError*/);
        }

        private bool WaitForManipulationEnd(string elementName, string stateTextBoxName, bool throwOnError = true)
        {
            UIObject elementStateUIObject = FindElement.ById(stateTextBoxName);
            Edit elementStateTextBox = new Edit(elementStateUIObject);
            Log.Comment("Current State: " + elementStateTextBox.Value);
            if (elementStateTextBox.Value != elementName + ".PART_Root.PART_ScrollingPresenter Idle")
            {
                using (var waiter = new ValueChangedEventWaiter(elementStateTextBox, elementName + ".PART_Root.PART_ScrollingPresenter Idle"))
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
                            if (elementStateTextBox.Value == elementName + ".PART_Root.PART_ScrollingPresenter Idle")
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

        private void WaitForScrollViewerFinalSize(UIObject scrollViewerUIObject, double expectedWidth, double expectedHeight)
        {
            int pauses = 0;
            int widthDelta = Math.Abs(scrollViewerUIObject.BoundingRectangle.Width - (int)expectedWidth);
            int heightDelta = Math.Abs(scrollViewerUIObject.BoundingRectangle.Height - (int)expectedHeight);

            Log.Comment("scrollViewerUIObject.BoundingRectangle={0}", scrollViewerUIObject.BoundingRectangle);

            while (widthDelta > 1 || heightDelta > 1 && pauses < 5)
            {
                Wait.ForMilliseconds(60);
                pauses++;
                Log.Comment("scrollViewerUIObject.BoundingRectangle={0}", scrollViewerUIObject.BoundingRectangle);
                widthDelta = Math.Abs(scrollViewerUIObject.BoundingRectangle.Width - (int)expectedWidth);
                heightDelta = Math.Abs(scrollViewerUIObject.BoundingRectangle.Height - (int)expectedHeight);
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

            // Triggering ScrollViewersWithSimpleContentsPage.GetFullLog() call.
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

            // Triggering ScrollViewersWithSimpleContentsPage.ClearFullLog() call.
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
                if (item.Name.Contains("PART_Root.PART_ScrollingPresenter ViewChanged"))
                {
                    viewChangeCount++;
                }
            }

            Log.Comment($"Log Entries Count={cmbFullLog.AllItems.Count}.");
            Log.Comment($"ViewChanged Count={viewChangeCount}.");

            ClearTraces();

            return viewChangeCount;
        }

        private void SetScrollingPresenterLoggingLevel(bool isPrivateLoggingEnabled)
        {
            Log.Comment("Retrieving chkLogScrollingPresenterMessages");
            CheckBox chkLogScrollingPresenterMessages = new CheckBox(FindElement.ById("chkLogScrollingPresenterMessages"));
            Verify.IsNotNull(chkLogScrollingPresenterMessages, "Verifying that chkLogScrollingPresenterMessages was found");

            if (isPrivateLoggingEnabled && chkLogScrollingPresenterMessages.ToggleState != ToggleState.On ||
                !isPrivateLoggingEnabled && chkLogScrollingPresenterMessages.ToggleState != ToggleState.Off)
            {
                Log.Comment("Toggling chkLogScrollingPresenterMessages.IsChecked to " + isPrivateLoggingEnabled);
                chkLogScrollingPresenterMessages.Toggle();
                Wait.ForIdle();
            }
        }

        private void SetScrollViewerLoggingLevel(bool isPrivateLoggingEnabled)
        {
            Log.Comment("Retrieving chkLogScrollViewerMessages");
            CheckBox chkLogScrollViewerMessages = new CheckBox(FindElement.ById("chkLogScrollViewerMessages"));
            Verify.IsNotNull(chkLogScrollViewerMessages, "Verifying that chkLogScrollViewerMessages was found");

            if (isPrivateLoggingEnabled && chkLogScrollViewerMessages.ToggleState != ToggleState.On ||
                !isPrivateLoggingEnabled && chkLogScrollViewerMessages.ToggleState != ToggleState.Off)
            {
                Log.Comment("Toggling chkLogScrollViewerMessages.IsChecked to " + isPrivateLoggingEnabled);
                chkLogScrollViewerMessages.Toggle();
                Wait.ForIdle();
            }
        }

        private class LoggingHelper : IDisposable
        {
            private ScrollViewerTestsWithInputHelper m_owner;

            public LoggingHelper(ScrollViewerTestsWithInputHelper owner)
            {
                m_owner = owner;

                m_owner.SetScrollingPresenterLoggingLevel(isPrivateLoggingEnabled: true);
                m_owner.SetScrollViewerLoggingLevel(isPrivateLoggingEnabled: true);
            }

            public void Dispose()
            {
                m_owner.SetScrollingPresenterLoggingLevel(isPrivateLoggingEnabled: false);
                m_owner.SetScrollViewerLoggingLevel(isPrivateLoggingEnabled: false);
            }
        }
    }
}