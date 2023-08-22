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
    public class RadialGradientBrushTests
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
        public void AddGradientStopTest()
        {
            using (var setup = new TestSetupHelper("RadialGradientBrush Tests"))
            {
                Log.Comment("Adding GradientStop to RadialGradientBrush");

                Button replaceGradientButton = FindElement.ByName<Button>("ReplaceGradientButton");
                replaceGradientButton.Invoke();

                TextBlock gradientCountTextBlock = FindElement.ByName<TextBlock>("GradientStopCountText");

                int oldStopCount = Convert.ToInt32(gradientCountTextBlock.DocumentText);

                Button addGradientStopButton = FindElement.ByName<Button>("AddGradientStopButton");
                addGradientStopButton.Invoke();

                int newStopCount = Convert.ToInt32(gradientCountTextBlock.DocumentText);

                Verify.IsTrue((newStopCount - oldStopCount) == 1);
            }
        }

        [TestMethod]
        public void RemoveGradientStopTest()
        {
            using (var setup = new TestSetupHelper("RadialGradientBrush Tests"))
            {
                Log.Comment("Removing GradientStop from RadialGradientBrush");

                Button replaceGradientButton = FindElement.ByName<Button>("ReplaceGradientButton");
                replaceGradientButton.Invoke();

                TextBlock gradientCountTextBlock = FindElement.ByName<TextBlock>("GradientStopCountText");

                int oldStopCount = Convert.ToInt32(gradientCountTextBlock.DocumentText);

                Button addGradientStopButton = FindElement.ByName<Button>("RemoveGradientStopButton");
                addGradientStopButton.Invoke();

                int newStopCount = Convert.ToInt32(gradientCountTextBlock.DocumentText);

                if (oldStopCount == 0)
                {
                    Verify.IsTrue(newStopCount == 0);
                }
                else
                {
                    Verify.IsTrue((oldStopCount - newStopCount) == 1);
                }
            }
        }

        [TestMethod]
        public void GradientRenderingTest()
        {
            using (var setup = new TestSetupHelper("RadialGradientBrush Tests"))
            {
                Log.Comment("Testing gradient rendering");

                var resultTextBox = new Edit(FindElement.ByName("ColorMatchTestResult"));

                using (var waiter = new ValueChangedEventWaiter(resultTextBox))
                {
                    Button generateRenderTargetBitmapButton = FindElement.ByName<Button>("GenerateRenderTargetBitmapButton");
                    generateRenderTargetBitmapButton.Invoke();

                    waiter.Wait();
                }

                Verify.AreEqual("Passed", resultTextBox.Value);
            }
        }
    }
}
