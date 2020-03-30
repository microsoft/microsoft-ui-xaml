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
    public class ProgressRingTests
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
        public void ChangeValueTest()
        {
            using (var setup = new TestSetupHelper("ProgressRing Tests"))
            {
                Log.Comment("Changing Value of ProgressRing");

                UIObject testProgressRing = FindElement.ByName("TestProgressRing");
                TextBlock valueText = FindElement.ByName<TextBlock>("ValueText");

                double oldValue = Convert.ToDouble(valueText.DocumentText);

                Button changeValueButton = FindElement.ByName<Button>("ChangeValueButton");
                changeValueButton.InvokeAndWait();

                double newValue = Convert.ToDouble(valueText.DocumentText);
                double diff = Math.Abs(oldValue - newValue);

                Log.Comment("ProgressRing value changed");
                Verify.IsGreaterThan(diff, Convert.ToDouble(0));
            }
        }

        [TestMethod]
        public void UpdateMinMaxTest()
        {
            using (var setup = new TestSetupHelper("ProgressRing Tests"))
            {
                Log.Comment("Updating Minimum and Maximum value of ProgressRing");

                UIObject testProgressRing = FindElement.ByName("TestProgressRing");

                TextBlock minimumInputText = FindElement.ByName<TextBlock>("MinimumInputText");
                TextBlock maximumInputText = FindElement.ByName<TextBlock>("MaximumInputText");
                TextBlock valueText = FindElement.ByName<TextBlock>("ValueText");

                double oldMinimumInputText = Convert.ToDouble(minimumInputText.DocumentText);
                double oldMaximumInputText = Convert.ToDouble(minimumInputText.DocumentText);

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

                Verify.AreEqual(Convert.ToDouble(minimumInputText.DocumentText), Convert.ToDouble(maximumInputText.DocumentText), "Maximum updates to equal Minimum");

                Log.Comment("Updating Minimum and Maximum when Minimum > Value");

                minimumInput.SetValue("15");
                updateMinMaxButton.InvokeAndWait();

                Verify.AreEqual(Convert.ToDouble(valueText.DocumentText), Convert.ToDouble(minimumInputText.DocumentText), "Value updates to equal Minimum");
                Verify.AreEqual(Convert.ToDouble(maximumInputText.DocumentText), Convert.ToDouble(minimumInputText.DocumentText), "Maximum also updates to equal Minimum");

                Log.Comment("Updating Minimum and Maximum to be a decimal number");

                minimumInput.SetValue("0.1");
                maximumInput.SetValue("1.1");

                updateMinMaxButton.InvokeAndWait();

                double oldValue = Convert.ToDouble(valueText.DocumentText);

                Button changeValueButton = FindElement.ByName<Button>("ChangeValueButton");
                changeValueButton.InvokeAndWait();

                double newValue = Convert.ToDouble(valueText.DocumentText);
                double diff = Math.Abs(oldValue - newValue);

                Verify.IsGreaterThan(diff, Convert.ToDouble(0), "Value of ProgressRing increments properly within range with decimal Minimum and Maximum");
            }
        }

        [TestMethod]
        public void ChangeStateTest()
        {
            using (var setup = new TestSetupHelper("ProgressRing Tests"))
            {
                Log.Comment("Verify IsIndeterminate is set to True for testing");

                TextBlock isIndeterminateText = FindElement.ByName<TextBlock>("ShowIsDeterminateText");
                Verify.IsTrue(Convert.ToBoolean(isIndeterminateText.DocumentText));

                TextBlock isPlayingText = FindElement.ByName<TextBlock>("IsPlayingText");

                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Log.Comment("Verify Lottie animation is active when in Indeterminate state");

                    Verify.IsTrue(Convert.ToBoolean(isPlayingText.DocumentText));
                }

                Log.Comment("All properties to false updates ProgressRing to Determinate");

                ToggleButton isIndeterminateCheckBox = FindElement.ByName<ToggleButton>("ShowIsDeterminateCheckBox");
                TextBlock visualStateText = FindElement.ByName<TextBlock>("VisualStateText");

                isIndeterminateCheckBox.ToggleAndWait();

                Verify.IsFalse(Convert.ToBoolean(isIndeterminateText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "Determinate");

                Log.Comment("Verify Lottie animation is inactive when in Determinate state (LottieRoot is hidden)");

                Verify.IsFalse(Convert.ToBoolean(isPlayingText.DocumentText));

                Log.Comment("IsIndeterminate = true updates ProgressRing to Indeterminate visual state");

                isIndeterminateCheckBox.ToggleAndWait();

                Verify.IsTrue(Convert.ToBoolean(isIndeterminateText.DocumentText));
                Verify.AreEqual(visualStateText.DocumentText, "Indeterminate");

                // Lottie animations only support Windows versions rs5 and above
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Log.Comment("Verify Lottie animation is active when in Indeterminate state");

                    Verify.IsTrue(Convert.ToBoolean(isPlayingText.DocumentText));
                }
            }
        }
    }
}
