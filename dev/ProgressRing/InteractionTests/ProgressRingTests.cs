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
        public void ChangeStateTest()
        {
            using (var setup = new TestSetupHelper("ProgressRing Tests"))
            {
                Log.Comment("Verify IsActive property is set to true by default for testing");

                ToggleButton isActiveCheckBox = FindElement.ByName<ToggleButton>("ShowIsActiveCheckBox");

                TextBlock isActiveText = FindElement.ByName<TextBlock>("ShowIsActiveText");
                TextBlock isPlayingText = FindElement.ByName<TextBlock>("IsPlayingText");
                TextBlock visualStateText = FindElement.ByName<TextBlock>("VisualStateText");
                TextBlock opacityText = FindElement.ByName<TextBlock>("OpacityText");

                Verify.IsTrue(Convert.ToBoolean(isActiveText.DocumentText));

                Log.Comment("IsActive set to true updates ProgressRing to Active state");

                Verify.AreEqual("Active", visualStateText.DocumentText);
                Log.Comment("Verity that opacity is 1 when Active");
                Verify.AreEqual("1", opacityText.DocumentText);

                // Lottie animations only support Windows versions rs5 and above
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Log.Comment("Verify Lottie animation is playing when in Active state");

                    Verify.IsTrue(Convert.ToBoolean(isPlayingText.DocumentText));
                }

                isActiveCheckBox.ToggleAndWait();

                Log.Comment("IsActive set to false updates ProgressRing to Inactive state");
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

                Log.Comment("Verify IsActive property is set to true by default for testing");

                ToggleButton customLottieSourceIsActiveCheckBox = FindElement.ByName<ToggleButton>("CustomLottieSourceIsActiveCheckBox");

                TextBlock isActiveText = FindElement.ByName<TextBlock>("ShowIsActiveText");
                TextBlock isPlayingText = FindElement.ByName<TextBlock>("IsPlayingText");
                TextBlock visualStateText = FindElement.ByName<TextBlock>("VisualStateText");
                TextBlock opacityText = FindElement.ByName<TextBlock>("OpacityText");

                Verify.IsTrue(Convert.ToBoolean(isActiveText.DocumentText));

                Log.Comment("IsActive set to true updates ProgressRing to Active state");

                Verify.AreEqual("Active", visualStateText.DocumentText);
                Log.Comment("Verity that opacity is 1 when Active");
                Verify.AreEqual("1", opacityText.DocumentText);

                // Lottie animations only support Windows versions rs5 and above
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Log.Comment("Verify Lottie animation is playing whith custom Lottie source and in Active state");

                    Verify.IsTrue(Convert.ToBoolean(isPlayingText.DocumentText));
                }

                customLottieSourceIsActiveCheckBox.ToggleAndWait();

                Log.Comment("IsActive set to false updates ProgressRing to Inactive state");
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
    }
}
