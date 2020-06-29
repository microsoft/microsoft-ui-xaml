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

                RangeValueSpinner progressBar = FindElement.ByName<RangeValueSpinner>("TestProgressBar");
                Verify.AreEqual(0, progressBar.Value);

                double oldValue = progressBar.Value;

                // NOTE: Interaction tests can only access what accessibility tools see. In this case, we can find the button because we
                // set AutomationProperties.Name on the button in ProgressBarPage.xaml
                Button changeValueButton = FindElement.ByName<Button>("ChangeValueButton");
                changeValueButton.InvokeAndWait();

                double newValue = progressBar.Value;
                double diff = Math.Abs(oldValue - newValue);

                Log.Comment("ProgressBar value changed");
                Verify.IsGreaterThan(diff, 0.0);
            }
        }

        [TestMethod]
        public void UpdateIndicatorWidthTest()
        {
            using (var setup = new TestSetupHelper("ProgressBar Tests"))
            {
                Log.Comment("Set ProgressBar settings to default for testing");

                RangeValueSpinner progressBar = FindElement.ByName<RangeValueSpinner>("TestProgressBar");

                Edit minimumInput = FindElement.ByName<Edit>("MinimumInput");
                Edit maximumInput = FindElement.ByName<Edit>("MaximumInput");
                Edit widthInput = FindElement.ByName<Edit>("WidthInput");

                TextBlock widthInputText = FindElement.ByName<TextBlock>("WidthInputText");
                TextBlock indicatorWidthText = FindElement.ByName<TextBlock>("IndicatorWidthText");

                Button changeValueButton = FindElement.ByName<Button>("ChangeValueButton");
                Button updateWidthButton = FindElement.ByName<Button>("UpdateWidthButton");

                minimumInput.SetValue("0");
                maximumInput.SetValue("100");
                widthInput.SetValue("100");

                Verify.AreEqual(progressBar.Minimum, 0);
                Verify.AreEqual(progressBar.Maximum, 100);
                Verify.AreEqual(Convert.ToDouble(widthInputText.DocumentText), 100);

                Log.Comment("Changing value of ProgressBar updates Indicator Width");

                changeValueButton.Invoke();

                Verify.AreEqual(progressBar.Value, Convert.ToDouble(indicatorWidthText.DocumentText));

                Log.Comment("Updating width of ProgressBar also updates Indicator Width");
                
                widthInput.SetValue("200");
                updateWidthButton.InvokeAndWait();

                Verify.AreEqual((progressBar.Value * 2), Convert.ToDouble(indicatorWidthText.DocumentText), "Indicator width is adjusted to ProgressBar width");

                Log.Comment("Changing value of ProgressBar of different width updates Indicator width");

                changeValueButton.InvokeAndWait();

                Verify.AreEqual((progressBar.Value * 2), Convert.ToDouble(indicatorWidthText.DocumentText), "Indicator width is adjusted to ProgressBar width");

                Log.Comment("Updating Maximum and Minimum also updates Indicator Width");

                minimumInput.SetValue("10");
                maximumInput.SetValue("16");

                changeValueButton.InvokeAndWait();

                double range = progressBar.Maximum - progressBar.Minimum;
                double adjustedValueFromRange = progressBar.Value - progressBar.Minimum;
                double calculatedValue = (adjustedValueFromRange / range) * Convert.ToDouble(widthInputText.DocumentText);

                Verify.AreEqual(calculatedValue, Convert.ToDouble(indicatorWidthText.DocumentText), "Indicator Width is adjusted based on range and ProgressBar width");
            }
        }

        [TestMethod]
        public void UpdateMinMaxTest()
        {
            using (var setup = new TestSetupHelper("ProgressBar Tests"))
            {
                Log.Comment("Updating Minimum and Maximum value of ProgressBar");

                RangeValueSpinner progressBar = FindElement.ByName<RangeValueSpinner>("TestProgressBar");

                double oldMinimumInputText = progressBar.Minimum;
                double oldMaximumInputText = progressBar.Maximum;

                Edit minimumInput = FindElement.ByName<Edit>("MinimumInput");
                Edit maximumInput = FindElement.ByName<Edit>("MaximumInput");

                minimumInput.SetValue("10");
                maximumInput.SetValue("15");

                Button updateMinMaxButton = FindElement.ByName<Button>("UpdateMinMaxButton");
                updateMinMaxButton.InvokeAndWait();

                double newMinimumInputText = progressBar.Minimum;
                double newMaximumInputText = progressBar.Maximum;

                Verify.AreNotSame(oldMinimumInputText, newMinimumInputText, "Minimum updated");
                Verify.AreNotSame(oldMaximumInputText, newMaximumInputText, "Maximum updated");

                // Below edge cases are handled by Rangebase

                Log.Comment("Updating Minimum and Maximum when Maximum < Minimum");

                maximumInput.SetValue("5");
                updateMinMaxButton.InvokeAndWait();

                Verify.AreEqual(progressBar.Minimum, progressBar.Maximum, "Maximum updates to equal Minimum");

                Log.Comment("Updating Minimum and Maximum when Minimum > Value");

                minimumInput.SetValue("15");
                updateMinMaxButton.InvokeAndWait();

                Verify.AreEqual(progressBar.Value, progressBar.Minimum, "Value updates to equal Minimum");
                Verify.AreEqual(progressBar.Maximum, progressBar.Minimum, "Maximum also updates to equal Minimum");

                Log.Comment("Updating Minimum and Maximum to be a decimal number");

                minimumInput.SetValue("0.1");
                maximumInput.SetValue("1.1");

                updateMinMaxButton.InvokeAndWait();

                double oldValue = progressBar.Value;

                Button changeValueButton = FindElement.ByName<Button>("ChangeValueButton");
                changeValueButton.InvokeAndWait();

                double newValue = progressBar.Value;
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
                ToggleButton isIndeterminateCheckBox = FindElement.ByName<ToggleButton>("ShowIsIndeterminateCheckBox");
                TextBlock showPausedText = FindElement.ByName<TextBlock>("ShowPausedText");
                TextBlock showErrorText = FindElement.ByName<TextBlock>("ShowErrorText");
                TextBlock isIndeterminateText = FindElement.ByName<TextBlock>("ShowIsIndeterminateText");
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

                Log.Comment("ShowPaused = true && IsIndeterminate = true updates ProgressBar to IndeterminatePaused visual state"); 

                isIndeterminateCheckBox.ToggleAndWait();

                Verify.IsTrue(Convert.ToBoolean(showPausedText.DocumentText));
                Verify.IsTrue(Convert.ToBoolean(isIndeterminateText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "IndeterminatePaused");

                Log.Comment("IsIndeterminate = true updates ProgressBar to Indeterminate visual state");

                showPausedCheckBox.ToggleAndWait();

                Verify.IsTrue(Convert.ToBoolean(isIndeterminateText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "Indeterminate");

                Log.Comment("ShowError = true && IsIndeterminate = true updates ProgressBar to IndeterminateError visual state");

                showErrorCheckBox.ToggleAndWait();

                Verify.IsTrue(Convert.ToBoolean(showErrorText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "IndeterminateError");

                Log.Comment("ShowError = true updates ProgressBar to Error visual state for Determinate");

                isIndeterminateCheckBox.ToggleAndWait();

                Verify.IsFalse(Convert.ToBoolean(isIndeterminateText.DocumentText));
                Verify.IsTrue(Convert.ToBoolean(showErrorText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "Error");
            }
        }

        [TestMethod]
        public void PaddingOffsetTest()
        {
            using (var setup = new TestSetupHelper("ProgressBar Tests"))
            {
                Log.Comment("Set ProgressBar Padding settings to default for testing");

                RangeValueSpinner progressBar = FindElement.ByName<RangeValueSpinner>("TestProgressBar");

                Edit paddingLeftInput = FindElement.ByName<Edit>("PaddingLeftInput");
                Edit paddingRightInput = FindElement.ByName<Edit>("PaddingRightInput");

                TextBlock paddingLeftText = FindElement.ByName<TextBlock>("PaddingLeftText");
                TextBlock paddingRightText = FindElement.ByName<TextBlock>("PaddingRightText");
                TextBlock indicatorWidthText = FindElement.ByName<TextBlock>("IndicatorWidthText");     

                paddingLeftInput.SetValue("0");
                paddingRightInput.SetValue("0");

                Button updatePaddingButton = FindElement.ByName<Button>("UpdatePaddingButton");
                updatePaddingButton.InvokeAndWait();

                Verify.AreEqual(Convert.ToDouble(paddingLeftText.DocumentText), 0);
                Verify.AreEqual(Convert.ToDouble(paddingRightText.DocumentText), 0);

                Log.Comment("IndicatorWidth offsets where ProgressBar has Padding");

                paddingLeftInput.SetValue("10");
                paddingRightInput.SetValue("10");

                updatePaddingButton.InvokeAndWait();

                Edit valueInput = FindElement.ByName<Edit>("ValueInput");
                valueInput.SetValue("100");

                Button updateValueButton = FindElement.ByName<Button>("UpdateValueButton");
                updateValueButton.InvokeAndWait();

                double maxIndicatorWidth = progressBar.Value - Convert.ToDouble(paddingLeftText.DocumentText) - Convert.ToDouble(paddingRightText.DocumentText);

                Verify.AreEqual(maxIndicatorWidth, Convert.ToDouble(indicatorWidthText.DocumentText), "Indicator at max width is offset by Padding");
            }
        }

        [TestMethod]
        public void RetemplateUpdateIndicatorWidthTest()
        {
            using (var setup = new TestSetupHelper("ProgressBar Tests"))
            {
                Log.Comment("Navigate to Progress Bar Re-template Page");

                Button navigateToReTemplatePage = FindElement.ByName<Button>("NavigateToReTemplatePage");

                Log.Comment("Set Re-template ProgressBar settings to default for testing");

                RangeValueSpinner progressBar = FindElement.ByName<RangeValueSpinner>("TestProgressBar");

                Edit minimumInput = FindElement.ByName<Edit>("MinimumInput");
                Edit maximumInput = FindElement.ByName<Edit>("MaximumInput");
                Edit widthInput = FindElement.ByName<Edit>("WidthInput");

                TextBlock widthInputText = FindElement.ByName<TextBlock>("WidthInputText");
                TextBlock indicatorWidthText = FindElement.ByName<TextBlock>("IndicatorWidthText");

                Button changeValueButton = FindElement.ByName<Button>("ChangeValueButton");
                Button updateWidthButton = FindElement.ByName<Button>("UpdateWidthButton");

                minimumInput.SetValue("0");
                maximumInput.SetValue("100");
                widthInput.SetValue("100");

                Verify.AreEqual(progressBar.Minimum, 0);
                Verify.AreEqual(progressBar.Maximum, 100);
                Verify.AreEqual(Convert.ToDouble(widthInputText.DocumentText), 100);

                Log.Comment("Changing value of Re-template ProgressBar updates Indicator Width");

                changeValueButton.Invoke();

                Verify.AreEqual(progressBar.Value, Convert.ToDouble(indicatorWidthText.DocumentText));

                Log.Comment("Updating width of Re-template ProgressBar also updates Indicator Width");

                widthInput.SetValue("200");
                updateWidthButton.InvokeAndWait();

                Verify.AreEqual((progressBar.Value * 2), Convert.ToDouble(indicatorWidthText.DocumentText), "Indicator width is adjusted to Re-template ProgressBar width");

                Log.Comment("Changing value of ProgressBar of different width updates Indicator width");

                changeValueButton.InvokeAndWait();

                Verify.AreEqual((progressBar.Value * 2), Convert.ToDouble(indicatorWidthText.DocumentText), "Indicator width is adjusted to ProgressBar width");

                Log.Comment("Updating Maximum and Minimum also updates Indicator Width");

                minimumInput.SetValue("10");
                maximumInput.SetValue("16");

                changeValueButton.InvokeAndWait();

                double range = progressBar.Maximum - progressBar.Minimum;
                double adjustedValueFromRange = progressBar.Value - progressBar.Minimum;
                double calculatedValue = (adjustedValueFromRange / range) * Convert.ToDouble(widthInputText.DocumentText);

                Verify.AreEqual(calculatedValue, Convert.ToDouble(indicatorWidthText.DocumentText), "Indicator Width is adjusted based on range and Re-template ProgressBar width");
            }
        }

        [TestMethod]
        public void ReTemplateChangeStateTest()
        {
            using (var setup = new TestSetupHelper("ProgressBar Tests"))
            {
                Log.Comment("Navigate to Progress Bar Re-template Page");

                Button navigateToReTemplatePage = FindElement.ByName<Button>("NavigateToReTemplatePage");

                Log.Comment("Verify all properties are set to false by default for testing");

                ToggleButton showPausedCheckBox = FindElement.ByName<ToggleButton>("ShowPausedCheckBox");
                ToggleButton showErrorCheckBox = FindElement.ByName<ToggleButton>("ShowErrorCheckBox");
                ToggleButton isIndeterminateCheckBox = FindElement.ByName<ToggleButton>("ShowIsIndeterminateCheckBox");
                TextBlock showPausedText = FindElement.ByName<TextBlock>("ShowPausedText");
                TextBlock showErrorText = FindElement.ByName<TextBlock>("ShowErrorText");
                TextBlock isIndeterminateText = FindElement.ByName<TextBlock>("ShowIsIndeterminateText");
                TextBlock visualStateText = FindElement.ByName<TextBlock>("VisualStateText");

                Verify.IsFalse(Convert.ToBoolean(showPausedText.DocumentText));
                Verify.IsFalse(Convert.ToBoolean(showErrorText.DocumentText));
                Verify.IsFalse(Convert.ToBoolean(isIndeterminateText.DocumentText));

                Log.Comment("All properties to false updates Re-template ProgressBar to Determinate");

                Verify.AreEqual(visualStateText.DocumentText, "Determinate");

                Log.Comment("ShowPaused = true updates Re-template ProgressBar to Paused visual state");

                showPausedCheckBox.ToggleAndWait();

                Verify.IsTrue(Convert.ToBoolean(showPausedText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "Paused");

                Log.Comment("IsIndeterminate = true updates Re-template ProgressBar to Indeterminate visual state");

                showPausedCheckBox.ToggleAndWait();
                isIndeterminateCheckBox.ToggleAndWait();

                Verify.IsTrue(Convert.ToBoolean(isIndeterminateText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "Indeterminate");

                Log.Comment("ShowError = true updates Re-template ProgressBar to Error visual state for Determinate");

                isIndeterminateCheckBox.ToggleAndWait();
                showErrorCheckBox.ToggleAndWait();

                Verify.IsFalse(Convert.ToBoolean(isIndeterminateText.DocumentText));
                Verify.IsTrue(Convert.ToBoolean(showErrorText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "Error");
            }
        }
    }
}
