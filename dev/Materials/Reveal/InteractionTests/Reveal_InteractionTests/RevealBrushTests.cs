// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Common;
using System.Threading.Tasks;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if BUILD_WINDOWS
using System.Windows.Automation;
using MS.Internal.Mita.Foundation;
using MS.Internal.Mita.Foundation.Controls;
using MS.Internal.Mita.Foundation.Patterns;
using MS.Internal.Mita.Foundation.Waiters;
#else
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class RevealBrushTests
    {
        private const string AcrylicRectangleName = "Rectangle1";
        private const string TintOpacitySliderName = "TintOpacity";
        private const string SizeSliderName = "SizeSlider";

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("MUXControlsTestEnabledForPhone", "True")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            if (TestEnvironment.TestContext.CurrentTestOutcome != UnitTestOutcome.Passed)
            {
                Log.Comment("Test failed. Queueing an app restart, since we may now be in an unknown state.");
                TestEnvironment.ShouldRestartApplication = true;
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125
        public void RevealBrushAudit()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Comment("Brush audit needs to be running on RS2 or greater");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToColorReveal" }))
            {
                var result = new Edit(FindElement.ByName("CheckBrushesResult"));

                FindElement.ByName<Button>("CheckBrushes").InvokeAndWait();

                string errors = result.Value;
                if (errors != "")
                {
                    Log.Error("Brush check result = {0}", errors);
                }
            }
        }

        [TestMethod]
        public void RevealAlwaysUseFallback()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Comment("RevealForceFallback needs to be running on RS2 or greater");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToRevealFallback" }))
            {
                ChooseFromComboBox("TestNameComboBox", "RevealAlwaysUseFallback");

                var result = new Edit(FindElement.ById("TestResult"));

                FindElement.ByName<Button>("RunTestButton").InvokeAndWait();

                if (result.Value.Equals("RevealAlwaysUseFallback: Skipped"))
                {
                    Log.Error("Error: FallbackBrush in use - expecting effect brush");
                    return;
                }

                string errors = result.Value;
                if (errors != "")
                {
                    Log.Error("RevealAlwaysUseFallback result = {0}", errors);
                }
            }
        }

        [TestMethod]
        [TestProperty("EnableForPGOTraining", "True")]
        public void RevealButtonStates()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Comment("RevealButtonStates needs to be running on RS2 or greater");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToRevealStates" }))
            {
                Log.Comment("Testing light states for Button... ");

                var target = FindElement.ById("LargeButton");
                ChooseFromComboBox("TargetComboBox", "LargeButton");
                RevealStatesHelper(target);
            }
        }

        [TestMethod]
        public void RevealListViewItemStates()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Comment("RevealListViewItemStates needs to be running on RS2 or greater");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToRevealStates" }))
            {
                Log.Comment("Testing RevealHoverLight states for ListViewItem... ");

                var target = FindElement.ById("NormalListViewItem");
                ChooseFromComboBox("TargetComboBox", "NormalListViewItem");
                RevealStatesHelper(target);
            }
        }

        [TestMethod]
        public void RevealSetStateNoCrashWhenParentIsNotControl()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Comment("RevealSetStateNoCrashWhenParentIsNotControl needs to be running on RS2 or greater");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToRevealStates" }))
            {
                var button = new Button(FindElement.ByName("SetStatePressed"));
                button.Invoke();
                Wait.ForIdle();

                var result = new Edit(FindElement.ById("TestResult"));
                Verify.AreEqual("SetState_Click: Clicked", result.Value);
            }
        }

        private void RevealStatesHelper(UIObject target)
        {
            var result = new Edit(FindElement.ById("TestResult"));

            Log.Comment("Move Mouse to (0,0). Validate target is initially in Normal state.");
            using (var waiter = new ValueChangedEventWaiter(result))
            {
                TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                Wait.ForIdle();
                ChooseFromComboBox("ValidationActionsComboBox", "HoverLight_ValidateNormalState");
                waiter.Wait();
            }
            LogResult(result, "Normal");

            Log.Comment("Move mouse over the target (which will attach RevealHoverLight). Validate PointerOver light state.");
            using (var waiter = new ValueChangedEventWaiter(result))
            {
                target.MovePointer(30, 30);
                Wait.ForIdle();
                ChooseFromComboBox("ValidationActionsComboBox", "HoverLight_ValidatePointerOverState");
                waiter.Wait();
            }
            LogResult(result, "PointerOver");

            Log.Comment("Click the Target. Verify Pressed light state.");
            using (var waiter = new ValueChangedEventWaiter(result))
            {
                target.MovePointer(30, 30);
                Wait.ForIdle();

                if (target is ListViewItem)
                {
                    ListView testListView = new ListView(FindElement.ById("TestListView"));
                    testListView.Click();
                }
                else
                {
                    target.Click();
                }
                waiter.Wait();
            }
            LogResult(result, "Pressed");

            Log.Comment("Move Mouse to (0, 0). Validate target button is again in Normal state.");
            using (var waiter = new ValueChangedEventWaiter(result))
            {
                TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                Wait.ForIdle();
                ChooseFromComboBox("ValidationActionsComboBox", "HoverLight_ValidateNormalState");
                waiter.Wait();
            }
            LogResult(result, "Normal");
        }

        [TestMethod]
        public void RevealBorderLightTouchTests()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToRevealStates" }))
            {
                Log.Comment("Testing RevealBorderLight for ListViewItem... ");

                var target = FindElement.ById("AnotherListViewItem");

                var result = new Edit(FindElement.ById("TestResult"));

                // Regression coverage for Bug 11713432: [Neon][Reveal] Reveal borders remain after swiping in Groove and Movies & TV left-nav pane on mobile and desktop
                Log.Comment("Tap ListViewItem, then pan away from it. After panning, validate RevealBorderLight is off.");
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    // Move mouse pointer away so it does not create BorderLight on the item
                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();

                    InputHelper.Pan(target, 300, Direction.North);
                    Wait.ForIdle();

                    ChooseFromComboBox("ValidationActionsComboBox", "BorderLight_ValidateBorderLightPanAway");
                    Wait.ForIdle();
                    waiter.Wait();
                }
                LogResult(result, "BorderLight_PanAway");

                Log.Comment("Tap and hold ListViewItem. While holding, validate RevealBorderLight is on.");
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    // Move mouse pointer away so it does not create BorderLight on the item
                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();

                    // Validation will be triggered on ListViewItem's Holding event (this way we are still in the holding state while validating).
                    InputHelper.TapAndHold(target, 1000);
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "BorderLight_TapAndHold");

            }
        }

        [TestMethod]
        public void RevealHoverLightPositionTests()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToRevealStates" }))
            {
                var result = new Edit(FindElement.ById("TestResult"));

                Log.Comment("Tap and hold Button at an offset. Verfiy RevealHoverLight uses pointer expression to position itself at the pointer.");
                var target = FindElement.ById("NarrowButton");
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    // Move mouse pointer away so it does not create HoverLight on the item
                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();

                    InputHelper.TapAndHold(target, 10, 35, 1000);
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "HoverLightExpression_Button_Touch");

                Log.Comment("Activate Button with Keyboard. Validate RevealHoverLight uses keyboard expression to position itself at center of the control.");
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    // Move mouse pointer away so it does not create BorderLight on the item
                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();

                    // Tab to next button and activate it with keyboard
                    KeyboardHelper.PressKey(Key.Tab);
                    KeyboardHelper.PressKey(Key.Space);
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "HoverLightExpression_Button_Keyboard");

                Log.Comment("Tap and hold ListViewItem at an offset. Verfiy RevealHoverLight uses pointer expression to position itself at the pointer.");
                target = FindElement.ById("OneMoreListViewItem");
                ChooseFromComboBox("TargetComboBox", "OneMoreListViewItem");
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    // Move mouse pointer away so it does not create HoverLight on the item
                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();

                    InputHelper.TapAndHold(target, 150, 35, 1000);
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "HoverLightExpression_ListViewItem_Touch");
            }
        }

        [TestMethod]
        public void RevealButtonStates_Values()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2))
            {
                Log.Comment("RevealButtonStates_Values needs to be running on RS3+");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToRevealStates" }))
            {
                var result = new Edit(FindElement.ById("TestResult"));
                var target = FindElement.ById("LargeButton2");

                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Log.Comment("Tap and hold button (which will attach reveal backgorund lights).");
                    InputHelper.TapAndHold(target, 3000);
                    Wait.ForIdle();

                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();

                    ChooseFromComboBox("ValidationActionsComboBox", "HoverLight_ValidatePointerOverState_Values");
                    FindElement.ByName<Button>("StartLoggingValues").InvokeAndWait();

                    Log.Comment("Move mouse over the target. Wait for light to reach Default state values.");
                    target.MovePointer(30, 30);
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "PointerOver_Values");

                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    ChooseFromComboBox("ValidationActionsComboBox", "HoverLight_ValidatePressedState_Values");
                    FindElement.ByName<Button>("StartLoggingValues").InvokeAndWait();

                    Log.Comment("Click button. Wait for light to reach Pressed state values.");
                    target.Click();
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "Pressed_Values");
            }
        }

        [TestMethod]
        public void RevealButtonStates_FastRelease_Values()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2))
            {
                Log.Comment("RevealButtonStates_FastRelease_Values needs to be running on RS3+");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToRevealStates" }))
            {
                var result = new Edit(FindElement.ById("TestResult"));
                var target = FindElement.ById("LargeButton2");

                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Log.Comment("Tap and hold button (which will attach reveal backgorund lights).");
                    InputHelper.TapAndHold(target, 3000);
                    Wait.ForIdle();

                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();

                    ChooseFromComboBox("ValidationActionsComboBox", "HoverLight_ValidateFastRelease_Values");
                    FindElement.ByName<Button>("StartLoggingValues").InvokeAndWait();

                    Log.Comment("Click button. Wait for light to first reach Pressed, then FastRelease values.");
                    InputHelper.LeftClick(target);
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "FastRelease_Values");
            }
        }

        [TestMethod]
        public void RevealButtonStates_SlowRelease_Values()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2))
            {
                Log.Comment("RevealButtonStates_SlowRelease_Values needs to be running on RS3+");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToRevealStates" }))
            {
                var result = new Edit(FindElement.ById("TestResult"));
                var target = FindElement.ById("LargeButton2");

                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Log.Comment("Tap and hold button (which will attach RevealHoverLight).");
                    InputHelper.TapAndHold(target, 3000);
                    Wait.ForIdle();

                    ChooseFromComboBox("ValidationActionsComboBox", "HoverLight_ValidateSlowRelease_Values");
                    FindElement.ByName<Button>("StartLoggingValues").InvokeAndWait();

                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();

                    Log.Comment("Tap and hold button. Wait for light to first reach Pressed, then SlowRelease values.");
                    InputHelper.TapAndHold(target, 5000);
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "SlowRelease_Values");
            }
        }


        [TestMethod]
        public void RevealHoverLightPosition_Values()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Comment("RevealHoverLightPosition_Values needs to be running on RS2 or greater");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToRevealStates" }))
            {
                var result = new Edit(FindElement.ById("TestResult"));
                var target = FindElement.ById("LargeButton2");

                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    // Note for this test case we specifically attach the hover light via hover, not press.
                    // This provides regression coverage for Bug 14079741: Reveal hover light doesn't follow pointer until first interaction/click
                    Log.Comment("Move pointer over LargeButton2 (which will attach RevealHoverLight).");
                    target.MovePointer(50, 50);
                    Wait.ForIdle();

                    ChooseFromComboBox("ValidationActionsComboBox", "HoverLight_ValidatePosition_Offset1_Values");
                    FindElement.ByName<Button>("StartLoggingValues").InvokeAndWait();

                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();

                    Log.Comment("Move pointer to Offset1 [10, 90] on LargeButton2. Wait for light to position to reach expected values.");
                    target.MovePointer(10, 90);
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "Offset1_Values");

                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    ChooseFromComboBox("ValidationActionsComboBox", "HoverLight_ValidatePosition_Offset2_Values");
                    FindElement.ByName<Button>("StartLoggingValues").InvokeAndWait();

                    // Move mouse pointer away so it does not create BorderLight on the item
                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();

                    Log.Comment("Move pointer to Offset2 [50, 50]. Wait for light to position to reach expected values.");
                    target.MovePointer(50, 50);
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "Offset2_Values");

                target = FindElement.ById("NarrowButton2");
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Log.Comment("Tap and hold NarrowButton2 (which will attach RevealHoverLight).");
                    InputHelper.TapAndHold(target, 3000);
                    Wait.ForIdle();

                    ChooseFromComboBox("ValidationActionsComboBox", "HoverLight_ValidatePosition_Offset3_Values");
                    FindElement.ByName<Button>("StartLoggingValues").InvokeAndWait();

                    TestEnvironment.Application.CoreWindow.MovePointer(0, 0);
                    Wait.ForIdle();

                    Log.Comment("Tap on NarrowButton2 at [10, 40]. Wait for light position to reach expected values.");
                    InputHelper.Tap(target, 10, 40);
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "Offset3_Values");
            }
        }

        [TestMethod]
        public void RevealCreateThenEnableEffects()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Comment("RevealCreateThenEnableEffects needs to be running on RS2 or greater");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToRevealRegressionTests" }))
            {
                ToggleButton simulateDisabledByPolicyToggleButton = new ToggleButton(FindElement.ById("SimluateDisabledByPolicyToggleButton"));
                var addRevealButton = FindElement.ById("AddRevealButton");
                var removeRevealButton = FindElement.ById("RemoveRevealButton");
                var addBackRevealButton = FindElement.ById("AddBackRevealButton");

                var result = new Edit(FindElement.ById("TestResult"));

                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Log.Comment("Disable Composition Effects.");
                    simulateDisabledByPolicyToggleButton.Toggle();      // Default is false, so this will disable effect

                    Log.Comment("Click Add Reveal Button, to add button while effects disabled");
                    addRevealButton.Click();
                    Wait.ForIdle();

                    var revealButton = FindElement.ById("Test1_RevealButton");

                    Log.Comment("Tap and hold button (which would attach reveal backgorund lights).");
                    InputHelper.TapAndHold(revealButton, 1000);
                    Wait.ForIdle();

                    Log.Comment("Click Remove Reveal Button.");
                    removeRevealButton.Click();
                    Wait.ForIdle();

                    Log.Comment("Enable Composition Effects.");
                    simulateDisabledByPolicyToggleButton.Toggle();
                    Wait.ForIdle();

                    Log.Comment("Add Back Reveal Button, which should now have reveal. Make sure there is no crash. ");
                    addBackRevealButton.Click();
                    Wait.ForIdle();

                    Log.Comment("Tap and hold button. Make sure there is no crash and 2 hover lights are present.");
                    revealButton.Click();
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "RevealCreateThenEnableEffects");
            }
        }

        [TestMethod]
        public void CoreWindowEventsTests()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Comment("CoreWindowEventsTests needs to be running on RS2 or greater");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToCoreWindowEventsTests" }))
            {
                var addReveal = FindElement.ById("AddReveal");
                var result = new Edit(FindElement.ById("TestResult"));

                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Log.Comment("Click Add Reveal Button to enterin RevealBorderLights");
                    addReveal.Click();
                    Wait.ForIdle();

                    Log.Comment("Move Mouse to (-1,-1) to cause PointerExited event, the app's handler will remove Reveal. Ensure there is no crash.");
                    TestEnvironment.Application.CoreWindow.MovePointer(-1, -1);
                    Wait.ForIdle();

                    Log.Comment("Add back Reveal, make sure things are still good");
                    addReveal.Click();
                    Wait.ForIdle();

                    waiter.Wait();
                }
                LogResult(result, "CoreWindowEventsTests");
            }
        }

        [TestMethod]
        public void RevealBrushFromMarkup()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Comment("RevealBrushFromMarkup needs to be running on RS5 or greater (due to WUX bug 18005612");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Reveal Tests", "navigateToRevealMarkup" }))
            {
                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();
                    waiter.Wait();
                }

                LogResult(result, "RevealBrushFromMarkup");
            }
        }

        private void LogResult(Edit result, string stateName)
        {
            string expectedResult = stateName + ": Passed";
            bool testPassed = (0 == String.Compare(expectedResult, 0, result.Value, 0, expectedResult.Length));

            if (testPassed)
            {
                Log.Comment(result.Value);
            }
            else
            {
                Log.Error(result.Value);
            }
        }

        private void ChooseFromComboBox(string textBoxName, string text)
        {
            Log.Comment("Retrieve text box with name '{0}'.", textBoxName);
            ComboBox comboBox = new ComboBox(FindElement.ById(textBoxName));
            comboBox.SelectItemByName(text);
        }
    }
}
