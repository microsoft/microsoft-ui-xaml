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
    }
}
