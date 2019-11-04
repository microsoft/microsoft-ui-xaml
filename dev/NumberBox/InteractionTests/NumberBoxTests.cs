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
                Verify.AreEqual(0, numBox.Value);

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

                Log.Comment("Change Step value to 5");
                RangeValueSpinner stepNumBox = FindElement.ByName<RangeValueSpinner>("StepNumberBox");
                stepNumBox.SetValue(5);
                Wait.ForIdle();

                Log.Comment("Verify that up button increases value by 5");
                upButton.InvokeAndWait();
                Verify.AreEqual(5, numBox.Value);

                Check("MinCheckBox");
                Check("MaxCheckBox");

                numBox.SetValue(98);
                Wait.ForIdle();
                Log.Comment("Verify that when wrapping is off, clicking the up button won't go past the max value.");
                upButton.InvokeAndWait();
                Verify.AreEqual(100, numBox.Value);

                Check("WrapCheckBox");

                Log.Comment("Verify that when wrapping is on, clicking the up button wraps to the min value.");
                upButton.InvokeAndWait();
                Verify.AreEqual(0, numBox.Value);

                Uncheck("WrapCheckBox");

                Log.Comment("Verify that when wrapping is off, clicking the down button won't go past the min value.");
                downButton.InvokeAndWait();
                Verify.AreEqual(0, numBox.Value);

                Check("WrapCheckBox");

                Log.Comment("Verify that when wrapping is on, clicking the down button wraps to the max value.");
                downButton.InvokeAndWait();
                Verify.AreEqual(100, numBox.Value);
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
                EnterText(FindElement.ByName<RangeValueSpinner>("MaxNumberBox"), "90");
                Verify.AreEqual(90, numBox.Value);
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

                Log.Comment("Verify that pressing escape cancels entered text");
                EnterText(numBox, "3", false);
                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();
                Verify.AreEqual(8, numBox.Value);

                Log.Comment("Verify that pressing up arrow increases the value");
                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                Verify.AreEqual(9, numBox.Value);

                Log.Comment("Verify that pressing down arrow decreases the value");
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                Verify.AreEqual(8, numBox.Value);
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

                Check("HyperScrollCheckBox");

                InputHelper.RotateWheel(numBox, 1);
                InputHelper.RotateWheel(numBox, 1);
                Wait.ForIdle();
                Verify.AreEqual(2, numBox.Value);

                InputHelper.RotateWheel(numBox, -1);
                InputHelper.RotateWheel(numBox, -1);
                InputHelper.RotateWheel(numBox, -1);
                Wait.ForIdle();
                Verify.AreEqual(-1, numBox.Value);
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

                Verify.AreEqual("8.00", edit.GetText());
            }
        }

        [TestMethod]
        public void CoersionTest()
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

        Button FindButton(UIObject parent, string buttonName)
        {
            foreach (UIObject elem in parent.Children)
            {
                if (elem.Name.Equals(buttonName))
                {
                    Log.Comment("Found " + buttonName + " button for object " + parent.Name);
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
                    Log.Comment("Found TextBox for object " + parent.Name);
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
