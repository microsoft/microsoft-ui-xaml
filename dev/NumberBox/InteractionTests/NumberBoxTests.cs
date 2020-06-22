// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.Collections.Generic;

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

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class NumberBoxTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void UpDownTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                RangeValueSpinner numBox = FindElement.ByName<RangeValueSpinner>("TestNumberBox");
                numBox.SetValue(0);

                ComboBox spinModeComboBox = FindElement.ByName<ComboBox>("SpinModeComboBox");
                spinModeComboBox.SelectItemByName("Inline");
                Wait.ForIdle();

                Button upButton = FindButton(numBox, "Increase");
                Button downButton = FindButton(numBox, "Decrease");

                Log.Comment("Verify that up button increases value by 1");
                upButton.InvokeAndWait();
                Verify.AreEqual(1, numBox.Value);

                Log.Comment("Verify that down button decreases value by 1");
                downButton.InvokeAndWait();
                Verify.AreEqual(0, numBox.Value);

                Log.Comment("Change SmallChange value to 5");
                RangeValueSpinner smallChangeNumBox = FindElement.ByName<RangeValueSpinner>("SmallChangeNumberBox");
                smallChangeNumBox.SetValue(5);
                Wait.ForIdle();

                Log.Comment("Verify that up button increases value by 5");
                upButton.InvokeAndWait();
                Verify.AreEqual(5, numBox.Value);

                Check("MinCheckBox");
                Check("MaxCheckBox");

                numBox.SetValue(100);
                Check("WrapCheckBox");

                Log.Comment("Verify that when wrapping is on, and value is at max, clicking the up button wraps to the min value.");
                upButton.InvokeAndWait();
                Verify.AreEqual(0, numBox.Value);

                Log.Comment("Verify that when wrapping is on, clicking the down button wraps to the max value.");
                downButton.InvokeAndWait();
                Verify.AreEqual(100, numBox.Value);

                Log.Comment("Verify that incrementing after typing in a value validates the text first.");
                EnterText(numBox, "50", false);
                upButton.InvokeAndWait();
                Verify.AreEqual(55, numBox.Value);
            }
        }


        [TestMethod]
        public void UpDownEnabledTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                RangeValueSpinner numBox = FindElement.ByName<RangeValueSpinner>("TestNumberBox");

                ComboBox spinModeComboBox = FindElement.ByName<ComboBox>("SpinModeComboBox");
                spinModeComboBox.SelectItemByName("Inline");
                Wait.ForIdle();

                Button upButton = FindButton(numBox, "Increase");
                Button downButton = FindButton(numBox, "Decrease");

                Check("MinCheckBox");
                Check("MaxCheckBox");

                Log.Comment("Verify that spin buttons are disabled if value is NaN.");
                Wait.ForIdle();
                Verify.IsFalse(upButton.IsEnabled);
                Verify.IsFalse(downButton.IsEnabled);

                Log.Comment("Verify that when Value is at Minimum, the down spin button is disabled.");
                numBox.SetValue(0);
                Wait.ForIdle();
                Verify.IsTrue(upButton.IsEnabled);
                Verify.IsFalse(downButton.IsEnabled);

                Log.Comment("Verify that when Value is at Maximum, the up spin button is disabled.");
                numBox.SetValue(100);
                Wait.ForIdle();
                Verify.IsFalse(upButton.IsEnabled);
                Verify.IsTrue(downButton.IsEnabled);

                Log.Comment("Verify that when wrapping is enabled, spin buttons are enabled.");
                Check("WrapCheckBox");
                Verify.IsTrue(upButton.IsEnabled);
                Verify.IsTrue(downButton.IsEnabled);
                Uncheck("WrapCheckBox");

                Log.Comment("Verify that when Maximum is updated the up button is updated also.");
                RangeValueSpinner maxBox = FindElement.ByName<RangeValueSpinner>("MaxNumberBox");
                maxBox.SetValue(200);
                Wait.ForIdle();
                Verify.IsTrue(upButton.IsEnabled);
                Verify.IsTrue(downButton.IsEnabled);

                ComboBox validationComboBox = FindElement.ByName<ComboBox>("ValidationComboBox");
                validationComboBox.SelectItemByName("Disabled");
                Wait.ForIdle();

                Log.Comment("Verify that when validation is off, spin buttons are enabled");
                numBox.SetValue(0);
                Wait.ForIdle();
                Verify.IsTrue(upButton.IsEnabled);
                Verify.IsTrue(downButton.IsEnabled);

                Log.Comment("...except in the NaN case");
                numBox.SetValue(double.NaN);
                Wait.ForIdle();
                Verify.IsFalse(upButton.IsEnabled);
                Verify.IsFalse(downButton.IsEnabled);
            }
        }

        [TestMethod]
        public void ValueTextTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                RangeValueSpinner numBox = FindElement.ByName<RangeValueSpinner>("TestNumberBox");
                numBox.SetValue(0);

                Log.Comment("Verify that focusing the NumberBox selects the text");
                numBox.SetFocus();
                Wait.ForIdle();
                Wait.ForSeconds(3);
                Edit edit = FindTextBox(numBox);
                Verify.AreEqual("0", edit.GetTextSelection());

                Log.Comment("Verify that setting the value through UIA changes the textbox text");
                numBox.SetValue(10);
                Wait.ForIdle();
                Verify.AreEqual("10", edit.GetText());

                Log.Comment("Verify that setting the text programmatically changes the value and textbox text");
                Button button = FindElement.ByName<Button>("SetTextButton");
                button.InvokeAndWait();
                Verify.AreEqual(15, numBox.Value);
                Verify.AreEqual("15", edit.GetText());

                Log.Comment("Verify that even if the value doesn't change, the textbox text is updated");
                EnterText(numBox, " 15 ");
                Verify.AreEqual("15", edit.GetText());
            }
        }

        [TestMethod]
        public void MinMaxTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                Check("MinCheckBox");
                Check("MaxCheckBox");

                RangeValueSpinner numBox = FindElement.ByName<RangeValueSpinner>("TestNumberBox");
                Verify.AreEqual(0, numBox.Minimum);
                Verify.AreEqual(100, numBox.Maximum);

                numBox.SetValue(10);
                Wait.ForIdle();

                Log.Comment("Verify that setting the value to -1 changes the value to 0");
                numBox.SetValue(-1);
                Wait.ForIdle();
                Verify.AreEqual(0, numBox.Value);

                Log.Comment("Verify that typing '123' in the NumberBox changes the value to 100");
                EnterText(numBox, "123");
                Verify.AreEqual(100, numBox.Value);

                Log.Comment("Changing Max to 90; verify value also changes to 90");
                RangeValueSpinner maxBox = FindElement.ByName<RangeValueSpinner>("MaxNumberBox");
                EnterText(maxBox, "90");
                Verify.AreEqual(90, numBox.Value);

                Log.Comment("Verify that setting the minimum above the maximum changes the maximum");
                RangeValueSpinner minBox = FindElement.ByName<RangeValueSpinner>("MinNumberBox");
                EnterText(minBox, "200");
                Verify.AreEqual(200, numBox.Minimum);
                Verify.AreEqual(200, numBox.Maximum);
                Verify.AreEqual(200, numBox.Value);

                Log.Comment("Verify that setting the maximum below the minimum changes the minimum");
                EnterText(maxBox, "150");
                Verify.AreEqual(150, numBox.Minimum);
                Verify.AreEqual(150, numBox.Maximum);
                Verify.AreEqual(150, numBox.Value);
            }
        }

        [TestMethod]
        public void BasicKeyboardTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                RangeValueSpinner numBox = FindElement.ByName<RangeValueSpinner>("TestNumberBox");

                Log.Comment("Verify that loss of focus validates textbox contents");
                EnterText(numBox, "8", false);
                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();
                Verify.AreEqual(8, numBox.Value);

                Log.Comment("Verify that whitespace around a number still evaluates to the number");
                EnterText(numBox, "  75 ", true);
                Wait.ForIdle();
                Verify.AreEqual(75, numBox.Value);

                Log.Comment("Verify that pressing escape cancels entered text");
                EnterText(numBox, "3", false);
                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();
                Verify.AreEqual(75, numBox.Value);

                Log.Comment("Verify that pressing up arrow increases the value");
                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                Verify.AreEqual(76, numBox.Value);

                Log.Comment("Verify that pressing down arrow decreases the value");
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                Verify.AreEqual(75, numBox.Value);

                Log.Comment("Verify that pressing PageUp key increases value by 10");
                KeyboardHelper.PressKey(Key.PageUp);
                Wait.ForIdle();
                Verify.AreEqual(85, numBox.Value);
            }
        }

        [TestMethod]
        public void GamepadTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                RangeValueSpinner numBox = FindElement.ByName<RangeValueSpinner>("TestNumberBox");

                Log.Comment("Verify that pressing A validates textbox contents");
                EnterText(numBox, "8", false);
                GamepadHelper.PressButton(numBox, GamepadButton.A);
                Wait.ForIdle();
                Verify.AreEqual(8, numBox.Value);

                Log.Comment("Verify that pressing B cancels entered text");
                EnterText(numBox, "3", false);
                GamepadHelper.PressButton(numBox, GamepadButton.B);
                Wait.ForIdle();
                Verify.AreEqual(8, numBox.Value);
            }
        }

        [TestMethod]
        public void ScrollTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                RangeValueSpinner numBox = FindElement.ByName<RangeValueSpinner>("TestNumberBox");
                numBox.SetValue(0);
                Wait.ForIdle();

                Log.Comment("Verify that scroll doesn't work when the control doesn't have focus.");
                FindElement.ByName("MaxCheckBox").SetFocus();
                Wait.ForIdle();
                InputHelper.RotateWheel(numBox, 1);
                Wait.ForIdle();
                Verify.AreEqual(0, numBox.Value);

                FindTextBox(numBox).SetFocus();
                Wait.ForIdle();

                InputHelper.RotateWheel(FindTextBox(numBox), 1);
                InputHelper.RotateWheel(FindTextBox(numBox), 1);
                Wait.ForIdle();
                Verify.AreEqual(2, numBox.Value);

                InputHelper.RotateWheel(FindTextBox(numBox), -1);
                InputHelper.RotateWheel(FindTextBox(numBox), -1);
                InputHelper.RotateWheel(FindTextBox(numBox), -1);
                Wait.ForIdle();
                Verify.AreEqual(-1, numBox.Value);

                // Testing for 1705
                RangeValueSpinner numBoxInScrollViewer = FindElement.ByName<RangeValueSpinner>("NumberBoxInScroller");
                numBoxInScrollViewer.SetValue(0);
                Wait.ForIdle();
                FindTextBox(numBoxInScrollViewer).SetFocus();
                InputHelper.RotateWheel(numBoxInScrollViewer, 1);
                InputHelper.RotateWheel(numBoxInScrollViewer, 1);
                Wait.ForIdle();
                Verify.AreEqual(2, numBoxInScrollViewer.Value);

                TextBlock vertOffset = FindElement.ByName<TextBlock>("VerticalOffsetDisplayBlock");
                Verify.AreEqual("0", vertOffset.GetText());

            }
        }

        [TestMethod]
        public void CustomFormatterTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                RangeValueSpinner numBox = FindElement.ByName<RangeValueSpinner>("TestNumberBox");
                numBox.SetValue(8);
                Wait.ForIdle();
                Edit edit = FindTextBox(numBox);
                Verify.AreEqual("8", edit.GetText());

                Button button = FindElement.ByName<Button>("CustomFormatterButton");
                button.InvokeAndWait();

                Verify.AreEqual("8,00", edit.GetText());

                Log.Comment("Verify that using a formatter with a different decimal symbol works as expected.");
                EnterText(numBox, "7,45");
                Verify.AreEqual(7.45, numBox.Value);
            }
        }

        [TestMethod]
        public void ValidationDisabledTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                ComboBox validationComboBox = FindElement.ByName<ComboBox>("ValidationComboBox");
                validationComboBox.SelectItemByName("Disabled");
                Wait.ForIdle();

                Check("MinCheckBox");
                Check("MaxCheckBox");

                Log.Comment("Verify that numbers outside the range can be set if validation is disabled.");
                RangeValueSpinner numBox = FindElement.ByName<RangeValueSpinner>("TestNumberBox");
                numBox.SetValue(-10);
                Wait.ForIdle();
                Verify.AreEqual(-10, numBox.Value);

                numBox.SetValue(150);
                Wait.ForIdle();
                Verify.AreEqual(150, numBox.Value);
            }
        }

        [TestMethod]
        public void ValueChangedTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                RangeValueSpinner numBox = FindElement.ByName<RangeValueSpinner>("TestNumberBox");

                TextBlock newValueTextBlock = FindElement.ByName<TextBlock>("NewValueTextBox");
                TextBlock oldValueTextBlock = FindElement.ByName<TextBlock>("OldValueTextBox");
                TextBlock textTextBlock = FindElement.ByName<TextBlock>("TextTextBox");

                Check("MinCheckBox");
                Check("MaxCheckBox");

                Log.Comment("Verify that entering a new number fires ValueChanged event.");
                EnterText(numBox, "12");
                Verify.AreEqual("12", textTextBlock.GetText());
                Verify.AreEqual("12", newValueTextBlock.GetText());
                Verify.AreEqual("NaN",  oldValueTextBlock.GetText());

                Log.Comment("Verify that setting value through UIA fires ValueChanged event.");
                Button button = FindElement.ByName<Button>("SetValueButton");
                button.InvokeAndWait();
                Verify.AreEqual("42", textTextBlock.GetText());
                Verify.AreEqual("42", newValueTextBlock.GetText());
                Verify.AreEqual("12", oldValueTextBlock.GetText());

                Log.Comment("Verify that setting value below min gives proper values.");
                EnterText(numBox, "-5");
                Verify.AreEqual("0", textTextBlock.GetText());
                Verify.AreEqual("0", newValueTextBlock.GetText());
                Verify.AreEqual("42", oldValueTextBlock.GetText());

                Log.Comment("Verify that setting value above max gives proper values.");
                EnterText(numBox, "150");
                Verify.AreEqual("100", textTextBlock.GetText());
                Verify.AreEqual("100", newValueTextBlock.GetText());
                Verify.AreEqual("0", oldValueTextBlock.GetText());

                Log.Comment("Verify that setting text to an empty string sets value to NaN.");
                EnterText(numBox, "");
                Verify.AreEqual("", textTextBlock.GetText());
                Verify.AreEqual("NaN", newValueTextBlock.GetText());
                Verify.AreEqual("100", oldValueTextBlock.GetText());

                Log.Comment("Verify that setting two way bound value to NaN doesn't cause a crash");
                Button twoWayBindNanbutton = FindElement.ByName<Button>("SetTwoWayBoundValueNaNButton");
                twoWayBindNanbutton.InvokeAndWait();
                TextBlock twoWayBoundNumberBoxValue = FindElement.ByName<TextBlock>("TwoWayBoundNumberBoxValue");
                Verify.AreEqual("NaN", twoWayBoundNumberBoxValue.GetText());

                Log.Comment("Verify that setting value to NaN doesn't have any effect");
                Button nanbutton = FindElement.ByName<Button>("SetValueNaNButton");
                nanbutton.InvokeAndWait();
                Verify.AreEqual("", textTextBlock.GetText());
                Verify.AreEqual("NaN", newValueTextBlock.GetText());
                Verify.AreEqual("100", oldValueTextBlock.GetText());
            }
        }

        [TestMethod]
        public void BasicExpressionTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                RangeValueSpinner numBox = FindElement.ByName<RangeValueSpinner>("TestNumberBox");
                numBox.SetValue(0);

                Log.Comment("Verify that expressions don't work if AcceptsExpression is false");
                EnterText(numBox, "5 + 3");
                Verify.AreEqual(0, numBox.Value);

                Check("ExpressionCheckBox");

                int numErrors = 0;
                const double resetValue = 1234;
                
                Dictionary<string, double> expressions = new Dictionary<string, double>
                {
                    // Valid expressions. None of these should evaluate to the reset value.
                    { "5", 5 },
                    { "-358", -358 },
                    { "12.34", 12.34 },
                    { "5 + 3", 8 },
                    { "12345 + 67 + 890", 13302 },
                    { "000 + 0011", 11 },
                    { "5 - 3 + 2", 4 },
                    { "3 + 2 - 5", 0 },
                    { "9 - 2 * 6 / 4", 6 },
                    { "9 - -7",  16 },
                    { "9-3*2", 3 },         // no spaces
                    { " 10  *   6  ", 60 }, // extra spaces
                    { "10 /( 2 + 3 )", 2 },
                    { "5 * -40", -200 },
                    { "(1 - 4) / (2 + 1)", -1 },
                    { "3 * ((4 + 8) / 2)", 18 },
                    { "23 * ((0 - 48) / 8)", -138 },
                    { "((74-71)*2)^3", 216 },
                    { "2 - 2 ^ 3", -6 },
                    { "2 ^ 2 ^ 2 / 2 + 9", 17 },
                    { "5 ^ -2", 0.04 },
                    { "5.09 + 14.333", 19.423 },
                    { "2.5 * 0.35", 0.875 },
                    { "-2 - 5", -7 },       // begins with negative number
                    { "(10)", 10 },         // number in parens
                    { "(-9)", -9 },         // negative number in parens
                    { "0^0", 1 },           // who knew?

                    // These should not parse, which means they will reset back to the previous value.
                    { "5x + 3y", resetValue },        // invalid chars
                    { "5 + (3", resetValue },         // mismatched parens
                    { "9 + (2 + 3))", resetValue },
                    { "(2 + 3)(1 + 5)", resetValue }, // missing operator
                    { "9 + + 7", resetValue },        // extra operators
                    { "9 - * 7",  resetValue },
                    { "9 - - 7",  resetValue },
                    { "+9", resetValue },
                    { "1 / 0", resetValue },          // divide by zero

                    // These don't currently work, but maybe should.
                    { "-(3 + 5)", resetValue }, // negative sign in front of parens -- should be -8
                };
                foreach (KeyValuePair<string, double> pair in expressions)
                {
                    numBox.SetValue(resetValue);
                    Wait.ForIdle();

                    EnterText(numBox, pair.Key);
                    string output = "Expression '" + pair.Key + "' - expected: " + pair.Value + ", actual: " + numBox.Value;
                    if (Math.Abs(pair.Value - numBox.Value) > 0.00001)
                    {
                        numErrors++;
                        Log.Warning(output);
                    }
                    else
                    {
                        Log.Comment(output);
                    }
                }

                Verify.AreEqual(0, numErrors);
            }
        }

        [TestMethod]
        public void VerifyNumberBoxHeaderBehavior()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                var headerBeforeApplyTemplate = FindElement.ByName<TextBlock>("HeaderBeforeApplyTemplateTest");
                Verify.IsNotNull(headerBeforeApplyTemplate); 
                
                var headerTemplateBeforeApplyTemplate = FindElement.ByName<TextBlock>("HeaderTemplateBeforeApplayTemplateTest");
                Verify.IsNotNull(headerBeforeApplyTemplate);

                var toggleHeaderButton = FindElement.ByName<Button>("ToggleHeaderValueButton");
                var header = FindElement.ByName<TextBlock>("NumberBoxHeaderClippingDemoHeader");
                
                Log.Comment("Check header is null");
                Verify.IsNull(header);

                Log.Comment("Set header");
                toggleHeaderButton.Invoke();
                Wait.ForIdle();
                
                header = FindElement.ByName<TextBlock>("NumberBoxHeaderClippingDemoHeader");
                Log.Comment("Check if header is present");
                Verify.IsNotNull(header);
                Log.Comment("Remove header");
                toggleHeaderButton.Invoke();
                Wait.ForIdle();
                ElementCache.Clear();

                Log.Comment("Check that header is null again");
                header = FindElement.ByName<TextBlock>("NumberBoxHeaderClippingDemoHeader");
                Verify.IsNull(header);


                var toggleHeaderTemplateButton = FindElement.ByName<Button>("ToggleHeaderTemplateValueButton");
                var headerTemplate = FindElement.ByName<TextBlock>("HeaderTemplateTestingBlock");

                Verify.IsNull(headerTemplate);

                toggleHeaderTemplateButton.Invoke();
                Wait.ForIdle();

                headerTemplate = FindElement.ByName<TextBlock>("HeaderTemplateTestingBlock");
                Verify.IsNotNull(headerTemplate);
            }
        }


        Button FindButton(UIObject parent, string buttonName)
        {
            foreach (UIObject elem in parent.Children)
            {
                if (elem.Name.Equals(buttonName))
                {
                    return new Button(elem);
                }
            }
            Log.Comment("Did not find " + buttonName + " button for object " + parent.Name);
            return null;
        }

        Edit FindTextBox(UIObject parent)
        {
            foreach (UIObject elem in parent.Children)
            {
                if (elem.ClassName.Equals("TextBox"))
                {
                    return new Edit(elem);
                }
            }
            Log.Comment("Did not find TextBox for object " + parent.Name);
            return null;
        }

        void Check(string checkboxName)
        {
            CheckBox checkBox = FindElement.ByName<CheckBox>(checkboxName);
            checkBox.Check();
            Wait.ForIdle();
        }

        void Uncheck(string checkboxName)
        {
            CheckBox checkBox = FindElement.ByName<CheckBox>(checkboxName);
            checkBox.Uncheck();
            Wait.ForIdle();
        }

        void EnterText(RangeValueSpinner numBox, string text, bool pressEnter = true)
        {
            Edit edit = FindTextBox(numBox);
            if (edit != null)
            {
                KeyboardHelper.EnterText(edit, text);
                if (pressEnter)
                {
                    KeyboardHelper.PressKey(Key.Enter);
                }
                Wait.ForIdle();
            }
        }
    }
}
