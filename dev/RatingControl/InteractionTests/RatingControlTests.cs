// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

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
using Image = Microsoft.Windows.Apps.Test.Foundation.Controls.Image;
using Point = System.Drawing.Point;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class RatingControlTests
    {

        const int RATING_ITEM_WIDTH = 24; // RATING_ITEM_HEIGHT/2 default size + 8 item spacing
        const int RATING_ITEM_HEIGHT = 32;

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

        // TODO: split into multiple tests (mouse vs. keyboard)
        [TestMethod]
        public void BasicInteractionTest()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                Log.Comment("Retrieve rating control as generic UIElement");
                UIObject ratingUIObject = FindElement.ByName("TestRatingControl");
                Verify.IsNotNull(ratingUIObject, "Verifying that we found a UIElement called TestRatingControl");

                Log.Comment("Retrieve the text block as a TextBlock");
                TextBlock textBlock = new TextBlock(FindElement.ByName("TestTextBlockControl"));

                Log.Comment("Verify a tap on the third star sets Rating to 3");
                InputHelper.Tap(ratingUIObject, 60, RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("3", textBlock.DocumentText);

                Log.Comment("Verify a tap on the third star sets Rating to 2.5 (placeholder value)");
                InputHelper.Tap(ratingUIObject, 60, RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("!2.5", textBlock.DocumentText);

                Button phButton = new Button(FindElement.ByName("PHButton"));
                InputHelper.Tap(phButton);

                Log.Comment("Verify a tap on the first star sets Rating to 1");
                InputHelper.Tap(ratingUIObject, 12, RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("1", textBlock.DocumentText);

                Log.Comment("Verify a swipe off the left sets Rating to NOTHING");
                InputHelper.Pan(ratingUIObject, 200, Direction.West);
                Verify.AreEqual("!", textBlock.DocumentText);

                Log.Comment("Verify a right key on an unset Rating, sets rating to 1");
                KeyboardHelper.PressKey(ratingUIObject, Key.Right);

                Verify.AreEqual("1", textBlock.DocumentText);

                Log.Comment("Verify a left key on an RTL rating increases the rating.");
                Button rtlbutton = new Button(FindElement.ByName("RTLButton"));
                rtlbutton.Invoke();
                Wait.ForIdle();

                KeyboardHelper.PressKey(ratingUIObject, Key.Left);
                Verify.AreEqual("2", textBlock.DocumentText);

                Log.Comment("Verify home/end keys in RTL");
                KeyboardHelper.PressKey(ratingUIObject, Key.Home);
                Verify.AreEqual("!", textBlock.DocumentText);

                KeyboardHelper.PressKey(ratingUIObject, Key.End);
                Verify.AreEqual("5", textBlock.DocumentText);

                Log.Comment("Verify up down keys in RTL");
                KeyboardHelper.PressKey(ratingUIObject, Key.Down);
                Verify.AreEqual("4", textBlock.DocumentText);

                KeyboardHelper.PressKey(ratingUIObject, Key.Up);
                Verify.AreEqual("5", textBlock.DocumentText);

                rtlbutton.Invoke();
                Wait.ForIdle();

                Log.Comment("Verify home/end keys in LTR");
                KeyboardHelper.PressKey(ratingUIObject, Key.Home);
                Verify.AreEqual("!", textBlock.DocumentText);

                KeyboardHelper.PressKey(ratingUIObject, Key.End);
                Verify.AreEqual("5", textBlock.DocumentText);

                Log.Comment("Verify up down keys in LTR");
                KeyboardHelper.PressKey(ratingUIObject, Key.Down);
                Verify.AreEqual("4", textBlock.DocumentText);

                KeyboardHelper.PressKey(ratingUIObject, Key.Up);
                Verify.AreEqual("5", textBlock.DocumentText);
            }
        }

        [TestMethod]
        public void MaxRatingTest()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This clicks the button corresponding to the test page, and navs there
            {
                if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
                {
                    // For some reason this test sometimes fails on RS1 when the "bounding box" of the 
                    // RatingControl has negative values, but it's not worth the investigation to fix:
                    Log.Comment("Test is disabled on RS1 due to reliability issues");
                    return;
                }

                TextBlock tb = new TextBlock(FindElement.ById("FrameDetails"));
                Log.Comment("FrameDetails: " + tb.DocumentText);

                Log.Comment("Retrieve rating control as generic UIElement");
                UIObject ratingUIObject = FindElement.ByName("MaxRating9Unset");
                Verify.IsNotNull(ratingUIObject, "Verifying that we found a UIElement called MaxRating9Unset");
                ratingUIObject.SetFocus();
                AutomationElement maxRatingAutomationElement = AutomationElement.FocusedElement;

                Log.Comment("Retrieve the accompanying text block as a TextBlock");
                TextBlock textBlock = new TextBlock(FindElement.ByName("MaxRating9UnsetTextBlock"));
                InputHelper.ScrollToElement(textBlock);

                Verify.AreEqual(false, ratingUIObject.IsOffscreen, "Verify the rating control is onscreen");
                Verify.AreEqual(false, textBlock.IsOffscreen, "Verify the results textblock is onscreen");

                for (int i = 0; i < 9; i++)
                {
                    Log.Comment("Verify value " + (i + 1));
                    InputHelper.Tap(ratingUIObject, (i * RATING_ITEM_WIDTH) + (RATING_ITEM_WIDTH / 2), RATING_ITEM_HEIGHT / 2);

                    String expected = (i + 1) + "";
                    String actual = textBlock.DocumentText;

                    Verify.IsTrue(expected.Equals(actual), "Verifying that tapping on the " + (i + 1) + "th [sic] star works, actual: " + textBlock.DocumentText);
                    VerifyValue_ValueEqualsOnAutomationElement(maxRatingAutomationElement, "Rating, " + (i+1) + " of 9"); // make sure the UIA interface shows the right max value
                }
            }
        }

        [TestMethod]
        public void UnclearableIsUnclearableTest()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                Log.Comment("Retrieve MyRatingIsClearEnabled rating control as generic UIElement");
                UIObject ratingUIObject = FindElement.ByName("MyRatingIsClearEnabled");
                Verify.IsNotNull(ratingUIObject, "Verifying that we found a UIElement called MyRatingIsClearEnabled");

                Log.Comment("Retrieve the text block as TextBlock");
                TextBlock textBlock = new TextBlock(FindElement.ByName("MyRatingIsClearEnabledText"));
                InputHelper.ScrollToElement(textBlock);

                Log.Comment("Verify a right key on an unset Rating, sets rating to 1 [2]");
                KeyboardHelper.PressKey(ratingUIObject, Key.Right);
                Verify.AreEqual("1", textBlock.DocumentText);

                Log.Comment("Verify a left key on a RatingControl with Value=1 AND IsClearEnabled=false leaves rating at 1");
                KeyboardHelper.PressKey(ratingUIObject, Key.Left);
                Verify.AreEqual("1", textBlock.DocumentText);

                KeyboardHelper.PressKey(ratingUIObject, Key.Right);
                Verify.AreEqual("2", textBlock.DocumentText);

                Log.Comment("Verify the home key for a IsClearEnabled=false RatingControl sets rating to 1");
                KeyboardHelper.PressKey(ratingUIObject, Key.Home);
                Verify.AreEqual("1", textBlock.DocumentText);
            }
        }

        [TestMethod]
        public void CanClearEveryValueTest()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
                {
                    // For some reason this test sometimes fails on RS1 when the "bounding box" of the 
                    // RatingControl has negative values, but it's not worth the investigation to fix:
                    Log.Comment("Test is disabled on RS1 due to reliability issues");
                    return;
                }

                TextBlock tb = new TextBlock(FindElement.ById("FrameDetails"));
                Log.Comment("FrameDetails: " + tb.DocumentText);

                Log.Comment("Retrieve rating control as generic UIElement");
                UIObject ratingUIObject = FindElement.ByName("TestRatingControl");
                Verify.IsNotNull(ratingUIObject, "Verifying that we found a UIElement called TestRatingControl");

                Log.Comment("Retrieve the text block as a TextBlock");
                TextBlock textBlock = new TextBlock(FindElement.ByName("TestTextBlockControl"));

                Wait.ForIdle();

                Log.Comment("Verify a tap on the first star sets Rating to 1");
                InputHelper.Tap(ratingUIObject, (0 * RATING_ITEM_WIDTH) + (RATING_ITEM_WIDTH / 2), RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("1", textBlock.DocumentText);

                Log.Comment("Verify a tap on the first star sets Rating to 2.5 (placeholder value)");
                InputHelper.Tap(ratingUIObject, (0 * RATING_ITEM_WIDTH) + (RATING_ITEM_WIDTH / 2), RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("!2.5", textBlock.DocumentText);

                Log.Comment("Verify a tap on the second star sets Rating to 2");
                InputHelper.Tap(ratingUIObject, (1 * RATING_ITEM_WIDTH) + (RATING_ITEM_WIDTH / 2), RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("2", textBlock.DocumentText);

                Log.Comment("Verify a tap on the second star sets Rating to 2.5 (placeholder value)");
                InputHelper.Tap(ratingUIObject, (1 * RATING_ITEM_WIDTH) + (RATING_ITEM_WIDTH / 2), RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("!2.5", textBlock.DocumentText);

                Log.Comment("Verify a tap on the third star sets Rating to 3");
                InputHelper.Tap(ratingUIObject, (2 * RATING_ITEM_WIDTH) + (RATING_ITEM_WIDTH / 2), RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("3", textBlock.DocumentText);

                Log.Comment("Verify a tap on the third star sets Rating to 2.5 (placeholder value)");
                InputHelper.Tap(ratingUIObject, (2 * RATING_ITEM_WIDTH) + (RATING_ITEM_WIDTH / 2), RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("!2.5", textBlock.DocumentText);

                Log.Comment("Verify a tap on the fourth star sets Rating to 4");
                InputHelper.Tap(ratingUIObject, (3 * RATING_ITEM_WIDTH) + (RATING_ITEM_WIDTH / 2), RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("4", textBlock.DocumentText);

                Log.Comment("Verify a tap on the fourth star sets Rating to 2.5 (placeholder value)");
                InputHelper.Tap(ratingUIObject, (3 * RATING_ITEM_WIDTH) + (RATING_ITEM_WIDTH / 2), RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("!2.5", textBlock.DocumentText);

                Log.Comment("Verify a tap on the fifth star sets Rating to 5");
                InputHelper.Tap(ratingUIObject, (4 * RATING_ITEM_WIDTH) + (RATING_ITEM_WIDTH / 2), RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("5", textBlock.DocumentText);

                Log.Comment("Verify a tap on the second star sets Rating to 2.5 (placeholder value)");
                InputHelper.Tap(ratingUIObject, (4 * RATING_ITEM_WIDTH) + (RATING_ITEM_WIDTH / 2), RATING_ITEM_HEIGHT / 2);
                Verify.AreEqual("!2.5", textBlock.DocumentText);
            }
        }

        [TestMethod]
        public void GamepadTest()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2))
                {
                    Log.Warning("Test is disabled on RS2 or older because Rating's engagement model relies on OnPreviewKey* virtuals");
                    return;
                }

                UIObject ratingUIObject = FindElement.ByName("TestRatingControl");
                Verify.IsNotNull(ratingUIObject, "Verifying that we found a UIElement called TestRatingControl");
                TextBlock textBlock = new TextBlock(FindElement.ByName("TestTextBlockControl"));

                Log.Comment("Verify gamepad engagement");
                ratingUIObject.SetFocus();
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                Wait.ForIdle();
                Verify.AreEqual("1", textBlock.DocumentText);

                Log.Comment("Verify gamepad one change and cancel");
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadRight);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.B);
                Wait.ForIdle();
                Verify.AreEqual("1", textBlock.DocumentText);

                Log.Comment("Verify gamepad one change and accept");
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadRight);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                Wait.ForIdle();
                Verify.AreEqual("2", textBlock.DocumentText);

                Log.Comment("Verify gamepad multiple changes");
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadLeft);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadRight);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadRight);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                Wait.ForIdle();
                Verify.AreEqual("3", textBlock.DocumentText);

                Log.Comment("Verify gamepad left stick and dpad work");
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadLeft);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.LeftThumbstickRight);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.LeftThumbstickRight);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                Wait.ForIdle();
                Verify.AreEqual("4", textBlock.DocumentText);

                Log.Comment("Verify gamepad dpad up down do nothing");
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadUp);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                Wait.ForIdle();
                Verify.AreEqual("4", textBlock.DocumentText);

                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadDown);
                GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                Wait.ForIdle();
                Verify.AreEqual("4", textBlock.DocumentText);
            }
        }

        [TestMethod]
        public void VerifyThatProgrammaticallyRemovingEngagementResetsValue()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests"))  // This literally clicks the button corresponding to the test page.
            {               
                if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2))
                {
                    Log.Warning("Test is disabled on RS2 or older because Rating's engagement model relies on OnPreviewKey* virtuals");
                    return;
                }

                TextBlock magicDisengagerTextBlock = new TextBlock(FindElement.ById("MagicDisengagerTextBlock"));
                magicDisengagerTextBlock.SetFocus();
                InputHelper.ScrollToElement(magicDisengagerTextBlock);

                UIObject magicDisengager = FindElement.ById("MagicDisengager");
                Verify.IsNotNull(magicDisengager, "Verifying that we found a UIElement called MagicDisengager");
                magicDisengager.SetFocus();

                Wait.ForIdle();

                GamepadHelper.PressButton(magicDisengager, GamepadButton.A);
                GamepadHelper.PressButton(magicDisengager, GamepadButton.DPadRight);
                Verify.AreEqual("2", magicDisengagerTextBlock.DocumentText);

                // This control is rigged to programmatically disengage at 3
                GamepadHelper.PressButton(magicDisengager, GamepadButton.DPadRight);
                Verify.AreEqual("null", magicDisengagerTextBlock.DocumentText);
            }
        }

        [TestMethod]
        [TestProperty("MinVersion", "RS1")]
        public void UIAValuePatternTest()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                Log.Comment("Retrieve rating control as generic UIElement");
                UIObject ratingUIObject = FindElement.ByName("TestRatingControl");

                TextBlock textBlock = new TextBlock(FindElement.ByName("TestTextBlockControl"));
                Verify.IsNotNull(ratingUIObject, "Verifying that we found a UIElement called TestRatingControl");

                Log.Comment("Verify the UIA Value before user clicks the control.");
                ratingUIObject.SetFocus(); // Setting focus just so we can use AE.FE below
                Wait.ForIdle();
                AutomationElement ratingPeer = AutomationElement.FocusedElement;

                VerifyValue_ValueEqualsOnAutomationElement(ratingPeer, "Community Rating, 2.5 of 5");

                if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2)) // engagement doesn't work pre RS3
                {
                    Log.Comment("Verify moving right 2 times with the gamepad sets the control to 3");
                    GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                    GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadRight);
                    GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadRight);
                    GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                    Wait.ForIdle();
                    Verify.AreEqual("3", textBlock.DocumentText);

                    Log.Comment("Verify rating of 3 sets UIA text to 3");

                    VerifyValue_ValueEqualsOnAutomationElement(ratingPeer, "Rating, 3 of 5");

                    // revert:
                    Log.Comment("Resetting control to community rating");
                    GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);
                    GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadLeft);
                    GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadLeft);
                    GamepadHelper.PressButton(ratingUIObject, GamepadButton.DPadLeft);
                    GamepadHelper.PressButton(ratingUIObject, GamepadButton.A);

                    VerifyValue_ValueEqualsOnAutomationElement(ratingPeer, "Community Rating, 2.5 of 5");
                }

                Log.Comment("Verify more complex navigation");
                KeyboardHelper.PressKey(ratingUIObject, Key.Right);
                KeyboardHelper.PressKey(ratingUIObject, Key.Right);
                KeyboardHelper.PressKey(ratingUIObject, Key.Right);
                KeyboardHelper.PressKey(ratingUIObject, Key.Left);
                VerifyValue_ValueEqualsOnAutomationElement(ratingPeer, "Rating, 2 of 5");

                // Verify read an unset rating
                ratingUIObject = FindElement.ByName("RatingBindingSample");
                Log.Comment("Verify the UIA Value of an unset Rating without a placeholder value");
                ratingUIObject.SetFocus();
                InputHelper.ScrollToElement(ratingUIObject);
                ratingPeer = AutomationElement.FocusedElement;

                VerifyValue_ValueEqualsOnAutomationElement(ratingPeer, "Rating Unset");

                // Verify rounding:
                Log.Comment("Verifying Value_Value rounding");                
                UIObject round1 = FindElement.ById("ValuePatternRoundTest1");
                round1.SetFocus();
                AutomationElement roundAE1 = AutomationElement.FocusedElement;
                UIObject round2 = FindElement.ById("ValuePatternRoundTest2");
                round2.SetFocus();
                AutomationElement roundAE2 = AutomationElement.FocusedElement;
                UIObject round3 = FindElement.ById("ValuePatternRoundTest3");
                round3.SetFocus();
                AutomationElement roundAE3 = AutomationElement.FocusedElement;

                VerifyValue_ValueEqualsOnAutomationElement(roundAE1, "Rating, 1.5 of 5");
                VerifyValue_ValueEqualsOnAutomationElement(roundAE2, "Rating, 1.55 of 5");
                VerifyValue_ValueEqualsOnAutomationElement(roundAE3, "Rating, 1.5 of 5");
            }
        }

        // Setting the value on a collapsed control can cause it to try and
        // interact with a non-existent AutomationPeer, causing a crash.
        [TestMethod]
        public void VerifyDontCrashWhenCollapsedAndValueSet()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                Button button = new Button(FindElement.ByName("CollapsedRatingControlButton"));
                button.Invoke();
                Wait.ForIdle();
            }
        }

        // Verify that tabbing around puts us where we expect
        [TestMethod]
        public void VerifyTabKeyWorks()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                UIObject ratingUIObject = FindElement.ByName("TestRatingControl");
                Verify.IsNotNull(ratingUIObject, "Verifying that we found a UIElement called TestRatingControl");
                Button button = new Button(FindElement.ByName("PHButton"));
                Verify.IsNotNull(ratingUIObject, "Verifying that we found a Button called PHButton");
                UIObject ratingReadOnly = FindElement.ByName("MyRatingReadOnly");
                Verify.IsNotNull(ratingUIObject, "Verifying that we found a UIElement called MyRatingReadOnly");
                UIObject ratingIsClearEnabled = FindElement.ByName("MyRatingReadOnlyWithValue");
                Verify.IsNotNull(ratingUIObject, "Verifying that we found a UIElement called MyRatingReadOnlyWithValue");
                InputHelper.ScrollToElement(ratingIsClearEnabled);

                Log.Comment("Verify we aren't breaking tab behaviour");
                ratingUIObject.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(ratingUIObject.HasKeyboardFocus, "Focus is on first RatingControl");
                KeyboardHelper.PressKey(Key.Tab);
                Verify.IsTrue(button.HasKeyboardFocus, "Focus is on the button");
                KeyboardHelper.PressKey(Key.Tab);
                Verify.IsTrue(ratingReadOnly.HasKeyboardFocus, "Focus is on the ReadOnly RatingControl");
                KeyboardHelper.PressKey(Key.Tab);
                Verify.IsTrue(ratingIsClearEnabled.HasKeyboardFocus, "Focus is on the ReadOnlyWithValue RatingControl");

                Log.Comment("Verify left/right/up/down keeps us within the control (w/ keyboard)");
                KeyboardHelper.PressKey(Key.Up);
                Verify.IsTrue(ratingIsClearEnabled.HasKeyboardFocus);
                KeyboardHelper.PressKey(Key.Down);
                Verify.IsTrue(ratingIsClearEnabled.HasKeyboardFocus);
                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(ratingIsClearEnabled.HasKeyboardFocus);
                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(ratingIsClearEnabled.HasKeyboardFocus);
            }
        }

        [TestMethod]
        public void VerifyDependencyPropertyBinding()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                UIObject ratingBinding = FindElement.ByName("RatingBindingSample");
                Button ratingBindingButton = new Button(FindElement.ByName("RatingBindingSampleButton"));
                InputHelper.ScrollToElement(ratingBindingButton);
                TextBlock ratingBindingText = new TextBlock(FindElement.ById("RatingBindingSampleText"));
                ratingBindingButton.Invoke();
                Wait.ForIdle();
                Verify.AreEqual("Binding successful", ratingBindingText.DocumentText);
                
                UIObject ratingXBind = FindElement.ByName("RatingXBindSample");
                Button ratingXBindButton = new Button(FindElement.ByName("RatingXBindSampleButton"));
                InputHelper.ScrollToElement(ratingXBindButton);
                TextBlock ratingXBindText = new TextBlock(FindElement.ByName("RatingXBindSampleText"));
                ratingXBindButton.Invoke();
                Wait.ForIdle();
                Verify.AreEqual("Binding successful", ratingXBindText.DocumentText);
            }
        }

        [TestMethod]
        public void VerifyUIAProperties()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                UIObject ratingReadOnly = FindElement.ByName("MyRatingReadOnly");
                ratingReadOnly.SetFocus();
                InputHelper.ScrollToElement(ratingReadOnly);
                AutomationElement ratingPeer = AutomationElement.FocusedElement;

                object valuePatternAsObject;
                ratingPeer.TryGetCurrentPattern(ValuePattern.Pattern, out valuePatternAsObject);
                Verify.IsNotNull(valuePatternAsObject); // ValuePattern should exist
                ValuePattern valuePattern = valuePatternAsObject as ValuePattern;

                object valueProperty = ratingPeer.GetCurrentPropertyValue(ValuePattern.IsReadOnlyProperty);
                bool isReadOnly = (bool)valueProperty;
                Verify.IsTrue(isReadOnly);

                // Verify the negative
                UIObject ratingReadable = FindElement.ByName("TestRatingControl");
                ratingReadable.SetFocus();
                Wait.ForIdle();
                ratingPeer = AutomationElement.FocusedElement;
                valueProperty = ratingPeer.GetCurrentPropertyValue(ValuePattern.IsReadOnlyProperty);
                isReadOnly = (bool)valueProperty;
                Verify.IsFalse(isReadOnly);

                ratingPeer.TryGetCurrentPattern(RangeValuePattern.Pattern, out valuePatternAsObject);
                Verify.IsNotNull(valuePatternAsObject); // RangeValuePattern should exist

                ratingPeer.TryGetCurrentPattern(ExpandCollapsePattern.Pattern, out valuePatternAsObject);
                Verify.IsNull(valuePatternAsObject); // ExpandCollapsePattern shouldn't exist

                String lctp = ratingPeer.GetCurrentPropertyValue(AutomationElementIdentifiers.LocalizedControlTypeProperty) as String;
                Verify.AreEqual("Rating Slider", lctp);

                object maximumProperty = ratingPeer.GetCurrentPropertyValue(RangeValuePattern.MaximumProperty);
                double maximumPropertyAsNumber = (double)maximumProperty;
                Verify.AreEqual(5, maximumPropertyAsNumber, "Verify the RangeValue_Max UIA property");
            }
        }

        [TestMethod]
        public void VerifyReadOnlyIsntInteractive()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {

                UIObject ratingReadOnly = FindElement.ByName("MyRatingReadOnlyWithValue");
                TextBlock tb = new TextBlock(FindElement.ByName("MyRatingReadOnlyTextBlock"));
                InputHelper.ScrollToElement(tb);

                Verify.AreEqual(tb.DocumentText, "2.2");

                ratingReadOnly.SetFocus();

                KeyboardHelper.PressKey(ratingReadOnly, Key.Right);
                KeyboardHelper.PressKey(ratingReadOnly, Key.Right);
                KeyboardHelper.PressKey(ratingReadOnly, Key.Right);
                KeyboardHelper.PressKey(ratingReadOnly, Key.Right);
                KeyboardHelper.PressKey(ratingReadOnly, Key.Right);
                KeyboardHelper.PressKey(ratingReadOnly, Key.Right);

                Verify.AreEqual(tb.DocumentText, "2.2");
            }
        }

        [TestMethod]
        public void DontCrashWhenRecyclingTest()
        {
            using (var temp = new TestSetupHelper("RatingControl Tests"))
            {
            }
            Wait.ForIdle();
            using (var setup = new TestSetupHelper("RatingControl Tests"))
            {
            }
        }

        [TestMethod]
        public void ImageInfoValidation()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                UIObject customImagesTwo = FindElement.ById("CustomImages");
                InputHelper.ScrollToElement(customImagesTwo);

                WaitForCheckbox("CustomImagesLoadedCheckBox");

                // Verify it has an <Image> child

                // Future: This RawContext doesn't seem to work to make us search the Raw UIA tree, 
                // in future, if we can figure out a way to do that, we can stop
                // the "move the image into the content tree" in the test app hack
                // Context.RawContext.Activate();

                Log.Comment("Trying to find the named Image: CustomImages_FirstImageItem");
                UIObject image = FindElement.ById("CustomImages_FirstImageItem");
                Image imageAsImage = image as Image;

                // Can't really verify anything else 
            }
        }

        // Need a test for when we CHANGE between font<->image rating items
        [TestMethod]
        public void ValidateSwitchingItemInfo()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                UIObject customImagesTwo = FindElement.ById("StageThreeCheckbox");
                InputHelper.ScrollToElement(customImagesTwo);

                WaitForCheckbox("StageOneCheckbox");

                Log.Comment("Trying to find the named Image: CustomImagesTwo_FirstImageItem");
                UIObject image = FindElement.ById("CustomImagesTwo_FirstImageItem");
                Image imageAsImage = image as Image;

                Button changeTypeButton = new Button(FindElement.ById("ChangeCustomImagesTwoType"));
                changeTypeButton.Invoke();
                Wait.ForIdle();

                WaitForCheckbox("StageTwoCheckbox");

                Log.Comment("Trying to find the named Image: CustomImagesTwo_FirstTextItem");
                UIObject textBlock = FindElement.ById("CustomImagesTwo_FirstTextItem");
                TextBlock textBlockAsTextBlock = textBlock as TextBlock;

                changeTypeButton.Invoke();
                Wait.ForIdle();

                WaitForCheckbox("StageThreeCheckbox");

                Log.Comment("Trying to find the named Image: CustomImagesTwo_FirstImageItem_Again");
                UIObject image2 = FindElement.ById("CustomImagesTwo_FirstImageItem_Again");
                Image imageAsImage2 = image2 as Image;
            }
        }

        // These two tests that try and test live mid-input visual changes are unreliable.
        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125
        public void EnsureScaleMaintainedOnTap()
        {
            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                Log.Comment("Retrieve rating control as generic UIElement");
                UIObject ratingUIObject = FindElement.ByName("TestRatingControl");
                Verify.IsNotNull(ratingUIObject, "Verifying that we found a UIElement called TestRatingControl");

                Log.Comment("Retrieve the three text blocks");
                TextBlock textBlock = new TextBlock(FindElement.ByName("TestTextBlockControl"));
                TextBlock textBlockX = new TextBlock(FindElement.ById("ScaleTextXBlock"));
                TextBlock textBlockY = new TextBlock(FindElement.ById("ScaleTextYBlock"));

                VerifyValueIsApproximately(0.5f, float.Parse(textBlockX.DocumentText));
                VerifyValueIsApproximately(0.5f, float.Parse(textBlockY.DocumentText));

                Log.Comment("Click to set the rating to 2");
                Point secondStar = new Point((1 * RATING_ITEM_WIDTH) + (RATING_ITEM_WIDTH / 2), RATING_ITEM_HEIGHT / 2);
                InputHelper.Pan(ratingUIObject, secondStar, 1, Direction.East); // Do it twice, using different methods to ensure test reliability
                InputHelper.LeftClick(ratingUIObject, secondStar.X, secondStar.Y); // (Across server/local runs)
                Verify.AreEqual("2", textBlock.DocumentText);
                Wait.ForMilliseconds(200); // composition property spy takes time!

                VerifyValueIsApproximately(0.8f, float.Parse(textBlockX.DocumentText));
                VerifyValueIsApproximately(0.8f, float.Parse(textBlockY.DocumentText));
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125
        public void VerifyRatingItemFallback()
        {
            // This test is actually performed in the test app itself, so go look at RatingControlPage.xaml.cs for the meat of it.

            using (var setup = new TestSetupHelper("RatingControl Tests")) // This literally clicks the button corresponding to the test page.
            {
                if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone1))
                {
                    Log.Warning("Test is disabled on RS1 due to scrolling unreliability");
                    return;
                }

                Log.Comment("Retrieve PointerOverPlaceholderFallbackRating rating control as generic UIElement");
                UIObject popRating = FindElement.ById("PointerOverPlaceholderFallbackRating");
                Verify.IsNotNull(popRating, "Verifying that we found a UIElement called PointerOverPlaceholderFallbackRating");

                InputHelper.ScrollToElement(popRating);

                popRating.Tap(); // Since on phone it's touch, I do a tap to trigger that.
                popRating.Click();
                Wait.ForIdle();

                Log.Comment("Retrieve PointerOverFallbackRating rating control as generic UIElement");
                UIObject pointerOver = FindElement.ById("PointerOverFallbackRating");
                Verify.IsNotNull(pointerOver, "Verifying that we found a UIElement called PointerOverFallbackRating");
                InputHelper.ScrollToElement(pointerOver);
                pointerOver.Click();
                pointerOver.Tap();
                Wait.ForIdle();

                TextBlock textBlock = new TextBlock(FindElement.ById("UnsetFallbackTextBlock"));
                Verify.AreEqual("+", textBlock.DocumentText, "Verify Unset glyph falls back onto Glyph");
                TextBlock textBlock2 = new TextBlock(FindElement.ById("PlaceholderFallbackTextBlock"));
                Verify.AreEqual("+", textBlock2.DocumentText, "Verify Placeholder glyph falls back onto Glyph");
                TextBlock textBlock3 = new TextBlock(FindElement.ById("DisabledFallbackTextBlock"));
                Verify.AreEqual("+", textBlock3.DocumentText, "Verify Disabled glyph falls back onto Glyph");

                ElementCache.Clear();

                TextBlock textBlock4 = new TextBlock(FindElement.ById("PointerOverPlaceholderFallbackTextBlock"));
                Verify.AreEqual("+", textBlock4.DocumentText, "Verify PointerOverPlaceholder glyph falls back onto Placeholder");
                TextBlock textBlock5 = new TextBlock(FindElement.ById("PointerOverFallbackTextBlock"));
                Verify.AreEqual("+", textBlock5.DocumentText, "Verify PointerOver glyph falls back onto Glyph");
                TextBlock textBlock6 = new TextBlock(FindElement.ById("NoFallbackTextBlock"));
                Verify.AreEqual("+", textBlock6.DocumentText, "Verify a glyph didn't fall back if it wasn't meant to");

                // Image:
                Log.Comment("Retrieve PointerOverPlaceholderImageFallbackRating rating control as generic UIElement");
                popRating = FindElement.ById("PointerOverPlaceholderImageFallbackRating");
                Verify.IsNotNull(popRating, "Verifying that we found a UIElement called PointerOverPlaceholderImageFallbackRating");

                InputHelper.ScrollToElement(popRating);
                Wait.ForIdle();

                popRating.Tap(); // Since on phone it's touch, I do a tap to trigger that.
                popRating.Click();
                Wait.ForIdle();

                Log.Comment("Retrieve PointerOverImageFallbackRating rating control as generic UIElement");
                pointerOver = FindElement.ById("PointerOverImageFallbackRating");
                Verify.IsNotNull(pointerOver, "Verifying that we found a UIElement called PointerOverImageFallbackRating");
                pointerOver.Tap();
                pointerOver.Click();
                Wait.ForIdle();

                textBlock = new TextBlock(FindElement.ById("UnsetImageFallbackTextBlock"));
                Verify.AreEqual("+", textBlock.DocumentText, "Verify Unset image falls back onto Image");
                textBlock2 = new TextBlock(FindElement.ById("PlaceholderImageFallbackTextBlock"));
                Verify.AreEqual("+", textBlock2.DocumentText, "Verify Placeholder image falls back onto Image");
                textBlock3 = new TextBlock(FindElement.ById("DisabledImageFallbackTextBlock"));
                Verify.AreEqual("+", textBlock3.DocumentText, "Verify Disabled image falls back onto Image");

                ElementCache.Clear();
                textBlock4 = new TextBlock(FindElement.ById("PointerOverPlaceholderImageFallbackTextBlock"));
                Verify.AreEqual("+", textBlock4.DocumentText, "Verify PointerOverPlaceholder image falls back onto PlaceholderImage");
                textBlock5 = new TextBlock(FindElement.ById("PointerOverImageFallbackTextBlock"));
                Verify.AreEqual("+", textBlock5.DocumentText, "Verify PointerOver image falls back onto Image");
            }
        }

        [TestMethod]
        public void VerifyCanLoadWUXCRatingControl()
        {
            // Regression coverage for:
            // Bug 19337410: WUX RatingControl crashes when WinUI is included 

            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                // WUXC RatingControl did not exist prior to RS3.
                return;
            }
    
            using (var setup = new TestSetupHelper("RatingControl Tests")) 
            {
                Log.Comment("Retrieve rating control as generic UIElement");
                UIObject wuxcRatingControl = FindElement.ById("wuxcRatingControl");
                Verify.IsNotNull(wuxcRatingControl);
            }
        }

        // Test Helpers:
        void WaitForCheckbox(string checkboxName)
        {
            Log.Comment("Waiting until " + checkboxName + " to be checked by test app.");
            CheckBox cb = new CheckBox(FindElement.ById(checkboxName));

            if (cb.ToggleState != ToggleState.On)
            {
                cb.GetToggledWaiter().Wait();
            }

            Wait.ForIdle();

            Log.Comment(checkboxName + " checkbox checked.");
        }

        void VerifyValue_ValueEqualsOnAutomationElement(AutomationElement ae, String expected, String reason = "")
        {
            object valuePatternAsObject;
            ae.TryGetCurrentPattern(ValuePattern.Pattern, out valuePatternAsObject);
            if (valuePatternAsObject != null)
            {
                ValuePattern vp = valuePatternAsObject as ValuePattern;
                object valueProperty = ae.GetCurrentPropertyValue(ValuePattern.ValueProperty);
                String valuePropertyString = valueProperty as String;
                Verify.AreEqual(expected, valuePropertyString, reason.Length > 0 ? reason : "Expected value of " + expected);
            }
        }

        const float APPROXIMATE_MARGIN = 0.025f;
        void VerifyValueIsApproximately(float expected, float actual)
        {
            Verify.IsTrue(actual >= expected - APPROXIMATE_MARGIN && actual <= expected + APPROXIMATE_MARGIN, "Verify " + actual + " is within -/+0.025 of " + expected);
        }

    }
}
