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

#if BUILD_WINDOWS || USING_TESTNET
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
    public class SplitButtonTests
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
        public void BasicInteractionTest()
        {
            using (var setup = new TestSetupHelper("SplitButton Tests"))
            {
                SplitButton splitButton = FindElement.ByName<SplitButton>("TestSplitButton");
                Button primaryButton = GetPrimaryButton(splitButton);
                Button secondaryButton = GetSecondaryButton(splitButton);

                TextBlock clickCountTextBlock = FindElement.ByName<TextBlock>("ClickCountTextBlock");
                TextBlock flyoutOpenedCountTextBlock = FindElement.ByName<TextBlock>("FlyoutOpenedCountTextBlock");
                TextBlock flyoutClosedCountTextBlock = FindElement.ByName<TextBlock>("FlyoutClosedCountTextBlock");
                
                Verify.AreEqual("0", clickCountTextBlock.DocumentText);
                Log.Comment("Click primary button");
                primaryButton.Click();
                Wait.ForIdle();
                Verify.AreEqual("1", clickCountTextBlock.DocumentText);
                VerifyElement.NotFound("TestFlyout", FindBy.Name);

                Verify.AreEqual("0", flyoutOpenedCountTextBlock.DocumentText);
                Log.Comment("Click secondary button");
                secondaryButton.Click();
                Wait.ForIdle();
                Verify.AreEqual("1", flyoutOpenedCountTextBlock.DocumentText);
                VerifyElement.Found("TestFlyout", FindBy.Name);

                Verify.AreEqual("0", flyoutClosedCountTextBlock.DocumentText);
                Log.Comment("Close flyout by clicking over the button");
                splitButton.Click();
                Wait.ForIdle();
                Verify.AreEqual("1", flyoutClosedCountTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void CommandTest()
        {
            using (var setup = new TestSetupHelper("SplitButton Tests"))
            {
                SplitButton splitButton = FindElement.ByName<SplitButton>("CommandSplitButton");
                Button primaryButton = GetPrimaryButton(splitButton);
                Button secondaryButton = GetSecondaryButton(splitButton);

                CheckBox canExecuteCheckBox = FindElement.ByName<CheckBox>("CanExecuteCheckBox");
                TextBlock executeCountTextBlock = FindElement.ByName<TextBlock>("ExecuteCountTextBlock");

                Log.Comment("Verify that the control starts out enabled");
                Verify.AreEqual(ToggleState.On, canExecuteCheckBox.ToggleState);
                Verify.AreEqual(true, primaryButton.IsEnabled);
                Verify.AreEqual(true, secondaryButton.IsEnabled);
                Verify.AreEqual("0", executeCountTextBlock.DocumentText);

                Log.Comment("Click primary button to execute command");
                InputHelper.MoveMouse(primaryButton, 0, 0);
                Wait.ForIdle();
                primaryButton.Click();
                Wait.ForIdle();
                Verify.AreEqual("1", executeCountTextBlock.DocumentText);
                
                Log.Comment("Verify that setting CanExecute to false disables the primary button");
                canExecuteCheckBox.Uncheck();
                Wait.ForIdle();
                Verify.AreEqual(false, primaryButton.IsEnabled);
                Verify.AreEqual(true, secondaryButton.IsEnabled);
            }
        }


        [TestMethod]
        public void TouchTest()
        {
            using (var setup = new TestSetupHelper("SplitButton Tests"))
            {
                SplitButton splitButton = FindElement.ByName<SplitButton>("TestSplitButton");
                Button primaryButton = GetPrimaryButton(splitButton);
                Button secondaryButton = GetSecondaryButton(splitButton);

                CheckBox simulateTouchCheckBox = FindElement.ByName<CheckBox>("SimulateTouchCheckBox");

                TextBlock clickCountTextBlock = FindElement.ByName<TextBlock>("ClickCountTextBlock");
                TextBlock flyoutOpenedCountTextBlock = FindElement.ByName<TextBlock>("FlyoutOpenedCountTextBlock");

                Log.Comment("Check simulate touch mode checkbox");
                simulateTouchCheckBox.Click(); // This conveniently moves the mouse over the checkbox so that it isn't over the split button yet
                Wait.ForIdle();

                Verify.AreEqual("0", clickCountTextBlock.DocumentText);
                Verify.AreEqual("0", flyoutOpenedCountTextBlock.DocumentText);

                Log.Comment("Click primary button to open flyout in touch mode");
                primaryButton.Click();
                Wait.ForIdle();
                
                Verify.AreEqual("0", clickCountTextBlock.DocumentText);
                Verify.AreEqual("1", flyoutOpenedCountTextBlock.DocumentText);

                Log.Comment("Close flyout by clicking over the button");
                splitButton.Click();
                Wait.ForIdle();
            }
        }


        [TestMethod]
        public void AccessibilityTest()
        {
            using (var setup = new TestSetupHelper("SplitButton Tests"))
            {
                SplitButton splitButton = FindElement.ByName<SplitButton>("TestSplitButton");

                TextBlock clickCountTextBlock = FindElement.ByName<TextBlock>("ClickCountTextBlock");
                TextBlock flyoutOpenedCountTextBlock = FindElement.ByName<TextBlock>("FlyoutOpenedCountTextBlock");
                TextBlock flyoutClosedCountTextBlock = FindElement.ByName<TextBlock>("FlyoutClosedCountTextBlock");

                Verify.AreEqual("0", clickCountTextBlock.DocumentText);
                Log.Comment("Verify that invoking the SplitButton causes a click");
                splitButton.InvokeAndWait();
                Verify.AreEqual("1", clickCountTextBlock.DocumentText);

                Verify.AreEqual("0", flyoutOpenedCountTextBlock.DocumentText);
                Log.Comment("Verify that expanding the SplitButton opens the flyout");
                splitButton.ExpandAndWait();
                Verify.AreEqual("1", flyoutOpenedCountTextBlock.DocumentText);

                Verify.AreEqual("0", flyoutClosedCountTextBlock.DocumentText);
                Log.Comment("Verify that collapsing the SplitButton closes the flyout");
                splitButton.CollapseAndWait();
                Verify.AreEqual("1", flyoutClosedCountTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void KeyboardTest()
        {
            using (var setup = new TestSetupHelper("SplitButton Tests"))
            {
                SplitButton splitButton = FindElement.ByName<SplitButton>("TestSplitButton");

                TextBlock clickCountTextBlock = FindElement.ByName<TextBlock>("ClickCountTextBlock");
                TextBlock flyoutOpenedCountTextBlock = FindElement.ByName<TextBlock>("FlyoutOpenedCountTextBlock");
                TextBlock flyoutClosedCountTextBlock = FindElement.ByName<TextBlock>("FlyoutClosedCountTextBlock");

                Verify.AreEqual("0", clickCountTextBlock.DocumentText);
                Log.Comment("Verify that pressing Space on SplitButton causes a click");
                splitButton.SetFocus();
                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.Space);
                Wait.ForIdle();
                Verify.AreEqual("1", clickCountTextBlock.DocumentText);

                Verify.AreEqual("0", flyoutOpenedCountTextBlock.DocumentText);
                Log.Comment("Verify that pressing alt-down on SplitButton opens the flyout");
                KeyboardHelper.PressDownModifierKey(ModifierKey.Alt);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Alt);
                Wait.ForIdle();
                Verify.AreEqual("1", flyoutOpenedCountTextBlock.DocumentText);

                Verify.AreEqual("0", flyoutClosedCountTextBlock.DocumentText);
                Log.Comment("Verify that press escape closes the flyout");
                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();
                Verify.AreEqual("1", flyoutClosedCountTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void ToggleTest()
        {
            using (var setup = new TestSetupHelper("SplitButton Tests"))
            {
                SplitButton splitButton = FindElement.ByName<SplitButton>("ToggleSplitButton");
                Button primaryButton = GetPrimaryButton(splitButton);
                Button secondaryButton = GetSecondaryButton(splitButton);

                TextBlock toggleStateTextBlock = FindElement.ByName<TextBlock>("ToggleStateTextBlock");
                TextBlock toggleStateOnClickTextBlock = FindElement.ByName<TextBlock>("ToggleStateOnClickTextBlock");

                Verify.AreEqual("Unchecked", toggleStateTextBlock.DocumentText);
                Verify.AreEqual("Unchecked", toggleStateOnClickTextBlock.DocumentText);

                Log.Comment("Click primary button to check button");
                primaryButton.Click();
                Wait.ForIdle();

                Verify.AreEqual("Checked", toggleStateTextBlock.DocumentText);
                Verify.AreEqual("Checked", toggleStateOnClickTextBlock.DocumentText);

                Log.Comment("Click primary button to uncheck button");
                primaryButton.Click();
                Wait.ForIdle();

                Verify.AreEqual("Unchecked", toggleStateTextBlock.DocumentText);
                Verify.AreEqual("Unchecked", toggleStateOnClickTextBlock.DocumentText);

                Log.Comment("Clicking secondary button should not change toggle state");
                secondaryButton.Click();
                Wait.ForIdle();

                Verify.AreEqual("Unchecked", toggleStateTextBlock.DocumentText);
                Verify.AreEqual("Unchecked", toggleStateOnClickTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void ToggleAccessibilityTest()
        {
            using (var setup = new TestSetupHelper("SplitButton Tests"))
            {
                ToggleButton toggleButton = FindElement.ByName<ToggleButton>("ToggleSplitButton");

                TextBlock toggleStateTextBlock = FindElement.ByName<TextBlock>("ToggleStateTextBlock");

                Verify.AreEqual("Unchecked", toggleStateTextBlock.DocumentText);
                Verify.AreEqual(ToggleState.Off, toggleButton.ToggleState);
                
                Log.Comment("Verify that toggling the SplitButton works");
                toggleButton.Toggle();
                Wait.ForIdle();

                Verify.AreEqual("Checked", toggleStateTextBlock.DocumentText);
                Verify.AreEqual(ToggleState.On, toggleButton.ToggleState);
            }
        }

        public Button GetPrimaryButton(SplitButton splitButton)
        {
            return new Button(FindDescendant(splitButton, "PrimaryButton"));
        }

        public Button GetSecondaryButton(SplitButton splitButton)
        {
            return new Button(FindDescendant(splitButton, "SecondaryButton"));
        }

        public UIObject FindDescendant(UIObject parent, string id)
        {
            UIObject child = null;

            foreach (UIObject o in parent.Descendants)
            {
                if (o.AutomationId == id)
                {
                    Log.Comment("Found " + id);
                    child = o;
                }
            }

            return child;
        }
    }
}
 