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
                Log.Comment("Verify all properties are set to false by default for testing");

                ToggleButton isIndeterminateCheckBox = FindElement.ByName<ToggleButton>("ShowIsDeterminateCheckBox");

                TextBlock isIndeterminateText = FindElement.ByName<TextBlock>("ShowIsDeterminateText");
                TextBlock isPlayingText = FindElement.ByName<TextBlock>("IsPlayingText");
                TextBlock visualStateText = FindElement.ByName<TextBlock>("VisualStateText");

                Verify.IsFalse(Convert.ToBoolean(isIndeterminateText.DocumentText));

                Log.Comment("All properties to false updates ProgressBar to Determinate");

                Verify.AreEqual(visualStateText.DocumentText, "Determinate");

                Log.Comment("Verify Lottie animation is inactive when in Determinate state (LottieRoot is hidden)");

                Verify.IsFalse(Convert.ToBoolean(isPlayingText.DocumentText));

                Log.Comment("IsIndeterminate = true updates ProgressBar to Indeterminate visual state");

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
