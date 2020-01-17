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

                UIObject testProgressBar = FindElement.ByName("TestProgressRing");
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
    }
}
