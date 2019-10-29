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
    public class ProgressBarTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void ChangeValueTest()
        {
            using (var setup = new TestSetupHelper("ProgressBar Tests"))
            {
                Log.Comment("Changing Value of ProgressBar");

                UIObject testProgressBar = FindElement.ByName("TestProgressBar");
                TextBlock valueText = FindElement.ByName<TextBlock>("ValueText");

                double oldValue = Convert.ToDouble(valueText.DocumentText);

                // NOTE: Interaction tests can only access what accessibility tools see. In this case, we can find the button because we
                // set AutomationProperties.Name on the button in ProgressBarPage.xaml
                Button changeValueButton = FindElement.ByName<Button>("ChangeValueButton");
                changeValueButton.InvokeAndWait();

                double newValue = Convert.ToDouble(valueText.DocumentText);
                double diff = Math.Abs(oldValue - newValue);

                Log.Comment("ProgressBar value changed");
                Verify.IsGreaterThan(diff, Convert.ToDouble(0));
            }
        }

        [TestMethod]
        public void UpdateIndicatorWidthTest()
        {
            using (var setup = new TestSetupHelper("ProgressBar Tests"))
            {
                Log.Comment("Set ProgressBar settings to default for testing");

                UIObject testProgressBar = FindElement.ByName("TestProgressBar");

                Edit minimumInput = FindElement.ByName<Edit>("MinimumInput");
                Edit maximumInput = FindElement.ByName<Edit>("MaximumInput");
                Edit widthInput = FindElement.ByName<Edit>("WidthInput");
                
                TextBlock minimumInputText = FindElement.ByName<TextBlock>("MinimumInputText");
                TextBlock maximumInputText = FindElement.ByName<TextBlock>("MaximumInputText");
                TextBlock widthInputText = FindElement.ByName<TextBlock>("WidthInputText");
                TextBlock valueText = FindElement.ByName<TextBlock>("ValueText");
                TextBlock indicatorWidthText = FindElement.ByName<TextBlock>("IndicatorWidthText");

                Button changeValueButton = FindElement.ByName<Button>("ChangeValueButton");
                Button updateWidthButton = FindElement.ByName<Button>("UpdateWidthButton");

                minimumInput.SetValue("0");
                maximumInput.SetValue("100");
                widthInput.SetValue("100");

                Verify.AreEqual(Convert.ToDouble(minimumInputText.DocumentText), 0);
                Verify.AreEqual(Convert.ToDouble(maximumInputText.DocumentText), 100);
                Verify.AreEqual(Convert.ToDouble(widthInputText.DocumentText), 100);

                Log.Comment("Changing value of ProgressBar updates Indicator Width");

                changeValueButton.Invoke();

                Verify.AreEqual(Convert.ToDouble(valueText.DocumentText), Convert.ToDouble(indicatorWidthText.DocumentText));

                Log.Comment("Updating width of ProgressBar also updates Indicator Width");                
                widthInput.SetValue("150");
                
                updateWidthButton.Invoke();

                Verify.AreEqual(Math.Ceiling(Convert.ToDouble(valueText.DocumentText) * 1.5), Convert.ToDouble(indicatorWidthText.DocumentText), "Indicator width is adjusted to ProgressBar width");

                Log.Comment("Changing value of ProgressBar of different width updates Indicator width");

                changeValueButton.Invoke();

                Verify.AreEqual(Math.Ceiling(Convert.ToDouble(valueText.DocumentText) * 1.5), Convert.ToDouble(indicatorWidthText.DocumentText), "Indicator width is adjusted to ProgressBar width");

                Log.Comment("Updating Maximum and Minimum also updates Indicator Width");

                minimumInput.SetValue("10");
                maximumInput.SetValue("15");

                changeValueButton.Invoke();

                double range = Convert.ToDouble(maximumInputText.DocumentText) - Convert.ToDouble(minimumInputText.DocumentText);
                double adjustedValueFromRange = Convert.ToDouble(valueText.DocumentText) - Convert.ToDouble(minimumInputText.DocumentText);
                double calculatedValue = Math.Ceiling((adjustedValueFromRange / range) * Convert.ToDouble(widthInputText.DocumentText));

                Verify.AreEqual(calculatedValue, Convert.ToDouble(indicatorWidthText.DocumentText), "Indicator Width is adjusted based on range and ProgressBar width");
            }
        }

        [TestMethod]
        public void UpdateMinMaxTest()
        {
            using (var setup = new TestSetupHelper("ProgressBar Tests"))
            {
                Log.Comment("Updating Minimum and Maximum value of ProgressBar");

                UIObject testProgressBar = FindElement.ByName("TestProgressBar");

                TextBlock valueText = FindElement.ByName<TextBlock>("ValueText");
                TextBlock minimumInputText = FindElement.ByName<TextBlock>("MinimumInputText");
                TextBlock maximumInputText = FindElement.ByName<TextBlock>("MaximumInputText");

                double oldMinimumInputText = Convert.ToDouble(minimumInputText.DocumentText);
                double oldMaximumInputText = Convert.ToDouble(maximumInputText.DocumentText);

                Edit minimumInput = FindElement.ByName<Edit>("MinimumInput");
                Edit maximumInput = FindElement.ByName<Edit>("MaximumInput");

                minimumInput.SetValue("10");
                maximumInput.SetValue("15");

                Button updateMinMaxButton = FindElement.ByName<Button>("UpdateMinMaxButton");
                updateMinMaxButton.InvokeAndWait();

                double newMinimumInputText = Convert.ToDouble(minimumInputText.DocumentText);
                double newMaximumInputText = Convert.ToDouble(maximumInputText.DocumentText);

                Verify.AreNotSame(oldMinimumInputText, newMinimumInputText, "Minimum updated");
                Verify.AreNotSame(oldMaximumInputText, newMaximumInputText, "Maximum updated");

                // Below edge cases are handled by Rangebase

                Log.Comment("Updating Minimum and Maximum when Maximum < Minimum");

                maximumInput.SetValue("5");
                updateMinMaxButton.InvokeAndWait();

                Verify.AreEqual(minimumInputText.DocumentText, maximumInputText.DocumentText, "Maximum updates to equal Minimum");

                Log.Comment("Updating Minimum and Maximum when Minimum > Value");

                minimumInput.SetValue("15");
                updateMinMaxButton.InvokeAndWait();

                Verify.AreEqual(valueText.DocumentText, minimumInputText.DocumentText, "Value updates to equal Minimum");
                Verify.AreEqual(maximumInputText.DocumentText, minimumInputText.DocumentText, "Maximum also updates to equal Minimum");

                Log.Comment("Updating Minimum and Maximum to be a decimal number");

                minimumInput.SetValue("0.1");
                maximumInput.SetValue("1.1");

                updateMinMaxButton.InvokeAndWait();

                double oldValue = Convert.ToDouble(valueText.DocumentText);

                Button changeValueButton = FindElement.ByName<Button>("ChangeValueButton");
                changeValueButton.InvokeAndWait();

                double newValue = Convert.ToDouble(valueText.DocumentText);
                double diff = Math.Abs(oldValue - newValue);

                Verify.IsGreaterThan(diff, Convert.ToDouble(0), "Value of ProgressBar increments properly within range with decimal Minimum and Maximum");
            }
        }

        [TestMethod]
        public void ChangeStateTest()
        {
            using (var setup = new TestSetupHelper("ProgressBar Tests"))
            {
                Log.Comment("Verify all properties are set to false by default for testing");

                ToggleButton showPausedCheckBox = FindElement.ByName<ToggleButton>("ShowPausedCheckBox");
                ToggleButton showErrorCheckBox = FindElement.ByName<ToggleButton>("ShowErrorCheckBox");
                ToggleButton isIndeterminateCheckBox = FindElement.ByName<ToggleButton>("ShowIsDeterminateCheckBox");
                TextBlock showPausedText = FindElement.ByName<TextBlock>("ShowPausedText");
                TextBlock showErrorText = FindElement.ByName<TextBlock>("ShowErrorText");
                TextBlock isIndeterminateText = FindElement.ByName<TextBlock>("ShowIsDeterminateText");
                TextBlock visualStateText = FindElement.ByName<TextBlock>("VisualStateText");

                Verify.IsFalse(Convert.ToBoolean(showPausedText.DocumentText));
                Verify.IsFalse(Convert.ToBoolean(showErrorText.DocumentText));
                Verify.IsFalse(Convert.ToBoolean(isIndeterminateText.DocumentText));

                Log.Comment("All properties to false updates ProgressBar to Determinate");

                Verify.AreEqual(visualStateText.DocumentText, "Determinate");

                Log.Comment("ShowPaused = true updates ProgressBar to Paused visual state");

                showPausedCheckBox.ToggleAndWait();

                Verify.IsTrue(Convert.ToBoolean(showPausedText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "Paused");

                Log.Comment("ShowPaused = true && IsIndeterminate = true updates ProgressBar to Error visual state"); // same visual treatment as Error State

                isIndeterminateCheckBox.ToggleAndWait();

                Verify.IsTrue(Convert.ToBoolean(showPausedText.DocumentText));
                Verify.IsTrue(Convert.ToBoolean(isIndeterminateText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "Error");

                Log.Comment("IsIndeterminate = true updates ProgressBar to Indeterminate visual state");

                showPausedCheckBox.ToggleAndWait();

                Verify.IsTrue(Convert.ToBoolean(isIndeterminateText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "Indeterminate");

                Log.Comment("ShowError = true updates ProgressBar to Error visual state for both Determinate and Indeterminate");

                showErrorCheckBox.ToggleAndWait();

                Verify.IsTrue(Convert.ToBoolean(showErrorText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "Error");

                isIndeterminateCheckBox.ToggleAndWait();

                Verify.IsFalse(Convert.ToBoolean(isIndeterminateText.DocumentText));
                Verify.IsTrue(Convert.ToBoolean(showErrorText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "Error");
            }
        }
    }
}
