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
using ToggleButton = Microsoft.Windows.Apps.Test.Foundation.Controls.ToggleButton;

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
         public void ChangeStateTest()
        {
            using (var setup = new TestSetupHelper("ProgressRing Tests"))
            {
                Log.Comment("Verify IsActive and IsIndeterminate property is set to true by default for testing");

                ToggleButton isActiveCheckBox = FindElement.ByName<ToggleButton>("ShowIsActiveCheckBox");
                ToggleButton isIndeterminateCheckBox = FindElement.ByName<ToggleButton>("ShowIsIndeterminateCheckBox");

                TextBlock isActiveText = FindElement.ByName<TextBlock>("ShowIsActiveText");
                TextBlock isIsIndeterminateText = FindElement.ByName<TextBlock>("ShowIsIndeterminateText");
                TextBlock isPlayingText = FindElement.ByName<TextBlock>("IsPlayingText");
                TextBlock visualStateText = FindElement.ByName<TextBlock>("VisualStateText");
                TextBlock opacityText = FindElement.ByName<TextBlock>("OpacityText");

                Verify.IsTrue(Convert.ToBoolean(isActiveText.DocumentText));
                Verify.IsTrue(Convert.ToBoolean(isIsIndeterminateText.DocumentText));

                Log.Comment("IsActive and IsIndeterminate set to true updates ProgressRing to Active state");

                Verify.AreEqual("Active", visualStateText.DocumentText);

                Log.Comment("Verity that opacity is 1 when Active");
                Verify.AreEqual("1", opacityText.DocumentText);

                // Lottie animations only support Windows versions rs5 and above
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Log.Comment("Verify Lottie animation is playing when in Active state");

                    Verify.IsTrue(Convert.ToBoolean(isPlayingText.DocumentText));
                }

                Log.Comment("IsActive set to True and IsIndeterminate set to false updates ProgressRing to DeterminateActive state");

                isIndeterminateCheckBox.ToggleAndWait();

                Verify.IsFalse(Convert.ToBoolean(isIsIndeterminateText.DocumentText));

                Verify.AreEqual("DeterminateActive", visualStateText.DocumentText);

                Log.Comment("Verify Lottie animation for Indeterminate player is not playing when in DeterminateActive state");
                Verify.IsFalse(Convert.ToBoolean(isPlayingText.DocumentText));

                Log.Comment("IsActive set to false updates ProgressRing to Inactive state");

                isActiveCheckBox.ToggleAndWait();
                
                Verify.AreEqual("Inactive", visualStateText.DocumentText);

                Log.Comment("Verify Lottie animation is not playing when in Inactive state");
                Verify.IsFalse(Convert.ToBoolean(isPlayingText.DocumentText));

                Wait.ForIdle();

                Log.Comment("Verity that opacity is 0 when Inactive");
                Verify.AreEqual("0", opacityText.DocumentText);

            }
        }

        [TestMethod]
        public void LottieCustomSourceTest()
        {
            using (var setup = new TestSetupHelper("ProgressRing Tests"))
            {
                Log.Comment("Navigate to Progress Ring Custom Lottie Source Page");

                Button navigateToCustomLottieSourcePage = FindElement.ByName<Button>("NavigateToCustomLottieSourcePage");

                navigateToCustomLottieSourcePage.InvokeAndWait();

                Log.Comment("Verify IsActive and IsIndeterminate property is set to true by default for testing");

                ToggleButton isActiveCheckBox = FindElement.ByName<ToggleButton>("ShowIsActiveCheckBox_CLS");
                ToggleButton isIndeterminateCheckBox = FindElement.ByName<ToggleButton>("ShowIsIndeterminateCheckBox_CLS");

                TextBlock isActiveText = FindElement.ByName<TextBlock>("ShowIsActiveText");
                TextBlock isIsIndeterminateText = FindElement.ByName<TextBlock>("ShowIsIndeterminateText");
                TextBlock isPlayingText = FindElement.ByName<TextBlock>("IsPlayingText");
                TextBlock visualStateText = FindElement.ByName<TextBlock>("VisualStateText");
                TextBlock opacityText = FindElement.ByName<TextBlock>("OpacityText");

                Verify.IsTrue(Convert.ToBoolean(isActiveText.DocumentText));
                Verify.IsTrue(Convert.ToBoolean(isIsIndeterminateText.DocumentText));

                Log.Comment("IsActive and IsIndeterminate set to true updates ProgressRing to Active state");

                Verify.AreEqual("Active", visualStateText.DocumentText);

                Log.Comment("Verity that opacity is 1 when Active");
                Verify.AreEqual("1", opacityText.DocumentText);

                // Lottie animations only support Windows versions rs5 and above
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Log.Comment("Verify Lottie animation is playing when in Active state");

                    Verify.IsTrue(Convert.ToBoolean(isPlayingText.DocumentText));
                }

                Log.Comment("IsActive set to True and IsIndeterminate set to false updates ProgressRing to DeterminateActive state");

                isIndeterminateCheckBox.ToggleAndWait();

                Verify.IsFalse(Convert.ToBoolean(isIsIndeterminateText.DocumentText));

                Verify.AreEqual("DeterminateActive", visualStateText.DocumentText);

                Log.Comment("Verify Lottie animation for Indeterminate player is not playing when in DeterminateActive state");
                Verify.IsFalse(Convert.ToBoolean(isPlayingText.DocumentText));

                Log.Comment("IsActive set to false updates ProgressRing to Inactive state");

                isActiveCheckBox.ToggleAndWait();

                Verify.AreEqual("Inactive", visualStateText.DocumentText);

                Log.Comment("Verify Lottie animation is not playing when in Inactive state");
                Verify.IsFalse(Convert.ToBoolean(isPlayingText.DocumentText));

                Wait.ForIdle();

                Log.Comment("Verity that opacity is 0 when Inactive");
                Verify.AreEqual("0", opacityText.DocumentText);
            }
        }

        [TestMethod]
        public void StoryboardAnimationRetemplateTest()
        {
            using (var setup = new TestSetupHelper("ProgressRing Tests"))
            {
                Log.Comment("Navigate to Progress Ring Storyboard Animation Page");

                Button navigateToStoryBoardAnimationPage = FindElement.ByName<Button>("NavigateToStoryBoardAnimationPage");

                navigateToStoryBoardAnimationPage.InvokeAndWait();

                Log.Comment("Verify IsActive property is set to true by default for testing");

                ToggleButton storyboardAnimationIsActiveCheckBox = FindElement.ByName<ToggleButton>("StoryboardAnimationIsActiveCheckBox");

                TextBlock isActiveText = FindElement.ByName<TextBlock>("ShowIsActiveText");
                TextBlock visualStateText = FindElement.ByName<TextBlock>("VisualStateText");

                Verify.IsTrue(Convert.ToBoolean(isActiveText.DocumentText));

                Log.Comment("IsActive set to true updates ProgressRing to Active state");

                Verify.AreEqual("Active", visualStateText.DocumentText);

                storyboardAnimationIsActiveCheckBox.ToggleAndWait();

                Log.Comment("IsActive set to false updates ProgressRing to Inactive state");
                Verify.AreEqual("Inactive", visualStateText.DocumentText);
            }
        }

        [TestMethod]
        public void ChangeValueTest()
        {
            using (var setup = new TestSetupHelper("ProgressRing Tests"))
            {
                Log.Comment("Changing Value of ProgressRing");

                CheckBox indeterminateCheckBox = FindElement.ByName<CheckBox>("ShowIsIndeterminateCheckBox");
                indeterminateCheckBox.Uncheck();

                RangeValueSpinner progressRing = FindElement.ByName<RangeValueSpinner>("TestProgressRing");
                Verify.AreEqual(0, progressRing.Value);

                double oldValue = progressRing.Value;

                // NOTE: Interaction tests can only access what accessibility tools see. In this case, we can find the button because we
                // set AutomationProperties.Name on the button in ProgressRingPage.xaml
                Button changeValueButton = FindElement.ByName<Button>("ChangeValueButton");
                changeValueButton.InvokeAndWait();

                double newValue = progressRing.Value;
                double diff = Math.Abs(oldValue - newValue);

                Log.Comment("ProgressRing value changed");
                Verify.IsGreaterThan(diff, 0.0);
            }
        }

        [TestMethod]
        public void UpdateMinMaxTest()
        {
            using (var setup = new TestSetupHelper("ProgressRing Tests"))
            {
                Log.Comment("Updating Minimum and Maximum value of ProgressBar");

                CheckBox indeterminateCheckBox = FindElement.ByName<CheckBox>("ShowIsIndeterminateCheckBox");
                indeterminateCheckBox.Uncheck();

                RangeValueSpinner progressRing = FindElement.ByName<RangeValueSpinner>("TestProgressRing");

                double oldMinimumInputText = progressRing.Minimum;
                double oldMaximumInputText = progressRing.Maximum;

                Edit minimumInput = FindElement.ByName<Edit>("MinimumInput");
                Edit maximumInput = FindElement.ByName<Edit>("MaximumInput");

                minimumInput.SetValue("10");
                maximumInput.SetValue("15");

                Button updateMinMaxButton = FindElement.ByName<Button>("UpdateMinMaxButton");
                updateMinMaxButton.InvokeAndWait();

                double newMinimumInputText = progressRing.Minimum;
                double newMaximumInputText = progressRing.Maximum;

                Verify.AreNotSame(oldMinimumInputText, newMinimumInputText, "Minimum updated");
                Verify.AreNotSame(oldMaximumInputText, newMaximumInputText, "Maximum updated");

                // Below edge cases are handled by Rangebase

                Log.Comment("Updating Minimum and Maximum when Maximum < Minimum");

                maximumInput.SetValue("5");
                updateMinMaxButton.InvokeAndWait();

                Verify.AreEqual(progressRing.Minimum, progressRing.Maximum, "Maximum updates to equal Minimum");

                Log.Comment("Updating Minimum and Maximum when Minimum > Value");

                minimumInput.SetValue("15");
                updateMinMaxButton.InvokeAndWait();

                Verify.AreEqual(progressRing.Value, progressRing.Minimum, "Value updates to equal Minimum");
                Verify.AreEqual(progressRing.Maximum, progressRing.Minimum, "Maximum also updates to equal Minimum");

                Log.Comment("Updating Minimum and Maximum to be a decimal number");

                minimumInput.SetValue("0.1");
                maximumInput.SetValue("1.1");

                updateMinMaxButton.InvokeAndWait();

                double oldValue = progressRing.Value;

                Button changeValueButton = FindElement.ByName<Button>("ChangeValueButton");
                changeValueButton.InvokeAndWait();

                double newValue = progressRing.Value;
                double diff = Math.Abs(oldValue - newValue);

                Verify.IsGreaterThan(diff, Convert.ToDouble(0), "Value of ProgressBar increments properly within range with decimal Minimum and Maximum");
            }
        }
    }
}
